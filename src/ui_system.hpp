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

	Slot current_attack_slot = Slot::Count;
	size_t current_attack = 0;

	Entity inventory_group = entt::null;
	Entity held_under_mouse = entt::null;
};