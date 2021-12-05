#version 330

// inputs
in int cur_vertex_id;

// Passed to fragment shader
out vec2 texcoord;
out vec2 world_pos;

// Application data
uniform mat3 transform;
uniform mat3 projection;
uniform uint room_layout[100];

uniform int frame;

const uint room_size = 10u;
const uint num_animated_tiles = 2u;
const uint animated_tiles[num_animated_tiles] = uint[](28u, 36u);

void main()
{
	float fraction = 1.f / room_size;
	// The six vertex positions of a tile, below is the layout for a 4-tile room
	//      0           2 6         8
	//      +----------------------
	//      |       -/3|       -/ | 9
	//      |    --/   |    --/   |
	//      |  -/      |7 -/      |
	//      |-/4      5|-/10      |11
	//    1 |--------14-----------|
	//    12|       --15 18    20-| 21
	//   	|   ---/   |     /--  |
	//      |--/       |19/--     |
	//    13/---------------------+ 23
	//       16        17   22

    vec3 vertex_positions[] = vec3[](
	    vec3(-fraction * room_size / 2,            -fraction * room_size / 2, 0.0),
		vec3(-fraction * room_size / 2,            -fraction * room_size / 2 + fraction, 0.0),
		vec3(-fraction * room_size / 2 + fraction, -fraction * room_size / 2, 0.0),
		vec3(-fraction * room_size / 2 + fraction, -fraction * room_size / 2, 0.0),
		vec3(-fraction * room_size / 2,            -fraction * room_size / 2 + fraction, 0.0),
		vec3(-fraction * room_size / 2 + fraction, -fraction * room_size / 2 + fraction, 0.0)
	);

	// The corresponding tile textures to tile vertices
	const vec2 texture_coord[] = vec2[](
		vec2(0.0,0.0),
		vec2(0.0, 32.0/256.0),
		vec2(32.0/256.0, 0),
		vec2(32.0/256.0, 0),
		vec2(0.0, 32.0/256.0),
		vec2(32.0/256.0, 32.0/256.0)
	);

	int vertex_id = cur_vertex_id % 6;
	uint texture_id = room_layout[cur_vertex_id / 6];
	for (int i = 0; i < 2; i++) {
		if (texture_id == animated_tiles[i]) {
			texture_id = texture_id + uint(frame);
		}
	}
	texcoord = texture_coord[vertex_id] + vec2((int(texture_id) % 8) * 32.0 / 256.0, (int(texture_id) / 8) * 32.0 / 256.0);

	int row = cur_vertex_id / (6 * int(room_size));
	int col = (cur_vertex_id % (6 * int(room_size))) / 6;
	vec3 vertex_position = vec3(vertex_positions[vertex_id].x + fraction * col, vertex_positions[vertex_id].y + fraction * row, 0.0);

	world_pos = (transform * vec3(fraction * col - fraction * room_size / 2, fraction * row - fraction * room_size / 2, 1.0)).xy;

	vec3 pos = projection * transform * vec3(vertex_position.xy, 1.0);
	gl_Position = vec4(pos, 1.0);
}