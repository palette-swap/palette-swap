#include "ui_system.hpp"
#include "world_init.hpp"

void UISystem::restart_game()
{
	Entity player = registry.view<Player>().front();

	Entity game_hud = create_ui_group(true);
	// Player Health Bar

	create_fancy_healthbar(game_hud);

	// Inventory
	inventory_group = create_ui_group(false);
	Inventory& inventory = registry.get<Inventory>(player);
	auto inventory_size = static_cast<float>(inventory.inventory_size);
	float small_count = floorf(sqrtf(inventory_size));
	float large_count = ceilf(inventory_size / small_count);
	for (size_t i = 0; i < inventory.inventory_size; i++) {
		create_inventory_slot(inventory_group, i, player, large_count, small_count);
	}
}
