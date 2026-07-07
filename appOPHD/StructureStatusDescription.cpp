#include "StructureStatusDescription.h"

#include "MapObjects/Structure.h"

#include <libOPHD/EnumDisabledReason.h>
#include <libOPHD/EnumIdleReason.h>

#include <map>


namespace
{
	const std::map<DisabledReason, std::string> disabledReasonTable =
	{
		{DisabledReason::None, "Not Disabled"},
		{DisabledReason::Chap, "CHAP Facility unavailable"},
		{DisabledReason::Disconnected, "Not connected to a Command Center"},
		{DisabledReason::Energy, "Insufficient Energy"},
		{DisabledReason::Population, "Insufficient Population"},
		{DisabledReason::RefinedResources, "Insufficient refined resources"},
		{DisabledReason::StructuralIntegrity, "Structural Integrity is compromised"}
	};

	const std::map<IdleReason, std::string> idleReasonTable =
	{
		{IdleReason::None, "Not Idle"},
		{IdleReason::PlayerSet, "Manually set to Idle"},
		{IdleReason::InternalStorageFull, "Internal storage full — haul trucks cannot move ore away fast enough"},
		{IdleReason::FactoryProductionComplete, "Production complete, awaiting shipment"},
		{IdleReason::FactoryInsufficientResources, "Insufficient resources"},
		{IdleReason::FactoryInsufficientRobotCommandCapacity, "Lack of robot command capacity"},
		{IdleReason::FactoryInsufficientWarehouseSpace, "Lack of Warehouse space"},
		{IdleReason::MineExhausted, "Mine exhausted"},
		{IdleReason::MineInactive, "Mine inactive"},
		{IdleReason::InsufficientLuxuryProduct, "Insufficient Luxury Product"}
	};
}


std::string structureStatusReason(const Structure& structure)
{
	if (structure.disabled())
	{
		return disabledReasonTable.at(structure.disabledReason());
	}

	if (structure.isIdle())
	{
		return idleReasonTable.at(structure.idleReason());
	}

	return {};
}