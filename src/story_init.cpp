#include <story_init.hpp>



Entity create_ui_for_conversation()
{
	auto ui_group = create_ui_group(false);
	create_background(ui_group, vec2(.5, .9), vec2(.6, .2), .5, vec4(.1, .1, .6, 1));
	create_ui_text(ui_group, text_init_pos, "", Alignment::Start, Alignment::Start, 48);
	return ui_group;
}


void create_room_cutscene(Entity entity, CutSceneType type, std::vector<std::string> texts, Entity actual_entity)
{
	registry.emplace<RoomTrigger>(entity);
	registry.emplace<CutScene>(entity, type, create_ui_for_conversation(), texts, actual_entity);
}

void create_radius_cutscene(Entity entity, float radius, CutSceneType type, std::vector<std::string> texts, Entity actual_entity) {
	registry.emplace<RadiusTrigger>(entity, radius);
	registry.emplace<CutScene>(entity, type, create_ui_for_conversation(), texts, actual_entity);
}
