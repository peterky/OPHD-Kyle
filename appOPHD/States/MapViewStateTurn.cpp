// ==================================================================================
// = This file implements the functions that handle processing a turn.
// ==================================================================================

#include "MapViewState.h"
#include "MapViewStateHelper.h"
#include "ColonyShip.h"


#include "../Map/OreHaulRoutes.h"
#include "../Map/Route.h"
#include "../Map/RouteFinder.h"
#include "../Map/TileMap.h"

#include "../MapObjects/StructureState.h"
#include "../MapObjects/Structures/CommandCenter.h"
#include "../MapObjects/Structures/Factory.h"
#include <libOPHD/EnumProductType.h>
#include <libOPHD/Technology/Technology.h>
#include <libOPHD/EnumProductType.h>
#include <libOPHD/Technology/Technology.h>
#include "../MapObjects/Structures/MineFacility.h"
#include "../MapObjects/Structures/MaintenanceFacility.h"
#include "../MapObjects/Structures/Recycling.h"
#include "../MapObjects/Structures/ResearchFacility.h"
#include "../MapObjects/Structures/Residence.h"
#include "../MapObjects/Structures/Road.h"
#include "../MapObjects/Structures/Warehouse.h"

#include "../CacheImage.h"
#include "../MoraleString.h"
#include "../StructureManager.h"
#include "../Constants/Strings.h"

#include <libOPHD/DirectionOffset.h>
#include <libOPHD/EnumDifficulty.h>
#include <libOPHD/EnumMoraleIndex.h>
#include <libOPHD/EnumIdleReason.h>
#include <libOPHD/StorableResources.h>
#include <libOPHD/MapObjects/StructureType.h>
#include <libOPHD/Population/MoraleChangeEntry.h>
#include <libOPHD/Technology/ColonyResearchEffects.h>
#include <libOPHD/Technology/ColonyResearchEffects.h>
#include <libOPHD/Technology/TechnologyCatalog.h>
#include <libOPHD/Technology/TechnologyCatalog.h>

#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Renderer.h>

#include <vector>
#include <algorithm>
#include <cfloat>
#include <set>
#include <sstream>
#include <set>
#include <sstream>


namespace
{
	const std::map<std::string, StructureID> StructureIdFromString =
	{
		{"Agridome", StructureID::Agridome},
		{"Chap", StructureID::Chap},
		{"CommTower", StructureID::CommTower},
		{"CommTower2", StructureID::CommTower},
		{"CommTower3", StructureID::CommTower},
		{"Commercial", StructureID::Commercial},
		{"FusionReactor", StructureID::FusionReactor},
		{"HotLaboratory", StructureID::HotLaboratory},
		{"Laboratory", StructureID::Laboratory},
		{"MaintenanceFacility", StructureID::MaintenanceFacility},
		{"MedicalCenter", StructureID::MedicalCenter},
		{"Nursery", StructureID::Nursery},
		{"Park", StructureID::Park},
		{"RecreationCenter", StructureID::RecreationCenter},
		{"Recycling", StructureID::Recycling},
		{"RedLightDistrict", StructureID::RedLightDistrict},
		{"Residence", StructureID::Residence},
		{"RobotCommand", StructureID::RobotCommand},
		{"RobotCommand2", StructureID::RobotCommand},
		{"Smelter", StructureID::Smelter},
		{"Smelter2", StructureID::Smelter},
		{"SolarPanel1", StructureID::SolarPanel1},
		{"SolarPanel2", StructureID::SolarPanel1},
		{"SolarPlant", StructureID::SolarPlant},
		{"StorageTanks", StructureID::StorageTanks},
		{"SurfaceFactory", StructureID::SurfaceFactory},
		{"SurfacePolice", StructureID::SurfacePolice},
		{"UndergroundFactory", StructureID::UndergroundFactory},
		{"UndergroundPolice", StructureID::UndergroundPolice},
		{"University", StructureID::University},
		{"Warehouse", StructureID::Warehouse},
	};


	const std::set<StructureID> UndergroundStructures =
	{
		StructureID::Commercial,
		StructureID::Laboratory,
		StructureID::MedicalCenter,
		StructureID::Nursery,
		StructureID::Park,
		StructureID::RecreationCenter,
		StructureID::RedLightDistrict,
		StructureID::Residence,
		StructureID::UndergroundFactory,
		StructureID::UndergroundPolice,
		StructureID::University,
	};


	std::string labTypeName(int labType)
	{
		return (labType == 0) ? "Underground Lab" : "Hot Lab";
	}

	// Length of "honeymoon period" of no crime/morale updates after landing, in turns
	const std::map<Difficulty, int> GracePeriod
	{
		{Difficulty::Beginner, 30},
		{Difficulty::Easy, 25},
		{Difficulty::Medium, 20},
		{Difficulty::Hard, 15}
	};

	// Morale loss multiplier on colonist death due to colony ship de-orbit
	const std::map<Difficulty, int> ColonyShipDeorbitMoraleLossMultiplier
	{
		{Difficulty::Beginner, 1},
		{Difficulty::Easy, 3},
		{Difficulty::Medium, 6},
		{Difficulty::Hard, 10}
	};



	int consumeFood(FoodProduction& producer, int amountToConsume)
	{
		const auto foodLevel = producer.foodLevel();
		const auto toTransfer = std::min(foodLevel, amountToConsume);

		producer.foodLevel(foodLevel - toTransfer);
		return toTransfer;
	}


	void consumeFood(const std::vector<FoodProduction*>& foodProducers, int amountToConsume)
	{
		for (auto* foodProducer : foodProducers)
		{
			if (amountToConsume <= 0) { break; }
			amountToConsume -= consumeFood(*foodProducer, amountToConsume);
		}
	}


	constexpr inline int ColonistsPerLander = 50;
	constexpr inline int CargoMoraleLossPerLander = 25;
}


void MapViewState::updatePopulation()
{
	int residences = mStructureManager.operationalCount(StructureClass::Residence);
	int universities = mStructureManager.operationalCount(StructureClass::University);
	int nurseries = mStructureManager.operationalCount(StructureClass::Nursery);
	int hospitals = mStructureManager.operationalCount(StructureClass::MedicalCenter);

	auto foodProducers = mStructureManager.getStructures<FoodProduction>();
	const auto& commandCenters = mStructureManager.getStructures<CommandCenter>();
	foodProducers.insert(foodProducers.end(), commandCenters.begin(), commandCenters.end());

	const auto researchEffects = computeColonyResearchEffects(mResearchTracker, mTechnologyReader);
	int amountToConsume = mPopulationModel.update(
		mMorale.currentMorale(),
		mFood,
		residences,
		universities,
		nurseries,
		hospitals,
		researchEffects.populationFertilityBonus,
		researchEffects.populationMortalityReduction,
		researchEffects.educationEfficiency);
	consumeFood(foodProducers, amountToConsume);
}


void MapViewState::updateCommercial()
{
	const auto& warehouses = mStructureManager.getStructures<Warehouse>();
	const auto& commercial = mStructureManager.structureList(StructureClass::Commercial);

	// No need to do anything if there are no commercial structures.
	if (commercial.empty()) { return; }

	int luxuryCount = mStructureManager.operationalCount(StructureClass::Commercial);
	int commercialCount = luxuryCount;

	for (auto* warehouse : warehouses)
	{
		ProductPool& productPool = warehouse->products();

		/**
		 * inspect for luxury products.
		 *
		 * FIXME: I feel like this could be done better. At the moment there
		 * is only one luxury item, clothing, but as this changes more
		 * items may be seen as luxury.
		 */
		int clothing = productPool.count(ProductType::PRODUCT_CLOTHING);

		if (clothing >= luxuryCount)
		{
			productPool.pull(ProductType::PRODUCT_CLOTHING, luxuryCount);
			luxuryCount = 0;
			break;
		}
		else if (clothing < luxuryCount)
		{
			productPool.pull(ProductType::PRODUCT_CLOTHING, clothing);
			luxuryCount -= clothing;
		}

		if (luxuryCount == 0)
		{
			break;
		}
	}

	auto commercialReverseIterator = commercial.rbegin();
	for (std::size_t i = 0; i < static_cast<std::size_t>(luxuryCount) && commercialReverseIterator != commercial.rend(); ++i, ++commercialReverseIterator)
	{
		if ((*commercialReverseIterator)->operational())
		{
			(*commercialReverseIterator)->idle(IdleReason::InsufficientLuxuryProduct);
		}
	}

	mMorale.journalMoraleChange({"Commercial - Luxury", commercialCount - luxuryCount});
}


void MapViewState::updateMorale()
{
	// POSITIVE MORALE EFFECTS
	// =========================================
	const int birthCount = mPopulationModel.birthCount();
	const int parkCount = mStructureManager.operationalCount(StructureClass::Park);
	const int recreationCount = mStructureManager.operationalCount(StructureClass::RecreationCenter);
	const int foodProducingStructures = mStructureManager.operationalCount(StructureClass::FoodProduction);
	const int commercialCount = mStructureManager.operationalCount(StructureClass::Commercial);

	// NEGATIVE MORALE EFFECTS
	// =========================================
	const int deathCount = mPopulationModel.deathCount();
	const int structuresDisabled = mStructureManager.disabledCount();
	const int structuresDestroyed = mStructureManager.destroyedCount();
	const int residentialOverCapacityHit = mPopulationModel.getPopulations().size() > mResidentialCapacity ? 2 : 0;
	const int foodProductionHit = foodProducingStructures > 0 ? 0 : 5;

	const auto& residences = mStructureManager.getStructures<Residence>();
	int bioWasteAccumulation = 0;
	for (const auto* residence : residences)
	{
		if (residence->wasteOverflow() > 0) { ++bioWasteAccumulation; }
	}

	// positive
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::Births), birthCount});
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::Parks), parkCount});
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::Recreation), recreationCount});
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::Commercial), commercialCount});

	const auto researchEffects = computeColonyResearchEffects(mResearchTracker, mTechnologyReader);
	const int researchMoraleBonus = researchEffects.moraleBonusPerTurn();
	if (researchMoraleBonus > 0)
	{
		mMorale.journalMoraleChange({"Research - Social Programs", researchMoraleBonus});
	}

	// negative
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::Deaths), -deathCount});
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::ResidentialOverflow), -residentialOverCapacityHit});
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::BiowasteOverflow), -bioWasteAccumulation * 2}); // TODO 2 is a magic number
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::StructuresDisabled), -structuresDisabled});
	mMorale.journalMoraleChange({moraleString(MoraleIndexs::StructuresDestroyed), -structuresDestroyed});
	mMorale.journalMoraleChange({"Food Production Issues", -foodProductionHit});


	for (const auto& moraleChangeEntry : mCrimeRateUpdate.moraleChanges())
	{
		mMorale.journalMoraleChange(moraleChangeEntry);
	}

	mPopulationPanel.crimeRate(mCrimeRateUpdate.meanCrimeRate());

	for (const auto& moraleChangeEntry : mCrimeExecution.moraleChanges())
	{
		mMorale.journalMoraleChange(moraleChangeEntry);
	}
}


void MapViewState::notifyBirthsAndDeaths()
{
	const int birthCount = mPopulationModel.birthCount();
	const int deathCount = mPopulationModel.deathCount();

	// Push notifications
	if (birthCount)
	{
		mNotificationArea.push({
			"Baby Born",
			std::to_string(birthCount) + (birthCount > 1 ? " babies were born." : " baby was born."),
			{{-1, -1}, 0},
			NotificationArea::NotificationType::Information});
	}

	if (deathCount)
	{
		mNotificationArea.push({
			"Colonist Died",
			std::to_string(deathCount) + (birthCount > 1 ? " colonists met their demise." : " colonist met their demise."),
			{{-1, -1}, 0},
			NotificationArea::NotificationType::Warning});
	}
}


void MapViewState::transportResourcesToStorage()
{
	const auto structureIsSmelter = [](const Structure& structure) { return structure.isSmelter(); };
	const auto& smelters = mStructureManager.getStructures(structureIsSmelter);
	for (auto* smelter : smelters)
	{
		if (!smelter->isOperable()) { continue; }

		auto& stored = smelter->storage();
		const auto toMove = stored.cap(25);

		const auto unmoved = addRefinedResources(toMove);
		stored -= (toMove - unmoved);
	}
}


void MapViewState::updateResources()
{
	mOreHaulRoutes->updateRoutes();
	mOreHaulRoutes->transportOreFromMines();
	transportResourcesToStorage();
	updatePlayerResources();
}


void MapViewState::checkColonyShip()
{
	if (mColonyShip.crashed())
	{
		const auto& crashedLanders = mColonyShip.crashedLanders();
		onColonyShipCrash(crashedLanders);
		mAnnouncement.onColonyShipCrash(mWindowStack, crashedLanders);
	}
}


void MapViewState::onColonyShipCrash(const ColonyShipLanders& colonyShipLanders)
{
	if(colonyShipLanders.colonist > 0)
	{
		int moraleChange = -1 * colonyShipLanders.colonist * ColonyShipDeorbitMoraleLossMultiplier.at(mDifficulty) * ColonistsPerLander;
		mMorale.journalMoraleChange({moraleString(MoraleIndexs::ColonistLanderLost), moraleChange});
	}

	if (colonyShipLanders.cargo > 0)
	{
		int moraleChange = -1 * colonyShipLanders.cargo * ColonyShipDeorbitMoraleLossMultiplier.at(mDifficulty) * CargoMoraleLossPerLander;
		mMorale.journalMoraleChange({moraleString(MoraleIndexs::CargoLanderLost), moraleChange});
	}
}


void MapViewState::checkWarehouseCapacity()
{
	const auto& warehouses = mStructureManager.getStructures<Warehouse>();

	if (warehouses.size() == 0) { return; } // no divisions by zero, pl0x

	int availableStorageTotal = 0;
	for (const auto warehouse : warehouses)
	{
		availableStorageTotal += warehouse->products().availableStoragePercent();
	}

	const int availableStorage = availableStorageTotal / static_cast<int>(warehouses.size());

	if (availableStorage == 0) // FIXME -- Magic Number
	{
		mNotificationArea.push({
			"No Warehouse Space",
			"You are out of storage space at your warehouses! Your Factories will go idle until you build more Warehouses or reduce inventory.",
			{{-1, -1}, 0},
			NotificationArea::NotificationType::Critical
		});
	}
	else if (availableStorage < 5) // FIXME -- Ditto
	{
		mNotificationArea.push({
			"Warehouse Space Critically Low",
			"Warehouse space is critically low! You only have " + std::to_string(availableStorage) + "% storage capacity remaining!",
			{{-1, -1}, 0},
			NotificationArea::NotificationType::Critical
		});
	}
	else if (availableStorage < 15) // FIXME -- Ditto
	{
		mNotificationArea.push({
			"Warehouse Space Low",
			"Warehouse space is running low. Current available storage capacity is at " + std::to_string(availableStorage) + "%.",
			{{-1, -1}, 0},
			NotificationArea::NotificationType::Warning
		});
	}
}


void MapViewState::updateResidentialCapacity()
{
	mResidentialCapacity = mStructureManager.totalResidentialCapacity();
	mPopulationPanel.residentialCapacity(mResidentialCapacity);
}


void MapViewState::updateBiowasteRecycling()
{
	const auto researchEffects = computeColonyResearchEffects(mResearchTracker, mTechnologyReader);
	int bioWasteProcessingCapacity = researchEffects.adjustedBioWasteProcessing(mStructureManager.totalBioWasteProcessingCapacity());
	if (bioWasteProcessingCapacity <= 0) { return; }

	// Process overflow first, prioritizing structures with minimal overflow
	auto residences = mStructureManager.getStructures<Residence>();
	std::ranges::stable_sort(residences, std::ranges::less{}, [](const Residence* residence) { return residence->wasteOverflow(); });
	for (auto* residence : residences)
	{
		const auto amountToProcess = std::min(bioWasteProcessingCapacity, residence->wasteOverflow());
		const auto processedWaste = residence->removeWaste(amountToProcess);
		bioWasteProcessingCapacity -= processedWaste;
		if (bioWasteProcessingCapacity <= 0) { return; }
	}

	// Process base amounts
	for (auto* residence : mStructureManager.getStructures<Residence>())
	{
		const auto processedWaste = residence->removeWaste(bioWasteProcessingCapacity);
		bioWasteProcessingCapacity -= processedWaste;
		if (bioWasteProcessingCapacity <= 0) { return; }
	}
}


void MapViewState::updateFood()
{
	mFood = 0;

	auto foodProducers = mStructureManager.getStructures<FoodProduction>();
	const auto& command = mStructureManager.getStructures<CommandCenter>();

	foodProducers.insert(foodProducers.begin(), command.begin(), command.end());

	for (const auto* foodProdcer : foodProducers)
	{
		if (foodProdcer->isOperable())
		{
			mFood += foodProdcer->foodLevel();
		}
	}
}


void MapViewState::transferFoodToCommandCenter()
{
	const auto& foodProducers = mStructureManager.getStructures<FoodProduction>();
	const auto& commandCenters = mStructureManager.getStructures<CommandCenter>();

	auto foodProducerIterator = foodProducers.begin();
	for (auto* commandCenter : commandCenters)
	{
		if (!commandCenter->operational()) { continue; }

		int foodToMove = commandCenter->foodStorageCapacity() - commandCenter->foodLevel();

		while (foodProducerIterator != foodProducers.end())
		{
			auto foodProducer = dynamic_cast<FoodProduction*>(*foodProducerIterator);
			const int foodMoved = std::clamp(foodToMove, 0, foodProducer->foodLevel());
			foodProducer->foodLevel(foodProducer->foodLevel() - foodMoved);
			commandCenter->foodLevel(commandCenter->foodLevel() + foodMoved);

			foodToMove -= foodMoved;

			if (foodToMove == 0) { break; }

			++foodProducerIterator;
		}
	}
}


/**
 * Update road intersection patterns
 */
void MapViewState::updateRoads()
{
	const auto& roads = mStructureManager.getStructures<Road>();

	for (auto* road : roads)
	{
		road->updateConnections(*mTileMap);
	}
}


void MapViewState::checkAgingStructures()
{
	const auto& structures = mStructureManager.agingStructures();

	for (const auto* structure : structures)
	{
		if (structure->age() == structure->maxAge() - 10)
		{
			mNotificationArea.push({
				"Aging Structure",
				structure->name() + " is getting old. You should replace it soon.",
				structure->xyz(),
				NotificationArea::NotificationType::Warning});
		}
		else if (structure->age() == structure->maxAge() - 5)
		{
			mNotificationArea.push({
				"Aging Structure",
				structure->name() + " is about to collapse. You should replace it right away or consider demolishing it.",
				structure->xyz(),
				NotificationArea::NotificationType::Critical});
		}
	}
}


void MapViewState::checkNewlyBuiltStructures()
{
	const auto& structures = mStructureManager.newlyBuiltStructures();

	for (const auto* structure : structures)
	{
		mNotificationArea.push({
			"Construction Finished",
			structure->name() + " completed construction.",
			structure->xyz(),
			NotificationArea::NotificationType::Success});
	}
}


void MapViewState::updateMaintenance()
{
	auto sortLambda = [](const Structure* lhs, const Structure* rhs) -> bool
	{
		return lhs->integrity() < rhs->integrity();
	};

	auto structures = mStructureManager.allStructures();
	std::sort(structures.begin(), structures.end(), sortLambda);

	const auto& maintenanceFacilities = mStructureManager.getStructures<MaintenanceFacility>();
	for (auto* maintenanceFacility : maintenanceFacilities)
	{
		maintenanceFacility->repairStructures(structures);
	}
}


void MapViewState::updateOverlays()
{
	updateRouteOverlay();
	updateCommRangeOverlay();
	updatePoliceOverlay();

	if (mBtnToggleRouteOverlay.isPressed()) { onToggleRouteOverlay(); }
	if (mBtnToggleConnectedness.isPressed()) { onToggleConnectedness(); }
	if (mBtnToggleCommRangeOverlay.isPressed()) { onToggleCommRangeOverlay(); }
	if (mBtnTogglePoliceOverlay.isPressed()) { onTogglePoliceOverlay(); }
}


void MapViewState::updateResearchStatusDisplay()
{
	std::ostringstream status;

	for (const auto& [techId, researchProgress] : mResearchTracker.currentResearch())
	{
		const auto* technology = mTechnologyReader.findTechnologyFromId(techId);
		if (!technology) { continue; }

		if (!status.str().empty()) { status << "  |  "; }
		status << "Researching: " << technology->name << " (" << labTypeName(technology->labType) << ", "
			<< researchProgress.progress << "/" << technology->cost << ")";
	}

	for (const auto& [labType, queue] : mResearchTracker.researchQueue())
	{
		if (queue.empty()) { continue; }

		const auto* nextQueued = mTechnologyReader.findTechnologyFromId(queue.front());
		if (!nextQueued) { continue; }

		if (!status.str().empty()) { status << "  |  "; }
		status << "Queued (" << labTypeName(labType) << "): " << nextQueued->name;
		if (queue.size() > 1)
		{
			status << " (+" << (queue.size() - 1) << " more)";
		}
	}

	mResearchStatusText = status.str();
}


void MapViewState::updateResearch()
{
	int regularResearchPoints = 0;
	int hotResearchPoints = 0;

	for (auto* lab : mStructureManager.getStructures<ResearchFacility>())
	{
		if (!lab->operational()) { continue; }

		regularResearchPoints += lab->regularResearchProduced();
		hotResearchPoints += lab->hotResearchProduced();
	}

	const auto inProgressResearch = mResearchTracker.currentResearch();
	std::vector<int> completedThisTurn;

	for (const auto& [techId, researchProgress] : inProgressResearch)
	{
		const auto& technology = mTechnologyReader.technologyFromId(techId);
		const int basePointsThisTurn = (technology.labType == 0) ? regularResearchPoints : hotResearchPoints;
		const auto researchEffects = computeColonyResearchEffects(mResearchTracker, mTechnologyReader);
		const int pointsThisTurn = researchEffects.adjustedResearchPoints(basePointsThisTurn);

		if (pointsThisTurn <= 0) { continue; }

		const int updatedProgress = researchProgress.progress + pointsThisTurn;

		if (updatedProgress >= technology.cost)
		{
			mResearchTracker.completeResearch(techId);
			completedThisTurn.push_back(techId);
			mNotificationArea.push({
				"Research Complete",
				technology.name + " has been researched.",
				{{-1, -1}, 0},
				NotificationArea::NotificationType::Success
			});

			for (const auto& unlock : technology.unlocks)
			{
				if (unlock.unlocks != Technology::Unlock::Unlocks::DisasterPrediction) { continue; }

				mNotificationArea.push({
					"Early Warning Online",
					"Colony sensors can now provide advance warning for " + unlock.value + " events.",
					{{-1, -1}, 0},
					NotificationArea::NotificationType::Information});
			}

			for (const auto& unlock : technology.unlocks)
			{
				if (unlock.unlocks != Technology::Unlock::Unlocks::DisasterPrediction) { continue; }

				mNotificationArea.push({
					"Early Warning Online",
					"Colony sensors can now provide advance warning for " + unlock.value + " events.",
					{{-1, -1}, 0},
					NotificationArea::NotificationType::Information});
			}
		}
		else
		{
			mResearchTracker.updateResearch(techId, updatedProgress, researchProgress.scientistsAssigned);
		}
	}

	for (const auto techId : completedThisTurn)
	{
		const auto* completedTechnology = mTechnologyReader.findTechnologyFromId(techId);
		if (!completedTechnology) { continue; }

		const auto labType = completedTechnology->labType;
		const auto nextTechId = mResearchTracker.popNextQueued(labType);
		if (nextTechId >= 0)
		{
			const auto* nextTechnology = mTechnologyReader.findTechnologyFromId(nextTechId);
			if (!nextTechnology) { continue; }

			mResearchTracker.startResearch(nextTechId, 0, 0);
			mNotificationArea.push({
				"Research Started",
				nextTechnology->name + " is now being researched at the " + labTypeName(labType) + ".",
				{{-1, -1}, 0},
				NotificationArea::NotificationType::Information
			});
		}
	}

	// get list of completed technologies
	const auto& completedTechs = mResearchTracker.completedResearch();
	std::vector<const Technology*> techList;
	for (const auto techId : completedTechs)
	{
		const auto* technology = mTechnologyReader.findTechnologyFromId(techId);
		if (technology) { techList.push_back(technology); }
	}

	// get list of completed technologies that unlock buildings
	std::vector<const Technology*> unlockedStructures;

	for (auto tech : techList)
	{
		for (const auto& unlock : (*tech).unlocks)
		{
			if (unlock.unlocks == Technology::Unlock::Unlocks::Structure)
			{
				unlockedStructures.push_back(tech);
			}
		}
	}

	for (const auto& tech : unlockedStructures)
	{
		for (const auto& unlock : tech->unlocks)
		{
			if (unlock.unlocks != Technology::Unlock::Unlocks::Structure) { continue; }

			const auto structureIt = StructureIdFromString.find(unlock.value);
			if (structureIt == StructureIdFromString.end()) { continue; }

			const auto structureId = structureIt->second;
			if (UndergroundStructures.find(structureId) != UndergroundStructures.end())
			{
				mStructureTracker.addUnlockedUndergroundStructure(structureId);
			}
			else
			{
				mStructureTracker.addUnlockedSurfaceStructure(structureId);
			}
		}
	}

	if (computeColonyResearchEffects(mResearchTracker, mTechnologyReader).explorerBotUnlocked)
	{
		for (auto* factory : mStructureManager.getStructures<Factory>())
		{
			const auto& products = factory->productList();
			if (std::find(products.begin(), products.end(), ProductType::PRODUCT_EXPLORER) == products.end())
			{
				factory->enableProduct(ProductType::PRODUCT_EXPLORER);
			}
		}
	}

	updateResearchStatusDisplay();
}


void MapViewState::nextTurn()
{
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	const auto imageProcessingTurn = &getImage("sys/processing_turn.png");
	renderer.drawImage(*imageProcessingTurn, renderer.center() - imageProcessingTurn->size() / 2);
	renderer.update();

	mNotificationWindow.hide();
	mNotificationArea.clear();

	mMorale.closeJournal();

	mMapObjectPicker.clearBuildMode();
	mMapObjectPicker.clearSelections();

	mPopulationPool.clear();

	mResourceBreakdownPanel.previousResources(mResourcesCount);

	updateConnectedness();

	const auto researchEffects = computeColonyResearchEffects(mResearchTracker, mTechnologyReader);
	mStructureManager.update(mResourcesCount, mPopulationPool, researchEffects);

	checkAgingStructures();
	checkNewlyBuiltStructures();

	transferFoodToCommandCenter();

	updateResidentialCapacity();

	// Colony will not have morale or crime effects until at least n turns from landing, depending on difficulty
	bool isMoraleEnabled = mTurnCount > mTurnNumberOfLanding + GracePeriod.at(mDifficulty);

	if (isMoraleEnabled)
	{
		mCrimeRateUpdate.update();
		auto structuresCommittingCrimes = mCrimeRateUpdate.structuresCommittingCrimes();
		mCrimeExecution.executeCrimes(structuresCommittingCrimes);
	}

	updateFood();
	updatePopulation();

	updateMaintenance();
	updateCommercial();
	updateBiowasteRecycling();

	if (isMoraleEnabled)
	{
		updateMorale();
	}

	notifyBirthsAndDeaths();

	updateRobots();
	updateResources();
	updateStructuresAvailability();
	updateRoads();

	updateOverlays();

	const auto& factories = mStructureManager.getStructures<Factory>();
	for (auto* factory : factories)
	{
		factory->updateProduction();
	}

	updateResearch();

	if (!mColonyShip.crashed())
	{
		mColonyShip.onTurn();
		checkColonyShip();
	}

	checkWarehouseCapacity();

	mMineOperationsWindow.updateTruckAvailability();

	// Check for Game Over conditions
	if (mPopulationModel.getPopulations().size() <= 0 && mColonyShip.colonistLanders() == 0)
	{
		hideUi();
		mGameOverDialog.show();
	}

	populateRobotMenu();
	populateStructureMenu();

	mMorale.commitMoraleChanges();

	mTurnCount++;

	mResourceInfoBar.ignoreGlow(false);
}
