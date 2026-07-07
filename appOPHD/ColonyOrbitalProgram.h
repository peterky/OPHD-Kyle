#pragma once

#include <libOPHD/StorableResources.h>

#include <NAS2D/Math/Point.h>

#include <string>
#include <set>
#include <vector>


namespace NAS2D
{
	namespace Xml
	{
		class XmlElement;
	}
}

class TileMap;
struct ColonyResearchEffects;


struct SatelliteDefinition
{
	std::string id;
	std::string displayName;
	std::string description;
	StorableResources launchCost;
	int minesRevealed{0};
	int digDepthBonus{0};
};


class ColonyOrbitalProgram
{
public:
	struct LaunchRecord
	{
		std::string satelliteId;
		std::string displayName;
		int minesRevealed{0};
		int digDepthBonus{0};
		std::vector<NAS2D::Point<int>> revealedLocations;
	};

	struct LaunchAttempt
	{
		bool success{false};
		std::string message;
		LaunchRecord record;
	};

	static const std::vector<SatelliteDefinition>& catalog();

	bool hasLaunched(const std::string& satelliteId) const;
	bool canLaunch(const std::string& satelliteId, const ColonyResearchEffects& researchEffects) const;
	const SatelliteDefinition* findDefinition(const std::string& satelliteId) const;

	LaunchAttempt attemptLaunch(
		const std::string& satelliteId,
		TileMap& tileMap,
		const StorableResources& availableResources,
		const ColonyResearchEffects& researchEffects);

	const std::vector<LaunchRecord>& launchHistory() const { return mLaunchHistory; }

	void serialize(NAS2D::Xml::XmlElement* root) const;
	void deserialize(NAS2D::Xml::XmlElement* root);

private:
	std::set<std::string> mLaunchedSatellites;
	std::vector<LaunchRecord> mLaunchHistory;
};