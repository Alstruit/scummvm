/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MOHAWK_ZOOMBINI_GRAPHICS_H
#define MOHAWK_ZOOMBINI_GRAPHICS_H

#include "graphics/font.h"
#include "graphics/fontman.h"
#include "graphics/surface.h"
#include "common/stack.h"

#include "mohawk/zoombini_resource.h"
#include "mohawk/graphics.h"

namespace Mohawk {

class MohawkEngine_Zoombini;
class MohawkSurface;
class ZoombiniText;
class ZmbHotspot;
class ZmbDrawRecord;

/**
 * Implements graphics of Zoombinis
 * 
 * There are two notation to index shape (aka sprite, subimage).
 * (1) subImageId: 0-based index, native to ScummVM mohawk engine bitmap implementation.
 * (2) shapeIdx: 1-based index, native to Mohawk Feature Hotspot.
 * Call proper functions following the index notation.
 */
class ZoombiniGraphics : public GraphicsManager {
public:
	explicit ZoombiniGraphics(MohawkEngine_Zoombini *vm);
	~ZoombiniGraphics() override;

	// [*] Screen related
	enum ScreenKind {
		kBackScreen,
		kShapeScreen,
	};
	Graphics::Surface *getScreen(ScreenKind screenKind);
	Graphics::Surface *getBackScreen() { return _backScreen; }
	Graphics::Surface *getShapeScreen() { return _shapeScreen; }

	// [*] Pixel format
	Graphics::PixelFormat getPixelFormat() { return _pixelFormat; }

	// [*] Screen captures
	void createScreen(Graphics::Surface &screen);
	void captureScreen(ScreenKind srcScreenKind, Graphics::Surface *destScreen);
	void captureComposedScreen(ScreenKind destScreenKind);
	void captureComposedScreen(Graphics::Surface *destScreen);

	// [*] Screen updates
	bool isDirty() { return _isScreenDirty; }
	void setDirty() { _isScreenDirty = true; }
	void flushScreens();
	void clearScreens();
	/**
	 * Switch the OSystem graphics mode between true-color and CLUT8.
	 * Clears all surface caches and recreates the internal screen buffers.
	 * Call with trueColor=true before playing Bink video; restore with
	 * trueColor=false afterward so palette-based rendering works normally.
	 */
	void reinitGraphics(bool trueColor);
	void clearScreen(ScreenKind screenKind);
	/**
	 * Copy background port → composite buffer (blitter port).
	 * IDA gfx_renderFrame 0x45F352: gfx_blitPortToPort(pScreenPort → pPortToBlitter)
	 * Called at the start of each render frame, before shapes are drawn.
	 */
	void copyBackToShapeScreen();
	/**
	 * Copy background port → composite buffer, clipped to the given rect.
	 * Only pixels within clipRect are overwritten on the shapeScreen.
	 * Used by the dirty-rect render pipeline to restore background in changed areas only.
	 */
	void copyBackToShapeScreen(const Common::Rect &clipRect);

	// [*] Render clip region
	/**
	 * IDA: port_selectActiveRegion (0x48F40C) — confine all drawing to
	 * the accumulated dirty region (list of rectangles).  The original engine
	 * uses Windows GDI clip regions (union of rectangles) set on the port's HDC.
	 * We replicate this by maintaining a list of individual dirty rects and
	 * clipping each draw operation to each rect's intersection.
	 */
	void setRenderClipRects(const Common::Array<Common::Rect> &rects);
	void addRenderClipRect(const Common::Rect &rect);
	void clearRenderClipRect();

	// [*] Resource Management Extensions
	MohawkSurface *findImage(ZmbResource imgResource);
	/**
	 * Find shape bitmap using 1-based shapIdx.
	 */
	MohawkSurface *findShape(ZmbResource imgResource, uint16 shapeIdx);
	/**
	 * Find shape bitmap using 0-based subImage id.
	 */
	MohawkSurface *findSubImage(ZmbResource imgResource, uint16 subImage);
	/**
	 * Get width and height of shape bitmap using 1-based shapeIdx.
	 */
	Common::Rect getShapeSize(ZmbResource imgResource, uint16 shapeIdx);
	/**
	 * Get width and height of shape bitmap using 0-based subImage id.
	 */
	Common::Rect getSubImageSize(ZmbResource imgResource, uint16 subImage);
	uint32 getShapeCount(ZmbResource imgResource);
	void clearCommonCache();

	// [*] CURS
	enum MouseCursorResourceId : uint16 {
		kResCursor00_Default = 0,
		kResCursor01_Watch,
		kResCursor02_EyeMiddle,
		kResCursor03_EyeRight,
		kResCursor04_EyeLeft,
		kResCursor05_EyeBlink,
	};
	uint32 _lastMouseCursorEyeAnimationFrameTime = 0;

	const ZoombiniGraphics::MouseCursorResourceId _mouseCursorEyeAnimationFrames[11] = {
		ZoombiniGraphics::kResCursor04_EyeLeft,
		ZoombiniGraphics::kResCursor02_EyeMiddle,
		ZoombiniGraphics::kResCursor03_EyeRight,
		ZoombiniGraphics::kResCursor02_EyeMiddle,
		ZoombiniGraphics::kResCursor05_EyeBlink,
		ZoombiniGraphics::kResCursor02_EyeMiddle,
		ZoombiniGraphics::kResCursor05_EyeBlink,
		ZoombiniGraphics::kResCursor02_EyeMiddle,
		ZoombiniGraphics::kResCursor04_EyeLeft,
		ZoombiniGraphics::kResCursor03_EyeRight,
		ZoombiniGraphics::kResCursor05_EyeBlink,
	};
	uint32 _mouseCursorEyeAnimationFrameIdx = 0;

	void setMouseCursor(MouseCursorResourceId cursorId);
	MouseCursorResourceId getMouseCursor() const { return _activeCursorId; }
	void startMouseCursorEyeAnimation(uint32 currentTimeMs);
	void stopMouseCursorEyeAnimation();
	void runMouseCursorEyeAnimationFrame(uint32 currentTimeMs);
	bool isMouseCursorEyeAnimationActive() const;

	// [*] tBMP
	void drawBackground(uint16 image);
	void drawBackground(ScreenKind screenKind, uint16 image);
	void drawBackground(Graphics::Surface *screen, uint16 image);
	void drawImage(ScreenKind screenKind, uint16 image, const Common::Point &destPos);
	/**
	 * Draw shape bitmap to screen using 1-based shapeIdx.
	 */
	Common::Rect drawShape(ScreenKind screenKind, ZmbResource res, uint16 shapeIdx, const Common::Point &destPos, bool clearBeforeRender = false);
	/**
	 * Draw shape bitmap to screen using 1-based shapeIdx.
	 */
	Common::Rect drawShape(ScreenKind screenKind, ZmbResource res, uint16 shapeIdx, const Common::Rect &destRect, bool clearBeforeRender = false);
	/**
	 * Draw shape bitmap to screen using a feature hotspot.
	 */
	Common::Rect drawShape(ScreenKind screenKind, ZmbResource res, const ZmbHotspot *hotspot, bool clearBeforeRender = false);
	/**
	 * Draw shape bitmap to screen using 1-based shapeIdx.
	 */
	Common::Rect drawShape(Graphics::Surface *screen, ZmbResource res, uint16 shapeIdx, const Common::Point &destPos, bool clearBeforeRender = false);
	/**
	 * Draw shape bitmap to screen using 1-based shapeIdx.
	 */
	Common::Rect drawShape(Graphics::Surface *screen, ZmbResource res, uint16 shapeIdx, const Common::Rect &destRect, bool clearBeforeRender = false);
	/**
	 * Draw shape bitmap to screen using a feature hotspot.
	 */
	Common::Rect drawShape(Graphics::Surface *screen, ZmbResource res, const ZmbHotspot *hotspot, bool clearBeforeRender = false);
	/**
	 * Draw shape bitmap to screen using 0-based subImage id.
	 */
	Common::Rect drawSubImage(ScreenKind screenKind, ZmbResource res, uint16 subImage, const Common::Point &destPos, bool clearBeforeRender = false);
	/**
	 * Draw shape bitmap to screen using 0-based subImage id.
	 */
	Common::Rect drawSubImage(ScreenKind screenKind, ZmbResource res, uint16 subImage, const Common::Rect &destRect, bool clearBeforeRender = false);
	/**
	 * Draw shape bitmap to screen using 0-based subImage id.
	 */
	Common::Rect drawSubImage(Graphics::Surface *screen, ZmbResource res, uint16 subImage, const Common::Point &destPos, bool clearBeforeRender = false);
	/**
	 * Draw shape bitmap to screen using 0-based subImage id.
	 */
	Common::Rect drawSubImage(Graphics::Surface *screen, ZmbResource res, uint16 subImage, const Common::Rect &destRect, bool clearBeforeRender = false);
	Common::Rect drawImageSectionToScreen(Graphics::Surface *screen, MohawkSurface *imgSurface, const Common::Rect &srcRect, const Common::Rect &dstRect, bool clearBeforeRender = false);

	// [*] DrawLine
	void drawLine(ScreenKind screenKind, const Common::Point &start, const Common::Point &end, uint32 color);
	void drawThickLine(ScreenKind screenKind, const Common::Point &start, const Common::Point &end, int penX, int penY, uint32 color);

	// [*] Clear / Fill Area
	void clearArea(ScreenKind screenKind, ZmbDrawRecord *record);
	void clearArea(ScreenKind screenKind, ZmbResource res, const ZmbHotspot *hotspot);
	void clearArea(ScreenKind screenKind, const Common::Rect &rect);
	void clearArea(Graphics::Surface *screen, ZmbDrawRecord *record);
	void clearArea(Graphics::Surface *screen, ZmbResource res, const ZmbHotspot *hotspot);
	void clearArea(Graphics::Surface *screen, const Common::Rect &rect);
	void fillArea(ScreenKind screenKind, ZmbDrawRecord *record, uint32 color = kTransparentKey);
	void fillArea(ScreenKind screenKind, ZmbResource res, const ZmbHotspot *hotspot, uint32 color = kTransparentKey);
	void fillArea(ScreenKind screenKind, const Common::Rect &rect, uint32 color = kTransparentKey);
	void fillArea(ScreenKind screenKind, uint32 color = kTransparentKey);
	void fillArea(Graphics::Surface *screen, ZmbDrawRecord *record, uint32 color = kTransparentKey);
	void fillArea(Graphics::Surface *screen, ZmbResource res, const ZmbHotspot *hotspot, uint32 color = kTransparentKey);
	void fillArea(Graphics::Surface *screen, const Common::Rect &rect, uint32 color = kTransparentKey);
	void fillArea(Graphics::Surface *screen, uint32 color = kTransparentKey);

	// [*] 256color Palette
	/**
	 * Apply palette from a SHPL resource.
	 */
	void setPalette(uint16 id) override;
	/**
	 * Read palette from a SHPL resource.
	 * @param destBuf The buffer to read the palette data into. 768 bytes or larger buffer is recommended.
	 * @param destBufSize Size of the destBuf.
	 * @return True if successfully read the palette data. False if the buffer size is not enough, or failed to read a resource.
	 */
	bool readPalette(uint16 id, byte *destBuf, size_t destBufSize);
	void clearPalette();

	enum PredefinedColor : uint32 {
		kTransparentKey = 0x00,
		kBlackKey = 0xFF,
		/**
		 * #FEFEFE
		 */
		kColor0A_White = 0x0A,
		/**
		 * #202020
		 */
		kColor0B_VeryDarkGray = 0x0B,
		/**
		 * #414141
		 */
		kColor0C_DarkGray = 0x0C,
		/**
		 * #828282
		 */
		kColor0D_LightGray = 0x0D,
		/**
		 * #C0C0C0
		 */
		kColor0E_VeryLightGray = 0x0E,
		/**
		 * #112135
		 */
		kColor0F_VeryDarkCyan = 0x0F,
		/**
		 * #2E4F7F
		 */
		kColor10_DarkCyan = 0x10,
		/**
		 * #4677AF
		 */
		kColor11_Cyan = 0x11,
		/**
		 * #5E9EFF
		 */
		kColor12_SkyBlue = 0x12,
		/**
		 * #7DAEFF
		 */
		kColor13_LightBlue = 0x13,
		/**
		 * #9EBFFF
		 */
		kColor14_PastelBlue = 0x14,
		/**
		 * #16002E
		 */
		kColor15_VeryDarkPurple = 0x15,
		/**
		 * #280A5C
		 */
		kColor16_DarkPurple = 0x16,
		/**
		 * #3A148A
		 */
		kColor17_Purple = 0x17,
		/**
		 * #5E1268
		 */
		kColor18_DarkMagenta = 0x18,
		/**
		 * #B415C2
		 */
		kColor19_Magenta = 0x19,
		/**
		 * #D443E1
		 */
		kColor1A_DarkPink = 0x1A,
		/**
		 * #F470FF
		 */
		kColor1B_Pink = 0x1B,
		/**
		 * #1316A9
		 */
		kColor1C_DarkAzure = 0x1C,
		/**
		 * #3D3DFF
		 */
		kColor1D_Azure = 0x1D,
		/**
		 * #9496FF
		 */
		kColor1E_LightAzure = 0x1E,
		/**
		 * #0C2701
		 */
		kColor1F_VeryDarkGreen = 0x1F,
		/**
		 * #005500 
		 */
		kColor20_DarkGreen = 0x20,
		/**
		 * #009900
		 */
		kColor21_Green = 0x21,
		/**
		 * #4ED24B 
		 */
		kColor22_LimeGreen = 0x22,
		/**
		 * #A34400
		 */
		kColor23_DarkOrange = 0x23,
		/**
		 * #FF7B00
		 */
		kColor24_Orange = 0x24,
		/**
		 * #FFB87A
		 */
		kColor25_LightOrange = 0x25,
		/**
		 * #961221
		 */
		kColor26_DarkRed = 0x26,
		/**
		 * #FC2C44
		 */
		kColor27_Red = 0x27,
		/**
		 * #FE8B9A
		 */
		kColor28_LightRed = 0x28,
		/**
		 * #7A3505
		 */
		kColor29_Brown = 0x29,
		/**
		 * #B58F21
		 */
		kColor2A_LightBrown = 0x2A,
		/**
		 * #EDF50A
		 */
		kColor2B_Yellow = 0x2B,
		/**
		 * #7F7F7F
		 */
		kColor2C_Gray = 0x2C,
		/**
		 * #000000
		 */		
		kColor2D_Black = 0x2D,
	};

	enum XferRoutePathLevelColor : uint32 {
		// Route path animation palette colors (XFER/RODMAP flood-fill overlay).
		// These form a gradient used by the route path flood-fill animation.
		// Original pixel values 1 and 2 in the shape are replaced with
		// these palette indices during the animation.
		// RGB values extracted from XFER SHPL_1000-4000 palettes.
		/**
		 * #D6A55A ~ #E7AD5A (warm tan, varies per view)
		 */
		kRoutePathColor2E_LevelOneBack1 = 0x2E,
		/**
		 * #D6AD9C ~ #FAB375 (peachy/salmon, varies per view)
		 */
		kRoutePathColor2F_LevelOneBack2 = 0x2F,
		/**
		 * #005F41 (dark teal green)
		 */
		kRoutePathColor30_LevelOneColor1 = 0x30,
		/**
		 * #579984 (medium teal)
		 */
		kRoutePathColor31_LevelOneColor2 = 0x31,
		/**
		 * #F4A200 (bright orange)
		 */
		kRoutePathColor32_LevelTwoColor1 = 0x32,
		/**
		 * #FFC863 (golden yellow)
		 */
		kRoutePathColor33_LevelTwoColor2 = 0x33,
		/**
		 * #FF5711 (bright red-orange)
		 */
		kRoutePathColor34_LevelThreeColor1 = 0x34,
		/**
		 * #FF9569 (coral/salmon)
		 */
		kRoutePathColor35_LevelThreeColor2 = 0x35,
		/**
		 * #BF0218 (dark red)
		 */
		kRoutePathColor36_LevelFourColor1 = 0x36,
		/**
		 * #E25161 (salmon red)
		 */
		kRoutePathColor37_LevelFourColor2 = 0x37,
	};

	// [*] Draw TrueType Text
	struct TextConf {
		ZoombiniFontUsage _fontUsage = ZoombiniFontUsage::kFontText;
		uint32 _textPalette = kColor2D_Black;
		bool _outlineEffect = false;
		uint32 _outlinePalette = kTransparentKey;
		/**
		 * For debug purposes
		 */
		bool _fillBackground = false;
		/**
		 * For debug purposes
		 */
		uint32 _fillBackgroundPalette = kTransparentKey;
		bool _wordWrap = false;
		Graphics::TextAlign _hAlign = Graphics::kTextAlignLeft;
		Graphics::TextAlign _vAlign = Graphics::kTextAlignStart;
	};
	void drawText(ScreenKind screenKind, uint32 textKey, const Common::Rect &destRect);
	void drawText(ScreenKind screenKind, uint32 textKey, const Common::Rect &destRect, const TextConf& tc);
	void drawText(ScreenKind screenKind, const Common::U32String &text, const Common::Rect &destRect);
	void drawText(ScreenKind screenKind, const Common::U32String &text, const Common::Rect &destRect, const TextConf& tc);
	Common::Point getTextBounds(const Common::U32String &text, int16 targetWidth, const TextConf& tc);
	int16 getFontHeight(const TextConf& tc);

	// [*] Transitions and effects
	enum FadeType {
		kFadeIn,
		kFadeOut
	};

	struct FadeEffect {
		FadeType _type;
		bool _isFading = false;
		uint32 _startTime; // In milliseconds
		uint32 _duration; // In milliseconds

		FadeEffect(FadeType type, uint32 duration)
			: _type(type), _duration(duration) { }
	};

	void queueFadeEffect(FadeType type, uint32 duration);
	/**
	 * Apply the fade effect if one was queued.
	 * @param currentTime Current time in milliseconds.
	 * @return True if a fade effect is in progress, false otherwise.
	 */
	bool applyFadeEffect(uint32 currentTime);
	/**
	 * Check if a fade effect is currently in progress.
	 * @return True if a fade effect is in progress, false otherwise.
	 */
	bool isFading() const;
	void dimPalette(uint16 idx, uint16 steps);

	/**
	 * Scale a range of palette entries by a percentage and apply to the screen.
	 * Does NOT modify the stored _paletteBytes reference. Entries outside [startEntry,
	 * startEntry+count) keep their original values.
	 * IDA: picker_applyBrightnessDim_42185D — scales entries 10..245 by 88/90/92%.
	 *
	 * @param startEntry First palette entry index to scale (0-based).
	 * @param count      Number of entries to scale.
	 * @param scalePercent Scale factor in percent (e.g. 88 = 88%).
	 */
	void scalePalettePartial(uint16 startEntry, uint16 count, uint8 scalePercent);

	static constexpr uint16 kScreenWidth = 640;
	static constexpr uint16 kScreenHeight = 480;

protected:
	MohawkSurface *decodeImage(uint16 id) override;
	MohawkSurface *decodeImage(ZmbResource imgResource);
	Common::Array<MohawkSurface *> decodeImages(uint16 id) override;
	Common::Array<MohawkSurface *> decodeImages(ZmbResource imgResource);
	MohawkEngine *getVM() override { return reinterpret_cast<MohawkEngine *>(_vm); }
	
private:
	Common::Array<Common::U32String> prepareTextLines(const Common::U32String &text, const Graphics::Font *font, bool wordWrap, int16 targetWidth);
	/**
	 * Calculate total draw height and max draw width
	 */
	Common::Point getTextLinesBounds(const Graphics::Font *font, bool outlineEffect, const Common::Array<Common::U32String> &lines);
	void drawTextLines(ScreenKind screenKind, const Graphics::Font *font, const Common::Array<Common::U32String> &lines, const Common::Rect &destRect, uint32 palette, Graphics::TextAlign hAlign, uint32 fillBackgroundColor = kTransparentKey);

	MohawkEngine_Zoombini *_vm;
	MohawkBitmap *_bmpDecoder;

	byte _paletteBytes[3 * 256];

	Graphics::PixelFormat _pixelFormat;
	Common::Rect _screenRect;

	Graphics::Surface *_backScreen = nullptr;
	Graphics::Surface *_shapeScreen = nullptr;
	bool _isScreenDirty = false;

	// IDA: port_selectActiveRegion — render clip region (list of rects).
	// The original engine uses GDI clip regions (union of rectangles) on the
	// port's HDC.  We store the individual rects and their bounding box.
	Common::Array<Common::Rect> _renderClipRects;
	Common::Rect _renderClipBounds;  // bounding box for quick early-out
	bool _hasRenderClipRect = false;

	// An image cache that stores ZOOMBINI.MHK images
	Common::HashMap<uint16, MohawkSurface *> _sysImageCache;
	Common::HashMap<uint16, Common::Array<MohawkSurface *> > _sysSubImageCache;

	// Fade effects
	Common::Queue<FadeEffect> _fadeQueue;

	// Active cursor
	MouseCursorResourceId _activeCursorId = kResCursor00_Default;
};

} // End of namespace Mohawk

#endif
