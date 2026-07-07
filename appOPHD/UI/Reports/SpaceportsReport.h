#pragma once

#include "Report.h"

#include <libControls/Button.h>

#include <NAS2D/Signal/Delegate.h>

#include <string>


namespace NAS2D
{
	class Font;
	class Image;
}

class ColonyOrbitalProgram;
class StructureManager;
class TileMap;
class TechnologyCatalog;
class ResearchTracker;
struct StorableResources;


class SpaceportsReport : public Report
{
public:
	using TakeMeThereDelegate = NAS2D::Delegate<void(const Structure*)>;
	using LaunchSatelliteDelegate = NAS2D::Delegate<void(const std::string&)>;

	SpaceportsReport(const StructureManager& structureManager, TakeMeThereDelegate takeMeThereHandler);
	~SpaceportsReport() override;

	bool canView(const Structure& structure) override;
	void selectStructure(Structure&) override;
	void clearSelected() override;
	void fillLists() override;
	void refresh() override;

	void update() override;

	void injectOrbitalProgram(
		ColonyOrbitalProgram& program,
		TileMap& tileMap,
		StorableResources& resources,
		TechnologyCatalog& catalog,
		ResearchTracker& tracker,
		LaunchSatelliteDelegate launchHandler);

private:
	void onResize() override;
	void draw(NAS2D::Renderer& renderer) const override;

	void onLaunchOrbitalSurvey();
	void onLaunchDeepScan();
	void refreshLaunchButtons();
	void layoutLaunchButtons();

	TakeMeThereDelegate mTakeMeThereHandler;
	LaunchSatelliteDelegate mLaunchSatelliteHandler;

	ColonyOrbitalProgram* mOrbitalProgram{nullptr};
	TileMap* mTileMap{nullptr};
	StorableResources* mResources{nullptr};
	TechnologyCatalog* mTechnologyCatalog{nullptr};
	ResearchTracker* mResearchTracker{nullptr};

	Button btnLaunchOrbitalSurvey{"Launch Orbital Survey", {this, &SpaceportsReport::onLaunchOrbitalSurvey}};
	Button btnLaunchDeepScan{"Launch Deep Scan", {this, &SpaceportsReport::onLaunchDeepScan}};

	const NAS2D::Font& fontMedium;
	const NAS2D::Font& fontMediumBold;
	const NAS2D::Font& fontBigBold;

	const NAS2D::Image& imageSpaceport;
};