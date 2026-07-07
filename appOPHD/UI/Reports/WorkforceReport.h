#pragma once

#include "Report.h"

#include "../StructureListBox.h"

#include <libControls/Button.h>

#include <NAS2D/Signal/Delegate.h>


namespace NAS2D
{
	class Font;
	class Image;

	template <typename BaseType> struct Point;
}


class PopulationModel;
class PopulationPool;
class Structure;
class StructureManager;


class WorkforceReport : public Report
{
public:
	using TakeMeThereDelegate = NAS2D::Delegate<void(const Structure*)>;

	WorkforceReport(const StructureManager& structureManager, TakeMeThereDelegate takeMeThereHandler);
	~WorkforceReport() override;

	bool canView(const Structure& structure) override;
	void selectStructure(Structure&) override;
	void clearSelected() override;
	void fillLists() override;
	void refresh() override;

	void injectPopulation(PopulationModel& populationModel, const PopulationPool& populationPool);

	void update() override;
	void draw(NAS2D::Renderer& renderer) const override;

private:
	enum class ConversionDirection
	{
		StatusQuo,
		ScientistsToWorkers,
		WorkersToScientists
	};

	void onResize() override;
	void onStatusQuoClicked();
	void onSciToWorkersClicked();
	void onWorkersToSciClicked();
	void onRate5Clicked();
	void onRate10Clicked();
	void onRate20Clicked();
	void onBiasAutoClicked();
	void onBias25Clicked();
	void onBias50Clicked();
	void onBias75Clicked();
	void selectConversionDirection(ConversionDirection direction);
	void selectConversionRate(int rate);
	void selectUniversityBias(int scientistPercent);
	void onTakeMeThere();
	void onStructureSelectionChange();
	void onDoubleClick(NAS2D::MouseButton button, NAS2D::Point<int> position);
	void clearConversionDirectionButtons();
	void clearConversionRateButtons();
	void clearBiasButtons();
	void applyConversionSettings();
	void setPopulationControlsEnabled(bool enabled);
	void fillShortageList();
	void layoutControls();
	void updateShortagePanelVisibility();
	int countStatLines() const;
	std::string conversionSummaryText() const;
	void drawSummaryPanel(NAS2D::Renderer& renderer) const;
	void drawRightPanel(NAS2D::Renderer& renderer) const;

	const StructureManager& mStructureManager;
	TakeMeThereDelegate mTakeMeThereHandler;
	PopulationModel* mPopulationModel{nullptr};
	const PopulationPool* mPopulationPool{nullptr};

	const NAS2D::Font& mFontMedium;
	const NAS2D::Font& mFontMediumBold;
	const NAS2D::Font& mFontBigBold;
	const NAS2D::Image& mImageUniversity;

	Button btnStatusQuo{"Status quo", {this, &WorkforceReport::onStatusQuoClicked}};
	Button btnSciToWorkers{"Sci -> Workers", {this, &WorkforceReport::onSciToWorkersClicked}};
	Button btnWorkersToSci{"Workers -> Sci", {this, &WorkforceReport::onWorkersToSciClicked}};
	Button btnRate5{"5 / turn", {this, &WorkforceReport::onRate5Clicked}};
	Button btnRate10{"10 / turn", {this, &WorkforceReport::onRate10Clicked}};
	Button btnRate20{"20 / turn", {this, &WorkforceReport::onRate20Clicked}};
	Button btnBiasAuto{"Auto", {this, &WorkforceReport::onBiasAutoClicked}};
	Button btnBias25{"25%", {this, &WorkforceReport::onBias25Clicked}};
	Button btnBias50{"50%", {this, &WorkforceReport::onBias50Clicked}};
	Button btnBias75{"75%", {this, &WorkforceReport::onBias75Clicked}};
	Button btnTakeMeThere;
	StructureListBox lstShortages;

	ConversionDirection mConversionDirection{ConversionDirection::StatusQuo};
	int mConversionRate{0};

	int mWorkersRequired{0};
	int mScientistsRequired{0};
	int mShortageCount{0};
	bool mUpdatingShortageList{false};
};