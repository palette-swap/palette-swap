#pragma once
#include "common.hpp"
#include <glm/gtx/norm.hpp>

namespace Geometry {
struct Circle;

struct Rectangle {
	vec2 center;
	vec2 size;

	Rectangle(vec2 center, vec2 size)
		: center(center)
		, size(size)
	{
	}

	vec2 top_left() const { return center - size / 2.f; }
	vec2 bottom_right() const { return center + size / 2.f; }

	bool intersects(const Rectangle& r) const;
	bool intersects(const Circle& c) const;
};

struct Circle {
	vec2 center;
	float radius;

	Circle(vec2 center, float radius)
		: center(center)
		, radius(radius)
	{
	}

	bool intersects(Circle c) const { return glm::distance2(center, c.center) <= radius * radius; }
	bool intersects(const Rectangle& r) const;
};
} // namespace Geometry