#include "MaintenanceTurnSummary.h"

#include "../CacheImage.h"

#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/Resource/Font.h>
#include <NAS2D/Resource/Image.h>

#include <array>
#include <string>


namespace
{
	constexpr int HudRowHeight{20};
	constexpr int HudIconEdge{20};
	constexpr int HudIconInsetX{5};


	NAS2D::Rectangle<float> hudIconDestination(NAS2D::Point<int> rowOrigin)
	{
		return NAS2D::Rectangle<float>{
			{static_cast<float>(rowOrigin.x + HudIconInsetX), static_cast<float>(rowOrigin.y + (HudRowHeight - HudIconEdge) / 2)},
			{static_cast<float>(HudIconEdge), static_cast<float>(HudIconEdge)}
		};
	}


	void drawMetricRow(
		NAS2D::Renderer& renderer,
		const NAS2D::Font& font,
		NAS2D::Point<int> rowOrigin,
		int panelWidth,
		const std::string& label,
		const std::string& value,
		NAS2D::Color valueColor = NAS2D::Color::White)
	{
		constexpr auto labelX = 30;
		constexpr auto rowTextY = 4;
		constexpr auto valuePad = 10;

		renderer.drawText(font, label, rowOrigin + NAS2D::Vector{labelX, rowTextY}, NAS2D::Color{180, 180, 180});
		const auto valueX = rowOrigin.x + panelWidth - font.width(value) - valuePad;
		renderer.drawText(font, value, NAS2D::Point{valueX, rowOrigin.y + rowTextY}, valueColor);
	}
}


void MaintenanceTurnSummary::setStats(const ColonyMaintenanceTurnStats& stats)
{
	mStats = stats;
}


void MaintenanceTurnSummary::draw(NAS2D::Renderer& renderer) const
{
	const auto& maintenanceIcon = getImage("structures/maintenance.png");
	const auto& font = Control::getDefaultFont();
	constexpr auto rowHeight = 20;
	constexpr auto labelColumn = 30;

	auto position = mRect.position;
	const auto maintenanceSrc = NAS2D::Rectangle<int>{{0, 0}, maintenanceIcon.size()};
	renderer.drawSubImageStretched(maintenanceIcon, hudIconDestination(position), maintenanceSrc.to<float>());
	renderer.drawText(font, "Maintenance", position + NAS2D::Vector{labelColumn, 4}, NAS2D::Color{160, 160, 160});
	position.y += rowHeight;

	struct Row
	{
		std::string label;
		std::string value;
		NAS2D::Color valueColor;
	};

	const auto hasData = mStats.personnelCapacity > 0 || mStats.required > 0 || mStats.wasteGenerated > 0;
	const auto wasteColor = mStats.residencesOverflowing > 0 ? NAS2D::Color{255, 180, 120} : NAS2D::Color::White;
	const auto wasteValue = hasData ?
		std::to_string(mStats.wasteProcessing) + "/" + std::to_string(mStats.wasteGenerated) +
			" (" + std::to_string(mStats.recyclingPlantsOperational) + " plants)" :
		"—";

	const std::array<Row, 4> rows{{
		{"Repaired", hasData ? std::to_string(mStats.repaired) + "/" + std::to_string(mStats.pending) + "/" + std::to_string(mStats.required) : "—", NAS2D::Color::White},
		{"Crew", hasData ? std::to_string(mStats.personnelAssigned) + "/" + std::to_string(mStats.personnelCapacity) : "—", NAS2D::Color::White},
		{"Parts", hasData ? std::to_string(mStats.suppliesOnHand) + "/" + std::to_string(mStats.suppliesCapacity) : "—", NAS2D::Color::White},
		{"Waste", wasteValue, wasteColor},
	}};

	for (const auto& row : rows)
	{
		drawMetricRow(renderer, font, position, mRect.size.x, row.label, row.value, row.valueColor);
		position.y += rowHeight;
	}
}