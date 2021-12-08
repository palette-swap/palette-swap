#pragma once
#include "components.hpp"

class TutorialSystem {
public:

	void restart_game();

	bool has_triggered(TutorialTooltip tip);

	void trigger_tooltip(TutorialTooltip tip, uvec2 map_pos);
	void trigger_tooltip(TutorialTooltip tip, Entity target = entt::null);
	void destroy_tooltip(TutorialTooltip tip);

private:

	void trigger_tooltip(TutorialTooltip tip, vec2 pos);

	Entity get_tooltip_target(TutorialTooltip tip);

	std::array<bool, (size_t)TutorialTooltip::Count> triggered;
	std::array<Entity, (size_t)TutorialTooltip::Count> tooltips;
};