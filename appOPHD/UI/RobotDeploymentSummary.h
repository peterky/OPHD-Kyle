#pragma once

#include <libControls/Control.h>


namespace NAS2D
{
	class Image;
}

class RobotPool;
class StructureManager;


class RobotDeploymentSummary : public Control
{
public:
	RobotDeploymentSummary(const RobotPool& robotPool, const StructureManager& structureManager);

	void showExplorer(bool showExplorer) { mShowExplorer = showExplorer; }

	void draw(NAS2D::Renderer& renderer) const override;

private:
	const RobotPool& mRobotPool;
	const StructureManager& mStructureManager;
	const NAS2D::Image& mRobotIcons;
	const NAS2D::Image& mStructureIcons;
	const NAS2D::Image& mTruckIcon;
	bool mShowExplorer{false};
};