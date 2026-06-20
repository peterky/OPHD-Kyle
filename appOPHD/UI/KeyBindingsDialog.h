#pragma once

#include <libControls/Window.h>
#include <libControls/Button.h>
#include <libControls/Label.h>

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
	void beginBinding(BindingTarget target);
	void assignKey(NAS2D::KeyCode key);
	void updateButtonText();

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
	Button btnBack;
};