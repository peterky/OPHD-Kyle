#pragma once

#include <libOPHD/Technology/ColonyResearchEffects.h>


class StructureManager;
class PopulationModel;


struct ColonyFoodForecast
{
	int productionPerTurn{0};
	int consumptionPerTurn{0};
	int netPerTurn{0};
};


ColonyFoodForecast computeColonyFoodForecast(
	const StructureManager& structureManager,
	const PopulationModel& population,
	const ColonyResearchEffects& researchEffects);

int foodConsumptionPerTurn(std::size_t population);

bool wouldStarveWithinTurns(int food, int productionPerTurn, int consumptionPerTurn, int turns);