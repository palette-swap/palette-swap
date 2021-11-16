#include "ui_system.hpp"

#include "ui_init.hpp"

#include "geometry.hpp"

void UISystem::on_key(int key, int action, int /*mod*/)
{
	if (action == GLFW_PRESS && key == GLFW_KEY_I) {
		UIGroup& group = registry.get<UIGroup>(groups[(size_t)Groups::Inventory]);
		group.visible = !group.visible;
		registry.get<UIGroup>(groups[(size_t)Groups::HUD]).visible = !group.visible;
	}
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
		UIGroup& group = registry.get<UIGroup>(groups[(size_t)Groups::Inventory]);
		group.visible = false;
		registry.get<UIGroup>(groups[(size_t)Groups::HUD]).visible = true;
	}

	// Change attack
	// TODO: Generalize for many attacks, check out of bounds
	// Key Codes for 1-9
	if (49 <= key && key <= 57) {
		if (key - 49 < 4) {
			size_t index = ((size_t)key) - 49;
			for (Slot slot: attacks_slots) {
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
	Inventory& inventory = registry.get<Inventory>(registry.view<Player>().front());
	Entity actual_item = (item == entt::null) ? entt::null : registry.get<Item>(item).item_template;
	if (current_attack_slot != Slot::Count && inventory.equipped.at((size_t)current_attack_slot) == actual_item) {
		set_current_attack(Slot::Count, 0);
	}
	if (InventorySlot* inv_slot = registry.try_get<InventorySlot>(container)) {
		inventory.inventory.at(inv_slot->slot) = actual_item;
	} else if (EquipSlot* equip_slot = registry.try_get<EquipSlot>(container)) {
		inventory.equipped.at((size_t)equip_slot->slot) = actual_item;
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
			group.visible = entity == button.action_target;
		}
	}
}

void UISystem::on_left_click(int action, dvec2 mouse_screen_pos)
{
	if (action == GLFW_PRESS) {
		for (auto [entity, draggable, screen_pos, interact_area] :
			 registry.view<Draggable, ScreenPosition, InteractArea>().each()) {
			if (Geometry::Rectangle(screen_pos.position, interact_area.size).contains(vec2(mouse_screen_pos))) {
				held_under_mouse = entity;
				return;
			}
		}
		for (auto [entity, element, pos, request, button] :
			 registry.view<UIElement, ScreenPosition, UIRenderRequest, Button>().each()) {
			Geometry::Rectangle button_rect = Geometry::Rectangle(
				pos.position + vec2((float)request.alignment_x, (float)request.alignment_y) * request.size * .5f,
				request.size);
			if (registry.get<UIGroup>(element.group).visible && button_rect.contains(vec2(mouse_screen_pos))) {
				do_action(button);
				return;
			}
		}
	} else if (action == GLFW_RELEASE && held_under_mouse != entt::null) {
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
}

void UISystem::on_mouse_move(vec2 mouse_screen_pos)
{
	if (held_under_mouse != entt::null) {
		registry.get<ScreenPosition>(held_under_mouse).position = mouse_screen_pos;
	}
}

bool UISystem::player_can_act() { return registry.get<UIGroup>(groups[(size_t)Groups::HUD]).visible; }

bool UISystem::has_current_attack() const {
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
	return registry.get<Weapon>(Inventory::get(registry.view<Player>().front(), current_attack_slot)).get_attack(current_attack);
}

void UISystem::add_to_inventory(Entity item, size_t slot) {
		if (item == entt::null) {
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
