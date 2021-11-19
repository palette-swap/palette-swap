#include <story_init.hpp>



Entity create_ui_for_conversation()
{
	auto ui_group = create_ui_group(false);
	create_background(ui_group, vec2(.5, .9), vec2(.6, .2), .5, vec4(.1, .1, .6, 1));
	return ui_group;
}

void create_cutscene(Entity attacher, float radius, std::vector<std::string> conversation)
{
	auto ui_entity = create_ui_for_conversation();
	registry.emplace<CutScene>(attacher, radius, ui_entity, conversation);
}
