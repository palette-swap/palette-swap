#pragma once
#include "components.hpp"
#include "ui_init.hpp"

static constexpr vec2 text_init_pos = { .3, .9 };

void create_cutscene(Entity attacher, float radius, std::string text);

Entity create_ui_for_conversation();


