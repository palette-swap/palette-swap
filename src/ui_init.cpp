#include "ui_init.hpp"
#include "ui_system.hpp"

void UISystem::init(RenderSystem* render_system) { renderer = render_system; }

void UISystem::restart_game()
{
	auto ui_group_view = registry.view<UIGroup>();
	registry.destroy(ui_group_view.begin(), ui_group_view.end());

	held_under_mouse = entt::null;
	destroy_tooltip();

	groups = { create_ui_group(false), create_ui_group(false), create_ui_group(true), create_ui_group(true) };

	Entity player = registry.view<Player>().front();

	// Player Health & Mana Bars
	create_fancy_healthbar(groups[(size_t)Groups::HUD]);
	Entity mana
		= create_fancy_healthbar(groups[(size_t)Groups::HUD], vec2(.025f, .09f), vec2(.15f, .03f), BarType::Mana);
	registry.get<Color>(mana).color = vec3(.1, .1, .8);

	// Health Potion counter
	Inventory& inventory = registry.get<Inventory>(player);
	vec2 health_pos = vec2(.3f, .05125f);
	health_potion_display = create_ui_text(groups[(size_t)Groups::HUD],
										   health_pos + vec2(-.002, .01),
										   std::to_string(inventory.health_potions),
										   Alignment::Center,
										   Alignment::Center,
										   64u);
	registry.get<Color>(health_potion_display).color = vec3(.7, 1, .7);
	Entity health_pot = create_ui_icon(groups[(size_t)Groups::HUD],
									   ivec2(0, 4),
									   vec2(MapUtility::tile_size),
									   health_pos,
									   4.f * vec2(MapUtility::tile_size) / vec2(window_default_size));
	registry.get<UIRenderRequest>(health_pot).alignment_y = Alignment::Center;
	registry.emplace<Button>(health_pot, health_potion_display, ButtonAction::TryHeal, player);

	// Attack Display
	attack_display = create_ui_text(
		groups[(size_t)Groups::HUD], vec2(0, 1), make_attack_display_text(), Alignment::Start, Alignment::End);

	// Inventory button
	create_button(groups[(size_t)Groups::HUD],
				  vec2(.98, .02),
				  vec2(.1, .07),
				  vec4(.1, .1, .1, 1),
				  ButtonAction::SwitchToGroup,
				  groups[(size_t)Groups::Inventory],
				  "Inventory",
				  48u,
				  Alignment::End,
				  Alignment::Start);

	// Inventory background
	create_background(groups[(size_t)Groups::Inventory], vec2(.5, .5), vec2(1, 1), 1.f, vec4(0, 0, 0, .8));

	// Inventory
	auto inventory_size = static_cast<float>(Inventory::inventory_size);
	float small_count = floorf(sqrtf(inventory_size));
	float large_count = ceilf(inventory_size / small_count);
	for (size_t i = 0; i < Inventory::inventory_size; i++) {
		create_inventory_slot(groups[(size_t)Groups::Inventory], i, player, large_count, small_count);
		add_to_inventory(inventory.inventory.at(i), i);
	}
	static const auto num_slots = (size_t)Slot::Count;
	for (size_t i = 0; i < num_slots; i++) {
		Entity slot = create_equip_slot(groups[(size_t)Groups::Inventory], (Slot)i, player, 1, num_slots);
		Entity item = inventory.equipped.at(i);
		if (item == entt::null) {
			continue;
		}
		create_ui_item(groups[(size_t)Groups::Inventory], slot, item);
	}

	// Close Inventory button
	create_button(groups[(size_t)Groups::Inventory],
				  vec2(.02 * window_height_px / window_width_px, .02),
				  vec2(.07 * window_height_px / window_width_px, .07),
				  vec4(.1, .1, .1, 1),
				  ButtonAction::SwitchToGroup,
				  groups[(size_t)Groups::HUD],
				  "X",
				  48u,
				  Alignment::Start,
				  Alignment::Start);

	// Menu background
	create_background(groups[(size_t)Groups::MainMenu], vec2(.25, .5), vec2(.5, 1), 1.f, vec4(.6, .1, .1, 1));
	create_background(groups[(size_t)Groups::MainMenu], vec2(.75, .5), vec2(.5, 1), 1.f, vec4(.1, .1, .6, 1));

	// Menu
	Entity title = create_ui_text(
		groups[(size_t)Groups::MainMenu], vec2(.5, .1), "PALETTE SWAP", Alignment::Center, Alignment::Start, 180);
	registry.get<Text>(title).border = 24;
	create_button(groups[(size_t)Groups::MainMenu],
				  vec2(.5, .5),
				  vec2(.1, .1),
				  vec4(.1, .1, .1, 1),
				  ButtonAction::SwitchToGroup,
				  groups[(size_t)Groups::HUD],
				  "Start");
}

Entity create_ui_group(bool visible)
{
	Entity entity = registry.create();
	registry.emplace<UIGroup>(entity).visible = visible;
	return entity;
}

Entity create_fancy_healthbar(Entity ui_group, vec2 pos, vec2 size, BarType target)
{
	Entity entity = registry.create();
	registry.emplace<ScreenPosition>(entity, pos);
	registry.emplace<UIRenderRequest>(entity,
									  TEXTURE_ASSET_ID::TEXTURE_COUNT,
									  EFFECT_ASSET_ID::FANCY_HEALTH,
									  GEOMETRY_BUFFER_ID::FANCY_HEALTH,
									  size,
									  0.f,
									  Alignment::Start,
									  Alignment::Start);
	registry.emplace<Color>(entity, vec3(.8, .1, .1));
	registry.emplace<TargettedBar>(entity, target);
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

Entity create_ui_icon(Entity ui_group, ivec2 offset, vec2 texture_size, vec2 pos, vec2 size, UILayer layer)
{
	Entity ui_item = registry.create();
	registry.emplace<ScreenPosition>(ui_item, pos);
	registry.emplace<UIRenderRequest>(
		ui_item, TEXTURE_ASSET_ID::ICONS, EFFECT_ASSET_ID::SPRITESHEET, GEOMETRY_BUFFER_ID::SPRITE, size);
	registry.emplace<TextureOffset>(ui_item, offset, texture_size);
	registry.emplace<Color>(ui_item, vec3(1));

	UIGroup::add_element(ui_group, ui_item, registry.emplace<UIElement>(ui_item, ui_group, true), layer);
	return ui_item;
}

Entity create_ui_item(Entity ui_group, Entity slot, Entity item)
{
	ItemTemplate& item_component = registry.get<ItemTemplate>(item);
	Entity ui_item = create_ui_icon(ui_group,
									item_component.texture_offset,
									item_component.texture_size,
									registry.get<ScreenPosition>(slot).position,
									vec2(.1 * window_default_size.y / window_default_size.x, .1),
									UILayer::Content);
	registry.emplace<Item>(ui_item, item);
	registry.emplace<Draggable>(ui_item, slot);
	registry.emplace<InteractArea>(ui_item, vec2(.1f));
	registry.get<UISlot>(slot).contents = ui_item;

	return ui_item;
}

Entity create_ui_text(Entity ui_group,
					  vec2 screen_position,
					  const std::string_view& text,
					  Alignment alignment_x,
					  Alignment alignment_y,
					  uint16 font_size)
{
	Entity entity = registry.create();
	registry.emplace<ScreenPosition>(entity, screen_position);
	registry.emplace<Color>(entity, vec3(1.f));
	registry.emplace<Text>(entity, std::string(text), font_size, alignment_x, alignment_y);
	UIGroup::add_element(ui_group, entity, registry.emplace<UIElement>(entity, ui_group, true), UILayer::Content);
	return entity;
}

Entity create_ui_tooltip(Entity ui_group, vec2 screen_position, const std::string_view& text, uint16 font_size)
{
	Entity entity = registry.create();
	registry.emplace<ScreenPosition>(entity, screen_position);
	registry.emplace<Color>(entity, vec3(1.f));
	registry.emplace<Text>(entity, std::string(text), font_size).border = 12;
	UIGroup::add_element(
		ui_group, entity, registry.emplace<UIElement>(entity, ui_group, true), UILayer::TooltipContent);
	return entity;
}

Entity create_background(Entity ui_group, vec2 pos, vec2 size, float opacity, vec4 fill_color)
{
	Entity entity = create_ui_rectangle(entt::null, pos, size);
	registry.get<UIElement>(entity).group = ui_group;
	registry.emplace<Background>(entity);
	registry.get<Color>(entity).color = fill_color;
	registry.emplace<UIRectangle>(entity, opacity, fill_color);
	return entity;
}

Entity create_button(Entity ui_group,
					 vec2 screen_pos,
					 vec2 size,
					 vec4 fill_color,
					 ButtonAction action,
					 Entity action_target,
					 const std::string& text,
					 uint16 font_size,
					 Alignment alignment_x,
					 Alignment alignment_y)
{
	Entity entity = create_ui_rectangle(ui_group, screen_pos, size);
	UIRenderRequest& request = registry.get<UIRenderRequest>(entity);
	request.alignment_x = alignment_x;
	request.alignment_y = alignment_y;
	vec2 text_pos = screen_pos + size * vec2((float)alignment_x, (float)alignment_y) * .5f;
	Entity label = create_ui_text(ui_group, text_pos, text, Alignment::Center, Alignment::Center, font_size);
	registry.emplace<Button>(entity, label, action, action_target);
	registry.emplace<UIRectangle>(entity, 1.f, fill_color);
	return entity;
}
