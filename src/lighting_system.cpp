#include "lighting_system.hpp"

#include "geometry.hpp"

void LightingSystem::init(std::shared_ptr<MapGeneratorSystem> map) { this->map_generator = std::move(map); }

void draw_tile(uvec2 pos) {
	Entity entity = registry.create();
	registry.emplace<LightingRequest>(entity);
	registry.emplace<WorldPosition>(entity, MapUtility::map_position_to_world_position(pos));
	registry.emplace<Color>(entity, vec3(1));
	registry.emplace<UIRectangle>(entity, 1.f, vec4(1));
}

void LightingSystem::step()
{
	registry.destroy(registry.view<LightingRequest>().begin(), registry.view<LightingRequest>().end());
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

	for (uint r = 0; r < (uint)Rotation::Count; r++) {
		auto rotation = (Rotation)r;
		vec2 left_bound = MapUtility::map_position_to_world_position({ player_map_pos.x - 1, player_map_pos.y + 1 });
		vec2 right_bound = MapUtility::map_position_to_world_position({ player_map_pos.x + 1, player_map_pos.y + 1 });
		scan_row(player_map_pos, 0, player_world_pos, left_bound, right_bound, rotation);
	}
}

template <typename T> inline tvec2<T> LightingSystem::rotate_pos(tvec2<T> pos, tvec2<T> origin, Rotation rotation)
{
	if (rotation == Rotation::Down) {
		return pos;
	}
	tvec2<T> difference = pos - origin;
	switch (rotation) {
	case Rotation::Up: {
		difference *= -1;
		break;
	}
	case Rotation::Left:
		difference *= -1;
	case Rotation::Right: {
		T new_x = difference.y;
		difference.y = difference.x;
		difference.x = new_x;
		break;
	}
	default:
		break;
	}
	return origin + difference;
}

void LightingSystem::scan_row(
	uvec2 origin, int dy, const vec2& player_world_pos, vec2 left_bound, vec2 right_bound, Rotation rotation)
{
	uvec2 prev_pos = uvec2(-1, -1);
	uvec2 rotated_prev_pos = uvec2(-1, -1);
	Geometry::Cone cone(player_world_pos, right_bound, left_bound);
	for (int dx = roundf(dy * cone.slope_inverse(0, 2)); dx <= roundf(dy * cone.slope_inverse(0, 1)); dx++) {
		if (dx + static_cast<int>(origin.x) < 0
			|| origin.x + dx >= MapUtility::map_size * MapUtility::room_size) {
			continue;
		}
		uvec2 pos = uvec2(origin.x + dx, origin.y + dy);
		uvec2 rotated_pos = rotate_pos(pos, origin, rotation);
		if (!map_generator->walkable(rotated_pos) || cone.contains(MapUtility::map_position_to_world_position(pos))) {
			draw_tile(rotated_pos);
		}
		if (prev_pos != uvec2(-1, -1) && !map_generator->walkable(rotated_prev_pos)
			&& map_generator->walkable(rotated_pos)) {
			left_bound = MapUtility::map_position_to_world_position(prev_pos);
			left_bound += MapUtility::tile_size / 2.f * vec2(1, (left_bound.x < player_world_pos.x ? 1 : -1));
			cone.vertices[2] = left_bound;
		}
		if (map_generator->walkable(rotated_prev_pos) && !map_generator->walkable(rotated_pos)) {
			vec2 new_right_bound = MapUtility::map_position_to_world_position(pos);
			new_right_bound
				+= MapUtility::tile_size / 2.f * vec2(-1, (new_right_bound.x > player_world_pos.x ? 1 : -1));
			scan_row(origin, dy + 1, player_world_pos, left_bound, new_right_bound, rotation);
		}

		prev_pos = pos;
		rotated_prev_pos = rotated_pos;
	}
	if (map_generator->walkable(rotated_prev_pos)) {
		scan_row(origin, dy + 1, player_world_pos, left_bound, right_bound, rotation);
	}
}
