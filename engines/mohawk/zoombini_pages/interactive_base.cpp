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
#include "mohawk/zoombini_pages/interactive_base.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

ZoombiniInteractive::ZoombiniInteractive(MohawkEngine_Zoombini *vm, ZoombiniPageType pageType) : ZoombiniPage(vm, ZoombiniPageCategory::kInteractive, pageType) {
	_useFadeEffect = true;

	_goMapButtonStateMap[kThreeButtons_Go] = ButtonState();
	_goMapButtonStateMap[kThreeButtons_SecondGo] = ButtonState();
	_goMapButtonStateMap[kThreeButtons_Map] = ButtonState();
	_helpButtonStateMap[kThreeButtons_Help] = ButtonState();
}

ZoombiniInteractive::~ZoombiniInteractive() {
	_vm->_midi->stop();
	_vm->_sound->stopSound();

	_vm->clearPageArchives();
}

ZmbEventHandleResult ZoombiniInteractive::onKeyDown(const Common::KeyState &kbd, bool kbdRepeat) {
	if (kbdRepeat)
		return ZmbEventHandleResult::kConsumed;

	ZmbEventHandleResult result = ZmbEventHandleResult::kConsumed;
	if (kbd.hasFlags(Common::KBD_CTRL)) {
		switch (kbd.keycode) {
		case Common::KEYCODE_n:
			_vm->_state->startNewGame(); // New Game (CTRL-N)
			break;
		case Common::KEYCODE_l: // Load Game (CTRL-L)
			_vm->openLoadDialog();
			break;
		case Common::KEYCODE_s: // Save Game (CTRL-S)
			_vm->openSaveDialog();
			break;
		case Common::KEYCODE_q: // Quit (CTRL-Q)
			Engine::quitGame();
			break;
		case Common::KEYCODE_d: // Dialog & Sound Effects (CTRL-D)
			_vm->_state->toggleSound();
			break;
		case Common::KEYCODE_b: // Background Music (CTRL-B)
			_vm->_state->toggleMusic();
			break;
		case Common::KEYCODE_j: // Sticky Mouse (CTRL-J)
			_vm->_state->toggleStickyMouse();
			break;
		case Common::KEYCODE_t: // Transition (CTRL-T)
			_vm->_state->toggleTransitions();
			break;
		case Common::KEYCODE_g: // Less/More Action (CTRL-G)
			_vm->_state->toggleLessMoreAction();
			break;
		case Common::KEYCODE_h: // Hide/Show Cursor (CTRL-H)
			_vm->_state->toggleCursorVisibility();
			break;
		default:
			result = ZmbEventHandleResult::kPassthrough;
			break;
		}
	} else {
		switch (kbd.keycode) {
		case Common::KEYCODE_SLASH:
		case Common::KEYCODE_QUESTION: // Options Dialog
			if (!_vm->hasDialogOpened())
				_vm->openOptionsDialog();
			break;
		case Common::KEYCODE_F1: // Play help sound
			playActiveHelpSound();
			break;
		default:
			result = ZmbEventHandleResult::kPassthrough;
			break;
		}
	}

	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Run feature level onKeyDown handlers
	return ZoombiniPage::onKeyDown(kbd, kbdRepeat);
}

void ZoombiniInteractive::continuousButton_selectShapes(ZmbFeature *feature, Common::Array<ZmbHotspot> &hotspots, Common::StableMap<uint32, ContinuousButtonState> &contButtonStateMap, uint16 pressedDeltaX, uint16 pressedDeltaY) {
	for (auto it = contButtonStateMap.begin(); it != contButtonStateMap.end(); it++) {
		ContinuousButtonState &cbs = it->second;

		if (!cbs._enabled)
			continue;

		ZmbHotspot &hsNormal = hotspots[cbs._hsNormalIdx];
		ZmbHotspot &hsPressed = hotspots[cbs._hsPressedIdx];

		if (cbs._pressed) {
			hsNormal._shapeIdx = ZmbHotspot::kShapeNone;
			hsPressed._x += pressedDeltaX;
			hsPressed._y += pressedDeltaY;
		} else {
			hsPressed._shapeIdx = ZmbHotspot::kShapeNone;
		}
	}
}

void ZoombiniInteractive::ContinuousButtonState::press() {
	_pressed = true;
}

void ZoombiniInteractive::ContinuousButtonState::release() {
	_pressed = false;
}

void ZoombiniInteractive::setGoButton(const Common::Rect &rect, uint16 shapeDisabledId, uint16 shapeEnabledId, uint16 shapePressedId) {
	_goButtonRect = rect;
	_goButtonShapeDisabledId = shapeDisabledId;
	_goButtonShapeEnabledId = shapeEnabledId;
	_goButtonShapePressedId = shapePressedId;

	ZmbResource soundResId(ZmbArchiveKind::kSystem, kResSound0999_ButtonSFX);
	ButtonState goButtonState = ButtonState(soundResId, kHotspotGoButtonNormal, kHotspotGoButtonPressed, shapeEnabledId, shapePressedId);
	goButtonState.setDisabledState(shapeDisabledId);
	goButtonState._isPressDisabled = true;
	_goMapButtonStateMap[kThreeButtons_Go] = goButtonState;
	_threeButtonRectMap[kThreeButtons_Go] = rect;
}

void ZoombiniInteractive::setSecondGoButton(const Common::Rect &rect, uint16 shapeDisabledId, uint16 shapeEnabledId, uint16 shapePressedId) {
	_secondGoButtonRect = rect;
	_secondGoButtonShapeDisabledId = shapeDisabledId;
	_secondGoButtonShapeEnabledId = shapeEnabledId;
	_secondGoButtonShapePressedId = shapePressedId;

	ZmbResource soundResId(ZmbArchiveKind::kSystem, kResSound0999_ButtonSFX);
	ButtonState secondGoButtonState = ButtonState(soundResId, kHotspotSecondGoButtonNormal, kHotspotSecondGoButtonPressed, shapeEnabledId, shapePressedId);
	secondGoButtonState.setDisabledState(shapeDisabledId);
	secondGoButtonState._isPressDisabled = true;
	_goMapButtonStateMap[kThreeButtons_SecondGo] = secondGoButtonState;
	_threeButtonRectMap[kThreeButtons_SecondGo] = rect;
}

void ZoombiniInteractive::setMapButton(const Common::Rect &rect, uint16 shapeNormalId, uint16 shapePressedId) {
	_mapButtonRect = rect;
	_mapButtonShapeNormalId = shapeNormalId;
	_mapButtonShapePressedId = shapePressedId;

	ZmbResource soundResId(ZmbArchiveKind::kSystem, kResSound0999_ButtonSFX);
	_goMapButtonStateMap[kThreeButtons_Map] = ButtonState(soundResId, kHotspotMapButtonNormal, kHotspotMapButtonPressed, shapeNormalId, shapePressedId);
	_threeButtonRectMap[kThreeButtons_Map] = rect;
}

void ZoombiniInteractive::setHelpButton(const Common::Rect &rect) {
	_helpButtonRect = rect;

	ZmbResource soundResId(ZmbArchiveKind::kSystem, kResSound0999_ButtonSFX);
	_helpButtonStateMap[kThreeButtons_Help] = ButtonState(soundResId, kHotspotHelpButtonNormal, kHotspotHelpButtonPressed, kShape0001_24_HelpButtonNormal, kShape0001_25_HelpButtonPressed);
	_threeButtonRectMap[kThreeButtons_Help] = rect;
}

void ZoombiniInteractive::loadGoMapButtonsFeature(uint16 bitmapResId) {
	_goMapBitmapResId = bitmapResId;

	// At least one of Go or Map button should be enabled to load the feature.
	if (!_goMapButtonStateMap[kThreeButtons_Map]._drawEnabled && !_goMapButtonStateMap[kThreeButtons_SecondGo]._drawEnabled && !_goMapButtonStateMap[kThreeButtons_Go]._drawEnabled)
		return;

	// Go/Map button shapes & hotspots are stored in page archives.
	// Derived class is responsible for setting proper shapes & hotspots.

	// [*] Virtual Feature - Go, Map Buttons
	ZmbFeature::EventHooks hooksGoMapButtons;
	hooksGoMapButtons.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractive::goMapButtons_preRenderShape));
	hooksGoMapButtons.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractive::goMapButtons_onPostRender));
	hooksGoMapButtons.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractive::goMapButtons_onLButtonDown));

	Common::Array<ZmbHotspot> hotspots;
	// Use the enabled shapes as the initial hotspot shape.
	// genericButton_selectShapes overrides to the disabled shape when _isPressDisabled = true,
	// so the initial value must be the enabled shape for the button to display correctly when enabled.
	hotspots.push_back(ZmbHotspot(kHotspotGoButtonNormal, _goButtonShapeEnabledId, 0, _goButtonRect));
	hotspots.push_back(ZmbHotspot(kHotspotSecondGoButtonNormal, _secondGoButtonShapeEnabledId, 0, _secondGoButtonRect));
	hotspots.push_back(ZmbHotspot(kHotspotMapButtonNormal, _mapButtonShapeNormalId, 0, _mapButtonRect));
	hotspots.push_back(ZmbHotspot(kHotspotGoButtonPressed, _goButtonShapePressedId, 0, _goButtonRect));
	hotspots.push_back(ZmbHotspot(kHotspotSecondGoButtonPressed, _secondGoButtonShapePressedId, 0, _secondGoButtonRect));
	hotspots.push_back(ZmbHotspot(kHotspotMapButtonPressed, _mapButtonShapePressedId, 0, _mapButtonRect));

	loadVirtualFeature(ZmbResource(ZmbArchiveKind::kPage, _goMapBitmapResId), kVirtualFeatureGoMapButtons,
					   hotspots, 0,
					   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
					   hooksGoMapButtons);
}

void ZoombiniInteractive::loadHelpButtonFeature() {
	if (!_helpButtonStateMap[kThreeButtons_Help]._drawEnabled)
		return;

	// [*] Help button state
	// Help button shapes & hotspots are stored in a common archive, ZOOMBINI.MHK.
	// Help feature is added in Zoombini 1.1 release, so its shapes are not in the original page archives.
	ZmbResource soundResId(ZmbArchiveKind::kSystem, kResSound0999_ButtonSFX);
	_helpButtonStateMap[kThreeButtons_Help] = ButtonState(soundResId, kHotspotHelpButtonNormal, kHotspotHelpButtonPressed,
														  kShape0001_24_HelpButtonNormal, kShape0001_25_HelpButtonPressed);

	// [*] Virtual Features (tBMP c:0001) - Help Button
	ZmbFeature::EventHooks hooksHelpMapButton;
	hooksHelpMapButton.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractive::helpButton_preRenderShape));
	hooksHelpMapButton.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractive::helpButton_onPostRender));
	hooksHelpMapButton.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractive::helpButton_onLButtonDown));

	Common::Array<ZmbHotspot> hotspots;
	hotspots.push_back(ZmbHotspot(kHotspotHelpButtonNormal, kShape0001_24_HelpButtonNormal, 0, _helpButtonRect));
	hotspots.push_back(ZmbHotspot(kHotspotHelpButtonPressed, kShape0001_25_HelpButtonPressed, 0, _helpButtonRect));

	loadVirtualFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap0001_Dialog), kVirtualFeatureHelpButton,
					   hotspots, 0,
					   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
					   hooksHelpMapButton);
}

void ZoombiniInteractive::goMapButtons_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	genericButton_selectShapes(feature, hotspots, _goMapButtonStateMap);
}

void ZoombiniInteractive::goMapButtons_onPostRender(ZmbFeature *feature) {
	genericButton_action(feature, _goMapButtonStateMap, reinterpret_cast<OnButtonActionFunc>(&ZoombiniInteractive::goMapButtons_onButtonAction));
}

ZmbEventHandleResult ZoombiniInteractive::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// IDA: All puzzle click handlers (bc1_onButtonClick_4117F9, bridge_onClickHandler_4157EB,
	// net_funcOnClick_43747F, pizza_funcOnClick_43CFA1, etc.) check puzzle_pendingTransitionTarget
	// at the very top: if set, they immediately complete the departure transition (skip animation).
	// Replicate by clearing _hasDepartSfxHandle so isDepartSfxDone() returns true;
	// onEveryFrame() will then fire the transition on the very next tick.
	if (_pendingGoDepart) {
		_hasDepartSfxHandle = false;
		return ZmbEventHandleResult::kConsumed;
	}
	return ZoombiniPage::onLButtonDown(absPos, relPos);
}

void ZoombiniInteractive::onGoButtonActivated() {
	playDepartSfx();
	_pendingGoDepart = true;
}

void ZoombiniInteractive::onSecondGoButtonActivated() {
	playDepartSfx();
	_pendingGoDepart = true;
}

void ZoombiniInteractive::executeDeparture() {
	// Default departure: navigate to kXfer with the configured source page.
	// Pages with custom departure logic (e.g. BC1/BC2 save pack state) override this.
	if (_departXferSrcSiPage != ZMB_SI_MINUS1) {
		_vm->_xferSrcSiPage = _departXferSrcSiPage;
		_vm->setNextPage(ZoombiniPageType::kXfer);
	}
	close();
}

void ZoombiniInteractive::startDepartWalkAnimation(const Common::Point &target, uint32 stagger) {
	// IDA: zmbMoveAnimation_45479D(staggerDelay, toY, toX)
	// Iterates idle snoids and sets walk-to-target animation with staggered timing.
	uint32 frameBase = getCurrentFrameCounter();
	uint16 walkerIdx = 0;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *snoid = it->second;
		if (!snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			continue;
		if (snoid->getAnimState() != kSnoidAnimIdle)
			continue;

		snoid->setAnimTargetPos(target);
		snoid->setAnimState(kSnoidAnimArrivalMotion, nullptr);

		if (walkerIdx > 0) {
			snoid->deactivateRender();
			snoid->setDelayUntilFrame(frameBase + walkerIdx * stagger);
		}
		walkerIdx++;
	}
}

bool ZoombiniInteractive::isDepartWalkComplete() const {
	// IDA: departure polling — all snoids idle or off-screen right edge.
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if (!it->second->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			continue;
		const ZmbSnoid *snoid = it->second;
		if (snoid->getAnimState() != kSnoidAnimIdle && snoid->getPointLoc().x < 640)
			return false;
	}
	return true;
}

void ZoombiniInteractive::playDepartSfx(uint16 systemSoundId) {
	Audio::SoundHandle *handle = _vm->_sound->playZmbSound(
		ZmbResource(ZmbArchiveKind::kSystem, systemSoundId),
		Audio::Mixer::kSFXSoundType);
	_hasDepartSfxHandle = (handle != nullptr);
	if (handle)
		_departSfxHandle = *handle;
}

bool ZoombiniInteractive::isDepartSfxDone() const {
	if (!_hasDepartSfxHandle)
		return true;
	return !_vm->_system->getMixer()->isSoundHandleActive(_departSfxHandle);
}

void ZoombiniInteractive::onMapButtonActivated() {
	_vm->setNextPage(ZoombiniPageType::kRodMap);
	close();
}

void ZoombiniInteractive::goMapButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs) {
	switch (bsIdx) {
	case kThreeButtons_Go:
		if (!bs._isPressDisabled)
			onGoButtonActivated();
		break;
	case kThreeButtons_SecondGo:
		if (!bs._isPressDisabled)
			onSecondGoButtonActivated();
		break;
	case kThreeButtons_Map:
		onMapButtonActivated();
		break;
	default:
		error("Invalid option dialog long button event(%u)", bsIdx);
		break;
	}
}

ZmbEventHandleResult ZoombiniInteractive::goMapButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericButton_onLButtonDown(feature, absPos, _goMapButtonStateMap, _threeButtonRectMap, reinterpret_cast<OnButtonActionFunc>(&ZoombiniInteractive::goMapButtons_onButtonAction));
}

void ZoombiniInteractive::helpButton_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	genericButton_selectShapes(feature, hotspots, _helpButtonStateMap);
}

void ZoombiniInteractive::helpButton_onPostRender(ZmbFeature *feature) {
	genericButton_action(feature, _helpButtonStateMap, reinterpret_cast<OnButtonActionFunc>(&ZoombiniInteractive::helpButton_onPostAnimation));
}

void ZoombiniInteractive::helpButton_onPostAnimation(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs) {
	switch (bsIdx) {
	case kThreeButtons_Help:
		_vm->openHelpDialog(_vm->getActivePage()->getPageType());
		break;
	default:
		error("Invalid option dialog long button event(%u)", bsIdx);
		break;
	}
}

ZmbEventHandleResult ZoombiniInteractive::helpButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericButton_onLButtonDown(feature, absPos, _helpButtonStateMap, _threeButtonRectMap);
}

void ZoombiniInteractive::showNotiBoxShort(const Common::U32String &ustr) {
	showNotiBox(ustr, false);
}

void ZoombiniInteractive::hideNotiBoxShort() {
	_notiBoxShowUntilFrame = 0;
}

void ZoombiniInteractive::showNotiBoxLong(ZoombiniText::Key textKey) {
	const Common::U32String &ustr = _vm->_text->getLocalizedString(textKey);
	showNotiBox(ustr, true);
}

void ZoombiniInteractive::showNotiBox(const Common::U32String &ustr, bool isNotiBoxLong) {
	_isNotiBoxLong = isNotiBoxLong;
	_notiBoxText = ustr;
	if (isNotiBoxLong)
		_notiBoxShowUntilFrame = _currentFrameCounter + NOTIBOX_LONG_SHOW_FRAME_DURATION;
	else
		_notiBoxShowUntilFrame = UINT32_MAX; // Virtually infinite duration

	// Only register NotiBox feature if not yet registered.
	if (_virtualFeatureMap.find(kVirtualFeatureMinus02_NotiBox) == _virtualFeatureMap.end()) {
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractive::notiBox_preRenderShape));
		hooks.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractive::notiBox_onPostRender));

		Common::Array<ZmbHotspot> hotspots;
		hotspots.push_back(ZmbHotspot(kHotspotNotiBoxShort, kShape3001_01_NotiBoxShort, 0, _notiBoxShortRect));
		hotspots.push_back(ZmbHotspot(kHotspotNotiBoxLong, kShape3001_02_NotiBoxLong, 0, _notiBoxLongRect));

		loadVirtualFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap3001_NotiBox), kVirtualFeatureMinus02_NotiBox,
						   hotspots, 0,
						   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						   hooks);
	}
}

void ZoombiniInteractive::notiBox_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	if (_notiBoxShowUntilFrame < _currentFrameCounter)
		feature->scheduleClose();

	uint16 hideShapeIdx = _isNotiBoxLong ? kHotspotNotiBoxShort : kHotspotNotiBoxLong;
	hotspots[hideShapeIdx]._shapeIdx = ZmbHotspot::kShapeNone;
}

void ZoombiniInteractive::notiBox_onPostRender(ZmbFeature *feature) {
	if (_notiBoxText.empty())
		return;

	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	ZoombiniGraphics::TextConf tc;
	tc._vAlign = Graphics::kTextAlignCenter;
	tc._hAlign = Graphics::kTextAlignCenter;
	const Common::Rect &textRect = _isNotiBoxLong ? _notiBoxLongRect : _notiBoxShortRect;
	_vm->_gfx->drawText(screenKind, _notiBoxText, textRect, tc);
}

void ZoombiniInteractive::genericStickyButton_selectShapes(ZmbFeature *feature, Common::Array<ZmbHotspot> &hotspots, Common::StableMap<uint32, StickyButtonState> &buttonStateMap) {
	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		StickyButtonState &bs = it->second;

		if (!bs._enabled)
			continue;

		ZmbHotspot &hsNormal = hotspots[bs._hsNormalId];
		ZmbHotspot &hsPressed = hotspots[bs._hsPressedId];
		if (bs._isStuck)
			hsNormal._shapeIdx = ZmbHotspot::kShapeNone;
		else
			hsPressed._shapeIdx = ZmbHotspot::kShapeNone;
	}
}

ZmbEventHandleResult ZoombiniInteractive::genericStickyButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, StickyButtonState> &buttonStateMap, OnStickyButtonActionFunc onActionFunc) {
	ZmbDrawRecord *drawRecord = feature->findDrawRecordAtPoint(absPos);
	if (!drawRecord)
		return ZmbEventHandleResult::kPassthrough;

	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		StickyButtonState &bs = it->second;

		if (!bs._enabled)
			continue;

		if (drawRecord->_hs._hsId != bs._hsNormalId && drawRecord->_hs._hsId != bs._hsPressedId)
			continue;

		bs.toggle(_vm);
		if (onActionFunc != nullptr)
			(this->*onActionFunc)(feature, it->first, bs);
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniInteractive::genericStickyButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, StickyButtonState> &buttonStateMap, const Common::HashMap<uint32, Common::Rect> &buttonRectMap, OnStickyButtonActionFunc onActionFunc) {
	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		uint32 bsIdx = it->first;
		StickyButtonState &bs = it->second;

		if (!bs._enabled)
			continue;

		auto rit = buttonRectMap.find(bsIdx);
		if (rit == buttonRectMap.end())
			continue;
		const Common::Rect &buttonRect = rit->_value;
		if (!buttonRect.contains(absPos))
			continue;

		bs.toggle(_vm);
		if (onActionFunc != nullptr)
			(this->*onActionFunc)(feature, bsIdx, bs);
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

void ZoombiniInteractive::StickyButtonState::toggle(MohawkEngine_Zoombini *vm) {
	_isStuck = !_isStuck;
	const ZmbResource &soundId = _isStuck ? _pressSoundId : _releaseSoundId;
	if (soundId.hasId())
		vm->_sound->playZmbSound(soundId, Audio::Mixer::kSFXSoundType, false);
}

// ---------------------------------------------------------------------------
// Ambient Sound Driver
// IDA: ambient_runPerFrameSoundDriver_435F33
//
// Per-puzzle ambient SND pools. All IDs are from ZOOMBINI.MHK (kSystem, range 900-944).
// Pool sizing and SND IDs are verified against the original binary.
// ---------------------------------------------------------------------------

// DI=4
static const uint16 kAmbientPoolBC1[15] = {924, 933, 904, 905, 906, 925, 926, 927, 928, 929, 917, 918, 919, 920, 936};
// DI=5
static const uint16 kAmbientPoolBC2[10] = {924, 904, 905, 906, 933, 936, 917, 918, 919, 920};
// DI=7
static const uint16 kAmbientPoolBridge[9] = {924, 933, 904, 905, 906, 942, 943, 944, 902};
// DI=8
static const uint16 kAmbientPoolCaves[9] = {911, 914, 915, 942, 943, 944, 904, 933, 934};
// DI=9
static const uint16 kAmbientPoolPizza[12] = {940, 941, 943, 921, 917, 918, 919, 920, 905, 911, 912, 916};
// DI=10
static const uint16 kAmbientPoolFerry[19] = {924, 933, 904, 905, 906, 937, 938, 939, 902, 903, 925, 926, 927, 928, 929, 917, 918, 919, 920};
// DI=11
static const uint16 kAmbientPoolLilly[20] = {930, 931, 932, 937, 938, 939, 925, 926, 927, 928, 929, 917, 918, 919, 920, 904, 905, 906, 924, 933};
// DI=12
static const uint16 kAmbientPoolSlides[13] = {911, 912, 913, 914, 921, 936, 904, 905, 906, 917, 918, 919, 920};
// DI=13
static const uint16 kAmbientPoolFleens[13] = {911, 912, 913, 914, 921, 936, 904, 905, 906, 917, 918, 919, 920};
// DI=15
static const uint16 kAmbientPoolNet[17] = {924, 933, 904, 905, 906, 931, 932, 938, 925, 926, 927, 928, 929, 917, 918, 919, 920};
// DI=16
static const uint16 kAmbientPoolTunnels[10] = {922, 923, 935, 907, 908, 909, 900, 901, 934, 910};
// DI=17
static const uint16 kAmbientPoolSmoke[10] = {922, 923, 935, 907, 908, 909, 900, 901, 934, 910};
// DI=18
static const uint16 kAmbientPoolMaze[10] = {922, 923, 935, 907, 908, 909, 900, 901, 934, 910};

void ZoombiniInteractive::onAnimFrame() {
	runAmbientSoundDriver();

	// Two-phase departure system: poll SFX completion.
	// IDA: all puzzle funcOnHover check puzzle_pendingTransitionTarget at the top.
	// When the departure SFX finishes, commit the transition via executeDeparture().
	if (_pendingGoDepart && isDepartSfxDone()) {
		_pendingGoDepart = false;
		executeDeparture();
	}

	ZoombiniPage::onAnimFrame();
}

// TODO: When every interactive pages are implemented, consider refactoring to move ambient sound driver logic to a separate class or to the main engine class, since it's shared across all interactive pages.
void ZoombiniInteractive::runAmbientSoundDriver() {
	// IDA: chBoolSFXTurnOnOff && chBoolBGMTurnOnOff guard
	if (!_vm->_state->getEnableSound() || !_vm->_state->getEnableMusic())
		return;

	// IDA: game_getFrameCounterOrDelta() >= ambient_dwNextPlayFrameTime
	if (_currentFrameCounter < _ambientNextPlayFrame)
		return;

	// IDA: snd_isWavSlotLoaded('SND', ambient_wLastPlayedSndId)
	// Check if the last-played sound is still active.
	if (_ambientLastSndId != 0 && _vm->_system->getMixer()->isSoundHandleActive(_ambientSndHandle)) {
		// Still playing — reset timer and return without interrupting.
		_ambientNextPlayFrame = _currentFrameCounter + _vm->_rnd->getRandomNumber(180, 240);
		return;
	}

	// Sound has finished. Reset timer and pick next sound.
	_ambientNextPlayFrame = _currentFrameCounter + _vm->_rnd->getRandomNumber(180, 240);

	// Select the sound pool for the current puzzle page.
	const uint16 *pool = nullptr;
	uint16 poolSize = 0;

	switch (getPageType()) {
	case ZoombiniPageType::kBasecamp1:
		pool = kAmbientPoolBC1;
		poolSize = ARRAYSIZE(kAmbientPoolBC1);
		break;
	case ZoombiniPageType::kBasecamp2:
		pool = kAmbientPoolBC2;
		poolSize = ARRAYSIZE(kAmbientPoolBC2);
		break;
	case ZoombiniPageType::kBridge:
		pool = kAmbientPoolBridge;
		poolSize = ARRAYSIZE(kAmbientPoolBridge);
		break;
	case ZoombiniPageType::kCaves:
		pool = kAmbientPoolCaves;
		poolSize = ARRAYSIZE(kAmbientPoolCaves);
		break;
	case ZoombiniPageType::kPizza:
		pool = kAmbientPoolPizza;
		poolSize = ARRAYSIZE(kAmbientPoolPizza);
		break;
	case ZoombiniPageType::kFerry:
		pool = kAmbientPoolFerry;
		poolSize = ARRAYSIZE(kAmbientPoolFerry);
		break;
	case ZoombiniPageType::kLilly:
		pool = kAmbientPoolLilly;
		poolSize = ARRAYSIZE(kAmbientPoolLilly);
		break;
	case ZoombiniPageType::kSlides:
		pool = kAmbientPoolSlides;
		poolSize = ARRAYSIZE(kAmbientPoolSlides);
		break;
	case ZoombiniPageType::kFleens:
		pool = kAmbientPoolFleens;
		poolSize = ARRAYSIZE(kAmbientPoolFleens);
		break;
	case ZoombiniPageType::kNet:
		pool = kAmbientPoolNet;
		poolSize = ARRAYSIZE(kAmbientPoolNet);
		break;
	case ZoombiniPageType::kTunnels:
		pool = kAmbientPoolTunnels;
		poolSize = ARRAYSIZE(kAmbientPoolTunnels);
		break;
	case ZoombiniPageType::kSmoke:
		pool = kAmbientPoolSmoke;
		poolSize = ARRAYSIZE(kAmbientPoolSmoke);
		break;
	case ZoombiniPageType::kMaze:
		pool = kAmbientPoolMaze;
		poolSize = ARRAYSIZE(kAmbientPoolMaze);
		break;
	default:
		return; // No ambient pool for this page (Town, Picker, RodMap, etc.)
	}

	// IDA: e2GetPoolValue_nonRepeatRandom_46EE10(0, poolSize, &bitmask)
	// Non-repeating random: pick a pool index not yet played in this cycle.
	// Bitmask tracks played entries; when full, it resets automatically.
	uint32 fullMask = (poolSize < 32) ? ((1u << poolSize) - 1u) : 0xFFFFFFFFu;
	if ((_ambientPoolBitmask & fullMask) == fullMask)
		_ambientPoolBitmask = 0;

	uint16 idx = _vm->_rnd->getRandomNumber(0, poolSize - 1);
	while (_ambientPoolBitmask & (1u << idx))
		idx = (idx + 1) % poolSize;
	_ambientPoolBitmask |= (1u << idx);

	uint16 sndId = pool[idx];
	if (sndId == 0)
		return;

	// IDA: ++ambient_wPreloadCounter %= 16 → if 0, preload SND 900-944
	// Preloading is an optimization for CD-ROM access; not needed in ScummVM.
	_ambientPreloadCounter = (_ambientPreloadCounter + 1) % 16;

	// IDA: scrb_enqueueSoundResource(0, sndId) — SND IDs < 1000 route to system archive.
	Audio::SoundHandle *handle = _vm->_sound->playZmbSound(
		ZmbResource(ZmbArchiveKind::kSystem, sndId),
		Audio::Mixer::kSFXSoundType);
	if (handle)
		_ambientSndHandle = *handle;

	_ambientLastSndId = sndId;
}

// ---------------------------------------------------------------------------
// Snoid drag-and-drop infrastructure
// IDA: beginDragFeatureRunner_45360F — universal drag handler.
// ---------------------------------------------------------------------------

const Common::Rect ZoombiniInteractive::kDefaultDragConstraint = Common::Rect(0, 0, 639, 479);

ZmbSnoid *ZoombiniInteractive::findSnoidAtPoint(const Common::Point &pos) {
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *snoid = it->second;
		if (!snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			continue;
		if (snoid->findDrawRecordAtPoint(pos))
			return snoid;
	}
	return nullptr;
}

void ZoombiniInteractive::startSnoidDrag(ZmbSnoid *snoid, const Common::Point &mousePos) {
	_draggedSnoid = snoid;
	_dragOrigPos = snoid->getPointLoc();
	_dragOffset = Common::Point(mousePos.x - _dragOrigPos.x, mousePos.y - _dragOrigPos.y);
	_dragPrevMouseX = mousePos.x;
	// Start directly with right-facing dangling feet animation (skip entry poses).
	// IDA: Entry poses are frames 0-1, looping animation starts at frame 2.
	// Set facing right initially for consistent appearance.
	snoid->setFacingLeft(false);
	snoid->setHoldingAnimPhase(2);
	beginSnoidDrag(snoid);

	// IDA: showNotiBoxMsg_454090 — show snoid name while dragging (all pages)
	if (!snoid->_name.empty())
		showNotiBoxShort(snoid->_name);
}

ZmbSnoid *ZoombiniInteractive::finishSnoidDrag() {
	ZmbSnoid *snoid = _draggedSnoid;
	_draggedSnoid = nullptr;
	endSnoidDrag(snoid);
	hideNotiBoxShort();
	return snoid;
}

const Common::Rect &ZoombiniInteractive::getDragConstraintRect() const {
	return kDefaultDragConstraint;
}

ZmbEventHandleResult ZoombiniInteractive::onMouseMove(const Common::Point &absPos, const Common::Point &relPos) {
	if (!_draggedSnoid)
		return ZoombiniPage::onMouseMove(absPos, relPos);

	// Update snoid position during drag (constrained to drag bounds rect)
	// IDA: beginDragFeatureRunner_45360F inner loop: pos = mouse - offset, clamped to dragBoundsRect
	const Common::Rect &constraint = getDragConstraintRect();
	int16 newX = CLIP<int16>(absPos.x - _dragOffset.x, constraint.left, constraint.right);
	int16 newY = CLIP<int16>(absPos.y - _dragOffset.y, constraint.top, constraint.bottom);
	_draggedSnoid->setPointLoc(Common::Point(newX, newY));

	// Update facing direction based on movement relative to drag origin
	// IDA: if prevMousePos.x > pPos.x → facing left; if prevMousePos.x < pPos.x → facing right
	if (absPos.x < _dragOrigPos.x && !_draggedSnoid->isFacingLeft())
		_draggedSnoid->setFacingLeft(true);
	else if (absPos.x > _dragOrigPos.x && _draggedSnoid->isFacingLeft())
		_draggedSnoid->setFacingLeft(false);

	// Update holding animation frame based on movement direction
	// IDA: Holding animation has 3 frames: 0=middle, 1=left lean, 2=right lean.
	// Frame selection follows mouse movement direction relative to previous position.
	if (_dragPrevMouseX > absPos.x) {
		// Mouse moved left → left lean (frame 1)
		_draggedSnoid->setHoldingFrameIdx(1);
	} else if (_dragPrevMouseX < absPos.x) {
		// Mouse moved right → right lean (frame 2)
		_draggedSnoid->setHoldingFrameIdx(2);
	}
	// If no horizontal movement, keep current frame (don't reset to middle)
	_dragPrevMouseX = absPos.x;

	return ZmbEventHandleResult::kConsumed;
}

void ZoombiniInteractive::layoutStaticAndWalkIn() {
	// IDA: zmb_layoutStaticAndWalkInGroups(0) — paramIdx=0 selects the default position table.
	// First 75% of loaded pack snoids stay idle at their pedestal positions.
	// Last 25% are moved off-screen left (x=-50) and given a walk-in animation.

	// Collect occupied pack snoids in load order (keys 10000, 10001, ...).
	Common::Array<ZmbSnoid *> occupied;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if (it->first >= 10000 && it->second->_packIsOccupied)
			occupied.push_back(it->second);
	}

	// IDA: v8 = 3 * LoadedZmbRunnerCount / 4 — 75% threshold.
	const int16 total = (int16)occupied.size();
	const int16 walkInStart = (3 * total) / 4;

	// Last 25%: start off-screen left, walk to pedestal.
	// IDA: posLoc.x = -50, posLoc.y = target.y, animDestPos = target,
	//       animateZoombini(0, 7, pZmb)  [state 7 = kSnoidAnimDepart].
	for (int16 i = walkInStart; i < total; i++) {
		ZmbSnoid *snoid = occupied[i];
		const Common::Point target = snoid->getPointLoc();
		snoid->setPointLoc(Common::Point(-50, target.y));
		snoid->setAnimTargetPos(target);
		snoid->setAnimState(kSnoidAnimDepart, nullptr);
	}
}

void ZoombiniInteractive::assignStaggeredWalkDelays() {
	// IDA: zmb_assignStaggeredWalkDelays(startX=0, endX=45)
	// Collect snoids that are walk-in animated (kSnoidAnimDepart or kSnoidAnimPath).
	Common::Array<ZmbSnoid *> walkers;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if (it->first < 10000)
			continue;
		ZmbSnoid *snoid = it->second;
		const SnoidAnimState st = snoid->getAnimState();
		if (st == kSnoidAnimDepart || st == kSnoidAnimPath)
			walkers.push_back(snoid);
	}

	if (walkers.empty())
		return;

	// IDA: zmb_insertionSortByYDepth — sort ascending by target Y coordinate.
	Common::sort(walkers.begin(), walkers.end(), [](ZmbSnoid *a, ZmbSnoid *b) {
		return a->getAnimTargetPos().y < b->getAnimTargetPos().y;
	});

	// IDA: iterate from count-1 to 0 so the highest-Y (front-most) snoid starts first,
	// with subsequent snoids staggered 45 frames apart.
	// First walker (highest Y, walkerIdx=0) starts immediately without deactivation.
	const uint32 baseFrame = getCurrentFrameCounter();
	uint16 walkerIdx = 0;
	for (int i = (int)walkers.size() - 1; i >= 0; i--) {
		if (walkerIdx > 0) {
			walkers[i]->deactivateRender();
			walkers[i]->setDelayUntilFrame(baseFrame + walkerIdx * 45);
		}
		walkerIdx++;
	}
}

void ZoombiniInteractive::playActiveHelpSound() {
	// IDA: playOrEnqueueActiveSound_4626DB
	// Replays the help voice sound stored in _activeHelpSoundId when F1 is pressed.
	// If the same sound is already playing, stop and restart it.
	// If not set (resource ID = 0), do nothing.

	if (!_activeHelpSoundId.hasId())
		return;

	// Stop any currently playing help voice (if it's the same resource)
	_vm->_sound->stopZmbSound(_activeHelpSoundId);

	// Play the help voice sound
	_vm->_sound->playZmbSound(_activeHelpSoundId, Audio::Mixer::kSpeechSoundType);
}

} // End of namespace Mohawk
