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
}
