#pragma once

#include "animation_system.hpp"
#include "components.hpp"
#include "story_init.hpp"
#include <deque>
#include <unordered_map>
#include <memory>

class StorySystem {

public:
	StorySystem(std::shared_ptr<AnimationSystem> animation_sys_ptr);
	bool in_cutscene();
	void on_key(int key, int action, int /*mod*/);
	void check_cutscene_invoktion();
	void step();
	//void register_animation(Entity entity, std::function<void(Entity)> animation_callback);
	void trigger_animation(CutSceneType type);
	void restart_game(const std::vector<Entity>& entities_for_cutscene);

private:
	Entity current_cutscene_entity = entt::null;
	std::deque<std::string> conversations;
	std::deque<std::string> text_frames; 
	void trigger_cutscene(CutScene& c, const vec2& trigger_pos);
	void proceed_conversation();
	bool in_cutscene_animation();
	void render_text_each_frame();
	void trigger_conversation(const vec2& trigger_pos);

	// text rendering position
	static constexpr vec2 text_init_pos = { .3, .9 };
	vec2 curr_text_pos = text_init_pos;

	// max text length for one conversation
	static constexpr uint max_line_len = 40;
	static constexpr uint max_word_in_conversation = 7;

	// position for current conversation rendering
	uint conversation_pos = 0;

	std::shared_ptr<AnimationSystem> animations;
};