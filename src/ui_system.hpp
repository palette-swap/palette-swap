#pragma once

#include "components.hpp"

class UISystem {
public:
	void restart_game();

	void on_key(int key, int action, int /*mod*/);
	void on_left_click(int action, dvec2 mouse_screen_pos);
	void on_mouse_move(vec2 mouse_screen_pos);

private:
	Entity inventory_group = entt::null;
	Entity held_under_mouse = entt::null;
};