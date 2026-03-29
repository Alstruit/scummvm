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

#ifndef MOHAWK_ZOOMBINI_PAGES_DIALOG_HELP_H
#define MOHAWK_ZOOMBINI_PAGES_DIALOG_HELP_H

#include "mohawk/zoombini_pages/dialog_base.h"

namespace Mohawk {

class ZoombiniDialogHelp : public ZoombiniDialog {
public:
	ZoombiniDialogHelp(MohawkEngine_Zoombini *vm, ZoombiniPageType forPage);
	~ZoombiniDialogHelp() override;

	void loadFeatures() override;
	
protected:
	ZoombiniPageType _forPageType;
	Common::Array<Common::U32String> _pageHelpBodyStrs;
	uint32 _pageHelpBodyIdx = 0;

	void helpDialog_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void helpDialog_onPostRender(ZmbFeature *feature);
	void helpDialog_onPostAnimation(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs);
	Common::Rect helpDialog_getButtonTextRect(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs, const Common::Rect &buttonRect);
	ZmbEventHandleResult helpDialog_onMouseLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	Common::Rect helpDialog_getButtonClickRect(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs, const Common::Rect &buttonRect);
	ZmbEventHandleResult helpDialog_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);

	// MapRect & MapSave data
	enum HelpDialogButtonIdx : uint32 {
		kHelpDialogButton_Prev = 0,
		kHelpDialogButton_Next = 1,
		kHelpDialogButton_Okay = 2,
	};

	Common::StableMap<uint32, ButtonState> _helpDialogButtonStateMap;
	const Common::Rect _helpDialogTitleRect = Common::Rect(0x104, 0x003E, 0x0174, 0x005B);
	Common::Rect _helpDialogButtonRects[3] = {
		Common::Rect(0x009A, 0x013F, 0x00F5, 0x0162),
		Common::Rect(0x0108, 0x013F, 0x0165, 0x0162),
		Common::Rect(0x01BB, 0x013F, 0x01E2, 0x0162),
	};
	Common::Rect _helpDialogTextRects[2] = {
		Common::Rect(0x00AD, 0x013F, 0x00F0, 0x0162),
		Common::Rect(0x010F, 0x013F, 0x0152, 0x0162),
	};
	const Common::Rect _helpDialogHeadRect = Common::Rect(0x00AA, 0x0063, 0x01DB, 0x0074);
	const Common::Rect _helpDialogBodyRect = Common::Rect(0x00AA, 0x0079, 0x01DB, 0x0127);
};

} // End of namespace Mohawk

#endif
