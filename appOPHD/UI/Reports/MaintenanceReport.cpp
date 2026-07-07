#include "MaintenanceReport.h"

#include "../../MaintenancePriority.h"
#include "../../CacheImage.h"
#include "../../CacheFont.h"
#include "../../StructureManager.h"
#include "../../Constants/Strings.h"
#include "../../Constants/UiConstants.h"
#include "../../MapObjects/Structures/MaintenanceFacility.h"

#include <NAS2D/Utility.h>
#include <NAS2D/EventHandler.h>
#include <NAS2D/EnumMouseButton.h>
#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/Resource/Font.h>

#include <algorithm>
#include <string>
#include <vector>


namespace
{
	constexpr auto filterButtonSectionOffset = NAS2D::Vector{10, 10};
	constexpr auto filterButtonSize = NAS2D::Vector{110, 26};
	constexpr auto infoSectionOffset = filterButtonSectionOffset + NAS2D::Vector{0, filterButtonSize.y + 10};
	constexpr auto infoSectionHeight = 78;
	constexpr auto structureListBoxOffset = infoSectionOffset + NAS2D::Vector{0, infoSectionHeight + 20};
	constexpr auto rightHeaderHeight = 56;
	constexpr auto rightDetailsTop = 150;


	std::string maintenanceStateDescription(const Structure& structure)
	{
		if (structure.destroyed()) { return "Destroyed"; }
		if (structure.underConstruction()) { return "Under Construction"; }
		if (structure.disabled()) { return structure.stateDescription(); }

		const auto integrity = structure.integrity();
		const auto degradation = structureDegradationPercent(structure);
		const auto priority = maintenancePriorityScore(structure);

		if (integrity < MaintenanceHighPriorityIntegrityThreshold)
		{
			return std::to_string(integrity) + "% integrity, " + std::to_string(degradation) + "% degraded, priority " + std::to_string(priority);
		}
		if (integrity < 100)
		{
			return std::to_string(integrity) + "% integrity, " + std::to_string(degradation) + "% degraded";
		}

		return std::to_string(integrity) + "% integrity, maintained";
	}


	std::vector<Structure*> allMaintainedStructures(const StructureManager& structureManager)
	{
		std::vector<Structure*> structures;
		for (auto* structure : structureManager.allStructures())
		{
			if (structure->destroyed()) { continue; }
			structures.push_back(structure);
		}

		std::sort(structures.begin(), structures.end(), [](const Structure* lhs, const Structure* rhs)
		{
			return maintenancePriorityScore(*lhs) > maintenancePriorityScore(*rhs);
		});

		return structures;
	}
}


MaintenanceReport::MaintenanceReport(const StructureManager& structureManager, TakeMeThereDelegate takeMeThereHandler) :
	mStructureManager{structureManager},
	mTakeMeThereHandler{takeMeThereHandler},
	mFontMedium{getFontMedium()},
	mFontMediumBold{getFontMediumBold()},
	mFontBigBold{getFontHugeBold()},
	mImageMaintenance{getImage("structures/maintenance.png")},
	btnShowAll{"All", {this, &MaintenanceReport::onShowAll}},
	btnNeedsRepair{"Needs Repair", {this, &MaintenanceReport::onNeedsRepair}},
	btnHighPriority{"High Priority", {this, &MaintenanceReport::onHighPriority}},
	btnDisabled{"Disabled", {this, &MaintenanceReport::onDisabled}},
	btnTakeMeThere{constants::TakeMeThere, {this, &MaintenanceReport::onTakeMeThere}},
	lstStructures{{this, &MaintenanceReport::onStructureSelectionChange}}
{
	auto buttonOffset = filterButtonSectionOffset;
	for (auto* button : {&btnShowAll, &btnNeedsRepair, &btnHighPriority, &btnDisabled})
	{
		button->size(filterButtonSize);
		button->type(Button::Type::Toggle);
		button->toggle(false);
		add(*button, buttonOffset);
		buttonOffset.x += button->size().x + constants::MarginTight;
	}

	btnShowAll.toggle(true);
	btnTakeMeThere.size({140, 30});

	NAS2D::Utility<NAS2D::EventHandler>::get().mouseDoubleClick().connect({this, &MaintenanceReport::onDoubleClick});

	fillLists();

	add(btnTakeMeThere, {10, 10});
	add(lstStructures, structureListBoxOffset);
}


MaintenanceReport::~MaintenanceReport()
{
	NAS2D::Utility<NAS2D::EventHandler>::get().mouseDoubleClick().disconnect({this, &MaintenanceReport::onDoubleClick});
}


bool MaintenanceReport::canView(const Structure& structure)
{
	return dynamic_cast<const MaintenanceFacility*>(&structure);
}


void MaintenanceReport::selectStructure(Structure& structure)
{
	lstStructures.setSelected(&structure);
}


void MaintenanceReport::clearSelected()
{
	lstStructures.clearSelected();
}


void MaintenanceReport::fillLists()
{
	mStructuresNeedingRepair = 0;
	mHighPriorityCount = 0;

	for (auto* structure : mStructureManager.allStructures())
	{
		if (structureNeedsMaintenance(*structure)) { ++mStructuresNeedingRepair; }
		if (structure->integrity() < MaintenanceHighPriorityIntegrityThreshold && !structure->destroyed() && !structure->underConstruction())
		{
			++mHighPriorityCount;
		}
	}

	fillListFromStructures(allMaintainedStructures(mStructureManager));
}


void MaintenanceReport::refresh()
{
	onShowAll();
}


void MaintenanceReport::fillListFromStructures(const std::vector<Structure*>& structures)
{
	lstStructures.clear();

	for (auto* structure : structures)
	{
		lstStructures.addItem(structure, maintenanceStateDescription(*structure));
	}
}


void MaintenanceReport::onShowAll()
{
	filterButtonClicked();
	btnShowAll.toggle(true);
	fillLists();
}


void MaintenanceReport::onNeedsRepair()
{
	filterButtonClicked();
	btnNeedsRepair.toggle(true);

	std::vector<Structure*> structures;
	for (auto* structure : mStructureManager.allStructures())
	{
		if (structureNeedsMaintenance(*structure))
		{
			structures.push_back(structure);
		}
	}

	std::sort(structures.begin(), structures.end(), [](const Structure* lhs, const Structure* rhs)
	{
		return maintenancePriorityScore(*lhs) > maintenancePriorityScore(*rhs);
	});

	fillListFromStructures(structures);
}


void MaintenanceReport::onHighPriority()
{
	filterButtonClicked();
	btnHighPriority.toggle(true);

	std::vector<Structure*> structures;
	for (auto* structure : mStructureManager.allStructures())
	{
		if (structure->destroyed() || structure->underConstruction()) { continue; }
		if (structure->integrity() < MaintenanceHighPriorityIntegrityThreshold)
		{
			structures.push_back(structure);
		}
	}

	std::sort(structures.begin(), structures.end(), [](const Structure* lhs, const Structure* rhs)
	{
		return maintenancePriorityScore(*lhs) > maintenancePriorityScore(*rhs);
	});

	fillListFromStructures(structures);
}


void MaintenanceReport::onDisabled()
{
	filterButtonClicked();
	btnDisabled.toggle(true);

	std::vector<Structure*> structures;
	for (auto* structure : mStructureManager.allStructures())
	{
		if (structure->disabled())
		{
			structures.push_back(structure);
		}
	}

	fillListFromStructures(structures);
}


void MaintenanceReport::filterButtonClicked()
{
	for (auto* button : {&btnShowAll, &btnNeedsRepair, &btnHighPriority, &btnDisabled})
	{
		button->toggle(false);
	}
}


void MaintenanceReport::onStructureSelectionChange()
{
}


void MaintenanceReport::onTakeMeThere()
{
	if (const auto* structure = lstStructures.selectedStructure())
	{
		if (mTakeMeThereHandler) { mTakeMeThereHandler(structure); }
	}
}


void MaintenanceReport::onDoubleClick(NAS2D::MouseButton button, NAS2D::Point<int> /*position*/)
{
	if (!visible() || button != NAS2D::MouseButton::Left) { return; }
	onTakeMeThere();
}


void MaintenanceReport::drawLeftPanel(NAS2D::Renderer& renderer) const
{
	const auto centerX = area().center().x;
	const auto panelTop = area().position.y + filterButtonSectionOffset.y;
	const auto leftPanel = NAS2D::Rectangle<int>{
		{area().position.x, panelTop},
		{centerX - area().position.x, area().size.y - filterButtonSectionOffset.y}
	};
	renderer.drawBoxFilled(leftPanel, NAS2D::Color{25, 25, 25});

	const auto infoOrigin = area().position + infoSectionOffset;
	renderer.drawText(mFontMediumBold, "Colony Maintenance", infoOrigin, constants::PrimaryTextColor);
	renderer.drawText(mFontMedium, "Structures needing repair: " + std::to_string(mStructuresNeedingRepair), infoOrigin + NAS2D::Vector{0, 22}, constants::PrimaryTextColor);
	renderer.drawText(mFontMedium, "High priority (< " + std::to_string(MaintenanceHighPriorityIntegrityThreshold) + "% integrity): " + std::to_string(mHighPriorityCount), infoOrigin + NAS2D::Vector{0, 42}, constants::PrimaryTextColor);
}


void MaintenanceReport::drawRightPanel(NAS2D::Renderer& renderer) const
{
	const auto centerX = area().center().x;
	const auto panelTop = area().position.y + filterButtonSectionOffset.y;
	const auto rightPanel = NAS2D::Rectangle<int>{
		{centerX, panelTop},
		{area().endPoint().x - centerX, area().size.y - filterButtonSectionOffset.y}
	};
	renderer.drawBoxFilled(rightPanel, NAS2D::Color{20, 20, 20});

	if (const auto* structure = lstStructures.selectedStructure())
	{
		const auto detailsOrigin = rightPanel.position + NAS2D::Vector{10, rightDetailsTop};
		renderer.drawText(mFontBigBold, structure->name(), detailsOrigin, constants::PrimaryTextColor);
		renderer.drawText(mFontMedium, maintenanceStateDescription(*structure), detailsOrigin + NAS2D::Vector{0, 30}, constants::PrimaryTextColor);
		renderer.drawText(mFontMedium, "Integrity: " + std::to_string(structure->integrity()) + "%", detailsOrigin + NAS2D::Vector{0, 52}, constants::PrimaryTextColor);
		renderer.drawText(mFontMedium, "Degradation: " + std::to_string(structureDegradationPercent(*structure)) + "%", detailsOrigin + NAS2D::Vector{0, 74}, constants::PrimaryTextColor);
		renderer.drawText(mFontMedium, "Priority score: " + std::to_string(maintenancePriorityScore(*structure)), detailsOrigin + NAS2D::Vector{0, 96}, constants::PrimaryTextColor);
	}
	else
	{
		const auto hintOrigin = rightPanel.position + NAS2D::Vector{10, rightHeaderHeight + 20};
		renderer.drawText(mFontMedium, "Select a structure to view maintenance details.", hintOrigin, NAS2D::Color::Yellow);
	}
}


void MaintenanceReport::update()
{
	if (!visible()) { return; }

	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	draw(renderer);
	ControlContainer::update();
}


void MaintenanceReport::draw(NAS2D::Renderer& renderer) const
{
	if (!visible()) { return; }

	drawLeftPanel(renderer);
	drawRightPanel(renderer);
}


void MaintenanceReport::onResize()
{
	Control::onResize();

	const auto centerX = area().center().x;
	const auto listHeight = area().size.y - structureListBoxOffset.y - 10;
	lstStructures.size({centerX - structureListBoxOffset.x - 10, std::max(listHeight, 100)});
	const auto rightWidth = area().endPoint().x - centerX;
	btnTakeMeThere.position({centerX + rightWidth - btnTakeMeThere.size().x - 10, area().position.y + 12});
}