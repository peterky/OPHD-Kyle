#include "ResearchTechTree.h"

#include "../../Constants/UiConstants.h"

#include <libOPHD/Technology/Technology.h>
#include <libOPHD/Technology/TechnologyCatalog.h>
#include <libOPHD/Technology/ResearchTracker.h>

#include <NAS2D/EventHandler.h>
#include <NAS2D/EnumMouseButton.h>
#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/Resource/Font.h>
#include <NAS2D/Resource/Image.h>
#include <NAS2D/Utility.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>


namespace
{
	constexpr NAS2D::Vector<int> NodeIconSize{32, 32};
	constexpr NAS2D::Vector<int> NodeSize{104, 58};
	constexpr int TierSpacingX = 118;
	constexpr int RowSpacingY = 68;
	constexpr int ContentPadding = 16;
	constexpr int LegendHeight = 24;

	constexpr NAS2D::Vector<int> TopicIconSheetSize{128, 128};


	NAS2D::Point<int> topicIconCoords(int iconIndex, int textureWidth)
	{
		const auto columns = std::max(1, textureWidth / TopicIconSheetSize.x);
		return {
			(iconIndex % columns) * TopicIconSheetSize.x,
			(iconIndex / columns) * TopicIconSheetSize.y};
	}


	std::string truncateName(const std::string& name, std::size_t maxLength = 14)
	{
		if (name.size() <= maxLength) { return name; }
		return name.substr(0, maxLength - 1) + "...";
	}


	std::string categoryNameForTech(const TechnologyCatalog& catalog, int techId)
	{
		for (const auto& category : catalog.categories())
		{
			for (const auto& technology : category.technologies)
			{
				if (technology.id == techId) { return category.name; }
			}
		}

		return "Other";
	}


	struct LayoutNode
	{
		int techId{0};
		std::string name;
		std::string categoryName;
		int tier{0};
		int row{0};
		bool isExternal{false};
		int progress{0};
		int cost{0};
		int iconIndex{0};
		int labType{0};
		std::vector<int> prerequisiteIds;
	};


	int computeTier(
		int techId,
		const std::set<int>& categoryTechIds,
		const TechnologyCatalog& catalog,
		std::map<int, int>& tierCache,
		std::set<int>& visiting)
	{
		const auto cachedTier = tierCache.find(techId);
		if (cachedTier != tierCache.end()) { return cachedTier->second; }

		if (visiting.find(techId) != visiting.end())
		{
			return 0;
		}

		const auto* technology = catalog.findTechnologyFromId(techId);
		if (!technology)
		{
			tierCache.insert_or_assign(techId, 0);
			return 0;
		}

		visiting.insert(techId);
		int maxPrerequisiteTier = -1;

		for (const auto prerequisiteId : technology->requiredTechnologies)
		{
			if (categoryTechIds.find(prerequisiteId) != categoryTechIds.end())
			{
				maxPrerequisiteTier = std::max(maxPrerequisiteTier, computeTier(prerequisiteId, categoryTechIds, catalog, tierCache, visiting));
			}
		}

		visiting.erase(techId);

		const int tier = maxPrerequisiteTier + 1;
		tierCache.insert_or_assign(techId, tier);
		return tier;
	}


	void assignRowsForTier(std::vector<LayoutNode*>& tierNodes, const std::map<int, int>& rowByTechId)
	{
		std::vector<std::pair<LayoutNode*, float>> desiredRows;
		desiredRows.reserve(tierNodes.size());

		for (auto* node : tierNodes)
		{
			float desiredRow = 0.0f;
			int parentCount = 0;

			for (const auto prerequisiteId : node->prerequisiteIds)
			{
				const auto parentRow = rowByTechId.find(prerequisiteId);
				if (parentRow != rowByTechId.end())
				{
					desiredRow += static_cast<float>(parentRow->second);
					++parentCount;
				}
			}

			if (parentCount > 0)
			{
				desiredRow /= static_cast<float>(parentCount);
			}
			else
			{
				desiredRow = static_cast<float>(desiredRows.size());
			}

			desiredRows.emplace_back(node, desiredRow);
		}

		std::sort(desiredRows.begin(), desiredRows.end(), [](const auto& lhs, const auto& rhs)
		{
			if (lhs.second == rhs.second) { return lhs.first->name < rhs.first->name; }
			return lhs.second < rhs.second;
		});

		int nextRow = 0;
		for (auto& [node, desiredRow] : desiredRows)
		{
			node->row = std::max(nextRow, static_cast<int>(std::round(desiredRow)));
			nextRow = node->row + 1;
		}
	}
}


ResearchTechTree::ResearchTechTree(
	const NAS2D::Font& font,
	const NAS2D::Image& topicIcons,
	SelectionChangedDelegate selectionChangedHandler) :
	mFont{font},
	mTopicIcons{topicIcons},
	mSelectionChangedHandler{selectionChangedHandler}
{
	auto& eventHandler = NAS2D::Utility<NAS2D::EventHandler>::get();
	eventHandler.mouseButtonDown().connect({this, &ResearchTechTree::onMouseDown});
	eventHandler.mouseMotion().connect({this, &ResearchTechTree::onMouseMove});
	eventHandler.mouseWheel().connect({this, &ResearchTechTree::onMouseWheel});
}


void ResearchTechTree::refresh(
	const TechnologyCatalog& catalog,
	const ResearchTracker& tracker,
	const std::string& categoryName,
	int standardLabsAvailable,
	int hotLabsAvailable)
{
	mCatalog = &catalog;
	mTracker = &tracker;
	mCategoryName = categoryName;
	mStandardLabsAvailable = standardLabsAvailable;
	mHotLabsAvailable = hotLabsAvailable;

	mScrollOffsetX = 0;
	mScrollOffsetY = 0;

	buildLayout(catalog, tracker, categoryName);
	updateScrollLimits();
}


bool ResearchTechTree::isItemSelected() const
{
	return mSelectedTechId >= 0;
}


int ResearchTechTree::selectedTechId() const
{
	return mSelectedTechId;
}


void ResearchTechTree::selectTechId(int techId)
{
	mSelectedTechId = techId;
	if (mSelectionChangedHandler) { mSelectionChangedHandler(); }
}


void ResearchTechTree::clearSelected()
{
	mSelectedTechId = -1;
	mHighlightTechId = -1;
}


ResearchTechTree::NodeStatus ResearchTechTree::nodeStatus(
	const TechnologyCatalog& catalog,
	const ResearchTracker& tracker,
	int techId,
	int labType,
	bool isExternal) const
{
	if (isExternal)
	{
		return tracker.isCompleted(techId) ? NodeStatus::Completed : NodeStatus::External;
	}

	if (tracker.isCompleted(techId)) { return NodeStatus::Completed; }
	if (tracker.isBeingResearched(techId)) { return NodeStatus::InProgress; }
	if (tracker.isQueued(techId)) { return NodeStatus::Queued; }

	if (!tracker.prerequisitesMet(techId, catalog)) { return NodeStatus::Locked; }

	const int labsAvailable = (labType == 0) ? mStandardLabsAvailable : mHotLabsAvailable;
	if (labsAvailable > 0)
	{
		return NodeStatus::Ready;
	}

	return NodeStatus::Locked;
}


void ResearchTechTree::buildLayout(
	const TechnologyCatalog& catalog,
	const ResearchTracker& tracker,
	const std::string& categoryName)
{
	mNodes.clear();
	mEdges.clear();

	const auto& technologies = catalog.technologiesInCategory(categoryName);
	std::set<int> categoryTechIds;
	for (const auto& technology : technologies)
	{
		categoryTechIds.insert(technology.id);
	}

	std::set<int> externalPrerequisiteIds;
	for (const auto& technology : technologies)
	{
		for (const auto prerequisiteId : technology.requiredTechnologies)
		{
			if (categoryTechIds.find(prerequisiteId) == categoryTechIds.end())
			{
				externalPrerequisiteIds.insert(prerequisiteId);
			}
		}
	}

	std::map<int, int> tierByTechId;
	std::set<int> visitingTiers;
	for (const auto& technology : technologies)
	{
		computeTier(technology.id, categoryTechIds, catalog, tierByTechId, visitingTiers);
	}

	const int tierOffset = externalPrerequisiteIds.empty() ? 0 : 1;
	std::vector<LayoutNode> layoutNodes;

	for (const auto externalTechId : externalPrerequisiteIds)
	{
		const auto* technology = catalog.findTechnologyFromId(externalTechId);
		if (!technology) { continue; }

		layoutNodes.push_back(LayoutNode{
			technology->id,
			technology->name,
			categoryNameForTech(catalog, technology->id),
			0,
			0,
			true,
			tracker.isBeingResearched(technology->id) ? tracker.researchProgress(technology->id).progress : 0,
			technology->cost,
			technology->iconIndex,
			technology->labType,
			technology->requiredTechnologies
		});
	}

	for (const auto& technology : technologies)
	{
		const auto tierIt = tierByTechId.find(technology.id);
		const int tier = tierIt != tierByTechId.end() ? tierIt->second : 0;

		layoutNodes.push_back(LayoutNode{
			technology.id,
			technology.name,
			categoryName,
			tier + tierOffset,
			0,
			false,
			tracker.isBeingResearched(technology.id) ? tracker.researchProgress(technology.id).progress : 0,
			technology.cost,
			technology.iconIndex,
			technology.labType,
			technology.requiredTechnologies
		});
	}

	std::map<int, int> rowByTechId;
	std::map<int, std::vector<LayoutNode*>> nodesByTier;

	for (auto& layoutNode : layoutNodes)
	{
		nodesByTier[layoutNode.tier].push_back(&layoutNode);
	}

	for (auto& [tier, tierNodes] : nodesByTier)
	{
		if (tier == 0 && !externalPrerequisiteIds.empty())
		{
			std::sort(tierNodes.begin(), tierNodes.end(), [](const LayoutNode* lhs, const LayoutNode* rhs)
			{
				return lhs->name < rhs->name;
			});

			for (std::size_t i = 0; i < tierNodes.size(); ++i)
			{
				tierNodes[i]->row = static_cast<int>(i);
				rowByTechId[tierNodes[i]->techId] = tierNodes[i]->row;
			}
		}
		else
		{
			assignRowsForTier(tierNodes, rowByTechId);
			for (const auto* tierNode : tierNodes)
			{
				rowByTechId[tierNode->techId] = tierNode->row;
			}
		}
	}

	for (const auto& layoutNode : layoutNodes)
	{
		const auto position = NAS2D::Point<int>{
			ContentPadding + layoutNode.tier * TierSpacingX,
			ContentPadding + layoutNode.row * RowSpacingY};

		mNodes.push_back(Node{
			layoutNode.techId,
			layoutNode.name,
			layoutNode.categoryName,
			{position, NodeSize},
			nodeStatus(catalog, tracker, layoutNode.techId, layoutNode.labType, layoutNode.isExternal),
			layoutNode.isExternal,
			layoutNode.progress,
			layoutNode.cost,
			layoutNode.iconIndex,
			layoutNode.labType,
			layoutNode.prerequisiteIds
		});
	}

	for (const auto& technology : technologies)
	{
		for (const auto prerequisiteId : technology.requiredTechnologies)
		{
			mEdges.push_back(Edge{prerequisiteId, technology.id});
		}
	}

	int maxX = 0;
	int maxY = 0;
	for (const auto& node : mNodes)
	{
		maxX = std::max(maxX, node.rect.endPoint().x);
		maxY = std::max(maxY, node.rect.endPoint().y);
	}

	mContentSize = {maxX + ContentPadding, maxY + ContentPadding};
}


void ResearchTechTree::updateScrollLimits()
{
	constexpr int scrollBarThickness = 16;
	mViewport = {
		area().position,
		{area().size.x - scrollBarThickness, area().size.y - scrollBarThickness - LegendHeight}};

	mVerticalScrollBar.position({area().endPoint().x - scrollBarThickness, area().position.y});
	mVerticalScrollBar.size({scrollBarThickness, mViewport.size.y});
	mHorizontalScrollBar.position({area().position.x, area().endPoint().y - scrollBarThickness - LegendHeight});
	mHorizontalScrollBar.size({mViewport.size.x, scrollBarThickness});

	const auto maxScrollY = std::max(0, mContentSize.y - mViewport.size.y);
	const auto maxScrollX = std::max(0, mContentSize.x - mViewport.size.x);

	mVerticalScrollBar.max(maxScrollY);
	mHorizontalScrollBar.max(maxScrollX);
	mVerticalScrollBar.visible(maxScrollY > 0);
	mHorizontalScrollBar.visible(maxScrollX > 0);

	mScrollOffsetY = std::clamp(mScrollOffsetY, 0, maxScrollY);
	mScrollOffsetX = std::clamp(mScrollOffsetX, 0, maxScrollX);
	mVerticalScrollBar.value(mScrollOffsetY);
	mHorizontalScrollBar.value(mScrollOffsetX);
}


void ResearchTechTree::onVerticalScroll(int newValue)
{
	mScrollOffsetY = newValue;
}


void ResearchTechTree::onHorizontalScroll(int newValue)
{
	mScrollOffsetX = newValue;
}


void ResearchTechTree::onResize()
{
	updateScrollLimits();
}


void ResearchTechTree::update()
{
	if (!visible()) { return; }

	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	draw(renderer);
	mVerticalScrollBar.update();
	mHorizontalScrollBar.update();
}


NAS2D::Point<int> ResearchTechTree::contentToScreen(NAS2D::Point<int> contentPosition) const
{
	return {
		area().position.x + contentPosition.x - mScrollOffsetX,
		area().position.y + contentPosition.y - mScrollOffsetY};
}


NAS2D::Point<int> ResearchTechTree::screenToContent(NAS2D::Point<int> screenPosition) const
{
	return {
		screenPosition.x - area().position.x + mScrollOffsetX,
		screenPosition.y - area().position.y + mScrollOffsetY};
}


const ResearchTechTree::Node* ResearchTechTree::nodeForTechId(int techId) const
{
	for (const auto& node : mNodes)
	{
		if (node.techId == techId) { return &node; }
	}

	return nullptr;
}


NAS2D::Point<int> ResearchTechTree::nodeAnchor(const Node& node, bool isSource) const
{
	const auto screenPosition = contentToScreen(node.rect.position);
	const auto centerY = screenPosition.y + node.rect.size.y / 2;

	if (isSource)
	{
		return {screenPosition.x + node.rect.size.x, centerY};
	}

	return {screenPosition.x, centerY};
}


void ResearchTechTree::drawEdges(NAS2D::Renderer& renderer) const
{
	for (const auto& edge : mEdges)
	{
		const auto* fromNode = nodeForTechId(edge.fromTechId);
		const auto* toNode = nodeForTechId(edge.toTechId);
		if (!fromNode || !toNode) { continue; }

		const auto startPoint = nodeAnchor(*fromNode, true);
		const auto endPoint = nodeAnchor(*toNode, false);
		const auto midX = (startPoint.x + endPoint.x) / 2;

		auto lineColor = NAS2D::Color{60, 60, 60};
		if (fromNode->status == NodeStatus::Completed)
		{
			lineColor = constants::PrimaryColor;
		}
		else if (fromNode->status == NodeStatus::InProgress)
		{
			lineColor = constants::SecondaryColor;
		}

		renderer.drawLine(startPoint.to<float>(), NAS2D::Point<float>{static_cast<float>(midX), static_cast<float>(startPoint.y)}, lineColor);
		renderer.drawLine(
			NAS2D::Point<float>{static_cast<float>(midX), static_cast<float>(startPoint.y)},
			NAS2D::Point<float>{static_cast<float>(midX), static_cast<float>(endPoint.y)},
			lineColor);
		renderer.drawLine(NAS2D::Point<float>{static_cast<float>(midX), static_cast<float>(endPoint.y)}, endPoint.to<float>(), lineColor);
	}
}


void ResearchTechTree::drawNodes(NAS2D::Renderer& renderer) const
{
	for (const auto& node : mNodes)
	{
		const auto drawPosition = contentToScreen(node.rect.position);
		const auto drawRect = NAS2D::Rectangle<int>{drawPosition, node.rect.size};

		if (drawRect.endPoint().x < mViewport.position.x ||
			drawRect.endPoint().y < mViewport.position.y ||
			drawRect.position.x > mViewport.endPoint().x ||
			drawRect.position.y > mViewport.endPoint().y)
		{
			continue;
		}

		NAS2D::Color borderColor{90, 90, 90};
		NAS2D::Color fillColor{20, 20, 20, 220};

		switch (node.status)
		{
		case NodeStatus::Completed:
			borderColor = constants::PrimaryColor;
			fillColor = NAS2D::Color{0, 60, 0, 220};
			break;
		case NodeStatus::InProgress:
			borderColor = constants::SecondaryColor;
			fillColor = NAS2D::Color{60, 60, 0, 220};
			break;
		case NodeStatus::Queued:
			borderColor = NAS2D::Color{180, 120, 40};
			fillColor = NAS2D::Color{45, 30, 0, 220};
			break;
		case NodeStatus::Ready:
			borderColor = constants::HighlightColor;
			fillColor = NAS2D::Color{0, 45, 45, 220};
			break;
		case NodeStatus::External:
			borderColor = NAS2D::Color{120, 120, 180};
			fillColor = NAS2D::Color{30, 30, 45, 200};
			break;
		case NodeStatus::Locked:
			borderColor = NAS2D::Color{70, 70, 70};
			fillColor = NAS2D::Color{15, 15, 15, 200};
			break;
		}

		if (node.techId == mSelectedTechId)
		{
			renderer.drawBoxFilled(drawRect, constants::PrimaryColorVariant);
		}
		else if (node.techId == mHighlightTechId)
		{
			renderer.drawBoxFilled(drawRect, constants::HighlightColor);
		}
		else
		{
			renderer.drawBoxFilled(drawRect, fillColor);
		}

		renderer.drawBox(drawRect, borderColor);

		const auto iconPosition = drawPosition + NAS2D::Vector<int>{(drawRect.size.x - NodeIconSize.x) / 2, 3};
		const auto iconUV = topicIconCoords(node.iconIndex, mTopicIcons.size().x);
		renderer.drawSubImageRepeated(
			mTopicIcons,
			{iconPosition.to<float>(), NodeIconSize.to<float>()},
			{iconUV.to<float>(), TopicIconSheetSize.to<float>()});

		if (node.status == NodeStatus::InProgress && node.cost > 0)
		{
			const auto barRect = NAS2D::Rectangle<int>{
				drawPosition + NAS2D::Vector<int>{6, NodeIconSize.y + 4},
				{drawRect.size.x - 12, 4}};
			renderer.drawBoxFilled(barRect, NAS2D::Color{40, 40, 40});
			const auto filledWidth = (barRect.size.x * node.progress) / node.cost;
			renderer.drawBoxFilled(
				{barRect.position.to<float>(), NAS2D::Vector<float>{static_cast<float>(filledWidth), static_cast<float>(barRect.size.y)}},
				constants::SecondaryColor);
		}

		const auto labBadge = (node.labType == 0) ? "UG" : "HOT";
		const auto labBadgeColor = (node.labType == 0) ? NAS2D::Color{120, 180, 255} : NAS2D::Color{255, 140, 80};
		renderer.drawText(mFont, labBadge, (drawPosition + NAS2D::Vector<int>{4, 1}).to<float>(), labBadgeColor);

		const auto namePosition = drawPosition + NAS2D::Vector<int>{4, drawRect.size.y - mFont.height() - 3};
		auto textColor = (node.status == NodeStatus::Locked) ? NAS2D::Color{140, 140, 140} : NAS2D::Color::White;
		renderer.drawText(mFont, truncateName(node.name), namePosition.to<float>(), textColor);

		if (node.isExternal)
		{
			renderer.drawText(
				mFont,
				truncateName(node.categoryName, 10),
				(drawPosition + NAS2D::Vector<int>{24, 1}).to<float>(),
				NAS2D::Color{160, 160, 220});
		}
	}
}


void ResearchTechTree::drawLegend(NAS2D::Renderer& renderer) const
{
	const auto legendY = area().endPoint().y - mFont.height() - 6;
	const auto legendX = area().position.x + 8;

	renderer.drawText(mFont, "Complete", NAS2D::Point<float>{static_cast<float>(legendX), static_cast<float>(legendY)}, constants::PrimaryColor);
	renderer.drawText(mFont, "In Progress", NAS2D::Point<float>{static_cast<float>(legendX + 78), static_cast<float>(legendY)}, constants::SecondaryColor);
	renderer.drawText(mFont, "Queued", NAS2D::Point<float>{static_cast<float>(legendX + 168), static_cast<float>(legendY)}, NAS2D::Color{180, 120, 40});
	renderer.drawText(mFont, "Ready", NAS2D::Point<float>{static_cast<float>(legendX + 238), static_cast<float>(legendY)}, constants::HighlightColor);
	renderer.drawText(mFont, "UG/HOT", NAS2D::Point<float>{static_cast<float>(legendX + 298), static_cast<float>(legendY)}, NAS2D::Color{120, 180, 255});
}


void ResearchTechTree::draw(NAS2D::Renderer& renderer) const
{
	renderer.drawBoxFilled(area(), NAS2D::Color{0, 0, 0, 200});
	renderer.drawBox(mViewport, constants::PrimaryColor);

	renderer.clipRect(mViewport.to<float>());
	drawEdges(renderer);
	drawNodes(renderer);
	renderer.clipRectClear();

	drawLegend(renderer);
}


void ResearchTechTree::onMouseDown(NAS2D::MouseButton button, NAS2D::Point<int> position)
{
	if (!visible() || button != NAS2D::MouseButton::Left || !mViewport.contains(position)) { return; }

	const auto contentPosition = screenToContent(position);

	for (const auto& node : mNodes)
	{
		if (node.rect.contains(contentPosition))
		{
			mSelectedTechId = node.techId;
			if (mSelectionChangedHandler) { mSelectionChangedHandler(); }
			return;
		}
	}
}


void ResearchTechTree::onMouseMove(NAS2D::Point<int> position, NAS2D::Vector<int> /*relative*/)
{
	if (!visible() || !mViewport.contains(position))
	{
		mHighlightTechId = -1;
		return;
	}

	const auto contentPosition = screenToContent(position);
	mHighlightTechId = -1;

	for (const auto& node : mNodes)
	{
		if (node.rect.contains(contentPosition))
		{
			mHighlightTechId = node.techId;
			return;
		}
	}
}


void ResearchTechTree::onMouseWheel(NAS2D::Vector<int> scrollAmount)
{
	if (!visible()) { return; }

	if (scrollAmount.y != 0)
	{
		mVerticalScrollBar.changeValue(-scrollAmount.y * 20);
		mScrollOffsetY = mVerticalScrollBar.value();
	}
	else if (scrollAmount.x != 0)
	{
		mHorizontalScrollBar.changeValue(-scrollAmount.x * 20);
		mScrollOffsetX = mHorizontalScrollBar.value();
	}
}