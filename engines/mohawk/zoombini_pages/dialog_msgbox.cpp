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
#include "mohawk/zoombini_pages/dialog_msgbox.h"

namespace Mohawk {

ZoombiniDialogMsgBox::ZoombiniDialogMsgBox(MohawkEngine_Zoombini *vm, ZoombiniMsgBoxType type) : ZoombiniDialog(vm, ZoombiniPageType::kDialogMsgBox), _type(type) {
	ZoombiniText::Key yesKey = ZoombiniText::kDialogButtonYes;
	ZoombiniText::Key noKey = ZoombiniText::kDialogButtonNo;

	switch (_type) {
	case ZoombiniMsgBoxType::kAlertNoSavedGame:
		_msgKey = ZoombiniText::kDialogBodyNoSavedGames;
		yesKey = ZoombiniText::kDialogButtonOkay;
		noKey = ZoombiniText::kNone;
		break;
	case ZoombiniMsgBoxType::kAskCreateAndSaveNewGame:
		_msgKey = ZoombiniText::kDialogBodyCreateAndSaveNewGame;
		yesKey = ZoombiniText::kDialogButtonNewGame;
		noKey = ZoombiniText::kDialogButtonCancel;
		break;
	case ZoombiniMsgBoxType::kAskCreateNewGame:
		_msgKey = ZoombiniText::kDialogBodyCreateNewGame;
		yesKey = ZoombiniText::kDialogButtonNewGame;
		noKey = ZoombiniText::kDialogButtonCancel;
		break;
	case ZoombiniMsgBoxType::kAskReplaceSave:
		_msgKey = ZoombiniText::kDialogBodyReplaceGame;
		break;
	case ZoombiniMsgBoxType::kAskSaveCurrentGame:
		_msgKey = ZoombiniText::kDialogBodySaveCurrentGame;
		break;
	case ZoombiniMsgBoxType::kAskSaveBeforeQuit:
		_msgKey = ZoombiniText::kDialogBodySaveBeforeQuit;
		break;
	case ZoombiniMsgBoxType::kAlertCannotSaveInPractice:
		_msgKey = ZoombiniText::kDialogBodyCannotSaveInPractice;
		yesKey = ZoombiniText::kDialogButtonOkay;
		noKey = ZoombiniText::kNone;
		break;
	case ZoombiniMsgBoxType::kAlertCannotSaveMoreGames:
		_msgKey = ZoombiniText::kDialogBodyCannotSaveMoreGame;
		yesKey = ZoombiniText::kDialogButtonOkay;
		noKey = ZoombiniText::kNone;
		break;
	case ZoombiniMsgBoxType::kAlertCannotLoadInPractice:
		_msgKey = ZoombiniText::kDialogBodyCannotLoadInPractice;
		yesKey = ZoombiniText::kDialogButtonOkay;
		noKey = ZoombiniText::kNone;
		break;
	case ZoombiniMsgBoxType::kAlertCannotCreateNewInPractice:
		_msgKey = ZoombiniText::kDialogBodyCannotCreateNewInPractice;
		yesKey = ZoombiniText::kDialogButtonOkay;
		noKey = ZoombiniText::kNone;
		break;
	case ZoombiniMsgBoxType::kAskReallyQuit:
		_msgKey = ZoombiniText::kDialogBodyReallyQuit;
		break;
	case ZoombiniMsgBoxType::kAskSaveDirtyGame:
		_msgKey = ZoombiniText::kDialogBodySaveDirtyGame;
		break;
	case ZoombiniMsgBoxType::kAskGoMapWillLost:
		_msgKey = ZoombiniText::kDialogBodyGoMapWillLost;
		yesKey = ZoombiniText::kDialogButtonLoseThem;
		noKey = ZoombiniText::kDialogButtonKeepThem;
		break;
	default:
		error("Invalid ZoombiniMsgBoxType: %u", static_cast<uint32>(_type));
		break;
	}

	ZmbResource soundResId(ZmbArchiveKind::kSystem, kResSound0999_ButtonSFX);
	_longButtonStateMap[kMsgBoxDialogButton_Yes] = ButtonState(yesKey, soundResId, 0, 2, kShape0001_12_LongGreenButtonNormal, kShape0001_13_LongGreenButtonPressed);
	_longButtonStateMap[kMsgBoxDialogButton_No] = ButtonState(noKey, soundResId, 1, 3, kShape0001_14_LongRedButtonNormal, kShape0001_15_LongRedButtonPressed);
	if (noKey == ZoombiniText::kNone) {
		_longButtonStateMap[kMsgBoxDialogButton_No]._drawEnabled = false;
	}
}

ZoombiniDialogMsgBox::~ZoombiniDialogMsgBox() {
}

void ZoombiniDialogMsgBox::loadFeatures() {
	// Load SCRBs
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap0001_Dialog), kResScrb0010_DialogMsgBox, 0,
					ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST);

	for (auto it = _longButtonStateMap.begin(); it != _longButtonStateMap.end(); it++)
		it->second.reset();
	ZmbFeature::EventHooks hooksLongButtons;
	hooksLongButtons.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniDialogMsgBox::longButtons_onPreRenderShape));
	hooksLongButtons.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniDialogMsgBox::longButtons_onPostRender));
	hooksLongButtons.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniDialogMsgBox::longButtons_onLButtonDown));
	hooksLongButtons.setKeyDownFunc(reinterpret_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogMsgBox::longButtons_onKeyDown));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap0001_Dialog), kResScrb0011_DialogMsgBox, 15,
					ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
					hooksLongButtons);
}

void ZoombiniDialogMsgBox::longButtons_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	// Adjust Yes button position if No button is disabled (Alert MessageBox)
	if (!_longButtonStateMap[kMsgBoxDialogButton_No]._drawEnabled) {
		uint16 hsYesNormalIdx = _longButtonStateMap[kMsgBoxDialogButton_Yes]._hsNormalId;
		uint16 hsYesPressedIdx = _longButtonStateMap[kMsgBoxDialogButton_Yes]._hsPressedId;
		ZmbHotspot &hsYesNormal = hotspots[hsYesNormalIdx];
		ZmbHotspot &hsYesPressed = hotspots[hsYesPressedIdx];
		hsYesNormal._x -= 90;
		hsYesPressed._x -= 90;

		uint16 hsNoNormalIdx = _longButtonStateMap[kMsgBoxDialogButton_No]._hsNormalId;
		uint16 hsNoPressedIdx = _longButtonStateMap[kMsgBoxDialogButton_No]._hsPressedId;
		ZmbHotspot &hsNoNormal = hotspots[hsNoNormalIdx];
		ZmbHotspot &hsNoPressed = hotspots[hsNoPressedIdx];
		hsNoNormal._shapeIdx = ZmbHotspot::kShapeNone;
		hsNoPressed._shapeIdx = ZmbHotspot::kShapeNone;
	}
	
	genericButton_selectShapes(feature, hotspots, _longButtonStateMap);
}

void ZoombiniDialogMsgBox::longButtons_onPostRender(ZmbFeature *feature) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	// [Post-Animation Events]
	genericButton_action(feature, _longButtonStateMap, 
		reinterpret_cast<ZoombiniPage::OnButtonActionFunc>(&ZoombiniDialogMsgBox::longButtons_onButtonAction));

	{ // [Text Render] Dialog Body
		ZoombiniGraphics::TextConf bodyConf;
		bodyConf._fontUsage = ZoombiniFontUsage::kFontTitle;
		bodyConf._hAlign = Graphics::kTextAlignCenter;
		bodyConf._vAlign = Graphics::kTextAlignCenter;
		bodyConf._outlineEffect = true;
		bodyConf._outlinePalette = ZoombiniGraphics::kColor0E_VeryLightGray;
		bodyConf._textPalette = ZoombiniGraphics::kBlackKey;
		bodyConf._wordWrap = true;
		_vm->_gfx->drawText(screenKind, _msgKey, _msgBoxDialogBodyRect, bodyConf);
	}

	// [Text Render] Yes/No Button Descriptions
	genericButton_textRender(feature, _longButtonStateMap, Graphics::kTextAlignCenter);
}

void ZoombiniDialogMsgBox::longButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs) {
	switch (bsIdx) {
	case kMsgBoxDialogButton_Yes:
		_dialogResult = ZoombiniDialogResult::kYes;
		close();
		break;
	case kMsgBoxDialogButton_No:
		_dialogResult = ZoombiniDialogResult::kNo;
		close();
		break;
	default:
		error("Invalid msgbox dialog button event(%u)", bsIdx);
		break;
	}
}

ZmbEventHandleResult ZoombiniDialogMsgBox::longButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericButton_onLButtonDown(feature, absPos, _longButtonStateMap);
}

ZmbEventHandleResult ZoombiniDialogMsgBox::longButtons_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kConsumed;
	switch (kbd.keycode) {
	case Common::KEYCODE_RETURN:
	case Common::KEYCODE_KP_ENTER:
		_longButtonStateMap[kMsgBoxDialogButton_Yes].press(_vm, _currentFrameCounter);
		break;
	case Common::KEYCODE_ESCAPE:
		if (_longButtonStateMap[kMsgBoxDialogButton_No]._drawEnabled)
			_longButtonStateMap[kMsgBoxDialogButton_No].press(_vm, _currentFrameCounter);
		break;
	default:
		result = ZmbEventHandleResult::kPassthrough;
		break;
	}
	return result;
}

} // End of namespace Mohawk
