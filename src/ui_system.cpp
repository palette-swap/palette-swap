#include "ui_system.hpp"

#include "ui_init.hpp"

#include "geometry.hpp"

void UISystem::on_key(int key, int action, int /*mod*/, dvec2 mouse_screen_pos)
{
	if (!game_in_progress()) {
		return;
	}

	if (action == GLFW_PRESS && key == GLFW_KEY_I) {
		if (!is_group_visible(Groups::Inventory)) {
			switch_to_group(groups[(size_t)Groups::Inventory]);
			tutorials->destroy_tooltip(TutorialTooltip::ItemPickedUp);
		} else {
			switch_to_group(groups[(size_t)Groups::HUD]);
		}
	}
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
		if (player_can_act()) {
			switch_to_group(groups[(size_t)Groups::PauseMenu]);
		} else {
			switch_to_group(groups[(size_t)Groups::HUD]);
			if (story->in_cutscene()) {
				registry.get<UIGroup>(groups[(size_t)Groups::Story]).visible = true;
			}
		}
	}

	if (action == GLFW_PRESS && key == GLFW_KEY_D && is_group_visible(Groups::Inventory)) {
		for (auto [entity, draggable, element, screen_pos, interact_area] :
			 registry.view<Draggable, UIElement, ScreenPosition, InteractArea>().each()) {
			if (element.visible
				&& Geometry::Rectangle(screen_pos.position, interact_area.size).contains(vec2(mouse_screen_pos))) {
				loot->drop_item(registry.get<MapPosition>(registry.view<Player>().front()).position,
								registry.get<Item>(entity).item_template);
				insert_into_slot(entt::null, draggable.container);
				registry.get<UISlot>(draggable.container).contents = entt::null;
				UIGroup::remove_element(element.group, entity, UILayer::Content);
				registry.destroy(entity);
				break;
			}
		}
	}

	// Change attack
	// Key Codes for 1-9
	if (49 <= key && key <= 57) {
		size_t index = ((size_t)key) - 49;
		for (Slot slot : attacks_slots) {
			Entity weapon_entity = Inventory::get(registry.view<Player>().front(), slot);
			if (weapon_entity != entt::null) {
				Weapon& weapon = registry.get<Weapon>(weapon_entity);
				if (index < weapon.given_attacks.size()) {
					set_current_attack(slot, index);
					break;
				}
				index -= weapon.given_attacks.size();
			}
		}
	}
}

void UISystem::try_settle_held()
{
	if (held_under_mouse == entt::null) {
		return;
	}
	Draggable& draggable = registry.get<Draggable>(held_under_mouse);
	ScreenPosition& container_pos = registry.get<ScreenPosition>(draggable.container);
	ScreenPosition& held_pos = registry.get<ScreenPosition>(held_under_mouse);
	bool found_new_target = false;
	Geometry::Rectangle target
		= Geometry::Rectangle(held_pos.position, registry.get<InteractArea>(held_under_mouse).size);
	for (auto [new_slot_entity, new_slot, new_pos, area] :
		 registry.view<UISlot, ScreenPosition, InteractArea>().each()) {
		if (Geometry::Rectangle(new_pos.position, area.size).intersects(target)) {
			if (!swap_or_move_item(container_pos, held_pos, new_pos, draggable, new_slot_entity, new_slot)) {
				continue;
			}
			found_new_target = true;
			break;
		}
	}
	if (!found_new_target) {
		held_pos.position = container_pos.position;
	}
	held_under_mouse = entt::null;
}

void UISystem::switch_to_group(Entity group)
{
	if (group == entt::null || registry.get<UIGroup>(group).visible) {
		return;
	}
	Groups group_id = registry.get<UIGroup>(group).identifier;
	Groups previous_group_id = Groups::Count;
	try_settle_held();
	destroy_tooltip();
	for (auto [entity, other_group] : registry.view<UIGroup>().each()) {
		if (other_group.visible && entity != group && other_group.identifier != Groups::Tooltips) {
			previous_group = entity;
			previous_group_id = other_group.identifier;
		}
		other_group.visible = entity == group || other_group.identifier == Groups::Tooltips;
	}
	if (group_id == Groups::HUD) {
		for (auto& callback : show_world_callbacks) {
			callback();
		}
		if (story->in_cutscene()) {
			registry.get<UIGroup>(groups[(size_t)Groups::Story]).visible = true;
		}
		if (previous_group_id == Groups::Inventory) {
			tutorials->destroy_tooltip(TutorialTooltip::OpenedInventory);
		} else {
			PlayerInactivePerception& player_perception
				= registry.get<PlayerInactivePerception>(registry.view<Player>().front());
			music->set_world(player_perception.inactive == ColorState::Blue ? MusicSystem::MusicState::RedWorld
																			: MusicSystem::MusicState::BlueWorld);
		}
	} else if (group_id == Groups::Inventory) {
		tutorials->trigger_tooltip(TutorialTooltip::OpenedInventory);
		tutorials->destroy_tooltip(TutorialTooltip::ItemPickedUp);
	}
}

bool UISystem::is_group_visible(Groups group) { return registry.get<UIGroup>(groups[(size_t)group]).visible; }

void UISystem::destroy_tooltip()
{
	if (!registry.valid(tooltip)) {
		tooltip = entt::null;
		return;
	}
	UIGroup::remove_element(groups[(size_t)Groups::Tooltips], tooltip, UILayer::TooltipContent);
	registry.destroy(tooltip);
	tooltip = entt::null;
}

void UISystem::align_tooltip(vec2 new_pos)
{
	if (tooltip == entt::null) {
		return;
	}

	Text& text = registry.get<Text>(tooltip);
	text.alignment_x = (new_pos.x > .5) ? Alignment::End : Alignment::Start;
	text.alignment_y = (new_pos.y > .5) ? Alignment::End : Alignment::Start;

	registry.get<ScreenPosition>(tooltip).position = new_pos + vec2(0, .03 * (int)text.alignment_y);
}

void UISystem::destroy_attack_preview()
{
	if (!registry.valid(attack_preview)) {
		return;
	}

	UIGroup::remove_element(groups[(size_t)Groups::HUD], attack_preview);
	registry.destroy(attack_preview);
	attack_preview = entt::null;
}

void UISystem::update_attack_preview(uvec2 mouse_map_pos)
{
	static constexpr vec2 base_size = vec2(MapUtility::tile_size) / vec2(window_default_size);
	Attack& attack = get_current_attack();
	Entity player = registry.view<Player>().front();
	if (!attack.can_reach(player, mouse_map_pos)) {
		destroy_attack_preview();
		return;
	}
	uvec2 player_pos = registry.get<MapPosition>(player).position;

	if (attack_preview == entt::null) {
		attack_preview = registry.create();
		registry.emplace<MapPosition>(attack_preview, mouse_map_pos);
		registry.emplace<Background>(attack_preview);
		vec2 distance = ivec2(mouse_map_pos - player_pos);
		registry.emplace<UIRenderRequest>(
			attack_preview,
			TEXTURE_ASSET_ID::TEXTURE_COUNT,
			attack.pattern == AttackPattern::Rectangle ? EFFECT_ASSET_ID::RECTANGLE : EFFECT_ASSET_ID::OVAL,
			GEOMETRY_BUFFER_ID::LINE,
			base_size * vec2(attack.parallel_size * 2 - 1, attack.perpendicular_size * 2 - 1)
				/ renderer->get_screen_scale(),
			atan2f(distance.y, distance.x));
		registry.emplace<UIRectangle>(attack_preview, .8f, vec4(0));
		UIGroup::add_element(groups[(size_t)Groups::HUD],
							 attack_preview,
							 registry.emplace<UIElement>(attack_preview, groups[(size_t)Groups::HUD], true));
	} else {
		registry.get<MapPosition>(attack_preview).position = mouse_map_pos;
		vec2 distance = ivec2(mouse_map_pos - player_pos);
		UIRenderRequest& request = registry.get<UIRenderRequest>(attack_preview);
		request.angle = atan2f(distance.y, distance.x);
		request.size = base_size * vec2(attack.parallel_size * 2 - 1, attack.perpendicular_size * 2 - 1)
			/ renderer->get_screen_scale();
	}
}

bool UISystem::can_insert_into_slot(Entity item, Entity container)
{
	if (EquipSlot* equip_slot = registry.try_get<EquipSlot>(container)) {
		return registry.get<ItemTemplate>(registry.get<Item>(item).item_template)
			.allowed_slots.at((size_t)equip_slot->slot);
	}
	return true;
}

void UISystem::insert_into_slot(Entity item, Entity container)
{
	Entity player = registry.view<Player>().front();
	Inventory& inventory = registry.get<Inventory>(player);
	Entity actual_item = (item == entt::null) ? entt::null : registry.get<Item>(item).item_template;
	if (current_attack_slot != Slot::Count && inventory.equipped.at((size_t)current_attack_slot) == actual_item) {
		set_current_attack(Slot::Count, 0);
	}
	if (InventorySlot* inv_slot = registry.try_get<InventorySlot>(container)) {
		inventory.inventory.at(inv_slot->slot) = actual_item;
	} else if (EquipSlot* equip_slot = registry.try_get<EquipSlot>(container)) {
		// Apply any item bonuses
		// Not sure if there's a better place to put this?
		StatBoosts::apply(inventory.equipped.at((size_t)equip_slot->slot), player, false);
		inventory.equipped.at((size_t)equip_slot->slot) = actual_item;
		StatBoosts::apply(inventory.equipped.at((size_t)equip_slot->slot), player, true);
	}
}

bool UISystem::swap_or_move_item(ScreenPosition& container_pos,
								 ScreenPosition& held_pos,
								 ScreenPosition& new_pos,
								 Draggable& draggable,
								 Entity new_slot_entity,
								 UISlot& new_slot)
{
	if (!can_insert_into_slot(held_under_mouse, new_slot_entity)) {
		return false;
	}
	if (new_slot.contents != entt::null) {
		if (!can_insert_into_slot(new_slot.contents, draggable.container)) {
			return false;
		}
		// Swap positions if already exists
		registry.get<ScreenPosition>(new_slot.contents).position = container_pos.position;
		registry.get<Draggable>(new_slot.contents).container = draggable.container;
	}
	insert_into_slot(new_slot.contents, draggable.container);
	registry.get<UISlot>(draggable.container).contents = new_slot.contents;
	held_pos.position = new_pos.position;
	draggable.container = new_slot_entity;
	insert_into_slot(held_under_mouse, new_slot_entity);
	new_slot.contents = held_under_mouse;
	registry.get<Text>(attack_display).text = make_attack_display_text();
	return true;
}

void UISystem::do_action(Button& button)
{
	switch (button.action) {
	case ButtonAction::SwitchToGroup: {
		switch_to_group(button.action_target);
		break;
	}
	case ButtonAction::GoToPreviousGroup: {
		switch_to_group(previous_group);
		break;
	}
	case ButtonAction::TryHeal:
	case ButtonAction::TryMana: {
		Inventory& inventory = registry.get<Inventory>(button.action_target);
		Resource resource = (button.action == ButtonAction::TryHeal) ? Resource::HealthPotion : Resource::ManaPotion;
		if (inventory.resources.at((size_t)resource) > 0) {
			Stats& stats = registry.get<Stats>(button.action_target);
			if (resource == Resource::HealthPotion) {
				stats.health = stats.health_max;
			} else {
				stats.mana = stats.mana_max;
			}
			inventory.resources.at((size_t)resource)--;
			update_resource_count();
		}
		tutorials->destroy_tooltip(TutorialTooltip::UseResource);
		break;
	}
	case ButtonAction::TryPalette: {
		try_change_color();
		tutorials->destroy_tooltip(TutorialTooltip::UseResource);
		break;
	}
	case ButtonAction::RestartGame: {
		restart_world();
		break;
	}
	default:
		break;
	}
}

bool UISystem::on_left_click(int action, dvec2 mouse_screen_pos)
{
	if (action == GLFW_PRESS) {
		for (auto [entity, draggable, element, screen_pos, interact_area] :
			 registry.view<Draggable, UIElement, ScreenPosition, InteractArea>().each()) {
			if (element.visible && registry.get<UIGroup>(element.group).visible
				&& Geometry::Rectangle(screen_pos.position, interact_area.size).contains(vec2(mouse_screen_pos))) {
				held_under_mouse = entity;
				destroy_tooltip();
				tutorials->destroy_tooltip(TutorialTooltip::ReadyToEquip);
				return true;
			}
		}
		for (auto [entity, element, pos, request, button] :
			 registry.view<UIElement, ScreenPosition, UIRenderRequest, Button>().each()) {
			vec2 size_scale = request.size * ((request.used_effect == EFFECT_ASSET_ID::RECTANGLE) ? 1.f : .5f);
			Geometry::Rectangle button_rect = Geometry::Rectangle(
				pos.position + vec2((float)request.alignment_x, (float)request.alignment_y) * .5f * size_scale,
				size_scale);
			if (element.visible && registry.get<UIGroup>(element.group).visible
				&& button_rect.contains(vec2(mouse_screen_pos))) {
				do_action(button);
				return true;
			}
		}
	} else if (action == GLFW_RELEASE && held_under_mouse != entt::null) {
		try_settle_held();
	}
	return false;
}

void UISystem::on_mouse_move(vec2 mouse_screen_pos)
{
	uvec2 mouse_map_pos
		= MapUtility::world_position_to_map_position(renderer->screen_position_to_world_position(mouse_screen_pos));
	if (held_under_mouse != entt::null) {
		registry.get<ScreenPosition>(held_under_mouse).position = mouse_screen_pos;
		return;
	}

	if (tooltip != entt::null) {
		Entity target = registry.get<Tooltip>(tooltip).target;

		if (!registry.valid(target)) {
			destroy_tooltip();
			return;
		}

		bool on_target = false;
		if (ScreenPosition* pos = registry.try_get<ScreenPosition>(target)) {
			on_target = Geometry::Rectangle(pos->position, registry.get<UIRenderRequest>(target).size)
							.contains(mouse_screen_pos);
		} else if (MapPosition* pos = registry.try_get<MapPosition>(target)) {
			on_target = mouse_map_pos == pos->position;
		}
		if (on_target) {
			align_tooltip(mouse_screen_pos);
		} else {
			destroy_tooltip();
		}
		return;
	}

	auto create_tooltip = [&](const std::string_view& text, Entity entity) {
		tooltip = create_ui_tooltip(groups[(size_t)Groups::Tooltips], mouse_screen_pos, text, 24u);
		registry.emplace<Tooltip>(tooltip, entity);
		align_tooltip(mouse_screen_pos);
	};

	for (auto [entity, item, pos, request, element] :
		 registry.group<Item>(entt::get<ScreenPosition, UIRenderRequest, UIElement>).each()) {
		if (element.visible && Geometry::Rectangle(pos.position, request.size).contains(mouse_screen_pos)
			&& registry.get<UIGroup>(element.group).visible) {
			create_tooltip(item.get_description(true), entity);
			return;
		}
	}
	if (player_can_act()) {
		if (debugging.in_debug_mode && has_current_attack()
			&& get_current_attack().targeting_type == TargetingType::Adjacent) {
			update_attack_preview(mouse_map_pos);
		} else {
			destroy_attack_preview();
		}
		for (auto [entity, item, pos, request] : registry.view<Item, MapPosition, RenderRequest>().each()) {
			if (request.visible && pos.position == mouse_map_pos) {
				create_tooltip(item.get_description(false), entity);
				return;
			}
		}
		for (auto [entity, pickup, pos, request] : registry.view<ResourcePickup, MapPosition, RenderRequest>().each()) {
			if (request.visible && pos.position == mouse_map_pos) {
				create_tooltip(resource_names.at((size_t)pickup.resource), entity);
				return;
			}
		}
	}
}

bool UISystem::player_can_act() { return is_group_visible(Groups::HUD); }

bool UISystem::game_in_progress()
{
	return is_group_visible(Groups::HUD) || is_group_visible(Groups::Inventory) || is_group_visible(Groups::PauseMenu);
}

bool UISystem::has_current_attack() const
{
	if (current_attack_slot == Slot::Count) {
		return false;
	}
	Entity weapon_entity = Inventory::get(registry.view<Player>().front(), current_attack_slot);
	if (weapon_entity == entt::null) {
		return false;
	}
	const auto& given_attacks = registry.get<Weapon>(weapon_entity).given_attacks;
	return current_attack < given_attacks.size();
}

Attack& UISystem::get_current_attack()
{
	return registry.get<Weapon>(Inventory::get(registry.view<Player>().front(), current_attack_slot))
		.get_attack(current_attack);
}

void UISystem::on_show_world(const std::function<void()>& on_show_world_callback)
{
	show_world_callbacks.push_back(on_show_world_callback);
}

void UISystem::add_to_inventory(Entity item, size_t slot)
{
	if (item == entt::null) {
		return;
	}
	if (slot == MAXSIZE_T) {
		update_resource_count();
		return;
	}
	auto view = registry.view<InventorySlot>();
	auto matching_slot = std::find_if(view.begin(), view.end(), [&view, slot](Entity entity) {
		return view.get<InventorySlot>(entity).slot == slot;
	});
	if (matching_slot == view.end()) {
		return;
	}
	Entity ui_item = create_ui_item(groups[(size_t)Groups::Inventory], (*matching_slot), item);
	tutorials->trigger_tooltip(TutorialTooltip::ReadyToEquip, ui_item);
}

void UISystem::update_resource_count()
{
	for (size_t i = 0; i < (size_t)Resource::Count; i++) {
		registry.get<Text>(resource_displays.at(i)).text
			= std::to_string(registry.get<Inventory>(registry.view<Player>().front()).resources.at(i));
	}
}

void UISystem::end_game(bool victory)
{
	switch_to_group(groups.at((size_t)(victory ? Groups::VictoryScreen : Groups::DeathScreen)));
	music->set_state(victory ? MusicSystem::MusicState::YouWon : MusicSystem::MusicState::YouDied);
}

void UISystem::set_current_attack(Slot slot, size_t attack)
{
	current_attack_slot = (Slot)slot;
	current_attack = attack;
	registry.get<Text>(attack_display).text = make_attack_display_text();
	destroy_attack_preview();
}

std::string UISystem::make_attack_display_text()
{
	std::string text;
	size_t count = 0;
	for (Slot slot : attacks_slots) {
		Entity weapon_entity = Inventory::get(registry.view<Player>().front(), slot);
		if (weapon_entity != entt::null) {
			Weapon& weapon = registry.get<Weapon>(weapon_entity);
			for (size_t i = 0; i < weapon.given_attacks.size(); i++) {
				Attack& attack = weapon.get_attack(i);
				if (slot == current_attack_slot && i == current_attack) {
					text += "\n[" + std::to_string(++count) + "] " + attack.name;
				} else {
					text += "\n " + std::to_string(++count) + "  " + attack.name;
				}
			}
		}
	}
	return text;
}
