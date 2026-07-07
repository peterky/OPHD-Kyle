#pragma once

#include "../ColonyMaintenanceStats.h"

#include <libControls/Control.h>


class MaintenanceTurnSummary : public Control
{
public:
	MaintenanceTurnSummary() = default;

	void setStats(const ColonyMaintenanceTurnStats& stats);
	void draw(NAS2D::Renderer& renderer) const override;

private:
	ColonyMaintenanceTurnStats mStats;
};