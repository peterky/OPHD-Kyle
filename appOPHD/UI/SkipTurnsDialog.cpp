#include "SkipTurnsDialog.h"

#include "../ColonyFoodForecast.h"

#include <NAS2D/EnumKeyCode.h>
#include <NAS2D/EventHandler.h>
#include <NAS2D/Utility.h>
#include <NAS2D/Renderer/Color.h>

#include <cstdlib>


namespace
{
	constexpr int DefaultSkipTurns{5};
	constexpr int MinSkipTurns{1};
	constexpr int MaxSkipTurns{999};
}


SkipTurnsDialog::SkipTurnsDialog(ConfirmHandler confirmHandler) :
	Window{"Skip Turns"},
	mConfirmHandler{confirmHandler},
	lblPrompt{"Advance the colony by this many turns:"},
	lblWarning{""},
	txtTurns{3},
	btnConfirm{"Confirm", {this, &SkipTurnsDialog::onConfirm}},
	btnCancel{"Cancel", {this, &SkipTurnsDialog::onCancel}}
{
	size({360, 190});
	position({0, 0});
	anchored(true);

	txtTurns.text(std::to_string(DefaultSkipTurns));
	txtTurns.numbersOnly(true);
	txtTurns.border(TextField::BorderVisibility::Always);
	lblWarning.color(NAS2D::Color::Orange);

	const auto left = mRect.position.x + 12;
	auto rowY = mRect.position.y + sWindowTitleBarHeight + 12;

	add(lblPrompt, {left, rowY});
	rowY += 28;
	txtTurns.size({80, 24});
	add(txtTurns, {left, rowY});
	rowY += 36;

	add(lblWarning, {left, rowY});
	rowY += 28;

	btnConfirm.size({90, 24});
	btnCancel.size({90, 24});
	add(btnConfirm, {left, rowY});
	add(btnCancel, {left + 100, rowY});

	NAS2D::Utility<NAS2D::EventHandler>::get().keyDown().connect({this, &SkipTurnsDialog::onKeyDown});
}


SkipTurnsDialog::~SkipTurnsDialog()
{
	NAS2D::Utility<NAS2D::EventHandler>::get().keyDown().disconnect({this, &SkipTurnsDialog::onKeyDown});
}


void SkipTurnsDialog::show()
{
	txtTurns.text(std::to_string(DefaultSkipTurns));
	lblWarning.text("");
	Window::show();
}


void SkipTurnsDialog::refreshWarning(const int food, const int productionPerTurn, const int consumptionPerTurn)
{
	if (!visible()) { return; }

	const auto turns = std::atoi(txtTurns.text().c_str());
	const auto clamped = std::max(MinSkipTurns, std::min(turns > 0 ? turns : DefaultSkipTurns, MaxSkipTurns));

	if (consumptionPerTurn <= 0)
	{
		lblWarning.text("");
		return;
	}

	if (wouldStarveWithinTurns(food, productionPerTurn, consumptionPerTurn, clamped))
	{
		lblWarning.text("Warning: food may run out and colonists could starve.");
		lblWarning.color(NAS2D::Color::Red);
		return;
	}

	if (productionPerTurn < consumptionPerTurn)
	{
		lblWarning.text("Food production is below consumption — watch reserves.");
		lblWarning.color(NAS2D::Color::Orange);
		return;
	}

	lblWarning.text("");
}


void SkipTurnsDialog::onConfirm()
{
	const auto turns = std::atoi(txtTurns.text().c_str());
	const auto clamped = std::max(MinSkipTurns, std::min(turns > 0 ? turns : DefaultSkipTurns, MaxSkipTurns));
	hide();
	mConfirmHandler(clamped);
}


void SkipTurnsDialog::onCancel()
{
	hide();
}


void SkipTurnsDialog::onKeyDown(NAS2D::KeyCode key, NAS2D::KeyModifier /*mod*/, bool /*repeat*/)
{
	if (!visible()) { return; }

	if (key == NAS2D::KeyCode::Escape) { onCancel(); }
	if (key == NAS2D::KeyCode::Enter) { onConfirm(); }
}