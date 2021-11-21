#pragma once
#include "../ext/stb_image/stb_image.h"
#include "common.hpp"

#include "map_utility.hpp"

#include <array>
#include <map>
#include <unordered_map>

#include "../ext/stb_image/stb_image.h"
#include "rapidjson/document.h"

#include "map_generator_system.hpp"

// Player component
struct Player {
};

// Camera component
struct Camera {
	uvec2 size, central;
};

// Data structure for toggling debug mode
struct Debug {
	bool in_debug_mode = false;
	bool in_freeze_mode = false;
};

// Sets the brightness of the screen
struct ScreenState {
	float darken_screen_factor = -1;
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent {
	// Note, an empty struct has size 1
};

// A timer that will be associated to dying salmon
struct DeathTimer {
	float counter_ms = 3000;
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
	// Bosses
	KING_MUSH = CLONE + 1,
	KING_MUSH_ATTACKS = KING_MUSH + 1,
	// Misc Assets
	CANNONBALL = KING_MUSH_ATTACKS + 1,
	SPELLS = CANNONBALL + 1,
	TILE_SET = SPELLS + 1,
	HELP_PIC = TILE_SET + 1,
	END_PIC = HELP_PIC + 1,
	TEXTURE_COUNT = END_PIC + 1,
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
	vec2(MapUtility::tile_size * 3, MapUtility::tile_size * 3),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size * 0.5, MapUtility::tile_size * 0.5),
	vec2(MapUtility::tile_size, MapUtility::tile_size),
	vec2(MapUtility::tile_size* MapUtility::room_size, MapUtility::tile_size* MapUtility::room_size),
	vec2(MapUtility::tile_size* MapUtility::room_size * 3, MapUtility::tile_size* MapUtility::room_size * 2),
};

enum class EFFECT_ASSET_ID {
	LINE = 0,
	RECTANGLE = LINE + 1,
	ENEMY = RECTANGLE + 1,
	PLAYER = ENEMY + 1,
	HEALTH = PLAYER + 1,
	FANCY_HEALTH = HEALTH + 1,
	TEXTURED = FANCY_HEALTH + 1,
	SPELL = TEXTURED + 1,
	AOE = SPELL + 1,
	WATER = AOE + 1,
	TILE_MAP = WATER + 1,
	EFFECT_COUNT = TILE_MAP + 1
};
constexpr int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID : uint8_t {
	SALMON = 0,
	SPRITE = SALMON + 1,
	SMALL_SPRITE = SPRITE + 1,
	HEALTH = SMALL_SPRITE + 1,
	FANCY_HEALTH = HEALTH + 1,
	LINE = FANCY_HEALTH + 1,
	DEBUG_LINE = LINE + 1,
	SCREEN_TRIANGLE = DEBUG_LINE + 1,
	ROOM = SCREEN_TRIANGLE + 1,
	GEOMETRY_COUNT = ROOM + 1
};
const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;


// Represents allowed directions for an animated sprite (e.g whether the sprite is facing left or right)
enum class Sprite_Direction : uint8_t { SPRITE_LEFT, SPRITE_RIGHT};

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
	void deserialize(const std::string& prefix, const rapidjson::Document& json);
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

struct Room {
	// use 0xff to indicate uninitialized value
	// this can have potential bug if we have up to 255 rooms, but we probably won't...
	MapUtility::RoomID room_id = 0xff;
	int level = -1;
};

// For TileMap vertex buffers, we need a separate tile_texture float because we want
// to be able to specify different textures for a room
struct TileMapVertex {
	vec3 position = vec3(0);
	vec2 texcoord = vec3(0);
};


enum class ColorState { None = 0, Red = 1, Blue = 2, All = Blue + 1 };

//---------------------------------------------------------------------------
//-------------------------           AI            -------------------------
//---------------------------------------------------------------------------

// Enemy List: https://docs.google.com/document/d/1HyGTf5afBIQPUthAuvrTZ-UZRlS8scZUTA4rekU3-kE/edit#heading=h.am6gzz477ssj
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
	// Boss Enemy Types
	KingMush = Clone + 1,
	EnemyCount = KingMush + 1,
};

enum class EnemyBehaviour {
	// Small Enemy Behaviours (State Machines)
	Dummy = 0,
	Basic = Dummy + 1,
	Cowardly = Basic + 1,
	Defensive = Cowardly + 1,
	Aggressive = Defensive + 1,
	// Boss Enemy Behaviours (Behaviour Trees)
	Summoner = Aggressive + 1,
	EnemyBehaviourCount = Summoner + 1,
};

const std::array<EnemyBehaviour, (size_t)EnemyType::EnemyCount> enemy_type_to_behaviour = {
	EnemyBehaviour::Dummy,
	EnemyBehaviour::Cowardly,
	EnemyBehaviour::Basic,
	EnemyBehaviour::Defensive,
	EnemyBehaviour::Aggressive, 
	EnemyBehaviour::Basic,	   
	EnemyBehaviour::Basic, 
	EnemyBehaviour::Cowardly,
	EnemyBehaviour::Aggressive, 
	EnemyBehaviour::Defensive,
	EnemyBehaviour::Summoner,
};

// Small Enemy Behaviours (State Machines) uses the following states.
// Dummy:		Idle, Active.
// Basic:		Idle, Active.
// Cowardly:	Idle, Active, Flinched.
// Defensive:	Idle, Active, Immortal.
// Aggressive:	Idle, Active, Powerup.
// 
// Boss Enemy Behaviours (Behaviour Trees) uses the following states.
// Summoner:	Idle, Charging.
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

	void serialize(const std::string & prefix, rapidjson::Document &json) const;
	void deserialize(const std::string& prefix, const rapidjson::Document& json, bool load_from_file = true);
};

struct AOESquare {
	// Released AOE square will be destroyed in the next turn.
	bool actual_attack_displayed = false;
	bool isReleased = false;
};

struct RedExclusive {

};

struct BlueExclusive {

};

// Component denoting the AOE entity that is displaying a boss's attack
struct AOEAttackActive {
	Entity aoe_attack;
};

// Component denoting an AOE's vector of intended attack targets
struct AOETargets {
	
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
	Count = Wind + 1,
};

const std::array<std::string_view, (size_t)DamageType::Count> damage_type_names = {
	"Physical", "Fire", "Cold", "Earth", "Wind",
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

	int mana_cost = 0;

	bool is_in_range(uvec2 source, uvec2 target, uvec2 pos) const;

	void serialize(const std::string& prefix, rapidjson::Document& json) const;
	void deserialize(const std::string& prefix, const rapidjson::Document& json);
	void deserialize(const rapidjson::GenericObject<false, rapidjson::Value>& attack_json);
};

enum class Effect {
	Shove = 0,
	Stun = Shove + 1,
	Count = Stun + 1,
};

const std::array<std::string_view, (size_t)Effect::Count> effect_names = {
	"Shove",
	"Stun",
};

struct EffectEntry {
	Entity next_effect;
	Effect effect;
	float chance;
	int magnitude;
};

struct Stunned {
	int rounds = 1;
};

struct Stats {

	// Health and its max
	int health = 140;
	int health_max = 140;

	// Mana and its max
	int mana = 100;
	int mana_max = 100;

	// Each time an attack is made, this flat amount is added to the attack total
	int to_hit_bonus = 10;

	// Each time damage is calculated, this flat amount is added to the damage total
	int damage_bonus = 5;

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

	void apply(Entity entity, bool applying);

	void serialize(const std::string& prefix, rapidjson::Document& json) const;
	void deserialize(const std::string& prefix, const rapidjson::Document& json);
};

struct StatBoosts {
	int health = 0;
	int mana = 0;
	int to_hit_bonus = 0;
	int damage_bonus = 0;
	int evasion = 0;
	DamageTypeList<int> damage_modifiers = { 0 };
	void deserialize(const rapidjson::GenericObject<false, rapidjson::Value>& boosts);
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

struct Inventory {
	static constexpr size_t inventory_size = 12;
	std::array<Entity, inventory_size> inventory;
	SlotList<Entity> equipped;
	size_t health_potions = 0;
	Inventory()
		: inventory()
		, equipped()
	{
		inventory.fill(entt::null);
		equipped.fill(entt::null);
	}

	static Entity get(Entity entity, Slot slot);
};

struct HealthPotion {
};

struct Item {
	Entity item_template;
};

struct ItemTemplate {
	std::string name;
	int tier = 0;
	SlotList<bool> allowed_slots = { false };
	void deserialize(Entity entity, const rapidjson::GenericObject<false, rapidjson::Value>& item);
};

struct Weapon {
	// TODO: Potentially replace with intelligent direct/indirect container
	std::vector<Entity> given_attacks;
	Attack& get_attack(size_t i) { return registry.get<Attack>(given_attacks.at(i)); }
};
//---------------------------------------------------------------------------
//-------------------------		  ANIMATIONS        -------------------------
//---------------------------------------------------------------------------

// Maps enemy types to corresponding texture asset
// Remember to add a mapping to a new texture (or use a default such as a slime)
// This will help load the animation by enemy type when you load enemies
const std::array<TEXTURE_ASSET_ID, static_cast<int>(EnemyType::EnemyCount)> enemy_type_textures {
	TEXTURE_ASSET_ID::DUMMY,
	TEXTURE_ASSET_ID::SLIME,
	TEXTURE_ASSET_ID::RAVEN,
	TEXTURE_ASSET_ID::ARMOR,
	TEXTURE_ASSET_ID::TREEANT,
	TEXTURE_ASSET_ID::WRAITH,
	TEXTURE_ASSET_ID::DRAKE,
	TEXTURE_ASSET_ID::MUSHROOM,
	TEXTURE_ASSET_ID::SPIDER,
	TEXTURE_ASSET_ID::CLONE,
	// TODO (Evan): temporarily used MUSHROOM to mock KINGMUSH for testing, please replace it when the texture is
	// available.
	TEXTURE_ASSET_ID::KING_MUSH,
};

const std::array<int, (size_t)EnemyState::EnemyStateCount> enemy_state_to_animation_state = {
	0, // Idle
	1, // Active
	2, // Flinched
	2, // Powerup
	2, // Immortal
	1, // Charging
};

// Render behind other elements in its grouping
struct Background {
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
	float speed_adjustment = 1;
	float elapsed_time = 0;
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
	bool turn_trigger = false;
	int frame = 0;
};

// Denotes that an animation event should stop being displayed after completion, but not erased
struct UndisplayEventAnimation {
	int frame = 0;
};

// Denotes that an entity has an textured asset, and should be rendered after regular assets (such as player/enemy)
struct EffectRenderRequest {
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

	bool visible = true;
};

const std::array<int, (size_t)DamageType::Count> damage_type_to_spell_impact = {
	4, // Physical (default is fire effect)
	4, // Fire effect
	5, // Ice effect
	6, // Earth effect
	7, // Wind effect
};

const std::map<EnemyType, TEXTURE_ASSET_ID> boss_type_attack_spritesheet { 
	{ EnemyType::KingMush, TEXTURE_ASSET_ID::KING_MUSH_ATTACKS } 
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

struct Child {
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
	Alignment alignment_x;
	Alignment alignment_y;

	UIRenderRequest(TEXTURE_ASSET_ID used_texture,
					EFFECT_ASSET_ID used_effect,
					GEOMETRY_BUFFER_ID used_geometry,
					vec2 size,
					float angle,
					Alignment alignment_x,
					Alignment alignment_y)
		: used_texture(used_texture)
		, used_effect(used_effect)
		, used_geometry(used_geometry)
		, size(size)
		, angle(angle)
		, alignment_x(alignment_x)
		, alignment_y(alignment_y)
	{
	}

	UIRenderRequest(EFFECT_ASSET_ID used_effect, vec2 size, float angle)
		: used_effect(used_effect)
		, size(size)
		, angle(angle)
		, alignment_x(Alignment::Center)
		, alignment_y(Alignment::Center)
	{
		if (used_effect == EFFECT_ASSET_ID::LINE) {
			used_geometry = GEOMETRY_BUFFER_ID::LINE;
		}
	}
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

struct UIGroup {
	bool visible = false;
	Entity first_element = entt::null;
	Entity first_text = entt::null;

	static void add_element(Entity group, Entity element, UIElement& ui_element);
	static void add_text(Entity group, Entity text, UIElement& ui_element);
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
	Alignment alignment_x;
	Alignment alignment_y;

	Text(std::string_view text, uint16 font_size, Alignment alignment_x, Alignment alignment_y)
		: text(text)
		, font_size(font_size)
		, alignment_x(alignment_x)
		, alignment_y(alignment_y)
	{
	}

	Text(std::string text, uint16 font_size, Alignment alignment_x, Alignment alignment_y)
		: text(std::move(text))
		, font_size(font_size)
		, alignment_x(alignment_x)
		, alignment_y(alignment_y)
	{
	}
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
};

struct Button {
	Entity label;
	ButtonAction action;
	Entity action_target;
};
