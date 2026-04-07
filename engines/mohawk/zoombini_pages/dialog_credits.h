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

#ifndef MOHAWK_ZOOMBINI_PAGES_DIALOG_CREDITS_H
#define MOHAWK_ZOOMBINI_PAGES_DIALOG_CREDITS_H

#include "mohawk/zoombini_pages/dialog_base.h"

namespace Mohawk {

class ZoombiniDialogCredits : public ZoombiniDialog {
public:
	ZoombiniDialogCredits(MohawkEngine_Zoombini *vm);
	~ZoombiniDialogCredits() override;

	void open() override;
	void loadFeatures() override;


protected:
	ZmbEventHandleResult creditScreen_onMouseLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	ZmbEventHandleResult creditScreen_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);

	ZmbRenderResult drawLines_render(ZmbFeature *feature);
	/**
	 * Get the Y position of a line in a given frame
	 */
	int32 drawLines_getLinePosY(uint32 elapsedFrames, uint32 lineIdx);
	/**
	 * In a given frame, find the smallest lineIdx where posY >= 0
	 */
	int32 drawLines_getStartLineIdx(uint32 elapsedFrames);
	/**
	 * In a given frame, find the largest lineIdx where posY <= 480
	 */
	int32 drawLines_getEndLineIdx(uint32 elapsedFrames);

	enum TextColors {
		kColorCreditsBackground = 0x2D,
		kColorCreditsTitle = 0x26,
		kColorCreditsLine = 0x23,
	};

	// MapRect
	const Common::Rect _textRect = Common::Rect(0x00BE, 0x01C3, 0x01C2, 0x01D2);
	const Common::Rect _scrollDestRect = Common::Rect(0x00BE, 0x0010, 0x01C2, 0x01D3);
	const Common::Rect _scrollSrcRect = Common::Rect(0x00BE, 0x000F, 0x01C2, 0x01D2);
	const Common::Rect _blitRect = Common::Rect(0x00BE, 0x000F, 0x01C2, 0x01D2);

	Common::Array<ZoombiniText::CreditParagraph> _creditParagraphs;
	uint32 _totalCreditLines = 0;
};

} // End of namespace Mohawk

#endif
