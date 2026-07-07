#pragma once

#include "PopulationTable.h"


class PopulationModel
{
public:
	int birthCount() const { return mBirthCount; }
	int deathCount() const { return mDeathCount; }

	const PopulationTable& getPopulations() const;
	const PopulationTable& populationGrowth() const { return mPopulationGrowth; }
	const PopulationTable& populationDeath() const { return mPopulationDeath; }

	void setPopulationProgress(const PopulationTable& growth, const PopulationTable& death);

	void addPopulation(const PopulationTable& population);
	void removePopulation(const PopulationTable& population);

	int update(int morale, int food, int residences, int universities, int nurseries, int hospitals, float fertilityBonus = 0.0f, float mortalityReduction = 0.0f, float educationEfficiency = 0.0f);

	void starveRate(float rate) { mStarveRate = rate; }

	/** Signed rate: positive converts scientists to workers, negative converts workers to scientists. */
	void setRetrainingRatePerTurn(int rate);
	int retrainingRatePerTurn() const { return mRetrainingRatePerTurn; }

	/** -1 uses automatic university-driven graduate split; otherwise 0-100 percent of students become scientists. */
	void setUniversityScientistPercent(int percent);
	int universityScientistPercent() const { return mUniversityScientistPercent; }

	int lastRetrainedCount() const { return mLastRetrainedCount; }

private:
	PopulationTable spawnRoles(const PopulationTable& growth, const PopulationTable& divisor);
	void spawnPopulation(int morale, int residences, int nurseries, int universities, float fertilityBonus, float educationEfficiency);

	void killRoles(const PopulationTable& divisor);
	void killPopulation(int morale, int nurseries, int hospitals, float mortalityReduction);

	int consumeFood(int food);
	void applyRetraining();


	int mBirthCount{0};
	int mDeathCount{0};
	int mLastRetrainedCount{0};
	int mRetrainingRatePerTurn{0};
	int mUniversityScientistPercent{-1};

	float mStarveRate{0.5f}; /**< Fraction of population that dies during food shortages. */
	std::size_t mStarveRoleIndex{0};

	PopulationTable mPopulation; /**< Current population. */
	PopulationTable mPopulationGrowth; /**< Population growth table. */
	PopulationTable mPopulationDeath; /**< Population death table. */
};
