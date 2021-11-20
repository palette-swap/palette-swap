#pragma once

#include "components.hpp"
#include "story_init.hpp"
#include <deque>

class StorySystem {

public:
	bool in_cutscene();
	void on_key(int key, int action, int /*mod*/);
	void check_cutscene_invoktion();
	void step();
	void restart_game(const std::vector<Entity>& entities_for_cutscene);

private:
	Entity current_cutscene_entity = entt::null;
	std::deque<std::string> conversations;
	std::deque<std::string> text_frames; 
	void trigger_cutscene(CutScene& c, const vec2& trigger_pos);
	void proceed_conversation();
	void render_text_each_frame();

	// text rendering position
	static constexpr vec2 text_init_pos = { .3, .9 };
	vec2 curr_text_pos = text_init_pos;

	// max text length for one conversation
	static constexpr uint max_line_len = 40;
	static constexpr uint max_word_in_conversation = 7;

	// position for current conversation rendering
	uint conversation_pos = 0;
};