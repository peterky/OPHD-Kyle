#include "ColonyFoodForecast.h"

#include "StructureManager.h"
#include "MapObjects/Structures/FoodProduction.h"

#include <libOPHD/Population/PopulationModel.h>


int foodConsumptionPerTurn(const std::size_t population)
{
	constexpr int PopulationPerFood = 10;
	return static_cast<int>((population + (PopulationPerFood - 1)) / PopulationPerFood);
}


ColonyFoodForecast computeColonyFoodForecast(
	const StructureManager& structureManager,
	const PopulationModel& population,
	const ColonyResearchEffects& researchEffects)
{
	ColonyFoodForecast forecast;
	forecast.consumptionPerTurn = foodConsumptionPerTurn(population.getPopulations().size());

	for (const auto* foodProducer : structureManager.getStructures<FoodProduction>())
	{
		if (!foodProducer->isOperable()) { continue; }
		forecast.productionPerTurn += researchEffects.adjustedFoodProduction(foodProducer->foodProduced());
	}

	forecast.netPerTurn = forecast.productionPerTurn - forecast.consumptionPerTurn;
	return forecast;
}


bool wouldStarveWithinTurns(const int food, const int productionPerTurn, const int consumptionPerTurn, const int turns)
{
	if (turns <= 0 || consumptionPerTurn <= 0) { return false; }

	int projectedFood = food;
	for (int turn = 0; turn < turns; ++turn)
	{
		if (projectedFood < consumptionPerTurn) { return true; }
		projectedFood += productionPerTurn - consumptionPerTurn;
	}

	return false;
}