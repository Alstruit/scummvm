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
#include "mohawk/zoombini_pages/dialog_saveload.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

ZoombiniDialogSaveLoad::ZoombiniDialogSaveLoad(MohawkEngine_Zoombini *vm, SaveLoadMode mode) : ZoombiniDialog(vm, ZoombiniPageType::kDialogSaveLoad), _mode(mode) {
	ZoombiniText::Key yesKey = ZoombiniText::kNone;
	ZoombiniText::Key noKey = ZoombiniText::kDialogButtonCancel;

	switch (_mode) {
	case kSaveMode:
		_titleKey = ZoombiniText::kDialogTitleSave;
		_titleRect = Common::Rect(0x00FA, 0x0039, 0x0198, 0x0055);
		yesKey = ZoombiniText::kDialogButtonSave;

		_saveInputText = _vm->_state->getActiveSaveName();
		_saveInputCursorPos = _saveInputText.size();
		break;
	case kLoadOrNewMode:
		noKey = ZoombiniText::kNewGame;
		// fall through
	case kLoadMode:
		_titleKey = ZoombiniText::kDialogTitleLoad;
		_titleRect = Common::Rect(0x00FA, 0x0043, 0x0198, 0x005F);
		yesKey = ZoombiniText::kDialogButtonLoad;
		break;
	default:
		error("Invalid ZoombiniMsgBoxType: %u", static_cast<uint32>(_mode));
		break;
	}

	ZmbResource soundResId = ZmbResource(ZmbArchiveKind::kSystem, kResSound0999_ButtonSFX);
	_scrollButtonStateMap[kSaveLoadDialogButton_ScrollUp] = ButtonState(soundResId, 0, 2, kShape0001_16_SaveLoadScrollUpButtonNormal, kShape0001_17_SaveLoadScrollUpButtonPressed);
	_scrollButtonStateMap[kSaveLoadDialogButton_ScrollDown] = ButtonState(soundResId, 1, 3, kShape0001_18_SaveLoadScrollDownButtonNormal, kShape0001_19_SaveLoadScrollDownButtonPressed);
	_longButtonStateMap[kSaveLoadDialogButton_Okay] = ButtonState(yesKey, soundResId, 0, 2, kShape0001_12_LongGreenButtonNormal, kShape0001_13_LongGreenButtonPressed);
	_longButtonStateMap[kSaveLoadDialogButton_Cancel] = ButtonState(noKey, soundResId, 1, 3, kShape0001_14_LongRedButtonNormal, kShape0001_15_LongRedButtonPressed);
}

ZoombiniDialogSaveLoad::~ZoombiniDialogSaveLoad() {
}

void ZoombiniDialogSaveLoad::loadFeatures() {
	uint16 frameScrb;
	uint16 scrollButtonsScrb;
	uint16 longButtonsScrb;
	if (_mode == kSaveMode) {
		frameScrb = kResScrb0007_DialogSave;
		scrollButtonsScrb = kResScrb0008_DialogSave;
		longButtonsScrb = kResScrb0009_DialogSave;
	} else {
		frameScrb = kResScrb0004_DialogLoad;
		scrollButtonsScrb = kResScrb0005_DialogLoad;
		longButtonsScrb = kResScrb0006_DialogLoad;
	}

	// Load SCRBs
	ZmbFeature::EventHooks hooksDialogFrame;
	hooksDialogFrame.setPostRenderFunc(static_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniDialogSaveLoad::dialogFrame_onPostRender));
	hooksDialogFrame.setLButtonDownFunc(static_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniDialogSaveLoad::dialogFrame_onLButtonDown));
	hooksDialogFrame.setKeyDownFunc(static_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogSaveLoad::dialogFrame_onKeyDown));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap0001_Dialog), frameScrb, 0,
					ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
					hooksDialogFrame);

	for (auto it = _scrollButtonStateMap.begin(); it != _scrollButtonStateMap.end(); it++)
		it->second.reset();
	ZmbFeature::EventHooks hooksScrollButtons;
	hooksScrollButtons.setPreRenderShapeFunc(static_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniDialogSaveLoad::scrollButtons_onPreRenderShape));
	hooksScrollButtons.setPostRenderFunc(static_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniDialogSaveLoad::scrollButtons_onPostRender));
	hooksScrollButtons.setWheelUpFunc(static_cast<ZmbFeature::OnWheelUpFunc>(&ZoombiniDialogSaveLoad::scrollButtons_onWheelUp));
	hooksScrollButtons.setWheelDownFunc(static_cast<ZmbFeature::OnWheelDownFunc>(&ZoombiniDialogSaveLoad::scrollButtons_onWheelDown));
	hooksScrollButtons.setLButtonDownFunc(static_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniDialogSaveLoad::scrollButtons_onLButtonDown));
	hooksScrollButtons.setKeyDownFunc(static_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogSaveLoad::scrollButtons_onKeyDown));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap0001_Dialog), scrollButtonsScrb, 11,
					ZmbFeature::FLAG_04000000_OVERLAY,
					hooksScrollButtons);

	for (auto it = _longButtonStateMap.begin(); it != _longButtonStateMap.end(); it++)
		it->second.reset();
	ZmbFeature::EventHooks hooksLongButtons;
	hooksLongButtons.setPreRenderShapeFunc(static_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniDialogSaveLoad::longButtons_onPreRenderShape));
	hooksLongButtons.setPostRenderFunc(static_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniDialogSaveLoad::longButtons_onPostRender));
	hooksLongButtons.setLButtonDownFunc(static_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniDialogSaveLoad::longButtons_onLButtonDown));
	hooksLongButtons.setKeyDownFunc(static_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogSaveLoad::longButtons_onKeyDown));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap0001_Dialog), longButtonsScrb, 13,
					ZmbFeature::FLAG_04000000_OVERLAY,
					hooksLongButtons);
}

Common::Rect ZoombiniDialogSaveLoad::getSaveEntryBaseRect() {
	Common::Rect saveEntryRect = Common::Rect(_saveEntryLeft, 0, _saveEntryRight, 0);
	if (_mode == kSaveMode) {
		saveEntryRect.top = _saveEntrySaveModeTop;
		saveEntryRect.bottom = _saveEntrySaveModeTop + _saveEntryHeight;
	} else {
		saveEntryRect.top = _saveEntryLoadModeTop;
		saveEntryRect.bottom = _saveEntryLoadModeTop + _saveEntryHeight;
	}
	return saveEntryRect;
}

void ZoombiniDialogSaveLoad::clampLoadSelection() {
	int32 saveCount = _vm->_state->_r._saveCount1;
	if (saveCount <= 0) {
		_saveEntrySelectedIdx = 0;
		_saveEntryBaseIdx = 0;
		return;
	}

	if (_saveEntrySelectedIdx < 0)
		_saveEntrySelectedIdx = 0;
	if (saveCount <= _saveEntrySelectedIdx)
		_saveEntrySelectedIdx = saveCount - 1;

	_saveEntryBaseIdx = MIN<int32>(_saveEntryBaseIdx, MAX<int32>(saveCount - SAVESLOTS_PER_SCREEN, 0));
	_saveEntryBaseIdx = MAX<int32>(_saveEntryBaseIdx, 0);
	if (_saveEntrySelectedIdx < _saveEntryBaseIdx) {
		_saveEntryBaseIdx = _saveEntrySelectedIdx;
	} else if (_saveEntryBaseIdx + SAVESLOTS_PER_SCREEN <= _saveEntrySelectedIdx) {
		_saveEntryBaseIdx = _saveEntrySelectedIdx - (SAVESLOTS_PER_SCREEN - 1);
	}
}

void ZoombiniDialogSaveLoad::dialogFrame_onPostRender(ZmbFeature *feature) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	{ // [Text Render] Dialog Title
		ZoombiniGraphics::TextConf titleConf;
		titleConf._fontUsage = ZoombiniFontUsage::kFontTitle;
		titleConf._hAlign = Graphics::kTextAlignCenter;
		titleConf._vAlign = Graphics::kTextAlignCenter;
		titleConf._outlineEffect = true;
		titleConf._outlinePalette = ZoombiniGraphics::kColor0E_VeryLightGray;
		titleConf._textPalette = ZoombiniGraphics::kColor2D_Black;
		_vm->_gfx->drawText(screenKind, _titleKey, _titleRect, titleConf);
	}

	Common::Rect saveEntryRect = getSaveEntryBaseRect();
	uint32 textPalette = 0;

	if (_mode == kSaveMode) {
		// [Text Render] TextBox Caption
		ZoombiniGraphics::TextConf titleConf;
		titleConf._fontUsage = ZoombiniFontUsage::kFontTitle;
		_vm->_gfx->drawText(screenKind, ZoombiniText::kDialogTitleSaveAs, _saveAsCaptionRect, titleConf);

		// [Text Render] Text input box content
		ZoombiniGraphics::TextConf inputConf;
		inputConf._fontUsage = ZoombiniFontUsage::kFontTitle;
		inputConf._textPalette = ZoombiniGraphics::kColor2D_Black;
		if (!_saveInputText.empty()) {
			_vm->_gfx->drawText(screenKind, _saveInputText, _saveTextBoxRect, inputConf);
		}

		// [Text Render] Blinking Cursor
		uint32 cursorBlinkDelta = _currentFrameTime - _saveInputCursorLastBlinkTimeMs;
		if (MohawkEngine_Zoombini::kTextCursorBlinkFrameTimeMs <= cursorBlinkDelta) {
			_saveInputCursorLastBlinkTimeMs = _currentFrameTime;
			_saveInputCursorVisible = !_saveInputCursorVisible;
		}

		// Draw the cursor
		if (_saveInputCursorVisible) {
			Common::U32String cursorStr = _saveInputText.substr(0, _saveInputCursorPos);

			int16 fontHeight = _vm->_gfx->getFontHeight(inputConf);
			int16 textOffsetY = MAX(0, (_saveTextBoxRect.height() - fontHeight) / 2);
			int16 cursorX = _vm->_gfx->getTextBounds(cursorStr, _saveTextBoxRect.width(), inputConf).x;

			Common::Point cursorTop = Common::Point(_saveTextBoxRect.left + cursorX, _saveTextBoxRect.top + textOffsetY);
			Common::Point cursorBottom = Common::Point(_saveTextBoxRect.left + cursorX, _saveTextBoxRect.bottom - textOffsetY);
			_vm->_gfx->drawLine(screenKind, cursorTop, cursorBottom, ZoombiniGraphics::kColor2D_Black);
		}

		// [Text Render] SaveGame List
		textPalette = ZoombiniGraphics::kColor0D_LightGray;
	} else {
		// [Text Render] LoadGame List
		textPalette = ZoombiniGraphics::kColor2D_Black;
	}

	// [Text Render] SaveEntry List (Up to 8 in one screen)
	for (int32 i = _saveEntryBaseIdx; i < _saveEntryBaseIdx + SAVESLOTS_PER_SCREEN && i < _vm->_state->_r._saveCount1; i++) {
		const ZmbRosterEntry &roster = _vm->_state->_r._entries[i];
		const Common::U32String &saveName = roster.getSaveName(_vm);

		ZoombiniGraphics::TextConf entryConf;
		entryConf._fontUsage = ZoombiniFontUsage::kFontTitle;
		entryConf._textPalette = textPalette;
		if (_mode != kSaveMode && _saveEntrySelectedIdx == i) {
			entryConf._outlineEffect = true;
			entryConf._outlinePalette = ZoombiniGraphics::kColor2D_Black;
			entryConf._textPalette = ZoombiniGraphics::kColor22_LimeGreen;
		}

		_vm->_gfx->drawText(screenKind, saveName, saveEntryRect, entryConf);

		saveEntryRect.top += 20;
		saveEntryRect.bottom += 20;
	}
}

ZmbEventHandleResult ZoombiniDialogSaveLoad::dialogFrame_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	if (_mode == kSaveMode)
		return ZmbEventHandleResult::kPassthrough;

	// Load dialogs only
	Common::Rect saveEntryRect = getSaveEntryBaseRect();
	for (int32 i = _saveEntryBaseIdx; i < _saveEntryBaseIdx + SAVESLOTS_PER_SCREEN && i < _vm->_state->_r._saveCount1; i++) {
		if (saveEntryRect.contains(absPos)) {
			_saveEntrySelectedIdx = i;
			if (_currentFrameCounter - _lastSaveEntryClickedFrame <= MohawkEngine_Zoombini::kDoubleClickFrameRate) { // Double-click
				_longButtonStateMap[kSaveLoadDialogButton_Okay].press(_vm, _currentFrameCounter);
			}
			_lastSaveEntryClickedFrame = _currentFrameCounter;
			return ZmbEventHandleResult::kConsumed;
		}

		saveEntryRect.top += 20;
		saveEntryRect.bottom += 20;
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniDialogSaveLoad::dialogFrame_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;
	if (_mode == kSaveMode) {
		// [SaveDialog] TextBox for SaveDialog
		switch (kbd.keycode) {
		case Common::KEYCODE_BACKSPACE:
			if (0 < _saveInputCursorPos) {
				_saveInputText.deleteChar(_saveInputCursorPos - 1);
				_saveInputCursorPos -= 1;
				result = ZmbEventHandleResult::kConsumed;
			}
			break;
		case Common::KEYCODE_DELETE:
			if (_saveInputCursorPos < _saveInputText.size()) {
				_saveInputText.deleteChar(_saveInputCursorPos);
				result = ZmbEventHandleResult::kConsumed;
			}
			break;
		case Common::KEYCODE_LEFT:
			if (0 < _saveInputCursorPos) {
				_saveInputCursorPos -= 1;
				result = ZmbEventHandleResult::kConsumed;
			}
			break;
		case Common::KEYCODE_RIGHT:
			if (_saveInputCursorPos < _saveInputText.size()) {
				_saveInputCursorPos += 1;
				result = ZmbEventHandleResult::kConsumed;
			}
			break;
		case Common::KEYCODE_HOME:
			_saveInputCursorPos = 0;
			result = ZmbEventHandleResult::kConsumed;
			break;
		case Common::KEYCODE_END:
			_saveInputCursorPos = _saveInputText.size();
			result = ZmbEventHandleResult::kConsumed;
			break;
		default: // Handle typing of printable characters
			// When Windows IME is compositing characters, no keyboard event is produced.
			// When Windows IME has finished compositing characters, keyboard event with KEYCODE_INVALID is produced.
			// Common::KEYCODE_DELETE passes this check, but it has been handled above
			if ((Common::KEYCODE_EXCLAIM <= kbd.keycode && kbd.keycode <= Common::KEYCODE_TILDE) || 
				kbd.keycode == Common::KEYCODE_INVALID ||
				(Common::KEYCODE_KP0 <= kbd.keycode && kbd.keycode <= Common::KEYCODE_KP9 && (kbd.flags & Common::KBD_NUM)) ||
				(Common::KEYCODE_KP_PERIOD <= kbd.keycode && kbd.keycode <= Common::KEYCODE_KP_EQUALS && kbd.keycode != Common::KEYCODE_KP_ENTER)) {
				result = saveTextBox_handleTyping(kbd);
			}
			break;
		}
	} else {
		// [LoadDialog]
		// - Delete opens the original remove-game confirmation overlay.
		// - Left/Right key selection is ScummVM-only.
		switch (kbd.keycode) {
		case Common::KEYCODE_DELETE:
			if (0 < _vm->_state->_r._saveCount1) {
				ZoombiniDialogResult dialogResult = _vm->openMsgBoxDialog(ZoombiniMsgBoxType::kAskRemoveSave);
				if (dialogResult == ZoombiniDialogResult::kYes)
					_vm->_state->deleteGameAndShiftRoster(_saveEntrySelectedIdx);
				clampLoadSelection();
			}
			result = ZmbEventHandleResult::kConsumed;
			break;
		case Common::KEYCODE_LEFT:
			_saveEntrySelectedIdx = MAX<int32>(0, _saveEntrySelectedIdx - 1);
			result = ZmbEventHandleResult::kConsumed;
			break;
		case Common::KEYCODE_RIGHT:
			_saveEntrySelectedIdx = MIN<int32>(_saveEntrySelectedIdx + 1, MAX<int32>(_vm->_state->_r._saveCount1 - 1, 0));
			result = ZmbEventHandleResult::kConsumed;
			break;
		default:
			return result;
		}

		clampLoadSelection();
	}

	return result;
}

ZmbEventHandleResult ZoombiniDialogSaveLoad::saveTextBox_handleTyping(const Common::KeyState &kbd) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;

	// kbd.ascii contains unicode code point of the typed key
	char32_t uch = kbd.ascii;

	// ASCII range - filter out control characters
	if (uch < 128 && !Common::isPrint(kbd.ascii))
		return result;
	
	// Check if adding this character would exceed maximum save name length
	// Non-ASCII characters may take more than 1 byte in the target code page
	Common::U32String newInputText = _saveInputText;
	newInputText.insertChar(kbd.ascii, _saveInputCursorPos);
	if (ZmbRosterEntry::checkSaveNameSize(_vm, newInputText)) {
		_saveInputText = newInputText;
		_saveInputCursorPos += 1;
		result = ZmbEventHandleResult::kConsumed;
	}

	// TODO: Handle IME composition for CJK input (which requires modifiying backend code)
	return result;
}

void ZoombiniDialogSaveLoad::scrollButtons_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	genericButton_selectShapes(feature, hotspots, _scrollButtonStateMap);
}

void ZoombiniDialogSaveLoad::scrollButtons_onPostRender(ZmbFeature *feature) {
	// [Post-Animation Events]
	genericButton_action(feature, _scrollButtonStateMap,
								static_cast<ZoombiniPage::OnButtonActionFunc>(&ZoombiniDialogSaveLoad::scrollButtons_onButtonAction));
}

void ZoombiniDialogSaveLoad::scrollButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs) {
	switch (bsIdx) {
	case kSaveLoadDialogButton_ScrollUp:
		_saveEntryBaseIdx -= SAVESLOTS_PER_SCREEN;
		break;
	case kSaveLoadDialogButton_ScrollDown:
		_saveEntryBaseIdx += SAVESLOTS_PER_SCREEN;
		break;
	default:
		error("Invalid saveload dialog button event(%u)", bsIdx);
		break;
	}

	// Ensure _saveEntryBaseIdx is within the visible range [0, _vm->_state->_r._saveCount1)
	_saveEntryBaseIdx = MIN<int32>(_saveEntryBaseIdx, MAX<int32>(_vm->_state->_r._saveCount1 - SAVESLOTS_PER_SCREEN, 0));
	_saveEntryBaseIdx = MAX<int32>(_saveEntryBaseIdx, 0);
}

ZmbEventHandleResult ZoombiniDialogSaveLoad::scrollButtons_onWheelUp(ZmbFeature *feature, const Common::Point &absPos) {
	scrollButtons_onButtonAction(feature, kSaveLoadDialogButton_ScrollUp, _scrollButtonStateMap[kSaveLoadDialogButton_ScrollUp]);
	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniDialogSaveLoad::scrollButtons_onWheelDown(ZmbFeature *feature, const Common::Point &absPos) {
	scrollButtons_onButtonAction(feature, kSaveLoadDialogButton_ScrollDown, _scrollButtonStateMap[kSaveLoadDialogButton_ScrollDown]);
	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniDialogSaveLoad::scrollButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericButton_onLButtonDown(feature, absPos, _scrollButtonStateMap);
}

ZmbEventHandleResult ZoombiniDialogSaveLoad::scrollButtons_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;
	if (kbd.hasFlags(0)) {
		switch (kbd.keycode) {
		case Common::KEYCODE_PAGEUP: // PgUp - ScummVM implementation only
		case Common::KEYCODE_UP:
			_scrollButtonStateMap[kSaveLoadDialogButton_ScrollUp].press(_vm, _currentFrameCounter);
			result = ZmbEventHandleResult::kConsumed;
			break;
		case Common::KEYCODE_KP9:
		case Common::KEYCODE_KP8:
			if ((kbd.flags & Common::KBD_NUM) == 0) {
				_scrollButtonStateMap[kSaveLoadDialogButton_ScrollUp].press(_vm, _currentFrameCounter);
				result = ZmbEventHandleResult::kConsumed;
			}
			break;
		case Common::KEYCODE_PAGEDOWN: // PgDn - ScummVM implementation only
		case Common::KEYCODE_DOWN:
			_scrollButtonStateMap[kSaveLoadDialogButton_ScrollDown].press(_vm, _currentFrameCounter);
			result = ZmbEventHandleResult::kConsumed;
			break;
		case Common::KEYCODE_KP3:
		case Common::KEYCODE_KP2:
			if ((kbd.flags & Common::KBD_NUM) == 0) {
				_scrollButtonStateMap[kSaveLoadDialogButton_ScrollDown].press(_vm, _currentFrameCounter);
				result = ZmbEventHandleResult::kConsumed;
			}
			break;
		default:
			break;
		}
	}
	return result;
}

void ZoombiniDialogSaveLoad::longButtons_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	genericButton_selectShapes(feature, hotspots, _longButtonStateMap);
}

void ZoombiniDialogSaveLoad::longButtons_onPostRender(ZmbFeature *feature) {
	// [Post-Animation Events]
	genericButton_action(feature, _longButtonStateMap,
								static_cast<ZoombiniPage::OnButtonActionFunc>(&ZoombiniDialogSaveLoad::longButtons_onButtonAction));

	// [Text Render] Long Button Descriptions
	genericButton_textRender(feature, _longButtonStateMap, Graphics::kTextAlignCenter);
}

void ZoombiniDialogSaveLoad::longButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs) {
	switch (bsIdx) {
	case kSaveLoadDialogButton_Okay:
		if (_mode == kSaveMode) {
			// Do nothing when the savename is empty
			if (_saveInputText.empty())
				return;

			// Search for existing save name in the selected slot
			int slot = _vm->_state->searchSaveSlotByName(_saveInputText);
			if (slot < 0) { // New save name - check for available slots
				slot = _vm->_state->getAvailableSaveSlot();
				if (slot < 0) // No available slots
					_vm->openMsgBoxDialog(ZoombiniMsgBoxType::kAlertCannotSaveMoreGames);
				else
					_vm->_state->saveGame(slot);
			} else { // Existing save name - Ask for confirmation to overwrite
				ZoombiniDialogResult dialogResult = _vm->openMsgBoxDialog(ZoombiniMsgBoxType::kAskReplaceSave);
				if (dialogResult == ZoombiniDialogResult::kYes)
					_vm->_state->saveGame(slot);
			}

			_dialogResult = ZoombiniDialogResult::kYes;
			close();
		} else {
			if (_vm->_state->_r._saveCount1 <= 0)
				return;

			do {
				// Spawn confirmation dialog if a current game is dirty
				if (_vm->_state->isStateDirty()) {
					ZoombiniDialogResult dialogResult = _vm->openMsgBoxDialog(ZoombiniMsgBoxType::kAskSaveDirtyGame);
					if (dialogResult == ZoombiniDialogResult::kNo) {
						// Discard current game state and continue to load
						break;
					} else {
						// Open Save Dialog
						dialogResult = _vm->openSaveDialog();
						if (dialogResult == ZoombiniDialogResult::kNo) {
							// Cancel loading
							break;
						}
					}
				}
			} while (false);

			if (!_vm->_state->loadGame(_saveEntrySelectedIdx)) {
				error("Failed to load savegame at slot %u", _saveEntrySelectedIdx);
				break;
			}

			_vm->getActivePage()->close();
			_vm->setNextPage(_vm->_state->_f.getCurrentPageType());

			_dialogResult = ZoombiniDialogResult::kYes;
			close();
		}
		break;
	case kSaveLoadDialogButton_Cancel:
		_dialogResult = ZoombiniDialogResult::kNo;
		close();
		break;
	default:
		error("Invalid saveload dialog button event(%u)", bsIdx);
		break;
	}
}

ZmbEventHandleResult ZoombiniDialogSaveLoad::longButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericButton_onLButtonDown(feature, absPos, _longButtonStateMap);
}

ZmbEventHandleResult ZoombiniDialogSaveLoad::longButtons_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;
	switch (kbd.keycode) {
	case Common::KEYCODE_ESCAPE:
		_longButtonStateMap[kSaveLoadDialogButton_Cancel].press(_vm, _currentFrameCounter);
		result = ZmbEventHandleResult::kConsumed;
		break;
	case Common::KEYCODE_RETURN:
	case Common::KEYCODE_KP_ENTER:
		_longButtonStateMap[kSaveLoadDialogButton_Okay].press(_vm, _currentFrameCounter);
		result = ZmbEventHandleResult::kConsumed;
		break;
	default:
		break;
	}
	return result;
}

} // End of namespace Mohawk
