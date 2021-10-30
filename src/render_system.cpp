// internal
#include "render_system.hpp"
#include <SDL.h>
#pragma warning(push)
#pragma warning(disable : 4201) // nameless struct; glm only uses it if it exists
#include <glm/gtc/type_ptr.hpp> // Allows nice passing of values to GL functions
#pragma warning(pop)

#include "tiny_ecs_registry.hpp"

Transform RenderSystem::get_transform(Entity entity)
{
	Transform transform;
	if (registry.map_positions.has(entity)) {
		MapPosition& map_position = registry.map_positions.get(entity);
		transform.translate(MapUtility::map_position_to_world_position(map_position.position));
	} else {
		// Most objects in the game are expected to use MapPosition, exceptions are:
		// Arrow, Room.
		transform.translate(registry.world_positions.get(entity).position);
		if (registry.velocities.has(entity)) {
			// Probably can provide a get if exist function here to boost performance
			transform.rotate(registry.velocities.get(entity).angle);
		}
	}
	return transform;
}

void RenderSystem::prepare_for_textured(GLuint texture_id)
{
	const auto program = (GLuint)effects.at((uint8)EFFECT_ASSET_ID::TEXTURED);

	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	gl_has_errors();
	assert(in_texcoord_loc >= 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();
}

RenderSystem::TextData RenderSystem::generate_text(const Text& text)
{
	TextData text_data = {};
	// Vertex Buffer creation.
	glGenBuffers(1, &(text_data.vbo));
	// Index Buffer creation.
	glGenBuffers(1, &(text_data.ibo));
	// Texture creation
	glGenTextures(1, &(text_data.texture));

	// Check if we have the font in the right size, otherwise load it
	TTF_Font* font = nullptr;
	auto font_itr = fonts.find(text.font_size);
	if (font_itr != fonts.end()) {
		font = font_itr->second;
	} else {
		font = fonts.emplace(text.font_size, TTF_OpenFont(fonts_path("VT323-Regular.ttf").c_str(), text.font_size)).first->second;
	}

	// Render the text using SDL
	SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.text.c_str(), SDL_Color({ 0, 0, 0, 0 }));
	SDL_LockSurface(surface);
	int width = surface->w;
	int height = surface->h;
	if (surface == nullptr) {
		fprintf(stderr, "Error TTF_RenderText %s\n", text.text.c_str());
		return text_data;
	}
	GLint colors = surface->format->BytesPerPixel;
	assert(colors == 3 || colors == 4);

	// Extract the SDL image data into the texture
	glBindTexture(GL_TEXTURE_2D, text_data.texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D,
				 0,
				 (colors == 4) ? GL_RGBA : GL_RGB,
				 width,
				 height,
				 0,
				 (colors == 4) ? GL_RGBA : GL_RGB,
				 GL_UNSIGNED_BYTE,
				 surface->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glBindTexture(GL_TEXTURE_2D, 0);
	gl_has_errors();

	// Free the SDL Image
	SDL_FreeSurface(surface);

	// Set the vbo & ibo accordingly
	std::vector<TexturedVertex> vertices(4);
	vertices[0].position = { (float)-width / 2.f, (float)+height / 2.f, 0.f };
	vertices[1].position = { (float)+width / 2.f, (float)+height / 2.f, 0.f };
	vertices[2].position = { (float)+width / 2.f, (float)-height / 2.f, 0.f };
	vertices[3].position = { (float)-width / 2.f, (float)-height / 2.f, 0.f };
	vertices[0].texcoord = { 0, 1 };
	vertices[1].texcoord = { 1, 1 };
	vertices[2].texcoord = { 1, 0 };
	vertices[3].texcoord = { 0, 0 };

	// Counterclockwise as it's the default opengl front winding direction.
	const std::vector<uint16_t> indices = { 0, 3, 1, 1, 3, 2 };
	glBindBuffer(GL_ARRAY_BUFFER, text_data.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	gl_has_errors();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(), indices.data(), GL_STATIC_DRAW);
	gl_has_errors();
	return text_data;
}

void RenderSystem::draw_textured_mesh(Entity entity, const mat3& projection)
{
	Transform transform = get_transform(entity);

	assert(registry.render_requests.has(entity));
	const RenderRequest& render_request = registry.render_requests.get(entity);

	transform.scale(scaling_factors.at(static_cast<int>(render_request.used_texture)));

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

		assert(registry.render_requests.has(entity));
		prepare_for_textured(texture_gl_handles.at((GLuint)registry.render_requests.get(entity).used_texture));
	} else if (render_request.used_effect == EFFECT_ASSET_ID::ENEMY || render_request.used_effect == EFFECT_ASSET_ID::PLAYER) {

		GLint in_position_loc = glGetAttribLocation(program, "in_position");
		GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
		gl_has_errors();
		assert(in_texcoord_loc >= 0);

		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(EnemyVertex), nullptr);
		gl_has_errors();

		glEnableVertexAttribArray(in_texcoord_loc);
		glVertexAttribPointer(in_texcoord_loc,
							  2,
							  GL_FLOAT,
							  GL_FALSE,
							  sizeof(EnemyVertex),
							  (void*)sizeof(vec3)); // note the stride to skip the preceeding vertex position
		// Enabling and binding texture to slot 0
		glActiveTexture(GL_TEXTURE0);
		gl_has_errors();

		Animation& animation = registry.animations.get(entity);
		assert(registry.animations.has(entity));

		// Switches direction based on requested animation direction
		transform.scale({ animation.direction, 1 });

		// Updates frame for entity
		GLint frame_loc = glGetUniformLocation(program, "frame");
		glUniform1i(frame_loc, animation.frame);	
		gl_has_errors();

		// Updates frame for entity
		GLint state_loc = glGetUniformLocation(program, "state");
		glUniform1i(state_loc, animation.state);
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
		// Enabling and binding texture to slot 0
		glActiveTexture(GL_TEXTURE0);
		gl_has_errors();

		assert(registry.render_requests.has(entity));
		GLuint texture_id = texture_gl_handles.at((GLuint)registry.render_requests.get(entity).used_texture);

		glBindTexture(GL_TEXTURE_2D, texture_id);
		gl_has_errors();
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

	draw_triangles(transform, projection);
}

void RenderSystem::draw_healthbar(Entity entity, const Stats& stats, const mat3& projection)
{
	Transform transform = get_transform(entity);
	transform.translate(vec2(2-MapUtility::tile_size / 2, -MapUtility::tile_size / 2));
	transform.scale(vec2(MapUtility::tile_size - 4, 3));

	const auto program = (GLuint)effects.at((uint8) EFFECT_ASSET_ID::HEALTH);

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	const GLuint vbo = vertex_buffers.at((int)GEOMETRY_BUFFER_ID::HEALTH);
	const GLuint ibo = index_buffers.at((int)GEOMETRY_BUFFER_ID::HEALTH);

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	gl_has_errors();

	GLint in_color_loc = glGetAttribLocation(program, "in_color");
	gl_has_errors();

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)0);
	gl_has_errors();

	glEnableVertexAttribArray(in_color_loc);
	glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)sizeof(vec3));
	gl_has_errors();

	GLint health_loc = glGetUniformLocation(program, "health");
	glUniform1f(health_loc, max(static_cast<float>(stats.health), 0.f) / static_cast<float>(stats.health_max));

	draw_triangles(transform, projection);
}

void RenderSystem::draw_text(Entity entity, const Text& text, const mat3& projection)
{
	Transform transform = get_transform(entity);

	const auto program = (GLuint)effects.at((uint8)EFFECT_ASSET_ID::TEXTURED);

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	auto text_data = text_buffers.find(text);

	if (text_data == text_buffers.end()) {
		TextData new_text_data = generate_text(text);
		text_data = text_buffers.emplace(text, new_text_data).first;
	}

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, text_data->second.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_data->second.ibo);
	gl_has_errors();

	prepare_for_textured(text_data->second.texture);

	draw_triangles(transform, projection);
}

void RenderSystem::draw_triangles(const Transform& transform, const mat3& projection)
{
	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	GLsizei num_indices = (int)size / sizeof(uint16_t);

	GLint curr_program = 0;
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
	glViewport(0, 0, screen_size.x, screen_size.y);
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
	glViewport(0, 0, screen_size.x, screen_size.y);
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
		// Note, its not very efficient to access elements indirectly via the entity
		// albeit iterating through all Sprites in sequence. A good point to optimize
		if (registry.render_requests.get(entity).visible) {
			draw_textured_mesh(entity, projection_2d);
		}
	}

	for (int i = 0; i < registry.stats.size(); i++) {
		draw_healthbar(registry.stats.entities[i], registry.stats.components[i], projection_2d);
	}

	for (int i = 0; i < registry.text.size(); i++) {
		draw_text(registry.text.entities[i], registry.text.components[i], projection_2d);
	}

	// Truely render to the screen
	draw_to_screen();

	// flicker-free display with a double buffer
	glfwSwapBuffers(window);
	gl_has_errors();
}

// projection matrix based on position of camera entity
mat3 RenderSystem::create_projection_matrix()
{
	// Fake projection matrix, scales with respect to window coordinates
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	gl_has_errors();

	vec2 top_left = get_top_left();
	vec2 bottom_right = top_left + (vec2(w,h) * screen_scale);

	float sx = 2.f / (w * screen_scale);
	float sy = -2.f / (h * screen_scale);
	float tx = -(bottom_right.x + top_left.x) / (w * screen_scale);
	float ty = (bottom_right.y + top_left.y) / (h * screen_scale);
	return { { sx, 0.f, 0.f }, { 0.f, sy, 0.f }, { tx, ty, 1.f } };
}

vec2 RenderSystem::get_top_left()
{
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	gl_has_errors();

	Entity player = registry.players.top_entity();
	vec2 player_pos = MapUtility::map_position_to_world_position(registry.map_positions.get(player).position);
	
	Entity camera = registry.cameras.top_entity();
	MapPosition& camera_map_pos = registry.map_positions.get(camera);

	vec2 buffer_top_left, buffer_down_right;
	
	std::tie(buffer_top_left, buffer_down_right)
		= CameraUtility::get_buffer_positions(MapUtility::map_position_to_world_position(camera_map_pos.position), w * screen_scale, h * screen_scale);

	update_camera_position(camera_map_pos, player_pos, buffer_top_left, buffer_down_right);

	vec2 final_camera_pos = MapUtility::map_position_to_world_position(camera_map_pos.position);

	return { final_camera_pos.x, final_camera_pos.y};
}

 void RenderSystem::scale_on_scroll(float offset)
{
	 // scale the camera based on scrolling offset
	 // scrolling forward -> zoom in
	 // scrolling backward -> zoom out
	 // max: 1.0, min: 0.2
	float zoom = offset / 10;
	if (this->screen_scale - zoom > 0.1 && this->screen_scale - zoom <= 1.0) {
		this->screen_scale -= zoom;
	}
}

void RenderSystem::on_resize(int width, int height) {
	screen_size = { width, height };

	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screen_size.x, screen_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	gl_has_errors();
}

 // update camera's map position when player move out of buffer
 void RenderSystem::update_camera_position(MapPosition& camera_map_pos,
								 const vec2& player_pos,
								 const vec2& buffer_top_left,
								 const vec2& buffer_down_right)
{
	vec2 offset_top_left = player_pos - buffer_top_left;
	vec2 offset_down_right = player_pos - buffer_down_right;

	if (offset_top_left.x >= 0 && offset_top_left.y >= 0 && offset_down_right.x <= 0 && offset_down_right.y <= 0) {
		return;
	}

	if (offset_top_left.x < 0 && camera_map_pos.position.x > CameraUtility::map_top_left) {
		camera_map_pos.position.x -= 1;
	}

	if (offset_top_left.y < 0 && camera_map_pos.position.y > CameraUtility::map_top_left) {
		camera_map_pos.position.y -= 1;
	}

	if (offset_down_right.x > 0 && camera_map_pos.position.x < CameraUtility::map_down_right) {
		camera_map_pos.position.x += 1;
	}

	if (offset_down_right.y > 0 && camera_map_pos.position.y < CameraUtility::map_down_right) {
		camera_map_pos.position.y += 1;
	}

}