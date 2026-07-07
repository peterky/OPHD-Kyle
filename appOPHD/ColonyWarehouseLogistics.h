#pragma once


class StructureManager;


int countMaintenancePartsInWarehouses(const StructureManager& structureManager);
int pullMaintenancePartFromWarehouses();
int storeMaintenancePartsInWarehouses(int count);
int countWarehouseRawOre(const StructureManager& structureManager);