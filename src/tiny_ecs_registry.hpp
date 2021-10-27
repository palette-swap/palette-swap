#pragma once
#include <vector>

#include "components.hpp"
#include "tiny_ecs.hpp"

class ECSRegistry {
public:
	// Manually created list of all components this game has
	ComponentContainer<DeathTimer> death_timers;
	ComponentContainer<Collision> collisions;
	ComponentContainer<Player> players;
	ComponentContainer<Camera> cameras;
	ComponentContainer<Mesh*> mesh_ptrs;
	ComponentContainer<RenderRequest> render_requests;
	ComponentContainer<ScreenState> screen_states;
	ComponentContainer<DebugComponent> debug_components;
	ComponentContainer<vec3> colors;

	// Map Generator
	ComponentContainer<Room> rooms;
	ComponentContainer<MapPosition> map_positions;
	ComponentContainer<WorldPosition> world_positions;
	ComponentContainer<Velocity> velocities;

	// AI
	ComponentContainer<EnemyState> enemy_states;
	ComponentContainer<RedDimension> red_entities;
	ComponentContainer<BlueDimension> blue_entities;
	ComponentContainer<EnemyNestPosition> enemy_nest_positions;

	// Physics
	ComponentContainer<Hittable> hittables;
	ComponentContainer<ActiveProjectile> active_projectiles;
	ComponentContainer<ResolvedProjectile> resolved_projectiles;

	// Combat
	ComponentContainer<Stats> stats;

	// Items
	ComponentContainer<Item> items;
	ComponentContainer<Weapon> weapons;
	ComponentContainer<Inventory> inventories;

	// Level Clearing / Loading
	ComponentContainer<LevelClearingRequest> level_clearing_requests;


private:
	// IMPORTANT: Don't forget to add any newly added containers!

	std::vector<ContainerInterface*> registry_list = {
		&death_timers,
		&collisions,
		&players,
		&mesh_ptrs,
		&render_requests,
		&screen_states,
		&debug_components,
		&colors,

		// Map Generator
		&rooms,
		&map_positions,
		&world_positions,
		&velocities,

		// AI
		&enemy_states,
		&red_entities,
		&blue_entities,
		&enemy_nest_positions,

		// Physics
		&hittables,
		&active_projectiles,
		&resolved_projectiles,

		// Combat
		&stats,

		// Items
		&items,
		&weapons,
		&inventories,

		// Level clearing & loading
		&level_clearing_requests,
	};


public:

	// Callbacks to remove a particular or all entities in the system
	void clear_all_components()
	{
		for (ContainerInterface* reg : registry_list) {
			reg->clear();
		}
	}

	void list_all_components()
	{
		printf("Debug info on all registry entries:\n");
		for (ContainerInterface* reg : registry_list) {
			if (reg->size() > 0) {
				printf("%4d components of type %s\n", (int)reg->size(), typeid(*reg).name());
			}
		}
	}

	void list_all_components_of(Entity e)
	{
		printf("Debug info on components of entity %u:\n", (unsigned int)e);
		for (ContainerInterface* reg : registry_list) {
			if (reg->has(e)) {
				printf("type %s\n", typeid(*reg).name());
			}
		}
	}

	void remove_all_components_of(Entity e)
	{
		for (ContainerInterface* reg : registry_list) {
			reg->remove(e);
		}
	}
};

extern ECSRegistry registry;