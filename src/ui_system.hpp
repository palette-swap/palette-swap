#pragma once

#include "components.hpp"

class UISystem {
public:
	void restart_game();

	void on_key(int key, int action, int mod);
	void on_left_click(dvec2 mouse_screen_pos);

private:
	Entity inventory_group;
};