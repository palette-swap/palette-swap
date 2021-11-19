// internal
#include "render_system.hpp"
#include <SDL.h>
#pragma warning(push)
#pragma warning(disable : 4201) // nameless struct; glm only uses it if it exists
#include <glm/gtc/type_ptr.hpp> // Allows nice passing of values to GL functions
#pragma warning(pop)

Transform RenderSystem::get_transform(Entity entity)
{
	Transform transform;
	if (registry.any_of<MapPosition>(entity)) {
		MapPosition& map_position = registry.get<MapPosition>(entity);
		transform.translate(MapUtility::map_position_to_world_position(map_position.position));
	} else if (registry.any_of<WorldPosition>(entity)) {
		// Most objects in the game are expected to use MapPosition, exceptions are:
		// Arrow, Room.
		transform.translate(registry.get<WorldPosition>(entity).position);
		if (registry.any_of<Velocity>(entity)) {
			// Probably can provide a get if exist function here to boost performance
			transform.rotate(registry.get<Velocity>(entity).angle);
		}
	} else {
		transform.translate(
			screen_position_to_world_position(registry.get<ScreenPosition>(entity).position * vec2(screen_size)));
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
	glVertexAttribPointer(
		in_texcoord_loc,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(TexturedVertex),
		(void*)sizeof(vec3)); // NOLINT(performance-no-int-to-ptr,cppcoreguidelines-pro-type-cstyle-cast)
	// Enabling and binding texture to slot 0
	glActiveTexture(GL_TEXTURE0);
	gl_has_errors();

	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();
}

RenderSystem::TextData RenderSystem::generate_text(const Text& text)
{
	TextData text_data = {};
	// Texture creation
	glGenTextures(1, &(text_data.texture));

	// Check if we have the font in the right size, otherwise load it
	TTF_Font* font = nullptr;
	auto font_itr = fonts.find(text.font_size);
	if (font_itr != fonts.end()) {
		font = font_itr->second;
	} else {
		font = fonts.emplace(text.font_size, TTF_OpenFont(fonts_path("VT323-Regular.ttf").c_str(), text.font_size))
				   .first->second;
	}

	// Render the text using SDL
	SDL_Surface* surface
		= TTF_RenderText_Blended_Wrapped(font, text.text.c_str(), SDL_Color({ 255, 255, 255, 0 }), screen_size.x);
	SDL_LockSurface(surface);
	text_data.texture_width = surface->w;
	text_data.texture_height = surface->h;
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
				 text_data.texture_width,
				 text_data.texture_height,
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
	return text_data;
}

void RenderSystem::draw_textured_mesh(Entity entity, const RenderRequest& render_request, const mat3& projection)
{
	Transform transform = get_transform(entity);

	transform.scale(scaling_factors.at(static_cast<int>(render_request.used_texture)));

	const auto used_effect_enum = (GLuint)render_request.used_effect;
	assert(used_effect_enum != (GLuint)EFFECT_ASSET_ID::EFFECT_COUNT);
	const auto program = (GLuint)effects.at(used_effect_enum);

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	assert(render_request.used_geometry != GEOMETRY_BUFFER_ID::GEOMETRY_COUNT);
	int vbo_ibo_offset = 0;

	const GLuint vbo = vertex_buffers.at((int)render_request.used_geometry + vbo_ibo_offset);
	const GLuint ibo = index_buffers.at((int)render_request.used_geometry + vbo_ibo_offset);

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	// Input data location as in the vertex buffer
	if (render_request.used_effect == EFFECT_ASSET_ID::TEXTURED) {

		assert(registry.any_of<RenderRequest>(entity));
		prepare_for_textured(texture_gl_handles.at((GLuint)registry.get<RenderRequest>(entity).used_texture));
	} else if (render_request.used_effect == EFFECT_ASSET_ID::ENEMY
			   || render_request.used_effect == EFFECT_ASSET_ID::PLAYER) {

		GLint in_position_loc = glGetAttribLocation(program, "in_position");
		GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
		gl_has_errors();
		assert(in_texcoord_loc >= 0);

		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(SmallSpriteVertex), nullptr);
		gl_has_errors();

		glEnableVertexAttribArray(in_texcoord_loc);
		glVertexAttribPointer(
			in_texcoord_loc,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(SmallSpriteVertex),
			(void*)sizeof(vec3)); // NOLINT(performance-no-int-to-ptr,cppcoreguidelines-pro-type-cstyle-cast)
		// Enabling and binding texture to slot 0
		glActiveTexture(GL_TEXTURE0);
		gl_has_errors();

		Animation& animation = registry.get<Animation>(entity);
		assert(registry.any_of<Animation>(entity));

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

		GLuint texture_id = texture_gl_handles.at((GLuint)render_request.used_texture);

		glBindTexture(GL_TEXTURE_2D, texture_id);
		gl_has_errors();

	} else {
		assert(false && "Type of render request not supported");
	}

	// Getting uniform locations for glUniform* calls
	if (registry.any_of<Animation>(entity)) {
		GLint color_uloc = glGetUniformLocation(program, "fcolor");
		GLint opacity_uloc = glGetUniformLocation(program, "opacity");
		const vec4 color = registry.get<Animation>(entity).display_color;
		glUniform3fv(color_uloc, 1, glm::value_ptr(color));
		glUniform1f(opacity_uloc, color.w);
		gl_has_errors();
	} else if (registry.any_of<Color>(entity)) {
		GLint color_uloc = glGetUniformLocation(program, "fcolor");
		const vec3 color = registry.get<Color>(entity).color;
		glUniform3fv(color_uloc, 1, glm::value_ptr(color));
		gl_has_errors();
	}

	draw_triangles(transform, projection);
}

void RenderSystem::draw_ui_element(Entity entity, const UIRenderRequest& ui_render_request, const mat3& projection)
{
	Transform transform = get_transform(entity);
	transform.scale(ui_render_request.size * screen_scale * vec2(window_width_px, window_height_px)
					* ((registry.any_of<Background>(entity)) ? vec2(screen_size) / vec2(window_default_size)
															 : vec2(get_ui_scale_factor())));
	transform.translate(vec2((float)ui_render_request.alignment_x * .5, (float)ui_render_request.alignment_y * .5));
	if (ui_render_request.used_effect == EFFECT_ASSET_ID::FANCY_HEALTH) {
		transform.translate(vec2(-.5f, 0));
		draw_stat_bar(transform,
					   registry.get<Stats>(registry.view<Player>().front()),
					   projection,
					   true,
					   ui_render_request.size.x / ui_render_request.size.y * 2.f,
					   entity);
	} else if (ui_render_request.used_effect == EFFECT_ASSET_ID::RECTANGLE) {
		draw_rectangle(entity, transform, ui_render_request.size * vec2(window_width_px, window_height_px), projection);
	}
}

void RenderSystem::draw_stat_bar(
	Transform transform, const Stats& stats, const mat3& projection, bool fancy, float ratio = 1.f, Entity entity = entt::null)
{
	const auto program = (fancy) ? (GLuint)effects.at((uint8)EFFECT_ASSET_ID::FANCY_HEALTH)
								 : (GLuint)effects.at((uint8)EFFECT_ASSET_ID::HEALTH);

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
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), nullptr);
	gl_has_errors();

	glEnableVertexAttribArray(in_color_loc);
	glVertexAttribPointer(
		in_color_loc,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(ColoredVertex),
		(void*)sizeof(vec3)); // NOLINT(performance-no-int-to-ptr,cppcoreguidelines-pro-type-cstyle-cast)
	gl_has_errors();

	GLint health_loc = glGetUniformLocation(program, "health");
	float percentage = 1.f;
	if (fancy && registry.get<TargettedBar>(entity).target == BarType::Mana) {
		percentage = max(static_cast<float>(stats.mana), 0.f) / static_cast<float>(stats.mana_max);
	} else {
		percentage = max(static_cast<float>(stats.health), 0.f) / static_cast<float>(stats.health_max);
	}
	glUniform1f(health_loc, percentage);

	if (fancy) {
		GLint xy_ratio_loc = glGetUniformLocation(program, "xy_ratio");
		glUniform1f(xy_ratio_loc, ratio);

		// Setup coloring
		if (registry.any_of<Color>(entity)) {
			GLint color_uloc = glGetUniformLocation(program, "fcolor");
			const vec3 color = registry.get<Color>(entity).color;
			glUniform3fv(color_uloc, 1, glm::value_ptr(color));
			gl_has_errors();
		}
	}

	draw_triangles(transform, projection);
}

void RenderSystem::draw_rectangle(Entity entity, Transform transform, vec2 scale, const mat3& projection)
{
	const auto program = (GLuint)effects.at((uint8)EFFECT_ASSET_ID::RECTANGLE);

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	const GLuint vbo = vertex_buffers.at((int)GEOMETRY_BUFFER_ID::LINE);
	const GLuint ibo = index_buffers.at((int)GEOMETRY_BUFFER_ID::LINE);

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	gl_has_errors();

	GLint in_color_loc = glGetAttribLocation(program, "in_color");
	gl_has_errors();

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), nullptr);
	gl_has_errors();

	glEnableVertexAttribArray(in_color_loc);
	glVertexAttribPointer(
		in_color_loc,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(ColoredVertex),
		(void*)sizeof(vec3)); // NOLINT(performance-no-int-to-ptr,cppcoreguidelines-pro-type-cstyle-cast)
	gl_has_errors();

	GLint scale_loc = glGetUniformLocation(program, "scale");
	glUniform2f(scale_loc, scale.x, scale.y);

	GLint thickness_loc = glGetUniformLocation(program, "thickness");
	glUniform1f(thickness_loc, 6);

	// Setup coloring
	vec4 color = vec4(1);
	vec4 fill_color = vec4(0);
	if (registry.any_of<Color>(entity)) {
		color = vec4(registry.get<Color>(entity).color, 1.f);
	}
	if (registry.any_of<UIRectangle>(entity)) {
		UIRectangle& rect = registry.get<UIRectangle>(entity);
		color = vec4(vec3(color), rect.opacity);
		fill_color = rect.fill_color;
	}

	GLint fill_color_uloc = glGetUniformLocation(program, "fcolor_fill");
	glUniform4fv(fill_color_uloc, 1, glm::value_ptr(fill_color));
	gl_has_errors();

	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	glUniform4fv(color_uloc, 1, glm::value_ptr(color));
	gl_has_errors();

	draw_triangles(transform, projection);
}

void RenderSystem::draw_text(Entity entity, const Text& text, const mat3& projection)
{
	if (text.text.empty()) {
		return;
	}

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

	// Scale to expected pixel size, apply screen scale so not affected by zoom
	transform.scale(vec2(text_data->second.texture_width, text_data->second.texture_height) * screen_scale
					* get_ui_scale_factor());

	// Shift according to desired alignment using fancy enum wizardry
	transform.translate({ (float)text.alignment_x * .5, (float)text.alignment_y * .5 });

	const GLuint vbo = vertex_buffers.at((int)GEOMETRY_BUFFER_ID::SPRITE);
	const GLuint ibo = index_buffers.at((int)GEOMETRY_BUFFER_ID::SPRITE);

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	prepare_for_textured(text_data->second.texture);

	// Setup coloring
	if (registry.any_of<Color>(entity)) {
		GLint color_uloc = glGetUniformLocation(program, "fcolor");
		const vec3 color = registry.get<Color>(entity).color;
		glUniform3fv(color_uloc, 1, glm::value_ptr(color));
		gl_has_errors();
	}

	draw_triangles(transform, projection);
}

void RenderSystem::draw_line(Entity entity, const Line& line, const mat3& projection)
{
	Transform transform = get_transform(entity);
	transform.rotate(line.angle);
	transform.scale(line.scale);

	const auto program = (GLuint)effects.at((uint8)EFFECT_ASSET_ID::LINE);

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	const GLuint vbo = vertex_buffers.at((int)GEOMETRY_BUFFER_ID::DEBUG_LINE);
	const GLuint ibo = index_buffers.at((int)GEOMETRY_BUFFER_ID::DEBUG_LINE);

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	GLint in_color_loc = glGetAttribLocation(program, "in_color");
	gl_has_errors();

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), nullptr);
	gl_has_errors();

	glEnableVertexAttribArray(in_color_loc);
	glVertexAttribPointer(
		in_color_loc,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(ColoredVertex),
		((void*)sizeof(vec3))); // NOLINT(performance-no-int-to-ptr,cppcoreguidelines-pro-type-cstyle-cast)
	gl_has_errors();

	// Setup coloring
	if (registry.any_of<Color>(entity)) {
		GLint color_uloc = glGetUniformLocation(program, "fcolor");
		const vec3 color = registry.get<Color>(entity).color;
		glUniform3fv(color_uloc, 1, glm::value_ptr(color));
		gl_has_errors();
	}

	draw_triangles(transform, projection);
}

void RenderSystem::draw_map(const mat3& projection)
{
	const auto program = (GLuint)effects.at((uint8)EFFECT_ASSET_ID::TILE_MAP);
	gl_has_errors();

	const GLuint vbo = vertex_buffers.at((int)GEOMETRY_BUFFER_ID::ROOM);
	const GLuint ibo = index_buffers.at((int)GEOMETRY_BUFFER_ID::ROOM);

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	glUseProgram(program);
	for (auto [entity, room] : registry.view<Room>().each()) {
		Transform transform = get_transform(entity);
		transform.scale(scaling_factors.at(static_cast<int>(TEXTURE_ASSET_ID::TILE_SET)));

		glActiveTexture(GL_TEXTURE0);
		gl_has_errors();
		GLuint texture_id = texture_gl_handles.at((GLuint)TEXTURE_ASSET_ID::TILE_SET);
		glBindTexture(GL_TEXTURE_2D, texture_id);
		gl_has_errors();

		const auto& room_layout = map_generator->get_room_layout(room.level, room.room_id);
		GLint room_layout_loc = glGetUniformLocation(program, "room_layout");
		glUniform1uiv(room_layout_loc, (GLsizei)room_layout.size(), room_layout.data());

		GLint vertex_id_loc = glGetAttribLocation(program, "cur_vertex_id");
		glEnableVertexAttribArray(vertex_id_loc);
		glVertexAttribIPointer(vertex_id_loc, 1, GL_INT, sizeof(int), nullptr);
		gl_has_errors();

		draw_triangles(transform, projection);
	}
}

void RenderSystem::draw_triangles(const Transform& transform, const mat3& projection)
{
	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	auto num_indices = static_cast<GLsizei>(size / sizeof(uint16_t));

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
	ScreenState& screen = registry.get<ScreenState>(screen_state_entity);
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
	glViewport(0, 0, screen_size_capped.x, screen_size_capped.y);
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

	draw_map(projection_2d);

	// Draw any backgrounds
	for (auto [entity, request] : registry.view<Background, RenderRequest>().each()) {
		if (request.visible) {
			draw_textured_mesh(entity, request, projection_2d);
		}
	}

	// Grabs player's perception of which colour is "inactive"
	Entity player = registry.view<Player>().front();
	PlayerInactivePerception& player_perception = registry.get<PlayerInactivePerception>(player);
	ColorState& inactive_color = player_perception.inactive;

	auto render_requests_lambda = [&](Entity entity, RenderRequest& render_request) {
		if (render_request.visible) {
			draw_textured_mesh(entity, render_request, projection_2d);
		}
	};

	auto health_group_lambda = [&](Entity entity, Stats& stats, Enemy& /*enemy*/) {
		Transform transform = get_transform(entity);
		transform.translate(vec2(2 - MapUtility::tile_size / 2, -MapUtility::tile_size / 2));
		transform.scale(vec2(MapUtility::tile_size - 4, 3));
		draw_stat_bar(transform, stats, projection_2d, false);
	};

	// Renders entities + healthbars depending on which state we are in
	if (inactive_color == ColorState::Red) {
		registry.view<RenderRequest>(entt::exclude<Background, RedExclusive>).each(render_requests_lambda);
		registry.view<Stats, Enemy>(entt::exclude<RedExclusive>).each(health_group_lambda);
	} else if (inactive_color == ColorState::Blue) {
		registry.view<RenderRequest>(entt::exclude<Background, BlueExclusive>).each(render_requests_lambda);
		registry.view<Stats, Enemy>(entt::exclude<BlueExclusive>).each(health_group_lambda);
	} else {
		registry.view<RenderRequest>().each(render_requests_lambda);
		registry.view<Stats, Enemy>().each(health_group_lambda);
	}

	draw_ui(projection_2d);

	// Truely render to the screen
	draw_to_screen();

	// flicker-free display with a double buffer
	glfwSwapBuffers(window);
	gl_has_errors();
}

void RenderSystem::draw_ui(const mat3& projection)
{
	// Draw any UI backgrounds
	for (auto [entity, element, request] : registry.view<Background, UIElement, UIRenderRequest>().each()) {
		if (registry.get<UIGroup>(element.group).visible) {
			draw_ui_element(entity, request, projection);
		}
	}

	// Draw rectangles, lines, etc.
	for (auto [entity, group] : registry.view<UIGroup>().each()) {
		if (!group.visible) {
			continue;
		}
		Entity curr = group.first_element;
		while (curr != entt::null) {
			UIElement& element = registry.get<UIElement>(curr);
			if (element.visible) {
				if (UIRenderRequest* ui_render_request = registry.try_get<UIRenderRequest>(curr)) {
					draw_ui_element(curr, *ui_render_request, projection);
				} else if (Line* line = registry.try_get<Line>(curr)) {
					draw_line(entity, *line, projection);
				}
			}
			curr = element.next;
		}
	}

	// Draw text over top
	for (auto [entity, group] : registry.view<UIGroup>().each()) {
		if (!group.visible) {
			continue;
		}
		Entity curr = group.first_text;
		while (curr != entt::null) {
			UIElement& element = registry.get<UIElement>(curr);
			if (element.visible) {
				draw_text(curr, registry.get<Text>(curr), projection);
			}
			curr = element.next;
		}
	}

	// Finally, draw debug lines over absolutely everything
	for (auto [entity, line] : registry.view<Line>(entt::exclude<UIElement>).each()) {
		draw_line(entity, line, projection);
	}
}

// projection matrix based on position of camera entity
mat3 RenderSystem::create_projection_matrix()
{
	// Fake projection matrix, scales with respect to window coordinates

	vec2 top_left, bottom_right;
	std::tie(top_left, bottom_right) = get_window_bounds();

	vec2 scaled_screen = vec2(screen_size) * screen_scale;

	float sx = 2.f / (scaled_screen.x);
	float sy = -2.f / (scaled_screen.y);
	float tx = -(bottom_right.x + top_left.x) / (scaled_screen.x);
	float ty = (bottom_right.y + top_left.y) / (scaled_screen.y);
	return { { sx, 0.f, 0.f }, { 0.f, sy, 0.f }, { tx, ty, 1.f } };
}

std::pair<vec2, vec2> RenderSystem::get_window_bounds()
{
	vec2 window_size = vec2(screen_size) * screen_scale;

	Entity player = registry.view<Player>().front();
	vec2 player_pos = MapUtility::map_position_to_world_position(registry.get<MapPosition>(player).position);

	Entity camera = registry.view<Camera>().front();
	WorldPosition& camera_world_pos = registry.get<WorldPosition>(camera);

	vec2 buffer_top_left, buffer_down_right;

	std::tie(buffer_top_left, buffer_down_right)
		= CameraUtility::get_buffer_positions(camera_world_pos.position, window_size.x, window_size.y);

	update_camera_position(camera_world_pos, player_pos, buffer_top_left, buffer_down_right);

	return { camera_world_pos.position - window_size / 2.f, camera_world_pos.position + window_size / 2.f };
}

float RenderSystem::get_ui_scale_factor() const
{
	vec2 ratios = vec2(screen_size) / vec2(window_default_size);
	return min(ratios.x, ratios.y);
}

vec2 RenderSystem::screen_position_to_world_position(vec2 screen_pos)
{
	return screen_pos * screen_scale + get_window_bounds().first;
}

void RenderSystem::scale_on_scroll(float offset)
{
	// scale the camera based on scrolling offset
	// scrolling forward -> zoom in
	// scrolling backward -> zoom out
	// max: 1.0, min: 0.2
	float zoom = offset / 10;
	if (debugging.in_debug_mode || (this->screen_scale - zoom > 0.1 && this->screen_scale - zoom <= 1.0)) {
		this->screen_scale -= zoom;
	}
}

void RenderSystem::on_resize(int width, int height)
{
	screen_size = { width, height };
	screen_size_capped = { min(width, window_width_px), min(height, window_height_px) };

	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA, screen_size_capped.x, screen_size_capped.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	gl_has_errors();
}

// update camera's map position when player move out of buffer
void RenderSystem::update_camera_position(WorldPosition& camera_world_pos,
										  const vec2& player_pos,
										  const vec2& buffer_top_left,
										  const vec2& buffer_down_right)
{
	vec2 offset_top_left = player_pos - buffer_top_left;
	vec2 offset_down_right = player_pos - buffer_down_right;
	vec2 map_top_left = MapUtility::map_position_to_world_position(MapUtility::map_top_left);
	vec2 map_bottom_right = MapUtility::map_position_to_world_position(MapUtility::map_down_right);

	camera_world_pos.position
		= max(min(camera_world_pos.position, camera_world_pos.position + offset_top_left), map_top_left);
	camera_world_pos.position
		= min(max(camera_world_pos.position, camera_world_pos.position + offset_down_right), map_bottom_right);
}