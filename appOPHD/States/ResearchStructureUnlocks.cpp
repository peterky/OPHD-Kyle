#include "ResearchStructureUnlocks.h"

#include "StructureTracker.h"

#include <libOPHD/EnumStructureID.h>
#include <libOPHD/Technology/ResearchTracker.h>
#include <libOPHD/Technology/TechnologyCatalog.h>

#include <map>
#include <set>


namespace
{
	const std::map<std::string, StructureID> StructureIdFromString =
	{
		{"Agridome", StructureID::Agridome},
		{"Chap", StructureID::Chap},
		{"CommTower", StructureID::CommTower},
		{"CommTower2", StructureID::CommTower},
		{"CommTower3", StructureID::CommTower},
		{"Commercial", StructureID::Commercial},
		{"FusionReactor", StructureID::FusionReactor},
		{"HotLaboratory", StructureID::HotLaboratory},
		{"Laboratory", StructureID::Laboratory},
		{"MaintenanceFacility", StructureID::MaintenanceFacility},
		{"MedicalCenter", StructureID::MedicalCenter},
		{"Nursery", StructureID::Nursery},
		{"Park", StructureID::Park},
		{"RecreationCenter", StructureID::RecreationCenter},
		{"Recycling", StructureID::Recycling},
		{"RedLightDistrict", StructureID::RedLightDistrict},
		{"Residence", StructureID::Residence},
		{"RobotCommand", StructureID::RobotCommand},
		{"RobotCommand2", StructureID::RobotCommand},
		{"Smelter", StructureID::Smelter},
		{"Smelter2", StructureID::Smelter},
		{"SolarPanel1", StructureID::SolarPanel1},
		{"SolarPanel2", StructureID::SolarPanel1},
		{"SolarPlant", StructureID::SolarPlant},
		{"StorageTanks", StructureID::StorageTanks},
		{"SurfaceFactory", StructureID::SurfaceFactory},
		{"SurfacePolice", StructureID::SurfacePolice},
		{"UndergroundFactory", StructureID::UndergroundFactory},
		{"UndergroundPolice", StructureID::UndergroundPolice},
		{"University", StructureID::University},
		{"Warehouse", StructureID::Warehouse},
	};


	const std::set<StructureID> UndergroundStructures =
	{
		StructureID::Commercial,
		StructureID::Laboratory,
		StructureID::MedicalCenter,
		StructureID::Nursery,
		StructureID::Park,
		StructureID::RecreationCenter,
		StructureID::RedLightDistrict,
		StructureID::Residence,
		StructureID::UndergroundFactory,
		StructureID::UndergroundPolice,
		StructureID::University,
	};
}


void applyStructureUnlocksFromTechnology(StructureTracker& tracker, const Technology& technology)
{
	for (const auto& unlock : technology.unlocks)
	{
		if (unlock.unlocks != Technology::Unlock::Unlocks::Structure) { continue; }

		const auto structureIt = StructureIdFromString.find(unlock.value);
		if (structureIt == StructureIdFromString.end()) { continue; }

		const auto structureId = structureIt->second;
		if (UndergroundStructures.find(structureId) != UndergroundStructures.end())
		{
			tracker.addUnlockedUndergroundStructure(structureId);
		}
		else
		{
			tracker.addUnlockedSurfaceStructure(structureId);
		}
	}
}


void syncStructureTrackerFromCompletedResearch(
	StructureTracker& tracker,
	const ResearchTracker& research,
	const TechnologyCatalog& catalog)
{
	for (const auto techId : research.completedResearch())
	{
		const auto* technology = catalog.findTechnologyFromId(techId);
		if (!technology) { continue; }

		applyStructureUnlocksFromTechnology(tracker, *technology);
	}
}