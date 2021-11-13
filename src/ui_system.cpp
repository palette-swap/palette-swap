#include "ui_system.hpp"

#include "geometry.hpp"

void UISystem::on_key(int key, int action, int /*mod*/ )
{
	if (action == GLFW_PRESS && key == GLFW_KEY_I) {
		registry.get<UIGroup>(inventory_group).visible = true;
	}
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
		registry.get<UIGroup>(inventory_group).visible = false;
	}
}

void UISystem::on_left_click(int action, dvec2 mouse_screen_pos) {
	if (action == GLFW_PRESS) {
		for (auto [entity, draggable, screen_pos] : registry.group<Draggable>(entt::get<ScreenPosition>).each()) {
			if (Geometry::Rectangle(screen_pos.position, draggable.size).contains(vec2(mouse_screen_pos))) {
				held_under_mouse = entity;
				break;
			}
		}
	} else if (action == GLFW_RELEASE) {
		held_under_mouse = entt::null;
	}
}

void UISystem::on_mouse_move(vec2 mouse_screen_pos) {
	if (held_under_mouse != entt::null) {
		registry.get<ScreenPosition>(held_under_mouse).position = mouse_screen_pos;
	}
}
