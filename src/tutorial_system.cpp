#include "tutorial_system.hpp"

#include "ui_init.hpp"

void TutorialSystem::restart_game()
{
	triggered.fill(false);
	tooltips.fill(entt::null);
}

bool TutorialSystem::has_triggered(TutorialTooltip tip) { return triggered.at((size_t)tip); }

void TutorialSystem::trigger_tooltip(TutorialTooltip tip, uvec2 map_pos)
{
	if (has_triggered(tip)) {
		return;
	}
	trigger_tooltip(tip, MapUtility::map_position_to_world_position(map_pos) - vec2(0, MapUtility::tile_size / 2.f));
}

void TutorialSystem::trigger_tooltip(TutorialTooltip tip, Entity target)
{
	if (has_triggered(tip)) {
		return;
	}
	vec2 pos = vec2(0);
	switch (tip) {
	case TutorialTooltip::ItemDropped: {
		pos = MapUtility::map_position_to_world_position(registry.get<MapPosition>(target).position)
			+ vec2(0, -MapUtility::tile_size / 2.f);
		break;
	case TutorialTooltip::ItemPickedUp: {
		target = get_tooltip_target(tip);
		pos = registry.get<ScreenPosition>(target).position + registry.get<UIRenderRequest>(target).size * vec2(-1, 1);
		break;
	}
	case TutorialTooltip::UseResource: {
		pos = registry.get<ScreenPosition>(get_tooltip_target(tip)).position + vec2(0, .03);
		break;
	}
	case TutorialTooltip::ReadyToEquip: {
		pos = registry.get<ScreenPosition>(target).position + registry.get<UIRenderRequest>(target).size * vec2(0, .5);
		break;
	}
	case TutorialTooltip::OpenedInventory: {
		target = get_tooltip_target(tip);
		pos = registry.get<ScreenPosition>(target).position + registry.get<UIRenderRequest>(target).size * vec2(1, .5);
		break;
	}
	}
	default:
		break;
	}
	trigger_tooltip(tip, pos);
}

void TutorialSystem::trigger_tooltip(TutorialTooltip tip, vec2 pos)
{
	if (has_triggered(tip)) {
		return;
	}
	triggered.at((size_t)tip) = true;
	std::string message;
	std::pair<Alignment, Alignment> alignments = { Alignment::Center, Alignment::Center };
	bool is_world = true;
	Entity group = entt::null;
	switch (tip) {
	case TutorialTooltip::ItemDropped: {
		message = "SHIFT to pick up";
		alignments.second = Alignment::End;
		group = UIGroup::find(Groups::HUD);
		break;
	}
	case TutorialTooltip::ChestSeen: {
		message = "SHIFT to open";
		alignments.second = Alignment::End;
		group = UIGroup::find(Groups::HUD);
		break;
	}
	case TutorialTooltip::LockedSeen: {
		message = "SHIFT to open\nFind keys in blue";
		alignments.second = Alignment::End;
		group = UIGroup::find(Groups::HUD);
		break;
	}
	case TutorialTooltip::ItemPickedUp: {
		message = "Click to open Inventory\n(Or press 'I')";
		alignments.second = Alignment::Start;
		is_world = false;
		group = UIGroup::find(Groups::HUD);
		break;
	}
	case TutorialTooltip::UseResource: {
		message = "Click to use resource\nHealth Potion, Mana Potion, Swap";
		alignments.second = Alignment::Start;
		is_world = false;
		group = UIGroup::find(Groups::HUD);
		break;
	}
	case TutorialTooltip::ReadyToEquip: {
		message = "Drag to slot to equip\n Press D to drop";
		alignments.second = Alignment::Start;
		is_world = false;
		group = UIGroup::find(Groups::Inventory);
		break;
	}
	case TutorialTooltip::OpenedInventory: {
		message = "Click to Close or press I";
		alignments.first = Alignment::Start;
		is_world = false;
		group = UIGroup::find(Groups::Inventory);
		break;
	}
	default:
		break;
	}
	tooltips.at((size_t)tip)
		= (is_world) ? create_world_tooltip(group, pos, message) : create_ui_tooltip(group, pos, message);
	registry.get<Text>(tooltips.at((size_t)tip)).alignment_x = alignments.first;
	registry.get<Text>(tooltips.at((size_t)tip)).alignment_y = alignments.second;
}

void TutorialSystem::destroy_tooltip(TutorialTooltip tip)
{
	Entity tooltip = tooltips.at((size_t)tip);
	if (!registry.valid(tooltip)) {
		return;
	}
	UIGroup::remove_element(registry.get<UIElement>(tooltip).group, tooltip, UILayer::TooltipContent);
	registry.destroy(tooltip);
	tooltips.at((size_t)tip) = entt::null;
}

Entity TutorialSystem::get_tooltip_target(TutorialTooltip tip)
{
	for (auto [entity, target] : registry.view<TutorialTarget>().each()) {
		if (target.tooltip == tip) {
			return entity;
		}
	}
	return entt::null;
}
