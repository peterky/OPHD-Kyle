#pragma once

#include "Wrapper.h"

#include <NAS2D/Signal/Delegate.h>
#include <NAS2D/Math/Point.h>
#include <NAS2D/Math/Rectangle.h>
#include <NAS2D/Math/Vector.h>

#include <array>
#include <cstdint>
#include <string>


class StructureManager;
class Structure;
class OreHaulRoutes;
class TechnologyCatalog;
class ResearchTracker;
class ColonyProductionHistory;
class ColonyOrbitalProgram;
class PopulationModel;
class PopulationPool;
class TileMap;
struct StorableResources;

namespace NAS2D
{
	enum class MouseButton;

	class Font;
	class Image;
	class Renderer;

	template <typename BaseType> struct Point;
	template <typename BaseType> struct Vector;
}


class Report;


class ReportsState : public Wrapper
{
public:
	using TakeMeThereDelegate = NAS2D::Delegate<void(const Structure*)>;
	using ShowReportsDelegate = NAS2D::Delegate<void()>;
	using HideReportsDelegate = NAS2D::Delegate<void()>;

public:
	ReportsState(const StructureManager& structureManager, TakeMeThereDelegate takeMeThereHandler, ShowReportsDelegate showReportsHandler, HideReportsDelegate hideReportsHandler);

	~ReportsState() override;

	enum class ReportPanel
	{
		Research,
		Factories,
		Warehouses,
		Mines,
		Production,
		Maintenance,
		Workforce,
		Satellites,
		Spaceports
	};

	void showReport();
	void showReport(Structure& structure);
	void showReportPanel(ReportPanel panel);

	void injectOreHaulRoutes(const OreHaulRoutes& oreHaulRoutes);
	void injectProductionHistory(const ColonyProductionHistory& history);
	void injectWorkforce(PopulationModel& populationModel, const PopulationPool& populationPool);
	void injectTechnology(TechnologyCatalog&, ResearchTracker&);
	void injectOrbitalProgram(
		ColonyOrbitalProgram& program,
		TileMap& tileMap,
		StorableResources& resources,
		TechnologyCatalog& catalog,
		ResearchTracker& tracker,
		NAS2D::Delegate<void(const std::string&)> launchHandler);

	void clearLists();

	void initialize() override;
	State* update() override;

protected:
	void onActivate() override;
	void onDeactivate() override;

	void onWindowResized(NAS2D::Vector<int> newSize);
	void onTakeMeThere(const Structure* structure);
	void onMouseDown(NAS2D::MouseButton button, NAS2D::Point<int> position);
	void onMouseMove(NAS2D::Point<int> position, NAS2D::Vector<int> relative);
	void onExit();

	void deselectAllPanels();
	void selectPanel(std::size_t panelIndex);
	void initializePanels();
	void onResizeTabBar(int width);
	void drawTab(NAS2D::Renderer& renderer, std::size_t panelIndex);

	struct ReportTab
	{
		ReportTab() = default;
		ReportTab(Report* newReport, std::string newName, const NAS2D::Image* newIcon) :
			report{newReport},
			name{std::move(newName)},
			icon{newIcon}
		{
		}

		Report* report = nullptr;
		std::string name;
		const NAS2D::Image* icon = nullptr;
		NAS2D::Rectangle<int> tabArea;
		NAS2D::Point<int> textPosition;
		NAS2D::Point<int> iconPosition;

		void select(Structure& structure);
		void selected(bool isSelected);
		bool selected() const { return mIsSelected; }
		void highlighted(bool isHighlighted) { mIsHighlighted = isHighlighted; }
		bool highlighted() const { return mIsHighlighted; }

	private:
		bool mIsSelected = false;
		bool mIsHighlighted = false;
	};

private:
	static constexpr std::size_t PanelCount = 10;
	static constexpr std::size_t ResearchPanelIndex = 0;
	static constexpr std::size_t FactoriesPanelIndex = 1;
	static constexpr std::size_t WarehousesPanelIndex = 2;
	static constexpr std::size_t MinePanelIndex = 3;
	static constexpr std::size_t ProductionPanelIndex = 4;
	static constexpr std::size_t MaintenancePanelIndex = 5;
	static constexpr std::size_t WorkforcePanelIndex = 6;
	static constexpr std::size_t SatellitesPanelIndex = 7;
	static constexpr std::size_t SpaceportsPanelIndex = 8;
	static constexpr std::size_t ExitPanelIndex = 9;

	const NAS2D::Font& fontMain;
	const StructureManager& mStructureManager;
	TakeMeThereDelegate mTakeMeThereHandler;
	ShowReportsDelegate mShowReportsHandler;
	HideReportsDelegate mHideReportsHandler;
	std::array<ReportTab, PanelCount> mPanels{};
};
