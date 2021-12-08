#include "loot_system.hpp"

#include <algorithm>
#include <filesystem>

#include <rapidjson/istreamwrapper.h>
#include <rapidjson/schema.h>

void LootSystem::init(std::shared_ptr<std::default_random_engine> global_rng,
					  std::shared_ptr<TutorialSystem> tutorial_system)
{
	this->rng = std::move(global_rng);
	this->tutorials = std::move(tutorial_system);
	std::ifstream file = std::ifstream(json_schema_path("items_schema.json"));
	rapidjson::IStreamWrapper schema_wrapper(file);
	rapidjson::Document schema_doc;
	schema_doc.ParseStream(schema_wrapper);
	rapidjson::SchemaDocument schema(schema_doc);
	for (const auto& entry : std::filesystem::recursive_directory_iterator(data_path() + "/items/")) {
		file = std::ifstream(entry.path());
		rapidjson::IStreamWrapper wrapper(file);
		rapidjson::Document json_doc;
		rapidjson::SchemaValidatingReader<rapidjson::kParseDefaultFlags, rapidjson::IStreamWrapper, rapidjson::UTF8<>>
			reader(wrapper, schema);
		json_doc.Populate(reader);

		assert(reader.GetParseResult());

		assert(json_doc.IsArray());

		for (rapidjson::SizeType i = 0; i < json_doc.Size(); i++) {
			Entity item_entity = registry.create();
			ItemTemplate& item_component = registry.emplace<ItemTemplate>(item_entity, "");
			item_component.deserialize(item_entity, json_doc[i].GetObj());
			while (loot_table.size() <= item_component.tier) {
				loot_table.emplace_back();
			}
			loot_table.at(item_component.tier).push_back(item_entity);
		}

		loot_count += json_doc.Size();
	}
}

void LootSystem::restart_game()
{

	looted = 0;
	loot_misses = 0;
	looted_per_tier.clear();

	for (auto& list : loot_table) {
		looted_per_tier.push_back(0);
		std::shuffle(list.begin(), list.end(), *rng);
	}
}

bool LootSystem::try_pickup_items(Entity player)
{
	ColorState& inactive_color = registry.get<PlayerInactivePerception>(player).inactive;
	MapPosition& player_pos = registry.get<MapPosition>(player);
	for (auto [entity, resource, pos] : registry.view<ResourcePickup, MapPosition>().each()) {
		if ((inactive_color == ColorState::Red && registry.any_of<RedExclusive>(entity))
			|| (inactive_color == ColorState::Blue && registry.any_of<BlueExclusive>(entity))) {
				continue;
		}
		ivec2 distance = abs(ivec2(player_pos.position - pos.position));
		if (distance.x <= 1 && distance.y <= 1) {
			registry.get<Inventory>(player).resources.at((size_t)resource.resource)++;
			registry.destroy(entity);

			for (const auto& callback : pickup_callbacks) {
				callback(entity, MAXSIZE_T);
			}
			return true;
		}
	}
	for (auto [entity, item, pos] : registry.view<Item, MapPosition>().each()) {
		ivec2 distance = abs(ivec2(player_pos.position - pos.position));
		if (distance.x <= 1 && distance.y <= 1) {
			Inventory& inventory = registry.get<Inventory>(player);
			for (size_t i = 0; i < Inventory::inventory_size; i++) {
				if (inventory.inventory.at(i) == entt::null) {
					inventory.inventory.at(i) = item.item_template;
					registry.destroy(entity);

					for (const auto& callback : pickup_callbacks) {
						callback(inventory.inventory.at(i), i);
					}
					tutorials->trigger_tooltip(TutorialTooltip::ItemPickedUp, entt::null);
					return true;
				}
			}
		}
	}
	return false;
}

void LootSystem::drop_loot(uvec2 center_position, float mode_tier, uint count)
{
	// Initial Drop rates are as follows
	// 1-2: Nothing
	static constexpr size_t init_min = 3;
	//  3 : Mana Potion
	// 4-5: Health Potion
	// 6-9: Item Drop
	// If all items have dropped, only drop health potions as follows:
	// 1-4: Nothing
	static constexpr size_t late_min = 5;
	// 5-6: Mana Potion
	// 7-9: Health Potion

	// This is tempered by increasing the floor by the number of consecutive misses
	static constexpr std::array<ivec2, 9> next_tile = {
		ivec2(0, 0), ivec2(1, 0),	ivec2(-1, 0), ivec2(0, 1),	ivec2(0, -1),
		ivec2(1, 1), ivec2(-1, -1), ivec2(-1, 1), ivec2(1, -1),
	};
	int luck = registry.get<PlayerStats>(registry.view<Player>().front()).luck;
	// Success is luck_chance >= 0, so for luck = 10 this is 50%, 20 is 67%, etc.
	std::uniform_real_distribution<float> luck_chance(-1, static_cast<float>(luck) / 10.f);
	// Luck improve item tier
	if (luck_chance(*rng) >= 0) {
		mode_tier = min(mode_tier + 1, static_cast<float>(get_max_tier()));
	}
	// Lucky random chance for an extra drop
	if (luck_chance(*rng) >= 0) {
		count += 1;
	}

	for (uint i = 0; i < count; i++) {
		uvec2 position = uvec2(ivec2(center_position) + next_tile.at(i % 9));
		bool all_dropped = looted >= loot_count;

		// Luck sometimes guarantees a drop
		size_t lucky = luck_chance(*rng) >= 0 ? (all_dropped ? late_min : init_min) : 0; 

		std::uniform_int_distribution<size_t> drop_chance(max(1 + loot_misses, lucky), 9);
		size_t result = drop_chance(*rng);
		if (result < init_min || (all_dropped && result <= late_min)) {
			loot_misses++;
			continue;
		}
		loot_misses = 0;
		if (result <= 5 || all_dropped) {
			bool mana = (all_dropped && result <= 6) || (!all_dropped && result == 3);
			drop_resource_pickup(position, mana ? Resource::ManaPotion : Resource::HealthPotion);
			continue;
		}
		drop_item(position, mode_tier);
	}
}

void LootSystem::drop_item(uvec2 position, float mode_tier)
{
	if (looted >= loot_count) {
		// Everything has dropped
		return;
	}
	Entity template_entity = entt::null;
	if (mode_tier >= 0.f) {
		// Distribute based on requested tier
		auto distribution
			= std::binomial_distribution<size_t>(get_max_tier(), mode_tier / static_cast<float>(get_max_tier()));
		// Prevent infinite looping
		for (size_t attempts = 0; attempts < 100; attempts++) {
			const size_t tier = distribution(*rng);
			if (looted_per_tier.at(tier) < loot_table.at(tier).size()) {
				template_entity = loot_table.at(tier).at(looted_per_tier.at(tier)++);
				looted++;
				break;
			}
		}
	} else {
		// Just drop the lowest tier item available
		for (size_t tier = 0; tier <= get_max_tier(); tier++) {
			if (looted_per_tier.at(tier) < loot_table.at(tier).size()) {
				template_entity = loot_table.at(tier).at(looted_per_tier.at(tier)++);
				looted++;
				break;
			}
		}
	}
	if (template_entity == entt::null) {
		return;
	}
	drop_item(position, template_entity);
}

void LootSystem::drop_item(uvec2 position, Entity template_entity)
{
	ItemTemplate& item = registry.get<ItemTemplate>(template_entity);
	Entity loot = registry.create();
	registry.emplace<Item>(loot, template_entity);
	registry.emplace<MapPosition>(loot, position);
	registry.emplace<RenderRequest>(
		loot, TEXTURE_ASSET_ID::ICONS, EFFECT_ASSET_ID::SPRITESHEET, GEOMETRY_BUFFER_ID::SPRITE, true);
	registry.emplace<TextureOffset>(loot, item.texture_offset, item.texture_size);
	registry.emplace<Color>(loot, vec3(1));
	tutorials->trigger_tooltip(TutorialTooltip::ItemDropped, loot);
}

void LootSystem::drop_resource_pickup(uvec2 position, Resource resource) {
	// Potion
	Entity pickup = registry.create();
	registry.emplace<ResourcePickup>(pickup, resource);
	registry.emplace<MapPosition>(pickup, position);
	registry.emplace<RenderRequest>(
		pickup, TEXTURE_ASSET_ID::ICONS, EFFECT_ASSET_ID::SPRITESHEET, GEOMETRY_BUFFER_ID::SPRITE, true);
	registry.emplace<TextureOffset>(pickup, resource_textures.at((size_t)resource), vec2(32, 32));
	registry.emplace<Color>(pickup, vec3(1));
	if (resource == Resource::Key) {
		registry.emplace<BlueExclusive>(pickup);
	}
	tutorials->trigger_tooltip(TutorialTooltip::ItemDropped, pickup);
}

void LootSystem::on_pickup(const std::function<void(const Entity& item, size_t slot)>& on_pickup_callback)
{
	pickup_callbacks.push_back(on_pickup_callback);
}
