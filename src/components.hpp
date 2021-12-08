#pragma once
#include "../ext/stb_image/stb_image.h"
#include "common.hpp"

#include <array>
#include <map>
#include <unordered_map>

#include "../ext/stb_image/stb_image.h"
#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"

// Player component
struct Player {
};

// Data structure for toggling debug mode
struct Debug {
	bool in_debug_mode = false;
	bool in_freeze_mode = false;
};

//---------------------------------------------------------------------------
//-------------------------        Rendering        -------------------------
//---------------------------------------------------------------------------

// Camera component
struct Camera {
	uvec2 size, central;
};

// Sets the brightness of the screen
struct ScreenState {
	float darken_screen_factor = -1;
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent {
	// Note, an empty struct has size 1
};

// Single Vertex Buffer element for non-textured meshes (coloured.vs.glsl & salmon.vs.glsl)
struct ColoredVertex {
	vec3 position = { 0, 0, 0 };
	vec3 color = { 0, 0, 0 };
};

// Single Vertex Buffer element for textured sprites (textured.vs.glsl)
struct TexturedVertex {
	vec3 position;
	vec2 texcoord;
};

// Mesh datastructure for storing vertex and index buffers
struct Mesh {
	static bool load_from_obj_file(const std::string& obj_path,
								   std::vector<ColoredVertex>& out_vertices,
								   std::vector<uint16_t>& out_vertex_indices,
								   vec2& out_size);
	vec2 original_size = { 1, 1 };
	std::vector<ColoredVertex> vertices;
	std::vector<uint16_t> vertex_indices;
};

// Test Texture Buffer element for enemies
// TODO: change to animated vertices after bringing player into this 3D element group
struct SmallSpriteVertex {
	vec3 position;
	vec2 texcoord;
};

/**
 * The following enumerators represent global identifiers refering to graphic
 * assets. For example TEXTURE_ASSET_ID are the identifiers of each texture
 * currently supported by the system.
 *
 * So, instead of referring to a game asset directly, the game logic just
 * uses these enumerators and the RenderRequest struct to inform the renderer
 * how to structure the next draw command.
 *
 * There are 2 reasons for this:
 *
 * First, game assets such as textures and meshes are large and should not be
 * copied around as this wastes memory and runtime. Thus separating the data
 * from its representation makes the system faster.
 *
 * Second, it is good practice to decouple the game logic from the render logic.
 * Imagine, for example, changing from OpenGL to Vulkan, if the game logic
 * depends on OpenGL semantics it will be much harder to do the switch than if
 * the renderer encapsulates all asset data and the game logic is agnostic to it.
 *
 * The final value in each enumeration is both a way to keep track of how many
 * enums there are, and as a default value to represent uninitialized fields.
 */

enum class TEXTURE_ASSET_ID : uint8_t {
	PALADIN = 0,
	// Small Enemies
	DUMMY = PALADIN + 1,
	SLIME = DUMMY + 1,
	ARMOR = SLIME + 1,
	TREEANT = ARMOR + 1,
	RAVEN = TREEANT + 1,
	WRAITH = RAVEN + 1,
	DRAKE = WRAITH + 1,
	MUSHROOM = DRAKE + 1,
	SPIDER = MUSHROOM + 1,
	CLONE = SPIDER + 1,
	SWORD_SOLDIER = CLONE + 1,
	SPEAR_SOLDIER = SWORD_SOLDIER + 1,
	APPARITION = SPEAR_SOLDIER + 1,
	KOBOLD_WARRIOR = APPARITION + 1,
	KOBOLD_MAGE = KOBOLD_WARRIOR + 1,
	// Bosses
	KING_MUSH = KOBOLD_MAGE + 1,
	KING_MUSH_ATTACKS = KING_MUSH + 1,
	KING_MUSH_ENTRY = KING_MUSH_ATTACKS + 1,
	TITHO = KING_MUSH_ENTRY + 1,
	TITHO_ATTACKS = TITHO + 1,
	TITHO_ENTRY = TITHO_ATTACKS + 1,
	DRAGON = TITHO_ENTRY + 1,
	DRAGON_ATTACKS = DRAGON + 1,
	DRAGON_ENTRY = DRAGON_ATTACKS + 1,
	//NPCS
	GUIDE = DRAGON_ENTRY + 1,
	// Misc Assets
	CANNONBALL = GUIDE + 1,
	SPELLS = CANNONBALL + 1,
	TILE_SET_RED = SPELLS + 1,
	TILE_SET_BLUE = TILE_SET_RED + 1,
	HELP_PIC = TILE_SET_BLUE + 1,
	END_PIC = HELP_PIC + 1,
	ICONS = END_PIC + 1,
	TEXTURE_COUNT = ICONS + 1,
};
const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

// Define the scaling factors needed for each textures
// Note: This needs to stay the same order as TEXTURE_ASSET_ID and texture_paths
static constexpr std::array<vec2, texture_count> scaling_factors = {
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size * 3, MapUtility::tile_size * 3),
	vec2(MapUtility::tile_size * 3, MapUtility::tile_size * 3),
	vec2(MapUtility::tile_size * 5, MapUtility::tile_size * 5),
	vec2(MapUtility::tile_size * 5, MapUtility::tile_size * 5),
	vec2(MapUtility::tile_size * 3, MapUtility::tile_size * 3),
	vec2(MapUtility::tile_size * 5, MapUtility::tile_size * 5),
	vec2(MapUtility::tile_size * 5, MapUtility::tile_size * 5),
	vec2(MapUtility::tile_size * 3, MapUtility::tile_size * 3),
	vec2(MapUtility::tile_size * 5, MapUtility::tile_size * 5),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size * 0.5, MapUtility::tile_size * 0.5),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size* MapUtility::room_size, MapUtility::tile_size* MapUtility::room_size),
	vec2(MapUtility::tile_size* MapUtility::room_size, MapUtility::tile_size* MapUtility::room_size),
	vec2(MapUtility::tile_size* MapUtility::room_size * 3, MapUtility::tile_size* MapUtility::room_size * 2),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
};

enum class EFFECT_ASSET_ID {
	LINE = 0,
	RECTANGLE = LINE + 1,
	OVAL = RECTANGLE + 1,
	ENEMY = OVAL + 1,
	PLAYER = ENEMY + 1,
	DEATH = PLAYER + 1,
	BOSS_INTRO_SHADER = DEATH + 1,
	HEALTH = BOSS_INTRO_SHADER + 1,
	FANCY_HEALTH = HEALTH + 1,
	TEXTURED = FANCY_HEALTH + 1,
	SPRITESHEET = TEXTURED + 1,
	SPELL = SPRITESHEET + 1,
	AOE = SPELL + 1,
	WATER = AOE + 1,
	TILE_MAP = WATER + 1,
	TEXT_BUBBLE = TILE_MAP + 1,
	LIGHT = TEXT_BUBBLE + 1,
	LIGHT_TRIANGLES = LIGHT + 1,
	LIGHTING = LIGHT_TRIANGLES + 1,
	EFFECT_COUNT = LIGHTING + 1,
};
constexpr int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID : uint8_t {
	SALMON = 0,
	SPRITE = SALMON + 1,
	SMALL_SPRITE = SPRITE + 1,
	ENTRY_ANIMATION_STRIP = SMALL_SPRITE + 1,
	DEATH = ENTRY_ANIMATION_STRIP + 1,
	HEALTH = DEATH + 1,
	FANCY_HEALTH = HEALTH + 1,
	LINE = FANCY_HEALTH + 1,
	DEBUG_LINE = LINE + 1,
	SCREEN_TRIANGLE = DEBUG_LINE + 1,
	ROOM = SCREEN_TRIANGLE + 1,
	LIGHTING_TRIANGLES = ROOM + 1,
	GEOMETRY_COUNT = LIGHTING_TRIANGLES + 1,
};
const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

struct Room {
	// use 0xff to indicate uninitialized value
	// this can have potential bug if we have up to 255 rooms, but we probably won't...
	MapUtility::RoomID room_id = 0xff;
	int level = -1;
	// Position within a particular map
	uint8_t room_index = 0;
	bool visible = false;
};

struct BigRoom {
	Entity first_room = entt::null;
	static void add_room(Entity big_room, Entity room);
};

struct BigRoomElement {
	Entity big_room = entt::null;
	Entity next_room = entt::null;
};

// For TileMap vertex buffers, we need a separate tile_texture float because we want
// to be able to specify different textures for a room
struct TileMapVertex {
	vec3 position = vec3(0);
	vec2 texcoord = vec3(0);
};

enum class ColorState { None = 0, Red = 1, Blue = 2, All = Blue + 1 };

//---------------------------------------------------------------------------
//-------------------------        Lighting         -------------------------
//---------------------------------------------------------------------------

struct LightingTriangle {
	vec2 p1;
	vec2 p2;
	vec2 p3;
};

struct LightingTile {
};

struct Light {
	float radius;
};

//---------------------------------------------------------------------------
//-------------------------           AI            -------------------------
//---------------------------------------------------------------------------

// Enemy List:
// https://docs.google.com/document/d/1HyGTf5afBIQPUthAuvrTZ-UZRlS8scZUTA4rekU3-kE/edit#heading=h.am6gzz477ssj
enum class EnemyType {
	// Small Enemy Types
	TrainingDummy = 0,
	Slime = TrainingDummy + 1,
	Raven = Slime + 1,
	Armor = Raven + 1,
	TreeAnt = Armor + 1,
	Wraith = TreeAnt + 1,
	Drake = Wraith + 1,
	Mushroom = Drake + 1,
	Spider = Mushroom + 1,
	Clone = Spider + 1,
	SwordSoldier = Clone + 1,
	SpearSoldier = SwordSoldier + 1,
	Apparition = SpearSoldier + 1,
	KoboldWarrior = Apparition + 1,
	KoboldMage = KoboldWarrior + 1,
	// Boss Enemy Types
	KingMush = KoboldMage + 1,
	Titho = KingMush + 1,
	Dragon = Titho + 1,
	AOERingGen = Dragon + 1,
	EnemyCount = AOERingGen + 1,
};

const std::array<EnemyType, ((size_t)EnemyType::AOERingGen - (size_t)EnemyType::KingMush)> enemy_type_bosses = {
	EnemyType::KingMush,
	EnemyType::Titho,
	EnemyType::Dragon,
};

enum class EnemyBehaviour {
	// Small Enemy Behaviours (State Machines)
	Dummy = 0,
	Basic = Dummy + 1,
	Cowardly = Basic + 1,
	Defensive = Cowardly + 1,
	Aggressive = Defensive + 1,
	Sacrificed = Aggressive + 1,
	// Boss Enemy Behaviours (Behaviour Trees)
	Summoner = Sacrificed + 1,
	WeaponMaster = Summoner + 1,
	Dragon = WeaponMaster + 1,
	AOERingGen = Dragon + 1,
	EnemyBehaviourCount = AOERingGen + 1,
};

const std::array<EnemyBehaviour, (size_t)EnemyType::EnemyCount> enemy_type_to_behaviour = {
	EnemyBehaviour::Dummy,		  EnemyBehaviour::Cowardly,	 EnemyBehaviour::Basic,		 EnemyBehaviour::Defensive,
	EnemyBehaviour::Aggressive,	  EnemyBehaviour::Basic,	 EnemyBehaviour::Basic,		 EnemyBehaviour::Cowardly,
	EnemyBehaviour::Aggressive,	  EnemyBehaviour::Defensive, EnemyBehaviour::Basic,		 EnemyBehaviour::Basic,
	EnemyBehaviour::Basic,		  EnemyBehaviour::Basic,	 EnemyBehaviour::Basic,		 EnemyBehaviour::Summoner,
	EnemyBehaviour::WeaponMaster, EnemyBehaviour::Dragon,	 EnemyBehaviour::AOERingGen,
};

// Small Enemy Behaviours (State Machines) uses the following states.
// Dummy:			Idle, Active.
// Basic:			Idle, Active.
// Cowardly:		Idle, Active, Flinched.
// Defensive:		Idle, Active, Immortal.
// Aggressive:		Idle, Active, Powerup.
// Sacrificed:		Idle, Active.
//
// Boss Enemy Behaviours (Behaviour Trees) uses the following states.
// Summoner:		Idle, Charging.
// WeaponMaster:	Idle.
enum class EnemyState {
	Idle = 0,
	Active = Idle + 1,
	Flinched = Active + 1,
	Powerup = Flinched + 1,
	Immortal = Powerup + 1,
	Charging = Immortal + 1,
	EnemyStateCount = Charging + 1,
};

// Structure to store enemy information.
struct Enemy {
	// Default is a slime.
	ColorState team = ColorState::Red;
	EnemyType type = EnemyType::Slime;
	EnemyBehaviour behaviour = EnemyBehaviour::Basic;
	EnemyState state = EnemyState::Idle;
	uvec2 nest_map_pos = { 0, 0 };

	uint radius = 3;
	uint speed = 1;
	uint attack_range = 1;
	bool active = true;

	uint danger_rating = 0;
	uint loot_multiplier = 1;

	void serialize(const std::string& prefix, rapidjson::Document& json) const;
	void deserialize(const std::string& prefix, const rapidjson::Document& json, bool load_from_file = true);
};

// Structure to store dragon boss information.
struct Dragon {
	bool is_sacrifice_used = false;
	std::vector<Entity> victims;
};

// Structure to store victim information.
struct Victim {
	Entity owner = entt::null;
};

struct LastKnownPlayerLocation {
	uvec2 position;
};

constexpr uint max_danger_rating = 5;

// Denotes that an enemy is a boss type
struct Boss {
};

struct AOESource {
	Entity children = entt::null;
	static Entity add(Entity parent);
};

struct AOESquare {
	Entity parent;
	Entity next_aoe;
	// Released AOE square will be destroyed in the next turn.
	bool actual_attack_displayed = false;
	bool is_released = false;
};

struct Environmental {
};

struct RedExclusive {
};

struct BlueExclusive {
};

// Component that denotes what colour the player cannot see at the moment
struct PlayerInactivePerception {
	ColorState inactive = ColorState::Red;
};

//---------------------------------------------------------------------------
//-------------------------         COMBAT          -------------------------
//---------------------------------------------------------------------------

enum class DamageType {
	Physical = 0,
	Fire = Physical + 1,
	Cold = Fire + 1,
	Earth = Cold + 1,
	Wind = Earth + 1,
	Light = Wind + 1,
	Count = Light + 1,
};

// Appears to be an issue with clang-format always putting these on one line, putting a comment on each is purportedly
// the best workaround
// https://stackoverflow.com/questions/39144255/clang-format-removes-new-lines-in-array-definition-with-designators/39287832#39287832
const std::array<std::string_view, (size_t)DamageType::Count> damage_type_names = {
	"Physical", //
	"Fire",		//
	"Cold",		//
	"Earth",	//
	"Wind",		//
	"Light",	//
};

enum class TargetingType {
	Adjacent = 0,
	Projectile = Adjacent + 1,
	Count = Projectile + 1,
};

template <typename T> using DamageTypeList = std::array<T, static_cast<size_t>(DamageType::Count)>;

enum class AttackPattern {
	Rectangle,
	Circle,
};

struct Attack {
	std::string name;

	// Each time an attack is made, a random number is chosen uniformly from [to_hit_min, to_hit_max]
	// This is added to the attack total
	int to_hit_min = 1;
	int to_hit_max = 20;

	// Each time damage is calculated, a random number is chosen uniformly from [damage_min, damage_max]
	// This is added to the damage total
	int damage_min = 10;
	int damage_max = 25;

	// This is used when calculating damage to work out if any of the target's damage_modifiers should apply
	DamageType damage_type = DamageType::Physical;
	TargetingType targeting_type = TargetingType::Projectile;
	int range = 1;
	AttackPattern pattern = AttackPattern::Circle;
	int parallel_size = 1;
	int perpendicular_size = 1;

	Entity effects = entt::null;

	int turn_cost = 1;
	int mana_cost = 0;

	bool can_reach(Entity attacker, uvec2 target) const;
	bool is_in_range(uvec2 source, uvec2 target, uvec2 pos) const;

	void serialize(const std::string& prefix, rapidjson::Document& json) const;
	void deserialize(const std::string& prefix, const rapidjson::Document& json);
	void deserialize(const rapidjson::GenericObject<false, rapidjson::Value>& attack_json);

	std::string get_description() const;
};

enum class Effect {
	// Per-Use conditions
	Immobilize = 0,
	Stun = Immobilize + 1,
	// Per-Turn conditions
	Disarm = Stun + 1,
	Entangle = Disarm + 1,
	Weaken = Entangle + 1,
	Bleed = Weaken + 1,
	Burn = Bleed + 1,
	// Make sure non-condition effects are last
	Crit = Burn + 1,
	Shove = Crit + 1,
	Count = Shove + 1,
};

constexpr size_t num_conditions = (size_t)Effect::Crit;
constexpr size_t num_per_use_conditions = (size_t)Effect::Disarm;
constexpr size_t num_per_turn_conditions = num_conditions - num_per_use_conditions;

// see damage_type_names for comment explanation
constexpr std::array<std::string_view, (size_t)Effect::Count> effect_names = {
	"Immobilize", //
	"Stun",		  //
	"Disarm",	  //
	"Entangle",	  //
	"Weaken",	  //
	"Bleed",	  //
	"Burn",		  //
	"Crit",		  //
	"Shove",	  //
};

struct EffectEntry {
	Entity next_effect;
	Effect effect;
	float chance;
	int magnitude;
};

struct ActiveConditions {
	std::array<int, num_conditions> conditions;
};

struct Stats {

	// Health and its max
	int health = 140;
	int health_max = 140;

	// Mana and its max
	int mana = 100;
	int mana_max = 100;

	// Each time an attack is made, this flat amount is added to the attack total
	int to_hit_weapons = 10;
	int to_hit_spells = 10;

	// Each time damage is calculated, this flat amount is added to the damage total
	DamageTypeList<int> damage_bonus = { 5 };

	// This number is compared to an attack total to see if it hits.
	// It hits if attack_total >= evasion
	int evasion = 12;

	// The default attack associated with this entity
	// TODO: Consider removing when multiple attacks are more readily supported
	Attack base_attack;

	// Each time damage is calculated, the modifier of the relevant type is added
	// A positive modifier is a weakness, like a straw golem being weak to fire
	// A negative modifeir is a resistance, like an iron golem being resistant to sword cuts
	DamageTypeList<int> damage_modifiers = { 0 };

	void serialize(const std::string& prefix, rapidjson::Document& json) const;
	void deserialize(const std::string& prefix, const rapidjson::Document& json);
};

struct PlayerStats {
	// Improves drop chances and drop quality
	int luck = 0;
};

struct StatBoosts {
	int health = 0;
	int mana = 0;
	int luck = 0;
	int light = 0;
	int to_hit_weapons = 0;
	int to_hit_spells = 0;
	DamageTypeList<int> damage_bonus = { 0 };
	int evasion = 0;
	DamageTypeList<int> damage_modifiers = { 0 };
	void deserialize(const rapidjson::GenericObject<false, rapidjson::Value>& boosts);

	std::string get_description() const;

	static void apply(Entity boosts, Entity target, bool applying);
};

enum class Slot {
	Weapon = 0,
	Armor = Weapon + 1,
	Spell1 = Armor + 1,
	Spell2 = Spell1 + 1,
	Ring = Spell2 + 1,
	Amulet = Ring + 1,
	Count = Amulet + 1,
};

const std::array<std::string_view, (size_t)Slot::Count> slot_names = {
	"Weapon", "Armor", "Spell", "Spell", "Ring", "Amulet",
};

template <typename T> using SlotList = std::array<T, static_cast<size_t>(Slot::Count)>;

enum class Resource {
	HealthPotion = 0,
	ManaPotion = HealthPotion + 1,
	PaletteSwap = ManaPotion + 1,
	Key = PaletteSwap + 1,
	Count = Key + 1,
};

// see damage_type_names for comment explanation
constexpr std::array<std::string_view, (size_t)Resource::Count> resource_names = {
	"Health Potion", //
	"Mana Potion",	 //
	"Palette Swap",	 //
	"Key",           //
};

// see damage_type_names for comment explanation
constexpr std::array<ivec2, (size_t)Resource::Count> resource_textures = {
	ivec2(0, 4), //
	ivec2(1, 4), //
	ivec2(0, 6), //
	ivec2(2, 4), //
};

struct Inventory {
	static constexpr size_t inventory_size = 12;
	std::array<Entity, inventory_size> inventory;
	SlotList<Entity> equipped;
	std::array<size_t, (size_t)Resource::Count> resources = { 3, 1, 3, 2 };
	Inventory()
		: inventory()
		, equipped()
	{
		inventory.fill(entt::null);
		equipped.fill(entt::null);
	}

	static Entity get(Entity entity, Slot slot);
};

struct ResourcePickup {
	Resource resource = Resource::HealthPotion;
	void serialize(const std::string& prefix, rapidjson::Document& json) const;
	void deserialize(const std::string& prefix, const rapidjson::Document& json);
};

struct Item {
	Entity item_template;
	std::string get_description(bool detailed) const;

	void serialize(const std::string& prefix, rapidjson::Document& json) const;
	void deserialize(const std::string& prefix, const rapidjson::Document& json);
};

struct ItemTemplate {
	std::string name;
	int tier = 0;
	SlotList<bool> allowed_slots = { false };
	ivec2 texture_offset = ivec2(0);
	vec2 texture_size = vec2(MapUtility::tile_size);
	void deserialize(Entity entity, const rapidjson::GenericObject<false, rapidjson::Value>& item);
};

struct Weapon {
	// TODO: Potentially replace with intelligent direct/indirect container
	std::vector<Entity> given_attacks;
	Attack& get_attack(size_t i) { return registry.get<Attack>(given_attacks.at(i)); }
	std::string get_description();
};

//---------------------------------------------------------------------------
//-------------------------		  ANIMATIONS        -------------------------
//---------------------------------------------------------------------------

// Represents allowed directions for an animated sprite (e.g whether the sprite is facing left or right)
enum class Sprite_Direction : uint8_t { SPRITE_LEFT, SPRITE_RIGHT };

// Animation details by type
struct AnimationProfile {
	TEXTURE_ASSET_ID texture;
	float travel_offset;
};

// Animation details for a boss's entry
struct BossEntryAnimation {
	TEXTURE_ASSET_ID texture;
	int max_frames;
};

struct EntryAnimationEnemy {
	EnemyType intro_enemy_type;
};

// Maps enemy types to corresponding animation profile
// Remember to add a mapping to a new texture (or use a default such as a slime)/enemy type
// This will help load the animation by enemy type when you load enemies
constexpr std::array<AnimationProfile, static_cast<int>(EnemyType::EnemyCount)> enemy_type_to_animation_profile {
	AnimationProfile { TEXTURE_ASSET_ID::DUMMY, 0.f }, 
	AnimationProfile { TEXTURE_ASSET_ID::SLIME, 0.2f }, 
	AnimationProfile { TEXTURE_ASSET_ID::RAVEN, 0.f },
	AnimationProfile { TEXTURE_ASSET_ID::ARMOR, 0.f },
	AnimationProfile { TEXTURE_ASSET_ID::TREEANT, 0.f },
	AnimationProfile { TEXTURE_ASSET_ID::WRAITH, 0.1f },
	AnimationProfile { TEXTURE_ASSET_ID::DRAKE, 0.1f },
	AnimationProfile { TEXTURE_ASSET_ID::MUSHROOM, 0.2f },
	AnimationProfile { TEXTURE_ASSET_ID::SPIDER, 0.2f },
	AnimationProfile { TEXTURE_ASSET_ID::CLONE, 0.f },
	AnimationProfile { TEXTURE_ASSET_ID::SWORD_SOLDIER, 0.f },
	AnimationProfile { TEXTURE_ASSET_ID::SPEAR_SOLDIER, 0.f },
	AnimationProfile { TEXTURE_ASSET_ID::APPARITION, 0.f },
	AnimationProfile { TEXTURE_ASSET_ID::KOBOLD_WARRIOR, 0.f },
	AnimationProfile { TEXTURE_ASSET_ID::KOBOLD_MAGE, 0.f },
	AnimationProfile { TEXTURE_ASSET_ID::KING_MUSH, 0.f },
	AnimationProfile { TEXTURE_ASSET_ID::TITHO, 0.f },
	AnimationProfile { TEXTURE_ASSET_ID::DRAGON, 0.f },
	AnimationProfile { TEXTURE_ASSET_ID::DRAGON, 0.f },
};

const std::map<EnemyType, BossEntryAnimation> boss_type_entry_animation_map {
	{ EnemyType::KingMush, BossEntryAnimation { TEXTURE_ASSET_ID::KING_MUSH_ENTRY, 32 } },
	{ EnemyType::Titho, BossEntryAnimation { TEXTURE_ASSET_ID::TITHO_ENTRY, 48 } },
	{ EnemyType::Dragon, BossEntryAnimation { TEXTURE_ASSET_ID::DRAGON_ENTRY, 42 } },
};

constexpr std::array<int, (size_t)EnemyState::EnemyStateCount> enemy_state_to_animation_state = {
	0, // Idle
	1, // Active
	2, // Flinched
	2, // Powerup
	2, // Immortal
	3, // Charging
};

// Render behind other elements in its grouping
struct Background {
};

struct TextureOffset {
	ivec2 offset;
	vec2 size;
};

struct RenderRequest {
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

	bool visible = true;
};

struct Color {
	vec3 color;
};

// Struct for denoting the frame that the rendering system should be rendering
// to the screen for a spritesheet
struct Animation {
	ColorState color = ColorState::None;
	vec4 display_color = { 1, 1, 1, 1 };
	int direction = 1;
	int frame = 0;
	int max_frames = 1;
	int state = 0;
	// Adjusts animation rate to be faster or slower than default
	// ie. faster things should change more frames. slower things should change less frames
	float speed_adjustment = 0.6f;
	float elapsed_time = 0.f;
	// Stores offset for the specific entity when requested for travel
	float travel_offset = 0.f;
};

// Struct denoting irregular animation events (ie attacking, containing information to restore an entity's
// animations after the irregular event animation completes
struct EventAnimation {
	float speed_adjustment = 1;
	vec4 restore_color = { 1, 1, 1, 1 };

	int restore_state = 0;
	float restore_speed = 1;
	int frame = 0;
};

// Struct denoting an animation event that will remove itself (and the entity) after completion of its animation
// NOTE: this is different from a regular event animation, and is used for temporary effects that will be removed
// such as attack/spell effects on a square
struct TransientEventAnimation {
	int frame = 0;
};

// Denotes that an animation event should stop being displayed after completion, but not erased
// Currently used for boss AOEs, as well as boss intro animations
struct UndisplayEventAnimation {
	int frame = 0;
};

// Denotes an animation event that controls the travel display of an entity. Upon completion,
// restores an entity to its previous state/speed, and removes the world position component from the associated
// entity
struct TravelEventAnimation {
	int restore_state = 0;
	float restore_speed = 1.f;

	float total_time = 0.f;
	float max_time = 200.f;

	vec2 start_point = { 0, 0 };
	vec2 middle_point = { 0, 0 };
	vec2 end_point = { 0, 0 };
};

// Denotes that an entity has an textured asset, and should be rendered after regular assets (such as player/enemy)
struct EffectRenderRequest {
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

	bool visible = true;
};

constexpr std::array<int, (size_t)DamageType::Count> damage_type_to_spell_impact = {
	4, // Physical (default is fire effect)
	4, // Fire effect
	5, // Ice effect
	6, // Earth effect
	7, // Wind effect
};

const std::map<EnemyType, TEXTURE_ASSET_ID> boss_type_attack_spritesheet { 
	{ EnemyType::KingMush, TEXTURE_ASSET_ID::KING_MUSH_ATTACKS } ,
	{ EnemyType::Titho, TEXTURE_ASSET_ID::TITHO_ATTACKS },
	{ EnemyType::Dragon, TEXTURE_ASSET_ID::DRAGON_ATTACKS },
	{ EnemyType::AOERingGen, TEXTURE_ASSET_ID::DRAGON_ATTACKS },
};

struct DeathDeformation {
	float side_direction = 0;
	float height_direction = 0;
};

struct RoomAnimation {
	uvec2 start_tile;
	float dist_per_second = MapUtility::tile_size * 6.f;
	float elapsed_time = 0;
};
//---------------------------------------------------------------------------
//-------------------------		    Physics         -------------------------
//---------------------------------------------------------------------------

// Struct indicating an object is hittable (Currently limited to projectiles
struct Hittable {
};

// Stucture to store collision information
struct Collision {
	Entity children;
	static void add(Entity parent, Entity child);
};

struct CollisionEntry {
	Entity parent;
	Entity next;
	Entity target;
};

// struct denoting a currently active projectile
struct ActiveProjectile {
	vec2 head_offset = { 0, 0 };
	float radius = 6;
	Entity shooter;

	explicit ActiveProjectile(const Entity& shooter)
		: shooter(shooter)
	{
	}
};

// Struct for resolving projectiles, including the arrow fired by the player
struct ResolvedProjectile {
	float counter = 150;
};

struct Velocity {
	float speed;
	float angle;
	vec2 get_direction() const { return { sin(angle), -cos(angle) }; }
	vec2 get_velocity() const { return get_direction() * speed; }
};

//---------------------------------------------------------------------------
//-------------------------		  Positioning       -------------------------
//---------------------------------------------------------------------------

// Represents the position on the map,
// top left is (0,0) bottom right is (99,99)
struct MapPosition {
	uvec2 position;
	explicit MapPosition(uvec2 position)
		: position(position)
	{
		assert(position.x <= MapUtility::map_down_right.x && position.y <= MapUtility::map_down_right.y);
	};

	void serialize(const std::string& prefix, rapidjson::Document& json) const;
	void deserialize(Entity entity, const std::string& prefix, const rapidjson::Document& json);
};

struct MapHitbox {
	uvec2 area;
	uvec2 center;
};

// Represents the screen position,
// top left is (0,0), bottom right is (1, 1)
struct ScreenPosition {
	vec2 position;
};

// Represents the world position,
// top left is (0,0), bottom right is (window_width_px, window_height_px)
struct WorldPosition {
	vec2 position;
};

//---------------------------------------------------------------------------
//-------------------------           UI            -------------------------
//---------------------------------------------------------------------------

enum class Alignment {
	Start = 1,
	Center = 0,
	End = -1,
};

struct UIRectangle {
	float opacity;
	vec4 fill_color;
};

struct UIRenderRequest {
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

	vec2 size;
	float angle = 0;
	Alignment alignment_x = Alignment::Center;
	Alignment alignment_y = Alignment::Center;
};

enum class BarType {
	Health,
	Mana,
};

struct TargettedBar {
	BarType target = BarType::Health;
};

struct UIElement {
	Entity group;
	Entity next = entt::null;
	bool visible = true;
	UIElement(Entity group, bool visible)
		: group(group)
		, visible(visible)
	{
	}
};

enum class UILayer {
	Boxes = 0,
	Content = Boxes + 1,
	TooltipBoxes = Content + 1,
	TooltipContent = TooltipBoxes + 1,
	Count = TooltipContent + 1
};

enum class Groups {
	HUD = 0,
	Inventory = HUD + 1,
	MainMenu = Inventory + 1,
	PauseMenu = MainMenu + 1,
	Help = PauseMenu + 1,
	DeathScreen = Help + 1,
	VictoryScreen = DeathScreen + 1,
	Tooltips = VictoryScreen + 1,
	Story = Tooltips + 1,
	Count = Story + 1,
};

struct UIGroup {
	bool visible = false;
	std::array<Entity, (size_t)UILayer::Count> first_elements = {};
	Groups identifier = Groups::Count;

	UIGroup() { first_elements.fill(entt::null); }

	static void add_element(Entity group, Entity element, UIElement& ui_element, UILayer layer = UILayer::Boxes);
	static void remove_element(Entity group, Entity element, UILayer layer = UILayer::Boxes);

	static Entity find(Groups group);
};

struct UISlot {
	Entity owner = entt::null;
	Entity contents = entt::null;
};

struct InventorySlot {
	size_t slot;
};

struct EquipSlot {
	Slot slot;
};

struct Draggable {
	Entity container;
};

struct Tooltip {
	Entity target;
};
enum class TutorialTooltip {
	ItemDropped = 0,
	ItemPickedUp = ItemDropped + 1,
	UseResource = ItemPickedUp + 1,
	ReadyToEquip = UseResource + 1,
	OpenedInventory = ReadyToEquip + 1,
	ChestSeen = OpenedInventory + 1,
	LockedSeen = ChestSeen + 1,
	Count = LockedSeen + 1,
};

struct TutorialTarget {
	TutorialTooltip tooltip;
};

struct InteractArea {
	vec2 size;
};

struct Line {
	vec2 scale;
	float angle;

	Line(vec2 scale, float angle)
		: scale(scale)
		, angle(angle)
	{
	}
};

struct Text {
	std::string text;
	uint16 font_size;
	Alignment alignment_x = Alignment::Center;
	Alignment alignment_y = Alignment::Center;
	size_t border = 0;
};

struct Cursive {
};

extern bool operator==(const Text& t1, const Text& t2);

template <> struct std::hash<Text> {
	std::size_t operator()(Text const& t) const
	{
		size_t text_hash = std::hash<std::string> {}(t.text);
		size_t size_hash = std::hash<uint16> {}(t.font_size);
		// Combination as per boost https://www.boost.org/doc/libs/1_35_0/doc/html/boost/hash_combine_id241013.html
		return text_hash ^ (size_hash + 0x9e3779b9 + (text_hash << 6) + (text_hash >> 2));
	}
};

enum class ButtonAction {
	SwitchToGroup,
	GoToPreviousGroup,
	TryHeal,
	TryMana,
	TryPalette,
	RestartGame,
};

struct Button {
	Entity label;
	ButtonAction action;
	Entity action_target;
};


struct Guide {
};
enum class CutSceneType {
	BossEntry = 0,
	NPCEntry = BossEntry + 1,
	Count = NPCEntry + 1,
};

struct RoomTrigger {
};

struct RadiusTrigger {
	// radius is based on number of TILE
	float radius;
};
// CutScene
struct CutScene {
	CutSceneType type;
	Entity ui_entity;
	std::vector<std::string> texts;
	Entity actual_entity;
};

const std::array<std::vector<std::string>, 4>
	boss_cutscene_texts = {
		std::vector<std::string> {
			std::string("Hmm? How did this ruffian find their way into my kingly chambers?"),
			std::string("Wait, there is only one possible explanation for this..."),
			std::string("An ASSASSIN?!"),
			std::string("Guards, to me!"),
			std::string("Defend your new king!"),
		},
		std::vector<std::string> {
			std::string("Ah, it's you again! It has been too long"),
			std::string("Do you think you will defeat my master this time?"),
			std::string("Or perhaps I shall have the honor of drawing your blood first."),
			std::string("Now, ON YOUR GUARD."),
		},
		std::vector<std::string> {
			std::string("And here you are at last."),
			std::string("Finally at the end of your vainglorious crusade."),
			std::string("Tell me, do you truly believe"),
			std::string("that all your efforts have meant something?"),
			std::string("That slaying my subordinates..."),
			std::string("in any way inconveniences me?"),
			std::string("Impudent whelp!"),
			std::string("Your efforts will mean nothing."),
			std::string("Now grovel, worm"),
			std::string("Witness the power of a god!"),
		},
	};