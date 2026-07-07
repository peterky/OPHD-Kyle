#pragma once

#include <NAS2D/Math/Vector.h>

#include <NAS2D/Math/Point.h>

#include <optional>
#include <string>
#include <vector>
#include <array>


namespace NAS2D
{
	namespace Xml
	{
		class XmlElement;
	}

	template <typename BaseType> struct Point;
	template <typename BaseType> struct Rectangle;
}

enum class Direction;
enum class OreDepositYield;
struct MapCoordinate;
class Tile;
class OreDeposit;


class TileMap
{
public:
	using OreDepositYields = std::array<int, 3>; // {low, med, high}

	TileMap(const std::string& mapPath, int maxDepth, std::size_t oreDepositCount, const OreDepositYields& oreDepositYields);
	TileMap(const std::string& mapPath, int maxDepth);
	TileMap(const TileMap&) = delete;
	TileMap& operator=(const TileMap&) = delete;
	~TileMap();

	NAS2D::Rectangle<int> area() const;
	NAS2D::Vector<int> size() const { return mSizeInTiles; }
	int maxDepth() const { return mMaxDepth; }
	void extendMaxDepth(int additionalLevels);
	std::size_t undiscoveredOreDepositCount() const { return mUndiscoveredOreDeposits.size(); }

	bool isValidPosition(const MapCoordinate& position) const;

	const Tile& getTile(const MapCoordinate& position) const;
	Tile& getTile(const MapCoordinate& position);

	bool hasOreDeposit(const MapCoordinate& mapCoordinate) const;
	const std::vector<OreDeposit*>& oreDeposits() const;
	void removeOreDepositLocation(const NAS2D::Point<int>& location);

	/**
	 * Searches for an undiscovered ore deposit within \c searchRadius tiles of \c searchCenter.
	 * If found, places the deposit on the map and returns its location.
	 */
	std::optional<NAS2D::Point<int>> prospectForOreDeposit(NAS2D::Point<int> searchCenter, int searchRadius);
	std::vector<NAS2D::Point<int>> prospectForOreDeposits(NAS2D::Point<int> searchCenter, int searchRadius, int depositCount);
	std::optional<NAS2D::Point<int>> spawnEmergencyOreDeposit(NAS2D::Point<int> preferredCenter);

	void serialize(NAS2D::Xml::XmlElement* element);
	void deserialize(NAS2D::Xml::XmlElement* element);

protected:
	std::size_t linearSize() const;
	std::size_t linearIndex(const MapCoordinate& position) const;

	void buildTerrainMap(const std::string& path);
	void populateUndiscoveredOreDeposits(std::size_t oreDepositCount, const OreDepositYields& oreDepositYields);

private:
	const NAS2D::Vector<int> mSizeInTiles;
	std::string mMapPath;
	int mMaxDepth = 0;
	struct UndiscoveredOreDeposit
	{
		NAS2D::Point<int> location;
		OreDepositYield yield;
	};

	std::vector<Tile> mTileMap;
	std::vector<OreDeposit*> mOreDeposits;
	std::vector<UndiscoveredOreDeposit> mUndiscoveredOreDeposits;
};
