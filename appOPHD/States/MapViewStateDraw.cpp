// ==================================================================================
// = This file implements the functions that handle processing a turn.
// ==================================================================================

#include "MapViewState.h"

#include "../Constants/Strings.h"
#include "../Constants/UiConstants.h"
#include "../StructureStatusDescription.h"
#include "../Map/Tile.h"
#include "../MapObjects/Structure.h"
#include "../UI/DetailMap.h"

#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/StringFrom.h>

#include <array>
#include <string>


extern NAS2D::Point<int> MOUSE_COORDS;


namespace
{
	constexpr int GuidanceTurnLimit{40};

	const std::array<const char*, 7> ColonyGuidanceTips
	{
		"Tip: Land colonists from the Colony Ship, then build an Agricultural Dome for food.",
		"Tip: Warehouses store factory products, trucks, and can buffer raw ore when smelters are full.",
		"Tip: Build a Recycling Facility to process residence biowaste into maintenance parts.",
		"Tip: Open Mines (F7) to assign haul trucks so ore reaches smelters along roads.",
		"Tip: Watch food net (+/turn) in the top bar — each food unit feeds 10 colonists.",
		"Tip: W/S next to population shows free workers and scientists for new structures.",
		"Tip: Alt+Enter skips turns; you'll get a warning if food would run out."
	};
}


void MapViewState::drawSystemButton() const
{
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();

	auto position = NAS2D::Point{renderer.size().x - 80, constants::MarginTight};
	constexpr auto textOffset = NAS2D::Vector{constants::ResourceIconSize + constants::Margin, 3 - constants::MarginTight};

	const auto drawIcon = [&renderer, &icons = mUiIcons, height = mResourceInfoBar.size().y](NAS2D::Point<int> iconPosition, NAS2D::Rectangle<int> imageRect)
	{
		const auto imageOffset = NAS2D::Vector{0, (height - imageRect.size.y - 1) / 2};
		renderer.drawSubImage(icons, iconPosition + imageOffset, imageRect);
	};

	// Turns
	const auto turnImageRect = NAS2D::Rectangle<int>{{128, 0}, {constants::ResourceIconSize, constants::ResourceIconSize}};
	drawIcon(position, turnImageRect);
	const auto& font = Control::getDefaultFont();
	renderer.drawText(font, std::to_string(mTurnCount), position + textOffset, NAS2D::Color::White);

	position = mTooltipSystemButton.area().position + NAS2D::Vector{constants::MarginTight, constants::MarginTight};
	bool isMouseInMenu = mTooltipSystemButton.area().contains(MOUSE_COORDS);
	int menuGearHighlightOffsetX = isMouseInMenu ? 144 : 128;
	const auto menuImageRect = NAS2D::Rectangle<int>{{menuGearHighlightOffsetX, 32}, {constants::ResourceIconSize, constants::ResourceIconSize}};
	drawIcon(position, menuImageRect);
}


void MapViewState::drawColonyGuidance()
{
	if (mTurnCount > GuidanceTurnLimit) { return; }

	const auto tipIndex = static_cast<std::size_t>(mTurnCount / 5) % ColonyGuidanceTips.size();
	const std::string guidanceText{ColonyGuidanceTips[tipIndex]};

	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	const auto& font = Control::getDefaultFont();
	const auto position = NAS2D::Point{constants::Margin, constants::ResourceIconSize + constants::MarginTight + 2};
	const auto textSize = font.size(guidanceText);
	const auto backgroundRect = NAS2D::Rectangle<int>{
		position - NAS2D::Vector<int>{4, 2},
		textSize + NAS2D::Vector<int>{8, 4}};
	renderer.drawBoxFilled(backgroundRect, NAS2D::Color{0, 0, 0, 180});
	renderer.drawText(font, guidanceText, position, NAS2D::Color{200, 220, 255});
}


void MapViewState::drawResearchStatus()
{
	updateResearchStatusDisplay();

	if (!mResearchStatusText.empty())
	{
		auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
		const auto& font = Control::getDefaultFont();
		const auto position = NAS2D::Point{constants::Margin, constants::ResourceIconSize + constants::MarginTight + 2};
		const auto textSize = font.size(mResearchStatusText);
		const auto backgroundRect = NAS2D::Rectangle<int>{
			position - NAS2D::Vector<int>{4, 2},
			textSize + NAS2D::Vector<int>{8, 4}};
		renderer.drawBoxFilled(backgroundRect, NAS2D::Color{0, 0, 0, 180});
		renderer.drawText(font, mResearchStatusText, position, constants::SecondaryColor);
		return;
	}

	drawColonyGuidance();
}


void MapViewState::drawMapHoverTooltip()
{
	if (modalUiElementDisplayed()) { return; }
	if (!mDetailMap->area().contains(MOUSE_COORDS)) { return; }
	if (!mDetailMap->isMouseOverTile()) { return; }

	auto& tile = mDetailMap->mouseTile();
	if (!tile.hasStructure()) { return; }

	const auto* structure = tile.structure();
	if (!structure || structure->destroyed()) { return; }

	std::string tooltipText = structure->name();
	const auto statusReason = structureStatusReason(*structure);
	if (!statusReason.empty())
	{
		tooltipText += "\n" + statusReason;
	}

	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	const auto& font = Control::getDefaultFont();
	constexpr NAS2D::Vector<int> padding{6, 4};
	const auto textSize = font.size(tooltipText);
	const auto tooltipSize = textSize + padding * 2;

	auto tooltipPosition = MOUSE_COORDS + NAS2D::Vector{16, 20};
	if (tooltipPosition.x + tooltipSize.x > renderer.size().x)
	{
		tooltipPosition.x = MOUSE_COORDS.x - tooltipSize.x - 16;
	}
	if (tooltipPosition.y + tooltipSize.y > renderer.size().y)
	{
		tooltipPosition.y = MOUSE_COORDS.y - tooltipSize.y - 12;
	}

	const auto backgroundRect = NAS2D::Rectangle<int>{tooltipPosition, tooltipSize};
	renderer.drawBoxFilled(backgroundRect, NAS2D::Color{32, 32, 32, 230});
	renderer.drawBox(backgroundRect, NAS2D::Color{96, 96, 96});
	const auto statusColor = structure->disabled() || structure->isIdle() ? NAS2D::Color::Yellow : NAS2D::Color::White;
	renderer.drawText(font, tooltipText, tooltipPosition + padding, statusColor);
}