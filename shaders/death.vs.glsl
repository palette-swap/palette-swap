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
	// Defines the boundaries of the box for the sprite based on the sprite
	float left_side = 1/num_frames * frame;
	float right_side = 1/num_frames * (frame + 1);
	float top_side = 1/num_states * state;
	float bottom_side = 1/num_states * (state + 1);

	vec3 distort_position = in_position;
	distort_position.x += 0.10 * sin(time);
	distort_position.y += 0.10 * sin(time + 0.3);
	// Creates output positions
	vec3 pos = projection * transform * vec3(distort_position.xy, 1.0);

	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}