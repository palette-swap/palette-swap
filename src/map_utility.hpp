#pragma once
#include "components.hpp"

namespace MapUtility {
static constexpr uint8_t num_rooms = 18;
static constexpr uint8_t num_levels = 3;

struct MapAreaIterator {
	using iterator_category = std::input_iterator_tag;
	using difference_type = vec2;
	using value_type = const uvec2;
	using pointer = value_type*;
	using reference = value_type&;

	reference operator*() const { return current_pos; }
	pointer operator->() { return &current_pos; }

	MapAreaIterator& operator++()
	{
		if (current_pos.x >= max_x) {
			current_pos.x = min_x;
			current_pos.y++;
		} else {
			current_pos.x++;
		}
		return *this;
	}

	MapAreaIterator operator++(int)
	{
		MapAreaIterator tmp = *this;
		++(*this);
		return tmp;
	}

	friend bool operator==(const MapAreaIterator& a, const MapAreaIterator& b)
	{
		return a.current_pos == b.current_pos;
	};
	friend bool operator!=(const MapAreaIterator& a, const MapAreaIterator& b)
	{
		return a.current_pos != b.current_pos;
	};

	MapAreaIterator(uint min_x, uint max_x, uvec2 current_pos);

private:
	uint min_x;
	uint max_x;
	uvec2 current_pos;
};

struct MapArea {

	MapArea(const MapPosition& map_pos, const MapSize& map_size);

	MapAreaIterator begin()
	{
		uvec2 current_pos;
		current_pos.x = (map_pos.position.x >= map_size.center.x) ? map_pos.position.x - map_size.center.x : 0;
		current_pos.y = (map_pos.position.y >= map_size.center.y) ? map_pos.position.y - map_size.center.y : 0;
		return MapAreaIterator(current_pos.x, current_pos.x + map_size.area.x - 1, current_pos);
	}
	MapAreaIterator end()
	{
		uvec2 current_pos;
		current_pos.x = (map_pos.position.x + map_size.area.x - 1 >= map_size.center.x)
			? map_pos.position.x + map_size.area.x - 1 - map_size.center.x
			: 0;
		current_pos.y = (map_pos.position.y + map_size.area.y - 1 >= map_size.center.y)
			? map_pos.position.y + map_size.area.y - 1 - map_size.center.y
			: 0;
		return MapAreaIterator(current_pos.x - map_size.area.x + 1, current_pos.x, current_pos);
	}

private:
	const MapPosition& map_pos;
	const MapSize& map_size;
};
} // namespace MapUtility