#pragma once

#include <libOPHD/StorableResources.h>

#include <array>
#include <string>
#include <vector>

namespace NAS2D::Xml
{
	class XmlElement;
}


struct ColonyTurnSnapshot
{
	int turn{0};
	std::array<int, 4> refinedResources{};
	int food{0};
	int population{0};
	int trucksAvailable{0};
	int trucksDeployed{0};
	int robotsAvailable{0};
	int robotsTotal{0};
};


class ColonyProductionHistory
{
public:
	static constexpr std::size_t MaxSnapshots{500};

	void recordTurn(const ColonyTurnSnapshot& snapshot);
	const std::vector<ColonyTurnSnapshot>& snapshots() const { return mSnapshots; }
	void clear();

	NAS2D::Xml::XmlElement* serialize() const;
	void deserialize(NAS2D::Xml::XmlElement* element);

private:
	std::vector<ColonyTurnSnapshot> mSnapshots;
};