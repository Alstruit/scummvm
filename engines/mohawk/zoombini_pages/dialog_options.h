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

#ifndef MOHAWK_ZOOMBINI_PAGES_DIALOG_OPTION_H
#define MOHAWK_ZOOMBINI_PAGES_DIALOG_OPTION_H

#include "mohawk/zoombini_pages/dialog_base.h"

namespace Mohawk {

class ZoombiniDialogOptions : public ZoombiniDialog {
public:
	ZoombiniDialogOptions(MohawkEngine_Zoombini *vm);
	~ZoombiniDialogOptions() override;

	void loadFeatures() override;

protected:
	bool getOptionDialogToggleValue(uint16 bsIdx);

	void redToggleButtons_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void redToggleButtons_onPostRender(ZmbFeature *feature);
	Common::Rect redButtons_textRect(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs, const Common::Rect &drawnRect);
	Common::Rect toggleButtons_textRect(ZmbFeature *feature, uint32 bsIdx, ToggleButtonState &bs, const Common::Rect &drawnRect);
	void redButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs);
	void toggleButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ToggleButtonState &bs);
	ZmbEventHandleResult redToggleButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);

	void longButtons_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void longButtons_onPostRender(ZmbFeature *feature);
	void longButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs);
	ZmbEventHandleResult longButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	ZmbEventHandleResult longButtons_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);
	
	// MapRect & MapSave data
	enum OptionDialogButtonIdx : uint32 {
		kOptionDialogButton_NewGame = 0,
		kOptionDialogButton_LoadGame = 1,
		kOptionDialogButton_SaveGame = 2,
		kOptionDialogButton_Quit = 3,
		kOptionDialogButton_Sound = 4,
		kOptionDialogButton_Music = 5,
		kOptionDialogButton_StickyMouse = 6,
		kOptionDialogButton_Transitions = 7,
		kOptionDialogButton_Okay = 8,
		kOptionDialogButton_Credits = 9,
	};

	Common::StableMap<uint32, ButtonState> _redButtonStateMap;
	Common::StableMap<uint32, ToggleButtonState> _toggleButtonStateMap;
	Common::StableMap<uint32, ButtonState> _longButtonStateMap;

	const Common::Rect _optionDialogTitleRect = Common::Rect(0x00FA, 0x003A, 0x017C, 0x0050);
	const Common::Rect _optionDialogToggleRect = Common::Rect(0x00A6, 0x00BE, 0x01D1, 0x00D2);
	const Common::Rect _optionDialogLegendOnRect = Common::Rect(0x00C5, 0x0140, 0x0104, 0x0159);
	const Common::Rect _optionDialogLegendOffRect = Common::Rect(0x00C5, 0x0159, 0x0104, 0x016D);
	const uint16 _optionDialogTextLeft = 0x00C4;
	const uint16 _optionDialogTextRight = 0x01D6;
	const uint16 _optionDialogTextTops[8] = {0x0066, 0x007C, 0x0091, 0x00A6, 0x00DD, 0x00F3, 0x0108, 0x011D};
};

} // End of namespace Mohawk

#endif
