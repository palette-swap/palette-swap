// internal
#include "render_system.hpp"
#include <SDL.h>
#pragma warning(push)
#pragma warning(disable : 4201) // nameless struct; glm only uses it if it exists
#include <glm/gtc/type_ptr.hpp> // Allows nice passing of values to GL functions
#pragma warning(pop)

Transform RenderSystem::get_transform(Entity entity, uvec2* tile) const
{
	Transform transform;
	if (WorldPosition* world_pos = registry.try_get<WorldPosition>(entity)) {
		// Most objects in the game are expected to use MapPosition, exceptions are:
		// Arrow, Room.
		transform.translate(world_pos->position);
		if (Velocity* velocity = registry.try_get<Velocity>(entity)) {
			// Probably can provide a get if exist function here to boost performance
			transform.rotate(velocity->angle);
		}
		if (tile != nullptr) {
			*tile = MapUtility::world_position_to_map_position(world_pos->position);
		}
	} else if (MapPosition* map_pos = registry.try_get<MapPosition>(entity)) {
		transform.translate(MapUtility::map_position_to_world_position(map_pos->position));
		if (tile != nullptr) {
			*tile = map_pos->position;
		}
	} else {
		transform.translate(screen_position_to_world_position(registry.get<ScreenPosition>(entity).position));
	}
	return transform;
}

Transform RenderSystem::get_transform_no_rotation(Entity entity) const
{
	Transform transform;
	if (registry.any_of<MapPosition>(entity)) {
		MapPosition& map_position = registry.get<MapPosition>(entity);
		transform.translate(MapUtility::map_position_to_world_position(map_position.position));
	} else if (registry.any_of<WorldPosition>(entity)) {
		// Most objects in the game are expected to use MapPosition, exceptions are:
		// Arrow, Room.
		transform.translate(registry.get<WorldPosition>(entity).position);
	} else {
		transform.translate(screen_position_to_world_position(registry.get<ScreenPosition>(entity).position));
	}
	return transform;
}

void RenderSystem::prepare_buffer(vec3 color) const
{
	// Clearing backbuffer
	glViewport(0, 0, (GLsizei)screen_size_capped().x, (GLsizei)screen_size_capped().y);
	glDepthRange(0.00001, 10);
	glClearColor(color.r, color.g, color.b, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST); // native OpenGL does not work with a depth buffer
							  // and alpha blending, one would have to sort
							  // sprites back to front
	gl_has_errors();
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

void RenderSystem::prepare_for_spritesheet(TEXTURE_ASSET_ID texture, vec2 offset, vec2 size)
{
	const auto program = (GLuint)effects.at((GLuint)EFFECT_ASSET_ID::SPRITESHEET);

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	const GLuint vbo = vertex_buffers.at((int)GEOMETRY_BUFFER_ID::SPRITE);
	const GLuint ibo = index_buffers.at((int)GEOMETRY_BUFFER_ID::SPRITE);

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();
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

	GLint offset_loc = glGetUniformLocation(program, "offset");
	glUniform2f(offset_loc, offset.x, offset.y);
	gl_has_errors();

	GLint size_loc = glGetUniformLocation(program, "size");
	glUniform2f(size_loc, size.x,size.y);
	gl_has_errors();

	GLuint texture_id = texture_gl_handles.at((GLuint)texture);
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

	const std::string& string = text.text;

	size_t start = 0;
	size_t end = string.find('\n');
	std::vector<SDL_Surface*> surfaces;
	while (true) {
		if (start == end) {
			start = end + 1;
			end = string.find('\n', start);
			continue;
		}

		// Render the text using SDL
		SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(
			font, string.substr(start, (end == -1) ? end : end - start).c_str(), SDL_Color({ 255, 255, 255, 255 }), static_cast<Uint32>(screen_size.x));
		if (surface == nullptr) {
			fprintf(stderr, "Error TTF_RenderText %s\n", text.text.c_str());
			break;
			//return text_data;
		}
		text_data.texture_width = max(text_data.texture_width, surface->w);
		text_data.texture_height += surface->h;
		surfaces.push_back(surface);

		if (end == -1) {
			break;
		}
		start = end + 1;
		end = string.find('\n', start);
	}

	void* pixels = nullptr;

	if (surfaces.size() == 1) {
		SDL_LockSurface(surfaces.at(0));
		pixels = surfaces.at(0)->pixels;
	} else {
		// Combine lines of text into one larger texture
		auto* texture = new uint32[text_data.texture_width * text_data.texture_height];
		int curr_height = 0;
		for (auto* surface : surfaces) {
			SDL_LockSurface(surface);
			for (int y = 0; y < surface->h; y++) {
				// The fix to this lint complaint requires C++ 20, don't really want to go there just yet
				for (int x = 0; x < surface->w; x++) {
					texture[x + (y + curr_height) * text_data.texture_width]
						= static_cast<uint32*>(surface->pixels)[x + y * surface->w];
				}
				for (int x = surface->w; x < text_data.texture_width; x++) {
					texture[x + (y + curr_height) * text_data.texture_width] = 0;
				}
			}
			curr_height += surface->h;
			SDL_FreeSurface(surface);
		}
		pixels = texture;
	}

	// Extract the SDL image data into the texture
	glBindTexture(GL_TEXTURE_2D, text_data.texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D,
				 0,
				 GL_RGBA,
				 text_data.texture_width,
				 text_data.texture_height,
				 0,
				 GL_BGRA,
				 GL_UNSIGNED_INT_8_8_8_8_REV,
				 pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glBindTexture(GL_TEXTURE_2D, 0);
	gl_has_errors();

	// Free single surface
	if (surfaces.size() == 1) {
		SDL_FreeSurface(surfaces.at(0));
	}
	return text_data;
}

void RenderSystem::draw_textured_mesh(Entity entity, const RenderRequest& render_request, const mat3& projection)
{
	uvec2 tile;
	Transform transform = get_transform(entity, &tile);

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
	} else if (render_request.used_effect == EFFECT_ASSET_ID::SPRITESHEET) {

		TextureOffset& texture_offset = registry.get<TextureOffset>(entity);
		vec2 shifted_offset = vec2(texture_offset.offset) * MapUtility::tile_size;
		prepare_for_spritesheet(render_request.used_texture, shifted_offset, texture_offset.size);

	} else if (render_request.used_effect == EFFECT_ASSET_ID::ENEMY
			   || render_request.used_effect == EFFECT_ASSET_ID::PLAYER
			   || render_request.used_effect == EFFECT_ASSET_ID::BOSS_INTRO_SHADER) {

		if (!lighting.is_visible(tile)) {
			return;
		}

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

	prepare_for_lit_entity(program);

	draw_triangles(transform, projection);
}

void RenderSystem::draw_effect(Entity entity, const EffectRenderRequest& render_request, const mat3& projection)
{
	Transform transform = get_transform_no_rotation(entity);

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

		// Updates time in shader program
		GLint time_uloc = glGetUniformLocation(program, "time");
		glUniform1f(time_uloc, (float)(glfwGetTime() * 10.0f));

		// Updates frame for entity
		GLint frame_loc = glGetUniformLocation(program, "frame");
		glUniform1i(frame_loc, animation.frame);
		gl_has_errors();

		// Updates frame for entity
		GLint state_loc = glGetUniformLocation(program, "state");
		glUniform1i(state_loc, animation.state);

	if (render_request.used_effect == EFFECT_ASSET_ID::SPELL) {
		// Updates frame for entity
		GLint spell_loc = glGetUniformLocation(program, "spelltype");
		glUniform1i(spell_loc, animation.state);
	}

	if (render_request.used_effect == EFFECT_ASSET_ID::AOE) {
		
		AOESquare& aoe_status = registry.get<AOESquare>(entity);
		if (!aoe_status.actual_attack_displayed) {
			transform.scale(vec2(1 / 3.f, 1 / 3.f));
		}
		GLint actual_aoe = glGetUniformLocation(program, "actual_aoe");
		glUniform1i(actual_aoe, static_cast<GLint>(aoe_status.actual_attack_displayed));
	}
	if (render_request.used_effect == EFFECT_ASSET_ID::DEATH) {
		transform.scale({ animation.direction, 1 });
	}

	GLuint texture_id = texture_gl_handles.at((GLuint)render_request.used_texture);

	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

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
	transform.rotate(ui_render_request.angle);
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
	} else if (ui_render_request.used_effect == EFFECT_ASSET_ID::RECTANGLE
			   || ui_render_request.used_effect == EFFECT_ASSET_ID::OVAL) {
		draw_rectangle(ui_render_request.used_effect,
					   entity,
					   transform,
					   ui_render_request.size * vec2(window_width_px, window_height_px),
					   projection);
	} else if (ui_render_request.used_effect == EFFECT_ASSET_ID::SPRITESHEET) {

		TextureOffset& texture_offset = registry.get<TextureOffset>(entity);

		vec2 shifted_offset = vec2(texture_offset.offset) * MapUtility::tile_size;

		prepare_for_spritesheet(ui_render_request.used_texture, shifted_offset, texture_offset.size);

		if (registry.any_of<Color>(entity)) {
			GLint color_uloc = glGetUniformLocation((GLuint)effects.at((GLuint)EFFECT_ASSET_ID::SPRITESHEET), "fcolor");
			const vec3 color = registry.get<Color>(entity).color;
			glUniform3fv(color_uloc, 1, glm::value_ptr(color));
			gl_has_errors();
		}

		draw_triangles(transform, projection);
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
	if (fancy && registry.any_of<TargettedBar>(entity) && registry.get<TargettedBar>(entity).target == BarType::Mana) {
		percentage = max(static_cast<float>(stats.mana), 0.f) / static_cast<float>(stats.mana_max);
	} else {
		percentage = max(static_cast<float>(stats.health), 0.f) / static_cast<float>(stats.health_max);
	}
	glUniform1f(health_loc, percentage);

	if (fancy) {
		GLint xy_ratio_loc = glGetUniformLocation(program, "xy_ratio");
		glUniform1f(xy_ratio_loc, ratio);

		// Setup coloring
		GLint color_uloc = glGetUniformLocation(program, "fcolor");
		vec3 color = vec3(.8, .1, .1);
		if (registry.any_of<Color>(entity) && !registry.any_of<MapHitbox>(entity)) {
			color = registry.get<Color>(entity).color;
		}
		glUniform3fv(color_uloc, 1, glm::value_ptr(color));
		gl_has_errors();
	}

	prepare_for_lit_entity(program);

	draw_triangles(transform, projection);
}

void RenderSystem::draw_rectangle(EFFECT_ASSET_ID asset, Entity entity, Transform transform, vec2 scale, const mat3& projection)
{
	const auto program = (GLuint)effects.at((uint8)asset);

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
	glUniform1f(thickness_loc, asset == EFFECT_ASSET_ID::RECTANGLE ? 6 : 4);

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

	const auto program = (GLuint)effects.at((text.border > 0) ? (uint8)EFFECT_ASSET_ID::TEXT_BUBBLE : (uint8)EFFECT_ASSET_ID::TEXTURED);

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	auto text_data = text_buffers.find(text);

	if (text_data == text_buffers.end()) {
		TextData new_text_data = generate_text(text);
		text_data = text_buffers.emplace(text, new_text_data).first;
	}

	// Scale to expected pixel size, apply screen scale so not affected by zoom
	vec2 text_size = vec2(text_data->second.texture_width, text_data->second.texture_height) + 2.f * static_cast<float>(text.border);
	transform.scale(text_size * screen_scale
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

	if (text.border > 0) {
		GLint border_uloc = glGetUniformLocation(program, "fborder");
		glUniform1ui(border_uloc, static_cast<GLuint>(text.border));
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

void RenderSystem::draw_map(const mat3& projection, ColorState color)
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

	glActiveTexture(GL_TEXTURE0);
	gl_has_errors();
	TEXTURE_ASSET_ID tex
		= (color == ColorState::Blue) ? TEXTURE_ASSET_ID::TILE_SET_BLUE : TEXTURE_ASSET_ID::TILE_SET_RED;
	GLuint texture_id = texture_gl_handles.at((GLuint)tex);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();
	for (auto [entity, room] : registry.view<Room>().each()) {
		if (use_lighting && !room.visible) {
			continue;
		}

		Transform transform = get_transform(entity);
		transform.scale(scaling_factors.at(static_cast<int>(tex)));

		const auto& room_layout = map_generator->get_room_layout(room.level, room.room_id);
		GLint room_layout_loc = glGetUniformLocation(program, "room_layout");
		glUniform1uiv(room_layout_loc, (GLsizei)room_layout.size(), room_layout.data());

		GLint vertex_id_loc = glGetAttribLocation(program, "cur_vertex_id");
		glEnableVertexAttribArray(vertex_id_loc);
		glVertexAttribIPointer(vertex_id_loc, 1, GL_INT, sizeof(int), nullptr);
		gl_has_errors();

		Animation& animation = registry.get<Animation>(entity);
		assert(registry.any_of<Animation>(entity));

		GLint frame_loc = glGetUniformLocation(program, "frame");
		glUniform1i(frame_loc, animation.frame);

		GLint appearing_loc = glGetUniformLocation(program, "appearing");
		bool appearing = false;
		if (RoomAnimation* appear = registry.try_get<RoomAnimation>(entity)) {
			GLint start_tile = glGetUniformLocation(program, "start_tile");
			glUniform2fv(start_tile, 1, glm::value_ptr(MapUtility::map_position_to_world_position(appear->start_tile)));

			GLint max_show_distance = glGetUniformLocation(program, "max_show_distance");
			glUniform1f(max_show_distance, appear->dist_per_second * (appear->elapsed_time / 1000.f));
			appearing = true;
		}
		glUniform1i(appearing_loc, static_cast<int>(appearing));

		draw_triangles(transform, projection);
	}
}

void RenderSystem::toggle_lighting() { use_lighting = !use_lighting; }

void RenderSystem::set_lighting(bool enabled) { use_lighting = enabled; }

void RenderSystem::prepare_for_lit_entity(GLuint program) const
{

	// Bind our textures in
	GLint texture_loc = glGetUniformLocation(program, "sampler0");
	GLint lighting_texture_loc = glGetUniformLocation(program, "lighting");
	GLint use_lighting_loc = glGetUniformLocation(program, "use_lighting");
	glUniform1i(texture_loc, 0);
	glUniform1i(lighting_texture_loc, 1);
	glUniform1i(use_lighting_loc, static_cast<int>(applying_lighting));

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, lighting_buffer_color);
}

void RenderSystem::create_lighting_texture(const mat3& projection)
{
	glBindFramebuffer(GL_FRAMEBUFFER, lighting_frame_buffer);
	gl_has_errors();
	prepare_buffer(vec3(0));

	for (auto [entity, light] : registry.view<Light>().each()) {
		draw_light(entity, light, projection);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
}

void RenderSystem::draw_light(Entity entity, const Light& light, const mat3& projection)
{
	Transform transform = get_transform(entity);

	transform.scale(vec2(2) * light.radius);
	const auto program = (GLuint)effects.at((uint8)EFFECT_ASSET_ID::LIGHT);

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

	// Setup coloring
	vec4 color = vec4(1);
	if (registry.any_of<Color>(entity)) {
		color = vec4(registry.get<Color>(entity).color, 1.f);
	}

	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	glUniform4fv(color_uloc, 1, glm::value_ptr(color));
	gl_has_errors();

	draw_triangles(transform, projection);
}

void RenderSystem::draw_lighting(const mat3& projection)
{

	glBindFramebuffer(GL_FRAMEBUFFER, los_frame_buffer);
	gl_has_errors();
	prepare_buffer(vec3(0));

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers.at((int)GEOMETRY_BUFFER_ID::LIGHTING_TRIANGLES));

	auto program = (GLuint)effects.at((uint8)EFFECT_ASSET_ID::LIGHT_TRIANGLES);
	gl_has_errors();

	std::vector<GLfloat> vertices;
	for (auto [entity, request] : registry.view<LightingTriangle>().each()) {
		for (uint i = 0; i < 2; i++) {
			vertices.push_back(request.p1[i]);
		}
		for (uint i = 0; i < 2; i++) {
			vertices.push_back(request.p2[i]);
		}
		for (uint i = 0; i < 2; i++) {
			vertices.push_back(request.p3[i]);
		}
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glUseProgram(program);

	GLint projection_loc = glGetUniformLocation(program, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));
	gl_has_errors();

	glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 2);
	gl_has_errors();
	glDisableVertexAttribArray(0);

	for (auto [entity] : registry.view<LightingTile>().each()) {
		Transform transform = get_transform(entity);
		transform.scale(MapUtility::tile_size * vec2(1));
		draw_rectangle(EFFECT_ASSET_ID::RECTANGLE, entity, transform, MapUtility::tile_size * vec2(1), projection);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

	// Draw the screen texture on the quad geometry
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
	glBindBuffer(
		GL_ELEMENT_ARRAY_BUFFER,
		index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
	gl_has_errors();

	// Apply Lighting
	program = (GLuint)effects.at((uint8)EFFECT_ASSET_ID::LIGHTING);

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	// Set the vertex position and vertex texture coordinates (both stored in the
	// same VBO)
	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr);
	gl_has_errors();

	// Bind our textures in
	GLint screen_texture_loc = glGetUniformLocation(program, "screen");
	GLint lighting_texture_loc = glGetUniformLocation(program, "lighting");
	GLint los_texture_loc = glGetUniformLocation(program, "los");
	glUniform1i(screen_texture_loc, 0);
	glUniform1i(lighting_texture_loc, 1);
	glUniform1i(los_texture_loc, 2);

	glActiveTexture(GL_TEXTURE0);
	gl_has_errors();
	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	gl_has_errors();

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, lighting_buffer_color);
	gl_has_errors();

	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, los_buffer_color);
	gl_has_errors();

	// Draw
	glDrawElements(GL_TRIANGLES,
				   3,
				   GL_UNSIGNED_SHORT,
				   nullptr); // one triangle = 3 vertices; nullptr indicates that there is
							 // no offset from the bound index buffer
	gl_has_errors();
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
	glViewport(0, 0, (GLsizei)screen_size.x, (GLsizei)screen_size.y);
	glDepthRange(0, 10);
	glClearColor(0, 0, 0, 1.0);
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
	// Grabs player's perception of which colour is "inactive"
	Entity player = registry.view<Player>().front();
	PlayerInactivePerception& player_perception = registry.get<PlayerInactivePerception>(player);
	ColorState& inactive_color = player_perception.inactive;

	// Reset lighting state
	applying_lighting = false;

	// First render to the custom framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();
	prepare_buffer((inactive_color == ColorState::Blue) ? vec3(7.f / 256, 6.f / 256, 6.f / 256)
														: vec3(7.f / 256, 6.f / 256, 6.f / 256));
	mat3 projection_2d = create_projection_matrix();

	draw_map(projection_2d, inactive_color == ColorState::Blue ? ColorState::Red : ColorState::Blue);

	// Draw any backgrounds
	for (auto [entity, request] : registry.view<Background, RenderRequest>().each()) {
		if (request.visible) {
			draw_textured_mesh(entity, request, projection_2d);
		}
	}

	if (use_lighting) {
		create_lighting_texture(projection_2d);
		// Start applying lighting to various entities
		applying_lighting = true;
	}

	auto render_requests_lambda = [&](Entity entity, RenderRequest& render_request) {
		if (render_request.visible) {
			draw_textured_mesh(entity, render_request, projection_2d);
		}
	};

	auto health_group_lambda = [&](Entity entity, RenderRequest& request, Stats& stats, Enemy& /*enemy*/) {
		if (!request.visible) {
			return;
		}
		uvec2 tile;
		Transform transform = get_transform(entity, &tile);
		if (!lighting.is_visible(tile)) {
			return;
		}
		vec2 shift = vec2(2 - MapUtility::tile_size / 2, -MapUtility::tile_size / 2);
		vec2 scale = vec2(MapUtility::tile_size - 4, 3);
		bool fancy = false;
		if (MapHitbox* hitbox = registry.try_get<MapHitbox>(entity)) {
			shift.x -= MapUtility::tile_size * hitbox->center.x;
			shift.y -= MapUtility::tile_size * hitbox->center.y;
			scale.x = MapUtility::tile_size * hitbox->area.x - 4;
			scale.y = min(9.f, hitbox->area.x * scale.y);
			fancy = true;
		}
		transform.translate(shift);
		transform.scale(scale);
		draw_stat_bar(transform, stats, projection_2d, fancy, scale.x / scale.y, entity);
	};

	// Renders entities + healthbars depending on which state we are in
	if (inactive_color == ColorState::Red) {
		registry.view<RenderRequest>(entt::exclude<Background, RedExclusive>).each(render_requests_lambda);
		registry.view<RenderRequest, Stats, Enemy>(entt::exclude<RedExclusive>).each(health_group_lambda);
	} else if (inactive_color == ColorState::Blue) {
		registry.view<RenderRequest>(entt::exclude<Background, BlueExclusive>).each(render_requests_lambda);
		registry.view<RenderRequest, Stats, Enemy>(entt::exclude<BlueExclusive>).each(health_group_lambda);
	} else {
		registry.view<RenderRequest>().each(render_requests_lambda);
		registry.view<RenderRequest, Stats, Enemy>().each(health_group_lambda);
	}

	// Renders effects (ie spells), intended to be overlayed on top of regular render effects
	for (auto [entity, effect_render_request] : registry.view<EffectRenderRequest>().each()) {
		// Note, its not very efficient to access elements indirectly via the entity
		// albeit iterating through all Sprites in sequence. A good point to optimize
		if (effect_render_request.visible) {
			draw_effect(entity, effect_render_request, projection_2d);
		}
	}

	if (use_lighting) {
		draw_lighting(projection_2d);

		// Done using lighting
		applying_lighting = false;
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

	auto draw_list = [&](Entity curr)
	{
		while (curr != entt::null) {
			UIElement& element = registry.get<UIElement>(curr);
			if (element.visible) {
				if (UIRenderRequest* ui_render_request = registry.try_get<UIRenderRequest>(curr)) {
					draw_ui_element(curr, *ui_render_request, projection);
				} else if (Line* line = registry.try_get<Line>(curr)) {
					draw_line(curr, *line, projection);
				} else if (Text* text = registry.try_get<Text>(curr)) {
					draw_text(curr, registry.get<Text>(curr), projection);
				}
			}
			curr = element.next;
		}
	};
	for (size_t layer = 0; layer < (size_t)UILayer::Count; layer++) {
		for (auto [entity, group] : registry.view<UIGroup>().each()) {
			if (!group.visible) {
				continue;
			}
			draw_list(group.first_elements.at(layer));
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

vec2 RenderSystem::mouse_pos_to_screen_pos(dvec2 mouse_pos) const
{
	vec2 screen_pos = vec2(mouse_pos) / vec2(screen_size);
	vec2 ratios = vec2(screen_size) / vec2(window_default_size);
	float shrink_factor = min(ratios.x, ratios.y) / max(ratios.x, ratios.y);
	if (ratios.x > ratios.y) {
		screen_pos.x = (screen_pos.x - (1 - shrink_factor) / 2.f) / shrink_factor;
	} else {
		screen_pos.y = (screen_pos.y - (1 - shrink_factor) / 2.f) / shrink_factor;
	}
	return screen_pos;
}

vec2 RenderSystem::screen_position_to_world_position(vec2 screen_pos) const
{
	vec2 ratios = vec2(screen_size) / vec2(window_default_size);
	float shrink_factor = min(ratios.x, ratios.y) / max(ratios.x, ratios.y);
	if (ratios.x > ratios.y) {
		screen_pos.x = screen_pos.x * shrink_factor + (1 - shrink_factor) / 2.f;
	} else {
		screen_pos.y = screen_pos.y * shrink_factor + (1 - shrink_factor) / 2.f;
	}
	return screen_pos * screen_scale * screen_size + get_window_bounds().first;
}

std::pair<vec2, vec2> RenderSystem::get_window_bounds() const
{
	vec2 window_size = vec2(screen_size) * screen_scale;

	Entity camera = registry.view<Camera>().front();
	WorldPosition& camera_world_pos = registry.get<WorldPosition>(camera);

	return { camera_world_pos.position - window_size / 2.f, camera_world_pos.position + window_size / 2.f };
}

float RenderSystem::get_ui_scale_factor() const
{
	vec2 ratios = vec2(screen_size) / vec2(window_default_size);
	return min(ratios.x, ratios.y);
}

void RenderSystem::scale_on_scroll(float offset)
{
	// scale the camera based on scrolling offset
	// scrolling forward -> zoom in
	// scrolling backward -> zoom out
	// max: 1.0, min: 0.2
	float zoom = offset / 10;
	if (debugging.in_debug_mode || (screen_scale - zoom > 0.1 && screen_scale - zoom <= 1.0)) {
		screen_scale -= zoom;
	}
}

void RenderSystem::on_resize(int width, int height)
{
	screen_size = { width, height };

	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	glTexImage2D(GL_TEXTURE_2D,
				 0,
				 GL_RGBA,
				 (GLsizei)screen_size_capped().x,
				 (GLsizei)screen_size_capped().y,
				 0,
				 GL_RGBA,
				 GL_UNSIGNED_BYTE,
				 nullptr);
	gl_has_errors();

	glBindTexture(GL_TEXTURE_2D, lighting_buffer_color);
	glTexImage2D(GL_TEXTURE_2D,
				 0,
				 GL_RGBA,
				 (GLsizei)screen_size_capped().x,
				 (GLsizei)screen_size_capped().y,
				 0,
				 GL_RGBA,
				 GL_UNSIGNED_BYTE,
				 nullptr);
	gl_has_errors();

	glBindTexture(GL_TEXTURE_2D, los_buffer_color);
	glTexImage2D(GL_TEXTURE_2D,
				 0,
				 GL_RGBA,
				 (GLsizei)screen_size_capped().x,
				 (GLsizei)screen_size_capped().y,
				 0,
				 GL_RGBA,
				 GL_UNSIGNED_BYTE,
				 nullptr);
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
