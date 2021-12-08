#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

// Passed to fragment shader
out vec2 texcoord;
out vec2 vpos;


// Application data
uniform mat3 transform;
uniform mat3 projection;
uniform float time;

// Value used for sprite size, used for calculating spritesheet offset
uniform float sprite_size = 32;
// Value used for representing number of frames:
uniform float num_frames = 8;
// Value denoting total number of states for an enemy:
uniform float num_states = 8;
// Denotes current frame_state (frame rendered in animation as well as entity state)
uniform int frame;
uniform int state;

uniform float side_offset = 0.f;
uniform float height_offset = 0.f;

// Value for maximum folding length
uniform float max_fold_length = 0.25f;
uniform float cut_distortion = 0.20f;
uniform int direction = 1;

void main()
{
	texcoord = in_texcoord;
	texcoord.x += (1/num_frames * frame);
	texcoord.y += (1/num_states * state);
	vpos = in_position.xy;
	// Defines the boundaries of the box for the sprite based on the sprite
	float left_side = 1/num_frames * frame;
	float right_side = 1/num_frames * (frame + 1);
	float top_side = 1/num_states * state;
	float bottom_side = 1/num_states * (state + 1);
	float height = abs(top_side - bottom_side);
	float width = abs(right_side - left_side);

	vec3 distort_position = in_position;

	// Slight sinusoidal distortion
	distort_position.x += 0.1 * sin(time/2);


	float x_prop = (texcoord.x - left_side)/width;
	float y_prop = (texcoord.y - top_side)/height;

	if (x_prop > 0.3 && x_prop < 0.7) {
		distort_position.y += frame * height_offset * cut_distortion;
	}
	if (y_prop > 0.3 && y_prop < 0.7) {
		if (side_offset > 0 ) {
			distort_position.x += frame * side_offset * direction * cut_distortion;
		} else {
			distort_position.x += frame * side_offset * direction * cut_distortion;
		}
	}

	// Creates output positions
	vec3 pos = projection * transform * vec3(distort_position.xy, 1.0);

	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}