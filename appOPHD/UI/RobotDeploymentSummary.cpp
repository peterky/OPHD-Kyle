#include "RobotDeploymentSummary.h"

#include "../RobotPool.h"
#include "../StructureManager.h"
#include "../TruckAvailability.h"
#include "../CacheImage.h"
#include "../MapObjects/RobotTypeIndex.h"

#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/Resource/Font.h>
#include <NAS2D/Resource/Image.h>

#include <vector>


namespace
{
	constexpr int HudRowHeight{20};
	constexpr int HudIconEdge{20};
	constexpr int HudIconInsetX{5};
	constexpr int HudLabelX{30};
	constexpr int HudRowTextY{4};
	constexpr int HudValuePad{10};

	constexpr int RobotPickerIconEdge{46};
	constexpr int StructurePickerIconEdge{46};
	constexpr int RobotCommandStructureIconIndex{14};


	NAS2D::Rectangle<int> iconSheetRect(const NAS2D::Image& sheet, int sheetIndex, int iconEdge)
	{
		const auto columns = sheet.size().x / iconEdge;
		return {
			{(sheetIndex % columns) * iconEdge, (sheetIndex / columns) * iconEdge},
			{iconEdge, iconEdge}
		};
	}


	NAS2D::Rectangle<float> hudIconDestination(NAS2D::Point<int> rowOrigin)
	{
		return NAS2D::Rectangle<float>{
			{static_cast<float>(rowOrigin.x + HudIconInsetX), static_cast<float>(rowOrigin.y + (HudRowHeight - HudIconEdge) / 2)},
			{static_cast<float>(HudIconEdge), static_cast<float>(HudIconEdge)}
		};
	}


	void drawHudIcon(
		NAS2D::Renderer& renderer,
		NAS2D::Point<int> rowOrigin,
		const NAS2D::Image& image,
		NAS2D::Rectangle<int> srcRect)
	{
		renderer.drawSubImageStretched(image, hudIconDestination(rowOrigin), srcRect.to<float>());
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
		renderer.drawText(font, label, rowOrigin + NAS2D::Vector{HudLabelX, HudRowTextY}, NAS2D::Color{180, 180, 180});
		const auto valueX = rowOrigin.x + panelWidth - font.width(value) - HudValuePad;
		renderer.drawText(font, value, NAS2D::Point{valueX, rowOrigin.y + HudRowTextY}, valueColor);
	}
}


RobotDeploymentSummary::RobotDeploymentSummary(const RobotPool& robotPool, const StructureManager& structureManager) :
	mRobotPool{robotPool},
	mStructureManager{structureManager},
	mRobotIcons{getImage("ui/robots.png")},
	mStructureIcons{getImage("ui/structures.png")},
	mTruckIcon{getImage("ui/interface/product_truck.png")}
{
}


void RobotDeploymentSummary::draw(NAS2D::Renderer& renderer) const
{
	const auto& font = Control::getDefaultFont();

	const auto hasCommandCenter = mRobotPool.robotControlMax() > 0;
	const auto truckStats = getTruckDeploymentStats(mStructureManager);
	const auto deployedRobots = mRobotPool.deployedRobots().size();
	const auto commandAtCapacity = hasCommandCenter && deployedRobots >= mRobotPool.robotControlMax();
	const auto commandColor = commandAtCapacity ? NAS2D::Color{255, 180, 120} : NAS2D::Color::White;

	const auto commandIconRect = iconSheetRect(mStructureIcons, RobotCommandStructureIconIndex, StructurePickerIconEdge);
	const auto diggerIconRect = iconSheetRect(mRobotIcons, 1, RobotPickerIconEdge);
	const auto dozerIconRect = iconSheetRect(mRobotIcons, 0, RobotPickerIconEdge);
	const auto minerIconRect = iconSheetRect(mRobotIcons, 2, RobotPickerIconEdge);
	const auto explorerIconRect = iconSheetRect(mRobotIcons, 3, RobotPickerIconEdge);
	const auto truckIconRect = NAS2D::Rectangle<int>{{0, 0}, mTruckIcon.size()};

	struct Row
	{
		const NAS2D::Image& image;
		NAS2D::Rectangle<int> srcRect;
		std::string label;
		std::string value;
		NAS2D::Color valueColor;
	};

	std::vector<Row> rows;
	rows.reserve(mShowExplorer ? 6 : 5);
	rows.push_back({mStructureIcons, commandIconRect, "Command", hasCommandCenter ? std::to_string(deployedRobots) + "/" + std::to_string(mRobotPool.robotControlMax()) : "—", commandColor});
	rows.push_back({mRobotIcons, diggerIconRect, "Diggers", hasCommandCenter ? std::to_string(mRobotPool.getAvailableCount(RobotTypeIndex::Digger)) + "/" + std::to_string(mRobotPool.diggers().size()) : "—", NAS2D::Color::White});
	rows.push_back({mRobotIcons, dozerIconRect, "Dozers", hasCommandCenter ? std::to_string(mRobotPool.getAvailableCount(RobotTypeIndex::Dozer)) + "/" + std::to_string(mRobotPool.dozers().size()) : "—", NAS2D::Color::White});
	rows.push_back({mRobotIcons, minerIconRect, "Miners", hasCommandCenter ? std::to_string(mRobotPool.getAvailableCount(RobotTypeIndex::Miner)) + "/" + std::to_string(mRobotPool.miners().size()) : "—", NAS2D::Color::White});

	if (mShowExplorer)
	{
		rows.push_back({mRobotIcons, explorerIconRect, "Explorers", hasCommandCenter ? std::to_string(mRobotPool.getAvailableCount(RobotTypeIndex::Explorer)) + "/" + std::to_string(mRobotPool.explorers().size()) : "—", NAS2D::Color::White});
	}

	rows.push_back({mTruckIcon, truckIconRect, "Trucks", std::to_string(truckStats.available) + "/" + std::to_string(truckStats.deployed) + "/" + std::to_string(truckStats.total), NAS2D::Color::White});

	auto position = mRect.position;
	renderer.drawText(font, "Units", position + NAS2D::Vector{HudLabelX, HudRowTextY}, NAS2D::Color{160, 160, 160});
	position.y += HudRowHeight;

	for (const auto& row : rows)
	{
		drawHudIcon(renderer, position, row.image, row.srcRect);
		drawMetricRow(renderer, font, position, mRect.size.x, row.label, row.value, row.valueColor);
		position.y += HudRowHeight;
	}
}