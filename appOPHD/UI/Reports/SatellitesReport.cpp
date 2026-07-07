#include "SatellitesReport.h"

#include "../../ColonyOrbitalProgram.h"
#include "../../Constants/UiConstants.h"
#include "../../CacheImage.h"
#include "../../CacheFont.h"
#include "../../Map/TileMap.h"

#include <libOPHD/Technology/ColonyResearchEffects.h>
#include <libOPHD/Technology/ResearchTracker.h>
#include <libOPHD/Technology/TechnologyCatalog.h>

#include <NAS2D/StringFrom.h>
#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/Resource/Font.h>


SatellitesReport::SatellitesReport(const StructureManager& /*structureManager*/, TakeMeThereDelegate takeMeThereHandler) :
	mTakeMeThereHandler{takeMeThereHandler},
	fontMedium{getFontMedium()},
	fontMediumBold{getFontMediumBold()},
	fontBigBold{getFontHugeBold()},
	imageSatellite{getImage("ui/icons/satellite.png")}
{
}


SatellitesReport::~SatellitesReport()
{
}


bool SatellitesReport::canView(const Structure& /*structure*/)
{
	return false;
}


void SatellitesReport::selectStructure(Structure&)
{
}


void SatellitesReport::clearSelected()
{
}


void SatellitesReport::fillLists()
{
}


void SatellitesReport::refresh()
{
}


void SatellitesReport::injectOrbitalProgram(
	ColonyOrbitalProgram& program,
	TileMap& tileMap,
	TechnologyCatalog& catalog,
	ResearchTracker& tracker)
{
	mOrbitalProgram = &program;
	mTileMap = &tileMap;
	mTechnologyCatalog = &catalog;
	mResearchTracker = &tracker;
}


void SatellitesReport::update()
{
	if (!visible()) { return; }

	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	draw(renderer);
	ControlContainer::update();
}


void SatellitesReport::onResize()
{
}


void SatellitesReport::draw(NAS2D::Renderer& renderer) const
{
	if (!visible()) { return; }

	const auto textOrigin = area().position + NAS2D::Vector{20, 12};
	renderer.drawText(fontBigBold, "Satellites Report", textOrigin, constants::PrimaryTextColor);

	auto lineY = textOrigin.y + fontBigBold.height() + 8;

	if (!mOrbitalProgram || !mTileMap || !mTechnologyCatalog || !mResearchTracker)
	{
		renderer.drawText(fontMedium, "Orbital program unavailable.", NAS2D::Point{textOrigin.x, lineY}, constants::PrimaryTextColor);
		return;
	}

	const auto researchEffects = computeColonyResearchEffects(*mResearchTracker, *mTechnologyCatalog);
	renderer.drawText(
		fontMedium,
		"Planet dig depth: " + std::to_string(mTileMap->maxDepth()) + "  |  Hidden deposits: " + std::to_string(mTileMap->undiscoveredOreDepositCount()),
		NAS2D::Point{textOrigin.x, lineY},
		constants::PrimaryTextColor);
	lineY += fontMedium.height() + 12;

	if (mOrbitalProgram->launchHistory().empty())
	{
		renderer.drawText(fontMedium, "No satellites in orbit yet. Use the Spaceports report to launch.", NAS2D::Point{textOrigin.x, lineY}, constants::PrimaryTextColor);
		return;
	}

	for (const auto& record : mOrbitalProgram->launchHistory())
	{
		renderer.drawText(fontMediumBold, record.displayName, NAS2D::Point{textOrigin.x, lineY}, constants::PrimaryTextColor);
		lineY += fontMediumBold.height() + 4;

		std::string summary = "Revealed " + std::to_string(record.minesRevealed) + " ore deposit";
		summary += record.minesRevealed == 1 ? "" : "s";
		if (record.digDepthBonus > 0)
		{
			summary += "; digging depth +" + std::to_string(record.digDepthBonus);
		}
		renderer.drawText(fontMedium, summary, NAS2D::Point{textOrigin.x, lineY}, constants::PrimaryTextColor);
		lineY += fontMedium.height() + 4;

		for (const auto& location : record.revealedLocations)
		{
			renderer.drawText(fontMedium, "  Mine at " + NAS2D::stringFrom(location), NAS2D::Point{textOrigin.x, lineY}, constants::PrimaryTextColor);
			lineY += fontMedium.height() + 2;
		}

		lineY += 8;
	}

	if (!researchEffects.unlockedSatellites.empty())
	{
		const auto pendingLaunches = researchEffects.unlockedSatellites.size() - mOrbitalProgram->launchHistory().size();
		if (pendingLaunches > 0)
		{
			renderer.drawText(fontMedium, std::to_string(pendingLaunches) + " satellite type(s) researched but not yet launched.", NAS2D::Point{textOrigin.x, lineY}, constants::PrimaryTextColor);
		}
	}
}