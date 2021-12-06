#include "story_system.hpp"
#include <sstream>
#include <vector>

void StorySystem::restart_game()
{
	current_cutscene_entity = entt::null;
	for (auto [entity, cutscene] : registry.view<CutScene>().each()) {
		registry.destroy(cutscene.ui_entity);
		registry.destroy(entity);
	}

	this->load_next_level();
}

StorySystem::StorySystem(std::shared_ptr<AnimationSystem> animation_sys_ptr,
						 std::shared_ptr<MapGeneratorSystem> map_system_ptr)
	: animations(std::move(animation_sys_ptr))
	, map_system(std::move(map_system_ptr))
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
	uvec2 player_map_pos = registry.get<MapPosition>(player).position;
	vec2 player_world_pos = MapUtility::map_position_to_world_position(player_map_pos);

	for (auto [entity, radius_trigger] : registry.view<RadiusTrigger>().each()) {
		uvec2 trigger_map_pos = registry.get<MapPosition>(entity).position;
		vec2 trigger_pos = MapUtility::map_position_to_world_position(trigger_map_pos);
		if (length(trigger_pos - player_world_pos) <= radius_trigger.radius * MapUtility::tile_size) {
			current_cutscene_entity = entity;
			CutScene c = registry.get<CutScene>(entity);
			trigger_cutscene(c);
			registry.remove<RadiusTrigger>(entity);
		}
	}

	// TODO: check if player and trigger's entity is in the same room
	for (auto [entity] : registry.view<RoomTrigger>().each()) {
		uvec2 trigger_map_pos = registry.get<MapPosition>(entity).position;
		MapUtility::RoomID player_room_idx = map_system->current_map()
												 .at(player_map_pos.y / MapUtility::room_size)
												 .at(player_map_pos.x / MapUtility::room_size);
		MapUtility::RoomID trigger_room_idx = map_system->current_map()
												  .at(trigger_map_pos.y / MapUtility::room_size)
												  .at(trigger_map_pos.x / MapUtility::room_size);

		if (player_room_idx == trigger_room_idx) {
			current_cutscene_entity = entity;
			CutScene c = registry.get<CutScene>(entity);
			trigger_cutscene(c);
			registry.remove<RoomTrigger>(entity);
		}
	}
}

void StorySystem::step()
{
	if (!in_cutscene()) {
		return;
	}

	CutScene c = registry.get<CutScene>(current_cutscene_entity);
	if (!registry.get<UIGroup>(c.ui_entity).visible && animations->boss_intro_complete(current_cutscene_entity)) {
		if (registry.any_of<RenderRequest>(c.actual_entity)) {
			registry.get<RenderRequest>(c.actual_entity).visible = true;
		}

		if (registry.any_of<Enemy>(c.actual_entity)) {
			registry.get<Enemy>(c.actual_entity).active = true;
		}
		//registry.destroy(current_cutscene_entity);
		registry.remove<CutScene>(current_cutscene_entity);
		current_cutscene_entity = entt::null;
	}
	render_text_each_frame();
}

void StorySystem::trigger_cutscene(CutScene& c)
{
	trigger_animation(c.type);
	if (!c.texts.empty()) {
		std::stringstream ss(c.texts);
		int total_char_count = 0;
		std::string buff;
		std::string acc;
		while (getline(ss, buff, ' ')) {
			acc += buff + " ";
			total_char_count += buff.length();
			if (total_char_count >= max_word_in_conversation) {
				conversations.push_back(acc);
				acc = "";
				total_char_count = 0;
			}
		}
		conversations.push_back(acc);
		trigger_conversation();
	}
}

void StorySystem::proceed_conversation()
{
	CutScene& c = registry.get<CutScene>(current_cutscene_entity);
	Entity ui_entity = c.ui_entity;

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

void StorySystem::render_text_each_frame()
{
	if (text_frames.empty()) {
		return;
	}
	CutScene c = registry.get<CutScene>(current_cutscene_entity);
	UIGroup group = registry.get<UIGroup>(c.ui_entity);
	Entity text_entity = group.first_elements.at(static_cast<size_t>(UILayer::Content));
	Text& text_comp = registry.get<Text>(text_entity);

	std::string text_per_frame = text_frames.front();
	text_frames.pop_front();

	text_comp.text += text_per_frame;
	if (text_comp.text.length() != 0 && text_comp.text.length() % max_line_len == 0) {
		text_comp.text += "\n";
	}
}

void StorySystem::trigger_conversation()
{
	CutScene c = registry.get<CutScene>(current_cutscene_entity);
	UIGroup& ui_group = registry.get<UIGroup>(c.ui_entity);
	ui_group.visible = true;
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

// TODO: Delete after integration with map system, only for testing
void StorySystem::load_next_level()
{
	for (auto [entity, cutscene] : registry.view<CutScene>().each()) {
		registry.remove_if_exists<RoomTrigger>(entity);
		registry.remove_if_exists<RadiusTrigger>(entity);
		registry.remove<CutScene>(entity);
		current_cutscene_entity = entt::null;
		this->text_frames.clear();
		this->conversations.clear();
	}

	for (auto [entity] : registry.view<Boss>().each()) {
		Enemy enemy = registry.get<Enemy>(entity);
		if (enemy.type == EnemyType::Titho) {
			vec2 position = registry.get<MapPosition>(entity).position;
			Entity entry_entity = animations->create_boss_entry_entity(enemy.type, position);
			create_radius_cutscene(entry_entity, 10.f, CutSceneType::BossEntry, boss_cutscene_texts[(size_t)enemy.type - (size_t)EnemyType::KingMush], entity);
		}
	}

	for (auto [entity] : registry.view<Guide>().each()) {
		vec2 position = registry.get<MapPosition>(entity).position;
		std::string texts("Hello warriors! I am here to provide guidence");
		create_radius_cutscene(entity, 10.f, CutSceneType::NPCEntry, texts, entity);
	}
}
