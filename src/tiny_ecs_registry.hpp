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
	ComponentContainer<ScreenState> screen_states;
	ComponentContainer<DebugComponent> debug_components;

	// Rendering
	ComponentContainer<Animation> animations;
	ComponentContainer<RenderRequest> render_requests;
	ComponentContainer<Event_Animation> event_animations;
	ComponentContainer<vec3> colors;

	// Map Generator
	ComponentContainer<Room> rooms;
	ComponentContainer<MapPosition> map_positions;
	ComponentContainer<WorldPosition> world_positions;
	ComponentContainer<Velocity> velocities;

	// AI
	ComponentContainer<Enemy> enemies;

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

private:
	// IMPORTANT: Don't forget to add any newly added containers!

	std::vector<ContainerInterface*> registry_list = {
		&death_timers,
		&collisions,
		&players,
		&mesh_ptrs,
		&screen_states,

		// Rendering/Animations
		&debug_components,
		&render_requests,
		&colors,
		&event_animations,
		&animations,

		// Map Generator
		&rooms,
		&map_positions,
		&world_positions,
		&velocities,

		// AI
		& enemies,

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