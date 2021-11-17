#include <story_init.hpp>

Entity create_ui_for_conversation(std::string conversation)
{
	auto ui_group = create_ui_group(false);
	create_background(ui_group, vec2(.75, .5), vec2(.5, 1), 1.f, vec4(.1, .1, .6, 1));
	create_ui_text(ui_group, vec2(.5, .1), conversation, Alignment::Center, Alignment::Start, 180);
	return ui_group
}

void create_cutscene(Entity attacher, float radius, std::string text)
{
	auto ui_entity = create_ui_for_conversation(text);
	registry.emplace<CutScene>(attacher, radius, ui_entity);

}
