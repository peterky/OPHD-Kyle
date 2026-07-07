#include "MineReport.h"

#include "../TextRender.h"
#include "../ProgressBar.h"

#include "../../Constants/Strings.h"
#include "../../Constants/UiConstants.h"

#include "../../CacheImage.h"
#include "../../CacheFont.h"
#include "../../Resources.h"
#include "../../StructureManager.h"
#include "../../TruckAvailability.h"

#include "../../Map/OreHaulRoutes.h"
#include "../../Map/Route.h"
#include "../../Map/Tile.h"
#include "../../MapObjects/Structures/MineFacility.h"
#include "../../StructureStatusDescription.h"

#include <libOPHD/MapObjects/OreDeposit.h>

#include <NAS2D/StringFrom.h>
#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/Resource/Font.h>

#include <array>
#include <cfloat>
#include <map>
#include <algorithm>


namespace
{
	std::string getStructureDescription(const Structure& structure)
	{
		const auto& surfaceLocation = structure.xyz().xy;
		return structure.name() + " at " + NAS2D::stringFrom(surfaceLocation);
	}


	std::string formatInternalStorage(const MineFacility& mineFacility)
	{
		const auto& stored = mineFacility.storage();
		const auto perTypeCapacity = mineFacility.rawOreStorageCapacity();
		const auto totalStored = stored.total();
		const auto totalCapacity = perTypeCapacity * static_cast<int>(stored.resources.size());
		return std::to_string(totalStored) + " / " + std::to_string(totalCapacity) + " raw ore at mine";
	}


	bool isMineBufferFull(const MineFacility& mineFacility)
	{
		return mineFacility.storage().total() >= mineFacility.rawOreStorageCapacity() * static_cast<int>(mineFacility.storage().resources.size());
	}


	int statusPaneContentHeight(const MineFacility& mineFacility, bool routeAvailable, const NAS2D::Font& font, const NAS2D::Font& fontMedium)
	{
		auto height = fontMedium.height() + constants::MarginTight + fontMedium.height() + constants::MarginTight * 2;

		if (mineFacility.isIdle())
		{
			height += font.height() + constants::MarginTight;
		}

		if (!mineFacility.destroyed() && !mineFacility.underConstruction())
		{
			height += font.height() + constants::MarginTight;
		}

		if (mineFacility.destroyed() || mineFacility.underConstruction())
		{
			return height;
		}

		height += fontMedium.height() + constants::MarginTight + (font.height() + constants::MarginTight) * 2;

		if (!mineFacility.isOperable())
		{
			return height;
		}

		height += fontMedium.height() + constants::MarginTight + (font.height() + constants::MarginTight) * 2;

		if (!routeAvailable)
		{
			height += fontMedium.height() + constants::MarginTight * 2;
			return height;
		}

		height += (font.height() + constants::MarginTight) * 3;
		height += fontMedium.height() + constants::MarginTight + (font.height() + constants::MarginTight) * 2;
		return height;
	}
}


MineReport::MineReport(const StructureManager& structureManager, TakeMeThereDelegate takeMeThereHandler) :
	mStructureManager{structureManager},
	mTakeMeThereHandler{takeMeThereHandler},
	font{Control::getDefaultFont()},
	fontBold{Control::getDefaultFontBold()},
	fontMedium{getFontMedium()},
	fontMediumBold{getFontMediumBold()},
	fontBigBold{getFontHugeBold()},
	mineFacilityImage{getImage("structures/mine_facility.png")},
	uiIcons{getImage("ui/icons.png")},
	btnShowAll{"All", {this, &MineReport::onShowAll}},
	btnShowActive{"Active", {this, &MineReport::onShowActive}},
	btnShowIdle{"Idle", {this, &MineReport::onShowIdle}},
	btnShowExhausted{"Exhausted", {this, &MineReport::onShowExhausted}},
	btnShowDisabled{"Disabled", {this, &MineReport::onShowDisabled}},
	btnIdle{constants::Idle, {this, &MineReport::onIdle}},
	btnDigNewLevel{"Dig New Level", {this, &MineReport::onDigNewLevel}},
	btnTakeMeThere{constants::TakeMeThere, {this, &MineReport::onTakeMeThere}},
	btnAddTruck{constants::AddTruck, {this, &MineReport::onAddTruck}},
	btnRemoveTruck{constants::RemoveTruck, {this, &MineReport::onRemoveTruck}},
	lstMineFacilities{{this, &MineReport::onMineFacilitySelectionChange}},
	mSelectedFacility{nullptr},
	mAvailableTrucks{0}
{
	auto buttonOffset = NAS2D::Vector{10, 10};
	const auto buttonSize = NAS2D::Vector{94, 26};
	const auto buttons = std::array{&btnShowAll, &btnShowActive, &btnShowIdle, &btnShowExhausted, &btnShowDisabled};
	for (auto button : buttons)
	{
		button->size(buttonSize);
		button->type(Button::Type::Toggle);
		add(*button, buttonOffset);
		buttonOffset.x += button->size().x + constants::MarginTight;
	}

	btnShowAll.toggle(true);

	add(lstMineFacilities, {10, 10 + buttonOffset.y + buttonSize.y});

	// DETAIL PANE
	btnIdle.type(Button::Type::Toggle);
	btnIdle.size({140, 30});
	btnDigNewLevel.size({140, 30});
	btnTakeMeThere.size({140, 30});
	btnAddTruck.size({140, 30});
	btnRemoveTruck.size({140, 30});

	add(btnIdle, {0, 40});
	add(btnDigNewLevel, {0, 75});
	add(btnTakeMeThere, {0, 110});
	add(btnAddTruck, {0, 145});
	add(btnRemoveTruck, {0, 180});

	fillLists();
}


bool MineReport::canView(const Structure& structure)
{
	return dynamic_cast<const MineFacility*>(&structure) || structure.isSmelter();
}


void MineReport::selectStructure(Structure& structure)
{
	lstMineFacilities.setSelected(&structure);
}


void MineReport::clearSelected()
{
	lstMineFacilities.clearSelected();
	mSelectedFacility = nullptr;
}


void MineReport::fillLists()
{
	lstMineFacilities.clear();
	for (auto* mineFacility : mStructureManager.getStructures<MineFacility>())
	{
		lstMineFacilities.addItem(mineFacility);
		lstMineFacilities.last()->text = getStructureDescription(*mineFacility);
	}

	lstMineFacilities.setSelected(mSelectedFacility);
	mAvailableTrucks = getTruckAvailability();
}


void MineReport::refresh()
{
	onShowAll();
}


void MineReport::injectOreHaulRoutes(const OreHaulRoutes& oreHaulRoutes)
{
	mOreHaulRoutes = &oreHaulRoutes;
}


void MineReport::onResize()
{
	Control::onResize();

	const auto centerX = area().center().x;
	lstMineFacilities.size({centerX - 20, area().crossYPoint().y - lstMineFacilities.position().y - 10});

	const auto buttonPositionX = area().size.x - 150;
	btnIdle.position({buttonPositionX, btnIdle.position().y});
	btnDigNewLevel.position({buttonPositionX, btnDigNewLevel.position().y});
	btnTakeMeThere.position({buttonPositionX, btnTakeMeThere.position().y});
	btnAddTruck.position({buttonPositionX, btnAddTruck.position().y});
	btnRemoveTruck.position({buttonPositionX, btnRemoveTruck.position().y});
}


void MineReport::onVisibilityChange(bool visible)
{
	Report::onVisibilityChange(visible);

	onManagementButtonsVisibilityChange();
}


void MineReport::onManagementButtonsVisibilityChange()
{
	bool isVisible = visible() && mSelectedFacility;

	btnIdle.visible(isVisible);
	btnDigNewLevel.visible(isVisible);
	btnTakeMeThere.visible(isVisible);

	bool isTruckButtonVisible = isVisible &&
		!(mSelectedFacility->destroyed() || mSelectedFacility->underConstruction());

	btnAddTruck.visible(isTruckButtonVisible);
	btnRemoveTruck.visible(isTruckButtonVisible);
}


void MineReport::onFilterButtonClicked()
{
	btnShowAll.toggle(false);
	btnShowActive.toggle(false);
	btnShowIdle.toggle(false);
	btnShowExhausted.toggle(false);
	btnShowDisabled.toggle(false);
}


void MineReport::onShowAll()
{
	onFilterButtonClicked();
	btnShowAll.toggle(true);

	fillLists();
}


void MineReport::onShowActive()
{
	onFilterButtonClicked();
	btnShowActive.toggle(true);
}


void MineReport::onShowIdle()
{
	onFilterButtonClicked();
	btnShowIdle.toggle(true);
}


void MineReport::onShowExhausted()
{
	onFilterButtonClicked();
	btnShowExhausted.toggle(true);
}


void MineReport::onShowDisabled()
{
	onFilterButtonClicked();
	btnShowDisabled.toggle(true);
}


void MineReport::onMineFacilitySelectionChange()
{
	mSelectedFacility = dynamic_cast<MineFacility*>(lstMineFacilities.selectedStructure());

	onManagementButtonsVisibilityChange();

	if (!mSelectedFacility) { return; }

	const auto& mineFacility = *mSelectedFacility;
	btnIdle.toggle(mineFacility.forceIdle());
	btnIdle.enabled(mineFacility.isOperable());

	btnDigNewLevel.toggle(mineFacility.extending());
	btnDigNewLevel.enabled(mineFacility.canExtend() && (mineFacility.isOperable()));
}


void MineReport::onIdle()
{
	mSelectedFacility->forceIdle(btnIdle.isPressed());
}


void MineReport::onDigNewLevel()
{
	auto& mineFacility = *mSelectedFacility;
	mineFacility.extend();

	btnDigNewLevel.toggle(mineFacility.extending());
	btnDigNewLevel.enabled(mineFacility.canExtend());
}


void MineReport::onTakeMeThere()
{
	if (mSelectedFacility)
	{
		if (mTakeMeThereHandler) { mTakeMeThereHandler(mSelectedFacility); }
	}
}


void MineReport::onAddTruck()
{
	if (!mSelectedFacility) { return; }

	auto& mineFacility = *mSelectedFacility;

	if (mineFacility.assignedTrucks() == mineFacility.maxTruckCount()) { return; }

	if (pullTruckFromInventory())
	{
		mineFacility.addTruck();
		mAvailableTrucks = getTruckAvailability();
	}
}


void MineReport::onRemoveTruck()
{
	if (!mSelectedFacility) { return; }

	auto& mineFacility = *mSelectedFacility;

	if (mineFacility.assignedTrucks() == 1) { return; }

	if (pushTruckIntoInventory())
	{
		mineFacility.removeTruck();
		mAvailableTrucks = getTruckAvailability();
	}
}


void MineReport::drawMineFacilityPane(NAS2D::Renderer& renderer, const NAS2D::Point<int>& origin) const
{
	const auto text = mSelectedFacility ? getStructureDescription(*mSelectedFacility) : "";
	renderer.drawText(fontBigBold, text, origin, constants::PrimaryTextColor);

	drawStatusPane(renderer, origin + NAS2D::Vector{0, fontBigBold.height() + constants::MarginTight});
}


void MineReport::drawStatusPane(NAS2D::Renderer& renderer, const NAS2D::Point<int>& origin) const
{
	renderer.drawText(fontMediumBold, "Status", origin, constants::PrimaryTextColor);

	const auto& mineFacility = *mSelectedFacility;
	const bool isStatusHighlighted = mineFacility.disabled() || mineFacility.destroyed();
	const auto statusPosition = origin + NAS2D::Vector{0, fontMediumBold.height() + constants::MarginTight};
	renderer.drawText(fontMedium, mineFacility.stateDescription(), statusPosition, (isStatusHighlighted ? NAS2D::Color::Red : constants::PrimaryTextColor));

	if (mineFacility.isIdle())
	{
		const auto reasonPosition = statusPosition + NAS2D::Vector{0, fontMedium.height() + constants::MarginTight};
		renderer.drawText(font, structureStatusReason(mineFacility), reasonPosition, NAS2D::Color::Yellow);
	}

	if (!mineFacility.destroyed() && !mineFacility.underConstruction())
	{
		const auto storageLineOffset = mineFacility.isIdle() ?
			fontMedium.height() + font.height() + constants::MarginTight * 3 :
			fontMedium.height() + constants::MarginTight * 2;
		const auto storagePosition = origin + NAS2D::Vector{0, fontMediumBold.height() + storageLineOffset};
		const auto isBufferFull = isMineBufferFull(mineFacility);
		renderer.drawText(
			font,
			formatInternalStorage(mineFacility),
			storagePosition,
			isBufferFull ? NAS2D::Color::Red : constants::PrimaryTextColor
		);
	}

	if (mineFacility.destroyed() || mineFacility.underConstruction())
	{
		return;
	}

	auto statusBlockHeight = fontMediumBold.height() + fontMedium.height() + constants::MarginTight * 2;
	if (mineFacility.isIdle())
	{
		statusBlockHeight += font.height() + constants::MarginTight;
	}
	if (!mineFacility.destroyed() && !mineFacility.underConstruction())
	{
		statusBlockHeight += font.height() + constants::MarginTight;
	}

	const auto titleSpacing = NAS2D::Vector{0, fontMediumBold.height() + constants::MarginTight};
	const auto trucksOrigin = origin + NAS2D::Vector{0, statusBlockHeight};
	renderer.drawText(fontMediumBold, "Trucks", trucksOrigin, constants::PrimaryTextColor);

	const auto truckValueOrigin = trucksOrigin + titleSpacing;
	const auto valueSpacing = NAS2D::Vector{0, font.height() + constants::MarginTight};
	const auto labelWidth = btnAddTruck.position().x - trucksOrigin.x - 10;
	drawLabelAndValueRightJustify(
		truckValueOrigin,
		labelWidth,
		"Assigned to Facility",
		std::to_string(mineFacility.assignedTrucks()),
		constants::PrimaryTextColor
	);
	drawLabelAndValueRightJustify(
		truckValueOrigin + valueSpacing,
		labelWidth,
		"Available in Storage",
		std::to_string(mAvailableTrucks),
		constants::PrimaryTextColor
	);

	if (!mineFacility.isOperable())
	{
		return;
	}

	const auto routeOrigin = truckValueOrigin + valueSpacing * 2;
	renderer.drawText(fontMediumBold, "Route", routeOrigin, constants::PrimaryTextColor);

	if (!mOreHaulRoutes)
	{
		renderer.drawText(fontMedium, "Route data unavailable.", routeOrigin + titleSpacing, NAS2D::Color::Yellow);
		return;
	}

	bool routeAvailable = mOreHaulRoutes->hasRoute(*mSelectedFacility);

	const auto routeValueOrigin = routeOrigin + titleSpacing;
	drawLabelAndValueRightJustify(
		routeValueOrigin,
		labelWidth,
		"Available",
		routeAvailable ? "Yes" : "No",
		routeAvailable ? constants::PrimaryTextColor : NAS2D::Color::Red
	);

	if (!routeAvailable)
	{
		renderer.drawText(
			fontMedium,
			"No path to an operational smelter. Roads help, but routes are blocked by structures, tubes, and deployed robots on the path. Toggle the route overlay to inspect.",
			routeValueOrigin + NAS2D::Vector{0, valueSpacing.y},
			NAS2D::Color::Yellow);
		return;
	}

	const auto routeCost = mOreHaulRoutes->getRouteCost(*mSelectedFacility);
	drawLabelAndValueRightJustify(
		routeValueOrigin + valueSpacing,
		labelWidth,
		"Cost",
		std::to_string(routeCost),
		constants::PrimaryTextColor
	);

	const auto oreHaulCapacity = mOreHaulRoutes->getOreHaulCapacity(*mSelectedFacility);
	drawLabelAndValueRightJustify(
		routeValueOrigin + valueSpacing * 2,
		labelWidth,
		"Total Haul Capacity",
		std::to_string(oreHaulCapacity * 4),
		constants::PrimaryTextColor
	);

	const auto& route = mOreHaulRoutes->getRoute(mineFacility);
	const auto& destination = *route.path.back()->structure();
	const auto haulDestinationOrigin = routeValueOrigin + valueSpacing * 4;
	renderer.drawText(fontMediumBold, "Haul Destination", haulDestinationOrigin, constants::PrimaryTextColor);
	const auto haulDestinationValueOrigin = haulDestinationOrigin + titleSpacing;
	const auto destinationLabel = destination.isWarehouse() ? "Warehouse buffer" : "Smelter";
	drawLabelAndValueRightJustify(
		haulDestinationValueOrigin,
		labelWidth,
		destinationLabel,
		destination.name() + " at " + NAS2D::stringFrom(destination.xyz().xy),
		constants::PrimaryTextColor
	);

	const auto& destinationOre = destination.isSmelter() ? destination.production() : destination.storage();
	const auto destinationOreStored = destinationOre.total();
	const auto destinationOreCapacity = destination.rawOreStorageCapacity() * static_cast<int>(destinationOre.resources.size());
	const auto destinationOreLabel = destination.isWarehouse() ? "Raw ore at warehouse" : "Raw ore at smelter";
	drawLabelAndValueRightJustify(
		haulDestinationValueOrigin + valueSpacing,
		labelWidth,
		destinationOreLabel,
		std::to_string(destinationOreStored) + " / " + std::to_string(destinationOreCapacity),
		destinationOreStored >= destinationOreCapacity ? NAS2D::Color::Red : constants::PrimaryTextColor
	);
}


void MineReport::drawOreProductionPane(NAS2D::Renderer& renderer, const NAS2D::Point<int>& origin) const
{
	if (!mOreHaulRoutes) { return; }

	renderer.drawText(fontMediumBold, "Ore Production", origin, constants::PrimaryTextColor);
	const auto panelWidth = area().endPoint().x - origin.x - 10;
	drawLabelRightJustify(origin, panelWidth, fontMediumBold, "Haul Capacity", constants::PrimaryTextColor);
	const auto lineOffset = NAS2D::Vector{0, fontMediumBold.height() + 1};
	const auto lineOrigin = origin + lineOffset;
	renderer.drawLine(lineOrigin, lineOrigin + NAS2D::Vector{panelWidth, 0}, constants::PrimaryTextColor, 1);

	const auto& oreDeposit = mSelectedFacility->oreDeposit();
	const auto oreAvailable = oreDeposit.availableResources();
	const auto oreTotalYield = oreDeposit.totalYield();

	const auto oreHaulCapacity = mOreHaulRoutes->getOreHaulCapacity(*mSelectedFacility);

	auto resourceOffset = lineOffset + NAS2D::Vector{0, 1 + constants::Margin + 2};
	const auto progressBarSize = NAS2D::Vector{area().endPoint().x - origin.x - 10, std::max(25, fontBold.height() + constants::MarginTight * 2)};
	for (size_t i = 0; i < 4; ++i)
	{
		const auto resourcePosition = origin + resourceOffset;
		renderer.drawSubImage(uiIcons, resourcePosition, ResourceImageRectsOre[i]);
		const auto resourceNameOffset = NAS2D::Vector{ResourceImageRectsOre[i].size.x + constants::MarginTight + 2, 0};
		renderer.drawText(fontBold, "Mine " + ResourceNamesOre[i], resourcePosition + resourceNameOffset, constants::PrimaryTextColor);
		drawLabelRightJustify(resourcePosition, panelWidth, font, std::to_string(oreHaulCapacity), constants::PrimaryTextColor);

		const auto resourceNameHeight = std::max(ResourceImageRectsOre[i].size.y, fontBold.height());
		const auto progressBarPosition = resourcePosition + NAS2D::Vector{0, resourceNameHeight + constants::MarginTight + 2};
		const auto progressBarArea = NAS2D::Rectangle{progressBarPosition, progressBarSize};
		drawProgressBar(
			oreAvailable.resources[i],
			oreTotalYield.resources[i],
			progressBarArea
		);

		const std::string str = std::to_string(oreAvailable.resources[i]) + " of " + std::to_string(oreTotalYield.resources[i]) + " Remaining";
		const auto strOffset = (progressBarSize - fontBold.size(str)) / 2;
		renderer.drawText(fontBold, str, progressBarPosition + strOffset);

		resourceOffset.y += resourceNameHeight + progressBarSize.y + constants::Margin + 23;
	}
}


void MineReport::update()
{
	if (!visible()) { return; }

	ControlContainer::update();
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	draw(renderer);
}


int MineReport::statusPaneHeight() const
{
	if (!mSelectedFacility || !mOreHaulRoutes) { return 0; }

	const auto routeAvailable = mOreHaulRoutes->hasRoute(*mSelectedFacility);
	return statusPaneContentHeight(*mSelectedFacility, routeAvailable, font, fontMedium);
}


void MineReport::draw(NAS2D::Renderer& renderer) const
{
	const auto startPoint = NAS2D::Point{area().center().x , area().position.y + 10};

	renderer.drawLine(startPoint, startPoint + NAS2D::Vector{0, area().size.y - 20}, constants::PrimaryTextColor);

	if (mSelectedFacility)
	{
		const auto paneOrigin = startPoint + NAS2D::Vector{10, 30};
		drawMineFacilityPane(renderer, paneOrigin);

		const auto orePaneOriginY = paneOrigin.y + std::max(130, statusPaneHeight() + 24);
		const auto orePaneHeight = area().endPoint().y - orePaneOriginY - 12;
		if (orePaneHeight >= 120)
		{
			drawOreProductionPane(renderer, {paneOrigin.x, orePaneOriginY});
		}
	}
}
