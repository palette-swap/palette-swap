#include "geometry.hpp"

bool Geometry::Rectangle::intersects(const Rectangle& r) const
{
	return r.top_left().x <= bottom_right().x && r.top_left().y <= bottom_right().y
		&& r.bottom_right().x >= top_left().x && r.bottom_right().y >= top_left().y;
}

// Inspiration from https://stackoverflow.com/questions/401847/circle-rectangle-collision-detection-intersection
bool Geometry::Rectangle::intersects(const Circle& c) const
{
	vec2 half_size = size / 2.f;
	vec2 distance = abs(c.center - center);
	// If completely out of the way on either axis, not intersecting
	if (distance.x > c.radius + half_size.x || distance.y > c.radius + half_size.y) {
		return false;
	}

	if (distance.x <= half_size.x || distance.y <= half_size.y) {
		return true;
	}

	// Now simply check if it's within a certain radius of a corner
	return glm::distance2(distance, half_size) <= c.radius * c.radius;
}

bool Geometry::Circle::intersects(const Geometry::Rectangle& r) const { return r.intersects(*this); }
