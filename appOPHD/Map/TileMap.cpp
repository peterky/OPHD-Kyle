#include "TileMap.h"

#include "Tile.h"

#include <libOPHD/EnumOreDepositYield.h>
#include <libOPHD/DirectionOffset.h>
#include <libOPHD/RandomNumberGenerator.h>
#include <libOPHD/MapObjects/OreDeposit.h>

#include <NAS2D/ParserHelper.h>
#include <NAS2D/Xml/XmlElement.h>
#include <NAS2D/Math/Point.h>
#include <NAS2D/Math/PointInRectangleRange.h>
#include <NAS2D/Math/Rectangle.h>
#include <NAS2D/Renderer/Color.h>
#include <NAS2D/Resource/Image.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <numeric>
#include <stdexcept>


namespace {
	const std::string MapTerrainExtension = "_a.png";
	const auto MapSize = NAS2D::Vector{300, 150};
	// Reserve headroom so extendMaxDepth() can grow without reallocating existing tiles
	// (Structure objects hold Tile& references that would dangle on vector reallocation).
	constexpr int ReservedExtraDepthLevels{2};


	const std::array<std::string, 4> ResourceFieldNames =
	{
		"common_metals", "common_minerals", "rare_metals", "rare_minerals"
	};


	constexpr std::size_t linearSize(NAS2D::Vector<int> size)
	{
		const auto converted = size.to<std::size_t>();
		return converted.x * converted.y;
	}


	constexpr std::size_t linearIndex(NAS2D::Point<int> point, int sizeX)
	{
		const auto converted = point.to<std::size_t>();
		return converted.x + static_cast<std::size_t>(sizeX) * converted.y;
	}


	std::vector<NAS2D::Point<int>> generateOreDeposits(NAS2D::Vector<int> mapSize, std::size_t oreDepositCount, const std::vector<bool>& usedLocations)
	{
		auto randPoint = [mapSize]() {
			return NAS2D::Point{
				randomNumber.generate<int>(5, mapSize.x - 5),
				randomNumber.generate<int>(5, mapSize.y - 5)
			};
		};

		std::vector<NAS2D::Point<int>> locations;
		locations.reserve(oreDepositCount);

		// Some locations might not be acceptable, so try up to twice as many locations
		// A high density of ore deposits could result in many rejected locations
		// Don't try indefinitely to avoid possibility of infinite loop
		auto blockedLocations = usedLocations;
		if (blockedLocations.size() != linearSize(mapSize))
		{
			blockedLocations.assign(linearSize(mapSize), false);
		}

		for (std::size_t i = 0; (locations.size() < oreDepositCount) && (i < oreDepositCount * 2); ++i)
		{
			// Generate a location and check surroundings for minimum spacing
			const auto point = randPoint();
			if (!blockedLocations[linearIndex(point, mapSize.x)])
			{
				locations.push_back(point);
				for (const auto& offset : DirectionScan3x3)
				{
					const auto usedPoint = point + offset;
					blockedLocations[linearIndex(usedPoint, mapSize.x)] = true;
				}
			}
		}

		return locations;
	}


	std::vector<bool> markOreDepositLocations(NAS2D::Vector<int> mapSize, const std::vector<NAS2D::Point<int>>& locations)
	{
		std::vector<bool> usedLocations(linearSize(mapSize));
		for (const auto& point : locations)
		{
			for (const auto& offset : DirectionScan3x3)
			{
				const auto usedPoint = point + offset;
				usedLocations[linearIndex(usedPoint, mapSize.x)] = true;
			}
		}
		return usedLocations;
	}


	auto makeYieldSelector(const TileMap::OreDepositYields& oreDepositYields)
	{
		const auto total = std::accumulate(oreDepositYields.begin(), oreDepositYields.end(), 0);
		return [oreDepositYields, total]() {
			const auto randValue = randomNumber.generate<int>(1, total);
			return (randValue <= oreDepositYields[0]) ? OreDepositYield::Low :
				(randValue <= oreDepositYields[0] + oreDepositYields[1]) ? OreDepositYield::Medium :
				OreDepositYield::High;
		};
	}


	std::vector<OreDeposit*> placeOreDeposits(TileMap& tileMap, const std::vector<NAS2D::Point<int>>& locations, const TileMap::OreDepositYields& oreDepositYields)
	{
		std::vector<OreDeposit*> oreDeposits;
		oreDeposits.reserve(locations.size());

		const auto randYield = makeYieldSelector(oreDepositYields);

		for (const auto& location : locations)
		{
			auto& tile = tileMap.getTile({location, 0});
			auto* oreDeposit = oreDeposits.emplace_back(new OreDeposit(randYield(), location));
			tile.placeOreDeposit(oreDeposit);
			tile.bulldoze();
		}
		return oreDeposits;
	}


	NAS2D::Xml::XmlElement* serializeOreDeposit(const OreDeposit& oreDeposit)
	{
		const auto location = oreDeposit.location();
		auto* element = NAS2D::dictionaryToAttributes(
			"mine",
			NAS2D::Dictionary{{
				{"x", location.x},
				{"y", location.y},
				{"depth", oreDeposit.digDepth()},
				{"yield", static_cast<int>(oreDeposit.yield())},
				// Unused fields, retained for backwards compatibility
				{"active", true},
				{"flags", "011111"},
			}}
		);

		const auto& availableResources = oreDeposit.availableResources();
		if (!availableResources.isEmpty())
		{
			element->linkEndChild(NAS2D::dictionaryToAttributes(
				"vein",
				NAS2D::Dictionary{{
					{ResourceFieldNames[0], availableResources.resources[0]},
					{ResourceFieldNames[1], availableResources.resources[1]},
					{ResourceFieldNames[2], availableResources.resources[2]},
					{ResourceFieldNames[3], availableResources.resources[3]},
				}}
			));
		}

		return element;
	}


	OreDeposit deserializeOreDeposit(NAS2D::Xml::XmlElement* element)
	{
		const auto dictionary = NAS2D::attributesToDictionary(*element);

		const auto x = dictionary.get<int>("x");
		const auto y = dictionary.get<int>("y");
		const auto digDepth = dictionary.get<int>("depth");
		const auto yield = static_cast<OreDepositYield>(dictionary.get<int>("yield"));

		StorableResources availableResources = {};
		// Keep the vein iteration so we can still load old saved games
		for (auto* vein = element->firstChildElement(); vein != nullptr; vein = vein->nextSiblingElement())
		{
			const auto veinDictionary = NAS2D::attributesToDictionary(*vein);
			const auto veinReserves = StorableResources{
				veinDictionary.get<int>(ResourceFieldNames[0], 0),
				veinDictionary.get<int>(ResourceFieldNames[1], 0),
				veinDictionary.get<int>(ResourceFieldNames[2], 0),
				veinDictionary.get<int>(ResourceFieldNames[3], 0),
			};
			availableResources += veinReserves;
		}

		return {availableResources, yield, {x, y}, digDepth};
	}
}


TileMap::TileMap(const std::string& mapPath, int maxDepth, std::size_t oreDepositCount, const OreDepositYields& oreDepositYields) :
	TileMap{mapPath, maxDepth}
{
	const auto visibleLocations = generateOreDeposits(mSizeInTiles, oreDepositCount, {});
	mOreDeposits = placeOreDeposits(*this, visibleLocations, oreDepositYields);
	populateUndiscoveredOreDeposits(oreDepositCount, oreDepositYields);
}


void TileMap::populateUndiscoveredOreDeposits(std::size_t oreDepositCount, const OreDepositYields& oreDepositYields)
{
	std::vector<NAS2D::Point<int>> visibleLocations;
	visibleLocations.reserve(mOreDeposits.size());
	for (const auto* deposit : mOreDeposits)
	{
		visibleLocations.push_back(deposit->location());
	}

	const auto blockedLocations = markOreDepositLocations(mSizeInTiles, visibleLocations);
	const auto hiddenLocations = generateOreDeposits(mSizeInTiles, oreDepositCount, blockedLocations);
	const auto randYield = makeYieldSelector(oreDepositYields);
	mUndiscoveredOreDeposits.reserve(mUndiscoveredOreDeposits.size() + hiddenLocations.size());
	for (const auto& location : hiddenLocations)
	{
		mUndiscoveredOreDeposits.push_back(UndiscoveredOreDeposit{location, randYield()});
	}
}


TileMap::TileMap(const std::string& mapPath, int maxDepth) :
	mSizeInTiles{MapSize},
	mMapPath{mapPath},
	mMaxDepth{maxDepth}
{
	buildTerrainMap(mapPath);
}


void TileMap::extendMaxDepth(int additionalLevels)
{
	if (additionalLevels <= 0) { return; }

	const auto previousMaxDepth = mMaxDepth;
	mMaxDepth += additionalLevels;

	const auto newLinearSize = linearSize();
	if (newLinearSize > mTileMap.capacity())
	{
		throw std::runtime_error("TileMap::extendMaxDepth(): tile storage capacity exceeded; structure tile references would be invalidated.");
	}

	const NAS2D::Image heightmap(mMapPath + MapTerrainExtension);
	mTileMap.resize(newLinearSize);

	for (int depth = previousMaxDepth + 1; depth <= mMaxDepth; ++depth)
	{
		for (const auto point : NAS2D::PointInRectangleRange{area()})
		{
			const auto color = heightmap.pixelColor(point);
			auto& tile = getTile({point, depth});
			const auto terrainType = static_cast<TerrainType>(std::clamp(color.red / 50, 1, 4));
			tile = {{point, depth}, terrainType};
		}
	}
}


TileMap::~TileMap()
{
	for (const auto* oreDeposit : mOreDeposits)
	{
		auto& tile = getTile({oreDeposit->location(), 0});
		tile.removeOreDeposit();
		delete oreDeposit;
	}
}


NAS2D::Rectangle<int> TileMap::area() const
{
	return {{0, 0}, mSizeInTiles};
}


bool TileMap::isValidPosition(const MapCoordinate& position) const
{
	return area().contains(position.xy) && position.z >= 0 && position.z <= mMaxDepth;
}


const Tile& TileMap::getTile(const MapCoordinate& position) const
{
	if (!isValidPosition(position))
	{
		throw std::runtime_error("Tile coordinates out of bounds: {" + std::to_string(position.xy.x) + ", " + std::to_string(position.xy.y) + ", " + std::to_string(position.z) + "}");
	}
	return mTileMap[linearIndex(position)];
}


Tile& TileMap::getTile(const MapCoordinate& position)
{
	const auto& constThis = *this;
	return const_cast<Tile&>(constThis.getTile(position));
}


bool TileMap::hasOreDeposit(const MapCoordinate& mapCoordinate) const
{
	return getTile({mapCoordinate.xy, 0}).hasOreDeposit();
}


const std::vector<OreDeposit*>& TileMap::oreDeposits() const
{
	return mOreDeposits;
}


std::optional<NAS2D::Point<int>> TileMap::prospectForOreDeposit(NAS2D::Point<int> searchCenter, int searchRadius)
{
	std::optional<std::size_t> bestIndex;
	int bestDistance = std::numeric_limits<int>::max();

	for (std::size_t i = 0; i < mUndiscoveredOreDeposits.size(); ++i)
	{
		const auto& deposit = mUndiscoveredOreDeposits[i];
		const auto delta = searchCenter - deposit.location;
		const auto distance = std::max(std::abs(delta.x), std::abs(delta.y));
		if (distance > searchRadius) { continue; }

		auto& tile = getTile({deposit.location, 0});
		if (tile.hasOreDeposit() || tile.hasMapObject()) { continue; }

		if (!bestIndex || distance < bestDistance)
		{
			bestDistance = distance;
			bestIndex = i;
		}
	}

	if (!bestIndex) { return std::nullopt; }

	const auto undiscoveredDeposit = mUndiscoveredOreDeposits[*bestIndex];
	mUndiscoveredOreDeposits.erase(mUndiscoveredOreDeposits.begin() + static_cast<std::ptrdiff_t>(*bestIndex));

	auto& tile = getTile({undiscoveredDeposit.location, 0});
	auto* oreDeposit = new OreDeposit(undiscoveredDeposit.yield, undiscoveredDeposit.location);
	tile.placeOreDeposit(oreDeposit);
	tile.bulldoze();
	mOreDeposits.push_back(oreDeposit);

	return undiscoveredDeposit.location;
}


std::optional<NAS2D::Point<int>> TileMap::spawnEmergencyOreDeposit(NAS2D::Point<int> preferredCenter)
{
	std::vector<NAS2D::Point<int>> existingLocations;
	existingLocations.reserve(mOreDeposits.size());
	for (const auto* deposit : mOreDeposits)
	{
		existingLocations.push_back(deposit->location());
	}

	const auto blockedLocations = markOreDepositLocations(mSizeInTiles, existingLocations);
	constexpr OreDepositYields defaultYields{30, 50, 20};
	const auto randYield = makeYieldSelector(defaultYields);

	for (int attempt = 0; attempt < 100; ++attempt)
	{
		const auto point = NAS2D::Point<int>{
			std::clamp(preferredCenter.x + randomNumber.generate<int>(-25, 25), 5, mSizeInTiles.x - 6),
			std::clamp(preferredCenter.y + randomNumber.generate<int>(-25, 25), 5, mSizeInTiles.y - 6)
		};

		const auto blockedIndex = static_cast<std::size_t>(point.x) + static_cast<std::size_t>(mSizeInTiles.x) * static_cast<std::size_t>(point.y);
		if (blockedLocations.at(blockedIndex)) { continue; }

		auto& tile = getTile({point, 0});
		if (tile.hasOreDeposit() || tile.hasMapObject()) { continue; }

		auto* oreDeposit = new OreDeposit(randYield(), point);
		tile.placeOreDeposit(oreDeposit);
		tile.bulldoze();
		mOreDeposits.push_back(oreDeposit);
		return point;
	}

	return std::nullopt;
}


std::vector<NAS2D::Point<int>> TileMap::prospectForOreDeposits(NAS2D::Point<int> searchCenter, int searchRadius, int depositCount)
{
	std::vector<NAS2D::Point<int>> discovered;
	discovered.reserve(static_cast<std::size_t>(depositCount));

	const auto mapSpan = std::max(mSizeInTiles.x, mSizeInTiles.y);

	for (int i = 0; i < depositCount; ++i)
	{
		std::optional<NAS2D::Point<int>> location = prospectForOreDeposit(searchCenter, searchRadius);
		if (!location)
		{
			location = prospectForOreDeposit(searchCenter, mapSpan);
		}
		if (!location)
		{
			location = spawnEmergencyOreDeposit(searchCenter);
		}

		if (location)
		{
			discovered.push_back(*location);
		}
	}

	return discovered;
}


void TileMap::removeOreDepositLocation(const NAS2D::Point<int>& location)
{
	auto& tile = getTile({location, 0});
	if (!tile.hasOreDeposit())
	{
		throw std::runtime_error("No ore deposit found to remove");
	}

	mOreDeposits.erase(find_if(mOreDeposits.begin(), mOreDeposits.end(), [location](OreDeposit* oreDeposit){ return oreDeposit->location() == location; }));
	auto* oreDeposit = tile.oreDeposit();
	tile.removeOreDeposit();
	delete oreDeposit;
}


void TileMap::serialize(NAS2D::Xml::XmlElement* element)
{
	// ==========================================
	// ORE DEPOSITS (MINES)
	// ==========================================
	auto* oreDeposits = new NAS2D::Xml::XmlElement("mines");
	element->linkEndChild(oreDeposits);

	for (const auto* oreDeposit : mOreDeposits)
	{
		oreDeposits->linkEndChild(serializeOreDeposit(*oreDeposit));
	}

	auto* hiddenOreDeposits = new NAS2D::Xml::XmlElement("hidden_mines");
	element->linkEndChild(hiddenOreDeposits);
	for (const auto& deposit : mUndiscoveredOreDeposits)
	{
		hiddenOreDeposits->linkEndChild(NAS2D::dictionaryToAttributes(
			"mine",
			NAS2D::Dictionary{{
				{"x", deposit.location.x},
				{"y", deposit.location.y},
				{"yield", static_cast<int>(deposit.yield)},
			}}
		));
	}


	// ==========================================
	// TILES
	// ==========================================
	auto* tiles = new NAS2D::Xml::XmlElement("tiles");
	element->linkEndChild(tiles);

	// We're only writing out tiles that don't have structures or robots in them that are
	// underground and excavated or surface and bulldozed.
	for (int depth = 0; depth <= maxDepth(); ++depth)
	{
		for (const auto point : NAS2D::PointInRectangleRange{area()})
		{
			auto& tile = getTile({point, depth});
			if (
				((depth > 0 && tile.excavated()) || tile.isBulldozed()) &&
				(!tile.hasMapObject() && !tile.hasOreDeposit())
			)
			{
				tiles->linkEndChild(
					NAS2D::dictionaryToAttributes(
						"tile",
						NAS2D::Dictionary{{
							{"x", point.x},
							{"y", point.y},
							{"depth", depth},
							{"index", static_cast<int>(tile.index())},
						}}
					)
				);
			}
		}
	}
}


void TileMap::deserialize(NAS2D::Xml::XmlElement* element)
{
	// Ore deposits
	for (auto* oreDepositElement = element->firstChildElement("mines")->firstChildElement("mine"); oreDepositElement; oreDepositElement = oreDepositElement->nextSiblingElement())
	{
		OreDeposit* oreDeposit = new OreDeposit(deserializeOreDeposit(oreDepositElement));

		auto& tile = getTile({oreDeposit->location(), 0});
		tile.placeOreDeposit(oreDeposit);
		tile.bulldoze();

		mOreDeposits.push_back(oreDeposit);
	}

	if (auto* hiddenOreDeposits = element->firstChildElement("hidden_mines"))
	{
		for (auto* hiddenDepositElement = hiddenOreDeposits->firstChildElement("mine"); hiddenDepositElement; hiddenDepositElement = hiddenDepositElement->nextSiblingElement())
		{
			const auto dictionary = NAS2D::attributesToDictionary(*hiddenDepositElement);
			mUndiscoveredOreDeposits.push_back(UndiscoveredOreDeposit{
				{dictionary.get<int>("x"), dictionary.get<int>("y")},
				static_cast<OreDepositYield>(dictionary.get<int>("yield")),
			});
		}
	}
	else
	{
		constexpr OreDepositYields defaultYields{30, 50, 20};
		populateUndiscoveredOreDeposits(mOreDeposits.size(), defaultYields);
	}

	// Tiles indexes
	for (auto* tileElement = element->firstChildElement("tiles")->firstChildElement("tile"); tileElement; tileElement = tileElement->nextSiblingElement())
	{
		const auto tileDictionary = NAS2D::attributesToDictionary(*tileElement);

		const auto x = tileDictionary.get<int>("x");
		const auto y = tileDictionary.get<int>("y");
		const auto depth = tileDictionary.get<int>("depth");
		const auto index = tileDictionary.get<int>("index");

		auto& tile = getTile({{x, y}, depth});
		tile.index(static_cast<TerrainType>(std::clamp(index, 0, 4)));

		if (depth > 0) { tile.excavate(); }
	}
}


std::size_t TileMap::linearSize() const
{
	const auto convertedSize = mSizeInTiles.to<std::size_t>();
	const auto adjustedZ = mMaxDepth + 1;
	return convertedSize.x * convertedSize.y * static_cast<std::size_t>(adjustedZ);
}


std::size_t TileMap::linearIndex(const MapCoordinate& position) const
{
	const auto convertedSize = mSizeInTiles.to<std::size_t>();
	const auto convertedPosition = position.xy.to<std::size_t>();
	const auto convertedZ = static_cast<std::size_t>(position.z);
	return ((convertedZ * convertedSize.y) + convertedPosition.y) * convertedSize.x + convertedPosition.x;
}


void TileMap::buildTerrainMap(const std::string& path)
{
	const NAS2D::Image heightmap(path + MapTerrainExtension);

	const auto mapArea = mSizeInTiles.to<std::size_t>();
	const auto tilesPerLevel = mapArea.x * mapArea.y;
	mTileMap.reserve(tilesPerLevel * static_cast<std::size_t>(mMaxDepth + 1 + ReservedExtraDepthLevels));
	mTileMap.resize(linearSize());

	/**
	 * Builds a terrain map based on the pixel color values in
	 * a maps height map.
	 *
	 * Height maps by default are in grey-scale. This method assumes
	 * that all channels are the same value so it only looks at the red.
	 * Color values are divided by 50 to get a height value from 1 - 4.
	 */
	for (int depth = 0; depth <= mMaxDepth; depth++)
	{
		for (const auto point : NAS2D::PointInRectangleRange{area()})
		{
			auto color = heightmap.pixelColor(point);
			auto& tile = getTile({point, depth});
			const auto terrainType = static_cast<TerrainType>(std::clamp(color.red / 50, 1, 4));
			tile = {{point, depth}, terrainType};
			if (depth == 0) { tile.excavate(); }
		}
	}
}
