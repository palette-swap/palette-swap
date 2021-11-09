#include "ui_system.hpp"

void UISystem::on_key(int key, int action, int mod)
{
	if (action == GLFW_PRESS && key == GLFW_KEY_I) {
		registry.get<UIGroup>(inventory_group).visible = true;
	}
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
		registry.get<UIGroup>(inventory_group).visible = false;
	}
}

void UISystem::on_left_click(dvec2 mouse_screen_pos) { }
