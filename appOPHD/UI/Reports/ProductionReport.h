#pragma once

#include "Report.h"

#include "../../ColonyProductionHistory.h"

#include <libControls/Label.h>
#include <libControls/TextField.h>

#include <array>
#include <optional>
#include <string>


namespace NAS2D
{
	class Font;
	class Image;

	template <typename BaseType> struct Point;
	template <typename BaseType> struct Rectangle;
}


class StructureManager;


class ProductionReport : public Report
{
public:
	using TakeMeThereDelegate = NAS2D::Delegate<void(const Structure*)>;

	ProductionReport(const StructureManager& structureManager, TakeMeThereDelegate takeMeThereHandler);
	~ProductionReport() override;

	bool canView(const Structure& structure) override;
	void selectStructure(Structure&) override;
	void clearSelected() override;
	void fillLists() override;
	void refresh() override;

	void injectHistory(const ColonyProductionHistory& history);

	void update() override;
	void draw(NAS2D::Renderer& renderer) const override;

protected:
	void onResize() override;
	void onMouseDown(NAS2D::MouseButton button, NAS2D::Point<int> position);
	void onMouseMove(NAS2D::Point<int> position, NAS2D::Vector<int> relative);

private:
	enum class SeriesId : std::size_t
	{
		CommonMetals,
		CommonMinerals,
		RareMetals,
		RareMinerals,
		Food,
		Population,
		TrucksAvailable,
		TrucksDeployed,
		RobotsAvailable,
		RobotsTotal,
		Count
	};

	struct SeriesInfo
	{
		SeriesId id;
		std::string name;
		NAS2D::Color color;
	};

	struct ChartLayout
	{
		NAS2D::Rectangle<int> chartArea;
		NAS2D::Rectangle<int> plotArea;
		std::size_t startIndex{0};
		std::size_t visibleCount{0};
		int maxValue{1};
		int plotWidth{1};
		int plotHeight{1};
	};

	struct ChartHoverInfo
	{
		bool active{false};
		std::size_t seriesIndex{0};
		int turn{0};
		int value{0};
		NAS2D::Point<int> plotPoint;
	};

	void onPeriodChanged(TextField& field);
	int seriesValue(SeriesId id, const ColonyTurnSnapshot& snapshot) const;
	NAS2D::Point<int> plotPointForIndex(const ChartLayout& layout, std::size_t seriesIndex, std::size_t snapshotOffset) const;
	std::optional<ChartLayout> buildChartLayout() const;
	void updateChartHover(NAS2D::Point<int> mousePosition);
	void drawChart(NAS2D::Renderer& renderer, const NAS2D::Rectangle<int>& chartArea) const;
	void drawLegend(NAS2D::Renderer& renderer, const NAS2D::Point<int>& origin) const;
	void drawChartTooltip(NAS2D::Renderer& renderer) const;

private:
	const StructureManager& mStructureManager;
	const ColonyProductionHistory* mHistory{nullptr};
	TakeMeThereDelegate mTakeMeThereHandler;

	const NAS2D::Font& mFont;
	const NAS2D::Font& mFontBold;
	const NAS2D::Font& mFontMediumBold;

	Label lblPeriod;
	TextField txtPeriod;
	std::array<bool, static_cast<std::size_t>(SeriesId::Count)> mSeriesChecks{};
	std::array<SeriesInfo, static_cast<std::size_t>(SeriesId::Count)> mSeriesInfo;

	int mDisplayTurns{50};
	mutable ChartHoverInfo mChartHover;
};