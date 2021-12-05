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
	bool contains(const vec2& p) const;
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

struct Triangle {
	std::array<vec2, 3> vertices;

	Triangle(vec2 v1, vec2 v2, vec2 v3)
		: vertices({ v1, v2, v3 })
	{
	}

	float slope_inverse(size_t start_index, size_t end_index);

	vec3 cartesian_to_barycentric(const vec2& p) const;
	virtual bool contains(const vec2& p) const;

};

struct Cone: Triangle {
	// Vertices specified in clockwise order
	Cone(vec2 v1_origin, vec2 v2, vec2 v3)
		: Triangle(v1_origin, v2, v3)
	{
	}
	bool contains(const vec2& p) const override;
};
} // namespace Geometry