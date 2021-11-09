#pragma once

#include "components.hpp"
#include "geometry.hpp"

// the player
Entity create_player(uvec2 pos);
// the camera
Entity create_camera(uvec2 pos);
// the enemy
Entity create_enemy(ColorState team, EnemyType type, uvec2 map_pos);
// arrows generated by player
Entity create_arrow(vec2 position);
// a red line for debugging purposes
Entity create_line(vec2 position, float length, float angle);
// a path point for debugging purposes
Entity create_path_point(vec2 position);
// Creates a team that encompasses a group of units
Entity create_team();
// Creates an item
Entity create_item(const std::string& name, const SlotList<bool>& allowed_slots);
// Creates a weapon (also an item)
Entity create_weapon(const std::string& name, std::vector<Attack> attacks);

////////////////////////
// UI

Entity create_ui_group(bool visible);
Entity create_fancy_healthbar(Entity ui_group);
Entity create_ui_rectangle(Entity ui_group, vec2 pos, vec2 size);
Entity create_grid_rectangle(
	Entity ui_group, size_t slot, Entity inventory, float width, float height, const Geometry::Rectangle& area);
Entity create_inventory_slot(Entity ui_group,
							 size_t slot,
							 Entity inventory,
							 float width,
							 float height,
							 const Geometry::Rectangle& area = { vec2(.375, .5), vec2(.75, 1) });
Entity create_equip_slot(Entity ui_group,
						 Slot slot,
						 Entity inventory,
						 float width,
						 float height,
						 const Geometry::Rectangle& area = { vec2(.875, .5), vec2(.25, 1) });
