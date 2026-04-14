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

	// [*] Callback-only runner - Go, Map Buttons
	// IDA: bc1_initAndSetupPuzzle and other pages register a wResId=0 runner
	// (overlay03) with preRender/postRender for proceed/map/help button drawing.
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

	// IDA overlay03: registered with FLAG_00001000_TOPMOST only (no OVERLAY).
	// TOPMOST → normalList tail → rendered last → always on top of all features.
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, _goMapBitmapResId), 0,
					hotspots, 0,
					ZmbFeature::FLAG_00001000_TOPMOST,
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

	// [*] Callback-only runner (tBMP c:0001) - Help Button
	// IDA: Same overlay03 wResId=0 runner handles Help alongside Go/Map.
	ZmbFeature::EventHooks hooksHelpMapButton;
	hooksHelpMapButton.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractive::helpButton_preRenderShape));
	hooksHelpMapButton.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractive::helpButton_onPostRender));
	hooksHelpMapButton.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractive::helpButton_onLButtonDown));

	Common::Array<ZmbHotspot> hotspots;
	hotspots.push_back(ZmbHotspot(kHotspotHelpButtonNormal, kShape0001_24_HelpButtonNormal, 0, _helpButtonRect));
	hotspots.push_back(ZmbHotspot(kHotspotHelpButtonPressed, kShape0001_25_HelpButtonPressed, 0, _helpButtonRect));

	// IDA overlay03: same TOPMOST-only flags as Go/Map buttons.
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap0001_Dialog), 0,
					hotspots, 0,
					ZmbFeature::FLAG_00001000_TOPMOST,
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

void ZoombiniInteractive::debugFinishPuzzle() {
	debugPrepareForDeparture();
	onGoButtonActivated();
}

void ZoombiniInteractive::executeDeparture() {
	// IDA: puzzleDispatch_sharedCleanup → save_updateZmbPacksOnPuzzleComplete(0, 1)
	// Write snoid runners back to active pack and route non-occupied to resting packs.
	// BC1/BC2 override this with their own save+snapshot logic.
	saveSnoidsToPack();
	routeNonOccupiedToRestingPack();

	// IDA: execActivePuzzle_435BE8 — route completion flag setting.
	// When a container puzzle departs, record the route's completion level
	// in the per-route completion flags. Uses adjusted routeLevel when
	// the level was just advanced by routeNonOccupiedToRestingPack().
	if (!_vm->_state->inPracticeMode()) {
		ZmbStateFile &f = _vm->_state->_f;
		int16 routeLevel = _vm->_state->readActivePageRouteLevel();
		if (_vm->_state->_routeLevelJustAdvanced && routeLevel > 0)
			routeLevel--;
		uint8 bitmask = static_cast<uint8>(1 << (routeLevel & 3));

		switch (f._currentPage) {
		case ZMB_DI_PIZZA_09:
			f._levelFlagRouteBigBadHungry |= bitmask;
			break;
		case ZMB_DI_SLIDES_12:
			f._levelFlagLoWhosBayouHiDeepDarkForest |= (bitmask & 0x0F);
			break;
		case ZMB_DI_NET_15:
			f._levelFlagLoWhosBayouHiDeepDarkForest |= static_cast<uint8>(bitmask << 4);
			break;
		case ZMB_DI_MAZE_18:
			f._levelFlagRouteMontDespair |= bitmask;
			break;
		default:
			break;
		}
	}

	if (_departXferSrcSiPage != ZMB_SI_MINUS1) {
		_vm->_xferSrcPage = _departXferSrcSiPage;
		_vm->setNextPage(ZoombiniPageType::kXfer);
	}
	close();
}

void ZoombiniInteractive::saveSnoidsToPack() {
	// IDA: save_updateZmbPacksOnPuzzleComplete(0, 1) — first half.
	// Two-pass: occupied snoids first, then non-occupied.
	// This writes ALL snoid runners back to _zmbPackActive.
	ZmbStateFile &f = _vm->_state->_f;

	// IDA 0x45537B: setZmbMovementDirection_45621A(1) — arrival turn-around.
	_vm->setArrivalTurnDirection(1);

	// IDA 0x4553DE-EC: reset skip-animation flags.
	f._zmbPackActive._bSkipOccupiedAnim = 0;
	f._zmbPackActive._bSkipUnoccupiedAnim = 0;

	// IDA 0x455407-439: re-activate hidden snoids (wBoolDoRender=1)
	// and reset their animation to idle before saving.
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *snoid = *it;
		if (!snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			continue;
		if (!snoid->isRenderActivated()) {
			snoid->activateRender();
		}
	}

	// Two-pass write: occupied first, then non-occupied.
	// IDA 0x4554B8-5CB: for i=0..1, iterate snoid runners.
	// Only real pack snoids (with _packIsOccupied set by loadZoombinisFromPack
	// or picker generation) are written. Animation-pool SCRS features (reject/
	// normal pools) also carry FLAG_00000001_TYPE_SNOID but must NOT be counted;
	// the original engine keeps them in separate arrays.
	int16 destIdx = 0;
	for (int pass = 0; pass < 2 && destIdx < 16; pass++) {
		bool wantOccupied = (pass == 0);
		for (auto it = _snoidMap.begin(); it != _snoidMap.end() && destIdx < 16; ++it) {
			ZmbSnoid *snoid = *it;
			if (!snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
				continue;
			if (snoid->_packIsOccupied != wantOccupied)
				continue;

			ZmbStateActiveEntry &entry = f._zmbPackActive._entries[destIdx];
			entry._traits = snoid->_trait;
			entry._posX = static_cast<uint16>(snoid->getPointLoc().x);
			entry._posY = static_cast<uint16>(snoid->getPointLoc().y);
			entry._bIsOccupied = wantOccupied ? 1 : 0;
			Common::String nameBytes = _vm->_text->fromU32String(snoid->_name);
			memset(entry._name, 0, sizeof(entry._name));
			uint32 nameLen = MIN<uint32>(nameBytes.size(), sizeof(entry._name));
			memcpy(entry._name, nameBytes.c_str(), nameLen);

			destIdx++;
		}
	}

	// IDA 0x455483: g_pGameState->zmbPackActive.wPackZmbCount = destIdx
	// Set count to the number of entries actually written, not the total
	// TYPE_SNOID feature count (which includes animation-pool SCRS features).
	f._zmbPackActive._wPackZmbCount = destIdx;
}

void ZoombiniInteractive::routeNonOccupiedToRestingPack() {
	// IDA: second half of save_updateZmbPacksOnPuzzleComplete (0x45536B).
	// Handles per-puzzle level flags, non-occupied snoid routing,
	// perfect streak tracking, and route level advancement.
	ZmbStateFile &f = _vm->_state->_f;
	ZMB_DI_PAGE currentPage = f._currentPage;

	// Only puzzle pages (DI 7-18) participate in route tracking.
	if (currentPage < ZMB_DI_BRIDGE_07 || currentPage > ZMB_DI_MAZE_18)
		return;

	// IDA: routeIdx = (wActivePuzzleId - 7) / 3 + 1 (1-4)
	uint16 routeIdx = static_cast<uint16>((currentPage - ZMB_DI_BRIDGE_07) / 3 + 1);

	bool isContainer = (currentPage == ZMB_DI_PIZZA_09 || currentPage == ZMB_DI_SLIDES_12 ||
	                    currentPage == ZMB_DI_NET_15 || currentPage == ZMB_DI_MAZE_18);

	// IDA 0x455632-690: Set per-puzzle level flag (low nibble).
	// pbPuzzleLevelFlagArr[puzzleFlagOffset + 3] |= (1 << routeLevel)
	// In ScummVM: _levelFlagPageArr[currentPage - 7] |= (1 << routeLevel)
	uint16 routeLevel = f._routeLevels[routeIdx - 1];
	uint8 levelBitmask = static_cast<uint8>(1 << (routeLevel & 3));
	f._levelFlagPageArr[currentPage - ZMB_DI_BRIDGE_07] |= levelBitmask;

	// Count non-occupied snoids in active pack.
	int16 nonOccupiedCount = 0;
	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount; i++) {
		if (!f._zmbPackActive._entries[i]._bIsOccupied)
			nonOccupiedCount++;
	}

	_vm->_state->_routeLevelJustAdvanced = false;

	if (nonOccupiedCount > 0) {
		// ---------------------------------------------------------------
		// Snoids were lost: clear perfect streak and route non-occupied
		// snoids to resting packs (container puzzles only).
		// IDA: 0x45569A-8F9
		// ---------------------------------------------------------------
		_vm->_state->_perfectStreakFlag = false;

		if (isContainer) {
			// Determine destination pack based on route.
			// IDA: Route 1 → BC0 (Isle), Route 2-3 → BC1, Route 4 → BC2.
			ZmbStateActivePack *destPack = nullptr;
			switch (routeIdx) {
			case 1:
				destPack = &f._zmbPackIsle;
				break;
			case 2:
			case 3:
				destPack = &f._zmbPackBC1;
				f._zmbStoredBC1Count += nonOccupiedCount;
				break;
			case 4:
				destPack = &f._zmbPackBC2;
				f._zmbStoredBC2Count += nonOccupiedCount;
				break;
			default:
				return;
			}

			// IDA 0x4556FC-755: compact out occupied entries from dest pack.
			if (destPack->_bSkipOccupiedAnim) {
				for (int16 k = 0; k < destPack->_wPackZmbCount; k++) {
					if (destPack->_entries[k]._bIsOccupied) {
						--destPack->_wPackZmbCount;
						for (int16 m = k; m < destPack->_wPackZmbCount; m++) {
							destPack->_entries[m] = destPack->_entries[m + 1];
						}
						--k;
					}
				}
			}
			destPack->_bSkipUnoccupiedAnim = 0;
			destPack->_bSkipOccupiedAnim = 0;

			// Find first non-occupied entry in active pack.
			int16 srcIdx = 0;
			while (srcIdx < f._zmbPackActive._wPackZmbCount && f._zmbPackActive._entries[srcIdx]._bIsOccupied)
				srcIdx++;

			// Find insertion point in dest pack.
			int16 destIdx = -1;
			for (int16 ii = 0; destIdx < 0 && ii <= destPack->_wPackZmbCount; ii++) {
				if (!destPack->_entries[ii]._traits.isComplete() || ii == destPack->_wPackZmbCount)
					destIdx = ii;
			}
			if (destIdx < 0) {
				destPack->_wPackZmbCount = 0;
				destIdx = 0;
			}

			// Copy non-occupied entries from active → dest pack.
			for (int16 j = 0; j < nonOccupiedCount; j++) {
				if (destPack->_wPackZmbCount >= 16 || destIdx >= 16)
					break;

				ZmbStateActiveEntry &dst = destPack->_entries[destIdx];
				ZmbStateActiveEntry &src = f._zmbPackActive._entries[srcIdx];
				dst._traits = src._traits;
				memcpy(dst._name, src._name, sizeof(dst._name));
				dst._bIsOccupied = 1;
				destIdx++;
				srcIdx++;
				destPack->_wPackZmbCount++;
			}

			// Update Val field for dest pack snapshot.
			switch (routeIdx) {
			case 1:
				f._wZmbPackIsleVal = f._wZmbPackActiveVal;
				break;
			case 2:
			case 3:
				f._wZmbPackBC1Val = f._wZmbPackActiveVal;
				break;
			case 4:
				f._wZmbPackBC2Val = f._wZmbPackActiveVal;
				break;
			default:
				break;
			}

			// Remove non-occupied entries from active pack.
			f._zmbPackActive._wPackZmbCount -= nonOccupiedCount;
		}
	} else if (_vm->_state->_perfectStreakFlag && isContainer) {
		// ---------------------------------------------------------------
		// All snoids survived AND perfect streak intact AND container puzzle.
		// IDA: 0x45591D-AEA — perfect completion path.
		// ---------------------------------------------------------------

		// IDA 0x455938-97D: Set per-puzzle level flag high nibble (perfect completion).
		f._levelFlagPageArr[currentPage - ZMB_DI_BRIDGE_07] |= static_cast<uint8>(levelBitmask << 4);

		// IDA 0x455A8D-AEA: Route level advancement.
		// Increment per-route perfect completion counter.
		// Three perfect completions → reset counter and advance route level.
		if (static_cast<int16>(routeLevel) < 3) {
			f._routePerfectCounters[routeIdx - 1]++;
			if (f._routePerfectCounters[routeIdx - 1] >= 3) {
				f._routePerfectCounters[routeIdx - 1] = 0;
				f._routeLevels[routeIdx - 1]++;
				_vm->_state->_routeLevelJustAdvanced = true;
				debugC(1, kZmbDebugAnimation, "Route %d level advanced to %d",
					routeIdx, f._routeLevels[routeIdx - 1]);
			}
		}
	}
}

void ZoombiniInteractive::startDepartWalkAnimation(const Common::Point &target, uint32 stagger) {
	// IDA: zmbMoveAnimation_45479D(staggerDelay, toY, toX)
	// 1. zmb_insertionSortByYDepth(0) — sorts snoids by animDestPos.x ascending.
	//    Only occupied snoids (unk00F7 != 0) are included in the sorted array.
	// 2. Iterates from count-1 to 0 (highest animDestPos.x first = rightmost/
	//    front-most snoid departs first).
	// 3. For each occupied idle snoid: copies posLoc→pos2, overwrites animDestPos
	//    with (toX, toY), sets animateZoombini state 10, staggers dNextRenderFrame.
	// IDA: only sets dNextRenderFrame for timing — does NOT touch wBoolDoRender.
	// IDA: resets ui_bDragLockActive = 0 at the start.
	_vm->_walkersInProgress = 0;

	// Collect eligible snoids: must be TYPE_SNOID, occupied, and idle.
	// IDA: zmb_insertionSortByYDepth filters by (bitmask & 1) and unk00F7,
	//      zmbMoveAnimation re-checks unk00F7 and snoidAnimateState == 0.
	Common::Array<ZmbSnoid *> walkers;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *snoid = *it;
		if (!snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			continue;
		if (!snoid->_packIsOccupied)
			continue;
		if (snoid->getAnimState() != kSnoidAnimIdle)
			continue;
		walkers.push_back(snoid);
	}

	if (walkers.empty())
		return;

	// IDA: zmb_insertionSortByYDepth — sort ascending by animDestPos.x.
	// animDestPos is set to posLoc at load time (zmb_loadAnimationsFromActivePack)
	// and updated on pedestal drag-drop. The X coordinate determines the
	// departure stagger order (rightmost/highest-X departs first).
	Common::sort(walkers.begin(), walkers.end(), [](ZmbSnoid *a, ZmbSnoid *b) {
		return a->getAnimTargetPos().x < b->getAnimTargetPos().x;
	});

	// IDA: iterate from count-1 to 0 — highest animDestPos.x (rightmost) departs first.
	uint32 frameBase = getCurrentFrameCounter();
	uint16 walkerIdx = 0;
	for (int i = (int)walkers.size() - 1; i >= 0; i--) {
		ZmbSnoid *snoid = walkers[i];
		snoid->setAnimTargetPos(target);
		snoid->setAnimState(kSnoidAnimArrivalMotion, nullptr);

		if (walkerIdx > 0) {
			snoid->setDelayUntilFrame(frameBase + walkerIdx * stagger);
		}
		walkerIdx++;
	}
}

bool ZoombiniInteractive::isDepartWalkComplete() const {
	// IDA: departure polling — all snoids idle or off-screen right edge.
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if (!(*it)->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			continue;
		const ZmbSnoid *snoid = *it;
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
	// IDA: Every page's cleanup calls puzzleDispatch_sharedCleanup() →
	// save_updateZmbPacksOnPuzzleComplete(0, 1) to write snoid runners
	// back to the active pack. Replicate that here.
	saveSnoidsToPack();

	// Copy active pack to the resting pack for the current route so snoids
	// are preserved when the player returns via rodmap.
	// Route 1 (Bridge/Tunnels/Pizza) → BC0 (Isle/Picker)
	// Route 2 (Ferry/Lilly/Slides) → BC1
	// Route 3 (Fleens/Hotel/Net) → BC1
	// Route 4 (Caves/Smoke/Maze) → BC2
	ZmbStateFile &f = _vm->_state->_f;
	ZMB_DI_PAGE currentPage = f._currentPage;
	if (currentPage >= ZMB_DI_BRIDGE_07 && currentPage <= ZMB_DI_PIZZA_09) {
		// Route 1 → BC0
		f._zmbPackActive.copyTo(f._zmbPackIsle);
		f._wZmbPackIsleVal = f._wZmbPackActiveVal;
		f._zmbPackActive._wPackZmbCount = 0;
	} else if (currentPage >= ZMB_DI_FERRY_10 && currentPage <= ZMB_DI_NET_15) {
		// Route 2/3 → BC1
		f._zmbPackActive.copyTo(f._zmbPackBC1);
		f._wZmbPackBC1Val = f._wZmbPackActiveVal;
		f._zmbPackActive._wPackZmbCount = 0;
	} else if (currentPage >= ZMB_DI_CAVES_16 && currentPage <= ZMB_DI_MAZE_18) {
		// Route 4 → BC2
		f._zmbPackActive.copyTo(f._zmbPackBC2);
		f._wZmbPackBC2Val = f._wZmbPackActiveVal;
		f._zmbPackActive._wPackZmbCount = 0;
	}

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
	if (!_notiBoxFeature) {
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractive::notiBox_preRenderShape));
		hooks.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractive::notiBox_onPostRender));

		Common::Array<ZmbHotspot> hotspots;
		hotspots.push_back(ZmbHotspot(kHotspotNotiBoxShort, kShape3001_01_NotiBoxShort, 0, _notiBoxShortRect));
		hotspots.push_back(ZmbHotspot(kHotspotNotiBoxLong, kShape3001_02_NotiBoxLong, 0, _notiBoxLongRect));

		_notiBoxFeature = loadScrbFeature(ZmbResource(ZmbArchiveKind::kSystem, kResShapeBitmap3001_NotiBox), 0,
						hotspots, 0,
						ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						hooks);
		// IDA runner_preRenderStandard LABEL_70 (0x4620F5): the original
		// computes clickRect from shape metadata in preRender so the
		// notibox area is dirty on its very first frame.  Because we
		// compute sortRect from drawn bounds (which may be clipped on
		// the first frame), explicitly add the notibox rect as an
		// external dirty rect to break the first-frame deadlock.
		addExternalDirtyRect(isNotiBoxLong ? _notiBoxLongRect : _notiBoxShortRect);
	}
}

void ZoombiniInteractive::notiBox_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	if (_notiBoxShowUntilFrame < _currentFrameCounter) {
		feature->scheduleClose();
		_notiBoxFeature = nullptr;
	}

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
		ZmbSnoid *snoid = *it;
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
	_dragHighlightSlot = -1;

	// IDA: beginDragFeatureRunner_45360F 0x4537C4–0x453811
	// Check if snoid is being picked up FROM a draw-on-reg slot.
	// If so, clear that slot's occupancy.
	_dragSourceSlot = findDrawOnRegSlotByOccupant(snoid->getId());
	if (_dragSourceSlot >= 0) {
		clearDrawOnRegOccupant(_dragSourceSlot);
	}

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
	clearDrawOnRegHighlight();
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

	// IDA: beginDragFeatureRunner_45360F 0x4539BF — update seat highlighting
	updateDrawOnRegHighlight();

	return ZmbEventHandleResult::kConsumed;
}

// ---------------------------------------------------------------------------
// updateDrawOnRegHighlight: Highlight/unhighlight seat runners during drag.
// IDA: beginDragFeatureRunner_45360F 0x4539BF–0x453B51
// ---------------------------------------------------------------------------
void ZoombiniInteractive::updateDrawOnRegHighlight() {
	if (_drawOnRegCount <= 0 || !_draggedSnoid)
		return;

	Common::Point snoidPos = _draggedSnoid->getPointLoc();
	int16 hitSlot = hitTestDrawOnRegSlot(snoidPos, _clickZoneRadius, true);

	if (hitSlot >= 0) {
		// Found an empty slot within zone
		uint16 seatRunnerId = _drawOnRegRunnerIds[hitSlot];
		if (hitSlot != _dragHighlightSlot) {
			// Changed slot — unhighlight old, highlight new
			clearDrawOnRegHighlight();
			_dragHighlightSlot = hitSlot;

			// IDA: 0x453ACB–0x453AE3: Enable render, set highlight frame
			// wBoolDoRender=1, wGroupFrameIdx0098=0, dwHotspotIdx009A=1
			ZmbFeature *seatRunner = _scrbFeatures.find(seatRunnerId);
			if (seatRunner) {
				seatRunner->activateRender();
				seatRunner->setLastFrameIdx(0);
				seatRunner->setNeedsRedraw(true);
			}
		}
	} else if (_dragHighlightSlot >= 0) {
		// Left all slots — unhighlight
		clearDrawOnRegHighlight();
	}
}

void ZoombiniInteractive::clearDrawOnRegHighlight() {
	if (_dragHighlightSlot < 0)
		return;

	// IDA: 0x453AA5–0x453AB4: Unhighlight — set SKIP_ONCE flag and trigger render
	uint16 seatRunnerId = _drawOnRegRunnerIds[_dragHighlightSlot];
	ZmbFeature *seatRunner = _scrbFeatures.find(seatRunnerId);
	if (seatRunner && seatRunner->isRenderActivated()) {
		seatRunner->addFlag(ZmbFeature::FLAG_00010000_SKIP_ONCE);
		seatRunner->resetNextRenderFrame();
	}
	_dragHighlightSlot = -1;
}

void ZoombiniInteractive::layoutStaticAndWalkIn() {
	// IDA: zmb_layoutStaticAndWalkInGroups(0) — paramIdx=0 selects the default position table.
	// First 75% of loaded pack snoids stay idle at their pedestal positions.
	// Last 25% are moved off-screen left (x=-50) and given a walk-in animation.

	// Collect occupied pack snoids in load order (keys 10000, 10001, ...).
	Common::Array<ZmbSnoid *> occupied;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if ((*it)->getId() >= 10000 && (*it)->_packIsOccupied)
			occupied.push_back(*it);
	}

	// IDA: v8 = 3 * LoadedZmbRunnerCount / 4 — 75% threshold.
	const int16 total = (int16)occupied.size();
	const int16 walkInStart = (3 * total) / 4;

	// IDA: zmb_layoutStaticAndWalkInGroups 0x4544E4–0x454576
	// Static snoids (first 75%): match each snoid's position to draw-on-reg
	// slots. If a slot's snap position falls within ±clickZoneRadius of the
	// snoid's position, mark that slot as occupied by this snoid.
	for (int16 i = 0; i < walkInStart && i < total; i++) {
		ZmbSnoid *snoid = occupied[i];
		const Common::Point sPos = snoid->getPointLoc();
		int16 slotIdx = hitTestDrawOnRegSlot(sPos, _clickZoneRadius, true);
		if (slotIdx >= 0) {
			setDrawOnRegOccupant(slotIdx, snoid->getId());
		}
	}

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
		if ((*it)->getId() < 10000)
			continue;
		ZmbSnoid *snoid = *it;
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
	// IDA: picker_assignWalkStartPositions_4545EE only sets dNextRenderFrame
	// (timing gate for the pre-render callback onRender_ZoombiniAnimation_452B9C).
	// wBoolDoRender is NOT touched — the post-render callback
	// (onPostRender_ZoombiniAnimation_452ADD) still draws the snoid at its
	// current position while the animation delay has not expired.
	const uint32 baseFrame = getCurrentFrameCounter();
	uint16 walkerIdx = 0;
	for (int i = (int)walkers.size() - 1; i >= 0; i--) {
		if (walkerIdx > 0) {
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
