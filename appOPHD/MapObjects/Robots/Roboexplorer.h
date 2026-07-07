#pragma once

#include "../Robot.h"

#include <NAS2D/Math/Point.h>

#include <vector>


class Roboexplorer : public Robot
{
public:
	Roboexplorer();

	bool discoveredDeposit() const { return !mDiscoveredLocations.empty(); }
	int discoveredCount() const { return static_cast<int>(mDiscoveredLocations.size()); }
	const std::vector<NAS2D::Point<int>>& discoveredLocations() const { return mDiscoveredLocations; }

protected:
	void onTaskComplete(TileMap& tileMap, StructureManager& structureManager) override;

private:
	std::vector<NAS2D::Point<int>> mDiscoveredLocations;
};