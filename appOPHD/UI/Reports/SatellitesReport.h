#pragma once

#include "Report.h"

#include <NAS2D/Signal/Delegate.h>


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


class SatellitesReport : public Report
{
public:
	using TakeMeThereDelegate = NAS2D::Delegate<void(const Structure*)>;

	SatellitesReport(const StructureManager& structureManager, TakeMeThereDelegate takeMeThereHandler);
	~SatellitesReport() override;

	bool canView(const Structure& structure) override;
	void selectStructure(Structure&) override;
	void clearSelected() override;
	void fillLists() override;
	void refresh() override;

	void update() override;

	void injectOrbitalProgram(
		ColonyOrbitalProgram& program,
		TileMap& tileMap,
		TechnologyCatalog& catalog,
		ResearchTracker& tracker);

private:
	void onResize() override;
	void draw(NAS2D::Renderer& renderer) const override;

	TakeMeThereDelegate mTakeMeThereHandler;

	ColonyOrbitalProgram* mOrbitalProgram{nullptr};
	TileMap* mTileMap{nullptr};
	TechnologyCatalog* mTechnologyCatalog{nullptr};
	ResearchTracker* mResearchTracker{nullptr};

	const NAS2D::Font& fontMedium;
	const NAS2D::Font& fontMediumBold;
	const NAS2D::Font& fontBigBold;

	const NAS2D::Image& imageSatellite;
};