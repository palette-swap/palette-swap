#include "ui_system.hpp"
#include "world_init.hpp"

void UISystem::restart_game()
{
	Entity player = registry.view<Player>().front();

	Entity game_hud = create_ui_group(true);
	// Player Health Bar

	create_fancy_healthbar(game_hud);

	// Inventory
	inventory_group = create_ui_group(false);
	Inventory& inventory = registry.get<Inventory>(player);
	auto inventory_size = static_cast<float>(Inventory::inventory_size);
	float small_count = floorf(sqrtf(inventory_size));
	float large_count = ceilf(inventory_size / small_count);
	for (size_t i = 0; i < Inventory::inventory_size; i++) {
		create_inventory_slot(inventory_group, i, player, large_count, small_count);
	}
	static const auto num_slots = (size_t) Slot::Count;
	for (size_t i = 0; i < num_slots; i++) {
		create_equip_slot(inventory_group, (Slot)i, player, 1, num_slots);
	}
	size_t i = 1;
	for (Entity item : inventory.inventory) {
		if (item == entt::null) {
			continue;
		}
		Entity text = create_ui_text(inventory_group, vec2(.75f / 5 * static_cast<float>(i++), .25f), registry.get<Item>(item).name);
		registry.emplace<Draggable>(text, vec2(.1f));
	}
}
