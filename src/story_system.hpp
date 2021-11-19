#pragma once

#include "components.hpp"
#include "story_init.hpp"
#include <deque>

class StorySystem {

public:
	bool in_cutscene();
	void on_key(int key, int action, int /*mod*/);
	void check_cutscene_invoktion(const vec2& pos);
	void step();
	void restart_game();

private:
	Entity current_cutscene_entity = entt::null;
	std::deque<std::string> conversation;
	void trigger_cutscene(CutScene& c, const vec2& trigger_pos);
	void proceed_conversation();

	// text rendering position
	static constexpr vec2 text_init_pos = { .3, .9 };
	vec2 curr_text_pos = text_init_pos;
};