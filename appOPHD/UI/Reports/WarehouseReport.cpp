#include "WarehouseReport.h"

#include "../ProgressBar.h"

#include "../../CacheImage.h"
#include "../../CacheFont.h"
#include "../../StructureManager.h"
#include "../../Constants/Strings.h"
#include "../../Constants/UiConstants.h"
#include "../../MapObjects/Structures/Warehouse.h"

#include <NAS2D/Utility.h>
#include <NAS2D/EventHandler.h>
#include <NAS2D/EnumMouseButton.h>
#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/Resource/Font.h>

#include <string>
#include <iterator>
#include <algorithm>


namespace
{
	constexpr auto filterButtonSectionOffset = NAS2D::Vector{10, 10};
	constexpr auto filterButtonSize = NAS2D::Vector{94, 26};
	constexpr auto infoSectionOffset = filterButtonSectionOffset + NAS2D::Vector{0, filterButtonSize.y + 10};
	constexpr auto infoSectionHeight = 78;
	constexpr auto structureListBoxOffset = infoSectionOffset + NAS2D::Vector{0, infoSectionHeight + 12};
	constexpr auto rightPanelHeaderHeight = 118;


	template <typename Predicate>
	std::vector<Warehouse*> selectWarehouses(const StructureManager& structureManager, const Predicate& predicate)
	{
		const auto& warehouses = structureManager.getStructures<Warehouse>();

		std::vector<Warehouse*> output;
		std::copy_if(warehouses.begin(), warehouses.end(), std::back_inserter(output), predicate);

		return output;
	}


	std::string structureStateDescription(const Warehouse& warehouse)
	{
		const auto& productPool = warehouse.products();

		if (!warehouse.operational()) { return warehouse.stateDescription(); }
		else if (productPool.empty()) { return constants::WarehouseEmpty; }
		else if (productPool.atCapacity()) { return constants::WarehouseFull; }
		else if (!productPool.empty() && !productPool.atCapacity()) { return constants::WarehouseVacancy; }

		return std::string{};
	}
}


WarehouseReport::WarehouseReport(const StructureManager& structureManager, TakeMeThereDelegate takeMeThereHandler) :
	mStructureManager{structureManager},
	mTakeMeThereHandler{takeMeThereHandler},
	fontMedium{getFontMedium()},
	fontMediumBold{getFontMediumBold()},
	fontBigBold{getFontHugeBold()},
	imageWarehouse{getImage("structures/warehouse.png")},
	btnShowAll{"All", {this, &WarehouseReport::onShowAll}},
	btnFull{"Full", {this, &WarehouseReport::onFull}},
	btnVacancy{"Vacancy", {this, &WarehouseReport::onVacancy}},
	btnEmpty{"Empty", {this, &WarehouseReport::onEmpty}},
	btnDisabled{"Disabled", {this, &WarehouseReport::onDisabled}},
	btnTakeMeThere{constants::TakeMeThere, {this, &WarehouseReport::onTakeMeThere}},
	lstStructures{{this, &WarehouseReport::onStructureSelectionChange}}
{
	auto buttonOffset = filterButtonSectionOffset;
	for (auto button : {&btnShowAll, &btnFull, &btnVacancy, &btnEmpty, &btnDisabled})
	{
		button->size(filterButtonSize);
		button->type(Button::Type::Toggle);
		button->toggle(false);
		add(*button, buttonOffset);
		buttonOffset.x += button->size().x + constants::MarginTight;
	}

	btnShowAll.toggle(true);

	btnTakeMeThere.size({140, 30});

	NAS2D::Utility<NAS2D::EventHandler>::get().mouseDoubleClick().connect({this, &WarehouseReport::onDoubleClick});

	fillLists();

	add(btnTakeMeThere, {10, 10});
	add(lstStructures, structureListBoxOffset);
	add(lstProducts, {10, 10});
}


WarehouseReport::~WarehouseReport()
{
	NAS2D::Utility<NAS2D::EventHandler>::get().mouseDoubleClick().disconnect({this, &WarehouseReport::onDoubleClick});
}


bool WarehouseReport::canView(const Structure& structure)
{
	return dynamic_cast<const Warehouse*>(&structure);
}


void WarehouseReport::selectStructure(Structure& structure)
{
	lstStructures.setSelected(&structure);
}


void WarehouseReport::clearSelected()
{
	lstStructures.clearSelected();
}


/**
 * Inherited interface. A better name for this function would be
 * fillListWithAll() or something to that effect.
 */
void WarehouseReport::fillLists()
{
	fillListFromStructureList(selectWarehouses(mStructureManager, [](Warehouse*) { return true; }));
}


void WarehouseReport::refresh()
{
	onShowAll();
}


const Warehouse* WarehouseReport::selectedWarehouse() const
{
	return dynamic_cast<const Warehouse*>(lstStructures.selectedStructure());
}


void WarehouseReport::computeTotalWarehouseCapacity()
{
	int capacityTotal = 0;
	int capacityAvailable = 0;

	const auto& warehouses = mStructureManager.getStructures<Warehouse>();
	for (const auto* warehouse : warehouses)
	{
		if (warehouse->operational())
		{
			const auto& warehouseProducts = warehouse->products();
			capacityAvailable += warehouseProducts.availableStorage();
			capacityTotal += warehouseProducts.capacity();
		}
	}

	warehouseCount = warehouses.size();
	warehouseCapacityTotal = capacityTotal;
	warehouseCapacityUsed = capacityTotal - capacityAvailable;
}


void WarehouseReport::fillListFromStructureList(const std::vector<Warehouse*>& warehouses)
{
	lstStructures.clear();

	for (auto* warehouse : warehouses)
	{
		lstStructures.addItem(warehouse, structureStateDescription(*warehouse));
	}

	if (!lstStructures.isEmpty()) { lstStructures.selectedIndex(0); }
	computeTotalWarehouseCapacity();
}


void WarehouseReport::fillListFull()
{
	const auto predicate = [](Warehouse* wh) {
		return wh->products().atCapacity() && wh->isOperable();
	};

	fillListFromStructureList(selectWarehouses(mStructureManager, predicate));
}


void WarehouseReport::fillListVacancy()
{
	const auto predicate = [](Warehouse* wh) {
		return !wh->products().atCapacity() && !wh->products().empty() && wh->isOperable();
	};

	fillListFromStructureList(selectWarehouses(mStructureManager, predicate));
}



void WarehouseReport::fillListEmpty()
{
	const auto predicate = [](Warehouse* wh) {
		return wh->products().empty() && wh->isOperable();
	};

	fillListFromStructureList(selectWarehouses(mStructureManager, predicate));
}


void WarehouseReport::fillListDisabled()
{
	const auto predicate = [](Warehouse* structure) {
		return structure->disabled() || structure->destroyed();
	};

	fillListFromStructureList(selectWarehouses(mStructureManager, predicate));
}


void WarehouseReport::onDoubleClick(NAS2D::MouseButton button, NAS2D::Point<int> position)
{
	if (!visible()) { return; }
	if (button != NAS2D::MouseButton::Left) { return; }

	if (lstStructures.area().contains(position) && selectedWarehouse())
	{
		if (mTakeMeThereHandler) { mTakeMeThereHandler(selectedWarehouse()); }
	}
}


void WarehouseReport::onResize()
{
	Control::onResize();

	const auto centerX = area().center().x;
	const auto leftWidth = centerX - area().position.x - 10;
	const auto rightOriginX = centerX + 10;
	const auto rightWidth = area().endPoint().x - rightOriginX - 10;

	lstStructures.size({leftWidth, area().endPoint().y - lstStructures.position().y - 10});

	const auto productsTop = area().position.y + rightPanelHeaderHeight;
	const auto productsHeight = area().endPoint().y - productsTop - 10;

	lstProducts.position({rightOriginX, productsTop});
	lstProducts.size({rightWidth, std::max(productsHeight, 100)});

	btnTakeMeThere.position({area().endPoint().x - btnTakeMeThere.size().x - 10, area().position.y + 12});
}


void WarehouseReport::onVisibilityChange(bool visible)
{
	Report::onVisibilityChange(visible);

	setVisibility();
}


void WarehouseReport::setVisibility()
{
	const auto isWarehouseSelected = lstStructures.isItemSelected();
	btnTakeMeThere.visible(isWarehouseSelected);
	lstProducts.visible(isWarehouseSelected);
}


void WarehouseReport::filterButtonClicked()
{
	btnShowAll.toggle(false);
	btnVacancy.toggle(false);
	btnFull.toggle(false);
	btnEmpty.toggle(false);
	btnDisabled.toggle(false);
}


void WarehouseReport::onShowAll()
{
	filterButtonClicked();
	btnShowAll.toggle(true);

	fillLists();
}


void WarehouseReport::onFull()
{
	filterButtonClicked();
	btnFull.toggle(true);

	fillListFull();
}


void WarehouseReport::onVacancy()
{
	filterButtonClicked();
	btnVacancy.toggle(true);

	fillListVacancy();
}


void WarehouseReport::onEmpty()
{
	filterButtonClicked();
	btnEmpty.toggle(true);

	fillListEmpty();
}


void WarehouseReport::onDisabled()
{
	filterButtonClicked();
	btnDisabled.toggle(true);

	fillListDisabled();
}


void WarehouseReport::onTakeMeThere()
{
	const auto* warehouse = selectedWarehouse();
	if (warehouse)
	{
		if (mTakeMeThereHandler) { mTakeMeThereHandler(warehouse); }
	}
}


void WarehouseReport::onStructureSelectionChange()
{
	const auto* warehouse = selectedWarehouse();
	if (warehouse != nullptr)
	{
		lstProducts.productPool(warehouse->products());
	}
	setVisibility();
}

void WarehouseReport::drawLeftPanel(NAS2D::Renderer& renderer) const
{
	const auto panelTop = area().position.y + filterButtonSectionOffset.y;
	const auto leftPanel = NAS2D::Rectangle<int>{
		{area().position.x, panelTop},
		{area().size.x / 2, area().size.y - filterButtonSectionOffset.y}
	};
	renderer.drawBoxFilled(leftPanel, NAS2D::Color{25, 25, 25});

	const auto textLineSpacing = infoSectionHeight / 3;
	const auto textOrigin = area().position + infoSectionOffset;
	renderer.drawText(fontMediumBold, "Warehouse Count", textOrigin, constants::PrimaryTextColor);
	renderer.drawText(fontMediumBold, "Total Storage", textOrigin + NAS2D::Vector{0, textLineSpacing}, constants::PrimaryTextColor);
	renderer.drawText(fontMediumBold, "Capacity Used", textOrigin + NAS2D::Vector{0, textLineSpacing * 2}, constants::PrimaryTextColor);

	const auto valueOrigin = textOrigin + NAS2D::Vector{leftPanel.size.x / 2 - 20, fontMediumBold.height() - fontMedium.height()};
	const auto warehouseCountText = std::to_string(warehouseCount);
	const auto warehouseCapacityText = std::to_string(warehouseCapacityTotal);
	const auto countTextWidth = fontMedium.width(warehouseCountText);
	const auto capacityTextWidth = fontMedium.width(warehouseCapacityText);
	renderer.drawText(fontMedium, warehouseCountText, valueOrigin + NAS2D::Vector{-countTextWidth, 0}, constants::PrimaryTextColor);
	renderer.drawText(fontMedium, warehouseCapacityText, valueOrigin + NAS2D::Vector{-capacityTextWidth, textLineSpacing}, constants::PrimaryTextColor);

	const auto capacityUsedTextWidth = fontMediumBold.width("Capacity Used");
	const auto capacityBarPosition = textOrigin + NAS2D::Vector{capacityUsedTextWidth + 10, 3 + textLineSpacing * 2};
	const auto capacityBarSize = NAS2D::Vector{valueOrigin.x - capacityBarPosition.x, 20};
	drawProgressBar(
		warehouseCapacityUsed,
		warehouseCapacityTotal,
		{capacityBarPosition, capacityBarSize}
	);
}


void WarehouseReport::drawRightPanel(NAS2D::Renderer& renderer) const
{
	const auto* warehouse = selectedWarehouse();
	if (!warehouse) { return; }

	const auto position = NAS2D::Point{area().center().x + 10, area().position.y + 12};
	renderer.drawText(fontBigBold, warehouse->name(), position, constants::PrimaryTextColor);
}


void WarehouseReport::update()
{
	if (!visible()) { return; }

	ControlContainer::update();
	auto& renderer = NAS2D::Utility<NAS2D::Renderer>::get();
	draw(renderer);
}


void WarehouseReport::draw(NAS2D::Renderer& renderer) const
{
	if (!visible()) { return; }

	drawLeftPanel(renderer);
	const auto dividerX = area().center().x;
	const auto dividerTop = area().position.y + filterButtonSectionOffset.y;
	renderer.drawLine(
		NAS2D::Point{dividerX, dividerTop},
		NAS2D::Point{dividerX, area().endPoint().y - 10},
		constants::PrimaryColor);
	drawRightPanel(renderer);
}
