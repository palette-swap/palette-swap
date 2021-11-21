#include "ui_system.hpp"

#include "ui_init.hpp"

#include "geometry.hpp"

void UISystem::on_key(int key, int action, int /*mod*/)
{
	if (!registry.get<UIGroup>(groups[(size_t)Groups::MainMenu]).visible) {
		if (action == GLFW_PRESS && key == GLFW_KEY_I) {
			try_settle_held();
			destroy_tooltip();
			UIGroup& group = registry.get<UIGroup>(groups[(size_t)Groups::Inventory]);
			group.visible = !group.visible;
			registry.get<UIGroup>(groups[(size_t)Groups::HUD]).visible = !group.visible;
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
			try_settle_held();
			destroy_tooltip();
			UIGroup& group = registry.get<UIGroup>(groups[(size_t)Groups::Inventory]);
			group.visible = false;
			registry.get<UIGroup>(groups[(size_t)Groups::HUD]).visible = true;
		}
	}

	// Change attack
	// TODO: Generalize for many attacks, check out of bounds
	// Key Codes for 1-9
	if (49 <= key && key <= 57) {
		if (key - 49 < 4) {
			size_t index = ((size_t)key) - 49;
			for (Slot slot : attacks_slots) {
				Entity weapon_entity = Inventory::get(registry.view<Player>().front(), slot);
				if (weapon_entity != entt::null) {
					Weapon& weapon = registry.get<Weapon>(weapon_entity);
					if (index < weapon.given_attacks.size()) {
						set_current_attack(slot, index);
						break;
					}
					index -= weapon.given_attacks.size();
				}
			}
		}
	}
}

void UISystem::try_settle_held()
{
	if (held_under_mouse == entt::null) {
		return;
	}
	Draggable& draggable = registry.get<Draggable>(held_under_mouse);
	ScreenPosition& container_pos = registry.get<ScreenPosition>(draggable.container);
	ScreenPosition& held_pos = registry.get<ScreenPosition>(held_under_mouse);
	bool found_new_target = false;
	Geometry::Rectangle target
		= Geometry::Rectangle(held_pos.position, registry.get<InteractArea>(held_under_mouse).size);
	for (auto [new_slot_entity, new_slot, new_pos, area] :
		 registry.view<UISlot, ScreenPosition, InteractArea>().each()) {
		if (Geometry::Rectangle(new_pos.position, area.size).intersects(target)) {
			if (!swap_or_move_item(container_pos, held_pos, new_pos, draggable, new_slot_entity, new_slot)) {
				continue;
			}
			found_new_target = true;
			break;
		}
	}
	if (!found_new_target) {
		held_pos.position = container_pos.position;
	}
	held_under_mouse = entt::null;
}

void UISystem::destroy_tooltip()
{
	if (tooltip == entt::null) {
		return;
	}
	UIGroup::remove_element(groups[(size_t)Groups::Tooltips], tooltip, UILayer::TooltipContent);
	registry.destroy(tooltip);
	tooltip = entt::null;
}

void UISystem::align_tooltip(vec2 new_pos)
{
	if (tooltip == entt::null) {
		return;
	}

	Text& text = registry.get<Text>(tooltip);
	text.alignment_x = (new_pos.x > .5) ? Alignment::End : Alignment::Start;
	text.alignment_y = (new_pos.y > .5) ? Alignment::End : Alignment::Start;

	registry.get<ScreenPosition>(tooltip).position
		= new_pos + vec2(0, .03 * (int)text.alignment_y);
}

bool UISystem::can_insert_into_slot(Entity item, Entity container)
{
	if (EquipSlot* equip_slot = registry.try_get<EquipSlot>(container)) {
		return registry.get<ItemTemplate>(registry.get<Item>(item).item_template)
			.allowed_slots.at((size_t)equip_slot->slot);
	}
	return true;
}

void UISystem::insert_into_slot(Entity item, Entity container)
{
	Entity player = registry.view<Player>().front();
	Inventory& inventory = registry.get<Inventory>(player);
	Entity actual_item = (item == entt::null) ? entt::null : registry.get<Item>(item).item_template;
	if (current_attack_slot != Slot::Count && inventory.equipped.at((size_t)current_attack_slot) == actual_item) {
		set_current_attack(Slot::Count, 0);
	}
	if (InventorySlot* inv_slot = registry.try_get<InventorySlot>(container)) {
		inventory.inventory.at(inv_slot->slot) = actual_item;
	} else if (EquipSlot* equip_slot = registry.try_get<EquipSlot>(container)) {
		// Apply any item bonuses
		// Not sure if there's a better place to put this?
		registry.get<Stats>(player).apply(inventory.equipped.at((size_t)equip_slot->slot), false);
		inventory.equipped.at((size_t)equip_slot->slot) = actual_item;
		registry.get<Stats>(player).apply(inventory.equipped.at((size_t)equip_slot->slot), true);
	}
}

bool UISystem::swap_or_move_item(ScreenPosition& container_pos,
								 ScreenPosition& held_pos,
								 ScreenPosition& new_pos,
								 Draggable& draggable,
								 Entity new_slot_entity,
								 UISlot& new_slot)
{
	if (!can_insert_into_slot(held_under_mouse, new_slot_entity)) {
		return false;
	}
	if (new_slot.contents != entt::null) {
		if (!can_insert_into_slot(new_slot.contents, draggable.container)) {
			return false;
		}
		// Swap positions if already exists
		registry.get<ScreenPosition>(new_slot.contents).position = container_pos.position;
		registry.get<Draggable>(new_slot.contents).container = draggable.container;
	}
	insert_into_slot(new_slot.contents, draggable.container);
	registry.get<UISlot>(draggable.container).contents = new_slot.contents;
	held_pos.position = new_pos.position;
	draggable.container = new_slot_entity;
	insert_into_slot(held_under_mouse, new_slot_entity);
	new_slot.contents = held_under_mouse;
	registry.get<Text>(attack_display).text = make_attack_display_text();
	return true;
}

void UISystem::do_action(Button& button)
{
	if (button.action == ButtonAction::SwitchToGroup) {
		for (auto [entity, group] : registry.view<UIGroup>().each()) {
			group.visible = entity == button.action_target || entity == groups[(size_t)Groups::Tooltips];
		}
	}
}

void UISystem::on_left_click(int action, dvec2 mouse_screen_pos)
{
	if (action == GLFW_PRESS) {
		for (auto [entity, draggable, element, screen_pos, interact_area] :
			 registry.view<Draggable, UIElement, ScreenPosition, InteractArea>().each()) {
			if (element.visible && registry.get<UIGroup>(element.group).visible
				&& Geometry::Rectangle(screen_pos.position, interact_area.size).contains(vec2(mouse_screen_pos))) {
				held_under_mouse = entity;
				destroy_tooltip();
				return;
			}
		}
		for (auto [entity, element, pos, request, button] :
			 registry.view<UIElement, ScreenPosition, UIRenderRequest, Button>().each()) {
			Geometry::Rectangle button_rect = Geometry::Rectangle(
				pos.position + vec2((float)request.alignment_x, (float)request.alignment_y) * request.size * .5f,
				request.size);
			if (element.visible && registry.get<UIGroup>(element.group).visible
				&& button_rect.contains(vec2(mouse_screen_pos))) {
				do_action(button);
				return;
			}
		}
	} else if (action == GLFW_RELEASE && held_under_mouse != entt::null) {
		try_settle_held();
	}
}

void UISystem::on_mouse_move(vec2 mouse_screen_pos)
{
	if (held_under_mouse != entt::null) {
		registry.get<ScreenPosition>(held_under_mouse).position = mouse_screen_pos;
		return;
	}

	if (tooltip != entt::null) {
		Entity target = registry.get<Tooltip>(tooltip).target;

		if (!registry.valid(target)) {
			destroy_tooltip();
			return;
		}

		bool on_target = false;
		if (ScreenPosition* pos = registry.try_get<ScreenPosition>(target)) {
			on_target = Geometry::Rectangle(pos->position, registry.get<UIRenderRequest>(target).size)
							.contains(mouse_screen_pos);
		} else if (MapPosition* pos = registry.try_get<MapPosition>(target)) {
			on_target = MapUtility::world_position_to_map_position(
							renderer->screen_position_to_world_position(mouse_screen_pos))
				== pos->position;
		}
		if (on_target) {
			align_tooltip(mouse_screen_pos);
		} else {
			destroy_tooltip();
		}
		return;
	}

	auto create_tooltip = [&](Entity entity, Item& item, bool detailed) {
		std::string text = item.get_description(detailed);
		tooltip = create_ui_tooltip(groups[(size_t)Groups::Tooltips], mouse_screen_pos, text, 24u);
		registry.get<Text>(tooltip).bubble = true;
		registry.emplace<Tooltip>(tooltip, entity);
		align_tooltip(mouse_screen_pos);
	};

	for (auto [entity, item, pos, request, element] :
		 registry.group<Item>(entt::get<ScreenPosition, UIRenderRequest, UIElement>).each()) {
		if (element.visible && Geometry::Rectangle(pos.position, request.size).contains(mouse_screen_pos)
			&& registry.get<UIGroup>(element.group).visible) {
			create_tooltip(entity, item, true);
			return;
		}
	}
	if (player_can_act()) {
		uvec2 mouse_map_pos
			= MapUtility::world_position_to_map_position(renderer->screen_position_to_world_position(mouse_screen_pos));
		for (auto [entity, item, pos, request] : registry.view<Item, MapPosition, RenderRequest>().each()) {
			if (request.visible && pos.position == mouse_map_pos) {
				create_tooltip(entity, item, false);
				return;
			}
		}
	}
}

bool UISystem::player_can_act() { return registry.get<UIGroup>(groups[(size_t)Groups::HUD]).visible; }

bool UISystem::has_current_attack() const
{
	if (current_attack_slot == Slot::Count) {
		return false;
	}
	Entity weapon_entity = Inventory::get(registry.view<Player>().front(), current_attack_slot);
	if (weapon_entity == entt::null) {
		return false;
	}
	const auto& given_attacks = registry.get<Weapon>(weapon_entity).given_attacks;
	return current_attack < given_attacks.size();
}

Attack& UISystem::get_current_attack()
{
	return registry.get<Weapon>(Inventory::get(registry.view<Player>().front(), current_attack_slot))
		.get_attack(current_attack);
}

void UISystem::add_to_inventory(Entity item, size_t slot)
{
	if (item == entt::null) {
		return;
	}
	if (slot == MAXSIZE_T) {
		update_potion_count();
		return;
	}
	auto view = registry.view<InventorySlot>();
	auto matching_slot = std::find_if(view.begin(), view.end(), [&view, slot](Entity entity) {
		return view.get<InventorySlot>(entity).slot == slot;
	});
	if (matching_slot == view.end()) {
		return;
	}
	create_ui_item(groups[(size_t)Groups::Inventory], (*matching_slot), item);
}

void UISystem::update_potion_count()
{
	registry.get<Text>(health_potion_display).text
		= std::to_string(registry.get<Inventory>(registry.view<Player>().front()).health_potions);
}

void UISystem::set_current_attack(Slot slot, size_t attack)
{
	current_attack_slot = (Slot)slot;
	current_attack = attack;
	registry.get<Text>(attack_display).text = make_attack_display_text();
}

std::string UISystem::make_attack_display_text()
{
	std::string text;
	size_t count = 0;
	for (Slot slot : attacks_slots) {
		Entity weapon_entity = Inventory::get(registry.view<Player>().front(), slot);
		if (weapon_entity != entt::null) {
			Weapon& weapon = registry.get<Weapon>(weapon_entity);
			for (size_t i = 0; i < weapon.given_attacks.size(); i++) {
				Attack& attack = weapon.get_attack(i);
				if (slot == current_attack_slot && i == current_attack) {
					text += "\n[" + std::to_string(++count) + "] " + attack.name;
				} else {
					text += "\n " + std::to_string(++count) + "  " + attack.name;
				}
			}
		}
	}
	return text;
}
