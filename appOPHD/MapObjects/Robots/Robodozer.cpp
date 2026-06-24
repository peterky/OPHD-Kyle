#include "Robodozer.h"

#include "../../Constants/Strings.h"
#include "../../Map/Tile.h"
#include "../Structure.h"

#include <libOPHD/EnumTerrainType.h>
#include <libOPHD/Technology/ColonyResearchEffects.h>


namespace
{
	int dozerTaskTurnsForTerrain(TerrainType terrain)
	{
		switch (terrain)
		{
		case TerrainType::Clear:
			return 1;
		case TerrainType::Rough:
			return 2;
		case TerrainType::Difficult:
		case TerrainType::Impassable:
			return 3;
		default:
			return 1;
		}
	}


	int adjustedDozerTaskTurns(Tile& tile)
	{
		auto turns = dozerTaskTurnsForTerrain(tile.index());
		if (const auto* researchEffects = Structure::activeResearchEffects())
		{
			turns = researchEffects->adjustedRobotTaskTurns(turns);
		}
		return Robot::clampTaskTurns(RobotTypeIndex::Dozer, turns);
	}
}


Robodozer::Robodozer() :
	Robot(RobotTypeIndex::Dozer)
{
}


void Robodozer::startTask(Tile& tile)
{
	mTileIndex = static_cast<std::size_t>(tile.index());
	Robot::startTask(tile, adjustedDozerTaskTurns(tile));
	tile.bulldoze();
}


void Robodozer::startTask(Tile& tile, int turns)
{
	mTileIndex = static_cast<std::size_t>(tile.index());
	Robot::startTask(tile, Robot::clampTaskTurns(RobotTypeIndex::Dozer, turns));
	tile.bulldoze();
}


void Robodozer::abortTask()
{
	Robot::abortTask();
	tile().index(static_cast<TerrainType>(mTileIndex));
}


void Robodozer::onTaskComplete(TileMap& /*tileMap*/, StructureManager& /*structureManager*/)
{
}