#include "MapViewState.h"

#include "../AutomationBridge.h"
#include "../ColonyDiagnostics.h"
#include "../UI/CheatMenu.h"
#include "../UI/FileIo.h"

#include <algorithm>
#include <sstream>


void MapViewState::queueNextAvailableResearch()
{
	for (const auto& category : mTechnologyReader.categories())
	{
		for (const auto& technology : category.technologies)
		{
			if (mResearchTracker.isCompleted(technology.id)) { continue; }
			if (mResearchTracker.isBeingResearched(technology.id)) { continue; }
			if (mResearchTracker.isQueued(technology.id)) { continue; }
			if (!mResearchTracker.prerequisitesMet(technology.id, mTechnologyReader)) { continue; }

			if (mResearchTracker.startOrQueueResearch(technology.id, technology.labType, mTechnologyReader))
			{
				ColonyDiagnostics::logEvent("automation: research " + technology.name);
				return;
			}
		}
	}

	ColonyDiagnostics::logEvent("automation: no research available to queue");
}


void MapViewState::processAutomationCommands()
{
	if (!active() || mGameOverDialog.visible()) { return; }

	std::vector<std::string> commands;
	if (!AutomationBridge::consumeCommands(commands)) { return; }

	for (const auto& command : commands)
	{
		std::istringstream stream{command};
		std::string verb;
		stream >> verb;

		if (verb == "turn")
		{
			int count{1};
			stream >> count;
			count = std::max(1, count);

			if (count == 1)
			{
				if (mBtnTurns.enabled()) { nextTurn(); }
			}
			else
			{
				skipTurns(count);
			}

			AutomationBridge::writeStatus("ok " + command);
			ColonyDiagnostics::logEvent("automation: " + command);
		}
		else if (verb == "cheat")
		{
			std::string code;
			stream >> code;
			onCheatCodeEntry(CheatMenu::cheatCodeFromString(code));
			AutomationBridge::writeStatus("ok " + command);
			ColonyDiagnostics::logEvent("automation: " + command);
		}
		else if (verb == "research_queue")
		{
			queueNextAvailableResearch();
			AutomationBridge::writeStatus("ok research_queue");
		}
		else if (verb == "escape")
		{
			if (mSkipTurnsDialog.visible()) { mSkipTurnsDialog.hide(); }
			if (mFileIoDialog.visible()) { mFileIoDialog.close(); }
			AutomationBridge::writeStatus("ok escape");
		}
		else
		{
			AutomationBridge::writeStatus("error unknown_command " + verb);
			ColonyDiagnostics::logEvent("automation: unknown command " + command);
		}
	}
}