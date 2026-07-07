#include "GameSettings.h"

#include <NAS2D/Configuration.h>
#include <NAS2D/EnumKeyCode.h>
#include <NAS2D/Utility.h>

#include <algorithm>
#include <cctype>
#include <string>


namespace
{
	constexpr auto ConfigSection = "game";

	NAS2D::KeyCode keyFromConfig(const NAS2D::Dictionary& dict, const std::string& key, NAS2D::KeyCode defaultKey)
	{
		if (!dict.has(key)) { return defaultKey; }
		return static_cast<NAS2D::KeyCode>(dict.get<int>(key));
	}


	void setKeyInConfig(NAS2D::Dictionary& dict, const std::string& key, NAS2D::KeyCode keyCode)
	{
		dict.set(key, static_cast<int>(keyCode));
	}
}


NAS2D::KeyCode GameSettings::robotSelectKey(RobotTypeIndex robotType) const
{
	switch (robotType)
	{
	case RobotTypeIndex::Digger:
		return mDiggerKey;
	case RobotTypeIndex::Dozer:
		return mDozerKey;
	case RobotTypeIndex::Miner:
		return mMinerKey;
	default:
		return NAS2D::KeyCode::Unknown;
	}
}


void GameSettings::setRobotSelectKey(RobotTypeIndex robotType, NAS2D::KeyCode key)
{
	switch (robotType)
	{
	case RobotTypeIndex::Digger:
		mDiggerKey = key;
		break;
	case RobotTypeIndex::Dozer:
		mDozerKey = key;
		break;
	case RobotTypeIndex::Miner:
		mMinerKey = key;
		break;
	default:
		break;
	}
}


void GameSettings::setPlaceRobotKey(NAS2D::KeyCode key)
{
	mPlaceRobotKey = key;
}


void GameSettings::setAutosaveIntervalTurns(int turns)
{
	mAutosaveIntervalTurns = std::clamp(turns, MinAutosaveIntervalTurns, MaxAutosaveIntervalTurns);
}


std::optional<RobotTypeIndex> GameSettings::robotForKey(NAS2D::KeyCode key) const
{
	if (key == mDiggerKey) { return RobotTypeIndex::Digger; }
	if (key == mDozerKey) { return RobotTypeIndex::Dozer; }
	if (key == mMinerKey) { return RobotTypeIndex::Miner; }
	return std::nullopt;
}


bool GameSettings::isRobotBindingKey(NAS2D::KeyCode key) const
{
	return robotForKey(key).has_value();
}


bool GameSettings::isPlaceRobotKey(NAS2D::KeyCode key) const
{
	return key == mPlaceRobotKey;
}


bool GameSettings::conflictsWithReservedKey(NAS2D::KeyCode key) const
{
	switch (key)
	{
	case NAS2D::KeyCode::Escape:
	case NAS2D::KeyCode::Enter:
	case NAS2D::KeyCode::W:
	case NAS2D::KeyCode::A:
	case NAS2D::KeyCode::S:
	case NAS2D::KeyCode::D:
	case NAS2D::KeyCode::Up:
	case NAS2D::KeyCode::Down:
	case NAS2D::KeyCode::Left:
	case NAS2D::KeyCode::Right:
	case NAS2D::KeyCode::PageUp:
	case NAS2D::KeyCode::PageDown:
	case NAS2D::KeyCode::Home:
	case NAS2D::KeyCode::End:
	case NAS2D::KeyCode::F1:
	case NAS2D::KeyCode::F2:
	case NAS2D::KeyCode::F3:
	case NAS2D::KeyCode::F4:
	case NAS2D::KeyCode::F5:
	case NAS2D::KeyCode::F6:
	case NAS2D::KeyCode::F7:
	case NAS2D::KeyCode::F8:
	case NAS2D::KeyCode::F9:
	case NAS2D::KeyCode::F10:
	case NAS2D::KeyCode::R:
	case NAS2D::KeyCode::Num0:
	case NAS2D::KeyCode::Num1:
	case NAS2D::KeyCode::Num2:
	case NAS2D::KeyCode::Num3:
	case NAS2D::KeyCode::Num4:
		return true;
	default:
		return false;
	}
}


bool GameSettings::isKeyInUse(NAS2D::KeyCode key, RobotTypeIndex ignoreRobot) const
{
	if (key == mPlaceRobotKey) { return true; }

	const auto isRobotKey = [&](RobotTypeIndex robotType, NAS2D::KeyCode robotKey)
	{
		return robotType != ignoreRobot && key == robotKey;
	};

	return isRobotKey(RobotTypeIndex::Digger, mDiggerKey) ||
		isRobotKey(RobotTypeIndex::Dozer, mDozerKey) ||
		isRobotKey(RobotTypeIndex::Miner, mMinerKey);
}


void GameSettings::loadFromConfig()
{
	auto& config = NAS2D::Utility<NAS2D::Configuration>::get();
	const auto& game = config[ConfigSection];

	mDiggerKey = keyFromConfig(game, "key-digger", NAS2D::KeyCode::G);
	mDozerKey = keyFromConfig(game, "key-dozer", NAS2D::KeyCode::Z);
	mMinerKey = keyFromConfig(game, "key-miner", NAS2D::KeyCode::M);
	mPlaceRobotKey = keyFromConfig(game, "key-place-robot", NAS2D::KeyCode::Space);

	mAutosaveEnabled = game.get("autosave-enabled", false);
	mAutosaveIntervalTurns = game.get("autosave-interval-turns", DefaultAutosaveIntervalTurns);
	setAutosaveIntervalTurns(mAutosaveIntervalTurns);
}


void GameSettings::saveToConfig() const
{
	auto& config = NAS2D::Utility<NAS2D::Configuration>::get();
	auto& game = config[ConfigSection];

	setKeyInConfig(game, "key-digger", mDiggerKey);
	setKeyInConfig(game, "key-dozer", mDozerKey);
	setKeyInConfig(game, "key-miner", mMinerKey);
	setKeyInConfig(game, "key-place-robot", mPlaceRobotKey);

	game.set("autosave-enabled", mAutosaveEnabled);
	game.set("autosave-interval-turns", mAutosaveIntervalTurns);
}


void GameSettings::resetToDefaults()
{
	mDiggerKey = NAS2D::KeyCode::G;
	mDozerKey = NAS2D::KeyCode::Z;
	mMinerKey = NAS2D::KeyCode::M;
	mPlaceRobotKey = NAS2D::KeyCode::Space;
	mAutosaveEnabled = false;
	mAutosaveIntervalTurns = DefaultAutosaveIntervalTurns;
}


std::string keyCodeDisplayName(NAS2D::KeyCode key)
{
	const auto keyValue = static_cast<uint32_t>(key);

	if (keyValue >= static_cast<uint32_t>(NAS2D::KeyCode::A) &&
		keyValue <= static_cast<uint32_t>(NAS2D::KeyCode::Z))
	{
		return std::string{1, static_cast<char>(std::toupper(static_cast<unsigned char>(keyValue)))};
	}

	if (keyValue >= static_cast<uint32_t>(NAS2D::KeyCode::Num0) &&
		keyValue <= static_cast<uint32_t>(NAS2D::KeyCode::Num9))
	{
		return std::string{1, static_cast<char>(keyValue)};
	}

	switch (key)
	{
	case NAS2D::KeyCode::Space:
		return "Space";
	case NAS2D::KeyCode::Escape:
		return "Esc";
	case NAS2D::KeyCode::Enter:
		return "Enter";
	case NAS2D::KeyCode::Tab:
		return "Tab";
	case NAS2D::KeyCode::Backspace:
		return "Backspace";
	default:
		return "?";
	}
}