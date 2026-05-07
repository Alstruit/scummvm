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
#include "mohawk/zoombini_resource.h"

#include "common/config-manager.h"
#include "common/substream.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "engines/util.h"
#include "graphics/fontman.h"
#include "graphics/fonts/ttf.h"
#include "graphics/paletteman.h"

#include "mohawk/cursors.h"
#include "mohawk/resource.h"
#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_page.h"
#include "mohawk/zoombini_scripts.h"
#include "mohawk/zoombini_text.h"
#include "zoombini_graphics.h"

namespace Mohawk {

ZoombiniGraphics::ZoombiniGraphics(MohawkEngine_Zoombini *vm) : GraphicsManager(), _vm(vm),
																_bmpDecoder(new MohawkBitmap()),
																_screenRect(Common::Rect(kScreenWidth, kScreenHeight)) {
	initGraphics(_screenRect.width(), _screenRect.height());
	clearPalette();

	_pixelFormat = Graphics::PixelFormat::createFormatCLUT8();
	memset(_paletteBytes, 0, sizeof(_paletteBytes));

	// Initialize Surface Screens
	_backScreen = new Graphics::Surface();
	_backScreen->create(kScreenWidth, kScreenHeight, _pixelFormat);
	_shapeScreen = new Graphics::Surface();
	_shapeScreen->create(kScreenWidth, kScreenHeight, _pixelFormat);
	clearScreens();
}

ZoombiniGraphics::~ZoombiniGraphics() {
	clearCommonCache();

	delete _bmpDecoder;

	_shapeScreen->free();
	delete _shapeScreen;
	_backScreen->free();
	delete _backScreen;
}

Graphics::Surface *ZoombiniGraphics::getScreen(ScreenKind screenKind) {
	switch (screenKind) {
	case kBackScreen:
		return _backScreen;
	case kShapeScreen:
		return _shapeScreen;
	default:
		error("Invalid ScreenKind %d", screenKind);
		return nullptr;
	}
}

void ZoombiniGraphics::createScreen(Graphics::Surface &screen) {
	screen.create(kScreenWidth, kScreenHeight, _pixelFormat);
}

void ZoombiniGraphics::captureScreen(ScreenKind srcScreenKind, Graphics::Surface *destScreen) {
	assert(destScreen != nullptr);

	Graphics::Surface *srcScreen = _vm->_gfx->getScreen(srcScreenKind);
	destScreen->copyFrom(*srcScreen);
}

void ZoombiniGraphics::copyToScreen(ScreenKind destScreenKind, const Graphics::Surface &srcScreen) {
	Graphics::Surface *destScreen = _vm->_gfx->getScreen(destScreenKind);
	destScreen->copyFrom(srcScreen);
	recordDirtyRect(destScreenKind, destScreen->getRect());
}

void ZoombiniGraphics::captureComposedScreen(ScreenKind destScreenKind) {
	Graphics::Surface *destScreen = _vm->_gfx->getScreen(destScreenKind);

	Graphics::Surface *systemScreen = _vm->_system->lockScreen();
	destScreen->copyFrom(*systemScreen);
	_vm->_system->unlockScreen();
	recordDirtyRect(destScreenKind, destScreen->getRect());
}

void ZoombiniGraphics::captureComposedScreen(Graphics::Surface *destScreen) {
	assert(destScreen != nullptr);

	Graphics::Surface *systemScreen = _vm->_system->lockScreen();
	destScreen->copyFrom(*systemScreen);
	_vm->_system->unlockScreen();
}

// [*] Screen updates
void ZoombiniGraphics::flushScreens() {
	// IDA loadPort_410C39: copies blitter port (composite) → device context.
	// _shapeScreen is the composite buffer (background + shapes already drawn on it).
	if (_isScreenDirty) {
		Graphics::Surface *systemScreen = _vm->_system->lockScreen();
		systemScreen->copyRectToSurface(*_shapeScreen, 0, 0, _screenRect);
		_vm->_system->unlockScreen();

		_isScreenDirty = false;
	}
}

void ZoombiniGraphics::clearScreens() {
	uint32 blackColor = kTransparentKey;
	_backScreen->fillRect(_screenRect, blackColor);
	_shapeScreen->fillRect(_screenRect, blackColor);

	_vm->_system->fillScreen(blackColor);
}

void ZoombiniGraphics::copyBackToShapeScreen() {
	// IDA gfx_renderFrame 0x45F352: gfx_blitPortToPort copies background port
	// (pScreenPort_4A79A8) → blitter port (pPortToBlitter_4B9D9C) before shape
	// rendering. Shapes are then drawn directly on top of the background.
	_shapeScreen->copyRectToSurface(*_backScreen, 0, 0, _screenRect);
	recordDirtyRect(kShapeScreen, _screenRect);
}

void ZoombiniGraphics::copyBackToShapeScreen(const Common::Rect &clipRect) {
	// Dirty-rect variant: only restore background within the given clip rect.
	// IDA gfx_renderFrame 0x45F352: the original clips this blit to the dirty
	// region via port_selectActiveRegion (0x45F311).
	Common::Rect rect = clipRect;
	rect.clip(_screenRect);
	if (rect.isEmpty())
		return;
	_shapeScreen->copyRectToSurface(*_backScreen, rect.left, rect.top, rect);
	recordDirtyRect(kShapeScreen, rect);
}

void ZoombiniGraphics::setRenderClipRects(const Common::Array<Common::Rect> &rects) {
	_renderClipRects = rects;
	if (rects.empty()) {
		_hasRenderClipRect = false;
		return;
	}
	_renderClipBounds = rects[0];
	for (uint32 i = 1; i < rects.size(); i++)
		_renderClipBounds.extend(rects[i]);
	_renderClipBounds.clip(_screenRect);
	_hasRenderClipRect = !_renderClipBounds.isEmpty();
}

void ZoombiniGraphics::addRenderClipRect(const Common::Rect &rect) {
	Common::Rect clipped = rect;
	clipped.clip(_screenRect);
	if (clipped.isEmpty())
		return;
	_renderClipRects.push_back(clipped);
	if (_hasRenderClipRect) {
		_renderClipBounds.extend(clipped);
	} else {
		_renderClipBounds = clipped;
		_hasRenderClipRect = true;
	}
}

void ZoombiniGraphics::clearRenderClipRect() {
	_renderClipRects.clear();
	_hasRenderClipRect = false;
}

void ZoombiniGraphics::beginDirtyRectTracking(bool expandRenderClip) {
	_isDirtyRectTracking = true;
	_hasTrackedDirtyBounds = false;
	_expandTrackedDirtyClip = expandRenderClip;
	_trackedDirtyBounds = Common::Rect();
}

Common::Rect ZoombiniGraphics::endDirtyRectTracking() {
	_isDirtyRectTracking = false;
	_expandTrackedDirtyClip = false;
	return _hasTrackedDirtyBounds ? _trackedDirtyBounds : Common::Rect();
}

void ZoombiniGraphics::recordDirtyRect(ScreenKind screenKind, const Common::Rect &rect) {
	if (rect.isEmpty())
		return;

	Graphics::Surface *screen = getScreen(screenKind);

	Common::Rect clipped = rect;
	clipped.clip(screen->w, screen->h);
	if (clipped.isEmpty())
		return;

	if (_isDirtyRectTracking) {
		if (_hasTrackedDirtyBounds) {
			_trackedDirtyBounds.extend(clipped);
		} else {
			_trackedDirtyBounds = clipped;
			_hasTrackedDirtyBounds = true;
		}

		if (_expandTrackedDirtyClip && screenKind == kShapeScreen)
			addRenderClipRect(clipped);
	}

	if (screenKind == kShapeScreen)
		_isScreenDirty = true;

	if (screenKind == kBackScreen && _vm->getCurrentPage())
		_vm->getCurrentPage()->scheduleForceRedraw();
}

void ZoombiniGraphics::clearScreen(ScreenKind screenKind) {
	uint32 blackColor = kTransparentKey;
	Graphics::Surface *screen = _vm->_gfx->getScreen(screenKind);
	screen->fillRect(_screenRect, blackColor);
	recordDirtyRect(screenKind, _screenRect);
}

void ZoombiniGraphics::reinitGraphics(bool trueColor) {
	// Enable true color support only when playing Bink videos; otherwise, use CLUT8 mode.
	bool isTrueColor = _pixelFormat.bytesPerPixel > 1;
	if (trueColor == isTrueColor)
		return;

	clearCache();
	clearCommonCache();

	_backScreen->free();
	_shapeScreen->free();

	if (trueColor) {
		initGraphics(kScreenWidth, kScreenHeight, nullptr);
		_pixelFormat = _vm->_system->getScreenFormat();
	} else {
		initGraphics(kScreenWidth, kScreenHeight);
		_pixelFormat = Graphics::PixelFormat::createFormatCLUT8();
		clearPalette();
	}

	_backScreen->create(kScreenWidth, kScreenHeight, _pixelFormat);
	_shapeScreen->create(kScreenWidth, kScreenHeight, _pixelFormat);
	_isScreenDirty = false;
}

// [*] Handle Cursor
void ZoombiniGraphics::setMouseCursor(MouseCursorResourceId cursorId) {
	if (cursorId == _activeCursorId)
		return;

	switch (cursorId) {
	case kResCursor00_Default:
		_vm->_cursor->setDefaultCursor();
		break;
	case kResCursor01_Watch:
	case kResCursor02_EyeMiddle:
	case kResCursor03_EyeRight:
	case kResCursor04_EyeLeft:
	case kResCursor05_EyeBlink:
		_vm->_cursor->setCursor(static_cast<uint16>(cursorId));
		break;
	default:
		error("Invalid CursorType %d", cursorId);
		break;
	}
	_activeCursorId = cursorId;
}

void ZoombiniGraphics::startMouseCursorEyeAnimation(uint32 currentTimeMs) {
	if (isMouseCursorEyeAnimationActive())
		return;

	setMouseCursor(ZoombiniGraphics::kResCursor02_EyeMiddle);
	_lastMouseCursorEyeAnimationFrameTime = currentTimeMs;
	_mouseCursorEyeAnimationFrameIdx = 0;
}

void ZoombiniGraphics::stopMouseCursorEyeAnimation() {
	if (!isMouseCursorEyeAnimationActive())
		return;

	setMouseCursor(ZoombiniGraphics::kResCursor00_Default);
	_lastMouseCursorEyeAnimationFrameTime = 0;
	_mouseCursorEyeAnimationFrameIdx = 0;
}

void ZoombiniGraphics::runMouseCursorEyeAnimationFrame(uint32 currentTimeMs) {
	if (currentTimeMs - _lastMouseCursorEyeAnimationFrameTime < MohawkEngine_Zoombini::kMouseCursorEyeFrameTimeMs)
		return;
	_lastMouseCursorEyeAnimationFrameTime = currentTimeMs;

	assert(_mouseCursorEyeAnimationFrameIdx < ARRAYSIZE(_mouseCursorEyeAnimationFrames));

	setMouseCursor(_mouseCursorEyeAnimationFrames[_mouseCursorEyeAnimationFrameIdx]);
	_mouseCursorEyeAnimationFrameIdx = (_mouseCursorEyeAnimationFrameIdx + 1) % ARRAYSIZE(_mouseCursorEyeAnimationFrames);
}

bool ZoombiniGraphics::isMouseCursorEyeAnimationActive() const {
	return getMouseCursor() != ZoombiniGraphics::kResCursor00_Default;
}

// [*] Handle Bitmap
void ZoombiniGraphics::drawBackground(uint16 image) {
	drawBackground(kBackScreen, image);
}

void ZoombiniGraphics::drawBackground(ScreenKind screenKind, uint16 image) {
	MohawkSurface *imgSurface = findImage(ZmbResource(ZmbArchiveKind::kPage, image));
	Graphics::Surface *rawSurface = findImage(ZmbResource(ZmbArchiveKind::kPage, image))->getSurface();
	Common::Rect imageRect(0, 0, rawSurface->w, rawSurface->h);
	drawImageSectionToScreen(screenKind, imgSurface, imageRect, _screenRect);
}

void ZoombiniGraphics::drawImage(ScreenKind screenKind, uint16 image, const Common::Point &destPos) {
	MohawkSurface *imgSurface = findImage(ZmbResource(ZmbArchiveKind::kPage, image));
	Graphics::Surface *rawSurface = imgSurface->getSurface();
	Common::Rect srcRect(0, 0, rawSurface->w, rawSurface->h);
	Common::Rect dstRect(destPos, rawSurface->w, rawSurface->h);
	drawImageSectionToScreen(screenKind, imgSurface, srcRect, dstRect);
}

Common::Rect ZoombiniGraphics::drawShape(ScreenKind screenKind, ZmbResource imgResource, uint16 shapeIdx, const Common::Point &destPos, bool clearBeforeRender) {
	return drawSubImage(screenKind, imgResource, shapeIdx - 1, destPos, clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawShape(ScreenKind screenKind, ZmbResource imgResource, uint16 shapeIdx, const Common::Rect &destRect, bool clearBeforeRender) {
	return drawSubImage(screenKind, imgResource, shapeIdx - 1, destRect, clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawShape(ScreenKind screenKind, ZmbResource imgResource, const ZmbHotspot *hotspot, bool clearBeforeRender) {
	return drawSubImage(screenKind, imgResource, hotspot->getSubImageId(), hotspot->getPos(), clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawSubImage(ScreenKind screenKind, ZmbResource imgResource, uint16 subImage, const Common::Point &destPos, bool clearBeforeRender) {
	assert(subImage != UINT16_MAX); // -1 check
	MohawkSurface *rawSurface = findSubImage(imgResource, subImage);
	Graphics::Surface *surface = rawSurface->getSurface();

	Common::Rect srcRect(0, 0, surface->w, surface->h);
	Common::Rect dstRect(destPos, surface->w, surface->h);
	return drawImageSectionToScreen(screenKind, rawSurface, srcRect, dstRect, clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawSubImage(ScreenKind screenKind, ZmbResource imgResource, uint16 subImage, const Common::Rect &destRect, bool clearBeforeRender) {
	assert(subImage != UINT16_MAX); // -1 check
	MohawkSurface *rawSurface = findSubImage(imgResource, subImage);
	Graphics::Surface *surface = rawSurface->getSurface();

	Common::Rect srcRect(0, 0, surface->w, surface->h);

	// If the destRect is larger than shape's actual size, align to the center.
	Common::Point startPos(destRect.left, destRect.top);
	if (surface->w < destRect.width())
		startPos.x += (destRect.width() - surface->w) / 2;
	if (surface->h < destRect.height())
		startPos.y += (destRect.height() - surface->h) / 2;
	Common::Rect dstRect(startPos, surface->w, surface->h);
	return drawImageSectionToScreen(screenKind, rawSurface, srcRect, dstRect, clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawImageSectionToScreen(ScreenKind screenKind, MohawkSurface *mhkSurface, const Common::Rect &srcRect, const Common::Rect &dstRect, bool clearBeforeRender) {
	Graphics::Surface *srcSurface = mhkSurface->getSurface();
	Graphics::Surface *screen = getScreen(screenKind);

	assert(srcRect.isValidRect() && dstRect.isValidRect());
	assert(srcRect.left >= 0 && srcRect.top >= 0);

	Common::Rect clipSrcRect = srcRect;
	Common::Rect clipDstRect = dstRect;
	clipSrcRect.clip(srcSurface->w, srcSurface->h);

	// Bail out early if the sprite is entirely off-screen.
	if (clipDstRect.right <= 0 || clipDstRect.bottom <= 0 ||
		clipDstRect.left >= screen->w || clipDstRect.top >= screen->h)
		return Common::Rect(0, 0, 0, 0);

	// Left/top clipping: when dstRect extends beyond the left or top screen edge,
	// advance srcRect by the same amount so we skip the off-screen source pixels.
	// Without this, sprites at negative coordinates (e.g. walk-in snoids at x=-50)
	// have their FULL source drawn starting at x=0, appearing fully on-screen
	// instead of being properly clipped.
	if (clipDstRect.left < 0) {
		clipSrcRect.left += -clipDstRect.left;
		clipDstRect.left = 0;
	}
	if (clipDstRect.top < 0) {
		clipSrcRect.top += -clipDstRect.top;
		clipDstRect.top = 0;
	}

	clipDstRect.clip(screen->w, screen->h);

	if (clipSrcRect.isEmpty() || clipDstRect.isEmpty())
		return Common::Rect(0, 0, 0, 0);

	// Right/bottom clipping: when dstRect.left + srcRect.width() exceeds screen
	// dimensions, trim the source rect from the right/bottom edge.
	if (screen->w < clipDstRect.left + clipSrcRect.width())
		clipSrcRect.right -= (clipDstRect.left + clipSrcRect.width() - screen->w);
	if (screen->h < clipDstRect.top + clipSrcRect.height())
		clipSrcRect.bottom -= (clipDstRect.top + clipSrcRect.height() - screen->h);

	// IDA runner_preRenderStandard (0x4619A1) at LABEL_70:
	// The original computes clickRect from hotspot positions + REGS shape sizes
	// during preRender, INDEPENDENT of the dirty-rect clip.  The drawn pixels
	// are clipped through port_selectActiveRegion, but clickRect is always the
	// full bounding box of the current frame's shapes.
	//
	// We emulate this by capturing the screen-clipped destination rect as the
	// "logical rect" BEFORE applying the render clip rect.  blitShapes() uses
	// the returned rect for sortRect/clickRect, making it independent of the
	// current dirty region — just like the original's metadata-based clickRect.
	Common::Rect logicalRect(clipDstRect.left, clipDstRect.top,
							 clipDstRect.left + clipSrcRect.width(),
							 clipDstRect.top + clipSrcRect.height());
	recordDirtyRect(screenKind, logicalRect);

	// IDA: port_selectActiveRegion (0x48F40C) — confine drawing to dirty region.
	// The original engine uses GDI clip regions (union of rectangles) set on the
	// port's HDC.  We replicate this by iterating individual dirty rects and
	// drawing only the intersection of the sprite with each dirty rect.
	// Non-dirty pixels persist on the shape-screen from previous frames.
	if (_hasRenderClipRect) {
		// Quick bounding-box reject: if the sprite doesn't touch the dirty
		// region's bounding box at all, skip drawing entirely.
		if (!clipDstRect.intersects(_renderClipBounds))
			return logicalRect;

		// Draw the sprite once for each intersecting dirty rect.
		for (const Common::Rect &dirtyRect : _renderClipRects) {
			Common::Rect subDst = clipDstRect;
			subDst.clip(dirtyRect);
			if (subDst.isEmpty())
				continue;

			Common::Rect subSrc = clipSrcRect;
			subSrc.left += subDst.left - clipDstRect.left;
			subSrc.top += subDst.top - clipDstRect.top;
			subSrc.right -= clipDstRect.right - subDst.right;
			subSrc.bottom -= clipDstRect.bottom - subDst.bottom;

			if (clearBeforeRender)
				screen->fillRect(subDst, kTransparentKey);
			screen->copyRectToSurfaceWithKey(*srcSurface, subDst.left, subDst.top, subSrc, kTransparentKey);
		}
		return logicalRect;
	}

	if (clearBeforeRender)
		screen->fillRect(clipDstRect, kTransparentKey);

	screen->copyRectToSurfaceWithKey(*srcSurface, clipDstRect.left, clipDstRect.top, clipSrcRect, kTransparentKey);

	return logicalRect;
}

void ZoombiniGraphics::drawLine(ScreenKind screenKind, const Common::Point &start, const Common::Point &end, uint32 color) {
	Graphics::Surface *screen = getScreen(screenKind);
	Common::Rect dirtyRect(MIN(start.x, end.x), MIN(start.y, end.y),
						   MAX(start.x, end.x) + 1, MAX(start.y, end.y) + 1);
	recordDirtyRect(screenKind, dirtyRect);
	screen->drawLine(start.x, start.y, end.x, end.y, color);
}

void ZoombiniGraphics::drawThickLine(ScreenKind screenKind, const Common::Point &start, const Common::Point &end, int penX, int penY, uint32 color) {
	Graphics::Surface *screen = getScreen(screenKind);
	Common::Rect dirtyRect(MIN(start.x, end.x) - penX, MIN(start.y, end.y) - penY,
						   MAX(start.x, end.x) + penX + 1, MAX(start.y, end.y) + penY + 1);
	recordDirtyRect(screenKind, dirtyRect);
	screen->drawThickLine(start.x, start.y, end.x, end.y, penX, penY, color);
}

void ZoombiniGraphics::clearArea(ScreenKind screenKind, ZmbDrawRecord *record) {
	fillArea(screenKind, record->_drawnRect, kTransparentKey);
}

void ZoombiniGraphics::clearArea(ScreenKind screenKind, ZmbResource imgResource, const ZmbHotspot *hotspot) {
	fillArea(screenKind, imgResource, hotspot, kTransparentKey);
}

void ZoombiniGraphics::clearArea(ScreenKind screenKind, const Common::Rect &rect) {
	fillArea(screenKind, rect, kTransparentKey);
}

void ZoombiniGraphics::fillArea(ScreenKind screenKind, ZmbDrawRecord *record, uint32 color) {
	fillArea(screenKind, record->_drawnRect, color);
}

void ZoombiniGraphics::fillArea(ScreenKind screenKind, ZmbResource imgResource, const ZmbHotspot *hotspot, uint32 color) {
	MohawkSurface *rawSurface = findShape(imgResource, hotspot->getSubImageId());
	Graphics::Surface *surface = rawSurface->getSurface();
	Graphics::Surface *screen = getScreen(screenKind);

	Common::Rect dstRect(hotspot->getPos(), surface->w, surface->h);
	recordDirtyRect(screenKind, dstRect);
	screen->fillRect(dstRect, color);
}

void ZoombiniGraphics::fillArea(ScreenKind screenKind, const Common::Rect &rect, uint32 color) {
	Graphics::Surface *screen = getScreen(screenKind);

	recordDirtyRect(screenKind, rect);
	if (_hasRenderClipRect) {
		for (const Common::Rect &dirtyRect : _renderClipRects) {
			Common::Rect clipped = rect;
			clipped.clip(dirtyRect);
			if (!clipped.isEmpty())
				screen->fillRect(clipped, color);
		}
		return;
	}
	if (!rect.isEmpty())
		screen->fillRect(rect, color);
}

void ZoombiniGraphics::fillArea(ScreenKind screenKind, uint32 color) {
	fillArea(screenKind, _screenRect, color);
}

void ZoombiniGraphics::drawText(ScreenKind screenKind, uint32 textKey, const Common::Rect &destRect) {
	drawText(screenKind, textKey, destRect, TextConf());
}

void ZoombiniGraphics::drawText(ScreenKind screenKind, uint32 textKey, const Common::Rect &destRect, const TextConf &tc) {
	const Common::U32String &text = _vm->_text->getLocalizedString(textKey);
	drawText(screenKind, text, destRect, tc);
}

void ZoombiniGraphics::drawText(ScreenKind screenKind, const Common::U32String &text, const Common::Rect &destRect) {
	return drawText(screenKind, text, destRect, TextConf());
}

void ZoombiniGraphics::drawText(ScreenKind screenKind, const Common::U32String &text, const Common::Rect &destRect, const TextConf &tc) {
	const Graphics::Font *font = _vm->_text->getFont(tc._fontUsage);
	if (!font)
		error("Zoombini: cannot open fontfile of kind %u", static_cast<uint32>(tc._fontUsage));

	const Common::Array<Common::U32String> &lines = prepareTextLines(text, font, tc._wordWrap, destRect.width());

	// Calculate total draw height and max draw width
	Common::Point boundSize = getTextLinesBounds(font, tc._outlineEffect, lines);

	// Adjust drawRect according to boundRect
	Common::Rect drawRect = destRect;
	drawRect.setWidth(MAX(drawRect.width(), boundSize.x));
	drawRect.setHeight(MAX(drawRect.height(), boundSize.y));

	// Handle background fill
	uint32 fillBackgroundPalette = kTransparentKey;
	if (tc._fillBackground)
		fillBackgroundPalette = tc._fillBackgroundPalette;

	// Virtualize Vertical Aligment
	if (0 < text.size()) {
		switch (tc._vAlign) {
		case Graphics::kTextAlignStart:
		case Graphics::kTextAlignLeft:
			// Do nothing
			break;
		case Graphics::kTextAlignCenter:
			drawRect.top = (drawRect.top + drawRect.bottom - boundSize.y) / 2;
			drawRect.bottom = drawRect.top + boundSize.y;
			break;
		case Graphics::kTextAlignEnd:
		case Graphics::kTextAlignRight:
			drawRect.top = drawRect.bottom - boundSize.y;
			break;
		default:
			error("Invalid vertical alignment %d", tc._vAlign);
			break;
		}
	}

	// Mimick outlined text rendering of Zoombini engine
	if (tc._outlineEffect) {
		for (uint32 i = 0; i < 4; i++) {
			Common::Rect outlineRect = drawRect;
			uint16 xDelta = 0;
			uint16 yDelta = 0;
			switch (i) {
			case 0:
				xDelta -= 1;
				break;
			case 1:
				yDelta -= 1;
				break;
			case 2:
				xDelta += 1;
				break;
			case 3:
				yDelta += 1;
				break;
			}
			outlineRect.left += xDelta;
			outlineRect.right += xDelta;
			outlineRect.top += yDelta;
			outlineRect.bottom += yDelta;

			recordDirtyRect(screenKind, outlineRect);
			drawTextLines(screenKind, font, lines, outlineRect, tc._outlinePalette, tc._hAlign, fillBackgroundPalette);
		}
	}

	recordDirtyRect(screenKind, drawRect);
	drawTextLines(screenKind, font, lines, drawRect, tc._textPalette, tc._hAlign, fillBackgroundPalette);
}

Common::Point ZoombiniGraphics::getTextBounds(const Common::U32String &text, int16 targetWidth, const TextConf &tc) {
	// If text is empty, return distance of font top and baseline as a height.
	if (text.empty()) {
		return Common::Point(0, getFontHeight(tc));
	}

	const Graphics::Font *font = _vm->_text->getFont(tc._fontUsage);
	if (!font)
		error("Zoombini: cannot open fontfile of kind %u", static_cast<uint32>(tc._fontUsage));

	const Common::Array<Common::U32String> &lines = prepareTextLines(text, font, tc._wordWrap, targetWidth);
	return getTextLinesBounds(font, tc._outlineEffect, lines);
}

int16 ZoombiniGraphics::getFontHeight(const TextConf &tc) {
	const Graphics::Font *font = _vm->_text->getFont(tc._fontUsage);
	if (!font)
		error("Zoombini: cannot open fontfile of kind %u", static_cast<uint32>(tc._fontUsage));

	return font->getFontHeight();
}

Common::Array<Common::U32String> ZoombiniGraphics::prepareTextLines(const Common::U32String &text, const Graphics::Font *font, bool wordWrap, int16 targetWidth) {
	// Tokenize strings with CR, LF or CRLF
	Common::Array<Common::U32String> lines = ZoombiniText::tokenizeLines(text);

	if (!wordWrap)
		return lines;

	// Handle word-wrapping
	Common::Array<Common::U32String> newLines;
	for (const Common::U32String &line : lines) {
		// Splice rawLine into a word-wrapped lines
		Common::Array<Common::U32String> wrapLines;
		font->wordWrapText(line, targetWidth, wrapLines);
		newLines.push_back(wrapLines);
	}
	return newLines;
}

Common::Point ZoombiniGraphics::getTextLinesBounds(const Graphics::Font *font, bool outlineEffect, const Common::Array<Common::U32String> &lines) {
	// Use font->getFontHeight() for line height, matching GDI DrawTextA with DT_EXTERNALLEADING.
	// getFontHeight() = ascender + descender + lineGap (tmHeight + tmExternalLeading in GDI terms).
	const int16 lineHeight = font->getFontHeight();
	int16 drawTotalHeight = 0;
	Common::Point boundSize;
	for (const Common::U32String &line : lines) {
		Common::Rect bbox = font->getBoundingBox(line, 0, 0, _screenRect.width(), Graphics::kTextAlignLeft);

		drawTotalHeight += lineHeight;
		boundSize.x = MAX(boundSize.x, static_cast<int16>(bbox.width()));
		boundSize.y = MAX(boundSize.y, drawTotalHeight);
	}

	if (outlineEffect) {
		// Outline adds 1 pixel in each direction (L/U/R/D)
		boundSize.x += 2;
		boundSize.y += 2;
	}

	return boundSize;
}

void ZoombiniGraphics::drawTextLines(ScreenKind screenKind, const Graphics::Font *font, const Common::Array<Common::U32String> &lines, const Common::Rect &destRect, uint32 palette, Graphics::TextAlign hAlign, uint32 fillBackgroundColor) {
	// IDA drawTextCore_48D104: text drawing goes through the port's active clip
	// region, just like shape blitting.  Skip entirely if destRect is outside
	// the render clip region's bounding box.
	if (_hasRenderClipRect && !destRect.intersects(_renderClipBounds))
		return;

	// Use font->getFontHeight() for line advancement, matching GDI DrawTextA with DT_EXTERNALLEADING.
	const int lineHeight = font->getFontHeight();
	Common::Rect drawRect = destRect;
	Graphics::Surface *screen = _vm->_gfx->getScreen(screenKind);

	for (uint32 i = 0; i < lines.size(); i++) {
		const Common::U32String &line = lines[i];

		// Clip: skip lines whose top is below the dest rect bottom (matching GDI IntersectClipRect)
		if (drawRect.top >= destRect.bottom)
			break;

		// Background is for debug purposes, Zoombini game itself does not use this feature
		if (fillBackgroundColor != kTransparentKey) {
			const Common::Rect &bbox = font->getBoundingBox(line, drawRect.left, drawRect.top, drawRect.width(), hAlign);
			screen->fillRect(bbox, fillBackgroundColor);
		}

		// Draw the text line by line, clipped to destRect width
		font->drawString(screen, line, drawRect.left, drawRect.top, drawRect.width(), palette, hAlign);

		if (i + 1 < lines.size()) {
			drawRect.top += lineHeight;
			drawRect.bottom += lineHeight;
		}
	}
}

// [*] Transitions and effects
void ZoombiniGraphics::queueFadeEffect(FadeType type, uint32 duration) {
	_fadeQueue.push(FadeEffect(type, duration));
}

bool ZoombiniGraphics::applyFadeEffect(uint32 currentTime) {
	if (_fadeQueue.empty())
		return false;

	FadeEffect &fe = _fadeQueue.front();
	if (!fe._isFading) {
		fe._isFading = true;
		fe._startTime = currentTime;
	}
	uint32 steps = fe._duration / MohawkEngine_Zoombini::kTargetFrameTimeMs;
	uint32 elapsedTime = currentTime - fe._startTime;
	if (elapsedTime <= fe._duration) { // Effect in progress
		uint32 stepIdx = MIN<uint32>(elapsedTime / MohawkEngine_Zoombini::kTargetFrameTimeMs, steps);
		switch (fe._type) {
		case kFadeIn:
			dimPalette(stepIdx, steps);
			break;
		case kFadeOut:
			dimPalette(steps - stepIdx, steps);
			break;
		default:
			error("Invalid fade effect type: %d", fe._type);
			break;
		}
		return true;
	} else { // Effect completed
		switch (fe._type) {
		case kFadeIn:
			dimPalette(steps, steps);
			break;
		case kFadeOut:
			dimPalette(0, steps);
			break;
		default:
			error("Invalid fade effect type: %d", fe._type);
			break;
		}

		_fadeQueue.pop();
		return false;
	}
}

bool ZoombiniGraphics::isFading() const {
	return !_fadeQueue.empty() && _fadeQueue.front()._isFading;
}

void ZoombiniGraphics::dimPalette(uint16 idx, uint16 steps) {
	assert(idx <= steps);
	assert(0 <= idx);

	const size_t bufSize = ARRAYSIZE(_paletteBytes);
	byte *fadePalette = new byte[bufSize];
	memset(fadePalette, 0, bufSize);

	for (uint16 i = 0; i < bufSize; i++)
		fadePalette[i] = static_cast<byte>(static_cast<uint32>(_paletteBytes[i]) * idx / steps);

	_vm->_system->getPaletteManager()->setPalette(fadePalette, 0, bufSize / 3);
	delete[] fadePalette;

	_vm->_system->updateScreen();
}

void ZoombiniGraphics::scalePalettePartial(uint16 startEntry, uint16 count, uint8 scalePercent) {
	const size_t bufSize = ARRAYSIZE(_paletteBytes);
	byte *modified = new byte[bufSize];
	memcpy(modified, _paletteBytes, bufSize);

	uint16 endEntry = (uint16)MIN<uint32>((uint32)startEntry + count, 256);
	for (uint16 i = startEntry; i < endEntry; i++) {
		for (int c = 0; c < 3; c++) {
			modified[i * 3 + c] = (byte)(((uint16)modified[i * 3 + c]) * scalePercent / 100);
		}
	}

	_vm->_system->getPaletteManager()->setPalette(modified, 0, bufSize / 3);
	delete[] modified;
}

MohawkSurface *ZoombiniGraphics::findImage(ZmbResource imgResource) {
	switch (imgResource._archiveKind) {
	case ZmbArchiveKind::kSystem:
		if (!_sysImageCache.contains(imgResource._id))
			_sysImageCache[imgResource._id] = decodeImage(imgResource);
		return _sysImageCache[imgResource._id];
	case ZmbArchiveKind::kPage:
		return GraphicsManager::findImage(imgResource._id);
	default:
		error("Invalid ZmbArchiveKind: %d", static_cast<int>(imgResource._archiveKind));
		break;
	}
	return nullptr;
}

MohawkSurface *ZoombiniGraphics::findShape(ZmbResource imgResource, uint16 shapeIdx) {
	return findSubImage(imgResource, shapeIdx - 1);
}

MohawkSurface *ZoombiniGraphics::findSubImage(ZmbResource imgResource, uint16 subImage) {
	switch (imgResource._archiveKind) {
	case ZmbArchiveKind::kSystem: {
		if (!_sysSubImageCache.contains(imgResource._id))
			_sysSubImageCache[imgResource._id] = decodeImages(imgResource);
		Common::Array<MohawkSurface *> &sysImages = _sysSubImageCache[imgResource._id];
		if (subImage >= sysImages.size()) {
			warning("ZoombiniGraphics::findSubImage: subImage %u out of bounds (size %u) for system resource %u", subImage, sysImages.size(), imgResource._id);
			subImage = 0;
		}
		return sysImages[subImage];
	}
	case ZmbArchiveKind::kPage:
		return GraphicsManager::findSubImage(imgResource._id, subImage);
	default:
		error("Invalid ZmbArchiveKind: %d", static_cast<int>(imgResource._archiveKind));
		break;
	}
	return nullptr;
}

Common::Rect ZoombiniGraphics::getShapeSize(ZmbResource imgResource, uint16 shapeIdx) {
	return getSubImageSize(imgResource, shapeIdx - 1);
}

Common::Rect ZoombiniGraphics::getSubImageSize(ZmbResource imgResource, uint16 subImage) {
	const MohawkSurface *mhkSurface = findSubImage(imgResource, subImage);
	if (!mhkSurface)
		error("Cannot find shapeIdx(%u) in image(%u)", subImage, imgResource._id);
	const Graphics::Surface *imgSurface = mhkSurface->getSurface();
	if (!imgSurface)
		error("Cannot get image surface from subImage(%u) in image(%u)", subImage, imgResource._id);
	return Common::Rect(imgSurface->w, imgSurface->h);
}

uint32 ZoombiniGraphics::getShapeCount(ZmbResource imgResource) {
	switch (imgResource._archiveKind) {
	case ZmbArchiveKind::kSystem:
		if (!_sysSubImageCache.contains(imgResource._id))
			_sysSubImageCache[imgResource._id] = decodeImages(imgResource);
		return _sysSubImageCache[imgResource._id].size();
	case ZmbArchiveKind::kPage:
		return GraphicsManager::getSubImageCount(imgResource._id);
	default:
		error("Invalid ZmbArchiveKind: %d", static_cast<int>(imgResource._archiveKind));
		return 0;
	}
}

void ZoombiniGraphics::clearCommonCache() {
	for (Common::HashMap<uint16, MohawkSurface *>::iterator it = _sysImageCache.begin(); it != _sysImageCache.end(); it++)
		delete it->_value;

	for (Common::HashMap<uint16, Common::Array<MohawkSurface *>>::iterator it = _sysSubImageCache.begin(); it != _sysSubImageCache.end(); it++) {
		Common::Array<MohawkSurface *> &array = it->_value;
		for (MohawkSurface *surface : array)
			delete surface;
	}

	_sysImageCache.clear();
	_sysSubImageCache.clear();
}

// [*] 256color Palette
void ZoombiniGraphics::setPalette(uint16 id) {
	if (!readPalette(id, _paletteBytes, ARRAYSIZE(_paletteBytes))) {
		error("Could not read palette from SHPL p:%04u", id);
		return;
	}

	_vm->_system->getPaletteManager()->setPalette(_paletteBytes, 0, ARRAYSIZE(_paletteBytes) / 3);
}

void ZoombiniGraphics::rotatePaletteRight(uint16 startEntry, uint16 count) {
	if (count < 2 || startEntry >= 256)
		return;

	uint16 endEntry = (uint16)MIN<uint32>((uint32)startEntry + count, 256);
	if (endEntry - startEntry < 2)
		return;

	byte saved[3];
	const uint16 lastEntry = endEntry - 1;
	memcpy(saved, &_paletteBytes[lastEntry * 3], sizeof(saved));
	memmove(&_paletteBytes[(startEntry + 1) * 3], &_paletteBytes[startEntry * 3],
			(endEntry - startEntry - 1) * 3);
	memcpy(&_paletteBytes[startEntry * 3], saved, sizeof(saved));

	_vm->_system->getPaletteManager()->setPalette(_paletteBytes, 0, ARRAYSIZE(_paletteBytes) / 3);
}

bool ZoombiniGraphics::readPalette(uint16 id, byte *destBuf, size_t destBufSize) {
	if (!destBuf || destBufSize == 0)
		return false;

	// Do not clear the readPalette, the old value must be kept when the value is not overwritten (e.g. XFER palette).
	Common::SeekableReadStream *shplStream = _vm->getResource(ID_SHPL, ZmbResource(ZmbArchiveKind::kPage, id));
	uint16 shplId = shplStream->readUint16BE();
	assert(shplId == id);
	shplStream->readUint16BE(); // Always 00 01
	uint16 paletteColorStart = shplStream->readUint16BE();
	uint16 paletteColorCount = shplStream->readUint16BE();
	assert(paletteColorStart <= 255);
	assert(paletteColorStart + paletteColorCount <= 256);

	// Is size of the buffer enough?
	if (destBufSize < 3 * (paletteColorStart + paletteColorCount))
		return false;

	for (uint16 i = paletteColorStart; i < paletteColorStart + paletteColorCount; i++) {
		destBuf[i * 3 + 0] = shplStream->readByte();
		destBuf[i * 3 + 1] = shplStream->readByte();
		destBuf[i * 3 + 2] = shplStream->readByte();
		shplStream->readByte(); // Skip flags byte
	}

	delete shplStream;

	// Apply brightness adjustment if enabled (matches original readSHPLbody_4512A5 behavior)
	if (ConfMan.getBool("brighten_palette")) {
		for (uint16 i = paletteColorStart; i < paletteColorStart + paletteColorCount; i++) {
			for (int ch = 0; ch < 3; ch++) {
				byte &v = destBuf[i * 3 + ch];
				if (v != 0)
					v = v + 31 - (v >> 3);
			}
		}
	}

	return true;
}

void ZoombiniGraphics::clearPalette() {
	// Set the palette to all black
	memset(_paletteBytes, 0, sizeof(_paletteBytes));
	_vm->_system->getPaletteManager()->setPalette(_paletteBytes, 0, ARRAYSIZE(_paletteBytes) / 3);
}

MohawkSurface *ZoombiniGraphics::decodeImage(uint16 id) {
	return _bmpDecoder->decodeImage(_vm->getResource(ID_TBMP, ZmbResource(ZmbArchiveKind::kPage, id)));
}

MohawkSurface *ZoombiniGraphics::decodeImage(ZmbResource imgResource) {
	return _bmpDecoder->decodeImage(_vm->getResource(ID_TBMP, imgResource));
}

Common::Array<MohawkSurface *> ZoombiniGraphics::decodeImages(uint16 id) {
	return _bmpDecoder->decodeImages(_vm->getResource(ID_TBMP, ZmbResource(ZmbArchiveKind::kPage, id)));
}

Common::Array<MohawkSurface *> ZoombiniGraphics::decodeImages(ZmbResource imgResource) {
	return _bmpDecoder->decodeImages(_vm->getResource(ID_TBMP, imgResource));
}

} // End of namespace Mohawk
