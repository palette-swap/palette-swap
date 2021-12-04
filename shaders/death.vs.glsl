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

	// TODO: Incrementaly applies a folding effect based on frame currently rendered
	

//	 Distorts the death animation sinusoidally
	distort_position.x += 0.03 * sin(time);
	distort_position.y += 0.03 * sin(time);

////	 Distorts the render left and right
//	if ((abs(texcoord.x - left_side))/width <= 0.5) {
//		distort_position.x += 0.01 * frame;
//	} else {
//		distort_position.x -= 0.01 * frame;
//	}
//
//	if ((abs(texcoord.y - top_side))/height <= 0.5) {
//		distort_position.y += 0.01 * frame;
//	} else {
//		distort_position.y -= 0.01 * frame;
//	}

	if (frame >= 1) {
		if ((texcoord.y - top_side)/height <= 0.4) {
			distort_position.y += 0.3 * abs((texcoord.y - (top_side + 0.3)))/0.3;
		}

		if ((texcoord.y - top_side)/height >= 0.6) {
			distort_position.y -= 0.3 * abs((texcoord.y - (bottom_side - 0.3)))/0.3;
		}
	}

	if (frame >= 3) {
		if ((texcoord.x - left_side)/width <= 0.4) {
			distort_position.x += 0.3 * abs((texcoord.x - (left_side + 0.3)))/0.3;
		}

		if ((texcoord.x - left_side)/width >= 0.6) {
			distort_position.x -= 0.3 * abs((texcoord.x - (right_side - 0.3)))/0.3;
		}
	}



	// Creates output positions
	vec3 pos = projection * transform * vec3(distort_position.xy, 1.0);

	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}