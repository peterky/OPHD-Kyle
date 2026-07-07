#include "TruckAvailability.h"

#include "StructureManager.h"
#include "MapObjects/Structures/MineFacility.h"
#include "MapObjects/Structures/Warehouse.h"

#include <libOPHD/EnumProductType.h>

#include <NAS2D/Utility.h>


int getTruckAvailability()
{
	return getTruckDeploymentStats(NAS2D::Utility<StructureManager>::get()).available;
}


TruckDeploymentStats getTruckDeploymentStats(const StructureManager& structureManager)
{
	TruckDeploymentStats stats;

	for (auto* warehouse : structureManager.getStructures<Warehouse>())
	{
		stats.available += warehouse->products().count(ProductType::PRODUCT_TRUCK);
	}

	for (auto* mineFacility : structureManager.getStructures<MineFacility>())
	{
		stats.deployed += mineFacility->assignedTrucks();
	}

	stats.total = stats.available + stats.deployed;
	return stats;
}


/**
 * \return 1 on success, 0 otherwise.
 */
int pullTruckFromInventory()
{
	int trucksAvailable = getTruckAvailability();

	if (trucksAvailable == 0) { return 0; }

	const auto& warehouseList = NAS2D::Utility<StructureManager>::get().getStructures<Warehouse>();
	for (auto* warehouse : warehouseList)
	{
		if (warehouse->products().pull(ProductType::PRODUCT_TRUCK, 1) > 0)
		{
			return 1;
		}
	}

	return 0;
}


/**
 * \return 1 on success, 0 otherwise.
 */
int pushTruckIntoInventory()
{
	const int storageNeededForTruck = storageRequiredPerUnit(ProductType::PRODUCT_TRUCK);

	const auto& warehouseList = NAS2D::Utility<StructureManager>::get().getStructures<Warehouse>();
	for (auto* warehouse : warehouseList)
	{
		if (warehouse->products().availableStorage() >= storageNeededForTruck)
		{
			warehouse->products().store(ProductType::PRODUCT_TRUCK, 1);
			return 1;
		}
	}

	return 0;
}
