#include "MapObjectPicker.h"

#include "../States/MapViewStateHelper.h"
#include "../Constants/Strings.h"
#include "../Constants/UiConstants.h"
#include "../MapObjects/RobotTypeIndex.h"
#include "../StructureCatalog.h"

#include <libOPHD/EnumStructureID.h>
#include <libOPHD/MapObjects/StructureType.h>


#include <array>


namespace
{
	constexpr unsigned int StructureIconSize{46};
	constexpr unsigned int RobotIconSize{46};

	constexpr auto NotSet = int{54};

	constexpr auto structureIconTable = std::array{
		NotSet, // None
		5, // Agridome
		NotSet, // AirShaft
		1, // CargoLander
		3, // Chap
		2, // ColonistLander
		NotSet, // CommandCenter
		66, // Commercial
		22, // CommTower
		21, // FusionReactor
		18, // HotLaboratory
		58, // Laboratory
		62, // MedicalCenter
		NotSet, // MineFacility
		NotSet, // MineShaft
		77, // Nursery
		75, // Park
		73, // RecreationCenter
		76, // RedLightDistrict
		55, // Residence
		24, // Road
		14, // RobotCommand
		98, // SeedFactory
		0, // SeedLander
		98, // SeedPower
		98, // SeedSmelter
		4, // Smelter
		33, // SolarPanel1
		10, // SolarPlant
		8, // StorageTanks
		11, // SurfaceFactory
		23, // SurfacePolice
		110, // Tube
		69, // UndergroundFactory
		61, // UndergroundPolice
		63, // University
		9, // Warehouse
		16, // Recycling
		54, // MaintenanceFacility
	};

	static_assert(structureIconTable.size() == static_cast<std::size_t>(StructureID::Count));


	IconGridItem idToIconGridItem(StructureID structureId)
	{
		const auto index = StructureCatalog::typeIndex(structureId);
		return {
			StructureCatalog::getType(index).name,
			structureIconTable.at(index),
			static_cast<int>(index),
		};
	}


	const std::vector<IconGridItem> SurfaceTubes = {
		{"Tube", 110},
	};

	const std::vector<IconGridItem> UndergroundTubes = {
		{"Tube", 113},
	};


	void setTubes(IconGrid& tubesGrid, const std::vector<IconGridItem>& items)
	{
		tubesGrid.clear();
		for (const auto& item : items)
		{
			tubesGrid.addItem(item);
		}
	}
}


enum class InsertMode
{
	None,
	Robot,
	Tube,
	Structure
};


MapObjectPicker::MapObjectPicker(const StorableResources& resources, SelectionChangedDelegate selectionChangedHandler) :
	ControlContainer{{&mStructures, &mConnections, &mMinerRobot, &mRobots}},
	mSelectionChangedHandler{selectionChangedHandler},
	mResourcesCount{resources},
	mInsertMode{InsertMode::None},
	mCurrentStructure{StructureID::None},
	mCurrentRobot{RobotTypeIndex::None},
	mStructures{{this, &MapObjectPicker::onStructuresSelectionChange}, "ui/structures.png", StructureIconSize, constants::MarginTight, true},
	mConnections{{this, &MapObjectPicker::onConnectionsSelectionChange}, "ui/structures.png", StructureIconSize, constants::MarginTight, true},
	mMinerRobot{{this, &MapObjectPicker::onMinerRobotSelectionChange}, "ui/robots.png", RobotIconSize, constants::MarginTight, true},
	mRobots{{this, &MapObjectPicker::onRobotsSelectionChange}, "ui/robots.png", RobotIconSize, constants::MarginTight, true}
{
}


MapObjectPicker::~MapObjectPicker()
{
}


void MapObjectPicker::setStructureIds(const std::vector<StructureID>& structureIds)
{
	mStructures.clear();
	for (const auto structureId : structureIds)
	{
		mStructures.addItem(idToIconGridItem(structureId));
	}
}


void MapObjectPicker::setTubesAboveGround()
{
	setTubes(mConnections, SurfaceTubes);
}


void MapObjectPicker::setTubesUnderGround()
{
	setTubes(mConnections, UndergroundTubes);
}


void MapObjectPicker::addStructure(StructureID structureId)
{
	mStructures.addItem(idToIconGridItem(structureId));
}


bool MapObjectPicker::isInserting() const
{
	return mInsertMode != InsertMode::None;
}


bool MapObjectPicker::isInsertingStructure() const
{
	return mInsertMode == InsertMode::Structure;
}


bool MapObjectPicker::isInsertingRobot() const
{
	return mInsertMode == InsertMode::Robot;
}


bool MapObjectPicker::isInsertingTube() const
{
	return mInsertMode == InsertMode::Tube;
}


StructureID MapObjectPicker::selectedStructureId() const
{
	return mCurrentStructure;
}


RobotTypeIndex MapObjectPicker::selectedRobotIndex() const
{
	return mCurrentRobot;
}


void MapObjectPicker::clearBuildMode()
{
	mInsertMode = InsertMode::None;
	mCurrentStructure = StructureID::None;
	mCurrentRobot = RobotTypeIndex::None;
	mSelectionChangedHandler();
}


void MapObjectPicker::clearSelections()
{
	mStructures.clearSelection();
	mConnections.clearSelection();
	mMinerRobot.clearSelection();
	mRobots.clearSelection();
}


bool MapObjectPicker::selectStructure(StructureID structureId)
{
	if (structureId == StructureID::None) { return false; }

	const auto typeIndex = StructureCatalog::typeIndex(structureId);
	const auto& structureType = StructureCatalog::getType(typeIndex);

	if (!mStructures.itemExists(structureType.name)) { return false; }

	if (!mStructures.itemAvailable(structureType.name))
	{
		resourceShortageMessage(mResourcesCount, structureId);
		return false;
	}

	mConnections.clearSelection();
	mMinerRobot.clearSelection();
	mRobots.clearSelection();
	mStructures.setSelectionByMeta(static_cast<int>(typeIndex));
	mCurrentStructure = structureId;
	mInsertMode = InsertMode::Structure;
	mSelectionChangedHandler();
	return true;
}


bool MapObjectPicker::selectRobot(RobotTypeIndex robotTypeIndex)
{
	if (robotTypeIndex == RobotTypeIndex::None) { return false; }

	std::string robotName;
	switch (robotTypeIndex)
	{
	case RobotTypeIndex::Digger:
		robotName = constants::Robodigger;
		break;
	case RobotTypeIndex::Dozer:
		robotName = constants::Robodozer;
		break;
	case RobotTypeIndex::Miner:
		robotName = constants::Robominer;
		break;
	case RobotTypeIndex::Explorer:
		robotName = constants::Roboexplorer;
		break;
	default:
		return false;
	}

	auto& robotGrid = robotTypeIndex == RobotTypeIndex::Miner ? mMinerRobot : mRobots;

	if (!robotGrid.itemExists(robotName) || !robotGrid.itemAvailable(robotName))
	{
		return false;
	}

	mStructures.clearSelection();
	mConnections.clearSelection();
	mRobots.clearSelection();
	mMinerRobot.clearSelection();
	robotGrid.setSelectionByMeta(static_cast<int>(robotTypeIndex));
	mCurrentRobot = robotTypeIndex;
	mInsertMode = InsertMode::Robot;
	mSelectionChangedHandler();
	return true;
}


void MapObjectPicker::onStructuresSelectionChange(const IconGridItem* item)
{
	if (!item)
	{
		clearBuildMode();
		return;
	}

	mConnections.clearSelection();
	mMinerRobot.clearSelection();
	mRobots.clearSelection();

	const auto structureId = static_cast<StructureID>(item->meta);

	// Check availability
	if (!item->available)
	{
		resourceShortageMessage(mResourcesCount, structureId);
		mStructures.clearSelection();
		return;
	}

	mCurrentStructure = structureId;
	mInsertMode = InsertMode::Structure;
	mSelectionChangedHandler();
}


/**
 * Handler for the Tubes Palette dialog.
 */
void MapObjectPicker::onConnectionsSelectionChange(const IconGridItem* item)
{
	if (!item)
	{
		clearBuildMode();
		return;
	}

	mMinerRobot.clearSelection();
	mRobots.clearSelection();
	mStructures.clearSelection();

	mCurrentStructure = StructureID::Tube;
	mInsertMode = InsertMode::Tube;
	mSelectionChangedHandler();
}


void MapObjectPicker::onMinerRobotSelectionChange(const IconGridItem* item)
{
	if (!item)
	{
		clearBuildMode();
		return;
	}

	mConnections.clearSelection();
	mRobots.clearSelection();
	mStructures.clearSelection();

	mCurrentRobot = static_cast<RobotTypeIndex>(item->meta);
	mInsertMode = InsertMode::Robot;
	mSelectionChangedHandler();
}


/**
 * Handles clicks of the Robot Selection Menu.
 */
void MapObjectPicker::onRobotsSelectionChange(const IconGridItem* item)
{
	if (!item)
	{
		clearBuildMode();
		return;
	}

	mConnections.clearSelection();
	mMinerRobot.clearSelection();
	mStructures.clearSelection();

	mCurrentRobot = static_cast<RobotTypeIndex>(item->meta);
	mInsertMode = InsertMode::Robot;
	mSelectionChangedHandler();
}


void MapObjectPicker::onResize()
{
	constexpr int columnWidth{52};
	const auto iconRowHeight = static_cast<int>(RobotIconSize) + constants::MarginTight;

	mRobots.position(mRect.crossXPoint() - NAS2D::Vector{columnWidth, 0});
	mConnections.position({mRobots.position().x - constants::MarginTight - columnWidth, mRect.position.y});
	mMinerRobot.position({mConnections.position().x, mConnections.position().y + iconRowHeight});
	mStructures.position(mRect.position);

	mConnections.size({columnWidth, iconRowHeight});
	mMinerRobot.size({columnWidth, iconRowHeight});
	mRobots.size({columnWidth, mRect.size.y});
	mStructures.size({mRect.size.x - columnWidth * 3 - constants::MarginTight * 2, mRect.size.y});
}
