#pragma once

#include <libControls/Window.h>
#include <libControls/Button.h>
#include <libControls/CheckBox.h>
#include <libControls/Label.h>
#include <libControls/TextField.h>

#include <cstdint>


namespace NAS2D
{
	enum class KeyCode : uint32_t;
	enum class KeyModifier : uint16_t;
}


class GameSettings;


class KeyBindingsDialog : public Window
{
public:
	using BackHandler = NAS2D::Delegate<void()>;

	KeyBindingsDialog(GameSettings& settings, BackHandler backHandler);
	~KeyBindingsDialog() override;

	void refreshLabels();

	/** Handles Esc while this dialog is open (binding cancel or back to game menu). */
	bool handleEscapeKey();

protected:
	void onKeyDown(NAS2D::KeyCode key, NAS2D::KeyModifier mod, bool repeat);
	void onEnableChange() override;

private:
	enum class BindingTarget
	{
		None,
		Digger,
		Dozer,
		Miner,
		PlaceRobot
	};

	void onBindDigger();
	void onBindDozer();
	void onBindMiner();
	void onBindPlaceRobot();
	void onResetDefaults();
	void onAutoSaveToggle();
	void onAutoSaveIntervalChanged(TextField& field);
	void beginBinding(BindingTarget target);
	void assignKey(NAS2D::KeyCode key);
	void updateButtonText();
	void syncAutoSaveControls();

private:
	GameSettings& mSettings;
	BackHandler mBackHandler;

	BindingTarget mBindingTarget{BindingTarget::None};

	Label lblInstructions;
	Label lblDigger;
	Label lblDozer;
	Label lblMiner;
	Label lblPlaceRobot;

	Button btnDiggerKey;
	Button btnDozerKey;
	Button btnMinerKey;
	Button btnPlaceRobotKey;
	Button btnResetDefaults;

	Label lblAutoSaveSection;
	CheckBox chkAutoSave;
	Label lblAutoSaveInterval;
	TextField txtAutoSaveInterval;

	Button btnBack;
};