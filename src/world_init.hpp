#pragma once

#include "common.hpp"
#include "render_system.hpp"
#include "tiny_ecs.hpp"

// the player
Entity create_player(uvec2 pos);
// the camera
Entity create_camera(uvec2 pos);
// the enemy
Entity create_enemy(uvec2 position, ColorState team = ColorState::Red);
// arrows generated by player
Entity create_arrow(vec2 position);
// a red line for debugging purposes
Entity create_line(vec2 position);
// Creates a room on the current world map
Entity create_room(vec2 position, MapUtility::RoomType roomType);
// Creates a team that encompasses a group of units
Entity create_team();
