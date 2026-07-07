#pragma once


class StructureManager;


struct TruckDeploymentStats
{
	int available{0};
	int deployed{0};
	int total{0};
};


int getTruckAvailability();
TruckDeploymentStats getTruckDeploymentStats(const StructureManager& structureManager);
int pullTruckFromInventory();
int pushTruckIntoInventory();