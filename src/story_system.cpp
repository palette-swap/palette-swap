#include "story_system.hpp"
#include <sstream>
#include <vector>

void StorySystem::restart_game()
{
	current_cutscene_entity = entt::null;
	boss_created = false;
	for (auto [entity, cutscene] : registry.view<CutScene>().each()) {
		registry.destroy(cutscene.cutscene_ui);
		registry.destroy(entity);
	}
}

StorySystem::StorySystem(std::shared_ptr<AnimationSystem> animation_sys_ptr)
	: animations(std::move(animation_sys_ptr))
{
}

bool StorySystem::in_cutscene() { return current_cutscene_entity != entt::null; }

void StorySystem::on_mouse_click(int /*button*/, int action)
{
	if (action == GLFW_PRESS) {
		proceed_conversation();
	}
}

void StorySystem::on_key(int /*key*/, int action, int /*mod*/)
{
	// TODO: handle story on key: basically press any key will make the conversation proceed
	if (action == GLFW_PRESS) {
		proceed_conversation();
	}
}

void StorySystem::check_cutscene()
{
	Entity player = registry.view<Player>().front();
	vec2 player_pos = MapUtility::map_position_to_world_position(registry.get<MapPosition>(player).position);
	for (auto [entity, cutscene] : registry.view<CutScene>().each()) {
		vec2 trigger_pos = MapUtility::map_position_to_world_position(registry.get<MapPosition>(entity).position);
		if (length(trigger_pos - player_pos) <= cutscene.radius) {
			current_cutscene_entity = entity;
			trigger_cutscene(cutscene, trigger_pos);
		}
	}
}

void StorySystem::step()
{
	if (!in_cutscene()) {
		return;
	}

	if (animations->boss_intro_complete(current_cutscene_entity) && !boss_created) {
		Entity actual_entity = registry.get<CutScene>(current_cutscene_entity).actual_entity;
		registry.get<RenderRequest>(actual_entity).visible = true;
		boss_created = true;
	}
	CutScene c = registry.get<CutScene>(current_cutscene_entity);
	if (!registry.get<UIGroup>(c.cutscene_ui).visible && animations->boss_intro_complete(current_cutscene_entity)) {
		registry.destroy(current_cutscene_entity);
		current_cutscene_entity = entt::null;
		boss_created = false;
	}
	render_text_each_frame();
}

void StorySystem::trigger_cutscene(CutScene& c, const vec2& trigger_pos)
{

	std::stringstream ss(c.texts);
	int word_count = 0;
	std::string buff;
	std::string acc;
	while (getline(ss, buff, ' ')) {
		acc += buff + " ";
		if (++word_count >= max_word_in_conversation) {
			conversations.push_back(acc);
			acc = "";
			word_count = 0;
		}
	}
	conversations.push_back(acc);
	trigger_animation(c.type);
	trigger_conversation(trigger_pos);
}

void StorySystem::proceed_conversation()
{
	CutScene& c = registry.get<CutScene>(current_cutscene_entity);
	Entity ui_entity = c.cutscene_ui;

	if (conversations.empty() && text_frames.empty()) {
		registry.get<UIGroup>(ui_entity).visible = false;
		return;
	}

	if (conversations.empty()) {
		return;
	}

	std::string text_in_frame = conversations.front();
	conversations.pop_front();

	for (char c : text_in_frame) {
		text_frames.emplace_back(1, c);
	}
}

bool StorySystem::in_cutscene_animation()
{
	assert(current_cutscene_entity != entt::null);
	if (registry.any_of<Animation>(current_cutscene_entity)) {
		Animation a = registry.get<Animation>(current_cutscene_entity);
		return a.frame <= a.max_frames - 2;
	}
	return false;
}

void StorySystem::render_text_each_frame()
{
	if (text_frames.empty()) {
		return;
	}
	CutScene c = registry.get<CutScene>(current_cutscene_entity);
	UIGroup group = registry.get<UIGroup>(c.cutscene_ui);
	Entity text_entity = group.first_elements.at(static_cast<size_t>(UILayer::Content));
	Text& text_comp = registry.get<Text>(text_entity);

	std::string text_per_frame = text_frames.front();
	text_frames.pop_front();

	text_comp.text += text_per_frame;
	if (text_comp.text.length() != 0 && text_comp.text.length() % max_line_len == 0) {
		text_comp.text += "\n";
	}
}

void StorySystem::trigger_conversation(const vec2& trigger_pos)
{
	CutScene c = registry.get<CutScene>(current_cutscene_entity);
	Entity camera = registry.view<Camera>().front();
	WorldPosition& camera_world_pos = registry.get<WorldPosition>(camera);
	UIGroup& ui_group = registry.get<UIGroup>(c.cutscene_ui);
	ui_group.visible = true;
	camera_world_pos.position = trigger_pos;
	proceed_conversation();
}

void StorySystem::trigger_animation(CutSceneType type)
{
	assert(current_cutscene_entity != entt::null);
	switch (type) {
	case CutSceneType::BossEntry:
		animations->trigger_full_boss_intro(current_cutscene_entity);
		break;
	default:
		break;
	}
}

void StorySystem::load_next_level() { 
	for (auto [entity, enemy] : registry.view<Enemy>().each()) {
		if (enemy.type == EnemyType::KingMush) {
			vec2 position = registry.get<MapPosition>(entity).position;
			auto entry_entity = animations->create_boss_entry_entity(EnemyType::KingMush, position);
			std::string texts("sssssssssssssssssssssssssssssssssssssssssssss");
			create_cutscene(entry_entity, entity, CutSceneType::BossEntry, 350, texts);
			// printf("Number of conversation characters: %d", texts.length());
		}
	}
}
