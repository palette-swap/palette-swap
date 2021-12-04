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

bool Geometry::Rectangle::contains(const vec2& p) const
{
	vec2 difference = p - center;
	return abs(difference.x) <= size.x * .5 && abs(difference.y) <= size.y * .5;
}

bool Geometry::Circle::intersects(const Geometry::Rectangle& r) const { return r.intersects(*this); }

float Geometry::Triangle::slope_inverse(size_t start_index, size_t end_index) {
	vec2 dpos = vertices.at(end_index) - vertices.at(start_index);
	assert(dpos.y != 0);
	return dpos.x / dpos.y;
}

float get_det_t(vec2 p1, vec2 p2, vec2 p3) { return (p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y); }

// For the definition of this conversion see
// https://en.wikipedia.org/wiki/Barycentric_coordinate_system#Conversion_between_barycentric_and_Cartesian_coordinates
vec3 Geometry::Triangle::cartesian_to_barycentric(const vec2& p) const
{
	vec3 bary;
	float det_t = get_det_t(vertices[0], vertices[1], vertices[2]);
	bary.x = (vertices[1].y - vertices[2].y) * (p.x - vertices[2].x)
		+ (vertices[2].x - vertices[1].x) * (p.y - vertices[2].y);
	bary.x /= det_t;

	bary.y = (vertices[2].y - vertices[0].y) * (p.x - vertices[2].x)
		+ (vertices[0].x - vertices[2].x) * (p.y - vertices[2].y);
	bary.y /= det_t;

	bary.z = 1 - bary.x - bary.y;
	return bary;
}

bool Geometry::Triangle::contains(const vec2& p) const
{
	vec3 bary = cartesian_to_barycentric(p);
	return bary.x >= 0 && bary.y >= 0 && bary.z >= 0;
}

bool Geometry::Cone::contains(const vec2& p) const
{
	float det_t = get_det_t(vertices[0], vertices[1], vertices[2]);
	if (det_t == 0) {
		return 0 == get_det_t(vertices[0], vertices[1], p);
	}
	vec3 bary = cartesian_to_barycentric(p);
	return bary.y >= 0 && bary.z >= 0;
}
