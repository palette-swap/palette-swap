#include "ui_system.hpp"

void UISystem::restart_game()
{
	// Player Health Bar
	Entity player_health = registry.create();
	registry.emplace<ScreenPosition>(player_health, vec2(.02, .02));
	registry.emplace<UIRenderRequest>(player_health,
									  TEXTURE_ASSET_ID::TEXTURE_COUNT,
									  EFFECT_ASSET_ID::FANCY_HEALTH,
									  GEOMETRY_BUFFER_ID::FANCY_HEALTH,
									  vec2(window_width_px / 4.f, window_height_px / 16.f),
									  0.f,
									  Alignment::Start,
									  Alignment::Start,
									  true);
}

void UISystem::on_key(int key, int action, int mod)
{
	if (action == GLFW_RELEASE && key == GLFW_KEY_ESCAPE) { }
}

void UISystem::on_left_click(dvec2 mouse_screen_pos) { }
