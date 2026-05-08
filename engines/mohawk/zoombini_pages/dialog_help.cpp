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
#include "mohawk/zoombini_pages/dialog_help.h"

namespace Mohawk {

ZoombiniDialogHelp::ZoombiniDialogHelp(MohawkEngine_Zoombini *vm, ZoombiniPageType forPage) : 
	ZoombiniDialog(vm, ZoombiniPageType::kDialogHelp), _forPageType(forPage) {
}

ZoombiniDialogHelp::~ZoombiniDialogHelp() {
	if (_helpSoundQueue != ZoombiniSound::kInvalidSoundQueueHandle)
		_vm->_sound->deleteSoundQueue(_helpSoundQueue);
}

void ZoombiniDialogHelp::loadFeatures() {
	// Load help string
	_helpStrlResId = _vm->_state->readPageHelpStrings(_forPageType, _pageHelpBodyStrs);
	if (_vm->isGameVariant(GF_ZMB_TLC))
		_helpSoundQueue = _vm->_sound->createSoundQueue();

	// Initialize button states
	ZmbResource soundResId = ZmbResource(ZmbArchiveKind::kSystem, kResSound0999_ButtonSFX);
	_helpDialogButtonStateMap[kHelpDialogButton_Prev] = ButtonState(ZoombiniText::kDialogButtonPrev, soundResId, 2, 5, kShape0001_26_HelpDialogPrevButtonNormal, kShape0001_27_HelpDialogPrevButtonPressed);
	_helpDialogButtonStateMap[kHelpDialogButton_Next] = ButtonState(ZoombiniText::kDialogButtonNext, soundResId, 3, 6, kShape0001_28_HelpDialogNextButtonNormal, kShape0001_29_HelpDialogNextButtonPressed);
	_helpDialogButtonStateMap[kHelpDialogButton_Okay] = ButtonState(ZoombiniText::kDialogButtonOkay, soundResId, 4, 7, kShape0001_09_ShortGreenButtonNormal, kShape0001_10_ShortGreenButtonPressed);

	// Load SCRBs
	for (auto it = _helpDialogButtonStateMap.begin(); it != _helpDialogButtonStateMap.end(); it++)
		it->second.reset();
	ZmbFeature::EventHooks hooks0017;
	hooks0017.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniDialogHelp::helpDialog_onPreRenderShape));
	hooks0017.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniDialogHelp::helpDialog_onPostRender));
	hooks0017.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniDialogHelp::helpDialog_onMouseLButtonDown));
	hooks0017.setKeyDownFunc(reinterpret_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogHelp::helpDialog_onKeyDown));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap0001_Dialog), kResScrb0017_DialogHelp, 0,
		ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
		hooks0017);

	playHelpVoice();
}

void ZoombiniDialogHelp::helpDialog_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	genericButton_selectShapes(feature, hotspots, _helpDialogButtonStateMap);
}

void ZoombiniDialogHelp::helpDialog_onPostRender(ZmbFeature *feature) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	// [Post-Animation Events]
	genericButton_action(feature, _helpDialogButtonStateMap, 
		reinterpret_cast<ZoombiniPage::OnButtonActionFunc>(&ZoombiniDialogHelp::helpDialog_onPostAnimation));

	{ // [Text Render] Dialog Title
		ZoombiniGraphics::TextConf titleConf;
		titleConf._fontUsage = ZoombiniFontUsage::kFontTitle;
		titleConf._hAlign = Graphics::kTextAlignCenter;
		titleConf._vAlign = Graphics::kTextAlignCenter;
		titleConf._outlineEffect = true;
		titleConf._outlinePalette = 0x0E;
		titleConf._textPalette = ZoombiniGraphics::kColor2D_Black;
		_vm->_gfx->drawText(screenKind, ZoombiniText::kDialogHelpTitle, helpDialog_getTitleRect(), titleConf);
	}

	// [Text Render] Prev/Next/Okay Button Descriptions
	ZoombiniGraphics::TextConf tc;
	tc._hAlign = Graphics::kTextAlignCenter;
	tc._vAlign = Graphics::kTextAlignCenter;
	genericButton_textRender(feature, _helpDialogButtonStateMap,
		reinterpret_cast<ZoombiniPage::ButtonGetRectFunc>(&ZoombiniDialogHelp::helpDialog_getButtonTextRect), tc);

	// [Text Render] String Header
	Common::U32String helpHead = _vm->_text->getPageName(_forPageType);
	uint16 routeLevel = _vm->_state->readActivePageRouteLevel();
	if (0 < routeLevel) {
		helpHead += Common::U32String::format(" %d ", routeLevel + 1);
		helpHead += _vm->_text->getLocalizedString(ZoombiniText::kDialogHelpLevel);
	}

	ZoombiniGraphics::TextConf headConf;
	headConf._textPalette = 0x23;
	_vm->_gfx->drawText(screenKind, helpHead, helpDialog_getHeadRect(), headConf);

	// [Text Render] String Body
	ZoombiniGraphics::TextConf bodyConf;
	bodyConf._wordWrap = true;
	_vm->_gfx->drawText(screenKind, _pageHelpBodyStrs[_pageHelpBodyIdx], helpDialog_getBodyRect(), bodyConf);
}

void ZoombiniDialogHelp::helpDialog_onPostAnimation(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs) {
	uint32 previousHelpBodyIdx = _pageHelpBodyIdx;

	switch (bsIdx) {
	case kHelpDialogButton_Prev:
		if (0 < _pageHelpBodyIdx)
			_pageHelpBodyIdx -= 1;
		break;
	case kHelpDialogButton_Next:
		if (_pageHelpBodyIdx + 1 < _pageHelpBodyStrs.size())
			_pageHelpBodyIdx += 1;
		break;
	case kHelpDialogButton_Okay:
		close();
		break;
	default:
		error("Invalid help dialog button event(%u)", bsIdx);
		break;
	}

	if (_pageHelpBodyIdx != previousHelpBodyIdx)
		playHelpVoice();
}

Common::Rect ZoombiniDialogHelp::helpDialog_getButtonTextRect(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs, const Common::Rect &buttonRect) {
	Common::Rect textRect;
	if (bsIdx < ARRAYSIZE(_helpDialogTextRects))
		textRect = _helpDialogTextRects[bsIdx];
	else
		textRect = _helpDialogButtonRects[bsIdx];

	if (!_vm->isGameVariant(GF_ZMB_TLC)) {
		textRect.top += 3;
		textRect.bottom += 3;
	}
	if (bs.isAnimating()) {
		textRect.top += 2;
		textRect.bottom += 2;
	}
	return textRect;
}

const Common::Rect &ZoombiniDialogHelp::helpDialog_getTitleRect() const {
	return _vm->isGameVariant(GF_ZMB_TLC) ? _helpDialogTlcTitleRect : _helpDialogTitleRect;
}

const Common::Rect &ZoombiniDialogHelp::helpDialog_getHeadRect() const {
	return _vm->isGameVariant(GF_ZMB_TLC) ? _helpDialogTlcHeadRect : _helpDialogHeadRect;
}

const Common::Rect &ZoombiniDialogHelp::helpDialog_getBodyRect() const {
	return _vm->isGameVariant(GF_ZMB_TLC) ? _helpDialogTlcBodyRect : _helpDialogBodyRect;
}

ZmbEventHandleResult ZoombiniDialogHelp::helpDialog_onMouseLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericButton_onLButtonDown(feature, absPos, _helpDialogButtonStateMap,
		reinterpret_cast<ZoombiniPage::ButtonGetRectFunc>(&ZoombiniDialogHelp::helpDialog_getButtonTextRect));
}

Common::Rect ZoombiniDialogHelp::helpDialog_getButtonClickRect(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs, const Common::Rect &buttonRect) {
	return _helpDialogButtonRects[bsIdx];
}

ZmbEventHandleResult ZoombiniDialogHelp::helpDialog_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;

	// ScummVM implementation only shortcuts
	switch (kbd.keycode) {
	case Common::KEYCODE_RETURN:
	case Common::KEYCODE_KP_ENTER:
	case Common::KEYCODE_ESCAPE:
		_helpDialogButtonStateMap[kHelpDialogButton_Okay].press(_vm, _currentFrameCounter);
		result = ZmbEventHandleResult::kConsumed;
		break;
	case Common::KEYCODE_LEFT:
	case Common::KEYCODE_UP:
	case Common::KEYCODE_PAGEUP:
		_helpDialogButtonStateMap[kHelpDialogButton_Prev].press(_vm, _currentFrameCounter);
		result = ZmbEventHandleResult::kConsumed;
		break;
	case Common::KEYCODE_KP4:
	case Common::KEYCODE_KP8:
	case Common::KEYCODE_KP9:
		if ((kbd.flags & Common::KBD_NUM) == 0) {
			_helpDialogButtonStateMap[kHelpDialogButton_Prev].press(_vm, _currentFrameCounter);
			result = ZmbEventHandleResult::kConsumed;
		}
		break;

	case Common::KEYCODE_RIGHT:
	case Common::KEYCODE_DOWN:
	case Common::KEYCODE_PAGEDOWN:
		_helpDialogButtonStateMap[kHelpDialogButton_Next].press(_vm, _currentFrameCounter);
		result = ZmbEventHandleResult::kConsumed;
		break;
	case Common::KEYCODE_KP6:
	case Common::KEYCODE_KP2:
	case Common::KEYCODE_KP3:
		if ((kbd.flags & Common::KBD_NUM) == 0) {
			_helpDialogButtonStateMap[kHelpDialogButton_Next].press(_vm, _currentFrameCounter);
			result = ZmbEventHandleResult::kConsumed;
		}
		break;
	default:
		break;
	}
	return result;
}

void ZoombiniDialogHelp::stopHelpVoice() {
	if (_helpSoundQueue != ZoombiniSound::kInvalidSoundQueueHandle)
		_vm->_sound->stopSoundQueue(_helpSoundQueue);
}

void ZoombiniDialogHelp::playHelpVoice() {
	if (!_vm->isGameVariant(GF_ZMB_TLC))
		return;

	stopHelpVoice();

	if (!_vm->_state->getEnableHelpAudio())
		return;
	if (_helpSoundQueue == ZoombiniSound::kInvalidSoundQueueHandle)
		return;
	if (!_helpStrlResId)
		return;

	uint32 voiceResId = static_cast<uint32>(_helpStrlResId) + _pageHelpBodyIdx + 20000;
	if (0xFFFF < voiceResId)
		return;

	ZmbResource voiceRes(ZmbArchiveKind::kSystem, static_cast<uint16>(voiceResId));
	if (!_vm->hasResource(ID_SND, voiceRes)) {
		debugC(kZmbDebugHelp, "Zoombini: TLC help voice SND %u missing from HELP.MHK", static_cast<uint16>(voiceResId));
		return;
	}

	_vm->_sound->queueZmbSound(_helpSoundQueue, voiceRes, Audio::Mixer::kSpeechSoundType, false);
}

} // End of namespace Mohawk
