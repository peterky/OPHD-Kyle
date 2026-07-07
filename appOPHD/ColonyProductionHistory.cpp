#include "ColonyProductionHistory.h"

#include <NAS2D/ParserHelper.h>
#include <NAS2D/Xml/XmlElement.h>


void ColonyProductionHistory::recordTurn(const ColonyTurnSnapshot& snapshot)
{
	mSnapshots.push_back(snapshot);
	if (mSnapshots.size() > MaxSnapshots)
	{
		mSnapshots.erase(mSnapshots.begin());
	}
}


void ColonyProductionHistory::clear()
{
	mSnapshots.clear();
}


NAS2D::Xml::XmlElement* ColonyProductionHistory::serialize() const
{
	auto* historyElement = new NAS2D::Xml::XmlElement("production_history");

	for (const auto& snapshot : mSnapshots)
	{
		historyElement->linkEndChild(NAS2D::dictionaryToAttributes(
			"snapshot",
			NAS2D::Dictionary{{
				{"turn", snapshot.turn},
				{"common", snapshot.refinedResources[0]},
				{"rare", snapshot.refinedResources[1]},
				{"uncommon", snapshot.refinedResources[2]},
				{"precious", snapshot.refinedResources[3]},
				{"food", snapshot.food},
				{"population", snapshot.population},
				{"trucks_available", snapshot.trucksAvailable},
				{"trucks_deployed", snapshot.trucksDeployed},
				{"robots_available", snapshot.robotsAvailable},
				{"robots_total", snapshot.robotsTotal},
			}}
		));
	}

	return historyElement;
}


void ColonyProductionHistory::deserialize(NAS2D::Xml::XmlElement* element)
{
	clear();

	if (!element) { return; }

	for (auto* snapshotElement = element->firstChildElement("snapshot"); snapshotElement; snapshotElement = snapshotElement->nextSiblingElement("snapshot"))
	{
		const auto dictionary = NAS2D::attributesToDictionary(*snapshotElement);

		ColonyTurnSnapshot snapshot;
		snapshot.turn = dictionary.get<int>("turn");
		snapshot.refinedResources = {
			dictionary.get<int>("common"),
			dictionary.get<int>("rare"),
			dictionary.get<int>("uncommon"),
			dictionary.get<int>("precious"),
		};
		snapshot.food = dictionary.get<int>("food");
		snapshot.population = dictionary.get<int>("population");
		snapshot.trucksAvailable = dictionary.get<int>("trucks_available");
		snapshot.trucksDeployed = dictionary.get<int>("trucks_deployed");
		snapshot.robotsAvailable = dictionary.get<int>("robots_available");
		snapshot.robotsTotal = dictionary.get<int>("robots_total");

		mSnapshots.push_back(snapshot);
	}
}