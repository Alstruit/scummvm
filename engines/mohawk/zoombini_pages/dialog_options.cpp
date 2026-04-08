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
#include "mohawk/zoombini_pages/dialog_options.h"

namespace Mohawk {

namespace {

constexpr uint16 kOptionDialogToggleTrueNormalShape = ZoombiniPage::kShape0001_05_OptionsOnButtonNormal;
constexpr uint16 kOptionDialogToggleTruePressedShape = ZoombiniPage::kShape0001_07_OptionsOnButtonPressed;
constexpr uint16 kOptionDialogToggleFalseNormalShape = ZoombiniPage::kShape0001_06_OptionsOffButtonNormal;
constexpr uint16 kOptionDialogToggleFalsePressedShape = ZoombiniPage::kShape0001_07_OptionsOnButtonPressed;

} // End of anonymous namespace

ZoombiniDialogOptions::ZoombiniDialogOptions(MohawkEngine_Zoombini *vm) : ZoombiniDialog(vm, ZoombiniPageType::kDialogOptions) {
	ZmbResource soundResId(ZmbArchiveKind::kSystem, kResSound0999_ButtonSFX);
	const bool isTlc = _vm->isGameVariant(GF_ZMB_TLC);
	const uint16 redPressedBaseIdx = isTlc ? 10 : 8;
	const uint16 togglePressedBaseIdx = isTlc ? 14 : 12;

	_redButtonStateMap[kOptionDialogButton_NewGame] = ButtonState(ZoombiniText::kOptionsNewGame, soundResId, 0, redPressedBaseIdx, kShape0001_03_OptionsRedButtonNormal, kShape0001_04_OptionsRedButtonPressed);
	_redButtonStateMap[kOptionDialogButton_LoadGame] = ButtonState(ZoombiniText::kOptionsLoadGame, soundResId, 1, redPressedBaseIdx + 1, kShape0001_03_OptionsRedButtonNormal, kShape0001_04_OptionsRedButtonPressed);
	_redButtonStateMap[kOptionDialogButton_SaveGame] = ButtonState(ZoombiniText::kOptionsSaveGame, soundResId, 2, redPressedBaseIdx + 2, kShape0001_03_OptionsRedButtonNormal, kShape0001_04_OptionsRedButtonPressed);
	_redButtonStateMap[kOptionDialogButton_Quit] = ButtonState(ZoombiniText::kOptionsQuit, soundResId, 3, redPressedBaseIdx + 3, kShape0001_03_OptionsRedButtonNormal, kShape0001_04_OptionsRedButtonPressed);

	_toggleButtonStateMap[kOptionDialogButton_Sound] = ToggleButtonState(ZoombiniText::kOptionsSound, soundResId, 4, togglePressedBaseIdx, kOptionDialogToggleTrueNormalShape, kOptionDialogToggleTruePressedShape, kOptionDialogToggleFalseNormalShape, kOptionDialogToggleFalsePressedShape);
	_toggleButtonStateMap[kOptionDialogButton_Music] = ToggleButtonState(ZoombiniText::kOptionsMusic, soundResId, 5, togglePressedBaseIdx + 1, kOptionDialogToggleTrueNormalShape, kOptionDialogToggleTruePressedShape, kOptionDialogToggleFalseNormalShape, kOptionDialogToggleFalsePressedShape);
	_toggleButtonStateMap[kOptionDialogButton_StickyMouse] = ToggleButtonState(ZoombiniText::kOptionsStickyMouse, soundResId, 6, togglePressedBaseIdx + 2, kOptionDialogToggleTrueNormalShape, kOptionDialogToggleTruePressedShape, kOptionDialogToggleFalseNormalShape, kOptionDialogToggleFalsePressedShape);
	_toggleButtonStateMap[kOptionDialogButton_Transitions] = ToggleButtonState(ZoombiniText::kOptionsTransitions, soundResId, 7, togglePressedBaseIdx + 3, kOptionDialogToggleTrueNormalShape, kOptionDialogToggleTruePressedShape, kOptionDialogToggleFalseNormalShape, kOptionDialogToggleFalsePressedShape);
	if (isTlc) {
		_toggleButtonStateMap[kOptionDialogButton_TouchSense] = ToggleButtonState(ZoombiniText::kOptionsTouchSense, soundResId, 8, togglePressedBaseIdx + 4, kOptionDialogToggleTrueNormalShape, kOptionDialogToggleTruePressedShape, kOptionDialogToggleFalseNormalShape, kOptionDialogToggleFalsePressedShape);
		_toggleButtonStateMap[kOptionDialogButton_HelpAudio] = ToggleButtonState(ZoombiniText::kOptionsHelpAudio, soundResId, 9, togglePressedBaseIdx + 5, kOptionDialogToggleTrueNormalShape, kOptionDialogToggleTruePressedShape, kOptionDialogToggleFalseNormalShape, kOptionDialogToggleFalsePressedShape);
	}

	_longButtonStateMap[kOptionDialogButton_Okay] = ButtonState(ZoombiniText::kDialogButtonOkay, soundResId, 0, 2, kShape0001_09_ShortGreenButtonNormal, kShape0001_10_ShortGreenButtonPressed);
	_longButtonStateMap[kOptionDialogButton_Credits] = ButtonState(ZoombiniText::kOptionsCredits, soundResId, 1, 3, kShape0001_14_LongRedButtonNormal, kShape0001_15_LongRedButtonPressed);
}

ZoombiniDialogOptions::~ZoombiniDialogOptions() {
}

void ZoombiniDialogOptions::loadFeatures() {
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap0001_Dialog), kResScrb0001_DialogOptionsFrame, 0,
		ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST);

	for (auto it = _redButtonStateMap.begin(); it != _redButtonStateMap.end(); it++)
		it->second.reset();
	for (auto it = _toggleButtonStateMap.begin(); it != _toggleButtonStateMap.end(); it++)
		it->second.reset();
	ZmbFeature::EventHooks hooks0002;
	hooks0002.setPreRenderShapeFunc(static_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniDialogOptions::redToggleButtons_onPreRenderShape));
	hooks0002.setPostRenderFunc(static_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniDialogOptions::redToggleButtons_onPostRender));
	hooks0002.setLButtonDownFunc(static_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniDialogOptions::redToggleButtons_onLButtonDown));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap0001_Dialog), kResScrb0002_DialogOptionsSmallButtons, 1,
		ZmbFeature::FLAG_04000000_OVERLAY,
		hooks0002);
	
	for (auto it = _longButtonStateMap.begin(); it != _longButtonStateMap.end(); it++)
		it->second.reset();
	ZmbFeature::EventHooks hooks0003;
	hooks0003.setPreRenderShapeFunc(static_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniDialogOptions::longButtons_onPreRenderShape));
	hooks0003.setPostRenderFunc(static_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniDialogOptions::longButtons_onPostRender));
	hooks0003.setLButtonDownFunc(static_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniDialogOptions::longButtons_onLButtonDown));
	hooks0003.setKeyDownFunc(static_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogOptions::longButtons_onKeyDown));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap0001_Dialog), kResScrb0003_DialogOptionsBigButtons, 9,
		ZmbFeature::FLAG_04000000_OVERLAY,
		hooks0003);
}

bool ZoombiniDialogOptions::getOptionDialogToggleValue(uint16 bsIdx) {
	switch (bsIdx) {
	case kOptionDialogButton_Sound:
		return _vm->_state->getEnableSound();
	case kOptionDialogButton_Music:
		return _vm->_state->getEnableMusic();
	case kOptionDialogButton_StickyMouse:
		return _vm->_state->getEnableStickyMouse();
	case kOptionDialogButton_Transitions:
		return _vm->_state->getEnableTransitions();
	case kOptionDialogButton_TouchSense:
		return _vm->_state->getEnableTouchSense();
	case kOptionDialogButton_HelpAudio:
		return _vm->_state->getEnableHelpAudio();
	default:
		error("Invalid option dialog toggle button idx: %u", bsIdx);
		return false;
	}
}

void ZoombiniDialogOptions::redToggleButtons_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	genericButton_selectShapes(feature, hotspots, _redButtonStateMap);

	for (auto it = _toggleButtonStateMap.begin(); it != _toggleButtonStateMap.end(); it++) {
		ToggleButtonState &bs = it->second;
		bs._toggleState = getOptionDialogToggleValue(it->first);
	}

	genericToggleButton_selectShapes(feature, hotspots, _toggleButtonStateMap);
}

void ZoombiniDialogOptions::redToggleButtons_onPostRender(ZmbFeature *feature) {	
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	{ // [Text Render] Dialog Title
		ZoombiniGraphics::TextConf titleConf;
		titleConf._fontUsage = ZoombiniFontUsage::kFontTitle;
		titleConf._hAlign = Graphics::kTextAlignCenter;
		titleConf._vAlign = Graphics::kTextAlignCenter;
		titleConf._outlineEffect = true;
		titleConf._outlinePalette = 0x0E;
		titleConf._textPalette = ZoombiniGraphics::kColor2D_Black;
		_vm->_gfx->drawText(screenKind, ZoombiniText::kOptionsTitle, getOptionDialogTitleRect(), titleConf);
	}
	
	{ // [Text Render] Toggle Title & Legend
		ZoombiniGraphics::TextConf bigConf;
		bigConf._fontUsage = ZoombiniFontUsage::kFontTitle;
		bigConf._textPalette = ZoombiniGraphics::kColor2D_Black;
		if (!_vm->isGameVariant(GF_ZMB_TLC))
			_vm->_gfx->drawText(screenKind, ZoombiniText::kOptionsToggle, _optionDialogToggleRect, bigConf);
		_vm->_gfx->drawText(screenKind, ZoombiniText::kOptionsLegendOn, _optionDialogLegendOnRect, bigConf);
		_vm->_gfx->drawText(screenKind, ZoombiniText::kOptionsLegendOff, _optionDialogLegendOffRect, bigConf);
	}
	
	// [Text Render] Small Button Descriptions
	ZoombiniGraphics::TextConf tc;
	tc._textPalette = ZoombiniGraphics::kColor2D_Black;
	genericButton_textRender(feature, _redButtonStateMap, 
		static_cast<ButtonGetRectFunc>(&ZoombiniDialogOptions::redButtons_textRect), 
		tc);
	genericToggleButton_textRender(feature, _toggleButtonStateMap, 
		static_cast<ToggleButtonGetRectFunc>(&ZoombiniDialogOptions::toggleButtons_textRect), 
		tc);

	// [Post-Animation Events]
	genericButton_action(feature, _redButtonStateMap, 
		static_cast<OnButtonActionFunc>(&ZoombiniDialogOptions::redButtons_onButtonAction));
	genericToggleButton_postAnimation(feature, _toggleButtonStateMap,
		static_cast<OnToggleButtonPostAnimationFunc>(&ZoombiniDialogOptions::toggleButtons_onButtonAction));
}

Common::Rect ZoombiniDialogOptions::redButtons_textRect(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs, const Common::Rect &drawnRect) {
	uint16 top = getOptionDialogTextTop(bsIdx);
	return Common::Rect(_optionDialogTextLeft, top, _optionDialogTextRight, top + drawnRect.height());
}

Common::Rect ZoombiniDialogOptions::toggleButtons_textRect(ZmbFeature *feature, uint32 bsIdx, ToggleButtonState &bs, const Common::Rect &drawnRect) {
	uint16 top = getOptionDialogTextTop(bsIdx);
	return Common::Rect(_optionDialogTextLeft, top, _optionDialogTextRight, top + drawnRect.height());
}

const Common::Rect &ZoombiniDialogOptions::getOptionDialogTitleRect() const {
	return _vm->isGameVariant(GF_ZMB_TLC) ? _optionDialogTlcTitleRect : _optionDialogTitleRect;
}

uint16 ZoombiniDialogOptions::getOptionDialogTextTop(uint32 bsIdx) const {
	if (_vm->isGameVariant(GF_ZMB_TLC)) {
		if (bsIdx < ARRAYSIZE(_optionDialogTlcTextTops))
			return _optionDialogTlcTextTops[bsIdx];
	} else if (bsIdx < ARRAYSIZE(_optionDialogTextTops)) {
		return _optionDialogTextTops[bsIdx];
	}

	error("Invalid option dialog text idx: %u", bsIdx);
	return 0;
}

void ZoombiniDialogOptions::redButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs) {
	switch (bsIdx) {
	case kOptionDialogButton_NewGame:
		_vm->_state->startNewGame();
		break;
	case kOptionDialogButton_LoadGame:
		_vm->openLoadDialog(false);
		break;
	case kOptionDialogButton_SaveGame:
		_vm->openSaveDialog();
		break;
	case kOptionDialogButton_Quit:
		_vm->quitGame();
		break;
	default:
		error("Invalid option dialog red button event(%u)", bsIdx);
		break;
	}
}

void ZoombiniDialogOptions::toggleButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ToggleButtonState &bs) {
	switch (bsIdx) {
	case kOptionDialogButton_Sound:
		_vm->_state->toggleSound();
		break;
	case kOptionDialogButton_Music:
		_vm->_state->toggleMusic();
		break;
	case kOptionDialogButton_StickyMouse:
		_vm->_state->toggleStickyMouse();
		break;
	case kOptionDialogButton_Transitions:
		_vm->_state->toggleTransitions();
		break;
	case kOptionDialogButton_TouchSense:
		_vm->_state->toggleTouchSense();
		break;
	case kOptionDialogButton_HelpAudio:
		_vm->_state->toggleHelpAudio();
		break;
	default:
		error("Invalid option dialog toggle button event(%u)", bsIdx);
		break;
	}
}

ZmbEventHandleResult ZoombiniDialogOptions::redToggleButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;

	// Small Button Descriptions
	result = genericButton_onLButtonDown(feature, absPos, _redButtonStateMap);
	result = genericToggleButton_onLButtonDown(feature, absPos, _toggleButtonStateMap);

	return result;
}

void ZoombiniDialogOptions::longButtons_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	genericButton_selectShapes(feature, hotspots, _longButtonStateMap);
}

void ZoombiniDialogOptions::longButtons_onPostRender(ZmbFeature *feature) {
	// [Text Render] Big Button Descriptions
	ZoombiniGraphics::TextConf tc;
	tc._fontUsage = ZoombiniFontUsage::kFontTitle;
	tc._hAlign = Graphics::kTextAlignCenter;
	tc._vAlign = Graphics::kTextAlignCenter;
	genericButton_textRender(feature, _longButtonStateMap, tc, -1, 1);
	
	// [Post-Animation Events]
	genericButton_action(feature, _longButtonStateMap, 
		static_cast<OnButtonActionFunc>(&ZoombiniDialogOptions::longButtons_onButtonAction));
}

void ZoombiniDialogOptions::longButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs) {
	switch (bsIdx) {
	case kOptionDialogButton_Okay:
		close();
		break;
	case kOptionDialogButton_Credits:
		_vm->openCreditsDialog();
		break;
	default:
		error("Invalid option dialog long button event(%u)", bsIdx);
		break;
	}
}

ZmbEventHandleResult ZoombiniDialogOptions::longButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericButton_onLButtonDown(feature, absPos, _longButtonStateMap);
}

ZmbEventHandleResult ZoombiniDialogOptions::longButtons_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kConsumed;
	switch (kbd.keycode) {
	case Common::KEYCODE_RETURN:
	case Common::KEYCODE_KP_ENTER:
	case Common::KEYCODE_ESCAPE:
		_longButtonStateMap[kOptionDialogButton_Okay].press(_vm, _currentFrameCounter);
		break;
	default:
		result = ZmbEventHandleResult::kPassthrough;
		break;
	}
	return result;
}

} // End of namespace Mohawk
