#include "Robot.h"

#include "RobotType.h"
#include "../Constants/Strings.h"
#include "../Map/Tile.h"
#include "Structure.h"

#include <libOPHD/EnumTerrainType.h>
#include <libOPHD/Map/MapCoordinate.h>
#include <libOPHD/Technology/ColonyResearchEffects.h>

#include <NAS2D/Dictionary.h>

#include <algorithm>
#include <array>


namespace
{
	constexpr int MinerTaskTime{6};
	constexpr int DiggerTaskTime{5};
	constexpr int ExplorerTaskTime{3};

	const std::array robotTypes{
		RobotType{constants::Robodigger, "robots/robodigger.sprite", "ui/interface/product_robodigger.png", DiggerTaskTime},
		RobotType{constants::Robodozer, "robots/robodozer.sprite", "ui/interface/product_robodozer.png", 0},
		RobotType{constants::Robominer, "robots/robominer.sprite", "ui/interface/product_robominer.png", MinerTaskTime},
		RobotType{constants::Roboexplorer, "robots/roboexplorer.sprite", "ui/interface/product_roboexplorer.png", ExplorerTaskTime},
	};

	int getTaskTime(RobotTypeIndex robotTypeIndex, Tile& tile)
	{
		const auto baseTaskTime = std::max(1, Robot::robotType(robotTypeIndex).basicTaskTime + static_cast<int>(tile.index()));
		int taskTime = baseTaskTime;
		if (const auto* researchEffects = Structure::activeResearchEffects())
		{
			taskTime = researchEffects->adjustedRobotTaskTurns(baseTaskTime);
		}
		return Robot::clampTaskTurns(robotTypeIndex, taskTime);
	}
}


int Robot::maxTaskTurns(RobotTypeIndex robotTypeIndex)
{
	constexpr auto maxTerrainIndex = static_cast<int>(TerrainType::Impassable);
	return std::max(1, robotType(robotTypeIndex).basicTaskTime + maxTerrainIndex);
}


int Robot::clampTaskTurns(RobotTypeIndex robotTypeIndex, int turns)
{
	return std::clamp(turns, 1, maxTaskTurns(robotTypeIndex));
}


void Robot::sanitizeTaskTurns()
{
	if (mTurnsToCompleteTask > 0)
	{
		mTurnsToCompleteTask = clampTaskTurns(mRobotTypeIndex, mTurnsToCompleteTask);
	}
}


const RobotType& Robot::robotType(RobotTypeIndex robotTypeIndex)
{
	return robotTypes.at(static_cast<std::size_t>(robotTypeIndex));
}


Robot::Robot(RobotTypeIndex robotTypeIndex) :
	Robot{robotTypeIndex, robotType(robotTypeIndex)}
{}


Robot::Robot(RobotTypeIndex robotTypeIndex, const RobotType& robotType) :
	MapObject(robotType.spritePath, "running"),
	mRobotType{robotType},
	mRobotTypeIndex{robotTypeIndex}
{}


const RobotType& Robot::robotType() const
{
	return mRobotType;
}


const std::string& Robot::name() const
{
	return mRobotType.name;
}


void Robot::die()
{
	mIsDead = true;
}


/// Robot is dead and should be cleaned up.
bool Robot::isDead() const
{
	return mIsDead;
}


void Robot::startTask(Tile& tile)
{
	startTask(tile, getTaskTime(mRobotTypeIndex, tile));
}


void Robot::startTask(Tile& tile, int turns)
{
	mTurnsToCompleteTask = clampTaskTurns(mRobotTypeIndex, turns);
	mTile = &tile;
}


void Robot::abortTask()
{
}


bool Robot::isPlaced() const
{
	return mTurnsToCompleteTask > 0 && mTile;
}


Tile& Robot::tile()
{
	if (!mTile) { throw std::runtime_error("Robot must be placed"); }
	return *mTile;
}


const Tile& Robot::tile() const
{
	if (!mTile) { throw std::runtime_error("Robot must be placed"); }
	return *mTile;
}


MapCoordinate Robot::mapCoordinate() const
{
	if (!mTile) { throw std::runtime_error("Robot must be placed to have a mapCoordinate"); }
	return mTile->xyz();
}


void Robot::taskCompleteHandler(TaskCompleteDelegate newTaskCompleteHandler)
{
	mTaskCompleteHandler = newTaskCompleteHandler;
}


NAS2D::Dictionary Robot::getDataDict() const
{
	auto dictionary = NAS2D::Dictionary{{
		{"type", static_cast<int>(mRobotTypeIndex)},
		{"age", mFuelCellAge},
		{"production", mTurnsToCompleteTask},
	}};

	if (isPlaced())
	{
		const auto location = mapCoordinate();
		dictionary += NAS2D::Dictionary{{
			{"x", location.xy.x},
			{"y", location.xy.y},
			{"depth", location.z},
		}};
	}

	return dictionary;
}


void Robot::processTurn(TileMap& tileMap, StructureManager& structureManager)
{
	if (mSelfDestruct)
	{
		die();
		return;
	}

	if (mCancelTask)
	{
		mTurnsToCompleteTask = 0;
		return;
	}

	mTurnsToCompleteTask--;

	if (mTurnsToCompleteTask == 0)
	{
		onTaskComplete(tileMap, structureManager);
		if (mTaskCompleteHandler) { mTaskCompleteHandler(*this); }
	}

	mFuelCellAge++;

	if (mFuelCellAge == 200)
	{
		die();
	}
}
