#version 330

// Passed to fragment shader
out vec2 texcoord;

// Application data
uniform mat3 transform;
uniform mat3 projection;
uniform uint room_layout[100];

const uint room_size = 10u;

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
		vec2(0.0, 31.0/256.0),
		vec2(31.0/256.0, 0),
		vec2(31.0/256.0, 0),
		vec2(0.0, 31.0/256.0),
		vec2(31.0/256.0, 31.0/256.0)
	);

	int vertex_id = gl_VertexID % 6;
	uint texture_id = room_layout[gl_VertexID / 6];
	texcoord = texture_coord[vertex_id] + vec2((int(texture_id) % 8) * 32.0 / 256.0, (int(texture_id) / 8) * 32.0 / 256.0);

	int row = (gl_VertexID % 600 ) / (6 * int(room_size));
	int col = (gl_VertexID % (6 * int(room_size))) / 6;
	vec3 vertex_position = vec3(vertex_positions[vertex_id].x + fraction * col, vertex_positions[vertex_id].y + fraction * row, 0.0);

	vec3 pos = projection * transform * vec3(vertex_position.xy, 1.0);
	gl_Position = vec4(pos, 1.0);
}