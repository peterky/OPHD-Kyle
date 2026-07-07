#include "ResearchWiki.h"

#include <libOPHD/Technology/ResearchTracker.h>
#include <libOPHD/Technology/TechnologyCatalog.h>

#include <algorithm>
#include <cmath>
#include <sstream>


namespace
{
	std::string formatPercent(float value)
	{
		const auto percent = static_cast<int>(std::abs(value) * 100.0f + 0.5f);
		return std::to_string(percent) + "%";
	}


	std::string categoryNameForTech(const TechnologyCatalog& catalog, int techId)
	{
		for (const auto& category : catalog.categories())
		{
			for (const auto& technology : category.technologies)
			{
				if (technology.id == techId) { return category.name; }
			}
		}

		return "Unknown";
	}
}


std::string modifierBenefitDescription(const Technology::Modifier& modifier)
{
	switch (modifier.modifies)
	{
	case Technology::Modifier::Modifies::AgricultureEfficiency:
		return "+" + formatPercent(modifier.value) + " food production";
	case Technology::Modifier::Modifies::BreakdownRate:
		return "-" + formatPercent(modifier.value) + " structure breakdown chance";
	case Technology::Modifier::Modifies::EducationEfficiency:
		return "+" + formatPercent(modifier.value) + " education / graduate throughput";
	case Technology::Modifier::Modifies::ExplorerEfficiency:
		return "+" + formatPercent(modifier.value) + " explorer survey yield";
	case Technology::Modifier::Modifies::MaintenanceCost:
		return "-" + formatPercent(modifier.value) + " maintenance supply cost";
	case Technology::Modifier::Modifies::MiningEfficiency:
		return "+" + formatPercent(modifier.value) + " mine production";
	case Technology::Modifier::Modifies::OreHaulEfficiency:
		return "+" + formatPercent(modifier.value) + " ore haul truck capacity";
	case Technology::Modifier::Modifies::PopulationFertility:
		return "+" + formatPercent(modifier.value) + " population fertility";
	case Technology::Modifier::Modifies::PopulationMorale:
		return "+" + formatPercent(modifier.value) + " morale per turn";
	case Technology::Modifier::Modifies::PopulationMortality:
		return "-" + formatPercent(modifier.value) + " mortality";
	case Technology::Modifier::Modifies::RecyclingEfficiency:
		return "+" + formatPercent(modifier.value) + " recycling recovery";
	case Technology::Modifier::Modifies::SmelterEfficiency:
		return "+" + formatPercent(modifier.value) + " smelter throughput";
	case Technology::Modifier::Modifies::StructureCost:
		return "-" + formatPercent(modifier.value) + " structure build cost";
	case Technology::Modifier::Modifies::StructureDecay:
		return "-" + formatPercent(modifier.value) + " structure decay";
	}

	return "Colony modifier";
}


std::string unlockBenefitDescription(const Technology::Unlock& unlock)
{
	if (unlock.unlocks == Technology::Unlock::Unlocks::Structure)
	{
		if (unlock.value == "CommTower2") { return "+25% communications tower range"; }
		if (unlock.value == "CommTower3") { return "+35% communications tower range (stacks with Mark II)"; }
		if (unlock.value == "comm_tower_build_range") { return "Build structures within comm tower range"; }
		if (unlock.value == "Smelter2") { return "+20% smelter throughput (existing smelters upgrade in place)"; }
		if (unlock.value == "SolarPanel2") { return "+25% solar panel output"; }
		if (unlock.value == "RobotCommand2") { return "+50% robot command center capacity"; }
		if (unlock.value == "FusionReactor") { return "Unlocks Fusion Reactor power structure"; }
		if (unlock.value == "Nursery") { return "Unlocks Nursery structure"; }
		if (unlock.value == "factory_efficiency_upgrade") { return "+25% factory production speed"; }
		if (unlock.value == "research_efficiency_upgrade") { return "+25% research points per turn"; }
		return "Unlocks structure: " + unlock.value;
	}

	if (unlock.unlocks == Technology::Unlock::Unlocks::Robot)
	{
		if (unlock.value == "RID_EXPLORER_BOT") { return "Unlocks RoboExplorer for hidden mine surveys"; }
		if (unlock.value == "robot_power_refit") { return "+15% robot task speed"; }
		if (unlock.value == "robot_vision_upgrade") { return "+25% robot task speed"; }
		if (unlock.value == "legged_robots_clearance" || unlock.value == "legged_robots_rough_terrain") { return "Legged robots can clear rough terrain"; }
		if (unlock.value == "humanoid_worker") { return "+10% factory production speed"; }
		return "Unlocks robot upgrade: " + unlock.value;
	}

	if (unlock.unlocks == Technology::Unlock::Unlocks::Satellite)
	{
		if (unlock.value == "orbital_survey") { return "Launch Orbital Survey satellite (reveals hidden mines)"; }
		if (unlock.value == "deep_scan") { return "Launch Deep Scan satellite (more mines + deeper digging)"; }
		return "Unlocks satellite: " + unlock.value;
	}

	if (unlock.unlocks == Technology::Unlock::Unlocks::DisasterPrediction)
	{
		if (unlock.value == "plague_immunity") { return "Permanent plague immunity"; }
		return "Predicts disaster: " + unlock.value;
	}

	if (unlock.unlocks == Technology::Unlock::Unlocks::Vehicle)
	{
		return "Unlocks vehicle: " + unlock.value;
	}

	return unlock.value;
}


std::vector<std::string> downstreamTechnologies(const TechnologyCatalog& catalog, int techId)
{
	std::vector<std::string> downstream;

	for (const auto& category : catalog.categories())
	{
		for (const auto& technology : category.technologies)
		{
			const auto found = std::find(technology.requiredTechnologies.begin(), technology.requiredTechnologies.end(), techId);
			if (found != technology.requiredTechnologies.end())
			{
				downstream.push_back(technology.name + " (" + category.name + ")");
			}
		}
	}

	std::sort(downstream.begin(), downstream.end());
	return downstream;
}


std::string summarizeTechnologyEffects(const Technology& technology, const TechnologyCatalog& catalog)
{
	std::ostringstream summary;
	bool hasEffect = false;

	if (!technology.modifiers.empty())
	{
		hasEffect = true;
		summary << "Modifiers:\n";
		for (const auto& modifier : technology.modifiers)
		{
			summary << "  - " << modifierBenefitDescription(modifier) << "\n";
		}
	}

	if (!technology.unlocks.empty())
	{
		hasEffect = true;
		summary << "Unlocks:\n";
		for (const auto& unlock : technology.unlocks)
		{
			summary << "  - " << unlockBenefitDescription(unlock) << "\n";
		}
	}

	const auto downstream = downstreamTechnologies(catalog, technology.id);
	if (!downstream.empty())
	{
		summary << "Enables research:\n";
		for (const auto& entry : downstream)
		{
			summary << "  - " << entry << "\n";
		}
	}

	if (!hasEffect && downstream.empty())
	{
		summary << "No direct colony bonus. This topic is a prerequisite for later research.\n";
	}

	return summary.str();
}


std::string buildTechIndexText(const TechnologyCatalog& catalog, const ResearchTracker& tracker)
{
	std::ostringstream index;
	index << "Colony Technology Index\n";
	index << "Browse every research topic, its payoff, and completion status.\n\n";

	for (const auto& category : catalog.categories())
	{
		index << category.name << "\n";

		for (const auto& technology : category.technologies)
		{
			const auto status = tracker.isCompleted(technology.id) ? "[Done]" :
				tracker.isBeingResearched(technology.id) ? "[Active]" :
				tracker.isQueued(technology.id) ? "[Queued]" :
				tracker.prerequisitesMet(technology.id, catalog) ? "[Ready]" : "[Locked]";

			index << "  " << status << " " << technology.name << " (" << technology.cost << " pts, "
				<< (technology.labType == 0 ? "UG Lab" : "Hot Lab") << ")\n";

			if (!technology.modifiers.empty() || !technology.unlocks.empty())
			{
				if (!technology.modifiers.empty())
				{
					for (const auto& modifier : technology.modifiers)
					{
						index << "      * " << modifierBenefitDescription(modifier) << "\n";
					}
				}

				if (!technology.unlocks.empty())
				{
					for (const auto& unlock : technology.unlocks)
					{
						index << "      * " << unlockBenefitDescription(unlock) << "\n";
					}
				}
			}
			else
			{
				const auto downstream = downstreamTechnologies(catalog, technology.id);
				if (!downstream.empty())
				{
					index << "      * Prerequisite for " << downstream.front();
					if (downstream.size() > 1)
					{
						index << " +" << (downstream.size() - 1) << " more";
					}
					index << "\n";
				}
				else
				{
					index << "      * Prerequisite / gate technology\n";
				}
			}

			if (!technology.requiredTechnologies.empty())
			{
				index << "      Needs: ";
				bool first = true;
				for (const auto prerequisiteId : technology.requiredTechnologies)
				{
					if (!first) { index << ", "; }
					first = false;
					index << catalog.technologyFromId(prerequisiteId).name;
				}
				index << "\n";
			}
		}

		index << "\n";
	}

	return index.str();
}