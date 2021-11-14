#pragma once

#include "components.hpp"

class UISystem {
public:
	void restart_game();

	void on_key(int key, int action, int /*mod*/);
	void on_left_click(int action, dvec2 mouse_screen_pos);
	void on_mouse_move(vec2 mouse_screen_pos);

	bool has_current_attack() const;
	Attack& get_current_attack();

private:
	bool can_insert_into_slot(Entity item, Entity container);
	void insert_into_slot(Entity item, Entity container);
	bool swap_or_move_item(ScreenPosition& container_pos,
						   ScreenPosition& held_pos,
						   ScreenPosition& new_pos,
						   Draggable& draggable,
						   Entity new_slot_entity,
						   UISlot& new_slot);

	void set_current_attack(Slot slot, size_t attack);

	std::string make_attack_display_text();

	static constexpr std::array<Slot, 3> attacks_slots = {Slot::Weapon, Slot::Spell1, Slot::Spell2};
	
	Slot current_attack_slot = Slot::Count;
	size_t current_attack = 0;

	Entity attack_display = entt::null;
	Entity inventory_group = entt::null;
	Entity hud_group = entt::null;
	Entity held_under_mouse = entt::null;
};