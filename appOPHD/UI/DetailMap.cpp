#include "DetailMap.h"

#include "../Map/Tile.h"
#include "../Map/TileMap.h"
#include "../Map/MapView.h"
#include "../MapObjects/MapObject.h"

#include <NAS2D/Utility.h>
#include <NAS2D/EventHandler.h>
#include <NAS2D/Renderer/Color.h>
#include <NAS2D/Renderer/Renderer.h>
#include <NAS2D/Math/PointInRectangleRange.h>

#include <algorithm>
#include <map>


namespace {
	const auto TileSize = NAS2D::Vector{128, 55};
	const auto TileDrawSize = NAS2D::Vector{128, 64};
	const auto TileDrawOffset = NAS2D::Vector{TileDrawSize.x / 2, TileDrawSize.y - TileSize.y};
	constexpr int MinViewSize{7};
	constexpr float MinTileScale{0.5f};

	const std::map<Tile::Overlay, NAS2D::Color> OverlayColors =
	{
		{Tile::Overlay::None, NAS2D::Color::Normal},
		{Tile::Overlay::Communications, {125, 200, 255}},
		{Tile::Overlay::Connectedness, NAS2D::Color::Green},
		{Tile::Overlay::TruckingRoutes, NAS2D::Color::Orange},
		{Tile::Overlay::Police, NAS2D::Color::Red}
	};

	const std::map<Tile::Overlay, NAS2D::Color> OverlayHighlightColors =
	{
		{Tile::Overlay::None, NAS2D::Color{125, 200, 255}},
		{Tile::Overlay::Communications, {100, 180, 230}},
		{Tile::Overlay::Connectedness, NAS2D::Color{71, 224, 146}},
		{Tile::Overlay::TruckingRoutes, NAS2D::Color{125, 200, 255}},
		{Tile::Overlay::Police, NAS2D::Color{100, 180, 230}}
	};


	const NAS2D::Color& overlayColor(Tile::Overlay overlay, bool isHighlighted)
	{
		return (isHighlighted ? OverlayHighlightColors : OverlayColors).at(overlay);
	}


	uint8_t glow()
	{
		static NAS2D::Timer throbTimer;
		constexpr int throbCycleTime = 2000;
		constexpr int glowAmplitude = 120;
		constexpr int glowOffset = 120;

		int sawtooth = static_cast<int>(throbTimer.tick()) % throbCycleTime;
		int triangle = sawtooth < throbCycleTime / 2 ? sawtooth : (throbCycleTime - sawtooth);
		return static_cast<uint8_t>(triangle * glowAmplitude / (throbCycleTime / 2) + glowOffset);
	}


	NAS2D::Color glowColor()
	{
		uint8_t glowValue = glow();
		return NAS2D::Color{glowValue, glowValue, glowValue};
	}


	void drawScaledSubImage(NAS2D::Renderer& renderer, const NAS2D::Image& image, NAS2D::Point<float> position, const NAS2D::Rectangle<float>& subImageRect, float scale, NAS2D::Color color)
	{
		if (scale == 1.0f)
		{
			renderer.drawSubImage(image, position, subImageRect, color);
			return;
		}

		const NAS2D::Rectangle<float> destRect{position, subImageRect.size * scale};
		renderer.drawSubImageStretched(image, destRect, subImageRect, color);
	}
}


float DetailMap::tileScale() const
{
	const auto viewSize = mMapView.viewSize();
	if (viewSize <= mBaseViewSize) { return 1.0f; }
	return static_cast<float>(mBaseViewSize) / static_cast<float>(viewSize);
}


NAS2D::Vector<float> DetailMap::scaledTileSize() const
{
	return TileSize.to<float>() * tileScale();
}


NAS2D::Vector<float> DetailMap::scaledTileDrawSize() const
{
	return TileDrawSize.to<float>() * tileScale();
}


NAS2D::Vector<float> DetailMap::scaledTileDrawOffset() const
{
	return TileDrawOffset.to<float>() * tileScale();
}


DetailMap::DetailMap(MapView& mapView, TileMap& tileMap, const std::string& tilesetPath) :
	mMapView{mapView},
	mTileMap{tileMap},
	mTileset{tilesetPath},
	mMineBeacon{"structures/mine_beacon.png"}
{
	size(NAS2D::Utility<NAS2D::Renderer>::get().size());

	auto& eventHandler = NAS2D::Utility<NAS2D::EventHandler>::get();
	eventHandler.mouseMotion().connect({this, &DetailMap::onMouseMove});
}


DetailMap::~DetailMap()
{
	auto& eventHandler = NAS2D::Utility<NAS2D::EventHandler>::get();
	eventHandler.mouseMotion().disconnect({this, &DetailMap::onMouseMove});
}


void DetailMap::applyViewSizeFromWindow()
{
	const auto size = this->size() - NAS2D::Vector{0, TileDrawOffset.y};

	const auto sizeInTiles = size.skewInverseBy(TileSize);
	mBaseViewSize = std::min(sizeInTiles.x, sizeInTiles.y);

	const auto maxZoomInOffset = std::max(0, mBaseViewSize - MinViewSize);
	const auto maxZoomOutOffset = std::max(0, static_cast<int>(static_cast<float>(mBaseViewSize) * (1.0f / MinTileScale - 1.0f)));

	mZoomInOffset = std::clamp(mZoomInOffset, 0, maxZoomInOffset);
	mZoomOutOffset = std::clamp(mZoomOutOffset, 0, maxZoomOutOffset);
	mMapView.viewSize(mBaseViewSize + mZoomOutOffset - mZoomInOffset);

	const auto scaledTileSize = this->scaledTileSize();
	mOriginPixelPosition = mRect.position + NAS2D::Vector{
		size.x / 2,
		static_cast<int>(scaledTileDrawOffset().y + (size.y - mMapView.viewSize() * scaledTileSize.y) / 2.0f)
	};
}


void DetailMap::onResize()
{
	applyViewSizeFromWindow();
}


void DetailMap::zoomIn()
{
	if (mZoomOutOffset > 0)
	{
		--mZoomOutOffset;
		applyViewSizeFromWindow();
		return;
	}

	const auto maxZoomInOffset = std::max(0, mBaseViewSize - MinViewSize);
	if (mZoomInOffset < maxZoomInOffset)
	{
		++mZoomInOffset;
		applyViewSizeFromWindow();
	}
}


void DetailMap::zoomOut()
{
	if (mZoomInOffset > 0)
	{
		--mZoomInOffset;
		applyViewSizeFromWindow();
		return;
	}

	const auto maxZoomOutOffset = std::max(0, static_cast<int>(static_cast<float>(mBaseViewSize) * (1.0f / MinTileScale - 1.0f)));
	if (mZoomOutOffset < maxZoomOutOffset)
	{
		++mZoomOutOffset;
		applyViewSizeFromWindow();
	}
}


/**
 * Returns true if the current tile highlight is actually within the visible diamond map.
 */
bool DetailMap::isMouseOverTile() const
{
	return mMapView.isVisibleTile(mouseTilePosition());
}


MapCoordinate DetailMap::mouseTilePosition() const
{
	return {mMouseTilePosition, mMapView.currentDepth()};
}


Tile& DetailMap::mouseTile()
{
	if (!isMouseOverTile())
	{
		throw std::runtime_error("Mouse not over a tile");
	}
	return mTileMap.getTile(mouseTilePosition());
}


void DetailMap::update()
{
	for (const auto tilePosition : NAS2D::PointInRectangleRange{mMapView.viewTileRect()})
	{
		auto& tile = mTileMap.getTile({tilePosition, mMapView.currentDepth()});

		if (tile.mapObject())
		{
			tile.mapObject()->updateAnimation();
		}
	}
}


void DetailMap::draw(NAS2D::Renderer& renderer) const
{
	const auto scale = tileScale();
	const auto scaledTileSize = this->scaledTileSize();
	const auto scaledTileDrawSize = this->scaledTileDrawSize();
	const auto scaledTileDrawOffset = this->scaledTileDrawOffset();
	int tsetOffset = mMapView.currentDepth() > 0 ? TileDrawSize.y : 0;

	for (const auto tilePosition : NAS2D::PointInRectangleRange{mMapView.viewTileRect()})
	{
		auto& tile = mTileMap.getTile({tilePosition, mMapView.currentDepth()});

		if (tile.excavated())
		{
			const auto offset = tilePosition - mMapView.viewTileRect().position;
			const auto position = mOriginPixelPosition.to<float>() - scaledTileDrawOffset + NAS2D::Vector<float>{
				(offset.x - offset.y) * scaledTileSize.x / 2.0f,
				(offset.x + offset.y) * scaledTileSize.y / 2.0f
			};
			const auto subImageRect = NAS2D::Rectangle<float>{{static_cast<float>(tile.index()) * TileDrawSize.x, static_cast<float>(tsetOffset)}, TileDrawSize.to<float>()};
			const bool isTileHighlighted = tilePosition == mMouseTilePosition;

			drawScaledSubImage(renderer, mTileset, position, subImageRect, scale, overlayColor(tile.overlay(), isTileHighlighted));

			// Draw a beacon on an unoccupied tile with a mine
			if (mTileMap.hasOreDeposit(tile.xyz()) && !tile.mapObject())
			{
				constexpr NAS2D::Vector<float> beaconOffsetInTile{0, -64};
				constexpr NAS2D::Vector<float> beaconLightOffsetInTile{59, 15};
				constexpr NAS2D::Rectangle<float> beaconLightSubImageRect{{59, 79}, {10, 7}};
				const auto beaconOffset = beaconOffsetInTile * scale;
				const auto beaconLightOffset = beaconLightOffsetInTile * scale;
				drawScaledSubImage(renderer, mMineBeacon, position + beaconOffset, NAS2D::Rectangle<float>{{0, 0}, TileDrawSize.to<float>()}, scale, NAS2D::Color::Normal);
				drawScaledSubImage(renderer, mMineBeacon, position + beaconLightOffset, beaconLightSubImageRect, scale, glowColor());
			}

			if (tile.mapObject())
			{
				tile.mapObject()->draw(position.to<int>(), scale);
			}
		}
	}
}


void DetailMap::drawGrid(NAS2D::Renderer& renderer) const
{
	const auto viewSize = mMapView.viewSize();
	const auto scaledTileSize = this->scaledTileSize();
	const auto incrementY = NAS2D::Vector<float>{-scaledTileSize.x, scaledTileSize.y};
	const auto leftEdge = mOriginPixelPosition.to<float>() + incrementY * viewSize / 2;
	const auto rightEdge = mOriginPixelPosition.to<float>() + scaledTileSize * viewSize / 2;
	for (int index = 0; index <= viewSize; ++index)
	{
		const auto offsetX = scaledTileSize * index / 2;
		const auto offsetY = incrementY * index / 2;
		renderer.drawLine(leftEdge + offsetX, mOriginPixelPosition.to<float>() + offsetX);
		renderer.drawLine(mOriginPixelPosition.to<float>() + offsetY, rightEdge + offsetY);
	}
}


void DetailMap::onMouseMove(NAS2D::Point<int> position, NAS2D::Vector<int> /*relative*/)
{
	const auto scaledTileSize = this->scaledTileSize();
	const auto pixelOffset = position.to<float>() - mOriginPixelPosition.to<float>();
	const auto tileOffset = NAS2D::Vector{
		static_cast<int>((pixelOffset.x * scaledTileSize.y + pixelOffset.y * scaledTileSize.x) / (scaledTileSize.x * scaledTileSize.y)),
		static_cast<int>((pixelOffset.y * scaledTileSize.x - pixelOffset.x * scaledTileSize.y) / (scaledTileSize.x * scaledTileSize.y))
	};
	mMouseTilePosition = mMapView.viewTileRect().position + tileOffset;
}