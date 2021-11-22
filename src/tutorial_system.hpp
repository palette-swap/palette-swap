#pragma once
#include "components.hpp"

class TutorialSystem {
public:

	void restart_game();

	void trigger_tooltip(TutorialTooltip tip, Entity target);
	void destroy_tooltip(TutorialTooltip tip);

private:

	Entity get_tooltip_target(TutorialTooltip tip);

	std::array<bool, (size_t)TutorialTooltip::Count> triggered;
	std::array<Entity, (size_t)TutorialTooltip::Count> tooltips;
};