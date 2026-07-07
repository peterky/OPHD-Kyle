#include "SpaceportsReport.h"

#include "../../ColonyOrbitalProgram.h"
#include "../../Constants/UiConstants.h"
#include "../../CacheImage.h"
#include "../../CacheFont.h"
#include "../../Map/TileMap.h"

#include <libOPHD/Technology/ColonyResearchEffects.h>
#include <libOPHD/Technology/ResearchTracker.h>
#include <libOPHD/Technology/TechnologyCatalog.h>
#include <libOPHD/StorableResources.h>

#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/Resource/Font.h>


namespace
{
	constexpr auto panelPadding = NAS2D::Vector{20, 12};
	constexpr auto satelliteBlockGap = 14;
	constexpr auto launchButtonGap = 8;
	constexpr auto launchButtonSize = NAS2D::Vector{220, 30};


	std::string formatLaunchCost(const StorableResources& cost)
	{
		return "Cost: " + std::to_string(cost.resources[0]) + " common, "
			+ std::to_string(cost.resources[1]) + " rare, "
			+ std::to_string(cost.resources[2]) + " metals";
	}


	struct SatelliteLayoutEntry
	{
		std::string id;
		int blockTop{0};
		int buttonY{0};
		bool showButton{false};
	};


	struct SpaceportsLayout
	{
		NAS2D::Point<int> textOrigin;
		int subtitleY{0};
		int contentStartY{0};
		int footerY{0};
		std::vector<SatelliteLayoutEntry> satellites;
	};


	int satelliteBlockHeight(const NAS2D::Font& fontMedium, const NAS2D::Font& fontMediumBold)
	{
		return fontMediumBold.height() + 4 + fontMedium.height() + 4 + fontMedium.height() + launchButtonGap;
	}


	SpaceportsLayout buildLayout(
		const NAS2D::Rectangle<int>& reportArea,
		const NAS2D::Font& fontMedium,
		const NAS2D::Font& fontMediumBold,
		const NAS2D::Font& fontBigBold,
		ColonyOrbitalProgram* orbitalProgram,
		TechnologyCatalog* technologyCatalog,
		ResearchTracker* researchTracker)
	{
		SpaceportsLayout layout;
		layout.textOrigin = reportArea.position + panelPadding;
		layout.subtitleY = layout.textOrigin.y + fontBigBold.height() + 6;
		layout.contentStartY = layout.subtitleY + fontMedium.height() + 16;

		if (!orbitalProgram || !technologyCatalog || !researchTracker)
		{
			layout.footerY = layout.contentStartY;
			return layout;
		}

		const auto researchEffects = computeColonyResearchEffects(*researchTracker, *technologyCatalog);
		auto contentY = layout.contentStartY;

		for (const auto& definition : ColonyOrbitalProgram::catalog())
		{
			if (!researchEffects.hasSatelliteUnlocked(definition.id)) { continue; }

			SatelliteLayoutEntry entry;
			entry.id = definition.id;
			entry.blockTop = contentY;
			contentY += satelliteBlockHeight(fontMedium, fontMediumBold);

			entry.showButton = orbitalProgram->canLaunch(definition.id, researchEffects);
			if (entry.showButton)
			{
				entry.buttonY = contentY;
				contentY += launchButtonSize.y + satelliteBlockGap;
			}
			else
			{
				contentY += satelliteBlockGap;
			}

			layout.satellites.push_back(entry);
		}

		layout.footerY = contentY;
		return layout;
	}
}


SpaceportsReport::SpaceportsReport(const StructureManager& /*structureManager*/, TakeMeThereDelegate takeMeThereHandler) :
	mTakeMeThereHandler{takeMeThereHandler},
	fontMedium{getFontMedium()},
	fontMediumBold{getFontMediumBold()},
	fontBigBold{getFontHugeBold()},
	imageSpaceport{getImage("ui/icons/spaceport.png")}
{
	btnLaunchOrbitalSurvey.size(launchButtonSize);
	btnLaunchDeepScan.size(launchButtonSize);
	add(btnLaunchOrbitalSurvey, panelPadding);
	add(btnLaunchDeepScan, panelPadding);
}


SpaceportsReport::~SpaceportsReport()
{
}


bool SpaceportsReport::canView(const Structure& /*structure*/)
{
	return false;
}


void SpaceportsReport::selectStructure(Structure&)
{
}


void SpaceportsReport::clearSelected()
{
}


void SpaceportsReport::fillLists()
{
}


void SpaceportsReport::refresh()
{
	refreshLaunchButtons();
}


void SpaceportsReport::injectOrbitalProgram(
	ColonyOrbitalProgram& program,
	TileMap& tileMap,
	StorableResources& resources,
	TechnologyCatalog& catalog,
	ResearchTracker& tracker,
	LaunchSatelliteDelegate launchHandler)
{
	mOrbitalProgram = &program;
	mTileMap = &tileMap;
	mResources = &resources;
	mTechnologyCatalog = &catalog;
	mResearchTracker = &tracker;
	mLaunchSatelliteHandler = launchHandler;
	refreshLaunchButtons();
}


void SpaceportsReport::layoutLaunchButtons()
{
	btnLaunchOrbitalSurvey.hide();
	btnLaunchDeepScan.hide();

	if (!visible()) { return; }

	const auto layout = buildLayout(area(), fontMedium, fontMediumBold, fontBigBold, mOrbitalProgram, mTechnologyCatalog, mResearchTracker);

	for (const auto& entry : layout.satellites)
	{
		if (!entry.showButton) { continue; }

		const auto buttonPosition = NAS2D::Point{layout.textOrigin.x, entry.buttonY};
		if (entry.id == "orbital_survey")
		{
			btnLaunchOrbitalSurvey.show();
			btnLaunchOrbitalSurvey.position(buttonPosition);
		}
		else if (entry.id == "deep_scan")
		{
			btnLaunchDeepScan.show();
			btnLaunchDeepScan.position(buttonPosition);
		}
	}
}


void SpaceportsReport::refreshLaunchButtons()
{
	layoutLaunchButtons();

	if (!mOrbitalProgram || !mResources || !mTechnologyCatalog || !mResearchTracker) { return; }

	const auto researchEffects = computeColonyResearchEffects(*mResearchTracker, *mTechnologyCatalog);

	if (const auto* orbitalSurvey = mOrbitalProgram->findDefinition("orbital_survey");
		mOrbitalProgram->canLaunch("orbital_survey", researchEffects))
	{
		btnLaunchOrbitalSurvey.enabled(*mResources >= orbitalSurvey->launchCost);
	}

	if (const auto* deepScan = mOrbitalProgram->findDefinition("deep_scan");
		mOrbitalProgram->canLaunch("deep_scan", researchEffects))
	{
		btnLaunchDeepScan.enabled(*mResources >= deepScan->launchCost);
	}
}


void SpaceportsReport::onLaunchOrbitalSurvey()
{
	if (mLaunchSatelliteHandler) { mLaunchSatelliteHandler("orbital_survey"); }
}


void SpaceportsReport::onLaunchDeepScan()
{
	if (mLaunchSatelliteHandler) { mLaunchSatelliteHandler("deep_scan"); }
}


void SpaceportsReport::update()
{
	if (!visible()) { return; }

	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	draw(renderer);
	ControlContainer::update();
}


void SpaceportsReport::onResize()
{
	layoutLaunchButtons();
}


void SpaceportsReport::draw(NAS2D::Renderer& renderer) const
{
	if (!visible()) { return; }

	const auto reportArea = area();
	const auto layout = buildLayout(reportArea, fontMedium, fontMediumBold, fontBigBold, mOrbitalProgram, mTechnologyCatalog, mResearchTracker);
	const auto textColumn = layout.textOrigin.x;

	renderer.drawText(fontBigBold, "Spaceports Report", NAS2D::Point{textColumn, layout.textOrigin.y}, constants::PrimaryTextColor);
	renderer.drawText(
		fontMedium,
		"Launch satellites from the colony ship's cargo bay.",
		NAS2D::Point{textColumn, layout.subtitleY},
		constants::PrimaryTextColor
	);

	if (!mOrbitalProgram || !mTechnologyCatalog || !mResearchTracker)
	{
		renderer.drawText(fontMedium, "Orbital program unavailable.", NAS2D::Point{textColumn, layout.contentStartY}, constants::PrimaryTextColor);
		return;
	}

	const auto researchEffects = computeColonyResearchEffects(*mResearchTracker, *mTechnologyCatalog);
	std::size_t entryIndex{0};

	for (const auto& definition : ColonyOrbitalProgram::catalog())
	{
		if (!researchEffects.hasSatelliteUnlocked(definition.id)) { continue; }
		if (entryIndex >= layout.satellites.size()) { break; }

		const auto& entry = layout.satellites[entryIndex++];
		auto lineY = entry.blockTop;

		const auto status = mOrbitalProgram->hasLaunched(definition.id) ? "Launched" : "Ready to launch";
		renderer.drawText(fontMediumBold, definition.displayName + " — " + status, NAS2D::Point{textColumn, lineY}, constants::PrimaryTextColor);
		lineY += fontMediumBold.height() + 4;
		renderer.drawText(fontMedium, definition.description, NAS2D::Point{textColumn, lineY}, constants::PrimaryTextColor);
		lineY += fontMedium.height() + 4;
		renderer.drawText(fontMedium, formatLaunchCost(definition.launchCost), NAS2D::Point{textColumn, lineY}, constants::PrimaryTextColor);
	}

	if (researchEffects.unlockedSatellites.empty())
	{
		renderer.drawText(
			fontMedium,
			"Complete Orbital Mechanics to unlock satellite launches.",
			NAS2D::Point{textColumn, layout.footerY},
			constants::PrimaryTextColor
		);
	}
	else if (mOrbitalProgram->launchHistory().size() == researchEffects.unlockedSatellites.size())
	{
		renderer.drawText(
			fontMedium,
			"All unlocked satellites have been launched.",
			NAS2D::Point{textColumn, layout.footerY},
			constants::PrimaryTextColor
		);
	}
}