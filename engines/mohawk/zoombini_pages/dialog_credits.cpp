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

#include "mohawk/console.h"
#include "mohawk/mohawk.h"
#include "mohawk/sound.h"
#include "mohawk/video.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_pages/dialog_credits.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

namespace {

const uint16 kTlcCreditsBinkLogoShape = 8;
const uint16 kTlcCreditsMilesLogoShape = 7;
const uint32 kTlcCreditsPreLogoBlankLineCount = 2;
const uint32 kTlcCreditsLogoLineCount = 4;
const uint32 kTlcCreditsPostLogoBlankLineCount = 15;
const int16 kTlcCreditsBinkLogoX = 203;
const int16 kTlcCreditsMilesLogoX = 349;

} // End of anonymous namespace

ZoombiniDialogCredits::ZoombiniDialogCredits(MohawkEngine_Zoombini *vm) : ZoombiniDialog(vm, ZoombiniPageType::kCreditScreen) {
}

ZoombiniDialogCredits::~ZoombiniDialogCredits() {
	_vm->_midi->pause(false);
	_vm->_sound->stopSound(kResSound20104_TownBGM);
}

void ZoombiniDialogCredits::open() {
	_vm->_midi->pause(true);
}

void ZoombiniDialogCredits::updateCreditScrollElapsedFrames() {
	const uint32 frameDelta = _currentFrameCounter - _lastCreditScrollFrameCounter;
	_lastCreditScrollFrameCounter = _currentFrameCounter;

	_creditScrollElapsedFrames += static_cast<int32>(frameDelta) * _creditScrollFramesPerFrame;
	if (_creditScrollElapsedFrames < 0)
		_creditScrollElapsedFrames = 0;
}

void ZoombiniDialogCredits::drawCreditLine(const Common::U32String &lineText, bool isTitle, int16 topY) {
	ZoombiniGraphics::TextConf tc;
	tc._fontUsage = ZoombiniFontUsage::kFontText;
	tc._hAlign = Graphics::kTextAlignCenter;
	tc._vAlign = Graphics::kTextAlignCenter;
	tc._textPalette = isTitle ? kColorCreditsTitle : kColorCreditsLine;

	Common::Rect drawRect = getCreditLineRect();
	drawRect.top = topY;
	drawRect.bottom = static_cast<int16>(drawRect.top + 16);
	_vm->_gfx->drawText(ZoombiniGraphics::kShapeScreen, lineText, drawRect, tc);
}

Common::Rect ZoombiniDialogCredits::getCreditLineRect() const {
	Common::Rect lineRect = _textRect;
	lineRect.left = 0;
	lineRect.right = ZoombiniGraphics::kScreenWidth;
	return lineRect;
}

uint32 ZoombiniDialogCredits::getCreditScrollLineCount() const {
	if (_vm->isGameVariant(GF_ZMB_TLC))
		return _totalCreditLines + kTlcCreditsPreLogoBlankLineCount +
			kTlcCreditsLogoLineCount + kTlcCreditsPostLogoBlankLineCount;

	return _totalCreditLines;
}

void ZoombiniDialogCredits::drawTlcEndLogos(uint32 elapsedFrames, int32 baseLineIdx, int32 startLineIdx, int32 endLineIdx) {
	if (!_vm->isGameVariant(GF_ZMB_TLC))
		return;

	const int32 logoStartLineIdx = static_cast<int32>(_totalCreditLines + kTlcCreditsPreLogoBlankLineCount + 1);
	const int32 logoEndLineIdx = static_cast<int32>(_totalCreditLines + kTlcCreditsPreLogoBlankLineCount + kTlcCreditsLogoLineCount);
	if (endLineIdx < logoStartLineIdx || logoEndLineIdx < startLineIdx)
		return;

	const int32 logoTopY = drawLines_getLinePosY(elapsedFrames, baseLineIdx + logoStartLineIdx);
	if (ZoombiniGraphics::kScreenHeight <= logoTopY || logoTopY <= -64)
		return;

	const int16 logoY = static_cast<int16>(logoTopY);
	const ZmbResource creditsBitmap(ZmbArchiveKind::kSystem, kResShapeBitmap0020_Credits);
	_vm->_gfx->drawShape(ZoombiniGraphics::kShapeScreen, creditsBitmap, kTlcCreditsBinkLogoShape,
		Common::Point(kTlcCreditsBinkLogoX, logoY), false);
	_vm->_gfx->drawShape(ZoombiniGraphics::kShapeScreen, creditsBitmap, kTlcCreditsMilesLogoShape,
		Common::Point(kTlcCreditsMilesLogoX, logoY), false);
}

void ZoombiniDialogCredits::loadFeatures() {
	_pageStartFrameTime = _vm->_system->getMillis();
	_pageStartFrameCounter = _pageStartFrameTime / MohawkEngine_Zoombini::kAnimateFrameTimeMs;
	_lastCreditScrollFrameCounter = _pageStartFrameCounter;
	_creditScrollElapsedFrames = 0;
	_creditScrollFramesPerFrame = 1;

	_vm->_text->getLocalizedCredits(_creditParagraphs);
	_totalCreditLines = 0;
	for (const ZoombiniText::CreditParagraph &paragraph : _creditParagraphs)
		_totalCreditLines += paragraph.getTotalLineCount();

	// Keep ScummVM's elapsed-frame scroll math instead of the original fixed-tick gate,
	// but render it from the original single credits runner so the frame shapes stay on top.
	ZmbFeature::EventHooks hooksBackground;
	hooksBackground.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniDialogCredits::creditScreen_render));
	hooksBackground.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniDialogCredits::creditScreen_onMouseLButtonDown));
	hooksBackground.setKeyDownFunc(reinterpret_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogCredits::creditScreen_onKeyDown));
	hooksBackground.setKeyUpFunc(reinterpret_cast<ZmbFeature::OnKeyUpFunc>(&ZoombiniDialogCredits::creditScreen_onKeyUp));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap0020_Credits), kResScrb0020_DialogCredits, 0,
					ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
					hooksBackground);

	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, kResSound20104_TownBGM), Audio::Mixer::SoundType::kMusicSoundType, true);
}

ZmbEventHandleResult ZoombiniDialogCredits::creditScreen_onMouseLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	close();
	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniDialogCredits::creditScreen_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	// ScummVM only debug controls: fast forward with `]`, rewind with `[`
	switch (kbd.keycode) {
	case Common::KEYCODE_RIGHTBRACKET:
		_creditScrollFramesPerFrame = 15;
		return ZmbEventHandleResult::kConsumed;
	case Common::KEYCODE_LEFTBRACKET:
		_creditScrollFramesPerFrame = -5;
		return ZmbEventHandleResult::kConsumed;
	default:
		break;
	}

	close();
	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniDialogCredits::creditScreen_onKeyUp(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	if (kbd.keycode == Common::KEYCODE_RIGHTBRACKET || kbd.keycode == Common::KEYCODE_LEFTBRACKET) {
		_creditScrollFramesPerFrame = 1;
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbRenderResult ZoombiniDialogCredits::creditScreen_render(ZmbFeature *feature) {
	// The credits runner owns the black backdrop; limiting this to _blitRect leaks
	// captured page pixels in the non-scroll area behind the frame art.
	_vm->_gfx->fillArea(ZoombiniGraphics::kShapeScreen, kColorCreditsBackground);
	updateCreditScrollElapsedFrames();

	if (_totalCreditLines == 0)
		return blitShapes(feature);

	const uint32 scrollLineCount = getCreditScrollLineCount();

	// Original engine draws one credit line per every 16 frames.
	// - lineIdx = frameCounter / 16
	// In each frame, the text pixel rects are scrolled up by 1 pixel to create scrolling effect.
	// - (scroll) y: 16 ~ 467 to 15 ~ 466, (text) y: 451 ~ 466
	// That implementation lags behind if the frame rate is lower than 60.

	// So instead, we determines which/how many lines to draw based on elapsed frames.
	// The text area patterns at frames divided by 16 should behave like:
	// - y: 451 - 466 (line N), 451-16 ~ 466-16 (line N-1), 451-32 ~ 466-32 (line N-2), ...

	// Find the lines to be drawn
	uint32 elapsedFrames = static_cast<uint32>(_creditScrollElapsedFrames);
	int32 rawStartLineIdx = MAX(0, drawLines_getStartLineIdx(elapsedFrames));
	int32 rawEndLineIdx = drawLines_getEndLineIdx(elapsedFrames);

	int32 baseLineIdx = (rawStartLineIdx / scrollLineCount) * scrollLineCount;
	int32 startLineIdx = rawStartLineIdx % scrollLineCount;
	int32 endLineIdx = MIN<int32>(rawEndLineIdx, scrollLineCount);

	// Find the paragraphs to be drawn
	do {
		int32 lineIdx = 0;
		for (const ZoombiniText::CreditParagraph &paragraph : _creditParagraphs) {
			if (static_cast<int32>(lineIdx + paragraph.getTotalLineCount()) <= startLineIdx) {
				lineIdx += paragraph.getTotalLineCount();
				continue;
			}

			// Text lines
			for (uint32 li = 0; li < paragraph._lines.size() && lineIdx <= endLineIdx; li++) {
				lineIdx += 1;

				if (lineIdx < startLineIdx)
					continue;

				assert(startLineIdx <= lineIdx || lineIdx <= endLineIdx);

				drawCreditLine(paragraph._lines[li], li == 0,
					static_cast<int16>(drawLines_getLinePosY(elapsedFrames, baseLineIdx + lineIdx)));
			}

			// Blank lines
			lineIdx += paragraph._blankLineCount;
		}

		drawTlcEndLogos(elapsedFrames, baseLineIdx, startLineIdx, endLineIdx);

		// End reached, loop to the beginning of the credits
		if (endLineIdx == static_cast<int32>(scrollLineCount)) {
			baseLineIdx += scrollLineCount;
			startLineIdx = 0;
			endLineIdx = rawEndLineIdx % scrollLineCount;
			continue;
		}

		break;
	} while (true);

	return blitShapes(feature);
}

int32 ZoombiniDialogCredits::drawLines_getLinePosY(uint32 elapsedFrames, uint32 lineIdx) {
	return static_cast<int32>(_textRect.top) - elapsedFrames + 16 * lineIdx;
}

int32 ZoombiniDialogCredits::drawLines_getStartLineIdx(uint32 elapsedFrames) {
	return (static_cast<int32>(elapsedFrames) - _textRect.top) / 16;
}

int32 ZoombiniDialogCredits::drawLines_getEndLineIdx(uint32 elapsedFrames) {
	return (ZoombiniGraphics::kScreenHeight + static_cast<int32>(elapsedFrames) - _textRect.top) / 16;
}

} // End of namespace Mohawk
