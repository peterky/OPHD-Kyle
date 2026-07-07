#pragma once

#include <libOPHD/Technology/Technology.h>

#include <string>
#include <vector>


class TechnologyCatalog;
class ResearchTracker;


std::string modifierBenefitDescription(const Technology::Modifier& modifier);
std::string unlockBenefitDescription(const Technology::Unlock& unlock);
std::vector<std::string> downstreamTechnologies(const TechnologyCatalog& catalog, int techId);
std::string summarizeTechnologyEffects(const Technology& technology, const TechnologyCatalog& catalog);
std::string buildTechIndexText(const TechnologyCatalog& catalog, const ResearchTracker& tracker);