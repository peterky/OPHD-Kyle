#include "ColonyWarehouseLogistics.h"

#include "StructureManager.h"
#include "MapObjects/Structures/Warehouse.h"

#include <libOPHD/EnumProductType.h>

#include <NAS2D/Utility.h>


int countMaintenancePartsInWarehouses(const StructureManager& structureManager)
{
	int total{0};
	for (const auto* warehouse : structureManager.getStructures<Warehouse>())
	{
		if (!warehouse->operational()) { continue; }
		total += warehouse->products().count(ProductType::PRODUCT_MAINTENANCE_PARTS);
	}
	return total;
}


int pullMaintenancePartFromWarehouses()
{
	const auto& warehouseList = NAS2D::Utility<StructureManager>::get().getStructures<Warehouse>();
	for (auto* warehouse : warehouseList)
	{
		if (!warehouse->operational()) { continue; }

		if (warehouse->products().pull(ProductType::PRODUCT_MAINTENANCE_PARTS, 1) > 0)
		{
			return 1;
		}
	}

	return 0;
}


int storeMaintenancePartsInWarehouses(const int count)
{
	if (count <= 0) { return 0; }

	int stored{0};
	const int storageNeeded = storageRequiredPerUnit(ProductType::PRODUCT_MAINTENANCE_PARTS);

	const auto& warehouseList = NAS2D::Utility<StructureManager>::get().getStructures<Warehouse>();
	for (auto* warehouse : warehouseList)
	{
		if (!warehouse->operational()) { continue; }

		auto& productPool = warehouse->products();
		while (stored < count)
		{
			if (productPool.availableStorage() < storageNeeded) { break; }
			productPool.store(ProductType::PRODUCT_MAINTENANCE_PARTS, 1);
			++stored;
		}

		if (stored >= count) { break; }
	}

	return stored;
}


int countWarehouseRawOre(const StructureManager& structureManager)
{
	int total{0};
	for (const auto* warehouse : structureManager.getStructures<Warehouse>())
	{
		if (!warehouse->operational()) { continue; }
		if (warehouse->rawOreStorageCapacity() <= 0) { continue; }
		total += warehouse->storage().total();
	}
	return total;
}