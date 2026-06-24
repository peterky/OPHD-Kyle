#include "Robodozer.h"

#include "../../Constants/Strings.h"
#include "../../Map/Tile.h"


Robodozer::Robodozer() :
	Robot(RobotTypeIndex::Dozer)
{
}


void Robodozer::bulldozeTile(Tile& tile)
{
	mTileIndex = static_cast<std::size_t>(tile.index());
	tile.bulldoze();
	resetTaskState();
}


void Robodozer::startTask(Tile& tile)
{
	bulldozeTile(tile);
}


void Robodozer::abortTask()
{
	Robot::abortTask();
	tile().index(static_cast<TerrainType>(mTileIndex));
}


void Robodozer::onTaskComplete(TileMap& /*tileMap*/, StructureManager& /*structureManager*/)
{
}
