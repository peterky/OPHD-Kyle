#include "Recycling.h"

#include <libOPHD/EnumStructureID.h>


Recycling::Recycling(Tile& tile) :
	Structure{StructureID::Recycling, tile}
{
}


void Recycling::recordTurnResults(const int wasteProcessed, const int partsGenerated)
{
	mWasteProcessedLastTurn = wasteProcessed;
	mPartsGeneratedLastTurn = partsGenerated;
}