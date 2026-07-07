#include "MaintenancePriority.h"

#include "MapObjects/Structure.h"

#include <libOPHD/EnumDisabledReason.h>


bool structureNeedsMaintenance(const Structure& structure)
{
	if (structure.destroyed() || structure.underConstruction()) { return false; }

	if (structure.disabled() && structure.disabledReason() == DisabledReason::StructuralIntegrity)
	{
		return true;
	}

	return structure.integrity() < 100;
}


int structureDegradationPercent(const Structure& structure)
{
	return 100 - structure.integrity();
}


int maintenancePriorityScore(const Structure& structure)
{
	if (!structureNeedsMaintenance(structure)) { return -1; }

	auto score = structureDegradationPercent(structure);

	if (structure.disabled() && structure.disabledReason() == DisabledReason::StructuralIntegrity)
	{
		score += 200;
	}

	if (structure.integrity() < MaintenanceHighPriorityIntegrityThreshold)
	{
		score += 100;
	}

	return score;
}