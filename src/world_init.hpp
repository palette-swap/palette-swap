#pragma once

#include "common.hpp"
#include "render_system.hpp"
#include "tiny_ecs.hpp"

// the player
Entity create_player(uvec2 pos);
// the camera
Entity create_camera(vec2 pos, vec2 size, ivec2 central);
// the enemy
Entity create_enemy(uvec2 position, ColorState team = ColorState::Red);
// arrows generated by player
Entity create_arrow(vec2 position);
// a red line for debugging purposes
Entity create_line(vec2 position);
// Creates a room on the current world map
Entity create_room(vec2 position, MapUtility::RoomType roomType, float angle);
// Creates a team that encompasses a group of units
Entity create_team();
// Creates an item
Entity create_item(const std::string& name, const SlotList<bool> allowed_slots);
// Creates a weapon (also an item)
Entity create_weapon(const std::string& name, std::vector<Attack> attacks);
// create a static picture on screen
Entity create_picture(vec2 pos);
