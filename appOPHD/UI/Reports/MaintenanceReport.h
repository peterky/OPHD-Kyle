#pragma once

#include "Report.h"

#include "../StructureListBox.h"

#include <libControls/Button.h>
#include <libControls/Label.h>

#include <NAS2D/Signal/Delegate.h>


namespace NAS2D
{
	class Font;
	class Image;
}


class Structure;
class StructureManager;


class MaintenanceReport : public Report
{
public:
	using TakeMeThereDelegate = NAS2D::Delegate<void(const Structure*)>;

	MaintenanceReport(const StructureManager& structureManager, TakeMeThereDelegate takeMeThereHandler);
	~MaintenanceReport() override;

	bool canView(const Structure& structure) override;
	void selectStructure(Structure&) override;
	void clearSelected() override;
	void fillLists() override;
	void refresh() override;

	void update() override;
	void draw(NAS2D::Renderer& renderer) const override;

private:
	void onResize() override;
	void onShowAll();
	void onNeedsRepair();
	void onHighPriority();
	void onDisabled();
	void onTakeMeThere();
	void onStructureSelectionChange();
	void onDoubleClick(NAS2D::MouseButton button, NAS2D::Point<int> position);
	void filterButtonClicked();
	void fillListFromStructures(const std::vector<Structure*>& structures);
	void drawLeftPanel(NAS2D::Renderer& renderer) const;
	void drawRightPanel(NAS2D::Renderer& renderer) const;

private:
	const StructureManager& mStructureManager;
	TakeMeThereDelegate mTakeMeThereHandler;

	const NAS2D::Font& mFontMedium;
	const NAS2D::Font& mFontMediumBold;
	const NAS2D::Font& mFontBigBold;
	const NAS2D::Image& mImageMaintenance;

	Button btnShowAll;
	Button btnNeedsRepair;
	Button btnHighPriority;
	Button btnDisabled;
	Button btnTakeMeThere;

	StructureListBox lstStructures;

	int mStructuresNeedingRepair{0};
	int mHighPriorityCount{0};
};