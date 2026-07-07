#include "ColonyResearchEffects.h"

#include "ResearchTracker.h"
#include "Technology.h"
#include "TechnologyCatalog.h"

#include <algorithm>
#include <cmath>


namespace
{
	float sumCompletedModifier(const ResearchTracker& tracker, const TechnologyCatalog& catalog, Technology::Modifier::Modifies modifies)
	{
		float total{0.0f};

		for (const auto techId : tracker.completedResearch())
		{
			const auto* technology = catalog.findTechnologyFromId(techId);
			if (!technology) { continue; }

			for (const auto& modifier : technology->modifiers)
			{
				if (modifier.modifies == modifies)
				{
					total += modifier.value;
				}
			}
		}

		return total;
	}


	void applyUnlockEffect(const Technology::Unlock& unlock, ColonyResearchEffects& effects)
	{
		if (unlock.unlocks == Technology::Unlock::Unlocks::Satellite)
		{
			effects.unlockedSatellites.insert(unlock.value);
			return;
		}

		if (unlock.unlocks == Technology::Unlock::Unlocks::DisasterPrediction)
		{
			effects.disasterPredictions.insert(unlock.value);

			if (unlock.value == "plague_immunity" || unlock.value == "no_more_plagues_placeholder")
			{
				effects.plagueImmunity = true;
				effects.populationMortalityReduction += 0.05f;
			}

			return;
		}

		if (unlock.unlocks != Technology::Unlock::Unlocks::Robot &&
			unlock.unlocks != Technology::Unlock::Unlocks::Structure)
		{
			return;
		}

		if (unlock.value == "CommTower2") { effects.commRangeBonus += 0.25f; }
		else if (unlock.value == "CommTower3") { effects.commRangeBonus += 0.35f; }
		else if (unlock.value == "Smelter2") { effects.smelterEfficiency += 0.20f; }
		else if (unlock.value == "SolarPanel2") { effects.solarEnergyBonus += 0.25f; }
		else if (unlock.value == "RobotCommand2") { effects.robotCommandCapacityBonus += 0.50f; }
		else if (unlock.value == "factory_efficiency_upgrade" || unlock.value == "placeholder_factory_production_upgrade") { effects.factoryProductionSpeedBonus += 0.25f; }
		else if (unlock.value == "research_efficiency_upgrade" || unlock.value == "placeholder_research_speed_upgrade") { effects.researchSpeedBonus += 0.25f; }
		else if (unlock.value == "robot_vision_upgrade" || unlock.value == "placeholder_robots_work_faster") { effects.robotTaskSpeedBonus += 0.25f; }
		else if (unlock.value == "robot_power_refit" || unlock.value == "placeholder_faster_robots") { effects.robotTaskSpeedBonus += 0.15f; }
		else if (unlock.value == "legged_robots_clearance" || unlock.value == "placeholder_spider_team_can_clear_mountains?") { effects.leggedRobotsUnlocked = true; effects.robotTaskSpeedBonus += 0.10f; }
		else if (unlock.value == "legged_robots_rough_terrain" || unlock.value == "placeholder_or_maybe_let_dozers_do_that?") { effects.leggedRobotsUnlocked = true; effects.robotTaskSpeedBonus += 0.10f; }
		else if (unlock.value == "humanoid_worker") { effects.factoryProductionSpeedBonus += 0.10f; }
		else if (unlock.value == "RID_EXPLORER_BOT") { effects.explorerBotUnlocked = true; }
		else if (unlock.value == "comm_tower_build_range") { effects.commTowerBuildRange = true; }
	}


	void applyCompletedUnlocks(const ResearchTracker& tracker, const TechnologyCatalog& catalog, ColonyResearchEffects& effects)
	{
		for (const auto techId : tracker.completedResearch())
		{
			const auto* technology = catalog.findTechnologyFromId(techId);
			if (!technology) { continue; }

			for (const auto& unlock : technology->unlocks)
			{
				applyUnlockEffect(unlock, effects);
			}
		}
	}


	constexpr int SystemsAnalysisTechId{5001};
}


float ColonyResearchEffects::maintenanceCostMultiplier() const
{
	return std::max(0.0f, 1.0f - std::min(maintenanceCostReduction, 1.0f));
}


float ColonyResearchEffects::structureCostMultiplier() const
{
	return std::max(0.0f, 1.0f - std::min(structureCostReduction, 1.0f));
}


int ColonyResearchEffects::effectiveIntegrityDecay(int baseDecayRate) const
{
	if (baseDecayRate <= 0) { return 0; }

	const auto reduction = std::min(structureDecayReduction, 1.0f);
	return std::max(0, static_cast<int>(baseDecayRate * (1.0f - reduction)));
}


int ColonyResearchEffects::breakdownChancePercent() const
{
	constexpr int BaseBreakdownChance{10};
	const auto reduction = std::min(breakdownReduction, 1.0f);
	return std::max(0, static_cast<int>(BaseBreakdownChance * (1.0f - reduction) + 0.5f));
}


int ColonyResearchEffects::maintenanceRepairPasses(int personnel) const
{
	const auto extraPasses = systemsAnalysisCompleted ? std::max(1, personnel) : 0;
	return 1 + extraPasses;
}


int ColonyResearchEffects::moraleBonusPerTurn() const
{
	return std::max(0, static_cast<int>(populationMoraleBonus * 100.0f + 0.5f));
}


int ColonyResearchEffects::recyclingRecoveryPercent(int basePercent) const
{
	const auto bonus = std::min(recyclingEfficiency, 1.0f);
	return std::min(100, static_cast<int>(basePercent * (1.0f + bonus) + 0.5f));
}


float ColonyResearchEffects::educationConversionRate(float baseRate) const
{
	return baseRate * (1.0f + std::min(educationEfficiency, 1.0f));
}


StorableResources ColonyResearchEffects::adjustedBuildCost(const StorableResources& baseCost) const
{
	const auto multiplier = structureCostMultiplier();
	StorableResources adjusted;

	for (std::size_t i = 0; i < adjusted.resources.size(); ++i)
	{
		adjusted.resources[i] = std::max(0, static_cast<int>(std::ceil(baseCost.resources[i] * multiplier)));
	}

	return adjusted;
}


StorableResources ColonyResearchEffects::adjustedRecyclingValue(const StorableResources& baseValue) const
{
	const auto recoveryPercent = recyclingRecoveryPercent(100);
	StorableResources adjusted;

	for (std::size_t i = 0; i < adjusted.resources.size(); ++i)
	{
		adjusted.resources[i] = baseValue.resources[i] * recoveryPercent / 100;
	}

	return adjusted;
}


int ColonyResearchEffects::adjustedFactoryTurns(int baseTurns) const
{
	const auto speedBonus = std::min(factoryProductionSpeedBonus, 0.90f);
	return std::max(1, static_cast<int>(baseTurns * (1.0f - speedBonus) + 0.5f));
}


int ColonyResearchEffects::adjustedMineProductionRate(int baseRate) const
{
	const auto bonus = std::min(miningEfficiency, 0.50f);
	return std::max(1, static_cast<int>(baseRate * (1.0f + bonus) + 0.5f));
}


int ColonyResearchEffects::adjustedOreHaulCapacity(int baseCapacity) const
{
	const auto bonus = std::min(oreHaulEfficiency, 1.0f);
	return std::max(0, static_cast<int>(baseCapacity * (1.0f + bonus) + 0.5f));
}


int ColonyResearchEffects::adjustedSmeltingOreThreshold(int baseThreshold) const
{
	const auto bonus = std::min(smelterEfficiency, 0.90f);
	return std::max(1, static_cast<int>(baseThreshold * (1.0f - bonus) + 0.5f));
}


int ColonyResearchEffects::adjustedSmeltingOutput(int baseOutput) const
{
	const auto bonus = std::min(smelterEfficiency, 2.50f);
	return std::max(1, static_cast<int>(baseOutput * (1.0f + bonus) + 0.5f));
}


int ColonyResearchEffects::adjustedResearchPoints(int basePoints) const
{
	const auto speedBonus = std::min(researchSpeedBonus, 2.0f);
	return static_cast<int>(basePoints * (1.0f + speedBonus));
}


int ColonyResearchEffects::adjustedRobotTaskTurns(int baseTurns) const
{
	const auto speedBonus = std::clamp(robotTaskSpeedBonus + (leggedRobotsUnlocked ? 0.05f : 0.0f), 0.0f, 0.90f);
	return std::max(1, static_cast<int>(baseTurns * (1.0f - speedBonus) + 0.5f));
}


int ColonyResearchEffects::adjustedCommRange(int baseRange) const
{
	if (baseRange <= 0) { return 0; }

	const auto bonus = std::min(commRangeBonus, 2.0f);
	return static_cast<int>(baseRange * (1.0f + bonus) + 0.5f);
}


int ColonyResearchEffects::adjustedSolarOutput(int baseOutput) const
{
	if (baseOutput <= 0) { return 0; }

	const auto bonus = std::min(solarEnergyBonus, 2.0f);
	return static_cast<int>(baseOutput * (1.0f + bonus) + 0.5f);
}


int ColonyResearchEffects::adjustedRobotCommandCapacity(int baseCapacity) const
{
	if (baseCapacity <= 0) { return 0; }

	const auto bonus = std::min(robotCommandCapacityBonus, 2.0f);
	return static_cast<int>(baseCapacity * (1.0f + bonus) + 0.5f);
}


int ColonyResearchEffects::adjustedBioWasteProcessing(int baseCapacity) const
{
	if (baseCapacity <= 0) { return 0; }

	const auto bonus = std::min(recyclingEfficiency, 1.0f);
	return static_cast<int>(baseCapacity * (1.0f + bonus) + 0.5f);
}


int ColonyResearchEffects::adjustedFoodProduction(int baseProduction) const
{
	if (baseProduction <= 0) { return 0; }

	const auto bonus = std::min(agricultureEfficiency, 2.0f);
	return std::max(1, static_cast<int>(baseProduction * (1.0f + bonus) + 0.5f));
}


int ColonyResearchEffects::adjustedExplorerYield(int baseYield) const
{
	if (baseYield <= 0) { return 1; }

	const auto bonus = std::min(explorerEfficiency, 2.0f);
	return std::max(1, static_cast<int>(baseYield * (1.0f + bonus) + 0.5f));
}


bool ColonyResearchEffects::hasDisasterPrediction(const std::string& disasterType) const
{
	return disasterPredictions.find(disasterType) != disasterPredictions.end();
}


bool ColonyResearchEffects::hasSatelliteUnlocked(const std::string& satelliteType) const
{
	return unlockedSatellites.find(satelliteType) != unlockedSatellites.end();
}


ColonyResearchEffects computeColonyResearchEffects(const ResearchTracker& tracker, const TechnologyCatalog& catalog)
{
	ColonyResearchEffects effects;
	effects.maintenanceCostReduction = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::MaintenanceCost);
	effects.structureDecayReduction = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::StructureDecay);
	effects.breakdownReduction = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::BreakdownRate);
	effects.agricultureEfficiency = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::AgricultureEfficiency);
	effects.explorerEfficiency = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::ExplorerEfficiency);
	effects.educationEfficiency = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::EducationEfficiency);
	effects.recyclingEfficiency = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::RecyclingEfficiency);
	effects.smelterEfficiency = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::SmelterEfficiency);
	effects.miningEfficiency = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::MiningEfficiency);
	effects.oreHaulEfficiency = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::OreHaulEfficiency);
	effects.structureCostReduction = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::StructureCost);
	effects.populationFertilityBonus = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::PopulationFertility);
	effects.populationMoraleBonus = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::PopulationMorale);
	effects.populationMortalityReduction = sumCompletedModifier(tracker, catalog, Technology::Modifier::Modifies::PopulationMortality);
	effects.systemsAnalysisCompleted = tracker.isCompleted(SystemsAnalysisTechId);
	applyCompletedUnlocks(tracker, catalog, effects);
	return effects;
}