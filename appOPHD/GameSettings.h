#pragma once

#include "MapObjects/RobotTypeIndex.h"

#include <NAS2D/EnumKeyCode.h>

#include <optional>
#include <string>


class GameSettings
{
public:
	static constexpr int DefaultAutosaveIntervalTurns = 10;
	static constexpr int MinAutosaveIntervalTurns = 1;
	static constexpr int MaxAutosaveIntervalTurns = 9999;

	NAS2D::KeyCode robotSelectKey(RobotTypeIndex robotType) const;
	NAS2D::KeyCode placeRobotKey() const { return mPlaceRobotKey; }

	void setRobotSelectKey(RobotTypeIndex robotType, NAS2D::KeyCode key);
	void setPlaceRobotKey(NAS2D::KeyCode key);

	bool autosaveEnabled() const { return mAutosaveEnabled; }
	int autosaveIntervalTurns() const { return mAutosaveIntervalTurns; }

	void setAutosaveEnabled(bool enabled) { mAutosaveEnabled = enabled; }
	void setAutosaveIntervalTurns(int turns);

	std::optional<RobotTypeIndex> robotForKey(NAS2D::KeyCode key) const;
	bool isRobotBindingKey(NAS2D::KeyCode key) const;
	bool isPlaceRobotKey(NAS2D::KeyCode key) const;

	bool conflictsWithReservedKey(NAS2D::KeyCode key) const;
	bool isKeyInUse(NAS2D::KeyCode key, RobotTypeIndex ignoreRobot = RobotTypeIndex::None) const;

	void loadFromConfig();
	void saveToConfig() const;

	void resetToDefaults();

private:
	NAS2D::KeyCode mDiggerKey{NAS2D::KeyCode::G};
	NAS2D::KeyCode mDozerKey{NAS2D::KeyCode::Z};
	NAS2D::KeyCode mMinerKey{NAS2D::KeyCode::M};
	NAS2D::KeyCode mPlaceRobotKey{NAS2D::KeyCode::Space};

	bool mAutosaveEnabled{false};
	int mAutosaveIntervalTurns{DefaultAutosaveIntervalTurns};
};

std::string keyCodeDisplayName(NAS2D::KeyCode key);