#include "CheatMenu.h"

#include <NAS2D/Math/Vector.h>
#include <NAS2D/Resource/Font.h>

#include <algorithm>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>


namespace
{
	struct CheatCodeEntry
	{
		std::string code;
		CheatMenu::CheatCode cheat;
		std::string description;
	};

	const std::vector<CheatCodeEntry>& cheatCodeEntries()
	{
		static const std::vector<CheatCodeEntry> entries{
			{"goldrush", CheatMenu::CheatCode::AddResources, "Add 1000 of each resource"},
			{"orderpizza", CheatMenu::CheatCode::AddFood, "Refill all food storage"},
			{"waveofbabies", CheatMenu::CheatCode::AddChildren, "Add 10 children"},
			{"savedbythebell", CheatMenu::CheatCode::AddStudents, "Add 10 students"},
			{"gettowork", CheatMenu::CheatCode::AddWorkers, "Add 10 workers"},
			{"smartypants", CheatMenu::CheatCode::AddScientists, "Add 10 scientists"},
			{"electronicoldmen", CheatMenu::CheatCode::AddRetired, "Add 10 retired colonists"},
			{"dropchildren", CheatMenu::CheatCode::RemoveChildren, "Remove 10 children"},
			{"dropstudents", CheatMenu::CheatCode::RemoveStudents, "Remove 10 students"},
			{"dropworkers", CheatMenu::CheatCode::RemoveWorkers, "Remove 10 workers"},
			{"dropscientists", CheatMenu::CheatCode::RemoveScientists, "Remove 10 scientists"},
			{"dropretirees", CheatMenu::CheatCode::RemoveRetired, "Remove 10 retired colonists"},
			{"beepboop", CheatMenu::CheatCode::AddRobots, "Add a RoboDigger, RoboMiner, RoboDozer, and RoboExplorer"},
		};
		return entries;
	}

	const auto maxCheatLength = std::ranges::max(std::views::transform(cheatCodeEntries(), [](const CheatCodeEntry& entry) { return entry.code.size(); }));

	std::string formatCheatCodeList()
	{
		std::ostringstream stream;
		stream << "Available codes:\n";
		for (const auto& entry : cheatCodeEntries())
		{
			stream << "  " << entry.code << " - " << entry.description << '\n';
		}
		return stream.str();
	}


}


CheatMenu::CheatCode CheatMenu::cheatCodeFromString(const std::string& cheatCode)
{
	for (const auto& entry : cheatCodeEntries())
	{
		if (entry.code == cheatCode) { return entry.cheat; }
	}

	return CheatMenu::CheatCode::Invalid;
}


CheatMenu::CheatMenu(CheatDelegate cheatHandler) :
	Window{"Cheating"},
	mCheatHandler{cheatHandler},
	mLabelCheatCode{"Code:"},
	txtCheatCode{maxCheatLength, {}, {this, &CheatMenu::onEnter}},
	btnOkay{"Okay", {this, &CheatMenu::onOkay}},
	mCheatCodeList{}
{
	btnOkay.size(btnOkay.size() + NAS2D::Vector{6, 2});

	constexpr auto margin = 10;
	constexpr auto contentWidth = 400;
	const auto fontHeight = Control::getDefaultFont().height();
	const auto listHeight = fontHeight * static_cast<int>(cheatCodeEntries().size() + 1);

	const auto inputRowY = sWindowTitleBarHeight + margin;
	add(mLabelCheatCode, {margin, inputRowY});
	add(txtCheatCode, {mLabelCheatCode.area().endPoint().x + 6, inputRowY});
	add(btnOkay, {txtCheatCode.area().endPoint().x + 6, inputRowY});

	const auto listY = inputRowY + txtCheatCode.size().y + margin;
	mCheatCodeList.size(NAS2D::Vector{contentWidth, listHeight});
	add(mCheatCodeList, {margin, listY});
	mCheatCodeList.text(formatCheatCodeList());

	size(NAS2D::Vector{contentWidth + margin * 2, listY + listHeight + margin});
}


void CheatMenu::onVisibilityChange(bool visible)
{
	if (visible)
	{
		txtCheatCode.hasFocus(true);
	}
}


void CheatMenu::onOkay()
{
	if (mCheatHandler) { mCheatHandler(cheatCodeFromString(txtCheatCode.text())); }
	txtCheatCode.clear();
	hide();
}


void CheatMenu::onEnter(TextField& /*textField*/)
{
	onOkay();
}