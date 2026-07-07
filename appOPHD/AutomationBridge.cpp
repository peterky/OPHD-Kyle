#include "AutomationBridge.h"

#include <NAS2D/Filesystem.h>
#include <NAS2D/Utility.h>

#include <fstream>
#include <sstream>


namespace
{
	const std::string CommandFile{"logs/automation.cmd"};
	const std::string StatusFile{"logs/automation.status"};


	std::string trim(const std::string& text)
	{
		const auto start = text.find_first_not_of(" \t\r\n");
		if (start == std::string::npos) { return {}; }
		const auto end = text.find_last_not_of(" \t\r\n");
		return text.substr(start, end - start + 1);
	}
}


namespace AutomationBridge
{
	bool consumeCommands(std::vector<std::string>& commands)
	{
		commands.clear();

		auto& filesystem = NAS2D::Utility<NAS2D::Filesystem>::get();
		const auto path = NAS2D::VirtualPath{CommandFile};
		if (!filesystem.exists(path)) { return false; }

		const auto diskPath = (filesystem.prefPath() / path).string();
		std::ifstream input{diskPath};
		if (!input.is_open()) { return false; }

		std::string line;
		while (std::getline(input, line))
		{
			const auto trimmed = trim(line);
			if (!trimmed.empty() && trimmed[0] != '#')
			{
				commands.push_back(trimmed);
			}
		}
		input.close();

		try
		{
			filesystem.del(path);
		}
		catch (...)
		{
			std::ofstream clear{diskPath, std::ios::trunc};
		}

		return !commands.empty();
	}


	void writeStatus(const std::string& status)
	{
		auto& filesystem = NAS2D::Utility<NAS2D::Filesystem>::get();
		filesystem.makeDirectory(NAS2D::VirtualPath{"logs"});
		const auto diskPath = (filesystem.prefPath() / NAS2D::VirtualPath{StatusFile}).string();

		std::ofstream output{diskPath, std::ios::trunc};
		if (output.is_open())
		{
			output << status << '\n';
		}
	}
}