#pragma once

class Structure;


/** Integrity below this threshold is treated as high-priority maintenance (65%+ degraded). */
constexpr int MaintenanceHighPriorityIntegrityThreshold{35};


bool structureNeedsMaintenance(const Structure& structure);
int structureDegradationPercent(const Structure& structure);
int maintenancePriorityScore(const Structure& structure);