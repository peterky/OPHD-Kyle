#include "Agridome.h"

#include <libOPHD/EnumStructureID.h>
#include <libOPHD/EnumIdleReason.h>
#include <libOPHD/Technology/ColonyResearchEffects.h>

#include <algorithm>


Agridome::Agridome(Tile& tile) :
	FoodProduction{StructureID::Agridome, tile}
{
}


void Agridome::think()
{
	if (isIdle() && idleReason() == IdleReason::InternalStorageFull && !isStorageFull())
	{
		enable();
	}

	if (isIdle()) { return; }

	auto foodProduction = foodProduced();
	if (const auto* researchEffects = activeResearchEffects())
	{
		foodProduction = researchEffects->adjustedFoodProduction(foodProduction);
	}

	mFoodLevel = std::clamp(mFoodLevel + foodProduction, 0, foodStorageCapacity());

	if (isStorageFull())
	{
		idle(IdleReason::InternalStorageFull);
	}
}


bool Agridome::isStorageFull()
{
	return mFoodLevel >= foodStorageCapacity();
}