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
#include "mohawk/zoombini_pages/interactive_rodmap.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"
#include "mohawk/zoombini_text.h"
#include "interactive_rodmap.h"

namespace Mohawk {

ZoombiniInteractiveRodMap::ZoombiniInteractiveRodMap(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kRodMap) {
	// Clickable Pos & Rect
	for (uint32 i = 0; i < ARRAYSIZE(_pageClickPoints); i++) {
		_pageClickRects[i] = Common::Rect(_pageClickPoints[i].x - 20, _pageClickPoints[i].y - 15, _pageClickPoints[i].x + 20, _pageClickPoints[i].y + 15);
	}

	buildPageRouteLevelMap();
}

ZoombiniInteractiveRodMap::~ZoombiniInteractiveRodMap() {
}

void ZoombiniInteractiveRodMap::open() {
	openArchive(ZMB_MHK_RODMAP);
}

void ZoombiniInteractiveRodMap::setBackgroundBitmap() {
	_vm->_gfx->setPalette(kResBackground300);
	_vm->_gfx->drawBackground(kResBackground300);
}

void ZoombiniInteractiveRodMap::loadFeatures() {
	// IDA: rodmap_exitAndRestoreScreen @ 0x42B531 — when re-entering the map
	// after a practice puzzle, restore the user's snapshotted state. Practice
	// mode (_practiceLevel, kept outside _f) stays on so the user can launch
	// another practice puzzle without re-toggling.
	if (_vm->_state->_practiceStateBackupActive) {
		_vm->_state->_f = _vm->_state->_practiceStateBackup;
		_vm->_state->_practiceStateBackupActive = false;
	}

	_vm->_gfx->preloadImage(kResBitmapShape1000);

	// [*] SCRB 1000: Page Icon
	ZmbFeature::EventHooks hooks1000;
	hooks1000.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractiveRodMap::patchPageShape1000_preRenderShape));
	hooks1000.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractiveRodMap::runPage1000_onLButtonDown));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1000), kResScrbPageIcon1000, 6,
					ZmbFeature::FLAG_08000000_REGION_TRACK | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00008000_LOOP_ANIM,
					hooks1000);
	// [*] SCRB 1001: Route Shapes
	// + Draw Route Name Text
	ZmbFeature::EventHooks hooks1001;
	hooks1001.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractiveRodMap::patchRouteShape1001_preRenderShape));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1000), kResScrbRoute1001, 6,
					ZmbFeature::FLAG_08000000_REGION_TRACK | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00008000_LOOP_ANIM,
					hooks1001);

	// [*] SCRB 1005: Only appears after hovering one of the puzzle
	ZmbFeature::EventHooks hooks1005;
	hooks1005.setPreRenderFunc(reinterpret_cast<ZmbFeature::OnPreRenderFunc>(&ZoombiniInteractiveRodMap::drawAfterPageIconHover1005_preRender));
	hooks1005.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniInteractiveRodMap::renderAfterPageIconHover1005));
	hooks1005.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveRodMap::textPageName1005_postRender));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1000), kResScrbPageNameHover1005, 6,
					ZmbFeature::FLAG_00100000_PLAY_ONCE,
					hooks1005);
	// [*] SCRB 1006: Option Button
	ZmbFeature::EventHooks hooks1006;
	hooks1006.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractiveRodMap::optionButton1006_preRenderShape));
	hooks1006.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveRodMap::optionButton1006_postRender));
	hooks1006.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractiveRodMap::optionButton1006_onLButtonDown));
	_optionButtonFeature = loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1000), kResScrbMenuButton1006, 3,
					ZmbFeature::FLAG_00001000_TOPMOST,
					hooks1006);

	// [*] SCRB 1004: Level Legend
	// Only clickable in practice mode
	ZmbFeature::EventHooks hooks1004;
	hooks1004.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractiveRodMap::patchSelectedLevelShape1004_preRenderShape));
	hooks1004.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveRodMap::textLegend1004_postRender));
	hooks1004.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractiveRodMap::legendLevel1004_onLButtonDown));
	hooks1004.setKeyDownFunc(reinterpret_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniInteractiveRodMap::legendLevel1004_onKeyDown));
	int modeFrameVal = _vm->_state->inPracticeMode() ? 6 : 0;
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1000), kResScrbLevelLegend1004, modeFrameVal,
					ZmbFeature::FLAG_00100000_PLAY_ONCE,
					hooks1004);

	// [*] SCRB 1002: Status (shape & text) / Mode Select Combobox (text only)
	ZmbFeature::EventHooks hooks1002;
	hooks1002.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveRodMap::textJourneyStat1002_postRender));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1000), kResScrbStatusRect1002, 0,
					ZmbFeature::FLAG_00100000_PLAY_ONCE,
					hooks1002);

	// [*] SCRB 1003: Mode Select Combobox (red circle shape)
	// IDA registers SCRB 1003 after 1002 with PLAY_ONCE only and relies on the
	// linked-list natural order. ScummVM's buildSortedRenderList re-sorts by
	// bottom-edge, which places 1002 (tall status panel) after 1003 (short
	// combobox), making 1002 draw on top. Per KB
	// AGENTS/Z1-MEMORY/InteractivePages/rodmap-navigation.md §"SCRB 1003 Z-Ordering Note",
	// FLAG_00001000_TOPMOST forces 1003 to the tail of the sorted render list.
	ZmbFeature::EventHooks hooks1003;
	hooks1003.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractiveRodMap::drawComboBox1003_preRenderShape));
	hooks1003.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractiveRodMap::selectMode1003_onLButtonDown));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1000), kResScrbModeCombobox1003, 0,
					ZmbFeature::FLAG_00001000_TOPMOST | ZmbFeature::FLAG_00100000_PLAY_ONCE,
					hooks1003);

	// [*] Callback-only runner: Route Names
	// IDA: rodmap_drawRouteNames is called DIRECTLY from
	// rodmap_runScrbPanels_1002_1003_1004 (not via a runner callback).
	// In the original engine route names are drawn once to the background port.
	// We approximate this with a scrbId=0 runner whose postRender draws labels.
	ZmbFeature::EventHooks hooksRouteNames;
	hooksRouteNames.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveRodMap::textRouteNames_postRender));
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, 0), 0, 0, ZmbFeature::FLAG_00100000_PLAY_ONCE, hooksRouteNames);

	// IDA rodmap_setupScreen @ 0x42A99A/0x42A9A9: snd_playOrLoadResSND('SND', 998)
	// + snd_playOrLoadResSND('SND', 999) — preload menu UI SFX. ScummVM's
	// resource layer auto-caches sound resources on first access, so
	// explicit preload is unnecessary for correctness; the first playZmbSound
	// call (line 195/486/534) loads and plays without perceptible latency.
}

ZmbEventHandleResult ZoombiniInteractiveRodMap::onMouseMove(const Common::Point &absPos, const Common::Point &relPos) {
	ZoombiniInteractive::onMouseMove(absPos, relPos);
	optionButton1006_updateTlcHover(absPos);

	uint32 hoveredPageIdx = UINT32_MAX;
	for (uint32 i = 0; i < ARRAYSIZE(_pageClickRects); i++) {
		if (!_pageClickRects[i].contains(absPos))
			continue;
		hoveredPageIdx = i;
		break;
	}

	// Do not clear even when hovering on non-clickable area, the last hovered page must persist.
	if (hoveredPageIdx != UINT32_MAX) {
		_lastHoveredPageType = _pageClickTypes[hoveredPageIdx];
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniInteractiveRodMap::onKeyDown(const Common::KeyState &kbd, bool kbdRepeat) {
	if (kbdRepeat)
		return ZmbEventHandleResult::kConsumed;

	ZmbEventHandleResult result = ZmbEventHandleResult::kConsumed;
	if ((kbd.flags & Common::KBD_CTRL) != 0) {
		switch (kbd.keycode) {
		case Common::KEYCODE_p: // Practice Mode Toggle
			togglePracticeMode();
			break;
		default:
			result = ZmbEventHandleResult::kPassthrough;
			break;
		}
	} else {
		result = ZmbEventHandleResult::kPassthrough;
	}
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	return ZoombiniInteractive::onKeyDown(kbd, kbdRepeat);
}

void ZoombiniInteractiveRodMap::setPracticeMode(bool setPracticeMode) {
	if ((setPracticeMode && _vm->_state->_practiceLevel == 0) || (!setPracticeMode && _vm->_state->_practiceLevel != 0))
		togglePracticeMode();
}

void ZoombiniInteractiveRodMap::togglePracticeMode() {
	if (_vm->_state->_practiceLevel == 0)
		_vm->_state->_practiceLevel = 1;
	else
		_vm->_state->_practiceLevel = 0;

	buildPageRouteLevelMap();

	// IDA 0x42BEF4: The original's postRender clears bHasClickRect each frame,
	// so the SCRB 1005 tooltip only persists while the hover handler re-sets it.
	// Resetting _lastHoveredPageType ensures our renderFunc gate (which replaces
	// the bHasClickRect check) hides the tooltip after the mode change.
	_lastHoveredPageType = ZoombiniPageType::kNone;

	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, kResSound0999_ButtonSFX), Audio::Mixer::kSFXSoundType);
}

void ZoombiniInteractiveRodMap::generatePracticePack() {
	// IDA: puzzleRodMap_maybeOnClickPuzzleIcon_42A9D6 — practice mode Zoombini generation.
	// Always 16 snoids: each of the 4 traits (head/eye/nose/foot) gets an independent
	// random value 1–5 (IDA: nextRand_410705(5, 1)).
	//
	// Snapshot the user's active state BEFORE clobbering the pack so we can
	// restore it on return to the rodmap (IDA: ZBtemp save+swap mechanism;
	// equivalent in-memory snapshot here). Only snapshot once per practice
	// session, mirroring IDA's g_bPracticeModeInited gating.
	if (!_vm->_state->_practiceStateBackupActive) {
		_vm->_state->_practiceStateBackup = _vm->_state->_f;
		_vm->_state->_practiceStateBackupActive = true;
	}
	_vm->_state->generateRandomPack();
}

void ZoombiniInteractiveRodMap::patchPageShape1000_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	// Hotspot: shape 1 ~ 16

	Common::HashMap<uint16, uint16> pageShapeIdMap;
	pageShapeIdMap[kResShapePicker16] = kResShapePicker16;

	// Hide the page icon if the page was never visited
	for (uint16 i = kResShapeBridge01; i <= kResShapeTown15; i++) {
		uint16 shapeId = ZmbHotspot::kShapeNone;
		if (0 < _pageRouteLevelMap[i + kResShapeRouteBigBadHungryP0_17])
			shapeId = i;
		pageShapeIdMap[i] = shapeId;
	}

	// Patch shapeId of the last hovered page icon
	if (_lastHoveredPageType != ZoombiniPageType::kNone) {
		uint16 shapeId = 0;
		for (uint16 i = 0; i < ARRAYSIZE(_pageClickTypes); i++) {
			if (_lastHoveredPageType == _pageClickTypes[i]) {
				shapeId = _pageClickShapes[i];
				break;
			}
		}

		if (_vm->_state->inPracticeMode()) {
			switch (shapeId) {
			case kResShapeBridge01:
			case kResShapeTunnels02:
			case kResShapePizza03:
			case kResShapeFerry05:
			case kResShapeLilly06:
			case kResShapeSlides07:
			case kResShapeFleens08:
			case kResShapeHotel09:
			case kResShapeNet10:
			case kResShapeCaves12:
			case kResShapeSmoke13:
			case kResShapeMaze14:
				if (pageShapeIdMap[shapeId] != ZmbHotspot::kShapeNone)
					pageShapeIdMap[shapeId] += 93;
				break;
			default:
				break;
			}
		} else {
			switch (shapeId) {
			case kResShapePicker16:
			case kResShapeBcOne04:
			case kResShapeBcTwo11:
			case kResShapeTown15:
				if (pageShapeIdMap[shapeId] != ZmbHotspot::kShapeNone)
					pageShapeIdMap[shapeId] += 93;
				break;
			default:
				break;
			}
		}
	}

	// Apply the patched shapeIds
	for (ZmbHotspot &hs : hotspots) {
		hs._shapeIdx = pageShapeIdMap[hs._shapeIdx];
	}
}

ZmbEventHandleResult ZoombiniInteractiveRodMap::runPage1000_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	for (uint32 i = 0; i < ARRAYSIZE(_pageClickRects); i++) {
		if (!_pageClickRects[i].contains(absPos))
			continue;

		ZoombiniPageType nextType = _pageClickTypes[i];

		if (_vm->_state->inPracticeMode()) {
			switch (nextType) {
			case ZoombiniPageType::kBridge:
			case ZoombiniPageType::kCaves:
			case ZoombiniPageType::kPizza:
			case ZoombiniPageType::kFerry:
			case ZoombiniPageType::kLilly:
			case ZoombiniPageType::kSlides:
			case ZoombiniPageType::kFleens:
			case ZoombiniPageType::kHotel:
			case ZoombiniPageType::kNet:
			case ZoombiniPageType::kTunnels:
			case ZoombiniPageType::kSmoke:
			case ZoombiniPageType::kMaze:
				generatePracticePack();
				_vm->setNextPage(nextType);
				close();
				return ZmbEventHandleResult::kConsumed;
			default:
				break;
			}
		} else {
			switch (nextType) {
			case ZoombiniPageType::kPicker:
			case ZoombiniPageType::kBasecamp1:
			case ZoombiniPageType::kBasecamp2:
			case ZoombiniPageType::kTown:
				_vm->setNextPage(nextType);
				close();
				return ZmbEventHandleResult::kConsumed;
			default:
				break;
			}
		}
	}

	return ZmbEventHandleResult::kPassthrough;
}

void ZoombiniInteractiveRodMap::patchRouteShape1001_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	// Level 1: shape 17 ~ 32 (Original Hotspot shapeid is in this range)
	// Level 2: shape 33 ~ 47
	// Level 3: shape 48 ~ 63
	// Level 4: shape 64 ~ 79

	if (_vm->_state->inPracticeMode()) {
		uint16 addShapeNum = (_vm->_state->_practiceLevel - 1) * 16;
		assert(0 <= addShapeNum && addShapeNum < 64);
		for (ZmbHotspot &hs : hotspots) {
			hs._shapeIdx += addShapeNum;
		}
		return;
	}

	for (uint16 i = kResShapeRouteBigBadHungryP0_17; i <= kResShapeRouteMontDespairP3_32; i++) {
		ZmbHotspot &hs = hotspots[i - kResShapeRouteBigBadHungryP0_17];

		// Hide the page icon if the page was never visited
		if (_pageRouteLevelMap[i] == 0) {
			hs._shapeIdx = ZmbHotspot::kShapeNone;
			continue;
		}
		// Determine the shapeId based on the route level
		hs._shapeIdx = i + (_pageRouteLevelMap[i] - 1) * 16;
	}
}

bool ZoombiniInteractiveRodMap::drawAfterPageIconHover1005_preRender(ZmbFeature *feature) {
	// Only drawn if a pageIcon (SCRB 1000) has been hovered.
	return _lastHoveredPageType != ZoombiniPageType::kNone;
}

ZmbRenderResult ZoombiniInteractiveRodMap::renderAfterPageIconHover1005(ZmbFeature *feature) {
	// IDA 0x42BEF4: The original's postRender callback gates both shape blitting
	// and text drawing behind bHasClickRect (set by hover handler, cleared each frame).
	// In ScummVM's split architecture, this custom renderFunc replicates that gate
	// so blitShapes only runs when a page icon has been hovered.
	if (_lastHoveredPageType == ZoombiniPageType::kNone)
		return ZmbRenderResult::kSkipped;
	return blitShapes(feature);
}

void ZoombiniInteractiveRodMap::textPageName1005_postRender(ZmbFeature *feature) {
	if (_lastHoveredPageType == ZoombiniPageType::kNone)
		return;
	
	const Common::U32String &pageName = _vm->_text->getPageName(_lastHoveredPageType);

	ZmbDrawRecord *record = feature->getDrawRecord(0, 0);
	Common::Rect textRect = record->_drawnRect;
	textRect.top += 2;
	textRect.bottom += 2;

	ZoombiniGraphics::TextConf tc;
	tc._textPalette = ZoombiniGraphics::kColor2D_Black;
	tc._wordWrap = true;
	tc._hAlign = Graphics::kTextAlignCenter;
	tc._vAlign = Graphics::kTextAlignCenter;
	_vm->_gfx->drawText(ZoombiniGraphics::kShapeScreen, pageName, textRect, tc);
}

void ZoombiniInteractiveRodMap::optionButton1006_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	if (_optionButtonState.isAnimating()) {
		uint32 elapsedFrames = _currentFrameCounter - _optionButtonState._animationStartFrame;
		if (elapsedFrames < _optionButtonState._animationFrameCount) {
			// Animate by changing hotspot shape id
			if (elapsedFrames < feature->getFrameInterval() - 1) {
				ZmbHotspot &hs = hotspots[0];
				hs._shapeIdx -= 1;
				hs._x += 1;
				hs._y += 2;
			}
		} else {
			_optionButtonState._animationStartFrame = 0;
			_optionButtonState._firePostAnimationEvent = true;
		}
	}
}

void ZoombiniInteractiveRodMap::optionButton1006_postRender(ZmbFeature *feature) {
	if (!_optionButtonState._drawEnabled)
		return;

	optionButton1006_renderTlcLabel();

	if (!_optionButtonState._firePostAnimationEvent)
		return;
	_optionButtonState._firePostAnimationEvent = false;

	_vm->openOptionsDialog();
}

void ZoombiniInteractiveRodMap::optionButton1006_renderTlcLabel() {
	// Z1-20U/TLC v2.0 release only: the rodmap button has a literal
	// lowercase "options" label over the bitmap.
	if (!_vm->isGameVariant(GF_ZMB_TLC))
		return;

	ZoombiniGraphics::TextConf tc;
	tc._textPalette = ZoombiniGraphics::kColor2D_Black;
	tc._hAlign = Graphics::kTextAlignCenter;
	tc._vAlign = Graphics::kTextAlignCenter;
	_vm->_gfx->drawText(ZoombiniGraphics::kShapeScreen, Common::U32String("options", Common::kUtf8), _tlcOptionButtonTextRect, tc);
}

void ZoombiniInteractiveRodMap::optionButton1006_updateTlcHover(const Common::Point &absPos) {
	// Z1-20U/TLC v2.0 release only: hover reloads SCRB 1007 onto the
	// existing options-button runner, then restores SCRB 1006 on leave.
	if (!_vm->isGameVariant(GF_ZMB_TLC) || !_optionButtonFeature)
		return;

	const bool hovered = _optionButtonFeature->findDrawRecordAtPoint(absPos) != nullptr;
	if (_optionButtonTlcHovered == hovered)
		return;

	const Common::Rect dirtyRect = _optionButtonFeature->getZSortRect();
	_optionButtonTlcHovered = hovered;
	loadScrbOntoFeature(_optionButtonFeature, hovered ? kResScrbMenuButtonHover1007 : kResScrbMenuButton1006);
	if (!dirtyRect.isEmpty())
		addExternalDirtyRect(dirtyRect);
}

ZmbEventHandleResult ZoombiniInteractiveRodMap::optionButton1006_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	ZmbDrawRecord *drawRecord = feature->findDrawRecordAtPoint(absPos);
	if (!drawRecord)
		return ZmbEventHandleResult::kPassthrough;

	_optionButtonState.press(_vm, _currentFrameCounter);
	return ZmbEventHandleResult::kConsumed;
}

void ZoombiniInteractiveRodMap::patchSelectedLevelShape1004_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	if (!_vm->_state->inPracticeMode())
		return;

	uint16 hsIdx = _vm->_state->_practiceLevel; // 1 ~ 4
	hotspots[hsIdx]._shapeIdx += 4;
	hotspots[hsIdx]._x -= 2;
	hotspots[hsIdx]._y -= 2;
}

void ZoombiniInteractiveRodMap::textLegend1004_postRender(ZmbFeature *feature) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	// Legend Title
	ZoombiniText::Key tKey = ZoombiniText::kTerrainKey;
	if (_vm->_state->inPracticeMode())
		tKey = ZoombiniText::kChooseLevel;

	ZmbDrawRecord *record = feature->getDrawRecord(0, 0);
	Common::Rect legendTitleRect = record->_drawnRect;
	legendTitleRect.top += 3;
	legendTitleRect.setHeight(15);

	ZoombiniGraphics::TextConf ttc;
	ttc._hAlign = Graphics::kTextAlignCenter;
	ttc._vAlign = Graphics::kTextAlignCenter;
	_vm->_gfx->drawText(screenKind, tKey, legendTitleRect, ttc);

	// Level Descriptions
	Common::Rect levelRect = record->_drawnRect;
	levelRect.top += 22;
	levelRect.setHeight(14);
	levelRect.left += 36;
	for (uint32 i = 0; i < 4; i++) {
		ZoombiniText::Key lKey = static_cast<ZoombiniText::Key>(ZoombiniText::kLevel1 + i);

		ZoombiniGraphics::TextConf ltc;
		ltc._hAlign = Graphics::kTextAlignLeft;

		// Use color on selected practice level
		ltc._outlineEffect = _vm->_state->_practiceLevel - 1u == i;
		if (1 <= _vm->_state->_practiceLevel && _vm->_state->_practiceLevel <= 4)
			ltc._outlinePalette = _levelLegendPalettes[_vm->_state->_practiceLevel - 1u];
		if (_vm->_state->_practiceLevel - 1u == i)
			ltc._textPalette = ZoombiniGraphics::kColor2D_Black;

		_vm->_gfx->drawText(screenKind, lKey, levelRect, ltc);

		// For next level
		levelRect.top += 14;
		levelRect.bottom += 14;
	}
}

ZmbEventHandleResult ZoombiniInteractiveRodMap::legendLevel1004_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	for (uint32 i = 0; i < ARRAYSIZE(_levelLegendClickRects); i++) {
		if (!_levelLegendClickRects[i].contains(absPos))
			continue;

		// Only in practice mode
		if (!_vm->_state->inPracticeMode())
			continue;

		if (4 <= i)
			error("Invalid onClickIdx idx(%u)", i);

		if (_vm->_state->_practiceLevel != i + 1) {
			_vm->_state->_practiceLevel = i + 1;
			_lastHoveredPageType = ZoombiniPageType::kNone;
			_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, kResSound0999_ButtonSFX), Audio::Mixer::kSFXSoundType);
		}
	}

	return ZmbEventHandleResult::kPassthrough;
}

/**
 * Keyboard shortcuts for level selection in practice mode: Ctrl+1 ~ Ctrl+4 to select levels 1 to 4.
 * Only works when already in practice mode, does not toggle practice mode on.
 */
ZmbEventHandleResult ZoombiniInteractiveRodMap::legendLevel1004_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	if (kbdRepeat)
		return ZmbEventHandleResult::kPassthrough;

	if (!kbd.hasFlags(0))
		return ZmbEventHandleResult::kPassthrough;

	if (_vm->_state->_practiceLevel == 0)
		return ZmbEventHandleResult::kPassthrough;
		
	uint16 selectedLevel = 0;
	switch (kbd.keycode) {
	case Common::KEYCODE_1:
	case Common::KEYCODE_KP1:
		selectedLevel = 1;
		break;
	case Common::KEYCODE_2:
	case Common::KEYCODE_KP2:
		selectedLevel = 2;
		break;
	case Common::KEYCODE_3:
	case Common::KEYCODE_KP3:
		selectedLevel = 3;
		break;
	case Common::KEYCODE_4:
	case Common::KEYCODE_KP4:
		selectedLevel = 4;
		break;
	default:
		return ZmbEventHandleResult::kPassthrough;
	}

	if (_vm->_state->_practiceLevel == selectedLevel)
		return ZmbEventHandleResult::kPassthrough;
	
	_vm->_state->_practiceLevel = selectedLevel;
	_lastHoveredPageType = ZoombiniPageType::kNone;
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, kResSound0999_ButtonSFX), Audio::Mixer::kSFXSoundType);
	return ZmbEventHandleResult::kConsumed;
}

void ZoombiniInteractiveRodMap::textJourneyStat1002_postRender(ZmbFeature *feature) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	// Stat Title
	Common::U32String titleStr;
	ZoombiniText::Key descKey[4];
	uint16 descVal[4];
	if (_vm->_state->inPracticeMode()) {
		titleStr = _vm->_text->getLocalizedString(ZoombiniText::kPracticeTitle);
		descKey[0] = ZoombiniText::kPracticeDesc1;
		descKey[1] = ZoombiniText::kPracticeDesc2;
		descKey[2] = ZoombiniText::kPracticeDesc3;
		descKey[3] = ZoombiniText::kPracticeDesc4;
	} else {
		titleStr = _vm->_state->getActiveSaveName();
		descKey[0] = ZoombiniText::kPicker;
		descKey[1] = ZoombiniText::kBasecamp1;
		descKey[2] = ZoombiniText::kBasecamp2;
		descKey[3] = ZoombiniText::kTown;
		descVal[1] = _vm->_state->_f._zmbStoredBC1Count;
		descVal[2] = _vm->_state->_f._zmbStoredBC2Count;
		descVal[3] = _vm->_state->_f._zmbStoredTownCount;
		descVal[0] = 625 - (descVal[1] + descVal[2] + descVal[3]);
	}

	ZmbDrawRecord *record = feature->getDrawRecord(0, 0);
	Common::Rect titleRect = record->_drawnRect;
	titleRect.top += 3;
	titleRect.setHeight(15);

	ZoombiniGraphics::TextConf ttc;
	ttc._hAlign = Graphics::kTextAlignCenter;
	ttc._vAlign = Graphics::kTextAlignCenter;
	_vm->_gfx->drawText(screenKind, titleStr, titleRect, ttc);

	// Stat Descriptions
	Common::Rect descStrRect = record->_drawnRect;
	descStrRect.top += 26;
	descStrRect.setHeight(18);
	descStrRect.left += 7;
	descStrRect.setWidth(115);
	Common::Rect descValRect = descStrRect;
	descValRect.left += 115;
	descValRect.right = record->_drawnRect.right - 7;
	for (uint32 i = 0; i < 4; i++) {
		ZoombiniGraphics::TextConf dktc;
		dktc._hAlign = Graphics::kTextAlignLeft;
		_vm->_gfx->drawText(screenKind, descKey[i], descStrRect, dktc);

		if (!_vm->_state->inPracticeMode()) {
			Common::U32String descValStr = Common::U32String::format("%u", descVal[i]);
			ZoombiniGraphics::TextConf dvtc;
			dvtc._hAlign = Graphics::kTextAlignRight;
			_vm->_gfx->drawText(screenKind, descValStr, descValRect, dvtc);
		}

		descStrRect.top += 18;
		descStrRect.bottom += 18;
		descValRect.top += 18;
		descValRect.bottom += 18;
	}

	// IDA rodmap_drawJourneyStats: combobox text ("Practice Mode" / "Continue Journey")
	// is drawn here together with the stat panel, not in SCRB 1003's postRender.
	Common::Rect comboPracticeRect = Common::Rect(50, 135, 197, 152);
	ZoombiniGraphics::TextConf cptc;
	cptc._hAlign = Graphics::kTextAlignLeft;
	_vm->_gfx->drawText(screenKind, ZoombiniText::kPracticeMode, comboPracticeRect, cptc);

	Common::Rect comboJourneyRect = Common::Rect(50, 154, 197, 176);
	ZoombiniGraphics::TextConf cjtc;
	cjtc._hAlign = Graphics::kTextAlignLeft;
	_vm->_gfx->drawText(screenKind, ZoombiniText::kContinueJourney, comboJourneyRect, cjtc);
}

void ZoombiniInteractiveRodMap::drawComboBox1003_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	// Hotspot Index
	// 1: Practice Mode + Outline (112)
	// 2: Practice Mode + Red Circle (111)
	// 3: Journey Mode + Outline (112)
	// 4: Journey Mode + Red Circle (111)
	if (hotspots.size() < 4)
		error("Invalid page mhk resource count: SCRB(%u) frame(%u) hsCount(%u)", feature->getId(), hsGroup->_frameIdx, hotspots.size());

	if (_vm->_state->inPracticeMode()) {
		hotspots[4 - 1]._shapeIdx = ZmbHotspot::kShapeNone;
	} else {
		hotspots[2 - 1]._shapeIdx = ZmbHotspot::kShapeNone;
	}
}


ZmbEventHandleResult ZoombiniInteractiveRodMap::selectMode1003_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	for (uint32 i = 0; i < ARRAYSIZE(_modeSelectClickRects); i++) {
		if (!_modeSelectClickRects[i].contains(absPos))
			continue;

		switch (i) {
		case 0:
			setPracticeMode(true);
			break;
		case 1:
			setPracticeMode(false);
			break;
		}
	}

	return ZmbEventHandleResult::kPassthrough;
}

void ZoombiniInteractiveRodMap::textRouteNames_postRender(ZmbFeature *feature) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	for (uint32 i = ZMB_ROUTE_BIG_BAD_HUNGRY; i <= ZMB_ROUTE_MONT_DESPAIR; i++) {
		// Do not draw the route name if the route was not visited yet
		if (_pageRouteLevelMap[kResShapeRouteBigBadHungryP0_17 + 4 * i + 1] == 0)
			continue;

		const Common::Rect &textRect = _routeNameRects[i];
		ZoombiniText::Key textKey = _routeNameTextKey[i];

		ZoombiniGraphics::TextConf tc;
		tc._outlineEffect = true;
		tc._outlinePalette = ZoombiniGraphics::kColor2D_Black;
		tc._textPalette = 0x0A;
		tc._hAlign = Graphics::kTextAlignCenter;
		tc._vAlign = Graphics::kTextAlignCenter;
		_vm->_gfx->drawText(screenKind, textKey, textRect, tc);
	}
}

void ZoombiniInteractiveRodMap::buildPageRouteLevelMap() {
	if (_vm->_state->inPracticeMode()) {
		for (uint32 i = kResShapeRouteBigBadHungryP0_17; i <= kResShapeRouteMontDespairP3_32; i++) {
			_pageRouteLevelMap[i] = _vm->_state->_practiceLevel;
		}
	} else {
		for (uint32 i = kResShapeRouteBigBadHungryP0_17; i <= kResShapeRouteMontDespairP3_32; i++) {
			_pageRouteLevelMap[i] = 0;
		}

		for (uint32 routeIdx = ZMB_ROUTE_BIG_BAD_HUNGRY; routeIdx <= ZMB_ROUTE_MONT_DESPAIR; routeIdx++) {
			uint16 routeLevel = _vm->_state->readRouteLevel(routeIdx);
			if (0 < routeLevel) {
				for (uint32 i = 0; i < 4; i++) {
					uint16 routeKey = 4 * routeIdx + i + kResShapeRouteBigBadHungryP0_17;
					_pageRouteLevelMap[routeKey] = routeLevel + 1;
				}
			} else { // Level 1 or Unvisited, detemine which one is true from level flags.
				// IDA `readPuzzleLevelValArr_42C1EC` indexes pbPuzzleLevelFlagArr
				// directly by puzzle slot when routeLevel == 0:
				//   Route 0 (BBH):  pbPuzzleLevelFlagArr[3..5]   = Bridge/Tunnels/Pizza
				//   Route 1 (WB):   pbPuzzleLevelFlagArr[6..8]   = Ferry/Lilly/Slides
				//   Route 2 (DDF):  pbPuzzleLevelFlagArr[9..11]  = Fleens/Hotel/Net
				//   Route 3 (MD):   pbPuzzleLevelFlagArr[12..14] = Caves/Smoke/Maze
				// Combined offset: `3 * routeIdx + 3 + i` (matches the IDA loop
				// indices `v2`, `v4`, `v6`, `v8` which scan 1-3, 5-7, 8-10, 12-14
				// with per-route +N offsets that all collapse to that formula).
				//
				// The previous port used `routeKey` (a shape resource ID in
				// [17..32]) as the array index — out-of-bounds against the
				// 15-byte _levelFlagPageArr, reading garbage memory and producing
				// arbitrary route-progress colors on the rodmap. routeKey stays
				// the StableMap key (it identifies the rodmap shape slot) but
				// the source data must be looked up by puzzle index.
				for (uint32 i = 0; i < 3; i++) {
					uint16 routeKey = 4 * routeIdx + i + kResShapeRouteBigBadHungryP0_17;
					uint16 puzzleIdx = 3 * routeIdx + 3 + i;
					uint16 levelFlag = _vm->_state->_f._levelFlagPageArr[puzzleIdx] & 0x0F;
					_pageRouteLevelMap[routeKey] = levelFlag;
				}
				uint16 levelFlagVal = 0;
				switch (routeIdx) {
				case ZMB_ROUTE_BIG_BAD_HUNGRY:
					levelFlagVal = _vm->_state->_f._levelFlagRouteBigBadHungry & 0x0F;
					break;
				case ZMB_ROUTE_WHOS_BAYOU: // Low nibble of the levelFlag byte
					levelFlagVal = _vm->_state->_f._levelFlagLoWhosBayouHiDeepDarkForest & 0x0F;
					break;
				case ZMB_ROUTE_DEEP_DARK_FOREST: // High nibble of the levelFlag byte
					levelFlagVal = (_vm->_state->_f._levelFlagLoWhosBayouHiDeepDarkForest & 0xF0) >> 4;
					break;
				case ZMB_ROUTE_MONT_DESPAIR:
					levelFlagVal = _vm->_state->_f._levelFlagRouteMontDespair & 0x0F;
					break;
				default:
					error("Invalid routeIdx %u", routeIdx);
					break;
				}
				uint16 lastRouteKey = 4 * routeIdx + 3 + kResShapeRouteBigBadHungryP0_17;
				_pageRouteLevelMap[lastRouteKey] = levelFlagVal;
			}
		}
	}
}

} // End of namespace Mohawk
