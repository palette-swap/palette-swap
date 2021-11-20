#include "story_system.hpp"
#include <vector>

void StorySystem::restart_game()
{
	current_cutscene_entity = entt::null;
	for (auto [entity, cutscene] : registry.view<CutScene>().each()) {
		registry.remove_all(cutscene.cutscene_ui);
		registry.remove_all(entity);
	}
	// register cutscene for the boss
	for (auto [entity, enemy] : registry.view<Enemy>().each()) {
		if (enemy.type == EnemyType::KingMush) {
			// create cutscene for KingMush
			create_cutscene(entity, 256, std::vector<std::string>{"This is the boss.", "Beat it!"});
		}
	}
}

bool StorySystem::in_cutscene() { return current_cutscene_entity != entt::null; }

void StorySystem::on_key(int key, int action, int)
{
	// TODO: handle story on key: basically press any key will make the conversation proceed
	if (action == GLFW_PRESS) {
		proceed_conversation();
	}
}

void StorySystem::check_cutscene_invoktion(const vec2& pos)
{
	for (auto [entity, cutscene] : registry.view<CutScene>().each()) {
		vec2 trigger_pos = MapUtility::map_position_to_world_position(registry.get<MapPosition>(entity).position);
		if (length(trigger_pos - pos) <= cutscene.radius) {
			current_cutscene_entity = entity;
			trigger_cutscene(cutscene, trigger_pos);
		}
	}
}

void StorySystem::step()
{
	Entity player = registry.view<Player>().front();
	if (current_cutscene_entity == entt::null) {
		check_cutscene_invoktion(
			MapUtility::map_position_to_world_position(registry.get<MapPosition>(player).position));
	}
}

void StorySystem::trigger_cutscene(CutScene& c, const vec2& trigger_pos)
{
	Entity camera = registry.view<Camera>().front();
	WorldPosition& camera_world_pos = registry.get<WorldPosition>(camera);
	UIGroup& ui_group = registry.get<UIGroup>(c.cutscene_ui);
	ui_group.visible = true;
	camera_world_pos.position = trigger_pos;
	for (std::string text : c.texts) {
		conversation.push_back(text);
	}
	proceed_conversation();
}

void StorySystem::proceed_conversation()
{
	CutScene& c = registry.get<CutScene>(current_cutscene_entity);
	Entity ui_entity = c.cutscene_ui;

	if (conversation.empty()) {
		registry.get<UIGroup>(ui_entity).visible = false;
		registry.remove<CutScene>(current_cutscene_entity);
		current_cutscene_entity = entt::null;
		curr_text_pos = text_init_pos;
		return;
	}

	std::string text = conversation.front();
	conversation.pop_front();
	create_ui_text(ui_entity, curr_text_pos, text, Alignment::Start, Alignment::Start, 48);
	curr_text_pos.y += 0.05;
}
