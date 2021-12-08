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

bool StorySystem::in_cutscene()
{
	return current_cutscene_entity != entt::null && registry.valid(current_cutscene_entity);
}

void StorySystem::on_key(int key, int action, int mod)
{
	if (mod == GLFW_MOD_CONTROL && key == GLFW_KEY_ENTER) {
		auto entity = current_cutscene_entity;
		if (registry.any_of<CutScene>(entity)) {
			CutScene& c = registry.get<CutScene>(entity);
			UIGroup& group = registry.get<UIGroup>(c.ui_entity);
			group.visible = false;
			this->conversations.clear();
			this->text_frames.clear();
		}
		return;
	}

	if (action == GLFW_PRESS && key != GLFW_KEY_ESCAPE) {
		if (this->text_frames.empty()) {
			proceed_conversation();
		}
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

		const std::set<MapUtility::RoomID> & trigger_room_idxes = map_system->get_room_at_position(trigger_map_pos);

		if (trigger_room_idxes.find(player_room_idx) != trigger_room_idxes.end()) {
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

	if (registry.any_of<CutScene>(current_cutscene_entity)) {
		CutScene c = registry.get<CutScene>(current_cutscene_entity);
		if (animations->boss_intro_complete(current_cutscene_entity)) {
			if (registry.any_of<RenderRequest>(c.actual_entity)) {
				registry.get<RenderRequest>(c.actual_entity).visible = true;
			}
		}
		if (animations->boss_intro_complete(current_cutscene_entity) && this->conversations.empty()
			&& this->text_frames.empty()) {

			if (registry.any_of<Enemy>(c.actual_entity)) {
				registry.get<Enemy>(c.actual_entity).active = true;
			}
			//registry.remove<CutScene>(current_cutscene_entity);
			//current_cutscene_entity = entt::null;
		}
		render_text_each_frame();
	}
}

void StorySystem::trigger_cutscene(CutScene& c)
{
	trigger_animation(c.type);
	conversations.insert(conversations.begin(), c.texts.begin(), c.texts.end());
	trigger_conversation();
}

void StorySystem::proceed_conversation()
{
	CutScene& c = registry.get<CutScene>(current_cutscene_entity);
	Entity ui_entity = c.ui_entity;

	if (conversations.empty() && text_frames.empty()) {
		cleanup_current_cutscene();
		return;
	}

	if (conversations.empty()) {
		return;
	}

	std::string text_in_frame = conversations.front();
	conversations.pop_front();

	int length = text_in_frame.length();
	int last_index = max_line_len;

	while (last_index < length++) {
		last_index = text_in_frame.find_last_of(' ', last_index);
		text_in_frame.replace(last_index, 1, "\n");
		last_index += max_line_len + 1;
	}

	for (char c : text_in_frame) {
		text_frames.emplace_back(1, c);
	}

	UIGroup group = registry.get<UIGroup>(c.ui_entity);
	Entity text_entity = group.first_elements.at(static_cast<size_t>(UILayer::Content));
	Text& text_comp = registry.get<Text>(text_entity);
	text_comp.text = "";
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
	//if (text_comp.text.length() != 0 && text_comp.text.length() % max_line_len == 0) {
	//	text_comp.text += "\n";
	//}
}

void StorySystem::trigger_conversation()
{
	CutScene c = registry.get<CutScene>(current_cutscene_entity);
	UIGroup& ui_group = registry.get<UIGroup>(c.ui_entity);
	ui_group.visible = true;
	proceed_conversation();
}

void StorySystem::cleanup_current_cutscene()
{
	if (!in_cutscene()) {
		return;
	}
	auto entity = current_cutscene_entity;
	if (registry.any_of<CutScene>(entity)) {
		CutScene& c = registry.get<CutScene>(entity);
		UIGroup& group = registry.get<UIGroup>(c.ui_entity);
		registry.remove_if_exists<RoomTrigger>(entity);
		registry.remove_if_exists<RadiusTrigger>(entity);
		group.visible = false;
		Entity text_entity = group.first_elements.at(static_cast<size_t>(UILayer::Content));
		Text& text_comp = registry.get<Text>(text_entity);
		text_comp.text = "";
		registry.remove<CutScene>(entity);
		if (registry.any_of<RenderRequest>(c.actual_entity)) {
			registry.get<RenderRequest>(c.actual_entity).visible = true;
		}
		if (registry.any_of<Enemy>(c.actual_entity)) {
			registry.get<Enemy>(c.actual_entity).active = true;
		}
	}

	this->text_frames.clear();
	this->conversations.clear();


	
	current_cutscene_entity = entt::null;
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

	if (current_level != -1) {
		clear_level();
	}

	load_level(++current_level);
}

void StorySystem::load_last_level()
{
	if (current_level == 0) {
		fprintf(stderr, "is already on first level");
		return;
	}
	clear_level();
	load_level(--current_level);
}

void StorySystem::load_level(uint level)
{
	if (level == 0) {
		Entity player = registry.view<Player>().front();
		create_guide(registry.get<MapPosition>(player).position + uvec2(2, 2));
	}
	for (auto [entity] : registry.view<Boss>().each()) {
		Enemy enemy = registry.get<Enemy>(entity);
		if (enemy.type == EnemyType::Titho) {
			vec2 position = registry.get<MapPosition>(entity).position;
			Entity entry_entity = animations->create_boss_entry_entity(enemy.type, position);
			create_radius_cutscene(entry_entity,
								   10.f,
								   CutSceneType::BossEntry,
								   boss_cutscene_texts[(size_t)enemy.type - (size_t)EnemyType::Titho],
								   entity);
		}

		if (enemy.type == EnemyType::KingMush) {
			vec2 position = registry.get<MapPosition>(entity).position;
			Entity entry_entity = animations->create_boss_entry_entity(enemy.type, position);
			create_room_cutscene(entry_entity,
								 CutSceneType::BossEntry,
								 boss_cutscene_texts[(size_t)enemy.type - (size_t)EnemyType::KingMush],
								 entity);
		}
		if (enemy.type == EnemyType::Dragon) {
			vec2 position = registry.get<MapPosition>(entity).position;
			auto entry_entity = animations->create_boss_entry_entity(EnemyType::Dragon, position);
			create_radius_cutscene(entry_entity,
								   10.f,
								   CutSceneType::BossEntry,
								   boss_cutscene_texts[(size_t)enemy.type - (size_t)EnemyType::KingMush],
								   entity);
		}
	}

	for (auto [entity] : registry.view<Guide>().each()) {
		vec2 position = registry.get<MapPosition>(entity).position;
		create_radius_cutscene(entity, 10.f, CutSceneType::NPCEntry, help_texts, entity);
	}
}

void StorySystem::clear_level()
{

	for (auto [entity, cutscene] : registry.view<CutScene>().each()) {
		UIGroup& group = registry.get<UIGroup>(cutscene.ui_entity);
		registry.remove_if_exists<RoomTrigger>(entity);
		registry.remove_if_exists<RadiusTrigger>(entity);
		group.visible = false;
		Entity text_entity = group.first_elements.at(static_cast<size_t>(UILayer::Content));
		Text& text_comp = registry.get<Text>(text_entity);
		text_comp.text = "";
		registry.remove<CutScene>(entity);
		registry.destroy(entity);
	}

	if (current_cutscene_entity != entt::null && registry.valid(current_cutscene_entity)) {
		registry.remove_all(current_cutscene_entity);
		current_cutscene_entity = entt::null;
	}

	this->text_frames.clear();
	this->conversations.clear();

	auto guide_view = registry.view<Guide>();
	registry.destroy(guide_view.begin(), guide_view.end());
}
