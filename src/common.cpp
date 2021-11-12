#include "common.hpp"

// Note, we could also use the functions from GLM but we write the transformations here to show the uderlying math
void Transform::scale(vec2 scale)
{
	mat3 s = {
		{ scale.x, 0.f, 0.f },
		{ 0.f, scale.y, 0.f },
		{ 0.f, 0.f, 1.f },
	};
	mat = mat * s;
}

void Transform::rotate(float radians)
{
	float c = cosf(radians);
	float s = sinf(radians);
	mat3 r = {
		{ c, s, 0.f },
		{ -s, c, 0.f },
		{ 0.f, 0.f, 1.f },
	};
	mat = mat * r;
}

void Transform::translate(vec2 offset)
{
	mat3 t = {
		{ 1.f, 0.f, 0.f },
		{ 0.f, 1.f, 0.f },
		{ offset.x, offset.y, 1.f },
	};
	mat = mat * t;
}

bool gl_has_errors()
{
	GLenum error = glGetError();

	if (error == GL_NO_ERROR) {
		return false;
	}

	while (error != GL_NO_ERROR) {
		const char* error_str = "";
		switch (error) {
		case GL_INVALID_OPERATION:
			error_str = "INVALID_OPERATION";
			break;
		case GL_INVALID_ENUM:
			error_str = "INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			error_str = "INVALID_VALUE";
			break;
		case GL_OUT_OF_MEMORY:
			error_str = "OUT_OF_MEMORY";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			error_str = "INVALID_FRAMEBUFFER_OPERATION";
			break;
		default:
			error_str = "INVALID_ERROR_CODE";
			break;
		}

		fprintf(stderr, "OpenGL: %s", error_str);
		error = glGetError();
		assert(false);
	}

	return true;
}

float direction_to_angle(Direction direction)
{
	switch (direction) {
	case Direction::Left:
		return 3 * glm::pi<float>() / 2;
	case Direction::Up:
		return 0;
	case Direction::Right:
		return glm::pi<float>() / 2;
	case Direction::Down:
		return glm::pi<float>();
	default:
		assert(false && "direction to angle: unexpected direction");
	}
	return 0.f;
}

entt::registry registry;// NOLINT(cppcoreguidelines-avoid-non-const-global-variables)


