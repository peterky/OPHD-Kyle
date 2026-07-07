#pragma once

#include <libOPHD/Technology/Technology.h>


class ResearchTracker;
class StructureTracker;
class TechnologyCatalog;


void applyStructureUnlocksFromTechnology(StructureTracker& tracker, const Technology& technology);

void syncStructureTrackerFromCompletedResearch(
	StructureTracker& tracker,
	const ResearchTracker& research,
	const TechnologyCatalog& catalog);