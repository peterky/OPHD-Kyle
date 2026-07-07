#include "Roboexplorer.h"

#include "../../Map/Tile.h"
#include "../../Map/TileMap.h"
#include "../Structure.h"

#include <libOPHD/Technology/ColonyResearchEffects.h>


namespace
{
	constexpr int ProspectSearchRadius{4};
	constexpr int BaseExplorerYield{1};
}


Roboexplorer::Roboexplorer() :
	Robot{RobotTypeIndex::Explorer}
{
}


void Roboexplorer::onTaskComplete(TileMap& tileMap, StructureManager& /*structureManager*/)
{
	auto depositCount = BaseExplorerYield;
	if (const auto* researchEffects = Structure::activeResearchEffects())
	{
		depositCount = researchEffects->adjustedExplorerYield(BaseExplorerYield);
	}

	mDiscoveredLocations = tileMap.prospectForOreDeposits(tile().xy(), ProspectSearchRadius, depositCount);
	die();
}