#pragma once

#include "../Robot.h"


class Roboexplorer : public Robot
{
public:
	Roboexplorer();

protected:
	void onTaskComplete(TileMap& tileMap, StructureManager& structureManager) override;
};