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

// Value for maximum folding length
uniform float max_fold_length = 0.25f;

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

	// Creates initial folding effect along bottom and top sets of vertices
	if (frame >= 1) {
		if ((texcoord.y - top_side)/height <= 0.3) {
			distort_position.y += max_fold_length * abs((texcoord.y - (top_side + max_fold_length)))/max_fold_length;
			distort_position.x += max_fold_length/2 * abs((texcoord.y - (top_side + max_fold_length)))/max_fold_length;
		}

		if ((texcoord.y - top_side)/height >= 0.7) {
			distort_position.y -= max_fold_length * abs((texcoord.y - (bottom_side - max_fold_length)))/max_fold_length;
			distort_position.x -= max_fold_length/2 * abs((texcoord.y - (top_side + max_fold_length)))/max_fold_length;
		} else {
			distort_position.y += max_fold_length/2 * abs((texcoord.y - (bottom_side - max_fold_length)))/max_fold_length;
			distort_position.x -= max_fold_length/2 * abs((texcoord.y - (top_side + max_fold_length)))/max_fold_length;
		}
	}

	// Creates final folding effect from left and right side of vertices
	if (frame >= 3) {
		if ((texcoord.x - left_side)/width <= 0.3) {
			distort_position.x += max_fold_length * abs((texcoord.x - (left_side + max_fold_length)))/max_fold_length;
			distort_position.y += max_fold_length/2 * abs((texcoord.y - (top_side + max_fold_length)))/max_fold_length;
		}

		if ((texcoord.x - left_side)/width >= 0.7) {
			distort_position.x -= max_fold_length * abs((texcoord.x - (right_side - max_fold_length)))/max_fold_length;
			distort_position.y -= max_fold_length/2 * abs((texcoord.y - (top_side + max_fold_length)))/max_fold_length;
		} else {
			distort_position.y -= max_fold_length/2 * abs((texcoord.y - (bottom_side - max_fold_length)))/max_fold_length;
			distort_position.x += max_fold_length/2 * abs((texcoord.y - (top_side + max_fold_length)))/max_fold_length;
		}
	}

	// Creates output positions
	vec3 pos = projection * transform * vec3(distort_position.xy, 1.0);

	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}