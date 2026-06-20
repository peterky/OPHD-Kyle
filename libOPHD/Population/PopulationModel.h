#pragma once

#include "PopulationTable.h"


class PopulationModel
{
public:
	int birthCount() const { return mBirthCount; }
	int deathCount() const { return mDeathCount; }

	const PopulationTable& getPopulations() const;

	void addPopulation(const PopulationTable& population);
	void removePopulation(const PopulationTable& population);

	int update(int morale, int food, int residences, int universities, int nurseries, int hospitals, float fertilityBonus = 0.0f, float mortalityReduction = 0.0f, float educationEfficiency = 0.0f);

	void starveRate(float rate) { mStarveRate = rate; }

private:
	PopulationTable spawnRoles(const PopulationTable& growth, const PopulationTable& divisor);
	void spawnPopulation(int morale, int residences, int nurseries, int universities, float fertilityBonus, float educationEfficiency);

	void killRoles(const PopulationTable& divisor);
	void killPopulation(int morale, int nurseries, int hospitals, float mortalityReduction);

	int consumeFood(int food);


	int mBirthCount{0};
	int mDeathCount{0};

	float mStarveRate{0.5f}; /**< Fraction of population that dies during food shortages. */
	std::size_t mStarveRoleIndex{0};

	PopulationTable mPopulation; /**< Current population. */
	PopulationTable mPopulationGrowth; /**< Population growth table. */
	PopulationTable mPopulationDeath; /**< Population death table. */
};
