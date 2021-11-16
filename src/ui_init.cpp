#include "ui_system.hpp"
#include "ui_init.hpp"

void UISystem::restart_game()
{
	auto ui_group_view = registry.view<UIGroup>();
	registry.destroy(ui_group_view.begin(), ui_group_view.end());

	Entity player = registry.view<Player>().front();

	hud_group = create_ui_group(true);

	// Player Health Bar
	create_fancy_healthbar(hud_group);

	// Attack Display
	Inventory& inventory = registry.get<Inventory>(player);
	attack_display
		= create_ui_text(hud_group, vec2(0, 1), make_attack_display_text(), Alignment::Start, Alignment::End);

	// Menus background
	menus_background = create_ui_rectangle(entt::null, vec2(.5, .5), vec2(1, 1));
	registry.emplace<Background>(menus_background);
	registry.emplace<UIRectangle>(menus_background, 1.f, vec4(0, 0, 0, .75));

	// Inventory
	inventory_group = create_ui_group(false);
	auto inventory_size = static_cast<float>(Inventory::inventory_size);
	float small_count = floorf(sqrtf(inventory_size));
	float large_count = ceilf(inventory_size / small_count);
	for (size_t i = 0; i < Inventory::inventory_size; i++) {
		Entity slot = create_inventory_slot(inventory_group, i, player, large_count, small_count);
		Entity item = inventory.inventory.at(i);
		if (item == entt::null) {
			continue;
		}
		create_ui_item(inventory_group, slot, item);
	}
	static const auto num_slots = (size_t)Slot::Count;
	for (size_t i = 0; i < num_slots; i++) {
		Entity slot = create_equip_slot(inventory_group, (Slot)i, player, 1, num_slots);
		Entity item = inventory.equipped.at(i);
		if (item == entt::null) {
			continue;
		}
		create_ui_item(inventory_group, slot, item);
	}
}

Entity create_ui_group(bool visible)
{
	Entity entity = registry.create();
	registry.emplace<UIGroup>(entity, visible);
	return entity;
}

Entity create_fancy_healthbar(Entity ui_group)
{
	Entity entity = registry.create();
	registry.emplace<ScreenPosition>(entity, vec2(.02f, .02f));
	registry.emplace<UIRenderRequest>(entity,
									  TEXTURE_ASSET_ID::TEXTURE_COUNT,
									  EFFECT_ASSET_ID::FANCY_HEALTH,
									  GEOMETRY_BUFFER_ID::FANCY_HEALTH,
									  vec2(.25f, .0625f),
									  0.f,
									  Alignment::Start,
									  Alignment::Start);
	UIGroup::add_element(ui_group, entity, registry.emplace<UIElement>(entity, ui_group, true));
	return entity;
}

Entity create_ui_rectangle(Entity ui_group, vec2 pos, vec2 size)
{
	Entity entity = registry.create();
	registry.emplace<ScreenPosition>(entity, pos);
	registry.emplace<Color>(entity, vec3(1, 1, 1));
	registry.emplace<UIRenderRequest>(entity,
									  TEXTURE_ASSET_ID::TEXTURE_COUNT,
									  EFFECT_ASSET_ID::RECTANGLE,
									  GEOMETRY_BUFFER_ID::LINE,
									  size,
									  0.f,
									  Alignment::Center,
									  Alignment::Center);
	UIGroup::add_element(ui_group, entity, registry.emplace<UIElement>(entity, ui_group, true));
	return entity;
}

Entity create_grid_rectangle(Entity ui_group, size_t slot, float width, float height, const Geometry::Rectangle& area)
{
	static constexpr float padding = .75;
	assert(width > 0 && height > 0);
	vec2 pos = area.top_left()
		+ vec2((static_cast<float>(slot % static_cast<size_t>(width)) + padding) / (width + (padding * 2.f) - 1.f),
			   (floorf(static_cast<float>(slot) / width) + padding) / (height + (padding * 2.f) - 1.f))
			* area.size;
	Entity entity = create_ui_rectangle(
		ui_group,
		pos,
		area.size * vec2(.75f / (width + (padding * 2.f) - 1.f), .75f / (height + (padding * 2.f) - 1.f)));
	return entity;
}

Entity create_inventory_slot(
	Entity ui_group, size_t slot, Entity inventory, float width, float height, const Geometry::Rectangle& area)
{
	Entity entity = create_grid_rectangle(ui_group, slot, width, height, area);
	registry.emplace<InteractArea>(entity, registry.get<UIRenderRequest>(entity).size);
	registry.emplace<UISlot>(entity, inventory);
	registry.emplace<InventorySlot>(entity, slot);
	return entity;
}

Entity create_equip_slot(
	Entity ui_group, Slot slot, Entity inventory, float width, float height, const Geometry::Rectangle& area)
{
	Entity entity = create_grid_rectangle(ui_group, (size_t)slot, width, height, area);
	UIRenderRequest& request = registry.get<UIRenderRequest>(entity);
	registry.get<Color>(entity).color = vec3(.7, .7, 1);
	registry.emplace<InteractArea>(entity, request.size);
	registry.emplace<UISlot>(entity, inventory);
	registry.emplace<EquipSlot>(entity, slot);

	create_ui_text(ui_group,
				   registry.get<ScreenPosition>(entity).position - request.size / 2.f,
				   slot_names.at((size_t)slot),
				   Alignment::Start,
				   Alignment::End);

	return entity;
}

Entity create_ui_item(Entity ui_group, Entity slot, Entity item)
{
	Entity ui_item
		= create_ui_text(ui_group, registry.get<ScreenPosition>(slot).position, registry.get<Item>(item).name);
	registry.emplace<Draggable>(ui_item, slot);
	registry.emplace<InteractArea>(ui_item, vec2(.1f));
	registry.emplace<UIItem>(ui_item, item);

	registry.get<UISlot>(slot).contents = ui_item;

	return ui_item;
}

Entity create_ui_text(
	Entity ui_group, vec2 screen_position, const std::string& text, Alignment alignment_x, Alignment alignment_y)
{
	Entity entity = registry.create();
	registry.emplace<ScreenPosition>(entity, screen_position);
	registry.emplace<Color>(entity, vec3(1.f));
	registry.emplace<Text>(entity, text, (uint16)48, alignment_x, alignment_y);
	UIGroup::add_text(ui_group, entity, registry.emplace<UIElement>(entity, ui_group, true));
	return entity;
}
