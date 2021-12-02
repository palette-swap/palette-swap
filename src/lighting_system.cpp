#include "lighting_system.hpp"

#include "geometry.hpp"

#include <glm/gtx/rotate_vector.hpp>

void LightingSystem::init(std::shared_ptr<MapGeneratorSystem> map) { this->map_generator = std::move(map); }

void light_tile(uvec2 pos)
{
	const static vec3 color(1);
	Entity entity = registry.create();
	registry.emplace<LightingTile>(entity);
	registry.emplace<WorldPosition>(entity, MapUtility::map_position_to_world_position(pos));
	registry.emplace<Color>(entity, color);
	registry.emplace<UIRectangle>(entity, 1.f, vec4(color, 1));
}

void light_triangle(vec2 p1, vec2 p2, vec2 p3)
{
	Entity entity = registry.create();
	registry.emplace<LightingTriangle>(entity, p1, p2, p3);
}

void LightingSystem::step()
{
	registry.destroy(registry.view<LightingTriangle>().begin(), registry.view<LightingTriangle>().end());
	registry.destroy(registry.view<LightingTile>().begin(), registry.view<LightingTile>().end());
	Entity player = registry.view<Player>().front();
	vec2 player_world_pos;
	uvec2 player_map_pos;
	if (WorldPosition* world_pos = registry.try_get<WorldPosition>(player)) {
		player_world_pos = world_pos->position;
		player_map_pos = MapUtility::world_position_to_map_position(player_world_pos);
	} else {
		player_map_pos = registry.get<MapPosition>(player).position;
		player_world_pos = MapUtility::map_position_to_world_position(player_map_pos);
	}

	spin(player_map_pos, player_world_pos);
}

bool LightingSystem::is_visible(uvec2 tile)
{
	// First, check los
	if (visible_tiles.count(tile) == 0) {
		return false;
	}

	// Then, check lighting
	Entity player = registry.view<Player>().front();
	uvec2 player_pos = registry.get<MapPosition>(player).position;
	uvec2 dist_2 = (tile - player_pos) * (tile - player_pos);
	return dist_2.x + dist_2.y <= pow(registry.get<Light>(player).radius / MapUtility::tile_size + .707f, 2);
}

void LightingSystem::spin(uvec2 player_map_pos, vec2 player_world_pos)
{
	visited_angles.clear();
	visible_tiles.clear();
	visible_tiles.emplace(player_map_pos);
	auto check_point = [&](uvec2 tile) {
		if (!map_generator->is_on_map(tile)) {
			return;
		}
		process_tile(player_world_pos, tile);
	};
	for (int radius = 1; radius < light_radius; radius++) {
		for (int dx = 0; dx <= radius; dx++) {
			for (int dy = 0; dy + dx <= radius; dy++) {
				if (dx + dy != radius) {
					continue;
				}
				if (dx * dx + dy * dy >= light_radius * light_radius) {
					continue;
				}
				check_point(uvec2(player_map_pos.x + dx, player_map_pos.y + dy));
				if (dx != 0 && dy != 0) {
					check_point(uvec2(player_map_pos.x - dx, player_map_pos.y - dy));
				}
				if (dx != 0) {
					check_point(uvec2(player_map_pos.x - dx, player_map_pos.y + dy));
				}
				if (dy != 0) {
					check_point(uvec2(player_map_pos.x + dx, player_map_pos.y - dy));
				}
				if (visited_angles.size() == 1 && visited_angles.at(0).x <= -glm::pi<double>()
					&& visited_angles.at(0).y >= glm::pi<double>()) {
					// We've darkened everything now
					update_visible();
					return;
				}
			}
		}
	}

	for (int i = 0; i <= visited_angles.size(); i++) {
		dvec2 angle;
		angle.x = (i == 0) ? -glm::pi<double>() : visited_angles.at(i - 1).y;
		angle.y = (i == visited_angles.size()) ? glm::pi<double>() : visited_angles.at(i).x;
		while (angle.x < angle.y) {
			auto scale = static_cast<double>(2 * light_radius * MapUtility::tile_size);
			vec2 p2 = player_world_pos + vec2(scale * glm::rotate(dvec2(1, 0), angle.x));
			vec2 p3 = player_world_pos
				+ vec2(scale * glm::rotate(dvec2(1, 0), min(angle.y, angle.x + glm::pi<double>() / 2.0)));
			light_triangle(player_world_pos, p2, p3);
			angle.x += glm::pi<double>() / 2.0;
		}
	}
	update_visible();
}

void LightingSystem::process_tile(vec2 player_world_pos, uvec2 tile)
{
	bool is_solid = !map_generator->walkable(tile);
	auto min_angle = glm::pi<double>();
	auto max_angle = -glm::pi<double>();
	int side = 0;
	bool cross_seam = false;
	for (const auto& offset : offsets) {
		dvec2 dpos = dvec2(MapUtility::map_position_to_world_position(tile) + MapUtility::tile_size / 2.f * vec2(offset)
						   - player_world_pos);
		double angle = atan2(dpos.y, dpos.x);
		if (abs(angle) >= glm::pi<double>() / 2.0 && side == 0) {
			side = (angle >= 0) ? 1 : -1;
		} else if (side != 0 && (side == 1) != (angle >= 0)) {
			// Crossing the seam case
			cross_seam = true;
			break;
		}
		min_angle = min(min_angle, angle);
		max_angle = max(max_angle, angle);
	}
	AngleResult result = AngleResult::Redundant;
	if (cross_seam) {
		dvec2 positive_angle = vec2(glm::pi<double>(), glm::pi<double>());
		dvec2 negative_angle = vec2(-glm::pi<double>(), -glm::pi<double>());
		for (const auto& offset : offsets) {
			dvec2 dpos = dvec2(MapUtility::map_position_to_world_position(tile)
							   + MapUtility::tile_size / 2.f * vec2(offset) - player_world_pos);
			double angle = atan2(dpos.y, dpos.x);
			if (angle >= 0) {
				positive_angle.x = min(positive_angle.x, angle);
			} else {
				negative_angle.y = max(negative_angle.y, angle);
			}
		}
		if (is_solid) {
			AngleResult pos_result = try_add_angle(positive_angle);
			AngleResult neg_result = try_add_angle(negative_angle);
			draw_tile(pos_result, positive_angle, tile, player_world_pos);
			draw_tile(neg_result, negative_angle, tile, player_world_pos);
			result = (AngleResult)((uint)pos_result | (uint)neg_result);
		} else {
			result = (AngleResult)((uint)check_visible(positive_angle) | (uint)check_visible(negative_angle));
		}
	} else {
		dvec2 angle = dvec2(min_angle, max_angle);
		if (is_solid) {
			result = try_add_angle(angle);
			draw_tile(result, angle, tile, player_world_pos);
		} else {
			result = check_visible(angle);
		}
	}
	if (result != AngleResult::Redundant) {
		mark_as_visible(tile);
	}
}

void LightingSystem::draw_tile(AngleResult result, const dvec2& angle, uvec2 tile, vec2 player_world_pos)
{ 
	switch (result) {
	case AngleResult::New: {
		for (int i = 0; i < offsets.size(); i++) {
			vec2 p2
				= MapUtility::map_position_to_world_position(tile) + center_offset * vec2(offsets.at(i));
			vec2 p3 = MapUtility::map_position_to_world_position(tile)
				+ center_offset * vec2(offsets.at((i + 1) % offsets.size()));
			light_triangle(player_world_pos, p2, p3);
			// light_tile(tile);
		}
		break;
	}
	case AngleResult::Overlap: {
		vec2 p2 = project_onto_tile(tile, player_world_pos, angle.x);
		vec2 p3 = project_onto_tile(tile, player_world_pos, angle.y);
		light_triangle(player_world_pos, p2, p3);
		light_tile(tile);
		break;
	}
	default:
		return;
	}
}

LightingSystem::AngleResult LightingSystem::try_add_angle(dvec2& angle)
{
	AngleResult result = AngleResult::New;
	size_t update_index = -1;
	for (size_t i = 0; i < visited_angles.size(); i++) {
		const auto& pair = visited_angles[i];
		if (rad_to_int(pair.x) <= rad_to_int(angle.x) && rad_to_int(pair.y) >= rad_to_int(angle.y)) {
			return AngleResult::Redundant;
		}
		if (rad_to_int(angle.x) <= rad_to_int(pair.y)) {
			if (rad_to_int(angle.y) >= rad_to_int(pair.x) && rad_to_int(angle.x) <= rad_to_int(pair.y)) {
				if (update_index == -1) {
					update_index = i;
				}
				// Intersect, remove redundant bits
				result = AngleResult::Overlap;
				if (rad_to_int(angle.x) <= rad_to_int(pair.x)) {
					// The ends overlapped, so we can break
					angle.y = pair.x;
					break;
				}
				// It might also overlap on the other end, so we need to keep going
				angle.x = pair.y;
			} else {
				// Didn't intersect / no longer intersecting, so we're in the clear
				break;
			}
		}
	}
	if (update_index == -1) {
		update_index = 0;
	}

	int inserted_pos = -1;
	ivec2 remove_range = ivec2(-1, -1);
	for (size_t i = update_index; i < visited_angles.size(); i++) {
		auto& pair = visited_angles[i];
		if (inserted_pos == -1 && pair.y >= angle.x) {
			visited_angles.emplace(visited_angles.begin() + i, angle.x, angle.y);
			inserted_pos = i;
			continue;
		}
		if (pair.x <= angle.y && pair.y >= angle.x) {
			visited_angles[inserted_pos]
				= dvec2(min(pair.x, visited_angles[inserted_pos].x), max(pair.y, visited_angles[inserted_pos].y));
			if (remove_range.x == -1) {
				remove_range.x = i;
			}
			remove_range.y = i + 1;
		}
		if (pair.x > angle.y) {
			break;
		}
	}
	if (inserted_pos == -1) {
		visited_angles.emplace_back(angle.x, angle.y);
	} else if (remove_range.x != -1) {
		visited_angles.erase(visited_angles.begin() + remove_range.x, visited_angles.begin() + remove_range.y);
	}
	return result;
}

vec2 LightingSystem::project_onto_tile(uvec2 tile, vec2 player_world_pos, double angle)
{
	dvec2 dpos = glm::rotate(dvec2(1, 0), angle);
	dvec2 sign = dvec2((dpos.x > 0) ? 1.f : -1.f, (dpos.y > 0) ? 1.f : -1.f);
	vec2 tile_center = MapUtility::map_position_to_world_position(tile);
	if (glm::epsilonEqual(dpos.x, 0.0, tol)) {
		return vec2(player_world_pos.x, tile_center.y + center_offset * sign.y);
	}
	if (glm::epsilonEqual(dpos.y, 0.0, tol)) {
		return vec2(tile_center.x + center_offset * sign.x, player_world_pos.y);
	}
	double min_dist = DBL_MAX;
	dvec2 min_pos;
	dvec2 test_pos;
	// First, try to land on a horizontal edge
	for (int i = 1; i >= -1; i -= 2) {
		test_pos.y = tile_center.y + center_offset * sign.y * i;
		test_pos.x = player_world_pos.x + (test_pos.y - player_world_pos.y) * (dpos.x / dpos.y);
		double dist = abs(static_cast<double>(tile_center.x) - test_pos.x);
		if (dist <= .5 * MapUtility::tile_size + tol) {
			// We're inside the tile bounds, so it worked
			return test_pos;
		} else if (dist < min_dist) {
			min_dist = dist;
			min_pos = test_pos;
		}
	}
	// Otherwise, it's a vertical edge
	for (int i = 1; i >= -1; i -= 2) {
		test_pos.x = tile_center.x + center_offset * sign.x * i;
		test_pos.y = player_world_pos.y + (test_pos.x - player_world_pos.x) * (dpos.y / dpos.x);
		double dist = abs(static_cast<double>(tile_center.y) - test_pos.y);
		if (dist <= .5 * MapUtility::tile_size + tol) {
			// We're inside the tile bounds, so it worked
			return test_pos;
		}
		if (dist < min_dist) {
			min_dist = dist;
			min_pos = test_pos;
		}
	}
	return min_pos;
}

LightingSystem::AngleResult LightingSystem::check_visible(dvec2& angle) {
	for (auto& pair : visited_angles) {
		if (rad_to_int(pair.x) <= rad_to_int(angle.x) && rad_to_int(pair.y) >= rad_to_int(angle.y)) {
			return AngleResult::Redundant;
		}
		if (rad_to_int(angle.x) <= rad_to_int(pair.y)) {
			if (rad_to_int(angle.y) >= rad_to_int(pair.x) && rad_to_int(angle.x) <= rad_to_int(pair.y)) {
				return AngleResult::Overlap;
			}
			return AngleResult::New;
		}
	}
	return AngleResult::New;
}

void LightingSystem::mark_as_visible(uvec2 tile)
{
	visible_tiles.insert(tile);
	visible_rooms.insert(MapUtility::get_room_index(tile));
}

void LightingSystem::update_visible()
{
	for (auto [entity, room] : registry.view<Room>().each()) {
		if (!room.visible && visible_rooms.count(room.room_index) > 0) {
			room.visible = true;
		}
	}
}
