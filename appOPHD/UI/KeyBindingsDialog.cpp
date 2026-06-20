#include "KeyBindingsDialog.h"

#include "../GameSettings.h"
#include "../Constants/Strings.h"

#include <NAS2D/EnumKeyCode.h>
#include <NAS2D/EventHandler.h>
#include <NAS2D/Utility.h>

#include <array>


namespace
{
	constexpr auto buttonSize = NAS2D::Vector{120, 24};
	constexpr auto rowHeight = 28;
	constexpr auto labelWidth = 180;
}


KeyBindingsDialog::KeyBindingsDialog(GameSettings& settings, BackHandler backHandler) :
	Window{"Key Bindings"},
	mSettings{settings},
	mBackHandler{backHandler},
	lblInstructions{"Select a binding, then press a key. Defaults: G = Digger, Z = Dozer, M = Miner, Space = Place."},
	lblDigger{constants::Robodigger + ":"},
	lblDozer{constants::Robodozer + ":"},
	lblMiner{constants::Robominer + ":"},
	lblPlaceRobot{"Place at cursor:"},
	btnDiggerKey{"?", {this, &KeyBindingsDialog::onBindDigger}},
	btnDozerKey{"?", {this, &KeyBindingsDialog::onBindDozer}},
	btnMinerKey{"?", {this, &KeyBindingsDialog::onBindMiner}},
	btnPlaceRobotKey{"?", {this, &KeyBindingsDialog::onBindPlaceRobot}},
	btnResetDefaults{"Reset Defaults", {this, &KeyBindingsDialog::onResetDefaults}},
	btnBack{"Back", backHandler}
{
	position({0, 0});

	const auto left = mRect.position.x + 10;
	auto rowY = mRect.position.y + sWindowTitleBarHeight + 8;

	add(lblInstructions, {left, rowY});
	rowY += 28;

	const auto addBindingRow = [&](Label& label, Button& button, int y)
	{
		add(label, {left, y + 4});
		button.size(buttonSize);
		add(button, {left + labelWidth, y});
	};

	addBindingRow(lblDigger, btnDiggerKey, rowY);
	rowY += rowHeight;
	addBindingRow(lblDozer, btnDozerKey, rowY);
	rowY += rowHeight;
	addBindingRow(lblMiner, btnMinerKey, rowY);
	rowY += rowHeight;
	addBindingRow(lblPlaceRobot, btnPlaceRobotKey, rowY);
	rowY += rowHeight + 8;

	btnResetDefaults.size({buttonSize.x, buttonSize.y});
	add(btnResetDefaults, {left, rowY});
	rowY += rowHeight;

	btnBack.size({buttonSize.x, buttonSize.y});
	add(btnBack, {left, rowY});
	rowY += rowHeight + 10;

	size({labelWidth + buttonSize.x + 24, rowY - mRect.position.y});

	anchored(true);
	refreshLabels();

	NAS2D::Utility<NAS2D::EventHandler>::get().keyDown().connect({this, &KeyBindingsDialog::onKeyDown});
}


KeyBindingsDialog::~KeyBindingsDialog()
{
	NAS2D::Utility<NAS2D::EventHandler>::get().keyDown().disconnect({this, &KeyBindingsDialog::onKeyDown});
}


void KeyBindingsDialog::refreshLabels()
{
	updateButtonText();
}


void KeyBindingsDialog::updateButtonText()
{
	btnDiggerKey.text(keyCodeDisplayName(mSettings.robotSelectKey(RobotTypeIndex::Digger)));
	btnDozerKey.text(keyCodeDisplayName(mSettings.robotSelectKey(RobotTypeIndex::Dozer)));
	btnMinerKey.text(keyCodeDisplayName(mSettings.robotSelectKey(RobotTypeIndex::Miner)));
	btnPlaceRobotKey.text(keyCodeDisplayName(mSettings.placeRobotKey()));
}


void KeyBindingsDialog::beginBinding(BindingTarget target)
{
	mBindingTarget = target;
	lblInstructions.text("Press a key for this binding (Esc to cancel)...");
}


void KeyBindingsDialog::assignKey(NAS2D::KeyCode key)
{
	if (mBindingTarget == BindingTarget::None) { return; }

	if (key == NAS2D::KeyCode::Escape)
	{
		mBindingTarget = BindingTarget::None;
		lblInstructions.text("Select a binding, then press a key. Defaults: G = Digger, Z = Dozer, M = Miner, Space = Place.");
		return;
	}

	if (mSettings.conflictsWithReservedKey(key))
	{
		lblInstructions.text("That key is reserved for game controls. Choose another.");
		return;
	}

	switch (mBindingTarget)
	{
	case BindingTarget::Digger:
		if (mSettings.isKeyInUse(key, RobotTypeIndex::Digger))
		{
			lblInstructions.text("That key is already assigned.");
			return;
		}
		mSettings.setRobotSelectKey(RobotTypeIndex::Digger, key);
		break;
	case BindingTarget::Dozer:
		if (mSettings.isKeyInUse(key, RobotTypeIndex::Dozer))
		{
			lblInstructions.text("That key is already assigned.");
			return;
		}
		mSettings.setRobotSelectKey(RobotTypeIndex::Dozer, key);
		break;
	case BindingTarget::Miner:
		if (mSettings.isKeyInUse(key, RobotTypeIndex::Miner))
		{
			lblInstructions.text("That key is already assigned.");
			return;
		}
		mSettings.setRobotSelectKey(RobotTypeIndex::Miner, key);
		break;
	case BindingTarget::PlaceRobot:
		if (mSettings.isKeyInUse(key))
		{
			lblInstructions.text("That key is already assigned.");
			return;
		}
		mSettings.setPlaceRobotKey(key);
		break;
	default:
		break;
	}

	mBindingTarget = BindingTarget::None;
	lblInstructions.text("Select a binding, then press a key. Defaults: G = Digger, Z = Dozer, M = Miner, Space = Place.");
	updateButtonText();
}


void KeyBindingsDialog::onBindDigger()
{
	beginBinding(BindingTarget::Digger);
}


void KeyBindingsDialog::onBindDozer()
{
	beginBinding(BindingTarget::Dozer);
}


void KeyBindingsDialog::onBindMiner()
{
	beginBinding(BindingTarget::Miner);
}


void KeyBindingsDialog::onBindPlaceRobot()
{
	beginBinding(BindingTarget::PlaceRobot);
}


void KeyBindingsDialog::onResetDefaults()
{
	mSettings.resetToDefaults();
	updateButtonText();
}


void KeyBindingsDialog::onKeyDown(NAS2D::KeyCode key, NAS2D::KeyModifier /*mod*/, bool /*repeat*/)
{
	if (!visible()) { return; }

	if (mBindingTarget != BindingTarget::None)
	{
		assignKey(key);
		return;
	}

	if (key == NAS2D::KeyCode::Escape) { mBackHandler(); }
}


void KeyBindingsDialog::onEnableChange()
{
	lblInstructions.enabled(enabled());
	lblDigger.enabled(enabled());
	lblDozer.enabled(enabled());
	lblMiner.enabled(enabled());
	lblPlaceRobot.enabled(enabled());
	btnDiggerKey.enabled(enabled());
	btnDozerKey.enabled(enabled());
	btnMinerKey.enabled(enabled());
	btnPlaceRobotKey.enabled(enabled());
	btnResetDefaults.enabled(enabled());
	btnBack.enabled(enabled());
}