#pragma once

struct ColonyMaintenanceTurnStats
{
	int required{0};
	int repaired{0};
	int pending{0};
	int personnelAssigned{0};
	int personnelCapacity{0};
	int suppliesOnHand{0};
	int suppliesCapacity{0};
	int wasteGenerated{0};
	int wasteProcessing{0};
	int residencesOverflowing{0};
	int recyclingPlantsOperational{0};
	int recyclingPlantsTotal{0};
	int wasteRecycled{0};
	int partsProducedFromWaste{0};
	int partsStoredFromWaste{0};
	int partsLostToWarehouseOverflow{0};
	int warehousePartsOnHand{0};
};