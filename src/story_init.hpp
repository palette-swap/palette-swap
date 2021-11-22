#pragma once
#include "components.hpp"
#include "ui_init.hpp"

static constexpr vec2 text_init_pos = { .3, .9 };

void create_cutscene(Entity attacher, Entity actual_entity, CutSceneType type, float radius, const std::string& text);

Entity create_ui_for_conversation();


