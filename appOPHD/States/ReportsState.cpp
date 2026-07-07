#include "ReportsState.h"

#include "../CacheImage.h"
#include "../CacheFont.h"
#include "../Constants/UiConstants.h"
#include "../StructureManager.h"
#include "../MapObjects/Structure.h"

#include "../UI/Reports/Report.h"

#include "../UI/Reports/FactoryReport.h"
#include "../UI/Reports/MineReport.h"
#include "../UI/Reports/ResearchReport.h"
#include "../UI/Reports/SatellitesReport.h"
#include "../UI/Reports/SpaceportsReport.h"
#include "../UI/Reports/WarehouseReport.h"
#include "../UI/Reports/ProductionReport.h"
#include "../UI/Reports/MaintenanceReport.h"
#include "../UI/Reports/WorkforceReport.h"
#include "../ColonyProductionHistory.h"
#include "../ColonyOrbitalProgram.h"
#include "../Map/TileMap.h"

#include <libOPHD/StorableResources.h>

#include <libOPHD/Population/PopulationModel.h>
#include <libOPHD/Population/PopulationPool.h>

#include <NAS2D/EnumKeyCode.h>
#include <NAS2D/EnumMouseButton.h>
#include <NAS2D/Utility.h>
#include <NAS2D/EventHandler.h>
#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/Resource/Font.h>
#include <NAS2D/Math/Point.h>
#include <NAS2D/Math/Vector.h>


void ReportsState::ReportTab::select(Structure& structure)
{
	selected(true);
	if (!report) { return; }

	report->visible(true);
	report->refresh();
	report->selectStructure(structure);
}


void ReportsState::ReportTab::selected(bool isSelected)
{
	mIsSelected = isSelected;
	if (report)
	{
		report->enabled(isSelected);
	}
}


ReportsState::ReportsState(const StructureManager& structureManager, TakeMeThereDelegate takeMeThereHandler, ShowReportsDelegate showReportsHandler, HideReportsDelegate hideReportsHandler) :
	fontMain{getFontBold(16)},
	mStructureManager{structureManager},
	mTakeMeThereHandler{takeMeThereHandler},
	mShowReportsHandler{showReportsHandler},
	mHideReportsHandler{hideReportsHandler}
{
	auto& eventHandler = NAS2D::Utility<NAS2D::EventHandler>::get();
	eventHandler.windowResized().connect({this, &ReportsState::onWindowResized});
	eventHandler.mouseButtonDown().connect({this, &ReportsState::onMouseDown});
	eventHandler.mouseMotion().connect({this, &ReportsState::onMouseMove});
}


ReportsState::~ReportsState()
{
	auto& eventHandler = NAS2D::Utility<NAS2D::EventHandler>::get();
	eventHandler.windowResized().disconnect({this, &ReportsState::onWindowResized});
	eventHandler.mouseButtonDown().disconnect({this, &ReportsState::onMouseDown});
	eventHandler.mouseMotion().disconnect({this, &ReportsState::onMouseMove});

	for (ReportTab& panel : mPanels)
	{
		delete panel.report;
		panel.report = nullptr;
	}
}


void ReportsState::initialize()
{
	initializePanels();
	const auto size = NAS2D::Utility<NAS2D::Renderer>::get().size().to<int>();
	onWindowResized(size);
}


void ReportsState::initializePanels()
{
	for (ReportTab& panel : mPanels)
	{
		delete panel.report;
		panel.report = nullptr;
	}

	mPanels = std::array<ReportTab, PanelCount>{
		ReportTab{new ResearchReport(mStructureManager, mTakeMeThereHandler), "Research", nullptr},
		ReportTab{new FactoryReport(mStructureManager, mTakeMeThereHandler), "Factory", nullptr},
		ReportTab{new WarehouseReport(mStructureManager, mTakeMeThereHandler), "Stores", nullptr},
		ReportTab{new MineReport(mStructureManager, mTakeMeThereHandler), "Mines", nullptr},
		ReportTab{new ProductionReport(mStructureManager, mTakeMeThereHandler), "Output", nullptr},
		ReportTab{new MaintenanceReport(mStructureManager, mTakeMeThereHandler), "Maint.", nullptr},
		ReportTab{new WorkforceReport(mStructureManager, mTakeMeThereHandler), "Workers", nullptr},
		ReportTab{new SatellitesReport(mStructureManager, mTakeMeThereHandler), "Sats", nullptr},
		ReportTab{new SpaceportsReport(mStructureManager, mTakeMeThereHandler), "Ports", nullptr},
		ReportTab{nullptr, "", nullptr}
	};

	for (auto& panel : mPanels)
	{
		if (panel.report)
		{
			panel.report->hide();
		}
	}
}


void ReportsState::onResizeTabBar(int width)
{
	auto& exitPanel = mPanels[ExitPanelIndex];
	exitPanel.tabArea = {{width - 48, 0}, {48, 48}};
	const auto exitLabel = std::string{"X"};
	exitPanel.textPosition = exitPanel.tabArea.position + (exitPanel.tabArea.size - fontMain.size(exitLabel)) / 2;

	const auto remainingWidth = width - exitPanel.tabArea.size.x;
	const auto panelSize = NAS2D::Vector{remainingWidth / 9, 48};

	auto panelPosition = NAS2D::Point{0, 0};
	for (std::size_t i = 0; i < mPanels.size() - 1; ++i)
	{
		auto& panel = mPanels[i];
		panel.tabArea = NAS2D::Rectangle{panelPosition, panelSize};
		panel.textPosition = panelPosition + (panelSize - fontMain.size(panel.name)) / 2;
		panelPosition.x += panelSize.x;
	}
}


void ReportsState::drawTab(NAS2D::Renderer& renderer, std::size_t panelIndex)
{
	auto& panel = mPanels[panelIndex];

	if (panel.highlighted())
	{
		renderer.drawBoxFilled(panel.tabArea, constants::HighlightColor);
	}

	const auto drawColor = panel.selected() ? constants::PrimaryColor : constants::SecondaryColor;

	if (panel.selected())
	{
		renderer.drawBoxFilled(panel.tabArea, constants::PrimaryColorVariant);

		if (panel.report)
		{
			panel.report->update();
		}
	}

	if (panelIndex == ExitPanelIndex)
	{
		renderer.drawText(fontMain, "X", panel.textPosition, drawColor);
	}
	else
	{
		renderer.drawText(fontMain, panel.name, panel.textPosition, drawColor);
	}
}


void ReportsState::onActivate()
{
	for (auto& panel : mPanels)
	{
		if (panel.report)
		{
			panel.report->fillLists();
			panel.report->refresh();
			panel.report->hide();
		}
	}
}


void ReportsState::onDeactivate()
{
	for (auto& panel : mPanels)
	{
		if (panel.report)
		{
			panel.report->hide();
			panel.report->clearSelected();
		}

		panel.selected(false);
	}
}


void ReportsState::onWindowResized(NAS2D::Vector<int> newSize)
{
	onResizeTabBar(newSize.x);
	for (ReportTab& panel : mPanels)
	{
		if (panel.report)
		{
			panel.report->area({{0, 48}, NAS2D::Vector{newSize.x, newSize.y - 48}});
		}
	}
}


void ReportsState::onTakeMeThere(const Structure* structure)
{
	for (auto& panel : mPanels)
	{
		if (panel.report && panel.report->canView(*structure) && !panel.selected())
		{
			return;
		}
	}

	if (mTakeMeThereHandler) { mTakeMeThereHandler(structure); }
}


void ReportsState::onMouseDown(NAS2D::MouseButton button, NAS2D::Point<int> position)
{
	if (!active())
	{
		return;
	}

	if (!NAS2D::Rectangle<int>{{0, 0}, {NAS2D::Utility<NAS2D::Renderer>::get().size().x, 40}}.contains(position))
	{
		return;
	}

	if (button == NAS2D::MouseButton::Left)
	{
		for (ReportTab& panel : mPanels)
		{
			const bool selected = panel.tabArea.contains(position);
			panel.selected(selected);

			if (panel.report)
			{
				panel.report->visible(selected);
			}
		}
	}

	if (mPanels[ExitPanelIndex].selected())
	{
		onExit();
	}
}


void ReportsState::onMouseMove(NAS2D::Point<int> position, NAS2D::Vector<int> /*relative*/)
{
	for (auto& panel : mPanels)
	{
		panel.highlighted(panel.tabArea.contains(position));
	}
}


void ReportsState::onExit()
{
	deselectAllPanels();

	for (auto& panel : mPanels)
	{
		if (panel.report)
		{
			panel.report->clearSelected();
		}
	}

	if (mHideReportsHandler) { mHideReportsHandler(); }
}


void ReportsState::deselectAllPanels()
{
	for (auto& panel : mPanels)
	{
		panel.selected(false);
	}
}


void ReportsState::showReport()
{
	if (mShowReportsHandler) { mShowReportsHandler(); }
}


void ReportsState::selectPanel(std::size_t panelIndex)
{
	for (std::size_t i = 0; i < mPanels.size(); ++i)
	{
		const bool isSelected = i == panelIndex;
		mPanels[i].selected(isSelected);

		if (mPanels[i].report)
		{
			mPanels[i].report->visible(isSelected);
		}
	}
}


void ReportsState::showReportPanel(ReportPanel panel)
{
	const auto panelIndex = static_cast<std::size_t>(panel);

	if (!active())
	{
		if (mShowReportsHandler) { mShowReportsHandler(); }
	}

	selectPanel(panelIndex);
}


void ReportsState::showReport(Structure& structure)
{
	for (auto& panel : mPanels)
	{
		if (panel.report && panel.report->canView(structure))
		{
			deselectAllPanels();
			panel.select(structure);
			if (mShowReportsHandler) { mShowReportsHandler(); }
			return;
		}
	}
}


void ReportsState::injectOreHaulRoutes(const OreHaulRoutes& oreHaulRoutes)
{
	auto* minePanel = mPanels[MinePanelIndex].report;
	if (!minePanel) { return; }
	dynamic_cast<MineReport&>(*minePanel).injectOreHaulRoutes(oreHaulRoutes);
}


void ReportsState::injectProductionHistory(const ColonyProductionHistory& history)
{
	auto* productionPanel = mPanels[ProductionPanelIndex].report;
	if (!productionPanel) { return; }
	dynamic_cast<ProductionReport&>(*productionPanel).injectHistory(history);
}


void ReportsState::injectWorkforce(PopulationModel& populationModel, const PopulationPool& populationPool)
{
	auto* workforcePanel = mPanels[WorkforcePanelIndex].report;
	if (!workforcePanel) { return; }
	dynamic_cast<WorkforceReport&>(*workforcePanel).injectPopulation(populationModel, populationPool);
}


void ReportsState::injectTechnology(TechnologyCatalog& catalog, ResearchTracker& tracker)
{
	auto* researchPanel = mPanels[ResearchPanelIndex].report;
	if (!researchPanel) { return; }
	dynamic_cast<ResearchReport&>(*researchPanel).injectTechReferences(catalog, tracker);
}


void ReportsState::injectOrbitalProgram(
	ColonyOrbitalProgram& program,
	TileMap& tileMap,
	StorableResources& resources,
	TechnologyCatalog& catalog,
	ResearchTracker& tracker,
	NAS2D::Delegate<void(const std::string&)> launchHandler)
{
	auto* satellitesPanel = mPanels[SatellitesPanelIndex].report;
	if (satellitesPanel)
	{
		dynamic_cast<SatellitesReport&>(*satellitesPanel).injectOrbitalProgram(program, tileMap, catalog, tracker);
	}

	auto* spaceportsPanel = mPanels[SpaceportsPanelIndex].report;
	if (spaceportsPanel)
	{
		dynamic_cast<SpaceportsReport&>(*spaceportsPanel).injectOrbitalProgram(program, tileMap, resources, catalog, tracker, launchHandler);
	}
}


void ReportsState::clearLists()
{
	for (auto& panel : mPanels)
	{
		if (panel.report)
		{
			panel.report->fillLists();
		}
	}
}


NAS2D::State* ReportsState::update()
{
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();

	renderer.clearScreen(NAS2D::Color{35, 35, 35});
	renderer.drawBoxFilled(NAS2D::Rectangle<int>{{0, 0}, {renderer.size().x, 48}}, NAS2D::Color::Black);

	for (std::size_t i = 0; i < mPanels.size(); ++i)
	{
		this->drawTab(renderer, i);
	}

	return this;
}