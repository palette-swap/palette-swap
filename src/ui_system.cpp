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
		for (auto [entity, slot, pos, area] : registry.view<UISlot, ScreenPosition, InteractArea>().each()) {
			if (Geometry::Rectangle(pos.position, area.size).intersects(target)) {
				if (slot.contents != entt::null) {
					// Swap positions if already exists
					registry.get<ScreenPosition>(slot.contents).position = container_pos.position;
					registry.get<Draggable>(slot.contents).container = draggable.container;
				}
				registry.get<UISlot>(draggable.container).contents = slot.contents;
				held_pos.position = pos.position;
				draggable.container = entity;
				slot.contents = held_under_mouse;
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
