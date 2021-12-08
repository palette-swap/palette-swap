#include <story_init.hpp>




void create_room_cutscene(Entity entity, CutSceneType type, std::vector<std::string> texts, Entity actual_entity)
{
	registry.emplace<RoomTrigger>(entity);
	registry.emplace<CutScene>(entity, type, UIGroup::find(Groups::Story), texts, actual_entity);
}

void create_radius_cutscene(Entity entity, float radius, CutSceneType type, std::vector<std::string> texts, Entity actual_entity) {
	registry.emplace<RadiusTrigger>(entity, radius);
	registry.emplace<CutScene>(entity, type, UIGroup::find(Groups::Story), texts, actual_entity);
}
