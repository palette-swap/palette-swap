// internal
#include "render_system.hpp"
#include <SDL.h>
#pragma warning(push)
#pragma warning(disable : 4201) // nameless struct; glm only uses it if it exists
#include <glm/gtc/type_ptr.hpp> // Allows nice passing of values to GL functions
#pragma warning(pop)

#include "tiny_ecs_registry.hpp"

void RenderSystem::draw_textured_mesh(Entity entity, const mat3& projection)
{
	Transform transform;
	if (registry.map_positions.has(entity)) {
		MapPosition& map_position = registry.map_positions.get(entity);
		transform.translate(map_position_to_screen_position(map_position.position));
		transform.scale(map_position.scale);
	} else {
		Motion& motion = registry.motions.get(entity);
		// Transformation code, see Rendering and Transformation in the template
		// specification for more info Incrementally updates transformation matrix,
		// thus ORDER IS IMPORTANT
		transform.translate(motion.position);
		transform.rotate(motion.angle);
		transform.scale(motion.scale);
	}

	assert(registry.render_requests.has(entity));
	const RenderRequest& render_request = registry.render_requests.get(entity);

	const auto used_effect_enum = (GLuint)render_request.used_effect;
	assert(used_effect_enum != (GLuint)EFFECT_ASSET_ID::EFFECT_COUNT);
	const auto program = (GLuint)effects.at(used_effect_enum);

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	assert(render_request.used_geometry != GEOMETRY_BUFFER_ID::GEOMETRY_COUNT);
	int vbo_ibo_offset = 0;
	if (render_request.used_geometry == GEOMETRY_BUFFER_ID::ROOM) {
		vbo_ibo_offset = registry.rooms.get(entity).type;
	}

	const GLuint vbo = vertex_buffers.at((int)render_request.used_geometry + vbo_ibo_offset);
	const GLuint ibo = index_buffers.at((int)render_request.used_geometry + vbo_ibo_offset);

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	// Input data location as in the vertex buffer
	if (render_request.used_effect == EFFECT_ASSET_ID::TEXTURED) {
		GLint in_position_loc = glGetAttribLocation(program, "in_position");
		GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
		gl_has_errors();
		assert(in_texcoord_loc >= 0);

		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), nullptr);
		gl_has_errors();

		glEnableVertexAttribArray(in_texcoord_loc);
		glVertexAttribPointer(in_texcoord_loc,
							  2,
							  GL_FLOAT,
							  GL_FALSE,
							  sizeof(TexturedVertex),
							  (void*)sizeof(vec3)); // note the stride to skip the preceeding vertex position
		// Enabling and binding texture to slot 0
		glActiveTexture(GL_TEXTURE0);
		gl_has_errors();

		assert(registry.render_requests.has(entity));
		GLuint texture_id = texture_gl_handles.at((GLuint)registry.render_requests.get(entity).used_texture);

		glBindTexture(GL_TEXTURE_2D, texture_id);
		gl_has_errors();
	} else if (render_request.used_effect == EFFECT_ASSET_ID::LINE) {
		GLint in_position_loc = glGetAttribLocation(program, "in_position");
		GLint in_color_loc = glGetAttribLocation(program, "in_color");
		gl_has_errors();

		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)0);
		gl_has_errors();

		glEnableVertexAttribArray(in_color_loc);
		glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)sizeof(vec3));
		gl_has_errors();
		// Consider handling drawing of player here
	} else if (render_request.used_effect == EFFECT_ASSET_ID::TILE_MAP) {
		GLint in_position_loc = glGetAttribLocation(program, "in_position");
		GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
		GLint tile_index_loc = glGetAttribLocation(program, "tile_index");

		gl_has_errors();
		assert(in_texcoord_loc >= 0);

		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TileMapVertex), (void*)0);
		gl_has_errors();

		glEnableVertexAttribArray(in_texcoord_loc);
		glVertexAttribPointer(in_texcoord_loc,
							  2,
							  GL_FLOAT,
							  GL_FALSE,
							  sizeof(TileMapVertex),
							  (void*)sizeof(vec3)); // note the stride to skip the preceeding vertex position

		// Pass in the tile indexes
		glEnableVertexAttribArray(tile_index_loc);
		glVertexAttribPointer(
			tile_index_loc, 1, GL_FLOAT, GL_FALSE, sizeof(TileMapVertex), (void*)(sizeof(vec3) + sizeof(vec2)));

		// Currently we pass in all tiles included, this can be inefficient and can
		// overflow the texture limit, some ways to optimize
		// 1. Pass in certian tiles based on camera position
		// 2. (Preferred) Use a sprite sheet(tilemap atlas), access multiple tiles for
		// a single load
		int samplers[num_tile_textures];
		for (int i = 0; i < num_tile_textures; i++)
		{
			// Maintenance Note:
			// To support OpenGL 3.3, glBindTextureUnit (OpenGL 4.5 feature) is replaced by glActiveTexture + glBindTexture.
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, texture_gl_handles[(GLuint)tile_textures[i]]);
      
			samplers[i] = i;
		}
    
		auto textures_loc = glGetUniformLocation(program, "tile_textures");
		glUniform1iv(textures_loc, num_tile_textures, samplers);
	} else {
		assert(false && "Type of render request not supported");
	}

	// Getting uniform locations for glUniform* calls
	if (registry.colors.has(entity))
	{
		GLint color_uloc = glGetUniformLocation(program, "fcolor");
		const vec3 color = registry.colors.get(entity);
		glUniform3fv(color_uloc, 1, glm::value_ptr(color));
		gl_has_errors();
	}

	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	GLsizei num_indices = size / sizeof(uint16_t);
	// GLsizei num_triangles = num_indices / 3;

	GLint curr_program;
	glGetIntegerv(GL_CURRENT_PROGRAM, &curr_program);
	// Setting uniform values to the currently bound program
	GLint transform_loc = glGetUniformLocation(curr_program, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, glm::value_ptr(transform.mat));
	GLint projection_loc = glGetUniformLocation(curr_program, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));
	gl_has_errors();
	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();
}

// draw the intermediate texture to the screen, with some distortion to simulate
// water
void RenderSystem::draw_to_screen()
{
	// Setting shaders
	// get the water texture, sprite mesh, and program
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::WATER]);
	gl_has_errors();
	// Clearing backbuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window_width_px, window_height_px);
	glDepthRange(0, 10);
	glClearColor(1.f, 0, 0, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_has_errors();
	// Enabling alpha channel for textures
	glDisable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	// Draw the screen texture on the quad geometry
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
	glBindBuffer(
		GL_ELEMENT_ARRAY_BUFFER,
		index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]); // Note, GL_ELEMENT_ARRAY_BUFFER associates
																	 // indices to the bound GL_ARRAY_BUFFER
	gl_has_errors();
	const GLuint water_program = effects[(GLuint)EFFECT_ASSET_ID::WATER];
	// Set clock
	GLint time_uloc = glGetUniformLocation(water_program, "time");
	GLint dead_timer_uloc = glGetUniformLocation(water_program, "darken_screen_factor");
	glUniform1f(time_uloc, (float)(glfwGetTime() * 10.0f));
	ScreenState& screen = registry.screen_states.get(screen_state_entity);
	glUniform1f(dead_timer_uloc, screen.darken_screen_factor);
	gl_has_errors();
	// Set the vertex position and vertex texture coordinates (both stored in the
	// same VBO)
	GLint in_position_loc = glGetAttribLocation(water_program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr);
	gl_has_errors();

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	gl_has_errors();
	// Draw
	glDrawElements(GL_TRIANGLES,
				   3,
				   GL_UNSIGNED_SHORT,
				   nullptr); // one triangle = 3 vertices; nullptr indicates that there is
							 // no offset from the bound index buffer
	gl_has_errors();
}

// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void RenderSystem::draw()
{
	// First render to the custom framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();
	// Clearing backbuffer
	glViewport(0, 0, window_width_px, window_height_px);
	glDepthRange(0.00001, 10);
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST); // native OpenGL does not work with a depth buffer
							  // and alpha blending, one would have to sort
							  // sprites back to front
	gl_has_errors();
	mat3 projection_2d = create_projection_matrix();
	// Draw all textured meshes that have a position and size component
	for (Entity entity : registry.render_requests.entities) {
		if (!registry.motions.has(entity) && !registry.map_positions.has(entity)) {
			continue;
		}
		// Note, its not very efficient to access elements indirectly via the entity
		// albeit iterating through all Sprites in sequence. A good point to optimize
		draw_textured_mesh(entity, projection_2d);
	}

	// Truely render to the screen
	draw_to_screen();

	// flicker-free display with a double buffer
	glfwSwapBuffers(window);
	gl_has_errors();
}

// TODO: Update projection matrix to be based on camera
// currently it's based on player
mat3 RenderSystem::create_projection_matrix()
{
	// Fake projection matrix, scales with respect to window coordinates
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	gl_has_errors();

	// set up 4 sides of window based on player
	Entity player = registry.players.entities[0];
	vec2 position = map_position_to_screen_position(registry.map_positions.get(player).position);

	float zoom = 0.25;// zoom in 4 times

	float right = position.x + w * zoom / 2.f;
	float left = position.x - w * zoom / 2.f;
	float top = position.y - h * zoom / 2.f;
	float bottom = position.y + h * zoom / 2.f;

	float sx = 2.f / (right - left);
	float sy = 2.f / (top - bottom);
	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);
	return { { sx, 0.f, 0.f }, { 0.f, sy, 0.f }, { tx, ty, 1.f } };
}