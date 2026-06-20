#pragma once

#include "Technology.h"

#include <map>
#include <vector>


class TechnologyCatalog;


class ResearchTracker
{
public:
	struct ResearchProgress
	{
		int progress;
		int scientistsAssigned;
	};


	const std::vector<int>& completedResearch() const
	{
		return mCompleted;
	}


	const std::map<int, ResearchProgress>& currentResearch() const
	{
		return mTechnologiesBeingResearched;
	}


	const std::map<int, std::vector<int>>& researchQueue() const
	{
		return mResearchQueue;
	}


	const std::vector<int>& queuedForLabType(int labType) const;


	void addCompletedResearch(int techId)
	{
		mCompleted.push_back(techId);
	}


	bool isCompleted(int techId) const;
	bool isBeingResearched(int techId) const;
	bool isQueued(int techId) const;
	bool prerequisitesMet(int techId, const TechnologyCatalog& catalog) const;
	bool hasActiveResearchForLabType(int labType, const TechnologyCatalog& catalog) const;

	void startResearch(int techId, int progress, int assigned);
	void queueResearch(int techId, int labType);
	bool tryQueueResearch(int techId, int labType);
	bool startOrQueueResearch(int techId, int labType, const TechnologyCatalog& catalog);
	int popNextQueued(int labType);
	void updateResearch(int techId, int progress, int assigned);
	void completeResearch(int techId);
	const ResearchProgress& researchProgress(int techId) const;


private:
	std::vector<int> mCompleted;
	std::map<int, ResearchProgress> mTechnologiesBeingResearched;
	std::map<int, std::vector<int>> mResearchQueue;
};