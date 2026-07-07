#include "ColonyOrbitalProgram.h"

#include "Map/TileMap.h"

#include <libOPHD/Technology/ColonyResearchEffects.h>

#include <NAS2D/Dictionary.h>
#include <NAS2D/ParserHelper.h>
#include <NAS2D/Xml/XmlElement.h>

#include <algorithm>


namespace
{
	const std::vector<SatelliteDefinition> SatelliteCatalog{
		{
			"orbital_survey",
			"Orbital Survey Satellite",
			"Low-orbit multispectral survey. Reveals ore deposits our initial orbital inspection missed.",
			{200, 150, 100, 0},
			5,
			0,
		},
		{
			"deep_scan",
			"Geological Penetrator Satellite",
			"Ground-penetrating radar from orbit. Reveals deep ore bodies and extends colony-wide digging depth by one level.",
			{400, 300, 200, 0},
			8,
			1,
		},
	};
}


const std::vector<SatelliteDefinition>& ColonyOrbitalProgram::catalog()
{
	return SatelliteCatalog;
}


bool ColonyOrbitalProgram::hasLaunched(const std::string& satelliteId) const
{
	return mLaunchedSatellites.find(satelliteId) != mLaunchedSatellites.end();
}


const SatelliteDefinition* ColonyOrbitalProgram::findDefinition(const std::string& satelliteId) const
{
	const auto found = std::ranges::find_if(SatelliteCatalog, [&](const SatelliteDefinition& definition) {
		return definition.id == satelliteId;
	});

	if (found == SatelliteCatalog.end()) { return nullptr; }
	return &*found;
}


bool ColonyOrbitalProgram::canLaunch(const std::string& satelliteId, const ColonyResearchEffects& researchEffects) const
{
	if (!researchEffects.hasSatelliteUnlocked(satelliteId)) { return false; }
	if (hasLaunched(satelliteId)) { return false; }
	return findDefinition(satelliteId) != nullptr;
}


ColonyOrbitalProgram::LaunchAttempt ColonyOrbitalProgram::attemptLaunch(
	const std::string& satelliteId,
	TileMap& tileMap,
	const StorableResources& availableResources,
	const ColonyResearchEffects& researchEffects)
{
	LaunchAttempt attempt;

	if (!researchEffects.hasSatelliteUnlocked(satelliteId))
	{
		attempt.message = "Research has not unlocked this satellite type.";
		return attempt;
	}

	if (hasLaunched(satelliteId))
	{
		attempt.message = "This satellite has already been launched.";
		return attempt;
	}

	const auto* definition = findDefinition(satelliteId);
	if (!definition)
	{
		attempt.message = "Unknown satellite type.";
		return attempt;
	}

	if (definition->launchCost > availableResources)
	{
		attempt.message = "Insufficient refined resources in storage.";
		return attempt;
	}

	const NAS2D::Point<int> mapCenter{tileMap.size().x / 2, tileMap.size().y / 2};
	const auto mapSpan = std::max(tileMap.size().x, tileMap.size().y);
	auto revealedLocations = tileMap.prospectForOreDeposits(mapCenter, mapSpan, definition->minesRevealed);

	LaunchRecord record;
	record.satelliteId = definition->id;
	record.displayName = definition->displayName;
	record.minesRevealed = static_cast<int>(revealedLocations.size());
	record.digDepthBonus = definition->digDepthBonus;
	record.revealedLocations = std::move(revealedLocations);

	mLaunchedSatellites.insert(definition->id);
	mLaunchHistory.push_back(record);

	attempt.success = true;
	attempt.record = record;

	if (record.minesRevealed > 0)
	{
		attempt.message = record.displayName + " launched. Revealed " + std::to_string(record.minesRevealed) + " ore deposit";
		attempt.message += record.minesRevealed == 1 ? "." : "s.";
	}
	else
	{
		attempt.message = record.displayName + " launched. No additional ore deposits were detected.";
	}

	if (record.digDepthBonus > 0)
	{
		attempt.message += " Colony digging depth increased by " + std::to_string(record.digDepthBonus) + " level";
		attempt.message += record.digDepthBonus == 1 ? "." : "s.";
	}

	return attempt;
}


void ColonyOrbitalProgram::serialize(NAS2D::Xml::XmlElement* root) const
{
	auto* orbitalElement = new NAS2D::Xml::XmlElement("orbital_program");
	root->linkEndChild(orbitalElement);

	for (const auto& satelliteId : mLaunchedSatellites)
	{
		orbitalElement->linkEndChild(NAS2D::dictionaryToAttributes(
			"launched_satellite",
			NAS2D::Dictionary{{{"id", satelliteId}}}
		));
	}

	for (const auto& record : mLaunchHistory)
	{
		auto* launchElement = NAS2D::dictionaryToAttributes(
			"launch",
			NAS2D::Dictionary{{
				{"id", record.satelliteId},
				{"name", record.displayName},
				{"mines", record.minesRevealed},
				{"depth_bonus", record.digDepthBonus},
			}}
		);

		for (const auto& location : record.revealedLocations)
		{
			launchElement->linkEndChild(NAS2D::dictionaryToAttributes(
				"revealed_mine",
				NAS2D::Dictionary{{
					{"x", location.x},
					{"y", location.y},
				}}
			));
		}

		orbitalElement->linkEndChild(launchElement);
	}
}


void ColonyOrbitalProgram::deserialize(NAS2D::Xml::XmlElement* root)
{
	mLaunchedSatellites.clear();
	mLaunchHistory.clear();

	auto* orbitalElement = root->firstChildElement("orbital_program");
	if (!orbitalElement) { return; }

	for (auto* launchedElement = orbitalElement->firstChildElement("launched_satellite"); launchedElement; launchedElement = launchedElement->nextSiblingElement("launched_satellite"))
	{
		mLaunchedSatellites.insert(NAS2D::attributesToDictionary(*launchedElement).get("id"));
	}

	for (auto* launchElement = orbitalElement->firstChildElement("launch"); launchElement; launchElement = launchElement->nextSiblingElement("launch"))
	{
		const auto dictionary = NAS2D::attributesToDictionary(*launchElement);

		LaunchRecord record;
		record.satelliteId = dictionary.get("id");
		record.displayName = dictionary.get("name", record.satelliteId);
		record.minesRevealed = dictionary.get<int>("mines");
		record.digDepthBonus = dictionary.get<int>("depth_bonus");

		for (auto* mineElement = launchElement->firstChildElement("revealed_mine"); mineElement; mineElement = mineElement->nextSiblingElement("revealed_mine"))
		{
			const auto mineDictionary = NAS2D::attributesToDictionary(*mineElement);
			record.revealedLocations.push_back({mineDictionary.get<int>("x"), mineDictionary.get<int>("y")});
		}

		mLaunchHistory.push_back(record);
	}
}