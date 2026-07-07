#pragma once

#include <string>
#include <vector>


namespace AutomationBridge
{
	bool consumeCommands(std::vector<std::string>& commands);
	void writeStatus(const std::string& status);
}