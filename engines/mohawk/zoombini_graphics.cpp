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

#include "common/substream.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "engines/util.h"
#include "graphics/fontman.h"
#include "graphics/fonts/ttf.h"
#include "graphics/paletteman.h"

#include "mohawk/resource.h"
#include "mohawk/cursors.h"
#include "mohawk/zoombini.h"
#include "mohawk/zoombini_scripts.h"
#include "mohawk/zoombini_text.h"
#include "mohawk/zoombini_graphics.h"

namespace Mohawk {

ZoombiniGraphics::ZoombiniGraphics(MohawkEngine_Zoombini *vm) : GraphicsManager(), _vm(vm),
																_bmpDecoder(new MohawkBitmap()),
																_screenRect(Common::Rect(kScreenWidth, kScreenHeight)),
																_systemPaletteLoaded(false) {
	initGraphics(_screenRect.width(), _screenRect.height());
	clearPalette();

	_pixelFormat = Graphics::PixelFormat::createFormatCLUT8();
	memset(_palette, 0, sizeof(_palette));
	memset(_systemPalette, 0, sizeof(_systemPalette));

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

void ZoombiniGraphics::captureComposedScreen(ScreenKind destScreenKind) {
	Graphics::Surface *destScreen = _vm->_gfx->getScreen(destScreenKind);

	Graphics::Surface *systemScreen = _vm->_system->lockScreen();
	destScreen->copyFrom(*systemScreen);
	_vm->_system->unlockScreen();
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
}

void ZoombiniGraphics::clearScreen(ScreenKind screenKind) {
	uint32 blackColor = kTransparentKey;
	Graphics::Surface *screen = _vm->_gfx->getScreen(screenKind);
	screen->fillRect(_screenRect, blackColor);
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
	drawBackground(getBackScreen(), image);
}

void ZoombiniGraphics::drawBackground(ScreenKind screenKind, uint16 image) {
	drawBackground(getScreen(screenKind), image);
}

void ZoombiniGraphics::drawBackground(Graphics::Surface *screen, uint16 image) {
	MohawkSurface *imgSurface = findImage(ZmbResource(ZmbArchiveKind::kPage, image));
	Graphics::Surface *rawSurface = findImage(ZmbResource(ZmbArchiveKind::kPage, image))->getSurface();
	Common::Rect imageRect(0, 0, rawSurface->w, rawSurface->h);
	drawImageSectionToScreen(screen, imgSurface, imageRect, _screenRect);
}

void ZoombiniGraphics::drawImage(ScreenKind screenKind, uint16 image, const Common::Point &destPos) {
	MohawkSurface *imgSurface = findImage(ZmbResource(ZmbArchiveKind::kPage, image));
	Graphics::Surface *rawSurface = imgSurface->getSurface();
	Common::Rect srcRect(0, 0, rawSurface->w, rawSurface->h);
	Common::Rect dstRect(destPos, rawSurface->w, rawSurface->h);
	drawImageSectionToScreen(getScreen(screenKind), imgSurface, srcRect, dstRect);
}

Common::Rect ZoombiniGraphics::drawShape(ScreenKind screenKind, ZmbResource imgResource, uint16 shapeIdx, const Common::Point &destPos, bool clearBeforeRender) {
	return drawShape(getScreen(screenKind), imgResource, shapeIdx, destPos, clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawShape(ScreenKind screenKind, ZmbResource imgResource, uint16 shapeIdx, const Common::Rect &destRect, bool clearBeforeRender) {
	return drawShape(getScreen(screenKind), imgResource, shapeIdx, destRect, clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawShape(ScreenKind screenKind, ZmbResource imgResource, const ZmbHotspot *hotspot, bool clearBeforeRender) {
	return drawShape(getScreen(screenKind), imgResource, hotspot, clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawShape(Graphics::Surface *screen, ZmbResource imgResource, uint16 shapeIdx, const Common::Point &destPos, bool clearBeforeRender) {
	return drawSubImage(screen, imgResource, shapeIdx - 1, destPos, clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawShape(Graphics::Surface *screen, ZmbResource imgResource, uint16 shapeIdx, const Common::Rect &destRect, bool clearBeforeRender) {
	return drawSubImage(screen, imgResource, shapeIdx - 1, destRect, clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawShape(Graphics::Surface *screen, ZmbResource imgResource, const ZmbHotspot *hotspot, bool clearBeforeRender) {
	return drawSubImage(screen, imgResource, hotspot->getSubImageId(), hotspot->getPos(), clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawSubImage(ScreenKind screenKind, ZmbResource imgResource, uint16 subImage, const Common::Point &destPos, bool clearBeforeRender) {
	return drawSubImage(getScreen(screenKind), imgResource, subImage, destPos, clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawSubImage(Graphics::Surface *screen, ZmbResource imgResource, uint16 subImage, const Common::Point &destPos, bool clearBeforeRender) {
	assert(subImage != UINT16_MAX); // -1 check
	MohawkSurface *rawSurface = findSubImage(imgResource, subImage);
	Graphics::Surface *surface = rawSurface->getSurface();

	Common::Rect srcRect(0, 0, surface->w, surface->h);
	Common::Rect dstRect(destPos, surface->w, surface->h);
	return drawImageSectionToScreen(screen, rawSurface, srcRect, dstRect, clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawSubImage(ScreenKind screenKind, ZmbResource imgResource, uint16 subImage, const Common::Rect &destRect, bool clearBeforeRender) {
	return drawSubImage(getScreen(screenKind), imgResource, subImage, destRect, clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawSubImage(Graphics::Surface *screen, ZmbResource imgResource, uint16 subImage, const Common::Rect &destRect, bool clearBeforeRender) {
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
	return drawImageSectionToScreen(screen, rawSurface, srcRect, dstRect, clearBeforeRender);
}

Common::Rect ZoombiniGraphics::drawImageSectionToScreen(Graphics::Surface *screen, MohawkSurface *mhkSurface, const Common::Rect &srcRect, const Common::Rect &dstRect, bool clearBeforeRender) {
	Graphics::Surface *srcSurface = mhkSurface->getSurface();

	assert(screen != nullptr);
	assert(srcRect.isValidRect() && dstRect.isValidRect());
	assert(srcRect.left >= 0 && srcRect.top >= 0);

	Common::Rect clipSrcRect = srcRect;
	Common::Rect clipDstRect = dstRect;
	clipSrcRect.clip(srcSurface->w, srcSurface->h);
	clipDstRect.clip(screen->w, screen->h);

	if (screen->w <= clipDstRect.left)
		return Common::Rect(0, clipDstRect.top, 0, clipDstRect.bottom);
	if (screen->h <= clipDstRect.top)
		return Common::Rect(clipDstRect.left, 0, clipDstRect.right, 0);

	if (screen->w < clipDstRect.left + clipSrcRect.width())
		clipSrcRect.right -= (clipDstRect.left + clipSrcRect.width() - screen->w);
	if (screen->h < clipDstRect.top + clipSrcRect.height())
		clipSrcRect.bottom -= (clipDstRect.top + clipSrcRect.height() - screen->h);

	if (clearBeforeRender)
		screen->fillRect(clipDstRect, kTransparentKey);

	screen->copyRectToSurfaceWithKey(*srcSurface, clipDstRect.left, clipDstRect.top, clipSrcRect, kTransparentKey);
	_isScreenDirty = true;

	return Common::Rect(clipDstRect.left, clipDstRect.top,
						clipDstRect.left + clipSrcRect.width(), clipDstRect.top + clipSrcRect.height());
}

void ZoombiniGraphics::drawLine(ScreenKind screenKind, const Common::Point &start, const Common::Point &end, uint32 color) {
	Graphics::Surface *screen = getScreen(screenKind);
	screen->drawLine(start.x, start.y, end.x, end.y, color);
}

void ZoombiniGraphics::drawThickLine(ScreenKind screenKind, const Common::Point &start, const Common::Point &end, int penX, int penY, uint32 color) {
	Graphics::Surface *screen = getScreen(screenKind);
	screen->drawThickLine(start.x, start.y, end.x, end.y, penX, penY, color);
}

void ZoombiniGraphics::clearArea(ScreenKind screenKind, ZmbDrawRecord *record) {
	clearArea(getScreen(screenKind), record);
}

void ZoombiniGraphics::clearArea(ScreenKind screenKind, ZmbResource imgResource, const ZmbHotspot *hotspot) {
	clearArea(getScreen(screenKind), imgResource, hotspot);
}

void ZoombiniGraphics::clearArea(ScreenKind screenKind, const Common::Rect &rect) {
	clearArea(getScreen(screenKind), rect);
}

void ZoombiniGraphics::clearArea(Graphics::Surface *screen, ZmbDrawRecord *record) {
	fillArea(screen, record, kTransparentKey);
}

void ZoombiniGraphics::clearArea(Graphics::Surface *screen, ZmbResource imgResource, const ZmbHotspot *hotspot) {
	fillArea(screen, imgResource, hotspot, kTransparentKey);
}

void ZoombiniGraphics::clearArea(Graphics::Surface *screen, const Common::Rect &rect) {
	fillArea(screen, rect, kTransparentKey);
}

void ZoombiniGraphics::fillArea(ScreenKind screenKind, ZmbDrawRecord *record, uint32 color) {
	fillArea(getScreen(screenKind), record, color);
}

void ZoombiniGraphics::fillArea(ScreenKind screenKind, ZmbResource imgResource, const ZmbHotspot *hotspot, uint32 color) {
	fillArea(getScreen(screenKind), imgResource, hotspot, color);
}

void ZoombiniGraphics::fillArea(ScreenKind screenKind, const Common::Rect &rect, uint32 color) {
	fillArea(getScreen(screenKind), rect, color);
}

void ZoombiniGraphics::fillArea(ScreenKind screenKind, uint32 color) {
	fillArea(getScreen(screenKind), color);
}

void ZoombiniGraphics::fillArea(Graphics::Surface *screen, ZmbDrawRecord *record, uint32 color) {
	screen->fillRect(record->_drawnRect, color);
}

void ZoombiniGraphics::fillArea(Graphics::Surface *screen, ZmbResource imgResource, const ZmbHotspot *hotspot, uint32 color) {
	MohawkSurface *rawSurface = findShape(imgResource, hotspot->getSubImageId());
	Graphics::Surface *surface = rawSurface->getSurface();

	Common::Rect srcRect(0, 0, surface->w, surface->h);
	screen->fillRect(srcRect, color);
}

void ZoombiniGraphics::fillArea(Graphics::Surface *screen, const Common::Rect &rect, uint32 color) {
	screen->fillRect(rect, color);
}

void ZoombiniGraphics::fillArea(Graphics::Surface *screen, uint32 color) {
	screen->fillRect(screen->getRect(), color);
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

			drawTextLines(screenKind, font, lines, outlineRect, tc._outlinePalette, tc._hAlign, fillBackgroundPalette);
		}
	}

	drawTextLines(screenKind, font, lines, drawRect, tc._textPalette, tc._hAlign, fillBackgroundPalette);
	_isScreenDirty = true;
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
	// Calculate total draw height and max draw width
	int16 drawTotalHeight = 0; // Height of will-be-drawn area
	Common::Point boundSize;
	for (const Common::U32String &line : lines) {
		// Check bounding box for each line
		Common::Rect bbox = font->getBoundingBox(line, 0, 0, _screenRect.width(), Graphics::kTextAlignLeft);
		if (outlineEffect) { // When outlineEffect is set, 2 more x/y pixels are drawen.
			bbox.right += 2;
			bbox.bottom += 2;
		}

		// Expand drawRect (virtualize GDI DT_NOCLIP)
		drawTotalHeight += bbox.height();
		boundSize.x = MAX(boundSize.x, bbox.width());	
		boundSize.y = MAX(boundSize.y, drawTotalHeight);
	}
	return boundSize;

#if 0
	// Calculate total draw height and max draw width
	int16 drawTotalHeight = 0; // Height of will-be-drawn area
	for (const Common::U32String &line : lines) {
		// Check bounding box for each line
		Common::Rect bbox = font->getBoundingBox(line, 0, 0, _screenRect.width(), Graphics::kTextAlignLeft);
		if (outlineEffect) { // When outlineEffect is set, 2 more x/y pixels are drawen.
			bbox.right += 2;
			bbox.bottom += 2;
		}

		// Expand drawRect (virtualize GDI DT_NOCLIP)
		drawTotalHeight += bbox.height();
		drawRect.setWidth(MAX(drawRect.width(), bbox.width()));	
		drawRect.setHeight(MAX(drawRect.height(), drawTotalHeight));
	}
	return drawTotalHeight;
#endif
}

void ZoombiniGraphics::drawTextLines(ScreenKind screenKind, const Graphics::Font *font, const Common::Array<Common::U32String> &lines, const Common::Rect &destRect, uint32 palette, Graphics::TextAlign hAlign, uint32 fillBackgroundColor) {
	Common::Rect drawRect = destRect;
	for (uint32 i = 0; i < lines.size(); i++) {
		const Common::U32String &line = lines[i];

		// Background is for debug purposes, Zoombini game itself does not use this feature
		if (fillBackgroundColor != kTransparentKey) {
			const Common::Rect &bbox = font->getBoundingBox(line, drawRect.left, drawRect.top, drawRect.width(), hAlign);
			_vm->_gfx->getScreen(screenKind)->fillRect(bbox, fillBackgroundColor);
		}

		// Draw the text line by line
		font->drawString(_vm->_gfx->getScreen(screenKind), line, drawRect.left, drawRect.top, drawRect.width(), palette, hAlign);

		if (i + 1 < lines.size()) {
			const Common::Rect &bbox = font->getBoundingBox(line, 0, 0, _screenRect.width(), Graphics::kTextAlignLeft);
			drawRect.top += bbox.height();
			drawRect.bottom += bbox.height();
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
	assert(_paletteColorCount <= 255);

	uint16 bufSize = _paletteColorCount * 3;
	byte *fadePalette = new byte[bufSize];
	memset(fadePalette, 0, sizeof(bufSize));

	for (uint16 i = 0; i < bufSize; i++) {
		fadePalette[i] = static_cast<byte>(static_cast<uint32>(_palette[i]) * idx / steps);
	}

	_vm->_system->getPaletteManager()->setPalette(fadePalette, _paletteColorStart, _paletteColorCount);
	delete[] fadePalette;

	_vm->_system->updateScreen();
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
	if (!readPalette(id, _palette, ARRAYSIZE(_palette), _paletteColorStart, _paletteColorCount)) {
		error("Could not read palette from SHPL p:%04u", id);
		return;
	}

	_vm->_system->getPaletteManager()->setPalette(_palette + _paletteColorStart * 3, _paletteColorStart, _paletteColorCount);
}

bool ZoombiniGraphics::readPalette(uint16 id, byte *destBuf, size_t destBufSize, uint16 &paletteColorStart, uint16 &paletteColorCount) {
	if (!destBuf || destBufSize == 0)
		return false;

	memset(destBuf, 0, destBufSize);

	Common::SeekableReadStream *shplStream = _vm->getResource(ID_SHPL, ZmbResource(ZmbArchiveKind::kPage, id));
	uint16 shplId = shplStream->readUint16BE();
	assert(shplId == id);
	shplStream->readUint16BE();                     // Always 00 01
	paletteColorStart = shplStream->readUint16BE();
	paletteColorCount = shplStream->readUint16BE();
	assert(paletteColorStart <= 255);
	assert(paletteColorStart + paletteColorCount <= 256);

	// Is size of the buffer enough?
	if (destBufSize < 3 * paletteColorCount)
		return false;

	for (uint16 i = 0; i < paletteColorCount; i++) {
		destBuf[i * 3 + 0] = shplStream->readByte();
		destBuf[i * 3 + 1] = shplStream->readByte();
		destBuf[i * 3 + 2] = shplStream->readByte();
		shplStream->readByte();  // Skip alpha/flags byte
	}

	delete shplStream;

	return true;
}

void ZoombiniGraphics::clearPalette() {
	// Set the palette to all black
	memset(_palette, 0, sizeof(_palette));
	_vm->_system->getPaletteManager()->setPalette(_palette, _paletteColorStart, _paletteColorCount);
}

void ZoombiniGraphics::loadSystemPalette() {
	if (_systemPaletteLoaded)
		return;

	// Load the palette from tBMP 3000 (main snoid shapes) in ZOOMBINI.MHK.
	// This contains the zoombini sprite colors and UI colors used across all pages.
	ZmbResource snoidShapesRes(ZmbArchiveKind::kSystem, 3000);
	if (!_vm->hasResource(ID_TBMP, snoidShapesRes)) {
		warning("ZoombiniGraphics::loadSystemPalette: tBMP 3000 not found in system archive");
		return;
	}

	Common::SeekableReadStream *stream = _vm->getResource(ID_TBMP, snoidShapesRes);
	MohawkSurface *surface = _bmpDecoder->decodeImage(stream);
	if (surface && surface->getPalette()) {
		memcpy(_systemPalette, surface->getPalette(), sizeof(_systemPalette));
		_systemPaletteLoaded = true;
		debugC(kZmbDebugRender, "Loaded system palette from tBMP 3000");
	} else {
		warning("ZoombiniGraphics::loadSystemPalette: Failed to get palette from tBMP 3000");
		memset(_systemPalette, 0, sizeof(_systemPalette));
	}
	delete surface;
}

void ZoombiniGraphics::mergeSystemPalette() {
	if (!_systemPaletteLoaded) {
		loadSystemPalette();
		if (!_systemPaletteLoaded)
			return;
	}

	// The page SHPL covers screen palette indices [_paletteColorStart, _paletteColorStart + _paletteColorCount).
	// We need to fill in system palette colors for indices OUTSIDE this range.
	// IDA: SHPL_copyPaletteSrcToDst(236, 10) merges indices 10-245.
	//
	// For XFER: SHPL 1000 covers indices 46-243, so we apply system palette:
	//   - Indices 10-45: zoombini sprite colors + UI (text fg/bg)
	//   - Indices 244-245: additional system colors if needed

	uint16 shplStart = _paletteColorStart;
	uint16 shplEnd = _paletteColorStart + _paletteColorCount;

	// Apply system palette for indices 10 to shplStart-1 (before SHPL range)
	if (shplStart > 10) {
		uint16 count = shplStart - 10;
		_vm->_system->getPaletteManager()->setPalette(_systemPalette + 10 * 3, 10, count);
	}

	// Apply system palette for indices shplEnd to 245 (after SHPL range)
	if (shplEnd < 246) {
		uint16 count = 246 - shplEnd;
		_vm->_system->getPaletteManager()->setPalette(_systemPalette + shplEnd * 3, shplEnd, count);
	}
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
