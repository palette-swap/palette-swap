#pragma once
#include <memory>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"
#include "tiny_ecs_registry.hpp"

// Total number of frames in a spritesheet. Currently used for some test spritesheets.
// Number of actual frames can be adapted to vary from asset to asset
// TODO: change number of frames for an animation to vary based on the asset

// Base animation speed for all animated entities
static constexpr int base_animation_speed = 150;

// Currently this number is used for all enemies animation frames (a total of 4)
// Moving forward the animation frames by enemy type may change
static constexpr int enemy_num_frames = 4;

// These values are used for the default setting for the player
static constexpr int player_num_frames = 6;
static constexpr int player_weapon_states = 2;
static constexpr float player_animation_speed = 1.2;
// Used for melee animation attack speeds for the player
static constexpr float player_melee_speed = 2.5;
static constexpr float player_heavy_melee_speed = 1;
static constexpr float player_running_speed = 1.8;
// Value denoting the animation states for the player
// KEEP ALIGNED WITH STATES REPRESENTED IN PLAYER SPRITESHEET
enum class player_animation_states { Idle = 0, Spellcast = 1, Melee = 2, Running = 3 };

static constexpr vec3 default_enemy_red = { 2, 1, 1 };
static constexpr vec3 default_enemy_blue = { 1, 1, 2 };


class AnimationSystem {

public:
	// Initializes the animation system
	void init();
	// Updates all animated entities based on elapsed time, changes their frame based on the time
	void update_animations(float elapsed_ms);	
	// Sets direction for an animated sprite, such that it faces either left or right
	void set_sprite_direction(Entity sprite, Sprite_Direction direction);

	// Sets an enemy entity to have the correct render texture, and color state
	void set_enemy_animation(Entity enemy, TEXTURE_ASSET_ID enemy_type, ColorState color);
	// triggers an enemy attack animation
	void enemy_attack_animation(Entity enemy);

	// initializes animation values for a player entity
	// NOTE: weird things will happen if the entity initialized as player is not a player
	void set_player_animation(Entity player);
	// Sets player's animation to idle
	void player_idle_animation(Entity player);
	// Sets player's animation to spellcast
	void player_spellcast_animation(Entity player);
	// Toggles player's weapon selection
	void AnimationSystem::player_toggle_weapon(Entity player);
	// TODO: Maybe generalize these two event animations to a general one
	// Triggers attack animation for a entity specified as the player
	// TODO: Add callback to confirm that a turn has ended
	void player_attack_animation(Entity player);
	// Triggers running animation for a entity specified as the player
	void player_running_animation(Entity player);

	// Triggers an animation to display that an entity has taken damage
	// For now, this is just a colour change. Will change in next version however
	void damage_animation(Entity entity);

	// Returns a boolean denoting whether or not all "irregular animations" such as attack
	// or damage calculations have been completed
	bool animation_events_completed();

private:
	// helper function, checks event animation components to see if they should be removed, and animation states should
	// be restored
	void resolve_event_animations();

};

