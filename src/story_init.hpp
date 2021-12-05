#pragma once
#include "components.hpp"
#include "ui_init.hpp"

static constexpr vec2 text_init_pos = vec2( 0.25, .8 );

void create_room_cutscene(Entity entity, CutSceneType type, std::string texts);
void create_radius_cutscene(Entity entity, float radius, CutSceneType type, std::string texts);
Entity create_ui_for_conversation();


