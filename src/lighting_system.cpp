#include "lighting_system.hpp"

void LightingSystem::init(std::shared_ptr<MapGeneratorSystem> map) { this->map_generator = std::move(map); }

void draw_tile(uvec2 pos) {
	Entity entity = registry.create();
	registry.emplace<LightingRequest>(entity);
	registry.emplace<MapPosition>(entity, pos);
	registry.emplace<Color>(entity, vec3(1));
	registry.emplace<UIRectangle>(entity, 1.f, vec4(1));
}

void LightingSystem::step()
{
	registry.destroy(registry.view<LightingRequest>().begin(), registry.view<LightingRequest>().end());
	uvec2 prev_tile = uvec2(-1, -1);
	uvec2 player_pos = registry.get<MapPosition>(registry.view<Player>().front()).position;
	for (int dy = 0; player_pos.y + dy < MapUtility::map_size * MapUtility::room_size; dy++) {
		for (int dx = -dy; dx <= dy; dx++) {
			if (dx + static_cast<int>(player_pos.x) < 0
				|| player_pos.x + dx >= MapUtility::map_size * MapUtility::room_size) {
				continue;
			}
			uvec2 pos = uvec2(player_pos.x + dx, player_pos.y + dy);
			if (!map_generator->walkable(pos)) {
				draw_tile(pos);
			}
			prev_tile = pos;
		}
	}
}