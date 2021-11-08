#include "ui_system.hpp"

void UISystem::restart_game()
{
}

void UISystem::on_key(int key, int action, int mod)
{
	if (action == GLFW_RELEASE && key == GLFW_KEY_ESCAPE) { }
}

void UISystem::on_left_click(dvec2 mouse_screen_pos) { }
