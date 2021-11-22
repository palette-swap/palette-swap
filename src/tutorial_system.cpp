#include "tutorial_system.hpp"

#include "ui_init.hpp"

void TutorialSystem::restart_game()
{
	triggered.fill(false);
	tooltips.fill(entt::null);
}

void TutorialSystem::trigger_tooltip(TutorialTooltip tip, Entity target)
{
	if (triggered.at((size_t)tip)) {
		return;
	}
	triggered.at((size_t)tip) = true;
	std::string message;
	std::pair<Alignment, Alignment> alignments = { Alignment::Center, Alignment::Center };
	vec2 pos = vec2(0);
	bool is_world = true;
	Entity group = entt::null;
	switch (tip) {
	case TutorialTooltip::ItemDropped: {
		message = "SHIFT to pick up";
		alignments.second = Alignment::End;
		pos = MapUtility::map_position_to_world_position(registry.get<MapPosition>(target).position)
			+ vec2(0, -MapUtility::tile_size / 2.f);
		break;
	case TutorialTooltip::ItemPickedUp: {
		message = "Click to open Inventory\n(Or press 'I')";
		alignments.second = Alignment::Start;
		target = get_tooltip_target(tip);
		pos = registry.get<ScreenPosition>(target).position + registry.get<UIRenderRequest>(target).size * vec2(-1, 1);
		is_world = false;
		break;
	}
	case TutorialTooltip::UseResource: {
		message = "Click to use resource\nHealth Potion, Mana Potion, Swap";
		alignments.second = Alignment::Start;
		target = get_tooltip_target(tip);
		pos = registry.get<ScreenPosition>(target).position + vec2(0, .03);
		is_world = false;
		break;
	}
	case TutorialTooltip::ReadyToEquip: {
		message = "Drag to slot to equip";
		alignments.second = Alignment::Start;
		pos = registry.get<ScreenPosition>(target).position + registry.get<UIRenderRequest>(target).size * vec2(0, .5);
		is_world = false;
		break;
	}
	case TutorialTooltip::OpenedInventory: {
		message = "Click to Close or press ESC";
		alignments.first = Alignment::Start;
		target = get_tooltip_target(tip);
		pos = registry.get<ScreenPosition>(target).position + registry.get<UIRenderRequest>(target).size * vec2(1, .5);
		is_world = false;
		break;
	}
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
