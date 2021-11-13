#include "ui_system.hpp"

#include "geometry.hpp"

void UISystem::on_key(int key, int action, int /*mod*/)
{
	if (action == GLFW_PRESS && key == GLFW_KEY_I) {
		registry.get<UIGroup>(inventory_group).visible = true;
	}
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
		registry.get<UIGroup>(inventory_group).visible = false;
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
		return registry.get<Item>(registry.get<UIItem>(item).actual_item).allowed_slots.at((size_t) equip_slot->slot);
	}
	return true;
}

void UISystem::insert_into_slot(Entity item, Entity container)
{
	Inventory& inventory = registry.get<Inventory>(registry.view<Player>().front());
	Entity actual_item = (item == entt::null) ? entt::null : registry.get<UIItem>(item).actual_item;
	if (current_attack_slot != Slot::Count && inventory.equipped.at((size_t)current_attack_slot) == actual_item) {
		set_current_attack(Slot::Count, 0);
	}
	if (InventorySlot* inv_slot = registry.try_get<InventorySlot>(container)) {
		inventory.inventory.at(inv_slot->slot) = actual_item;
	} else if (EquipSlot* equip_slot = registry.try_get<EquipSlot>(container)) {
		inventory.equipped.at((size_t)equip_slot->slot) = actual_item;
	}
}

void UISystem::on_left_click(int action, dvec2 mouse_screen_pos)
{
	if (action == GLFW_PRESS) {
		for (auto [entity, draggable, screen_pos, interact_area] :
			 registry.view<Draggable, ScreenPosition, InteractArea>().each()) {
			if (Geometry::Rectangle(screen_pos.position, interact_area.size).contains(vec2(mouse_screen_pos))) {
				held_under_mouse = entity;
				break;
			}
		}
	} else if (action == GLFW_RELEASE && held_under_mouse != entt::null) {
		Draggable& draggable = registry.get<Draggable>(held_under_mouse);
		ScreenPosition& container_pos = registry.get<ScreenPosition>(draggable.container);
		ScreenPosition& held_pos = registry.get<ScreenPosition>(held_under_mouse);
		bool found_new_target = false;
		Geometry::Rectangle target
			= Geometry::Rectangle(held_pos.position, registry.get<InteractArea>(held_under_mouse).size);
		for (auto [new_slot_entity, new_slot, pos, area] : registry.view<UISlot, ScreenPosition, InteractArea>().each()) {
			if (Geometry::Rectangle(pos.position, area.size).intersects(target)) {
				if (!can_insert_into_slot(held_under_mouse, new_slot_entity)) {
					continue;
				}
				if (new_slot.contents != entt::null) {
					if (!can_insert_into_slot(new_slot.contents, draggable.container)) {
						continue;
					}
					// Swap positions if already exists
					registry.get<ScreenPosition>(new_slot.contents).position = container_pos.position;
					registry.get<Draggable>(new_slot.contents).container = draggable.container;
					insert_into_slot(new_slot.contents, draggable.container);
				}
				registry.get<UISlot>(draggable.container).contents = new_slot.contents;
				held_pos.position = pos.position;
				draggable.container = new_slot_entity;
				insert_into_slot(held_under_mouse, new_slot_entity);
				new_slot.contents = held_under_mouse;
				found_new_target = true;
				registry.get<Text>(attack_display).text = make_attack_display_text();
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
	return registry.get<Weapon>(Inventory::get(registry.view<Player>().front(), current_attack_slot))
		.given_attacks.at(current_attack);
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
				Attack& attack = weapon.given_attacks.at(i);
				if (slot == current_attack_slot && i == current_attack) {
					text += "\n[" + std::to_string(++count) + "] " + attack.name;
				} else {
					text += "\n " + std::to_string(++count) + "  " + attack.name;
				}
			}
		}
	}
	return std::move(text);
}
