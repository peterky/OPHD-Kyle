#include "ProductionReport.h"

#include "../../Constants/UiConstants.h"
#include "../../Resources.h"

#include <NAS2D/EnumKeyCode.h>
#include <NAS2D/EnumMouseButton.h>
#include <NAS2D/EventHandler.h>
#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/Resource/Font.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>


extern NAS2D::Point<int> MOUSE_COORDS;


namespace
{
	constexpr int MinDisplayTurns{5};
	constexpr int MaxDisplayTurns{200};
	constexpr int DefaultDisplayTurns{50};
	constexpr int LegendRowHeight{22};
	constexpr int LineHitThreshold{8};
	constexpr int TooltipPadding{6};


	double distanceToSegment(NAS2D::Point<int> point, NAS2D::Point<int> start, NAS2D::Point<int> end)
	{
		const auto dx = static_cast<double>(end.x - start.x);
		const auto dy = static_cast<double>(end.y - start.y);
		const auto lengthSquared = dx * dx + dy * dy;

		if (lengthSquared <= 0.0)
		{
			const auto px = static_cast<double>(point.x - start.x);
			const auto py = static_cast<double>(point.y - start.y);
			return std::sqrt(px * px + py * py);
		}

		auto t = ((point.x - start.x) * dx + (point.y - start.y) * dy) / lengthSquared;
		t = std::clamp(t, 0.0, 1.0);

		const auto projX = start.x + static_cast<int>(std::lround(t * dx));
		const auto projY = start.y + static_cast<int>(std::lround(t * dy));
		const auto distX = static_cast<double>(point.x - projX);
		const auto distY = static_cast<double>(point.y - projY);
		return std::sqrt(distX * distX + distY * distY);
	}
}


ProductionReport::ProductionReport(const StructureManager& structureManager, TakeMeThereDelegate takeMeThereHandler) :
	mStructureManager{structureManager},
	mTakeMeThereHandler{takeMeThereHandler},
	mFont{Control::getDefaultFont()},
	mFontBold{Control::getDefaultFontBold()},
	mFontMediumBold{Control::getDefaultFontBold()},
	lblPeriod{"Plot last N turns:"},
	txtPeriod{3, {this, &ProductionReport::onPeriodChanged}},
	mSeriesInfo{{
		{SeriesId::CommonMetals, ResourceNamesRefined[0], NAS2D::Color{180, 140, 80}},
		{SeriesId::CommonMinerals, ResourceNamesRefined[1], NAS2D::Color{120, 180, 120}},
		{SeriesId::RareMetals, ResourceNamesRefined[2], NAS2D::Color{160, 160, 200}},
		{SeriesId::RareMinerals, ResourceNamesRefined[3], NAS2D::Color{200, 120, 180}},
		{SeriesId::Food, "Food", NAS2D::Color{220, 200, 80}},
		{SeriesId::Population, "Population", NAS2D::Color{100, 200, 220}},
		{SeriesId::TrucksAvailable, "Trucks (storage)", NAS2D::Color{200, 160, 80}},
		{SeriesId::TrucksDeployed, "Trucks (deployed)", NAS2D::Color{220, 120, 60}},
		{SeriesId::RobotsAvailable, "Robots (available)", NAS2D::Color{120, 220, 120}},
		{SeriesId::RobotsTotal, "Robots (total)", NAS2D::Color{80, 180, 100}},
	}}
{
	mSeriesChecks.fill(true);

	txtPeriod.text(std::to_string(DefaultDisplayTurns));
	txtPeriod.numbersOnly(true);
	txtPeriod.border(TextField::BorderVisibility::Always);

	position({0, 0});
	size({800, 600});

	const auto left = 20;
	auto rowY = 60;
	add(lblPeriod, {left, rowY});
	txtPeriod.size({60, 24});
	add(txtPeriod, {left + 140, rowY});

	auto& eventHandler = NAS2D::Utility<NAS2D::EventHandler>::get();
	eventHandler.mouseButtonDown().connect({this, &ProductionReport::onMouseDown});
	eventHandler.mouseMotion().connect({this, &ProductionReport::onMouseMove});
}


ProductionReport::~ProductionReport()
{
	auto& eventHandler = NAS2D::Utility<NAS2D::EventHandler>::get();
	eventHandler.mouseButtonDown().disconnect({this, &ProductionReport::onMouseDown});
	eventHandler.mouseMotion().disconnect({this, &ProductionReport::onMouseMove});
}


bool ProductionReport::canView(const Structure& /*structure*/)
{
	return false;
}


void ProductionReport::selectStructure(Structure&)
{
}


void ProductionReport::clearSelected()
{
}


void ProductionReport::fillLists()
{
}


void ProductionReport::refresh()
{
}


void ProductionReport::injectHistory(const ColonyProductionHistory& history)
{
	mHistory = &history;
}


void ProductionReport::onPeriodChanged(TextField& field)
{
	const auto turns = std::atoi(field.text().c_str());
	mDisplayTurns = std::clamp(turns > 0 ? turns : DefaultDisplayTurns, MinDisplayTurns, MaxDisplayTurns);
	field.text(std::to_string(mDisplayTurns));
}


int ProductionReport::seriesValue(SeriesId id, const ColonyTurnSnapshot& snapshot) const
{
	switch (id)
	{
	case SeriesId::CommonMetals: return snapshot.refinedResources[0];
	case SeriesId::CommonMinerals: return snapshot.refinedResources[1];
	case SeriesId::RareMetals: return snapshot.refinedResources[2];
	case SeriesId::RareMinerals: return snapshot.refinedResources[3];
	case SeriesId::Food: return snapshot.food;
	case SeriesId::Population: return snapshot.population;
	case SeriesId::TrucksAvailable: return snapshot.trucksAvailable;
	case SeriesId::TrucksDeployed: return snapshot.trucksDeployed;
	case SeriesId::RobotsAvailable: return snapshot.robotsAvailable;
	case SeriesId::RobotsTotal: return snapshot.robotsTotal;
	default: return 0;
	}
}


std::optional<ProductionReport::ChartLayout> ProductionReport::buildChartLayout() const
{
	if (!mHistory || mHistory->snapshots().size() < 2) { return std::nullopt; }

	const auto& snapshots = mHistory->snapshots();
	const auto startIndex = snapshots.size() > static_cast<std::size_t>(mDisplayTurns) ?
		snapshots.size() - static_cast<std::size_t>(mDisplayTurns) : 0;
	const auto visibleCount = snapshots.size() - startIndex;
	if (visibleCount < 2) { return std::nullopt; }

	ChartLayout layout;
	layout.chartArea = NAS2D::Rectangle{
		area().position + NAS2D::Vector{20, 90},
		NAS2D::Vector{area().size.x - 300, area().size.y - 110}};
	layout.plotArea = layout.chartArea.inset({40, 20}, {20, 30});
	layout.startIndex = startIndex;
	layout.visibleCount = visibleCount;
	layout.plotWidth = std::max(1, layout.plotArea.size.x - 1);
	layout.plotHeight = std::max(1, layout.plotArea.size.y - 1);

	for (std::size_t i = startIndex; i < snapshots.size(); ++i)
	{
		for (std::size_t s = 0; s < mSeriesInfo.size(); ++s)
		{
			if (!mSeriesChecks[s]) { continue; }
			layout.maxValue = std::max(layout.maxValue, seriesValue(mSeriesInfo[s].id, snapshots[i]));
		}
	}

	return layout;
}


NAS2D::Point<int> ProductionReport::plotPointForIndex(const ChartLayout& layout, std::size_t seriesIndex, std::size_t snapshotOffset) const
{
	const auto& snapshots = mHistory->snapshots();
	const auto& snapshot = snapshots[layout.startIndex + snapshotOffset];
	const auto value = seriesValue(mSeriesInfo[seriesIndex].id, snapshot);
	const auto x = layout.plotArea.position.x + static_cast<int>((snapshotOffset * layout.plotWidth) / (layout.visibleCount - 1));
	const auto y = layout.plotArea.position.y + layout.plotHeight - static_cast<int>((value * layout.plotHeight) / layout.maxValue);
	return NAS2D::Point<int>{x, y};
}


void ProductionReport::updateChartHover(NAS2D::Point<int> mousePosition)
{
	mChartHover = {};

	if (!visible()) { return; }

	const auto layout = buildChartLayout();
	if (!layout || !layout->plotArea.contains(mousePosition)) { return; }

	double bestDistance = static_cast<double>(LineHitThreshold) + 1.0;
	std::size_t bestSeries{0};
	std::size_t bestSnapshotOffset{0};
	NAS2D::Point<int> bestPoint;

	for (std::size_t s = 0; s < mSeriesInfo.size(); ++s)
	{
		if (!mSeriesChecks[s]) { continue; }

		auto previousPoint = plotPointForIndex(*layout, s, 0);
		for (std::size_t i = 1; i < layout->visibleCount; ++i)
		{
			const auto currentPoint = plotPointForIndex(*layout, s, i);
			const auto distance = distanceToSegment(mousePosition, previousPoint, currentPoint);

			if (distance < bestDistance)
			{
				const auto distToPrevious = std::hypot(
					static_cast<double>(mousePosition.x - previousPoint.x),
					static_cast<double>(mousePosition.y - previousPoint.y));
				const auto distToCurrent = std::hypot(
					static_cast<double>(mousePosition.x - currentPoint.x),
					static_cast<double>(mousePosition.y - currentPoint.y));

				bestDistance = distance;
				bestSeries = s;
				bestSnapshotOffset = distToPrevious <= distToCurrent ? i - 1 : i;
				bestPoint = distToPrevious <= distToCurrent ? previousPoint : currentPoint;
			}

			previousPoint = currentPoint;
		}
	}

	if (bestDistance > LineHitThreshold) { return; }

	const auto& snapshot = mHistory->snapshots()[layout->startIndex + bestSnapshotOffset];
	mChartHover.active = true;
	mChartHover.seriesIndex = bestSeries;
	mChartHover.turn = snapshot.turn;
	mChartHover.value = seriesValue(mSeriesInfo[bestSeries].id, snapshot);
	mChartHover.plotPoint = bestPoint;
}


void ProductionReport::onMouseMove(NAS2D::Point<int> position, NAS2D::Vector<int> /*relative*/)
{
	updateChartHover(position);
}


void ProductionReport::onMouseDown(NAS2D::MouseButton button, NAS2D::Point<int> position)
{
	if (!visible() || button != NAS2D::MouseButton::Left) { return; }

	const auto legendOrigin = area().position + NAS2D::Vector{area().size.x - 260, 90};
	const auto legendWidth = 240;

	if (!NAS2D::Rectangle<int>{legendOrigin, {legendWidth, static_cast<int>(mSeriesInfo.size()) * LegendRowHeight}}.contains(position))
	{
		return;
	}

	const auto row = (position.y - legendOrigin.y) / LegendRowHeight;
	if (row < 0 || row >= static_cast<int>(mSeriesInfo.size())) { return; }

	mSeriesChecks[static_cast<std::size_t>(row)] = !mSeriesChecks[static_cast<std::size_t>(row)];
}


void ProductionReport::drawLegend(NAS2D::Renderer& renderer, const NAS2D::Point<int>& origin) const
{
	renderer.drawText(mFontMediumBold, "Series (click to toggle)", origin, constants::PrimaryTextColor);

	auto rowY = origin.y + mFontMediumBold.height() + constants::MarginTight;
	for (std::size_t i = 0; i < mSeriesInfo.size(); ++i)
	{
		const auto& series = mSeriesInfo[i];
		const auto enabled = mSeriesChecks[i];
		const auto rowOrigin = NAS2D::Point{origin.x, rowY};

		renderer.drawBoxFilled(NAS2D::Rectangle{rowOrigin, {14, 14}}, enabled ? series.color : NAS2D::Color{60, 60, 60});
		renderer.drawText(mFont, series.name, rowOrigin + NAS2D::Vector{20, -2}, enabled ? constants::PrimaryTextColor : NAS2D::Color{120, 120, 120});
		rowY += LegendRowHeight;
	}
}


void ProductionReport::drawChart(NAS2D::Renderer& renderer, const NAS2D::Rectangle<int>& chartArea) const
{
	renderer.drawBoxFilled(chartArea, NAS2D::Color{25, 25, 25});
	renderer.drawBox(chartArea, constants::PrimaryTextColor);

	if (!mHistory || mHistory->snapshots().empty())
	{
		renderer.drawText(mFont, "No production history yet. Advance a few turns to populate this chart.", chartArea.position + NAS2D::Vector{20, chartArea.size.y / 2}, NAS2D::Color::Yellow);
		return;
	}

	const auto layout = buildChartLayout();
	if (!layout) { return; }

	for (std::size_t s = 0; s < mSeriesInfo.size(); ++s)
	{
		if (!mSeriesChecks[s]) { continue; }

		const auto& series = mSeriesInfo[s];
		NAS2D::Point<int> previousPoint{-1, -1};

		for (std::size_t i = 0; i < layout->visibleCount; ++i)
		{
			const auto point = plotPointForIndex(*layout, s, i);

			if (previousPoint.x >= 0)
			{
				renderer.drawLine(previousPoint, point, series.color, 2);
			}
			previousPoint = point;
		}
	}

	if (mChartHover.active)
	{
		const auto& series = mSeriesInfo[mChartHover.seriesIndex];
		const auto markerRect = NAS2D::Rectangle<int>{mChartHover.plotPoint - NAS2D::Vector{3, 3}, {6, 6}};
		renderer.drawBoxFilled(markerRect, series.color);
		renderer.drawBox(markerRect, NAS2D::Color::White);
	}

	const auto& snapshots = mHistory->snapshots();
	const auto& firstTurn = snapshots[layout->startIndex].turn;
	const auto& lastTurn = snapshots.back().turn;
	const auto axisLabelY = chartArea.position.y + chartArea.size.y - 24;
	renderer.drawText(mFont, "Turn " + std::to_string(firstTurn), NAS2D::Point<int>{layout->plotArea.position.x, axisLabelY}, constants::PrimaryTextColor);
	const auto lastLabel = "Turn " + std::to_string(lastTurn);
	const auto lastLabelSize = mFont.size(lastLabel);
	renderer.drawText(mFont, lastLabel, NAS2D::Point<int>{layout->plotArea.position.x + layout->plotArea.size.x - lastLabelSize.x, axisLabelY}, constants::PrimaryTextColor);
	renderer.drawText(mFont, std::to_string(layout->maxValue), NAS2D::Point<int>{chartArea.position.x + 4, layout->plotArea.position.y}, constants::PrimaryTextColor);
}


void ProductionReport::drawChartTooltip(NAS2D::Renderer& renderer) const
{
	if (!mChartHover.active) { return; }

	const auto& series = mSeriesInfo[mChartHover.seriesIndex];
	const auto tooltipText = series.name + " — Turn " + std::to_string(mChartHover.turn) + ": " + std::to_string(mChartHover.value);
	const auto textSize = mFont.size(tooltipText);
	const auto tooltipSize = textSize + NAS2D::Vector{TooltipPadding * 2, TooltipPadding * 2};

	auto tooltipPosition = mChartHover.plotPoint + NAS2D::Vector{12, -tooltipSize.y - 8};
	const auto reportBounds = area();

	if (tooltipPosition.x + tooltipSize.x > reportBounds.endPoint().x)
	{
		tooltipPosition.x = reportBounds.endPoint().x - tooltipSize.x - 4;
	}
	if (tooltipPosition.y < reportBounds.position.y)
	{
		tooltipPosition.y = mChartHover.plotPoint.y + 12;
	}

	const auto tooltipRect = NAS2D::Rectangle<int>{tooltipPosition, tooltipSize};
	renderer.drawBoxFilled(tooltipRect, NAS2D::Color{245, 245, 245});
	renderer.drawBox(tooltipRect, series.color);
	renderer.drawText(mFont, tooltipText, tooltipPosition + NAS2D::Vector{TooltipPadding, TooltipPadding}, NAS2D::Color::Black);
}


void ProductionReport::update()
{
	if (!visible()) { return; }

	updateChartHover(MOUSE_COORDS);

	ControlContainer::update();
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	draw(renderer);
}


void ProductionReport::draw(NAS2D::Renderer& renderer) const
{
	if (!visible()) { return; }

	const auto origin = area().position + NAS2D::Vector{20, 20};
	renderer.drawText(mFontMediumBold, "Colony Production", origin, constants::PrimaryTextColor);

	const auto chartOrigin = area().position + NAS2D::Vector{20, 90};
	const auto chartSize = NAS2D::Vector{area().size.x - 300, area().size.y - 110};
	drawChart(renderer, NAS2D::Rectangle{chartOrigin, chartSize});
	drawLegend(renderer, area().position + NAS2D::Vector{area().size.x - 260, 90});
	drawChartTooltip(renderer);
}


void ProductionReport::onResize()
{
	Control::onResize();

	const auto origin = area().position + NAS2D::Vector{20, 20};
	const auto controlsY = origin.y + mFontMediumBold.height() + constants::MarginTight * 2;
	lblPeriod.position({origin.x, controlsY});
	txtPeriod.position({origin.x + 150, controlsY});
}