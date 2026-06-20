#include "ResearchTracker.h"

#include "TechnologyCatalog.h"

#include <algorithm>
#include <stdexcept>


bool ResearchTracker::isCompleted(int techId) const
{
	return std::find(mCompleted.begin(), mCompleted.end(), techId) != mCompleted.end();
}


bool ResearchTracker::isBeingResearched(int techId) const
{
	return mTechnologiesBeingResearched.find(techId) != mTechnologiesBeingResearched.end();
}


bool ResearchTracker::isQueued(int techId) const
{
	for (const auto& [labType, queue] : mResearchQueue)
	{
		(void)labType;
		if (std::find(queue.begin(), queue.end(), techId) != queue.end())
		{
			return true;
		}
	}

	return false;
}


bool ResearchTracker::prerequisitesMet(int techId, const TechnologyCatalog& catalog) const
{
	const auto& technology = catalog.technologyFromId(techId);

	for (const auto requiredTechId : technology.requiredTechnologies)
	{
		if (!isCompleted(requiredTechId))
		{
			return false;
		}
	}

	return true;
}


bool ResearchTracker::hasActiveResearchForLabType(int labType, const TechnologyCatalog& catalog) const
{
	for (const auto& [techId, values] : mTechnologiesBeingResearched)
	{
		(void)values;
		if (catalog.technologyFromId(techId).labType == labType)
		{
			return true;
		}
	}

	return false;
}


void ResearchTracker::startResearch(int techId, int progress, int assigned)
{
	if (std::find(mCompleted.begin(), mCompleted.end(), techId) != mCompleted.end())
	{
		throw std::runtime_error("Can't update a technology that's already researched.");
	}

	if (isQueued(techId))
	{
		for (auto& [labType, queue] : mResearchQueue)
		{
			queue.erase(std::remove(queue.begin(), queue.end(), techId), queue.end());
		}
	}

	mTechnologiesBeingResearched.insert_or_assign(techId, ResearchProgress{progress, assigned});
}


void ResearchTracker::queueResearch(int techId, int labType)
{
	if (isCompleted(techId) || isBeingResearched(techId) || isQueued(techId))
	{
		throw std::runtime_error("Can't queue a technology that's already researched or queued.");
	}

	mResearchQueue[labType].push_back(techId);
}


bool ResearchTracker::tryQueueResearch(int techId, int labType)
{
	if (isCompleted(techId) || isBeingResearched(techId) || isQueued(techId))
	{
		return false;
	}

	mResearchQueue[labType].push_back(techId);
	return true;
}


const std::vector<int>& ResearchTracker::queuedForLabType(int labType) const
{
	static const std::vector<int> emptyQueue;
	const auto queueIt = mResearchQueue.find(labType);
	return queueIt != mResearchQueue.end() ? queueIt->second : emptyQueue;
}


bool ResearchTracker::startOrQueueResearch(int techId, int labType, const TechnologyCatalog& catalog)
{
	if (isCompleted(techId) || isBeingResearched(techId) || isQueued(techId))
	{
		return false;
	}

	if (!prerequisitesMet(techId, catalog))
	{
		return false;
	}

	if (hasActiveResearchForLabType(labType, catalog))
	{
		queueResearch(techId, labType);
		return true;
	}

	startResearch(techId, 0, 0);
	return true;
}


int ResearchTracker::popNextQueued(int labType)
{
	auto queueIt = mResearchQueue.find(labType);
	if (queueIt == mResearchQueue.end() || queueIt->second.empty())
	{
		return -1;
	}

	const auto nextTechId = queueIt->second.front();
	queueIt->second.erase(queueIt->second.begin());
	return nextTechId;
}


void ResearchTracker::updateResearch(int techId, int progress, int assigned)
{
	if (std::find(mCompleted.begin(), mCompleted.end(), techId) != mCompleted.end())
	{
		throw std::runtime_error("Can't update a technology that's already researched.");
	}

	mTechnologiesBeingResearched.at(techId) = {progress, assigned};
}


void ResearchTracker::completeResearch(int techId)
{
	if (!isBeingResearched(techId))
	{
		throw std::runtime_error("Can't complete a technology that isn't being researched.");
	}

	mTechnologiesBeingResearched.erase(techId);
	addCompletedResearch(techId);
}


const ResearchTracker::ResearchProgress& ResearchTracker::researchProgress(int techId) const
{
	return mTechnologiesBeingResearched.at(techId);
}