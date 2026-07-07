#include "OreHaulRoutes.h"

#include "Route.h"
#include "Tile.h"
#include "../StructureManager.h"
#include "../MapObjects/Structure.h"
#include "../MapObjects/Structures/MineFacility.h"
#include "../MapObjects/Structures/Warehouse.h"

#include <libOPHD/StorableResources.h>

#include <map>
#include <algorithm>
#include <stdexcept>


namespace
{
	/**<
	 * The number of times a truck can traverse the shortest possible path
	 * between a mine and a smelter (adjacent to one another). A truck can move 1
	 * unit of ore per trip. The shortest path cost is 0.50f. This number
	 * represents 100 round trips between the mine/smelter for effectively 100
	 * units of ore transported per turn.
	 */
	constexpr int ShortestPathTraversalCount{1000};
	constexpr int LoadUnloadTime{36};
	constexpr int WarehouseToSmelterTransferPerTurn{75};

	StorableResources& rawOrePool(Structure& structure)
	{
		return structure.isSmelter() ? structure.production() : structure.storage();
	}

	bool structureIsOreBuffer(const Structure& structure)
	{
		return structure.isSmelter() ||
			(structure.isWarehouse() && structure.rawOreStorageCapacity() > 0);
	}

	std::map<const MineFacility*, Route> routeTable;
}


OreHaulRoutes::OreHaulRoutes(TileMap& tileMap, const StructureManager& structureManager) :
	mRouteFinder{tileMap},
	mStructureManager{structureManager}
{
}


bool OreHaulRoutes::hasRoute(const MineFacility& mineFacility) const
{
	return routeTable.find(&mineFacility) != routeTable.end();
}


const Route& OreHaulRoutes::getRoute(const MineFacility& mineFacility) const
{
	if (!hasRoute(mineFacility))
	{
		throw std::runtime_error("Called getRoute when no valid route");
	}
	return routeTable.at(&mineFacility);
}


int OreHaulRoutes::getRouteCost(const MineFacility& mineFacility) const
{
	return getRoute(mineFacility).cost;
}


int OreHaulRoutes::getOreHaulCapacity(const MineFacility& mineFacility) const
{
	if (!hasRoute(mineFacility)) { return 0; }
	const auto routeCost = LoadUnloadTime + getRouteCost(mineFacility);
	const auto baseCapacity = ShortestPathTraversalCount * mineFacility.assignedTrucks() / routeCost;

	if (const auto* researchEffects = Structure::activeResearchEffects())
	{
		return researchEffects->adjustedOreHaulCapacity(baseCapacity);
	}

	return baseCapacity;
}


void OreHaulRoutes::removeMine(const MineFacility& mineFacility)
{
	routeTable.erase(&mineFacility);
}


void OreHaulRoutes::clear()
{
	routeTable.clear();
}


void OreHaulRoutes::updateRoutes()
{
	routeTable.clear();

	const auto structureIsSmelter = [](const Structure& structure) { return structure.isSmelter(); };
	const auto structureIsOperableMine = [](const Structure& structure) { return structure.isMineFacility() && structure.isOperable(); };
	const auto& smelters = mStructureManager.getStructures(structureIsSmelter);
	std::vector<Structure*> oreDestinations;
	oreDestinations.insert(oreDestinations.end(), smelters.begin(), smelters.end());
	for (auto* warehouse : mStructureManager.getStructures<Warehouse>())
	{
		if (warehouse->operational() && warehouse->rawOreStorageCapacity() > 0)
		{
			oreDestinations.push_back(warehouse);
		}
	}

	const auto& mines = mStructureManager.getStructures(structureIsOperableMine);
	for (const auto* mineFacility : mines)
	{
		auto newRoute = mRouteFinder.findLowestCostRoute(mineFacility, oreDestinations);
		if (!newRoute.isEmpty()) {
			routeTable[dynamic_cast<const MineFacility*>(mineFacility)] = newRoute;
		}
	}
}


void OreHaulRoutes::transportOreFromMines()
{
	for (const auto* mineFacilityPtr : mStructureManager.getStructures<MineFacility>())
	{
		auto routeIt = routeTable.find(mineFacilityPtr);
		if (routeIt != routeTable.end())
		{
			const auto& route = routeIt->second;
			auto& destination = *route.path.back()->structure();
			auto& mineFacility = dynamic_cast<MineFacility&>(*route.path.front()->structure());

			if (!destination.operational() || !structureIsOreBuffer(destination)) { continue; }

			const int oreHaulCapacity = getOreHaulCapacity(mineFacility);
			const auto movementCap = StorableResources{oreHaulCapacity, oreHaulCapacity, oreHaulCapacity, oreHaulCapacity};

			auto& mineStorage = mineFacility.storage();
			auto& destinationStored = rawOrePool(destination);

			const auto oreAvailable = destinationStored + mineStorage.cap(movementCap);
			const auto newDestinationStored = oreAvailable.cap(destination.rawOreStorageCapacity());
			const auto movedOre = newDestinationStored - destinationStored;

			mineStorage -= movedOre;
			destinationStored = newDestinationStored;
		}
	}
}


void OreHaulRoutes::transportOreFromWarehouses()
{
	const auto structureIsSmelter = [](const Structure& structure) { return structure.isSmelter(); };
	const auto& smelters = mStructureManager.getStructures(structureIsSmelter);
	if (smelters.empty()) { return; }

	for (auto* warehouse : mStructureManager.getStructures<Warehouse>())
	{
		if (!warehouse->operational() || warehouse->rawOreStorageCapacity() <= 0) { continue; }
		if (warehouse->storage().isEmpty()) { continue; }

		const auto route = mRouteFinder.findLowestCostRoute(warehouse, smelters);
		if (route.isEmpty()) { continue; }

		auto& smelter = *route.path.back()->structure();
		if (!smelter.operational()) { continue; }

		const auto movementCap = StorableResources{
			WarehouseToSmelterTransferPerTurn,
			WarehouseToSmelterTransferPerTurn,
			WarehouseToSmelterTransferPerTurn,
			WarehouseToSmelterTransferPerTurn};

		auto& warehouseStorage = warehouse->storage();
		auto& smelterStored = smelter.production();

		const auto oreAvailable = smelterStored + warehouseStorage.cap(movementCap);
		const auto newSmelterStored = oreAvailable.cap(smelter.rawOreStorageCapacity());
		const auto movedOre = newSmelterStored - smelterStored;

		warehouseStorage -= movedOre;
		smelterStored = newSmelterStored;
	}
}


std::vector<Tile*> OreHaulRoutes::getRouteOverlay() const
{
	std::vector<Tile*> routeTiles;
	for (auto& [_, route] : routeTable)
	{
		for (auto* tile : route.path)
		{
			routeTiles.push_back(tile);
		}
	}
	return routeTiles;
}
