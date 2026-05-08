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

#ifndef MOHAWK_ZOOMBINI_PAGES_DIALOG_SAVELOAD_H
#define MOHAWK_ZOOMBINI_PAGES_DIALOG_SAVELOAD_H

#include "mohawk/zoombini_pages/dialog_base.h"

namespace Mohawk {

class ZoombiniDialogSaveLoad : public ZoombiniDialog {
public:
	enum SaveLoadMode {
		kSaveMode,
		kLoadMode,
		kLoadOrNewMode,
	};

	ZoombiniDialogSaveLoad(MohawkEngine_Zoombini *vm, SaveLoadMode mode);
	~ZoombiniDialogSaveLoad() override;

	bool isSaveDialog() const { return _mode == kSaveMode; }
	bool isLoadDialog() const { return _mode != kSaveMode; }

	void loadFeatures() override;

	Common::Rect getSaveEntryBaseRect();

	void dialogFrame_onPostRender(ZmbFeature *feature);
	ZmbEventHandleResult dialogFrame_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	ZmbEventHandleResult dialogFrame_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);
	ZmbEventHandleResult saveTextBox_handleTyping(const Common::KeyState &kbd);

	void scrollButtons_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void scrollButtons_onPostRender(ZmbFeature *feature);
	void scrollButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs);
	ZmbEventHandleResult scrollButtons_onWheelUp(ZmbFeature *feature, const Common::Point &absPos);
	ZmbEventHandleResult scrollButtons_onWheelDown(ZmbFeature *feature, const Common::Point &absPos);
	ZmbEventHandleResult scrollButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	ZmbEventHandleResult scrollButtons_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);

	void longButtons_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void longButtons_onPostRender(ZmbFeature *feature);
	void longButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs);
	ZmbEventHandleResult longButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	ZmbEventHandleResult longButtons_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);
	
private:
	void clampLoadSelection();

	SaveLoadMode _mode;

	Common::Rect _titleRect;
	ZoombiniText::Key _titleKey;
	int32 _saveEntryBaseIdx = 0;
	int32 _saveEntrySelectedIdx = 0;
	uint32 _lastSaveEntryClickedFrame = 0;

	// MapRect & MapSave data
	enum SaveLoadDialogButtonIdx : uint32 {
		kSaveLoadDialogButton_ScrollUp = 0,
		kSaveLoadDialogButton_ScrollDown = 1,
		kSaveLoadDialogButton_Okay = 2,
		kSaveLoadDialogButton_Cancel = 3,
	};

	static constexpr int32 SAVESLOTS_PER_SCREEN = 8;

	Common::StableMap<uint32, ButtonState> _scrollButtonStateMap;
	Common::StableMap<uint32, ButtonState> _longButtonStateMap;

	const Common::Rect _saveAsCaptionRect = Common::Rect(0x00BB, 0x010F, 0x017C, 0x0122);
	const Common::Rect _saveTextBoxRect = Common::Rect(0x00C0, 0x012A, 0x01C8, 0x013C);
	static constexpr uint16 _saveEntryLeft = 192;
	static constexpr uint16 _saveEntryRight = 405;
	static constexpr uint16 _saveEntrySaveModeTop = 94;

	// Text input for SaveDialog
	Common::U32String _saveInputText;
	uint32 _saveInputCursorPos = 0;
	uint32 _saveInputCursorLastBlinkTimeMs = 0;
	bool _saveInputCursorVisible = true;
	static constexpr uint32 MAX_SAVENAME_BYTES = 22;
	static constexpr uint16 _saveEntryLoadModeTop = 110;
	static constexpr uint16 _saveEntryHeight = 20;
};

} // End of namespace Mohawk

#endif
