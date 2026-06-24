#pragma once

#include <string>


struct ColonyTurnLogInfo
{
	int turn{0};
	int population{0};
	int structures{0};
	int completedResearch{0};
	int activeResearch{0};
	int queuedResearch{0};
	int resourcesTotal{0};
	int morale{0};
	int food{0};
	int robotsDeployed{0};
	int dozersIdle{0};
};


namespace ColonyDiagnostics
{
	void initialize();
	void shutdown(bool cleanShutdown);

	void beginSession(const std::string& sessionName);
	void endSession();

	void setTurnCount(int turn);
	void logTurn(const ColonyTurnLogInfo& info);
	void logEvent(const std::string& message);

	void notifyAutoSave(int turn, const std::string& savePath);
}