#pragma once

#include <libControls/Window.h>
#include <libControls/Button.h>
#include <libControls/Label.h>
#include <libControls/TextField.h>


class SkipTurnsDialog : public Window
{
public:
	using ConfirmHandler = NAS2D::Delegate<void(int turns)>;

	SkipTurnsDialog(ConfirmHandler confirmHandler);
	~SkipTurnsDialog() override;

	void show() override;
	void refreshWarning(int food, int productionPerTurn, int consumptionPerTurn);

protected:
	void onKeyDown(NAS2D::KeyCode key, NAS2D::KeyModifier mod, bool repeat);

private:
	void onConfirm();
	void onCancel();

	ConfirmHandler mConfirmHandler;
	Label lblPrompt;
	Label lblWarning;
	TextField txtTurns;
	Button btnConfirm;
	Button btnCancel;
};