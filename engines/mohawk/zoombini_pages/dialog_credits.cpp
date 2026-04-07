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
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"
#include "mohawk/zoombini_pages/dialog_credits.h"
#include "dialog_credits.h"

namespace Mohawk {

ZoombiniDialogCredits::ZoombiniDialogCredits(MohawkEngine_Zoombini *vm) : ZoombiniDialog(vm, ZoombiniPageType::kCreditScreen) {
}

ZoombiniDialogCredits::~ZoombiniDialogCredits() {
	_vm->_midi->pause(false);
	_vm->_sound->stopSound(kResSound20104_TownBGM);
}

void ZoombiniDialogCredits::open() {
	_vm->_midi->pause(true);
}

void ZoombiniDialogCredits::loadFeatures() {
	_pageStartFrameTime = _vm->_system->getMillis();
	_pageStartFrameCounter = _pageStartFrameTime / MohawkEngine_Zoombini::kAnimateFrameTimeMs;

	_vm->_text->getLocalizedCredits(_creditParagraphs);
	_totalCreditLines = 0;
	for (const ZoombiniText::CreditParagraph &paragraph : _creditParagraphs)
		_totalCreditLines += paragraph.getTotalLineCount();

	// Load SCRBs
	ZmbFeature::EventHooks hooksBackground;
	hooksBackground.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniDialogCredits::creditScreen_onMouseLButtonDown));
	hooksBackground.setKeyDownFunc(reinterpret_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogCredits::creditScreen_onKeyDown));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap0020_Credits), kResScrb0020_DialogCredits, 0,
					ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
					hooksBackground);

	// Load Virtual Feature - drawLines
	ZmbFeature::EventHooks hooksDrawLines;
	hooksDrawLines.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniDialogCredits::drawLines_render));
loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, 0), 0, 0,
					ZmbFeature::FLAG_00001000_TOPMOST,
					hooksDrawLines);

	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, kResSound20104_TownBGM), Audio::Mixer::SoundType::kMusicSoundType, true);
}


ZmbEventHandleResult ZoombiniDialogCredits::creditScreen_onMouseLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	close();
	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniDialogCredits::creditScreen_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	close();
	return ZmbEventHandleResult::kConsumed;
}

ZmbRenderResult ZoombiniDialogCredits::drawLines_render(ZmbFeature *feature) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	_vm->_gfx->fillArea(screenKind, kColorCreditsBackground);

	// Original engine draws one credit line per every 16 frames.
	// - lineIdx = frameCounter / 16
	// In each frame, the text pixel rects are scrolled up by 1 pixel to create scrolling effect.
	// - (scroll) y: 16 ~ 467 to 15 ~ 466, (text) y: 451 ~ 466
	// That implementation lags behind if the frame rate is lower than 60.

	// So instead, we determines which/how many lines to draw based on elapsed frames.
	// The text area patterns at frames divided by 16 should behave like:
	// - y: 451 - 466 (line N), 451-16 ~ 466-16 (line N-1), 451-32 ~ 466-32 (line N-2), ...

	// Find the lines to be drawn
	uint32 elapsedFrames = _currentFrameCounter - _pageStartFrameCounter;
	int32 rawStartLineIdx = MAX(0, drawLines_getStartLineIdx(elapsedFrames));
	int32 rawEndLineIdx = drawLines_getEndLineIdx(elapsedFrames);

	int32 baseLineIdx = (rawStartLineIdx / _totalCreditLines) * _totalCreditLines;
	int32 startLineIdx = rawStartLineIdx % _totalCreditLines;
	int32 endLineIdx = MIN<int32>(rawEndLineIdx, _totalCreditLines);

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

				bool isLineTitle = (li == 0);
				ZoombiniGraphics::TextConf tc;
				tc._fontUsage = ZoombiniFontUsage::kFontText;
				tc._hAlign = Graphics::kTextAlignCenter;
				tc._vAlign = Graphics::kTextAlignCenter;
				if (isLineTitle) {
					tc._textPalette = kColorCreditsTitle;
				} else {
					tc._textPalette = kColorCreditsLine;
				}

				Common::Rect drawRect = _textRect;
				drawRect.top = static_cast<int16>(drawLines_getLinePosY(elapsedFrames, baseLineIdx + lineIdx));
				drawRect.bottom = static_cast<int16>(drawRect.top + 16);
				_vm->_gfx->drawText(screenKind, paragraph._lines[li], drawRect, tc);
			}

			// Blank lines
			lineIdx += paragraph._blankLineCount;
		}

		// End reached, loop to the beginning of the credits
		if (endLineIdx == static_cast<int32>(_totalCreditLines)) {
			baseLineIdx += _totalCreditLines;
			startLineIdx = 0;
			endLineIdx = rawEndLineIdx % _totalCreditLines;
			continue;
		}

		break;
	} while (true);

	return ZmbRenderResult::kRendered;
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
