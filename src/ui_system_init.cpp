#include "ui_system.hpp"
#include "world_init.hpp"

void UISystem::restart_game()
{
	auto ui_group_view = registry.view<UIGroup>();
	registry.destroy(ui_group_view.begin(), ui_group_view.end());

	Entity player = registry.view<Player>().front();

	hud_group = create_ui_group(true);

	// Player Health Bar
	create_fancy_healthbar(hud_group);

	// Attack Display
	Inventory& inventory = registry.get<Inventory>(player);
	attack_display = registry.create();
	registry.emplace<ScreenPosition>(attack_display, vec2(0, 1));
	registry.emplace<Text>(attack_display, make_attack_display_text(),
						   (uint16)48,
						   Alignment::Start,
						   Alignment::End);
	registry.emplace<Color>(attack_display, vec3(1, 1, 1));
	UIGroup::add(hud_group, attack_display, registry.emplace<UIElement>(attack_display, hud_group, true));

	// Inventory
	inventory_group = create_ui_group(false);
	auto inventory_size = static_cast<float>(Inventory::inventory_size);
	float small_count = floorf(sqrtf(inventory_size));
	float large_count = ceilf(inventory_size / small_count);
	for (size_t i = 0; i < Inventory::inventory_size; i++) {
		Entity slot = create_inventory_slot(inventory_group, i, player, large_count, small_count);
		Entity item = inventory.inventory.at(i);
		if (item == entt::null) {
			continue;
		}
		create_ui_item(inventory_group, slot, item);
	}
	static const auto num_slots = (size_t)Slot::Count;
	for (size_t i = 0; i < num_slots; i++) {
		Entity slot = create_equip_slot(inventory_group, (Slot)i, player, 1, num_slots);
		Entity item = inventory.equipped.at(i);
		if (item == entt::null) {
			continue;
		}
		create_ui_item(inventory_group, slot, item);
	}
}
