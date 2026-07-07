#pragma once

#include "FoodProduction.h"


class Agridome : public FoodProduction
{
public:
	Agridome(Tile& tile);

protected:
	void think() override;

private:
	bool isStorageFull();
};
