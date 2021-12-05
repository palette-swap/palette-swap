#pragma once

#include "components.hpp"

#include "loot_system.hpp"
#include "render_system.hpp"
#include "tutorial_system.hpp"

class UISystem {
public:

	void init(RenderSystem* render_system,
			  std::shared_ptr<LootSystem> loot_system,
			  std::shared_ptr<TutorialSystem> tutorial_system,
			  std::function<void()> change_color,
			  std::function<void()> restart_world);

	void restart_game();

	void on_key(int key, int action, int /*mod*/, dvec2 mouse_screen_pos);
	bool on_left_click(int action, dvec2 mouse_screen_pos);
	void on_mouse_move(vec2 mouse_screen_pos);

	bool player_can_act();
	bool game_in_progress();

	bool has_current_attack() const;
	Attack& get_current_attack();

	void on_show_world(const std::function<void()>& on_show_world_callback);

	void add_to_inventory(Entity item, size_t slot);
	void update_resource_count();

	void end_game(bool victory);

private:
	void try_settle_held();

	void switch_to_group(Entity group);
	bool is_group_visible(Groups group);

	// Remove tooltip from group, destroy entity
	void destroy_tooltip();
	// Set tooltip alignment based on its position
	void align_tooltip(vec2 new_pos);

	bool can_insert_into_slot(Entity item, Entity container);
	void insert_into_slot(Entity item, Entity container);
	bool swap_or_move_item(ScreenPosition& container_pos,
						   ScreenPosition& held_pos,
						   ScreenPosition& new_pos,
						   Draggable& draggable,
						   Entity new_slot_entity,
						   UISlot& new_slot);

	void do_action(Button& button);

	void set_current_attack(Slot slot, size_t attack);

	std::string make_attack_display_text();

	static constexpr std::array<Slot, 3> attacks_slots = {Slot::Weapon, Slot::Spell1, Slot::Spell2};
	
	Slot current_attack_slot = Slot::Count;
	size_t current_attack = 0;

	std::array<Entity, (size_t)Resource::Count> resource_displays = { entt::null };
	Entity attack_display = entt::null;

	std::array<Entity, (size_t)Groups::Count> groups = { entt::null };
	Entity held_under_mouse = entt::null;
	Entity tooltip = entt::null;

	RenderSystem* renderer;
	std::shared_ptr<LootSystem> loot;
	std::shared_ptr<TutorialSystem> tutorials;
	std::function<void()> try_change_color;
	std::function<void()> restart_world;

	std::vector<std::function<void()>> show_world_callbacks;
};