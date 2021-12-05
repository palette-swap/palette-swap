#pragma once

#include "animation_system.hpp"
#include "map_generator_system.hpp"
#include "components.hpp"
#include "story_init.hpp"
#include "world_init.hpp"

#include <deque>
#include <memory>
#include <unordered_map>

class StorySystem {

public:
	explicit StorySystem(std::shared_ptr<AnimationSystem> animation_sys_ptr,
						 std::shared_ptr<MapGeneratorSystem> map_system_ptr);
	bool in_cutscene();
	void on_mouse_click(int button, int action);
	void on_key(int key, int action, int /*mod*/);
	void check_cutscene();
	void step();
	void trigger_animation(CutSceneType type);
	void load_next_level();
	void restart_game();

private:
	Entity current_cutscene_entity = entt::null;
	std::deque<std::string> conversations;
	std::deque<std::string> text_frames;
	void trigger_cutscene(CutScene& c);
	void proceed_conversation();
	void render_text_each_frame();
	void trigger_conversation();

	// max text length for one conversation
	static constexpr uint max_line_len = 80;
	static constexpr uint max_word_in_conversation = 7;

	std::shared_ptr<AnimationSystem> animations;
	std::shared_ptr<MapGeneratorSystem> map_system;

};