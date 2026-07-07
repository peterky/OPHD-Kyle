#include "WorkforceReport.h"

#include "../../CacheImage.h"
#include "../../CacheFont.h"
#include "../../Constants/Strings.h"
#include "../../Constants/UiConstants.h"
#include "../../StructureManager.h"
#include "../../MapObjects/Structure.h"

#include <libOPHD/EnumDisabledReason.h>
#include <libOPHD/Population/PopulationModel.h>
#include <libOPHD/Population/PopulationPool.h>
#include <libOPHD/Population/PopulationRequirements.h>

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
	constexpr auto panelPadding = NAS2D::Vector{12, 12};
	constexpr auto controlButtonSize = NAS2D::Vector{54, 26};
	constexpr auto directionButtonSize = NAS2D::Vector{104, 26};
	constexpr auto headerRowHeight = 40;
	constexpr auto statsLineHeight = 17;
	constexpr auto sectionGap = 18;
	constexpr auto rowGap = 10;
	constexpr auto labelHeight = 16;
	constexpr auto controlRowGap = 12;

	constexpr auto rightHeaderHeight = 58;
	constexpr auto rightDetailsHeight = 96;


	struct LeftPanelLayout
	{
		int conversionTitleY{0};
		int conversionSummaryY{0};
		int directionButtonsY{0};
		int rateLabelY{0};
		int rateButtonsY{0};
		int rateHintY{0};
		int biasTitleY{0};
		int biasButtonsY{0};
		int biasHintY{0};
	};


	bool hasWorkerShortage(const Structure& structure)
	{
		if (structure.destroyed() || structure.underConstruction()) { return false; }

		const auto& requirements = structure.populationRequirements();
		if (requirements.workers <= 0) { return false; }

		const auto& available = structure.populationAvailable();
		return available.workers < requirements.workers;
	}


	std::string shortageDescription(const Structure& structure)
	{
		const auto& requirements = structure.populationRequirements();
		const auto& available = structure.populationAvailable();

		std::string description = std::to_string(available.workers) + " / " + std::to_string(requirements.workers) + " workers";
		if (structure.disabled() && structure.disabledReason() == DisabledReason::Population)
		{
			description += ", disabled (staffing)";
		}

		return description;
	}


	std::string universityBiasLabel(int scientistPercent)
	{
		if (scientistPercent < 0) { return "Auto"; }
		return std::to_string(scientistPercent) + "% scientists";
	}


	LeftPanelLayout buildLeftPanelLayout(int statLineCount)
	{
		const auto statsStart = headerRowHeight + rowGap;
		const auto statsEnd = statsStart + statLineCount * statsLineHeight;

		LeftPanelLayout layout;
		layout.conversionTitleY = statsEnd + sectionGap;
		layout.conversionSummaryY = layout.conversionTitleY + labelHeight + rowGap;
		layout.directionButtonsY = layout.conversionSummaryY + labelHeight + controlRowGap;
		layout.rateLabelY = layout.directionButtonsY + directionButtonSize.y + controlRowGap;
		layout.rateButtonsY = layout.rateLabelY + labelHeight + rowGap;
		layout.rateHintY = layout.rateButtonsY + controlButtonSize.y + rowGap;
		layout.biasTitleY = layout.rateHintY + labelHeight + sectionGap;
		layout.biasButtonsY = layout.biasTitleY + labelHeight + controlRowGap;
		layout.biasHintY = layout.biasButtonsY + controlButtonSize.y + rowGap;
		return layout;
	}


	NAS2D::Point<int> contentOrigin(const NAS2D::Rectangle<int>& reportArea)
	{
		return reportArea.position + panelPadding;
	}


	int leftPanelWidth(const NAS2D::Rectangle<int>& reportArea)
	{
		return std::max(reportArea.size.x / 2 - panelPadding.x, 200);
	}


	int rightPanelOriginX(const NAS2D::Rectangle<int>& reportArea)
	{
		return reportArea.position.x + reportArea.size.x / 2;
	}


	int rightPanelWidth(const NAS2D::Rectangle<int>& reportArea)
	{
		return reportArea.endPoint().x - rightPanelOriginX(reportArea) - panelPadding.x;
	}
}


WorkforceReport::WorkforceReport(const StructureManager& structureManager, TakeMeThereDelegate takeMeThereHandler) :
	mStructureManager{structureManager},
	mTakeMeThereHandler{takeMeThereHandler},
	mFontMedium{getFontMedium()},
	mFontMediumBold{getFontMediumBold()},
	mFontBigBold{getFontHugeBold()},
	mImageUniversity{getImage("structures/university.png")},
	btnTakeMeThere{constants::TakeMeThere, {this, &WorkforceReport::onTakeMeThere}},
	lstShortages{{this, &WorkforceReport::onStructureSelectionChange}}
{
	for (auto* button : {&btnStatusQuo, &btnSciToWorkers, &btnWorkersToSci})
	{
		button->size(directionButtonSize);
		button->type(Button::Type::Toggle);
		button->toggle(false);
		button->enabled(false);
		add(*button, panelPadding);
	}
	btnStatusQuo.toggle(true);

	for (auto* button : {&btnRate5, &btnRate10, &btnRate20, &btnBiasAuto, &btnBias25, &btnBias50, &btnBias75})
	{
		button->size(controlButtonSize);
		button->type(Button::Type::Toggle);
		button->toggle(false);
		button->enabled(false);
		add(*button, panelPadding);
	}

	btnTakeMeThere.size({132, 28});
	btnTakeMeThere.hide();

	add(btnTakeMeThere, panelPadding);
	add(lstShortages, panelPadding);
	lstShortages.hide();

	NAS2D::Utility<NAS2D::EventHandler>::get().mouseDoubleClick().connect({this, &WorkforceReport::onDoubleClick});

	layoutControls();
}


WorkforceReport::~WorkforceReport()
{
	NAS2D::Utility<NAS2D::EventHandler>::get().mouseDoubleClick().disconnect({this, &WorkforceReport::onDoubleClick});
}


void WorkforceReport::injectPopulation(PopulationModel& populationModel, const PopulationPool& populationPool)
{
	mPopulationModel = &populationModel;
	mPopulationPool = &populationPool;
	setPopulationControlsEnabled(true);
	refresh();
}


void WorkforceReport::setPopulationControlsEnabled(bool enabled)
{
	for (auto* button : {&btnStatusQuo, &btnSciToWorkers, &btnWorkersToSci, &btnRate5, &btnRate10, &btnRate20, &btnBiasAuto, &btnBias25, &btnBias50, &btnBias75})
	{
		button->enabled(enabled);
	}

	if (enabled && mConversionDirection == ConversionDirection::StatusQuo)
	{
		btnRate5.enabled(false);
		btnRate10.enabled(false);
		btnRate20.enabled(false);
	}
}


bool WorkforceReport::canView(const Structure& /*structure*/)
{
	return false;
}


void WorkforceReport::selectStructure(Structure& structure)
{
	if (mShortageCount > 0)
	{
		lstShortages.setSelected(&structure);
	}
}


void WorkforceReport::clearSelected()
{
	lstShortages.clearSelected();
}


int WorkforceReport::countStatLines() const
{
	int lines = 5;
	if (mPopulationPool && mPopulationPool->scientistsAsWorkers() > 0) { ++lines; }
	return lines;
}


void WorkforceReport::layoutControls()
{
	const auto reportArea = area();
	const auto origin = contentOrigin(reportArea);
	const auto statLines = countStatLines();
	const auto layout = buildLeftPanelLayout(statLines);

	auto directionOffset = NAS2D::Point{origin.x, origin.y + layout.directionButtonsY};
	for (auto* button : {&btnStatusQuo, &btnSciToWorkers, &btnWorkersToSci})
	{
		button->position(directionOffset);
		directionOffset.x += button->size().x + constants::MarginTight;
	}

	auto rateOffset = NAS2D::Point{origin.x, origin.y + layout.rateButtonsY};
	for (auto* button : {&btnRate5, &btnRate10, &btnRate20})
	{
		button->position(rateOffset);
		const bool rateEnabled = mPopulationModel && mConversionDirection != ConversionDirection::StatusQuo;
		button->enabled(rateEnabled);
		rateOffset.x += button->size().x + constants::MarginTight;
	}

	auto biasOffset = NAS2D::Point{origin.x, origin.y + layout.biasButtonsY};
	for (auto* button : {&btnBiasAuto, &btnBias25, &btnBias50, &btnBias75})
	{
		button->position(biasOffset);
		button->enabled(mPopulationModel != nullptr);
		biasOffset.x += button->size().x + constants::MarginTight;
	}

	const auto rightOriginX = rightPanelOriginX(reportArea);
	const auto rightWidth = rightPanelWidth(reportArea);

	if (mShortageCount > 0)
	{
		btnTakeMeThere.show();
		btnTakeMeThere.position({
			reportArea.endPoint().x - btnTakeMeThere.size().x - panelPadding.x,
			origin.y
		});

		const auto hasSelection = lstShortages.selectedStructure() != nullptr;
		const auto detailsReserve = hasSelection ? rightDetailsHeight : 0;
		const auto listTop = origin.y + rightHeaderHeight;
		const auto listHeight = reportArea.endPoint().y - listTop - detailsReserve - panelPadding.y;
		lstShortages.show();
		lstShortages.position({rightOriginX + panelPadding.x, listTop});
		lstShortages.size({rightWidth, std::max(listHeight, 80)});
	}
	else
	{
		btnTakeMeThere.hide();
		lstShortages.hide();
		lstShortages.clearSelected();
		lstShortages.size({0, 0});
	}
}


void WorkforceReport::updateShortagePanelVisibility()
{
	layoutControls();
}


void WorkforceReport::fillLists()
{
	mWorkersRequired = 0;
	mScientistsRequired = 0;
	mShortageCount = 0;

	for (auto* structure : mStructureManager.allStructures())
	{
		if (structure->destroyed() || structure->underConstruction()) { continue; }

		const auto& requirements = structure->populationRequirements();
		mWorkersRequired += requirements.workers;
		mScientistsRequired += requirements.scientists;

		if (hasWorkerShortage(*structure)) { ++mShortageCount; }
	}

	fillShortageList();
	updateShortagePanelVisibility();
}


void WorkforceReport::refresh()
{
	if (!mPopulationModel) { return; }

	const auto signedRate = mPopulationModel->retrainingRatePerTurn();
	if (signedRate == 0)
	{
		mConversionDirection = ConversionDirection::StatusQuo;
		mConversionRate = 0;
	}
	else if (signedRate > 0)
	{
		mConversionDirection = ConversionDirection::ScientistsToWorkers;
		mConversionRate = signedRate;
	}
	else
	{
		mConversionDirection = ConversionDirection::WorkersToScientists;
		mConversionRate = -signedRate;
	}

	clearConversionDirectionButtons();
	clearConversionRateButtons();
	clearBiasButtons();

	switch (mConversionDirection)
	{
	case ConversionDirection::ScientistsToWorkers: btnSciToWorkers.toggle(true); break;
	case ConversionDirection::WorkersToScientists: btnWorkersToSci.toggle(true); break;
	default: btnStatusQuo.toggle(true); break;
	}

	switch (mConversionRate)
	{
	case 20: btnRate20.toggle(true); break;
	case 10: btnRate10.toggle(true); break;
	case 5: btnRate5.toggle(true); break;
	default:
		if (mConversionDirection != ConversionDirection::StatusQuo)
		{
			btnRate5.toggle(true);
			mConversionRate = 5;
		}
		break;
	}

	const auto scientistPercent = mPopulationModel->universityScientistPercent();
	if (scientistPercent < 0) { btnBiasAuto.toggle(true); }
	else if (scientistPercent <= 25) { btnBias25.toggle(true); }
	else if (scientistPercent <= 50) { btnBias50.toggle(true); }
	else { btnBias75.toggle(true); }

	fillLists();
}


void WorkforceReport::fillShortageList()
{
	std::vector<Structure*> shortages;
	for (auto* structure : mStructureManager.allStructures())
	{
		if (!structure || structure->destroyed() || structure->underConstruction()) { continue; }
		if (hasWorkerShortage(*structure))
		{
			shortages.push_back(structure);
		}
	}

	std::sort(shortages.begin(), shortages.end(), [](const Structure* lhs, const Structure* rhs)
	{
		const auto lhsGap = lhs->populationRequirements().workers - lhs->populationAvailable().workers;
		const auto rhsGap = rhs->populationRequirements().workers - rhs->populationAvailable().workers;
		return lhsGap > rhsGap;
	});

	mUpdatingShortageList = true;
	lstShortages.clear();
	for (auto* structure : shortages)
	{
		lstShortages.addItem(structure, shortageDescription(*structure));
	}
	mUpdatingShortageList = false;
}


void WorkforceReport::clearConversionDirectionButtons()
{
	for (auto* button : {&btnStatusQuo, &btnSciToWorkers, &btnWorkersToSci})
	{
		button->toggle(false);
	}
}


void WorkforceReport::clearConversionRateButtons()
{
	for (auto* button : {&btnRate5, &btnRate10, &btnRate20})
	{
		button->toggle(false);
	}
}


std::string WorkforceReport::conversionSummaryText() const
{
	switch (mConversionDirection)
	{
	case ConversionDirection::ScientistsToWorkers:
		return "Converting " + std::to_string(mConversionRate) + " scientists into workers each turn.";
	case ConversionDirection::WorkersToScientists:
		return "Converting " + std::to_string(mConversionRate) + " workers into scientists each turn.";
	default:
		return "Status quo: no adult role changes each turn.";
	}
}


void WorkforceReport::applyConversionSettings()
{
	if (!mPopulationModel) { return; }

	switch (mConversionDirection)
	{
	case ConversionDirection::ScientistsToWorkers:
		mPopulationModel->setRetrainingRatePerTurn(mConversionRate);
		break;
	case ConversionDirection::WorkersToScientists:
		mPopulationModel->setRetrainingRatePerTurn(-mConversionRate);
		break;
	default:
		mPopulationModel->setRetrainingRatePerTurn(0);
		break;
	}
}


void WorkforceReport::clearBiasButtons()
{
	for (auto* button : {&btnBiasAuto, &btnBias25, &btnBias50, &btnBias75})
	{
		button->toggle(false);
	}
}


void WorkforceReport::selectConversionDirection(ConversionDirection direction)
{
	if (!mPopulationModel) { return; }

	clearConversionDirectionButtons();
	mConversionDirection = direction;

	switch (direction)
	{
	case ConversionDirection::ScientistsToWorkers:
		btnSciToWorkers.toggle(true);
		if (mConversionRate == 0) { mConversionRate = 5; }
		break;
	case ConversionDirection::WorkersToScientists:
		btnWorkersToSci.toggle(true);
		if (mConversionRate == 0) { mConversionRate = 5; }
		break;
	default:
		mConversionDirection = ConversionDirection::StatusQuo;
		mConversionRate = 0;
		btnStatusQuo.toggle(true);
		break;
	}

	clearConversionRateButtons();
	switch (mConversionRate)
	{
	case 20: btnRate20.toggle(true); break;
	case 10: btnRate10.toggle(true); break;
	default:
		if (mConversionDirection != ConversionDirection::StatusQuo)
		{
			mConversionRate = 5;
			btnRate5.toggle(true);
		}
		break;
	}

	applyConversionSettings();
	layoutControls();
}


void WorkforceReport::selectConversionRate(int rate)
{
	if (!mPopulationModel || mConversionDirection == ConversionDirection::StatusQuo) { return; }

	clearConversionRateButtons();
	mConversionRate = rate;

	switch (rate)
	{
	case 20: btnRate20.toggle(true); break;
	case 10: btnRate10.toggle(true); break;
	default:
		mConversionRate = 5;
		btnRate5.toggle(true);
		break;
	}

	applyConversionSettings();
}


void WorkforceReport::selectUniversityBias(int scientistPercent)
{
	if (!mPopulationModel) { return; }

	clearBiasButtons();
	mPopulationModel->setUniversityScientistPercent(scientistPercent);

	if (scientistPercent < 0) { btnBiasAuto.toggle(true); }
	else if (scientistPercent <= 25) { btnBias25.toggle(true); }
	else if (scientistPercent <= 50) { btnBias50.toggle(true); }
	else { btnBias75.toggle(true); }
}


void WorkforceReport::onStatusQuoClicked()
{
	selectConversionDirection(ConversionDirection::StatusQuo);
}


void WorkforceReport::onSciToWorkersClicked()
{
	selectConversionDirection(ConversionDirection::ScientistsToWorkers);
}


void WorkforceReport::onWorkersToSciClicked()
{
	selectConversionDirection(ConversionDirection::WorkersToScientists);
}


void WorkforceReport::onRate5Clicked()
{
	selectConversionRate(5);
}


void WorkforceReport::onRate10Clicked()
{
	selectConversionRate(10);
}


void WorkforceReport::onRate20Clicked()
{
	selectConversionRate(20);
}


void WorkforceReport::onBiasAutoClicked()
{
	selectUniversityBias(-1);
}


void WorkforceReport::onBias25Clicked()
{
	selectUniversityBias(25);
}


void WorkforceReport::onBias50Clicked()
{
	selectUniversityBias(50);
}


void WorkforceReport::onBias75Clicked()
{
	selectUniversityBias(75);
}


void WorkforceReport::onTakeMeThere()
{
	if (const auto* structure = lstShortages.selectedStructure())
	{
		if (mTakeMeThereHandler) { mTakeMeThereHandler(structure); }
	}
}


void WorkforceReport::onStructureSelectionChange()
{
	if (mUpdatingShortageList) { return; }
	layoutControls();
}


void WorkforceReport::onDoubleClick(NAS2D::MouseButton button, NAS2D::Point<int> /*position*/)
{
	if (!visible() || button != NAS2D::MouseButton::Left) { return; }
	onTakeMeThere();
}


void WorkforceReport::drawSummaryPanel(NAS2D::Renderer& renderer) const
{
	const auto reportArea = area();
	const auto leftPanel = NAS2D::Rectangle<int>{
		reportArea.position,
		{reportArea.size.x / 2, reportArea.size.y}
	};
	renderer.drawBoxFilled(leftPanel, NAS2D::Color{25, 25, 25});

	const auto origin = contentOrigin(reportArea);
	renderer.drawText(mFontMediumBold, "Colony Workforce", origin, constants::PrimaryTextColor);

	if (!mPopulationModel || !mPopulationPool)
	{
		renderer.drawText(mFontMedium, "Population data unavailable.", origin + NAS2D::Vector{0, headerRowHeight + rowGap}, NAS2D::Color::Yellow);
		return;
	}

	const auto& population = mPopulationModel->getPopulations();
	const auto availableWorkers = mPopulationPool->availableWorkers();
	const auto availableScientists = mPopulationPool->availableScientists();
	const auto scientistsCoveringWorkers = mPopulationPool->scientistsAsWorkers();
	const auto statLines = countStatLines();
	const auto layout = buildLeftPanelLayout(statLines);

	auto textY = headerRowHeight + rowGap;
	const auto drawLine = [&](const std::string& text, NAS2D::Color color = constants::PrimaryTextColor)
	{
		renderer.drawText(mFontMedium, text, origin + NAS2D::Vector{0, textY}, color);
		textY += statsLineHeight;
	};

	drawLine("Children: " + std::to_string(population.child) + "    Students: " + std::to_string(population.student));
	drawLine("Workers: " + std::to_string(population.worker) + "    Scientists: " + std::to_string(population.scientist) + "    Retired: " + std::to_string(population.retiree));
	drawLine(
		"Available: " + std::to_string(availableWorkers) + " workers, " + std::to_string(availableScientists) + " scientists",
		(availableWorkers < 0 || availableScientists < 0) ? NAS2D::Color::Red : constants::PrimaryTextColor
	);
	drawLine("Demand: " + std::to_string(mWorkersRequired) + " workers, " + std::to_string(mScientistsRequired) + " scientists");
	if (scientistsCoveringWorkers > 0)
	{
		drawLine("Scientists covering worker slots: " + std::to_string(scientistsCoveringWorkers), NAS2D::Color{255, 220, 160});
	}
	drawLine("Structures short on workers: " + std::to_string(mShortageCount), mShortageCount > 0 ? NAS2D::Color{255, 180, 120} : NAS2D::Color{120, 200, 120});

	const auto layoutBottom = layout.biasHintY + labelHeight;
	const auto leftContentHeight = leftPanel.size.y - panelPadding.y * 2;
	if (layoutBottom > leftContentHeight)
	{
		renderer.drawText(
			mFontMedium,
			"Resize the window for full workforce controls.",
			origin + NAS2D::Vector{0, leftContentHeight - labelHeight},
			NAS2D::Color::Yellow
		);
		return;
	}

	const auto statsStart = headerRowHeight + rowGap;
	const auto statsEnd = statsStart + statLines * statsLineHeight;
	renderer.drawLine(
		(origin + NAS2D::Vector{0, statsEnd + 6}).to<float>(),
		(origin + NAS2D::Vector{leftPanelWidth(reportArea), statsEnd + 6}).to<float>(),
		NAS2D::Color{48, 48, 48}
	);

	renderer.drawText(mFontMediumBold, "Workforce Conversion", origin + NAS2D::Vector{0, layout.conversionTitleY}, constants::PrimaryTextColor);
	renderer.drawText(mFontMedium, conversionSummaryText(), origin + NAS2D::Vector{0, layout.conversionSummaryY}, NAS2D::Color{150, 150, 150});
	renderer.drawText(mFontMedium, "Per turn:", origin + NAS2D::Vector{0, layout.rateLabelY}, NAS2D::Color{150, 150, 150});
	renderer.drawText(mFontMedium, "Direction first, then rate. Status quo changes nothing.", origin + NAS2D::Vector{0, layout.rateHintY}, NAS2D::Color{120, 120, 120});

	renderer.drawText(
		mFontMediumBold,
		"University graduates: " + universityBiasLabel(mPopulationModel->universityScientistPercent()),
		origin + NAS2D::Vector{0, layout.biasTitleY},
		constants::PrimaryTextColor
	);
	renderer.drawText(mFontMedium, "New students become scientists vs workers.", origin + NAS2D::Vector{0, layout.biasHintY}, NAS2D::Color{120, 120, 120});
}


void WorkforceReport::drawRightPanel(NAS2D::Renderer& renderer) const
{
	const auto reportArea = area();
	const auto rightOriginX = rightPanelOriginX(reportArea);
	const auto rightPanel = NAS2D::Rectangle<int>{
		{rightOriginX, reportArea.position.y},
		{reportArea.endPoint().x - rightOriginX, reportArea.size.y}
	};
	renderer.drawBoxFilled(rightPanel, NAS2D::Color{20, 20, 20});

	const auto origin = contentOrigin(reportArea);
	const auto rightContentOrigin = NAS2D::Point{rightOriginX + panelPadding.x, origin.y};
	renderer.drawText(mFontMediumBold, "Worker Shortages", rightContentOrigin, constants::PrimaryTextColor);
	renderer.drawText(
		mFontMedium,
		"Understaffed structures, sorted by severity.",
		rightContentOrigin + NAS2D::Vector{0, labelHeight + 4},
		NAS2D::Color{150, 150, 150}
	);

	if (mShortageCount == 0)
	{
		const auto message = std::string{"All structures are fully staffed."};
		const auto submessage = std::string{"No worker shortages this turn."};
		const auto messageSize = mFontMediumBold.size(message);
		const auto submessageSize = mFontMedium.size(submessage);
		const auto centerY = rightPanel.position.y + rightPanel.size.y / 2;

		renderer.drawText(
			mFontMediumBold,
			message,
			NAS2D::Point<int>{rightPanel.position.x + (rightPanel.size.x - messageSize.x) / 2, centerY - 16},
			NAS2D::Color{120, 200, 120}
		);
		renderer.drawText(
			mFontMedium,
			submessage,
			NAS2D::Point<int>{rightPanel.position.x + (rightPanel.size.x - submessageSize.x) / 2, centerY + 10},
			NAS2D::Color{150, 150, 150}
		);
		return;
	}

	if (const auto* structure = lstShortages.selectedStructure();
		structure && !structure->destroyed())
	{
		const auto detailsOrigin = NAS2D::Point{
			rightContentOrigin.x,
			reportArea.endPoint().y - rightDetailsHeight - panelPadding.y
		};
		renderer.drawLine(
			detailsOrigin + NAS2D::Vector{0, -8},
			detailsOrigin + NAS2D::Vector{rightPanelWidth(reportArea), -8},
			NAS2D::Color{48, 48, 48}
		);
		renderer.drawText(mFontBigBold, structure->name(), detailsOrigin, constants::PrimaryTextColor);
		renderer.drawText(mFontMedium, shortageDescription(*structure), detailsOrigin + NAS2D::Vector{0, 30}, constants::PrimaryTextColor);
	}
	else if (lstShortages.area().size.y > 0)
	{
		const auto hint = std::string{"Select a structure, then use Take Me There."};
		const auto hintSize = mFontMedium.size(hint);
		const auto hintY = origin.y + rightHeaderHeight + (lstShortages.area().size.y - hintSize.y) / 2;
		renderer.drawText(
			mFontMedium,
			hint,
			NAS2D::Point{rightContentOrigin.x + (rightPanelWidth(reportArea) - hintSize.x) / 2, hintY},
			NAS2D::Color::Yellow
		);
	}
}


void WorkforceReport::update()
{
	if (!visible()) { return; }

	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	draw(renderer);
	ControlContainer::update();
}


void WorkforceReport::draw(NAS2D::Renderer& renderer) const
{
	if (!visible()) { return; }

	drawSummaryPanel(renderer);
	drawRightPanel(renderer);

	const auto dividerX = rightPanelOriginX(area());
	renderer.drawLine(
		NAS2D::Point{dividerX, area().position.y + panelPadding.y},
		NAS2D::Point{dividerX, area().endPoint().y - panelPadding.y},
		constants::PrimaryColor
	);
}


void WorkforceReport::onResize()
{
	Control::onResize();
	layoutControls();
}