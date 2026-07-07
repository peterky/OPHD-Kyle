#pragma once

#include <libControls/Control.h>

#include <NAS2D/Resource/Image.h>


struct MapCoordinate;
class Tile;
class TileMap;
class MapView;


class DetailMap : public Control
{
public:
	DetailMap(MapView& mapView, TileMap& tileMap, const std::string& tilesetPath);
	~DetailMap() override;

	bool isMouseOverTile() const;

	MapCoordinate mouseTilePosition() const;
	Tile& mouseTile();

	void update() override;
	void draw(NAS2D::Renderer& renderer) const override;

	void zoomIn();
	void zoomOut();

protected:
	void onResize() override;
	void onMouseMove(NAS2D::Point<int> position, NAS2D::Vector<int> relative);

	void applyViewSizeFromWindow();

	void drawGrid(NAS2D::Renderer& renderer) const;

private:
	float tileScale() const;
	NAS2D::Vector<float> scaledTileSize() const;
	NAS2D::Vector<float> scaledTileDrawSize() const;
	NAS2D::Vector<float> scaledTileDrawOffset() const;

private:
	MapView& mMapView;
	TileMap& mTileMap;
	const NAS2D::Image mTileset;
	const NAS2D::Image mMineBeacon;

	NAS2D::Point<int> mOriginPixelPosition; // Top pixel at top of diamond
	NAS2D::Point<int> mMouseTilePosition;
	int mBaseViewSize{0};
	int mZoomInOffset{0};
	int mZoomOutOffset{0};
};