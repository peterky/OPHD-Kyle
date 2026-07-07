#pragma once

#include "../Structure.h"


class Recycling : public Structure
{
public:
	Recycling(Tile& tile);

	int wasteProcessedLastTurn() const { return mWasteProcessedLastTurn; }
	int partsGeneratedLastTurn() const { return mPartsGeneratedLastTurn; }

	void recordTurnResults(int wasteProcessed, int partsGenerated);

private:
	int mWasteProcessedLastTurn{0};
	int mPartsGeneratedLastTurn{0};
};