#pragma once

#include <libControls/Control.h>
#include <libControls/ScrollBar.h>

#include <NAS2D/Math/Point.h>
#include <NAS2D/Math/Rectangle.h>
#include <NAS2D/Signal/Delegate.h>

#include <string>
#include <vector>


namespace NAS2D
{
	class Font;
	class Image;
	enum class MouseButton;
}


class TechnologyCatalog;
class ResearchTracker;


class ResearchTechTree : public Control
{
public:
	using SelectionChangedDelegate = NAS2D::Delegate<void()>;

	ResearchTechTree(
		const NAS2D::Font& font,
		const NAS2D::Image& topicIcons,
		SelectionChangedDelegate selectionChangedHandler = {});

	void refresh(
		const TechnologyCatalog& catalog,
		const ResearchTracker& tracker,
		const std::string& categoryName,
		int standardLabsAvailable,
		int hotLabsAvailable);

	bool isItemSelected() const;
	int selectedTechId() const;
	void selectTechId(int techId);
	void clearSelected();

	void update() override;

protected:
	void draw(NAS2D::Renderer& renderer) const override;
	void onResize() override;
	void onMouseDown(NAS2D::MouseButton button, NAS2D::Point<int> position);
	void onMouseMove(NAS2D::Point<int> position, NAS2D::Vector<int> relative);
	void onMouseWheel(NAS2D::Vector<int> scrollAmount);

private:
	enum class NodeStatus
	{
		Completed,
		InProgress,
		Queued,
		Ready,
		Locked,
		External
	};

	struct Node
	{
		int techId{0};
		std::string name;
		std::string categoryName;
		NAS2D::Rectangle<int> rect{};
		NodeStatus status{NodeStatus::Locked};
		bool isExternal{false};
		int progress{0};
		int cost{0};
		int iconIndex{0};
		int labType{0};
		std::vector<int> prerequisiteIds;
	};

	struct Edge
	{
		int fromTechId{0};
		int toTechId{0};
	};

	void buildLayout(
		const TechnologyCatalog& catalog,
		const ResearchTracker& tracker,
		const std::string& categoryName);

	NodeStatus nodeStatus(
		const TechnologyCatalog& catalog,
		const ResearchTracker& tracker,
		int techId,
		int labType,
		bool isExternal) const;

	void updateScrollLimits();
	void onVerticalScroll(int newValue);
	void onHorizontalScroll(int newValue);

	void drawEdges(NAS2D::Renderer& renderer) const;
	void drawNodes(NAS2D::Renderer& renderer) const;
	void drawLegend(NAS2D::Renderer& renderer) const;

	const Node* nodeForTechId(int techId) const;
	NAS2D::Point<int> contentToScreen(NAS2D::Point<int> contentPosition) const;
	NAS2D::Point<int> screenToContent(NAS2D::Point<int> screenPosition) const;
	NAS2D::Point<int> nodeAnchor(const Node& node, bool isSource) const;

private:
	const NAS2D::Font& mFont;
	const NAS2D::Image& mTopicIcons;

	SelectionChangedDelegate mSelectionChangedHandler;

	const TechnologyCatalog* mCatalog{nullptr};
	const ResearchTracker* mTracker{nullptr};
	std::string mCategoryName;

	int mStandardLabsAvailable{0};
	int mHotLabsAvailable{0};

	std::vector<Node> mNodes;
	std::vector<Edge> mEdges;

	NAS2D::Vector<int> mContentSize{0, 0};
	NAS2D::Rectangle<int> mViewport{};
	ScrollBar mVerticalScrollBar{ScrollBar::ScrollBarType::Vertical, 20, {this, &ResearchTechTree::onVerticalScroll}};
	ScrollBar mHorizontalScrollBar{ScrollBar::ScrollBarType::Horizontal, 20, {this, &ResearchTechTree::onHorizontalScroll}};

	int mScrollOffsetX{0};
	int mScrollOffsetY{0};
	int mHighlightTechId{-1};
	int mSelectedTechId{-1};
};