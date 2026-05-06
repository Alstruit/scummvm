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

#ifndef MOHAWK_ZOOMBINI_PAGES_DIALOG_MSGBOX_H
#define MOHAWK_ZOOMBINI_PAGES_DIALOG_MSGBOX_H

#include "mohawk/zoombini_pages/dialog_base.h"

namespace Mohawk {

class ZoombiniDialogMsgBox : public ZoombiniDialog {
public:
	ZoombiniDialogMsgBox(MohawkEngine_Zoombini *vm, ZoombiniMsgBoxType type);
	ZoombiniDialogMsgBox(MohawkEngine_Zoombini *vm, const Common::U32String &message);
	~ZoombiniDialogMsgBox() override;

	void loadFeatures() override;

	void longButtons_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void longButtons_onPostRender(ZmbFeature *feature);
	void longButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs);
	ZmbEventHandleResult longButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	ZmbEventHandleResult longButtons_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);
	
private:
	// MapRect & MapSave data
	enum MsgBoxDialogButtonIdx : uint32 {
		kMsgBoxDialogButton_Yes = 0,
		kMsgBoxDialogButton_No = 1,
	};

	Common::StableMap<uint32, ButtonState> _longButtonStateMap;
	void initButtons(ZoombiniText::Key yesKey, ZoombiniText::Key noKey);

	const Common::Rect _msgBoxDialogBodyRect = Common::Rect(0x009A, 0x0091, 0x01E7, 0x00DC);
	const Common::Rect _msgBoxDialogYesRect = Common::Rect(0x0000, 0x0000, 0x0000, 0x0000);
	const Common::Rect _msgBoxDialogNoRect = Common::Rect(0x0000, 0x0000, 0x0000, 0x0000);

	ZoombiniMsgBoxType _type;
	ZoombiniText::Key _msgKey;
	Common::U32String _msgText;
};

} // End of namespace Mohawk

#endif
