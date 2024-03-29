#pragma once

#include "components.hpp"
#include "geometry.hpp"

Entity create_ui_group(bool visible, Groups identifier = Groups::Count);
Entity create_fancy_healthbar(Entity ui_group,
							  vec2 pos = vec2(.02f),
							  vec2 size = vec2(.25f, .0625f),
							  BarType target = BarType::Health);
Entity create_ui_counter(Entity group, Resource resource, ivec2 offset, int size, vec2 pos);
Entity create_ui_rectangle(Entity ui_group, vec2 pos, vec2 size);
Entity create_grid_rectangle(Entity ui_group, size_t slot, float width, float height, const Geometry::Rectangle& area);
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
Entity
create_ui_icon(Entity ui_group, ivec2 offset, vec2 texture_size, vec2 pos, vec2 size, UILayer layer = UILayer::Boxes);
Entity create_ui_item(Entity ui_group, Entity slot, Entity item);
Entity create_ui_text(Entity ui_group,
					  vec2 screen_position,
					  const std::string_view& text,
					  Alignment alignment_x = Alignment::Center,
					  Alignment alignment_y = Alignment::Center,
					  uint16 font_size = 48);
Entity create_ui_tooltip(Entity ui_group, vec2 screen_position, const std::string_view& text, uint16 font_size = 24);
Entity create_world_tooltip(Entity ui_group, vec2 world_position, const std::string_view& text, uint16 font_size = 24);
Entity create_background(Entity ui_group, vec2 pos, vec2 size, float opacity, vec4 fill_color);
Entity create_button(Entity ui_group,
					 vec2 screen_pos,
					 vec2 size,
					 vec4 fill_color,
					 ButtonAction action,
					 Entity action_target,
					 const std::string& text,
					 uint16 font_size = 60,
					 Alignment alignment_x = Alignment::Center,
					 Alignment alignment_y = Alignment::Center);
