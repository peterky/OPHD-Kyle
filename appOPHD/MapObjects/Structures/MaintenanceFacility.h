#pragma once

#include "../Structure.h"

#include <vector>


struct StorableResources;

using StructureList = std::vector<Structure*>;


class MaintenanceFacility : public Structure
{
public:
	MaintenanceFacility(Tile& tile);

	void resources(const StorableResources& resources);
	bool suppliesAvailable() const;

	int maintenancePersonnel() const;
	void addPersonnel();
	void removePersonnel();
	int personnel() const;
	void personnel(int assigned);
	bool personnelAvailable() const;

	void addPriorityStructure(Structure* structure);
	void removePriorityStructure(Structure* structure);
	bool hasPriorityStructures() const;

	void repairStructures(StructureList& structures);

	void beginMaintenanceTurn();
	int repairsThisTurn() const { return mRepairsThisTurn; }
	int assignedPersonnelThisTurn() const { return mAssignedPersonnel; }
	int materialsLevel() const { return mMaterialsLevel; }
	static constexpr int materialsCapacity() { return 100; }

protected:
	bool canMakeRepairs() const;

	void moveStructureToBack(StructureList& structures, Structure* structure);

	void repairPriorityStructures(StructureList& structures);
	void repairStructure(Structure* structure);

	void think() override;

private:
	const StorableResources& resources();

	int mMaterialsLevel;
	int mMaintenancePersonnel;
	int mAssignedPersonnel;
	int mRepairsThisTurn{0};
	std::size_t mRoundRobinIndex{0};

	StructureList mPriorityList;

	const StorableResources* mResources;
};
