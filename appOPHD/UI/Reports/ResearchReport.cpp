#include "ResearchReport.h"

#include "../../Constants/Strings.h"
#include "../../Constants/UiConstants.h"
#include "../../CacheImage.h"
#include "../../CacheFont.h"
#include "../../StructureManager.h"
#include "../../MapObjects/StructureState.h"
#include "../../MapObjects/Structures/HotLaboratory.h"
#include "../../MapObjects/Structures/Laboratory.h"

#include <libOPHD/Technology/Technology.h>
#include <libOPHD/Technology/TechnologyCatalog.h>
#include <libOPHD/Technology/ResearchTracker.h>

#include <NAS2D/Utility.h>
#include <NAS2D/EventHandler.h>
#include <NAS2D/EnumMouseButton.h>
#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/Resource/Font.h>
#include <NAS2D/Resource/Image.h>
#include <NAS2D/Math/Vector.h>

#include <sstream>
#include <string>
#include <vector>
#include <algorithm>


namespace
{
	constexpr NAS2D::Vector<int> LabTypeIconSize{32, 32};
	constexpr NAS2D::Vector<int> CategoryIconSize{64, 64};
	constexpr NAS2D::Vector<int> TopicIconSize{128, 128};
	constexpr auto MarginSize = 10;

	constexpr NAS2D::Rectangle<int> HotLabIconRect = {{32, 224}, LabTypeIconSize};
	constexpr NAS2D::Rectangle<int> StandardLabIconRect = {{0, 224}, LabTypeIconSize};

	constexpr NAS2D::Vector<int> CategorySelectorPadding{2, 2};
	constexpr NAS2D::Vector<int> SectionPadding{10, 10};
	constexpr NAS2D::Vector<int> ResearchControlButtonSize{140, 26};

	std::string truncateText(const NAS2D::Font& font, const std::string& text, int maxWidth)
	{
		if (font.size(text).x <= maxWidth) { return text; }

		std::string truncated = text;
		while (!truncated.empty() && font.size(truncated + "...").x > maxWidth)
		{
			truncated.pop_back();
		}

		return truncated + "...";
	}


	void drawDetailsHeaderSeparator(NAS2D::Renderer& renderer, const NAS2D::Rectangle<int>& area)
	{
		const NAS2D::Point<int> lineStartPoint{area.crossYPoint() + NAS2D::Vector<int>{0, SectionPadding.y}};
		const NAS2D::Point<int> lineEndPoint{area.endPoint() + NAS2D::Vector<int>{0, SectionPadding.y}};
		renderer.drawLine(lineStartPoint, lineEndPoint, constants::PrimaryTextColor);
	}

	NAS2D::Rectangle<int> getCategorySlice(const int imageWidth, const int iconIndex)
	{
		const int columns = imageWidth / CategoryIconSize.x;
		const NAS2D::Point<int> sliceStartPosition
		{
			(iconIndex % columns) * CategoryIconSize.x,
			(iconIndex / columns) * CategoryIconSize.y
		};

		return {sliceStartPosition, CategoryIconSize};
	}

	NAS2D::Point<int> getIconTextureCoords(const Technology& tech, const int textureWidth)
	{
		const auto columns = textureWidth / TopicIconSize.x;

		return {
			(tech.iconIndex % columns) * TopicIconSize.x,
			(tech.iconIndex / columns) * TopicIconSize.y};
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

		return "Unknown";
	}

	std::string unlockDescription(const Technology::Unlock& unlock)
	{
		switch (unlock.unlocks)
		{
		case Technology::Unlock::Unlocks::Structure:
			return "Structure: " + unlock.value;
		case Technology::Unlock::Unlocks::Robot:
			return "Robot: " + unlock.value;
		case Technology::Unlock::Unlocks::Vehicle:
			return "Vehicle: " + unlock.value;
		case Technology::Unlock::Unlocks::Satellite:
			return "Satellite: " + unlock.value;
		case Technology::Unlock::Unlocks::DisasterPrediction:
			return "Disaster prediction: " + unlock.value;
		}

		return unlock.value;
	}
}


ResearchReport::ResearchReport(const StructureManager& structureManager, TakeMeThereDelegate takeMeThereHandler) :
	mStructureManager{structureManager},
	mTakeMeThereHandler{takeMeThereHandler},
	fontMedium{getFontMedium()},
	fontMediumBold{getFontMediumBold()},
	fontBigBold{getFontHugeBold()},
	imageLab{getImage("ui/interface/lab_ug.png")},
	imageUiIcons{getImage("ui/icons.png")},
	imageCategoryIcons{getImage("categoryicons.png")},
	imageTopicIcons{getImage("topicicons.png")},
	mTechTree{getFontMedium(), imageTopicIcons, {this, &ResearchReport::handleTopicChanged}},
	btnStartResearch{constants::ResearchReportStartResearch, ResearchControlButtonSize, {this, &ResearchReport::onStartResearch}},
	lblResearchProgress{},
	txtTopicDescription{getFontMedium(), constants::PrimaryTextColor}
{
	auto& eventHandler = NAS2D::Utility<NAS2D::EventHandler>::get();
	eventHandler.mouseButtonDown().connect({this, &ResearchReport::onMouseDown});
	eventHandler.mouseMotion().connect({this, &ResearchReport::onMouseMove});

	add(mTechTree, {});

	add(btnStartResearch, {});
	add(lblResearchProgress, {});
	btnStartResearch.enabled(false);

	add(txtTopicDescription, {});
}


ResearchReport::~ResearchReport()
{
	auto& eventHandler = NAS2D::Utility<NAS2D::EventHandler>::get();
	eventHandler.mouseButtonDown().disconnect({this, &ResearchReport::onMouseDown});
	eventHandler.mouseMotion().disconnect({this, &ResearchReport::onMouseMove});
}


bool ResearchReport::canView(const Structure& structure)
{
	return dynamic_cast<const ResearchFacility*>(&structure);
}


void ResearchReport::selectStructure(Structure&)
{
}


void ResearchReport::clearSelected()
{
	resetCategorySelection();
	resetResearchDetails();
	refreshTechTree();
}


void ResearchReport::fillLists()
{
	resetCategorySelection();
	resetResearchDetails();
	refreshTechTree();
}


void ResearchReport::refresh()
{
	if (mCategoryPanels.empty()) { return; }

	checkForLabAvailability();

	adjustCategoryIconSpacing();

	resetCategorySelection();
	resetResearchDetails();
	refreshTechTree();

	setSectionRects();
	setIconPositions();
	layoutContentAreas();

	mTechTree.area(mResearchTopicArea);

	const auto researchControlsY = area().endPoint().y - ResearchControlButtonSize.y - SectionPadding.y;
	btnStartResearch.position({mTopicDetailsHeaderArea.startPoint().x, researchControlsY});
	lblResearchProgress.position({btnStartResearch.area().endPoint().x + SectionPadding.x, researchControlsY + 4});

	txtTopicDescription.text("");
	txtTopicDescription.area(mTopicDetailsArea);

	updateResearchControls();
}


void ResearchReport::update()
{
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	draw(renderer);
	ControlContainer::update();
}


void ResearchReport::injectTechReferences(TechnologyCatalog& catalog, ResearchTracker& tracker)
{
	mTechCatalog = &catalog;
	mResearchTracker = &tracker;

	processCategories();
}


void ResearchReport::onResize()
{
	refresh();
}


void ResearchReport::onMouseMove(NAS2D::Point<int> position, NAS2D::Vector<int> /*relative*/)
{
	for (const auto& panel : mCategoryPanels)
	{
		if (panel.rect.contains(position))
		{
			mMouseHighlightPanel = &panel;
			return;
		}
	}
	mMouseHighlightPanel = nullptr;
}


void ResearchReport::onMouseDown(NAS2D::MouseButton button, NAS2D::Point<int> position)
{
	if (!visible() ||
		!area().contains(position) ||
		button != NAS2D::MouseButton::Left)
	{
		return;
	}

	if (mCategoryIconArea.contains(position))
	{
		handleMouseDownInCategories(position);
	}
}


void ResearchReport::handleMouseDownInCategories(NAS2D::Point<int>& position)
{
	CategoryPanel* lastPanel = mSelectedCategory;
	bool panelClickedOn = false;
	for (auto& panel : mCategoryPanels)
	{
		panel.selected = false;

		if (panel.rect.contains(position))
		{
			panel.selected = true;
			mSelectedCategory = &panel;
			panelClickedOn = true;
			resetResearchDetails();
			refreshTechTree();
		}
	}

	if (!panelClickedOn && lastPanel != nullptr)
	{
		mSelectedCategory = lastPanel;
		mSelectedCategory->selected = true;
	}
}


void ResearchReport::setIconPositions()
{
	const auto startPoint{mTopicDetailsHeaderArea.startPoint()};

	mHotLabIconPosition = {startPoint + NAS2D::Vector<int>{0, fontBigBold.height() + SectionPadding.y}};
	mStdLabIconPosition = {startPoint + NAS2D::Vector<int>{(area().size.x - startPoint.x) / 2, fontBigBold.height() + SectionPadding.y}};
	mStdLabTextPosition = {mStdLabIconPosition + NAS2D::Vector<int>{LabTypeIconSize.x + SectionPadding.x, LabTypeIconSize.y / 2 - fontMedium.height() / 2}};
	mHotLabTextPosition = {mHotLabIconPosition + NAS2D::Vector<int>{LabTypeIconSize.x + SectionPadding.x, LabTypeIconSize.y / 2 - fontMedium.height() / 2}};
}


void ResearchReport::setSectionRects()
{
	mCategoryIconArea =
	{
		mCategoryPanels.begin()->rect.startPoint(),
		{
			CategoryIconSize.x,
			mCategoryPanels.rbegin()->rect.endPoint().y - mCategoryPanels.begin()->rect.startPoint().y
		}
	};

	mResearchTreeWidth = ((area().size.x / 3) * 2) - (MarginSize * 4) - CategoryIconSize.x;

	const auto detailsColumnX = SectionPadding.x * 2 + area().position.x + MarginSize * 3 + CategoryIconSize.x + mResearchTreeWidth;
	const auto detailsColumnWidth = area().size.x - mResearchTopicArea.endPoint().x - SectionPadding.x * 3;
	const auto labRowHeight = fontBigBold.height() + SectionPadding.y + LabTypeIconSize.y + SectionPadding.y;

	mTopicDetailsHeaderArea =
	{
		area().position + NAS2D::Vector<int>{detailsColumnX, SectionPadding.y},
		{detailsColumnWidth, labRowHeight}
	};

	mTopicDetailsIconPosition =
	{
		mTopicDetailsHeaderArea.position.x,
		mTopicDetailsHeaderArea.crossYPoint().y + SectionPadding.y
	};

	const auto researchControlsTop = area().endPoint().y - ResearchControlButtonSize.y - SectionPadding.y * 2;
	const NAS2D::Point<int> topicDetailStart{
		mTopicDetailsIconPosition + NAS2D::Vector<int>{0, TopicIconSize.y + SectionPadding.y}};
	mTopicDetailsArea =
	{
		topicDetailStart,
		{
			detailsColumnWidth,
			researchControlsTop - topicDetailStart.y
		}
	};
}


void ResearchReport::adjustCategoryIconSpacing()
{
	const int minimumHeight = CategoryIconSize.x * (static_cast<int>(mCategoryPanels.size()));
	const int padding = ((area().size.y - 20) - minimumHeight) / static_cast<int>(mCategoryPanels.size() - 1);

	for (size_t i = 0; i < mCategoryPanels.size(); ++i)
	{
		const NAS2D::Point<int> point
		{
			area().position.x + MarginSize,
			area().position.y + MarginSize + static_cast<int>(i) * CategoryIconSize.y + static_cast<int>(i) * padding
		};

		mCategoryPanels[i].rect = {point, CategoryIconSize};
	}
}


void ResearchReport::checkForLabAvailability()
{
	mLabsAvailable = {mStructureManager.operationalCount(StructureID::Laboratory), mStructureManager.operationalCount(StructureID::HotLaboratory)};
}


void ResearchReport::processCategories()
{
	for (const auto& category : mTechCatalog->categories())
	{
		const NAS2D::Rectangle<int> sliceRect{getCategorySlice(imageCategoryIcons.size().x, category.icon_index)};
		mCategoryPanels.emplace_back(CategoryPanel{{}, sliceRect, category.name, false});
	}

	std::sort(mCategoryPanels.begin(), mCategoryPanels.end());
}


void ResearchReport::resetCategorySelection()
{
	for (auto& panel : mCategoryPanels)
	{
		panel.selected = false;
	}

	mCategoryPanels.front().selected = true;
	mSelectedCategory = &mCategoryPanels.front();
}


void ResearchReport::refreshTechTree()
{
	if (!mTechCatalog || !mResearchTracker || !mSelectedCategory) { return; }

	mTechTree.refresh(
		*mTechCatalog,
		*mResearchTracker,
		mSelectedCategory->name,
		mLabsAvailable.standard,
		mLabsAvailable.hot);
}


void ResearchReport::resetResearchDetails()
{
	mTechTree.clearSelected();
	txtTopicDescription.text("");
	btnStartResearch.enabled(false);
	lblResearchProgress.text("");
}


void ResearchReport::handleTopicChanged()
{
	txtTopicDescription.text("");

	if (!mTechTree.isItemSelected())
	{
		updateResearchControls();
		return;
	}

	const auto& technology = mTechCatalog->technologyFromId(mTechTree.selectedTechId());
	txtTopicDescription.text(buildTopicDetailsText(technology));
	mTopicDetailsIconUV = getIconTextureCoords(technology, imageTopicIcons.size().x);
	updateResearchControls();
}


std::vector<std::string> ResearchReport::readyTechnologiesInCategory() const
{
	std::vector<std::string> readyTopics;

	if (!mTechCatalog || !mResearchTracker || !mSelectedCategory) { return readyTopics; }

	for (const auto& technology : mTechCatalog->technologiesInCategory(mSelectedCategory->name))
	{
		if (mResearchTracker->isCompleted(technology.id) ||
			mResearchTracker->isBeingResearched(technology.id) ||
			mResearchTracker->isQueued(technology.id))
		{
			continue;
		}
		if (!mResearchTracker->prerequisitesMet(technology.id, *mTechCatalog)) { continue; }

		const int labsAvailable = (technology.labType == 0) ? mLabsAvailable.standard : mLabsAvailable.hot;
		if (labsAvailable > 0)
		{
			readyTopics.push_back(technology.name);
		}
	}

	std::sort(readyTopics.begin(), readyTopics.end());
	return readyTopics;
}


std::string ResearchReport::buildTopicDetailsText(const Technology& technology) const
{
	std::ostringstream details;

	details << technology.name << "\n\n";

	if (!technology.description.empty())
	{
		details << technology.description << "\n\n";
	}

	details << "Research Cost: " << technology.cost << " points\n";
	details << "Required Facility: " << (technology.labType == 0 ? "Underground Laboratory" : "Surface Hot Laboratory") << "\n\n";

	if (mResearchTracker->isQueued(technology.id))
	{
		const auto& queue = mResearchTracker->queuedForLabType(technology.labType);
		const auto queueIt = std::find(queue.begin(), queue.end(), technology.id);
		if (queueIt != queue.end())
		{
			const auto queuePosition = static_cast<int>(queueIt - queue.begin()) + 1;
			details << "Status: Queued (position " << queuePosition << " for this lab type)\n\n";
		}
	}

	details << "Prerequisites:\n";
	if (technology.requiredTechnologies.empty())
	{
		details << "  None\n";
	}
	else
	{
		for (const auto prerequisiteId : technology.requiredTechnologies)
		{
			const auto& prerequisite = mTechCatalog->technologyFromId(prerequisiteId);
			const auto status = mResearchTracker->isCompleted(prerequisiteId) ? "Complete" : "Needed";
			details << "  [" << status << "] " << prerequisite.name;
			if (categoryNameForTech(*mTechCatalog, prerequisiteId) != mSelectedCategory->name)
			{
				details << " (" << categoryNameForTech(*mTechCatalog, prerequisiteId) << ")";
			}
			details << "\n";
		}
	}

	if (!technology.unlocks.empty())
	{
		details << "\nUnlocks:\n";
		for (const auto& unlock : technology.unlocks)
		{
			details << "  - " << unlockDescription(unlock) << "\n";
		}
	}

	const auto readyTopics = readyTechnologiesInCategory();
	if (!readyTopics.empty())
	{
		details << "\nReady in this category:\n";
		for (const auto& topicName : readyTopics)
		{
			details << "  - " << topicName << "\n";
		}
	}

	return details.str();
}


bool ResearchReport::willQueueSelectedResearch() const
{
	if (!mTechCatalog || !mResearchTracker || !mTechTree.isItemSelected())
	{
		return false;
	}

	const auto techId = mTechTree.selectedTechId();
	const auto& technology = mTechCatalog->technologyFromId(techId);
	return mResearchTracker->hasActiveResearchForLabType(technology.labType, *mTechCatalog);
}


bool ResearchReport::canStartOrQueueSelectedResearch() const
{
	if (!mTechCatalog || !mResearchTracker || !mTechTree.isItemSelected())
	{
		return false;
	}

	const auto techId = mTechTree.selectedTechId();
	const auto& technology = mTechCatalog->technologyFromId(techId);

	if (mResearchTracker->isCompleted(techId) ||
		mResearchTracker->isBeingResearched(techId) ||
		mResearchTracker->isQueued(techId) ||
		!mResearchTracker->prerequisitesMet(techId, *mTechCatalog))
	{
		return false;
	}

	const int labsAvailable = (technology.labType == 0) ? mLabsAvailable.standard : mLabsAvailable.hot;
	return labsAvailable > 0;
}


std::string ResearchReport::activeResearchLine() const
{
	if (!mTechCatalog || !mResearchTracker) { return {}; }

	std::ostringstream summary;

	for (const auto& [techId, progress] : mResearchTracker->currentResearch())
	{
		const auto& technology = mTechCatalog->technologyFromId(techId);
		if (!summary.str().empty()) { summary << "   "; }
		summary << technology.name << " (" << (technology.labType == 0 ? "UG Lab" : "Hot Lab") << ": "
			<< progress.progress << "/" << technology.cost << ")";
	}

	return summary.str();
}


std::string ResearchReport::queuedResearchLine() const
{
	if (!mTechCatalog || !mResearchTracker) { return {}; }

	std::ostringstream summary;

	for (const auto& [labType, queue] : mResearchTracker->researchQueue())
	{
		if (queue.empty()) { continue; }

		const auto& nextQueued = mTechCatalog->technologyFromId(queue.front());
		if (!summary.str().empty()) { summary << "   "; }
		summary << (labType == 0 ? "UG" : "Hot") << " queue: " << nextQueued.name;
		if (queue.size() > 1)
		{
			summary << " (+" << (queue.size() - 1) << ")";
		}
	}

	return summary.str();
}


void ResearchReport::layoutContentAreas()
{
	const auto contentLeft = area().position.x + MarginSize * 3 + CategoryIconSize.x;
	const int lineHeight = fontMedium.height() + 4;

	mCategoryHeaderTextPosition = {contentLeft, area().position.y + SectionPadding.y};

	auto statusY = mCategoryHeaderTextPosition.y + fontBigBold.height() + SectionPadding.y;
	mActiveResearchBannerPosition = {contentLeft, statusY};

	if (!activeResearchLine().empty())
	{
		statusY += lineHeight;
	}

	mQueuedResearchBannerPosition = {contentLeft, statusY};
	if (!queuedResearchLine().empty())
	{
		statusY += lineHeight;
	}

	mRecommendedTopicsPosition = {contentLeft, statusY};
	if (!readyTechnologiesInCategory().empty())
	{
		statusY += lineHeight;
	}

	const auto treeTop = statusY + SectionPadding.y;
	mResearchTopicArea =
	{
		{contentLeft, treeTop},
		{
			mResearchTreeWidth,
			area().size.y - treeTop + area().position.y - MarginSize * 2
		}
	};
}


void ResearchReport::updateResearchControls()
{
	const bool canStartOrQueue = canStartOrQueueSelectedResearch();
	btnStartResearch.enabled(canStartOrQueue);
	btnStartResearch.text(willQueueSelectedResearch() ?
		constants::ResearchReportQueueResearch :
		constants::ResearchReportStartResearch);

	if (!mTechTree.isItemSelected() || !mTechCatalog || !mResearchTracker)
	{
		lblResearchProgress.text("");
		return;
	}

	const auto techId = mTechTree.selectedTechId();
	const auto& technology = mTechCatalog->technologyFromId(techId);

	if (mResearchTracker->isCompleted(techId))
	{
		lblResearchProgress.text("Research complete");
	}
	else if (mResearchTracker->isBeingResearched(techId))
	{
		const auto& progress = mResearchTracker->researchProgress(techId);
		lblResearchProgress.text("In progress: " + std::to_string(progress.progress) + " / " + std::to_string(technology.cost) +
			" at " + (technology.labType == 0 ? "Underground Lab" : "Hot Lab"));
	}
	else if (mResearchTracker->isQueued(techId))
	{
		const auto& queue = mResearchTracker->queuedForLabType(technology.labType);
		const auto queueIt = std::find(queue.begin(), queue.end(), techId);
		if (queueIt != queue.end())
		{
			const auto queuePosition = static_cast<int>(queueIt - queue.begin()) + 1;
			lblResearchProgress.text("Queued (position " + std::to_string(queuePosition) + ")");
		}
		else
		{
			lblResearchProgress.text("Queued");
		}
	}
	else if (!mResearchTracker->prerequisitesMet(techId, *mTechCatalog))
	{
		lblResearchProgress.text("Prerequisites not met");
	}
	else
	{
		const int labsAvailable = (technology.labType == 0) ? mLabsAvailable.standard : mLabsAvailable.hot;
		if (labsAvailable == 0)
		{
			lblResearchProgress.text("No matching laboratories available");
		}
		else if (mResearchTracker->hasActiveResearchForLabType(technology.labType, *mTechCatalog))
		{
			lblResearchProgress.text("Will queue for " + std::string(technology.labType == 0 ? "Underground Lab" : "Hot Lab"));
		}
		else
		{
			lblResearchProgress.text("Cost: " + std::to_string(technology.cost) + " research points");
		}
	}
}


void ResearchReport::onStartResearch()
{
	if (!canStartOrQueueSelectedResearch()) { return; }

	const auto techId = mTechTree.selectedTechId();
	const auto& technology = mTechCatalog->technologyFromId(techId);
	mResearchTracker->startOrQueueResearch(techId, technology.labType, *mTechCatalog);
	refreshTechTree();
	mTechTree.selectTechId(techId);
	handleTopicChanged();
}


void ResearchReport::drawCategories(NAS2D::Renderer& renderer) const
{
	for (const auto& panel : mCategoryPanels)
	{
		const auto panelRect = NAS2D::Rectangle<int>::Create(
			panel.rect.position - CategorySelectorPadding,
			panel.rect.endPoint() + CategorySelectorPadding);

		if (panel.selected)
		{
			renderer.drawBoxFilled(panelRect, constants::PrimaryColorVariant);
		}
		else if (&panel == mMouseHighlightPanel)
		{
			renderer.drawBoxFilled(panelRect, constants::HighlightColor);
		}

		renderer.drawSubImage(imageCategoryIcons, panel.rect.position, panel.imageSlice);
	}
}


void ResearchReport::drawCategoryHeader(NAS2D::Renderer& renderer) const
{
	renderer.drawText(fontBigBold, mSelectedCategory->name, mCategoryHeaderTextPosition, constants::PrimaryTextColor);
}


void ResearchReport::drawVerticalSectionSpacer(NAS2D::Renderer& renderer, const int startX) const
{
	const NAS2D::Point<int> start{startX, area().position.y + SectionPadding.y};
	const NAS2D::Point<int> end{startX, area().position.y + area().size.y - SectionPadding.y};
	renderer.drawLine(start, end, constants::PrimaryColor);
}


void ResearchReport::drawTopicLabRequirements(NAS2D::Renderer& renderer) const
{
	renderer.drawSubImage(imageUiIcons, mStdLabIconPosition, StandardLabIconRect);
	renderer.drawSubImage(imageUiIcons, mHotLabIconPosition, HotLabIconRect);

	auto stdLabColor = constants::PrimaryTextColor;
	auto hotLabColor = constants::PrimaryTextColor;
	std::string stdLabLabel = "Underground Lab";
	std::string hotLabLabel = "Hot Lab";

	if (mTechTree.isItemSelected() && mTechCatalog)
	{
		const auto& technology = mTechCatalog->technologyFromId(mTechTree.selectedTechId());

		if (technology.labType == 0)
		{
			stdLabColor = constants::HighlightColor;
			stdLabLabel = ">> Underground Lab <<";
			if (mLabsAvailable.standard == 0) { stdLabColor = constants::WarningTextColor; }
		}
		else
		{
			hotLabColor = constants::HighlightColor;
			hotLabLabel = ">> Hot Lab <<";
			if (mLabsAvailable.hot == 0) { hotLabColor = constants::WarningTextColor; }
		}
	}

	renderer.drawText(fontMedium, stdLabLabel + " (" + std::to_string(mLabsAvailable.standard) + ")", mStdLabTextPosition, stdLabColor);
	renderer.drawText(fontMedium, hotLabLabel + " (" + std::to_string(mLabsAvailable.hot) + ")", mHotLabTextPosition, hotLabColor);
}


void ResearchReport::drawResearchStatusStrip(NAS2D::Renderer& renderer) const
{
	const auto activeLine = activeResearchLine();
	const auto queuedLine = queuedResearchLine();
	const auto readyTopics = readyTechnologiesInCategory();

	if (activeLine.empty() && queuedLine.empty() && readyTopics.empty()) { return; }

	const auto stripBottom = mResearchTopicArea.position.y - SectionPadding.y;
	const auto stripRect = NAS2D::Rectangle<int>{
		{mActiveResearchBannerPosition.x - 4, mCategoryHeaderTextPosition.y + fontBigBold.height()},
		{mResearchTreeWidth + 8, stripBottom - (mCategoryHeaderTextPosition.y + fontBigBold.height())}};
	renderer.drawBoxFilled(stripRect, NAS2D::Color{0, 0, 0, 140});

	if (!activeLine.empty())
	{
		const auto text = truncateText(fontMediumBold, "Researching: " + activeLine, mResearchTreeWidth);
		renderer.drawText(
			fontMediumBold,
			text,
			mActiveResearchBannerPosition.to<float>(),
			constants::SecondaryColor);
	}

	if (!queuedLine.empty())
	{
		const auto text = truncateText(fontMedium, "Queued: " + queuedLine, mResearchTreeWidth);
		renderer.drawText(
			fontMedium,
			text,
			mQueuedResearchBannerPosition.to<float>(),
			constants::SecondaryColor);
	}

	if (!readyTopics.empty())
	{
		std::ostringstream stream;
		stream << "Ready to research: " << readyTopics.front();
		for (std::size_t i = 1; i < readyTopics.size() && i < 3; ++i)
		{
			stream << ", " << readyTopics[i];
		}

		const auto text = truncateText(fontMedium, stream.str(), mResearchTreeWidth);
		renderer.drawText(fontMedium, text, mRecommendedTopicsPosition.to<float>(), constants::SecondaryColor);
	}
}


void ResearchReport::drawTopicHeaderPanel(NAS2D::Renderer& renderer) const
{
	if (!mTechTree.isItemSelected()) { return; }

	renderer.drawText(fontBigBold, constants::ResearchReportTopicDetails, mTopicDetailsHeaderArea.startPoint(), constants::PrimaryTextColor);

	drawTopicLabRequirements(renderer);
	drawDetailsHeaderSeparator(renderer, mTopicDetailsHeaderArea);
}


void ResearchReport::drawTopicDetailsPanel(NAS2D::Renderer& renderer) const
{
	if (!mTechTree.isItemSelected()) { return; }

	renderer.drawSubImage(imageTopicIcons, mTopicDetailsIconPosition, {mTopicDetailsIconUV, TopicIconSize});
}


void ResearchReport::draw(NAS2D::Renderer& renderer) const
{
	drawCategories(renderer);
	drawVerticalSectionSpacer(renderer, mCategoryPanels.front().rect.endPoint().x + SectionPadding.x);
	drawCategoryHeader(renderer);
	drawResearchStatusStrip(renderer);
	drawVerticalSectionSpacer(renderer, (area().size.x / 3) * 2);
	drawTopicHeaderPanel(renderer);
	drawTopicDetailsPanel(renderer);
}