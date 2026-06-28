#pragma once

#include "../StorableResources.h"

#include <set>
#include <string>


class ResearchTracker;
class TechnologyCatalog;


struct ColonyResearchEffects
{
	float maintenanceCostReduction{0.0f};
	float structureDecayReduction{0.0f};
	float breakdownReduction{0.0f};
	bool systemsAnalysisCompleted{false};

	float agricultureEfficiency{0.0f};
	float explorerEfficiency{0.0f};
	float educationEfficiency{0.0f};
	float recyclingEfficiency{0.0f};
	float smelterEfficiency{0.0f};
	float miningEfficiency{0.0f};
	float oreHaulEfficiency{0.0f};
	float structureCostReduction{0.0f};
	float populationFertilityBonus{0.0f};
	float populationMoraleBonus{0.0f};
	float populationMortalityReduction{0.0f};

	float factoryProductionSpeedBonus{0.0f};
	float researchSpeedBonus{0.0f};
	float robotTaskSpeedBonus{0.0f};
	float commRangeBonus{0.0f};
	float solarEnergyBonus{0.0f};
	float robotCommandCapacityBonus{0.0f};

	bool explorerBotUnlocked{false};
	bool commTowerBuildRange{false};
	bool leggedRobotsUnlocked{false};
	bool plagueImmunity{false};
	std::set<std::string> disasterPredictions;

	float maintenanceCostMultiplier() const;
	float structureCostMultiplier() const;
	int effectiveIntegrityDecay(int baseDecayRate) const;
	int breakdownChancePercent() const;
	int maintenanceRepairPasses(int personnel) const;
	int moraleBonusPerTurn() const;
	int recyclingRecoveryPercent(int basePercent) const;
	float educationConversionRate(float baseRate) const;
	StorableResources adjustedBuildCost(const StorableResources& baseCost) const;
	StorableResources adjustedRecyclingValue(const StorableResources& baseValue) const;
	int adjustedFactoryTurns(int baseTurns) const;
	int adjustedMineProductionRate(int baseRate) const;
	int adjustedOreHaulCapacity(int baseCapacity) const;
	int adjustedSmeltingOreThreshold(int baseThreshold) const;
	int adjustedSmeltingOutput(int baseOutput) const;
	int adjustedResearchPoints(int basePoints) const;
	int adjustedRobotTaskTurns(int baseTurns) const;
	int adjustedCommRange(int baseRange) const;
	int adjustedSolarOutput(int baseOutput) const;
	int adjustedRobotCommandCapacity(int baseCapacity) const;
	int adjustedBioWasteProcessing(int baseCapacity) const;
	int adjustedFoodProduction(int baseProduction) const;
	int adjustedExplorerYield(int baseYield) const;
	bool hasDisasterPrediction(const std::string& disasterType) const;
};


ColonyResearchEffects computeColonyResearchEffects(const ResearchTracker& tracker, const TechnologyCatalog& catalog);