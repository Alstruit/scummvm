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

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_pages/puzzle_smoke.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions from 0x4A4368 (20 POINTS)
const Common::Point ZoombiniPuzzleSmoke::kSnoidPositions[20] = {
	Common::Point(214, 128), Common::Point(175, 126), Common::Point(135, 127), Common::Point( 94, 126),
	Common::Point( 53, 128), Common::Point(237, 176), Common::Point(196, 177), Common::Point(150, 178),
	Common::Point(110, 176), Common::Point( 69, 178), Common::Point(234,  36), Common::Point(195,  37),
	Common::Point(155,  36), Common::Point(114,  35), Common::Point( 73,  38), Common::Point(237,  79),
	Common::Point(196,  78), Common::Point(150,  80), Common::Point(110,  78), Common::Point( 69,  79),
};

// IDA: DRAW_ON_REG position at stru_4A400C+0x00
const Common::Point ZoombiniPuzzleSmoke::kDrawOnRegPosition = Common::Point(43, 258);

// IDA: stru_4A400C+0x18 (8 cliff runner positions)
const Common::Point ZoombiniPuzzleSmoke::kCliffRunnerPositions[8] = {
	Common::Point(459, 26), Common::Point(535, 25), Common::Point(429, 80), Common::Point(500, 84),
	Common::Point(619, 76), Common::Point(423, 168), Common::Point(525, 167), Common::Point(605, 163),
};

// IDA: stru_4A400C+0x40 (8 grid runner positions — rows 0-7, row 8 is kCliffDropSnapPosition)
const Common::Point ZoombiniPuzzleSmoke::kGridRunnerPositions[8] = {
	Common::Point(441, 66), Common::Point(531, 70), Common::Point(605, 67), Common::Point(421, 160),
	Common::Point(483, 153), Common::Point(612, 153), Common::Point(548, 255), Common::Point(616, 253),
};

// IDA: stru_4A400C+0x70 (2 exit runner positions)
const Common::Point ZoombiniPuzzleSmoke::kExitRunnerPositions[2] = {
	Common::Point(124, 255), Common::Point(548, 255),
};

// IDA: dword_4A4184 (2 bottom runner default positions)
const Common::Point ZoombiniPuzzleSmoke::kBottomRunnerPositions[2] = {
	Common::Point(317, 254), Common::Point(354, 254),
};

// IDA: dword_4A4008 — off-screen hide position
const Common::Point ZoombiniPuzzleSmoke::kHidePosition = Common::Point(-8, 258);

// IDA: dword_4A4004 — rejection position
const Common::Point ZoombiniPuzzleSmoke::kRejectPosition = Common::Point(530, 384);

// IDA: stru_4A400C+0x74 — cliff drop snap position
const Common::Point ZoombiniPuzzleSmoke::kCliffDropSnapPosition = Common::Point(548, 255);

// IDA: stru_4A400C+0x80 — L1-2 cliff drop rectangle
const Common::Rect ZoombiniPuzzleSmoke::kCliffDropRect = Common::Rect(525, 211, 582, 300);

// IDA: word_4A435C — cliff column X snap positions (5 columns)
const int16 ZoombiniPuzzleSmoke::kColumnSnapX[5] = { 38, 81, 126, 176, 221 };

// IDA: unk_4A40DC — L3-4 drag zone rects group A (3 slots × 3 rects)
const Common::Rect ZoombiniPuzzleSmoke::kDragRectsA[9] = {
	// slot 0
	Common::Rect(185, 230, 245, 293), Common::Rect(0, 0, 0, 0), Common::Rect(0, 0, 0, 0),
	// slot 1
	Common::Rect(137, 230, 197, 293), Common::Rect(236, 230, 296, 293), Common::Rect(0, 0, 0, 0),
	// slot 2
	Common::Rect(124, 230, 184, 293), Common::Rect(197, 230, 257, 293), Common::Rect(265, 230, 325, 293),
};

// IDA: unk_4A4124 — L3-4 drag zone rects group B (3 slots × 3 rects)
const Common::Rect ZoombiniPuzzleSmoke::kDragRectsB[9] = {
	// slot 0
	Common::Rect(426, 230, 486, 293), Common::Rect(0, 0, 0, 0), Common::Rect(0, 0, 0, 0),
	// slot 1
	Common::Rect(372, 230, 432, 293), Common::Rect(474, 230, 534, 293), Common::Rect(0, 0, 0, 0),
	// slot 2
	Common::Rect(350, 230, 400, 293), Common::Rect(419, 230, 469, 293), Common::Rect(485, 230, 535, 293),
};

// IDA: dword_4A418C — display pair normal A (12 frames, x=317)
const Common::Point ZoombiniPuzzleSmoke::kDisplayPairNormalA[12] = {
	Common::Point(317, 263), Common::Point(317, 248), Common::Point(317, 236), Common::Point(317, 210),
	Common::Point(317, 201), Common::Point(317, 192), Common::Point(317, 201), Common::Point(317, 210),
	Common::Point(317, 236), Common::Point(317, 248), Common::Point(317, 263), Common::Point(317, 254),
};

// IDA: dword_4A41C4 — display pair normal B (12 frames, x=354)
const Common::Point ZoombiniPuzzleSmoke::kDisplayPairNormalB[12] = {
	Common::Point(354, 263), Common::Point(354, 248), Common::Point(354, 236), Common::Point(354, 210),
	Common::Point(354, 201), Common::Point(354, 192), Common::Point(354, 201), Common::Point(354, 210),
	Common::Point(354, 236), Common::Point(354, 248), Common::Point(354, 263), Common::Point(354, 254),
};

// IDA: dword_4A41FC — display pair swapped A (12 frames, x=317)
const Common::Point ZoombiniPuzzleSmoke::kDisplayPairSwappedA[12] = {
	Common::Point(317, 257), Common::Point(317, 261), Common::Point(317, 264), Common::Point(317, 267),
	Common::Point(317, 270), Common::Point(317, 274), Common::Point(317, 277), Common::Point(317, 280),
	Common::Point(317, 277), Common::Point(317, 270), Common::Point(317, 267), Common::Point(317, 264),
};

// IDA: dword_4A4240 — display pair swapped B (12 frames, x=354)
const Common::Point ZoombiniPuzzleSmoke::kDisplayPairSwappedB[12] = {
	Common::Point(354, 257), Common::Point(354, 261), Common::Point(354, 264), Common::Point(354, 267),
	Common::Point(354, 270), Common::Point(354, 274), Common::Point(354, 277), Common::Point(354, 280),
	Common::Point(354, 277), Common::Point(354, 270), Common::Point(354, 267), Common::Point(354, 264),
};

ZoombiniPuzzleSmoke::ZoombiniPuzzleSmoke(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kSmoke) {
}

ZoombiniPuzzleSmoke::~ZoombiniPuzzleSmoke() {
	// IDA: chBoolSFXTurnOnOff_4B8252 = smoke_savedSFXState;
	_vm->_sound->setSfxMuted(!_savedSFXState);
}

void ZoombiniPuzzleSmoke::open() {
	if (!_vm->isGameVariant(GF_ZMB_TLC))
		openArchive(ZMB_MHK_MIDIMPC);
	openArchive(ZMB_MHK_SMOKE);
}

void ZoombiniPuzzleSmoke::setBackgroundMusic() {
	// IDA: scrb_enqueueSoundResource(30030 + routeDiffLevel)
	if (!_vm->isGameVariant(GF_ZMB_TLC)) {
		int16 routeLevel = _vm->_state->readActivePageRouteLevel();
		_vm->_midi->playZmbMidi(ZmbResource(ZmbArchiveKind::kPage, (uint16)(30030 + routeLevel)));
	}
}

void ZoombiniPuzzleSmoke::setBackgroundBitmap() {
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

ZmbSmokeRunnerState *ZoombiniPuzzleSmoke::findRunnerState(ZmbFeature *feature) {
	for (int16 i = 0; i < _cliffRunnerCount; i++) {
		if (_cliffRunners[i] == feature)
			return &_cliffRunnerStates[i];
	}
	for (int16 i = 0; i < _level2RunnerCount; i++) {
		if (_level2Runners[i] == feature)
			return &_level2RunnerStates[i];
	}
	for (int16 i = 0; i < _gridRunnerCount; i++) {
		if (_gridRunners[i] == feature)
			return &_gridRunnerStates[i];
	}
	for (int16 i = 0; i < _exitRunnerCount; i++) {
		if (_exitRunners[i] == feature)
			return &_exitRunnerStates[i];
	}
	for (int16 i = 0; i < _bottomRunnerCount; i++) {
		if (_bottomRunners[i] == feature)
			return &_bottomRunnerStates[i];
	}
	return nullptr;
}

void ZoombiniPuzzleSmoke::loadFeatures() {
	// IDA: smoke_init (0x44983c)

	// --- State initialization (matches IDA init sequence exactly) ---
	_puzzleActive = false;
	_bExitGateEnabled = false;
	_displayPairIdx = 0;
	_bMatchReady = false;
	_currentDragZmb = nullptr;
	_bFirstAttrAssign = true;
	_answerState = 2;
	_zmbCount = 0;
	_dragSlotIdxA = 0;
	_dragSlotIdxB = 0;
	_level2RunnerCount = 0;
	_exitRunnerCount = 0;
	_bottomRunnerCount = 0;
	_placedZmbCount = 0;
	_currentZmbIdx = 0;
	_bRunnerToggle = false;
	_transitionPhase = 3;
	_loadedOnCliffCount = 0;
	_compareIdx = 0;
	_bPlaceZmb = false;
	_bLinkRunners = false;
	_bReloadScrb = false;
	_bResetLevel = false;
	_bShowResults = false;
	_bReloadMainRunner = false;
	_questionResult = 0;
	_bShowAnswer = false;
	_bCheatMode = false;
	_bDragLocked = false;
	_lastIdleFrameTime = 0;
	_idlePoolState = 0;
	_idleAnimCount = 0;
	_bIdleAnimActive = false;
	_smokeColumnCount = 0;
	_rejectedCount = 0;
	_resultHotspotIdx = 0;
	_processingFrame = false;
	_animSetIdx = 0;
	_bCompareSwapped = false;

	// IDA: smoke_savedSFXState = chBoolSFXTurnOnOff_4B8252; chBoolSFXTurnOnOff_4B8252 = 0;
	_savedSFXState = !_vm->_sound->isSfxMuted();
	_vm->_sound->setSfxMuted(true);

	// Initialize permutation array
	for (int i = 0; i < 8; i++)
		_permutation[i] = i;

	// Clear display runner tracking
	memset(_displayRunnerArr, 0, sizeof(_displayRunnerArr));

	// Clear runner and zmb arrays
	for (int i = 0; i < 20; i++) {
		_smokeColumnRunners[i] = nullptr;
		_zmbOnCliff[i] = nullptr;
		_cliffRunners[i] = nullptr;
	}
	for (int i = 0; i < 6; i++)
		_level2Runners[i] = nullptr;
	for (int i = 0; i < 9; i++)
		_gridRunners[i] = nullptr;
	for (int i = 0; i < 4; i++)
		_exitRunners[i] = nullptr;
	for (int i = 0; i < 2; i++)
		_bottomRunners[i] = nullptr;
	for (int i = 0; i < 21; i++)
		_zmbQueue[i] = 0;

	// Clear attribute state
	memset(_attrDisplayTable, 0, sizeof(_attrDisplayTable));
	memset(_attrGridPrimary, 0, sizeof(_attrGridPrimary));
	memset(_attrGridSecondary, 0, sizeof(_attrGridSecondary));
	memset(_attrGridMatchFlags, 0, sizeof(_attrGridMatchFlags));
	memset(_questionAttrs, 0, sizeof(_questionAttrs));
	memset(_level1AttrHistory, 0, sizeof(_level1AttrHistory));
	memset(_level2AttrHistory, 0, sizeof(_level2AttrHistory));

	// --- Difficulty setup ---
	_difficultyLevel = static_cast<ZmbPuzzleDifficultyLevel>(_vm->_state->readActivePageRouteLevel() + 1);
	if (_difficultyLevel > kPuzzleDiffLevel4)
		_difficultyLevel = kPuzzleDiffLevel4;

	debugC(kZmbDebugPage, "Smoke: difficultyLevel=%d", _difficultyLevel);

	// --- Per-difficulty SCRB IDs (IDA: 0x449911) ---
	if (_difficultyLevel <= kPuzzleDiffLevel2) {
		_scrbAnimIdArr[0] = 11024;
		_scrbAnimIdArr[1] = 11025;
		_scrbAnimIdArr[2] = 11026;
		_scrbAnimIdArr[3] = 11027;
		_scrbZmbAnimIdArr[0] = 11999;
		_scrbZmbAnimIdArr[1] = 12004;
		_scrbSmokeStackResA = 11032;
		_scrbSmokeStackResB = 0;
		_scrbTravelResId = 0;
		_scrbPickupResId = 0;
		_scrbDropResId = 0;
		_scrbWalkResId = 0;
	} else {
		_scrbAnimIdArr[0] = 11028;
		_scrbAnimIdArr[1] = 11029;
		_scrbAnimIdArr[2] = 11030;
		_scrbAnimIdArr[3] = 11031;
		_scrbZmbAnimIdArr[0] = 12009;
		_scrbZmbAnimIdArr[1] = 12014;
		_scrbSmokeStackResA = 11033;
		_scrbSmokeStackResB = 11034;
		_scrbTravelResId = 11035;
		_scrbPickupResId = 12038;
		_scrbDropResId = 12039;
		_scrbWalkResId = 12040;
	}

	if (_difficultyLevel == kPuzzleDiffLevel4) {
		_scrbOverlayResId = 11011;
		_scrbTransitionResId = 11012;
	} else {
		_scrbOverlayResId = 11013;
		_scrbTransitionResId = 0;
	}

	// --- Terrain + preloads ---
	loadTerrainBitmap(100);

	// Preload shape images at tBMP 10000 (0x2710)
	// IDA: shape_loadSubShapesFromArchive(&stru_4B1D0C, 0x2710u)
	_vm->_gfx->preloadImage(10000);
	_vm->_gfx->preloadImage(11000);

	// --- Feature group + snoid pools ---
	createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrs_registerPool0_4524AF(0, 1, 11999) — reject pool
	loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 10000),
			  11999,
			  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);

	// IDA: scrs_registerPool1_45258E(0, 50, 12000) — normal pool
	for (uint16 i = 0; i < 50; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 10000),
				  12000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// --- Feature runners (IDA: smoke_init 0x4499xx-0x449Fxx) ---

	// IDA: smoke_scrbOverlayAnim — interval=10
	_overlayAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), _scrbOverlayResId, 10,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbLevel12Extra — SCRB 11076, L1-2 only, interval=10
	if (_difficultyLevel <= kPuzzleDiffLevel2) {
		_level12ExtraFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 11076, 10,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER |
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// IDA: smoke_scrbCliffLeft — SCRB 11006, interval=10
	_cliffLeftFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), 11006, 10,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbCliffRight — SCRB 11007, interval=10
	_cliffRightFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), 11007, 10,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbMainAnim — interval=6, uses _scrbAnimIdArr[animSetIdx]
	_mainAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), _scrbAnimIdArr[_animSetIdx], 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbSmokeStackA — interval=6
	_smokeStackAFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), _scrbSmokeStackResA, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbSmokeStackB — L3-4 only
	if (_difficultyLevel >= kPuzzleDiffLevel3) {
		_smokeStackBFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), _scrbSmokeStackResB, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// IDA: smoke_scrbSecondAnim — interval=6, uses _scrbAnimIdArr[animSetIdx+1]
	_secondAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), _scrbAnimIdArr[_animSetIdx + 1], 6,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_04000000_OVERLAY |
		ZmbFeature::FLAG_08000000_REGION_TRACK);

	// IDA: smoke_scrbCompareA — SCRB 11018, interval=6
	_compareAFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), 11018, 6,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbCompareB — SCRB 11019, interval=6
	_compareBFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), 11019, 6,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_04000000_OVERLAY |
		ZmbFeature::FLAG_08000000_REGION_TRACK);

	// IDA: smoke_scrbBgOverlay — SCRB 11009, interval=6
	_bgOverlayFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), 11009, 6,
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbRejection — SCRB 11036, interval=6
	_rejectionFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), 11036, 6,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbBackground — SCRB 11008, interval=0
	_backgroundFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), 11008, 0,
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbAnswerZone — SCRB 11002, interval=5
	_answerZoneFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), 11002, 5,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbHoldingArea — SCRB 11077, interval=0
	_holdingAreaFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), 11077, 0,
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);

	// --- Load Zoombinis and build runner stacks ---
	loadZoombinisFromPack();

	// IDA: smoke_cliffRunnerCount = 0; smoke_gridRunnerCount = 1;
	_cliffRunnerCount = 0;
	_gridRunnerCount = 1;  // NOTE: starts at 1 in original!

	buildRunnerStacks();

	// IDA: DRAW_ON_REG for L1-2 only
	if (_difficultyLevel < kPuzzleDiffLevel3) {
		_drawOnRegFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 11001, 7,
			kDrawOnRegPosition,
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER);
	}

	// --- Buttons ---
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(10000);
	loadHelpButtonFeature();

	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagSmoke);
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, _vm->_rnd->getRandomNumber(20066, 20067));

	// IDA: Initial SCRB loading: mainAnim=11015, secondAnim=11016
	if (_mainAnimFeature)
		loadScrbOntoFeature(_mainAnimFeature, 11015);
	if (_secondAnimFeature)
		loadScrbOntoFeature(_secondAnimFeature, 11016);

	// IDA: Link currentDragZmb to secondAnim (will be set later)
	// scrb_registerHotspotGroup handled by ScummVM internally

	// For levels 3-4: show answer display on init
	if (_difficultyLevel >= kPuzzleDiffLevel3) {
		_bShowAnswer = true;
		loadScrbOnAnswerRunner(11003);
	}

	_puzzleActive = true;
}

void ZoombiniPuzzleSmoke::onGoButtonActivated() {
	// IDA: smoke_onClickHandler case 2
	// Route 4: Smoke -> Maze (via Xfer)
	// NOTE: Original uses SND_0 (no departure SFX).
	_departXferSrcSiPage = ZMB_SI_SMOKE_15;
	_pendingGoDepart = true;
}

void ZoombiniPuzzleSmoke::loadZoombinisFromPack() {
	ZmbStateFile &f = _vm->_state->_f;
	uint16 posIdx = 0;

	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount && posIdx < 20; i++) {
		ZmbStateActiveEntry &entry = f._zmbPackActive._entries[i];
		if (!entry._bIsOccupied)
			continue;

		Common::Point pos = kSnoidPositions[posIdx];
		uint16 snoidId = 10000 + posIdx;

		ZmbSnoid *snoid = loadSnoidFromPack(snoidId, pos,
		                                    ZmbFeature::FLAG_00000001_TYPE_SNOID);
		if (snoid) {
			snoid->_trait = entry._traits;
			snoid->_name = entry.getU32Name(_vm);
			snoid->_packIsOccupied = true;
			snoid->setupIdleHotspots();

			// Add to queue
			if (_zmbQueueSize < 21) {
				_zmbQueue[_zmbQueueSize++] = snoidId;
			}
		}
		posIdx++;
	}
	_zmbCount = posIdx;
}

// =========================================================================
// Helper methods
// =========================================================================

void ZoombiniPuzzleSmoke::playZmbScript(bool linkToHotspot, ZmbFeature *parentFeature, uint16 scrsId, ZmbSnoid *snoid) {
	// IDA: smoke_playZmbScript / snoidScript_initAndPlay
	if (!snoid)
		return;
	Common::SeekableReadStream *stream = _vm->getResource(ID_SCRS, ZmbResource(ZmbArchiveKind::kPage, scrsId));
	if (stream) {
		snoid->startScrsPlayback(stream, linkToHotspot);
		if (linkToHotspot && parentFeature)
			snoid->setSubFeature(parentFeature);
	}
}

void ZoombiniPuzzleSmoke::unloadTimerScrb() {
	// IDA: smoke_unloadTimerScrb
	if (_level12ExtraFeature) {
		_level12ExtraFeature->addFlag(ZmbFeature::FLAG_01000000_DEFER_RENDER);
		_level12ExtraFeature->deactivateRender();
	}
}

void ZoombiniPuzzleSmoke::loadScrbOnAnswerRunner(uint16 scrbId) {
	// IDA: smoke_loadSCRBOnAnswerRunner (0x44BA3D)
	if (_answerZoneFeature) {
		_answerZoneFeature->addFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM));
		_answerZoneFeature->removeFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER));
		loadScrbOntoFeature(_answerZoneFeature, scrbId, true);
	}
}

void ZoombiniPuzzleSmoke::loadScrbOnWellRunner(uint16 scrbId) {
	// IDA: smoke_loadSCRBOnWellRunner (0x44BAB9)
	if (_answerZoneFeature) {
		addExternalDirtyRect(_answerZoneFeature->getClickRect());
		_answerZoneFeature->setFrameInterval(0);
		loadScrbOntoFeature(_answerZoneFeature, scrbId, false);
		// Set flags to 0x04188000 = OVERLAY | PLAY_ONCE | DEFER_ANIM | LOOP_ANIM
		_answerZoneFeature->addFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE));
		_answerZoneFeature->removeFlag(ZmbFeature::FLAG_01000000_DEFER_RENDER);
	}
}

void ZoombiniPuzzleSmoke::loadScoreDisplayScrbs() {
	// IDA: smoke_loadScoreDisplayScrbs (0x44BB5D)
	// Set flags to 0x05188000 = OVERLAY | DEFER_RENDER | PLAY_ONCE | DEFER_ANIM | LOOP_ANIM
	if (_cliffLeftFeature) {
		_cliffLeftFeature->addFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00008000_LOOP_ANIM));
		loadScrbOntoFeature(_cliffLeftFeature, 11006, false);
	}
	if (_cliffRightFeature) {
		_cliffRightFeature->addFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00008000_LOOP_ANIM));
		loadScrbOntoFeature(_cliffRightFeature, 11007, false);
	}
}

void ZoombiniPuzzleSmoke::loadTimerScrb() {
	// IDA: smoke_loadTimerScrb (0x44BBAD)
	if (_level12ExtraFeature) {
		// Set flags to 0x04008000 = OVERLAY | LOOP_ANIM
		_level12ExtraFeature->addFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM));
		_level12ExtraFeature->removeFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER));
		loadScrbOntoFeature(_level12ExtraFeature, 11076, true);
	}
}

// =========================================================================
// Attribute management
// =========================================================================

void ZoombiniPuzzleSmoke::clearZmbAttrs(int16 idx) {
	// IDA: smoke_clearZmbAttrs (0x44C5E5)
	ZmbSmokeRunnerState *state = nullptr;

	if (idx <= 1) {
		if (idx >= 0 && idx < _exitRunnerCount && _exitRunners[idx])
			state = findRunnerState(_exitRunners[idx]);
	} else if (idx == 7 || idx == 8) {
		if (idx < _gridRunnerCount && _gridRunners[idx])
			state = findRunnerState(_gridRunners[idx]);
	}

	if (state) {
		state->cachedAttrs[0] = 0;
		state->cachedAttrs[1] = 0;
		state->cachedAttrs[2] = 0;
		state->cachedAttrs[3] = 0;
		state->attrCyclePos = 4;
	}
}

void ZoombiniPuzzleSmoke::clearRunnerSlot(int16 slotIdx) {
	// IDA: smoke_clearRunnerSlot (0x44C65C)
	if (slotIdx >= 0 && slotIdx < 8) {
		_attrDisplayTable[4 * slotIdx] = 0;
		_attrDisplayTable[4 * slotIdx + 1] = 0;
		_attrDisplayTable[4 * slotIdx + 2] = 0;
		_attrDisplayTable[4 * slotIdx + 3] = 0;
	}
}

void ZoombiniPuzzleSmoke::clearAllRunnerSlots() {
	// IDA: smoke_clearAllRunnerSlots (0x44C69A)
	memset(_attrDisplayTable, 0, sizeof(_attrDisplayTable));
}

void ZoombiniPuzzleSmoke::clearDisplayRunners() {
	// IDA: smoke_clearDisplayRunners (0x44C6D0)
	for (int16 i = 0; i < 2; ++i) {
		ZmbFeature *runner = _bottomRunners[i];
		if (!runner)
			continue;
		ZmbSmokeRunnerState *state = findRunnerState(runner);
		if (!state)
			continue;

		runner->setFrameInterval(0);
		runner->setNeedsRedraw(true);
		state->attrs[0] = 0;
		state->attrs[1] = 0;
		state->attrs[2] = 0;
		state->attrs[3] = 0;
		addExternalDirtyRect(runner->getClickRect());
	}
}

void ZoombiniPuzzleSmoke::assignZmbAttrsFromSrc(int16 srcIdx, ZmbSnoid *zmb) {
	// IDA: smoke_assignZmbAttrsFromSrc (0x44C048)
	// Copies zmb's trait bytes into the runner's cachedAttrs.
	// srcIdx: 0/1 -> _exitRunners, 7/8 -> _gridRunners
	ZmbFeature *srcRunner = nullptr;

	if (srcIdx == 0) {
		srcRunner = (_exitRunnerCount > 0) ? _exitRunners[0] : nullptr;
	} else if (srcIdx == 1) {
		srcRunner = (_exitRunnerCount > 1) ? _exitRunners[1] : nullptr;
	} else if (srcIdx == 7) {
		srcRunner = (_gridRunnerCount > 7) ? _gridRunners[7] : nullptr;
	} else if (srcIdx == 8) {
		srcRunner = (_gridRunnerCount > 8) ? _gridRunners[8] : nullptr;
	}

	if (!srcRunner)
		return;
	ZmbSmokeRunnerState *srcState = findRunnerState(srcRunner);
	if (!srcState)
		return;

	if (zmb) {
		srcState->cachedAttrs[0] = zmb->_trait._head;
		srcState->cachedAttrs[1] = zmb->_trait._eye;
		srcState->cachedAttrs[2] = zmb->_trait._nose;
		srcState->cachedAttrs[3] = zmb->_trait._foot;
		srcState->attrCyclePos = 4;

		// IDA: runner_linkRelativeToParent — NO-OP in ScummVM
	}
}

void ZoombiniPuzzleSmoke::cacheZmbAttrs(int16 srcIdx, ZmbSnoid *zmb) {
	// IDA: smoke_cacheZmbAttrs (0x44C124)
	if (!zmb)
		return;

	_attrDisplayTable[4 * srcIdx] = zmb->_trait._head;
	_attrDisplayTable[4 * srcIdx + 1] = zmb->_trait._eye;
	_attrDisplayTable[4 * srcIdx + 2] = zmb->_trait._nose;
	_attrDisplayTable[4 * srcIdx + 3] = zmb->_trait._foot;
}

void ZoombiniPuzzleSmoke::loadZmbAttrsToCache() {
	// IDA: smoke_loadZmbAttrsToCache (0x44C181)
	// Read attrs from display runners [1..3] into attrDisplayTable slots [1..3]
	for (int16 i = 1; i < 4; ++i) {
		ZmbFeature *feature = _displayRunnerArr[i];
		if (feature) {
			ZmbSmokeRunnerState *state = findRunnerState(feature);
			if (state) {
				_attrDisplayTable[4 * i] = state->attrs[0];
				_attrDisplayTable[4 * i + 1] = state->attrs[1];
				_attrDisplayTable[4 * i + 2] = state->attrs[2];
				_attrDisplayTable[4 * i + 3] = state->attrs[3];
				continue;
			}
		}
		_attrDisplayTable[4 * i] = 0;
		_attrDisplayTable[4 * i + 1] = 0;
		_attrDisplayTable[4 * i + 2] = 0;
		_attrDisplayTable[4 * i + 3] = 0;
	}
}

void ZoombiniPuzzleSmoke::cacheAnswerRunnerAttrs() {
	// IDA: smoke_cacheAnswerRunnerAttrs (0x44C218)
	// Read attrs from display runners [4..6] into attrDisplayTable slots [4..6]
	for (int16 i = 4; i < 7; ++i) {
		ZmbFeature *feature = (i - 1 < 6) ? _displayRunnerArr[i - 1] : nullptr;
		if (feature) {
			ZmbSmokeRunnerState *state = findRunnerState(feature);
			if (state) {
				_attrDisplayTable[4 * i] = state->attrs[0];
				_attrDisplayTable[4 * i + 1] = state->attrs[1];
				_attrDisplayTable[4 * i + 2] = state->attrs[2];
				_attrDisplayTable[4 * i + 3] = state->attrs[3];
				continue;
			}
		}
		_attrDisplayTable[4 * i] = 0;
		_attrDisplayTable[4 * i + 1] = 0;
		_attrDisplayTable[4 * i + 2] = 0;
		_attrDisplayTable[4 * i + 3] = 0;
	}
}

void ZoombiniPuzzleSmoke::cycleZmbAttrDisplay() {
	// IDA: smoke_cycleZmbAttrDisplay (0x44C2AB)
	for (int16 i = 0; i < 3; ++i) {
		ZmbFeature *feature = _displayRunnerArr[i];
		if (!feature)
			continue;
		ZmbSmokeRunnerState *state = findRunnerState(feature);
		if (!state)
			continue;

		state->attrCyclePos = 4;
		if (state->matchCount == 0)
			continue;

		int16 mc = state->matchCount;
		uint8 val = 0;

		if (i == 0) {
			val = _attrDisplayTable[mc - 1];
		} else if (i == 1) {
			val = _attrDisplayTable[mc + 3];
			if (!val)
				val = _attrDisplayTable[mc - 1];
		} else if (i == 2) {
			val = _attrDisplayTable[mc + 7];
			if (!val) {
				val = _attrDisplayTable[mc + 3];
				if (!val)
					val = _attrDisplayTable[mc - 1];
			}
		}

		state->cachedAttrs[mc - 1] = val + 1;
		if (state->cachedAttrs[mc - 1] > 5)
			state->cachedAttrs[mc - 1] = 1;

		_attrDisplayTable[4 * i + 3 + mc] = state->cachedAttrs[mc - 1];
		feature->setNeedsRedraw(true);
	}
}

void ZoombiniPuzzleSmoke::advanceAnswerRunnerFrames() {
	// IDA: smoke_advanceAnswerRunnerFrames (0x44C444)
	for (int16 i = 5; i > 2; --i) {
		ZmbFeature *feature = _displayRunnerArr[i];
		if (!feature)
			continue;
		ZmbSmokeRunnerState *state = findRunnerState(feature);
		if (!state)
			continue;

		state->attrCyclePos = 4;
		if (state->matchCount == 0)
			continue;

		int16 mc = state->matchCount;
		uint8 val = 0;

		switch (i) {
		case 3:
			val = _attrDisplayTable[mc + 19];
			if (!val) {
				val = _attrDisplayTable[mc + 23];
				if (!val)
					val = _attrDisplayTable[mc + 27];
			}
			break;
		case 4:
			val = _attrDisplayTable[mc + 23];
			if (!val)
				val = _attrDisplayTable[mc + 27];
			break;
		case 5:
			val = _attrDisplayTable[mc + 27];
			break;
		default:
			break;
		}

		state->cachedAttrs[mc - 1] = val + 1;
		if (state->cachedAttrs[mc - 1] > 5)
			state->cachedAttrs[mc - 1] = 1;

		_attrDisplayTable[4 * i + 3 + mc] = state->cachedAttrs[mc - 1];
		feature->setNeedsRedraw(true);
	}
}

// =========================================================================
// Compare / Match logic
// =========================================================================

int16 ZoombiniPuzzleSmoke::compareTwoOrderLines() {
	// IDA: pizza_compareTwoOrderLines (0x44C7D0)
	// Compares attrs in bottom runners [0] and [1] using attrDisplayTable.
	// Returns 0 if match, 2 if mismatch.
	ZmbSmokeRunnerState *stateA = nullptr;
	ZmbSmokeRunnerState *stateB = nullptr;

	if (_bottomRunners[0]) {
		_bottomRunners[0]->activateRender();
		stateA = findRunnerState(_bottomRunners[0]);
		if (stateA) {
			stateA->attrCyclePos = 4;
			for (int16 i = 0; i < 4; ++i) {
				if (_attrDisplayTable[4 * i])
					stateA->attrs[0] = _attrDisplayTable[4 * i];
				if (_attrDisplayTable[4 * i + 1])
					stateA->attrs[1] = _attrDisplayTable[4 * i + 1];
				if (_attrDisplayTable[4 * i + 2])
					stateA->attrs[2] = _attrDisplayTable[4 * i + 2];
				if (_attrDisplayTable[4 * i + 3])
					stateA->attrs[3] = _attrDisplayTable[4 * i + 3];
			}
		}
	}

	if (_bottomRunners[1]) {
		_bottomRunners[1]->activateRender();
		stateB = findRunnerState(_bottomRunners[1]);
		if (stateB) {
			stateB->attrCyclePos = 4;
			for (int16 j = 7; j > 3; --j) {
				if (_attrDisplayTable[4 * j])
					stateB->attrs[0] = _attrDisplayTable[4 * j];
				if (_attrDisplayTable[4 * j + 1])
					stateB->attrs[1] = _attrDisplayTable[4 * j + 1];
				if (_attrDisplayTable[4 * j + 2])
					stateB->attrs[2] = _attrDisplayTable[4 * j + 2];
				if (_attrDisplayTable[4 * j + 3])
					stateB->attrs[3] = _attrDisplayTable[4 * j + 3];
			}
		}
	}

	if (!stateA || !stateB)
		return 0;

	if (stateA->attrs[0] != stateB->attrs[0])
		return 2;
	if (stateA->attrs[1] != stateB->attrs[1])
		return 2;
	if (stateA->attrs[2] != stateB->attrs[2])
		return 2;
	if (stateA->attrs[3] != stateB->attrs[3])
		return 2;
	return 0;
}

void ZoombiniPuzzleSmoke::initMatchCompareRunners() {
	// IDA: smoke_initMatchCompareRunners (0x44C739)
	_compareIdx = compareTwoOrderLines();
	_bCompareSwapped = (_compareIdx != 0);

	if (_compareAFeature)
		loadScrbOntoFeature(_compareAFeature, 11018, true);
	if (_compareBFeature)
		loadScrbOntoFeature(_compareBFeature, 11019, true);
}

void ZoombiniPuzzleSmoke::startNextCompareSequence() {
	// IDA: smoke_startNextCompareSequence (0x44C91A)
	_displayPairIdx = 0;

	ZmbFeature *stackFeature = _bRunnerToggle ? _smokeStackBFeature : _smokeStackAFeature;
	if (stackFeature) {
		// Set flags to 0x05188000
		stackFeature->addFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00008000_LOOP_ANIM));
		if (_difficultyLevel <= kPuzzleDiffLevel2) {
			loadScrbOntoFeature(stackFeature, _scrbSmokeStackResA, false);
		} else {
			_bRunnerToggle = !_bRunnerToggle;
			loadScrbOntoFeature(stackFeature, _scrbSmokeStackResB, false);
		}
	}

	if (_mainAnimFeature)
		loadScrbOntoFeature(_mainAnimFeature, _scrbAnimIdArr[_compareIdx], true);

	if (_secondAnimFeature) {
		loadScrbOntoFeature(_secondAnimFeature, _scrbAnimIdArr[_compareIdx + 1], true);
		// IDA: runner_linkRelativeToParent — NO-OP in ScummVM
	}

	// IDA: runner_linkRelativeToParent for currentDragZmb -> secondAnim — NO-OP
}

// =========================================================================
// Question / Selection
// =========================================================================

void ZoombiniPuzzleSmoke::selectQuestionZmb() {
	// IDA: smoke_selectQuestionZmb (0x44D372)
	int16 count = 0;
	uint16 available[21] = {};

	for (int16 i = 0; i < _zmbCount; ++i) {
		if (_zmbQueue[i] != 0)
			available[count++] = _zmbQueue[i];
	}

	if (count == 0) {
		_questionResult = 0;
		return;
	}

	int16 randIdx = _vm->_rnd->getRandomNumber(count - 1);
	ZmbSnoid *snoid = getSnoid(available[randIdx]);
	if (snoid) {
		_questionAttrs[0] = snoid->_trait._head;
		_questionAttrs[1] = snoid->_trait._eye;
		_questionAttrs[2] = snoid->_trait._nose;
		_questionAttrs[3] = snoid->_trait._foot;
	}

	if (_difficultyLevel < kPuzzleDiffLevel3 || count <= 1) {
		_questionAttrs[4] = 0;
		_questionAttrs[5] = 0;
		_questionAttrs[6] = 0;
		_questionAttrs[7] = 0;
	} else {
		int16 idx2 = randIdx + 1;
		if (idx2 == count)
			idx2 = 0;
		ZmbSnoid *snoid2 = getSnoid(available[idx2]);
		if (snoid2) {
			_questionAttrs[4] = snoid2->_trait._head;
			_questionAttrs[5] = snoid2->_trait._eye;
			_questionAttrs[6] = snoid2->_trait._nose;
			_questionAttrs[7] = snoid2->_trait._foot;
		}
	}

	_questionResult = count;
}

int16 ZoombiniPuzzleSmoke::copyPairToCompareBuffer() {
	// IDA: smoke_copyPairToCompareBuffer (0x44D459)
	if (_currentZmbIdx >= _zmbCount)
		return 0;

	int16 result = 1;
	ZmbSnoid *snoid1 = getSnoid(_zmbQueue[_currentZmbIdx]);
	if (snoid1) {
		_questionAttrs[0] = snoid1->_trait._head;
		_questionAttrs[1] = snoid1->_trait._eye;
		_questionAttrs[2] = snoid1->_trait._nose;
		_questionAttrs[3] = snoid1->_trait._foot;
	}

	if (_currentZmbIdx + 1 >= _zmbCount) {
		_questionAttrs[4] = 0;
		_questionAttrs[5] = 0;
		_questionAttrs[6] = 0;
		_questionAttrs[7] = 0;
	} else {
		result = 2;
		ZmbSnoid *snoid2 = getSnoid(_zmbQueue[_currentZmbIdx + 1]);
		if (snoid2) {
			_questionAttrs[4] = snoid2->_trait._head;
			_questionAttrs[5] = snoid2->_trait._eye;
			_questionAttrs[6] = snoid2->_trait._nose;
			_questionAttrs[7] = snoid2->_trait._foot;
		}
	}

	return result;
}

// =========================================================================
// Runner initialization
// =========================================================================

void ZoombiniPuzzleSmoke::initQuestionRunners(int16 count) {
	// IDA: smoke_initQuestionRunners (0x44D510)
	if (count <= 0)
		return;

	int16 cliffCount = (_cliffRunnerCount < 8) ? _cliffRunnerCount : 8;
	int16 randTarget = (cliffCount > 0) ? _vm->_rnd->getRandomNumber(cliffCount - 1) : 0;

	for (int16 i = 0; i < cliffCount; ++i) {
		ZmbFeature *runner = _cliffRunners[i];
		if (!runner)
			continue;
		ZmbSmokeRunnerState *state = findRunnerState(runner);
		if (!state)
			continue;

		runner->activateRender();
		runner->setNeedsRedraw(true);
		state->attrCyclePos = 4;
		runner->setPointLoc(kCliffRunnerPositions[i]);

		for (int16 j = 0; j < 4; ++j)
			state->attrs[j] = _vm->_rnd->getRandomNumber(1, 4);

		state->orientation = 1;

		if (i == randTarget) {
			state->attrs[0] = _questionAttrs[0] ? _questionAttrs[0] : _vm->_rnd->getRandomNumber(1, 4);
			state->attrs[1] = _questionAttrs[1] ? _questionAttrs[1] : _vm->_rnd->getRandomNumber(1, 4);
			state->attrs[2] = _questionAttrs[2] ? _questionAttrs[2] : _vm->_rnd->getRandomNumber(1, 4);
			state->attrs[3] = _questionAttrs[3] ? _questionAttrs[3] : _vm->_rnd->getRandomNumber(1, 4);
		}
	}
}

void ZoombiniPuzzleSmoke::assignAllRunnersAttrs() {
	// IDA: smoke_assignAllRunnersAttrs (0x44D651)
	for (int16 i = 0; i < _level2RunnerCount; ++i) {
		ZmbFeature *runner = _level2Runners[i];
		if (!runner)
			continue;
		ZmbSmokeRunnerState *state = findRunnerState(runner);
		if (!state)
			continue;
		assignRunnerAttrsForLevel(i, *state);
	}
}

void ZoombiniPuzzleSmoke::assignRunnerAttrsForLevel(int16 levelIdx, ZmbSmokeRunnerState &state) {
	// IDA: smoke_assignRunnerAttrsForLevel (0x44D67C)
	// Port of the original engine logic:
	//   - Clears state.attrs[0..3].
	//   - Resets per-call shuffle tables (slot: {0..4}, value: {1..6}) with shrinking bounds.
	//   - levelIdx 0 clears _seenAttrA/B arrays (fresh start).
	//   - Outer loop runs up to 4 times; Rand budget (1-2) limits successful writes.
	//   - Level 0/1: orientation=0, pick via shuffle arrays; skip if seen already equals new.
	//   - Level 2/3: orientation=2, ~34% chance (rand(100) > 65) to reuse _seenAttrA value;
	//     otherwise pull from _questionAttrs (the persistent "question" set).
	//     Last-iteration fallback (i == 3 AND no actions taken) forces reuse when seen exists.

	// Zero runner attrs.
	for (int16 j = 0; j < 4; ++j)
		state.attrs[j] = 0;

	// Per-call shuffle tables (local — match IDA's reset at top of function).
	uint8 slotShuffle[5] = {0, 1, 2, 3, 4};
	uint8 valueShuffle[6] = {1, 2, 3, 4, 5, 6};
	int16 valueCursorBound = 4;  // IDA: word_4B1E98 (5 - 1)
	int16 slotCursorBound = 3;   // IDA: word_4B1E9C (4 - 1)

	// IDA: levelIdx==0 clears the seenAttr arrays (fresh start across the full puzzle).
	if (levelIdx == 0) {
		for (int16 j = 0; j < 4; ++j) {
			_seenAttrA[j] = 0;
			_seenAttrB[j] = 0;
		}
	}

	// Action budget: 1 or 2 successful writes per call.
	int16 randBudget = _vm->_rnd->getRandomNumber(1, 2);
	const int16 initialBudget = randBudget;

	for (int16 i = 0; i < 4 && randBudget > 0; ++i) {
		// Pick random cursor within current bounds.
		int16 valueCursor = _vm->_rnd->getRandomNumber(0, valueCursorBound);
		int16 slotCursor = _vm->_rnd->getRandomNumber(0, slotCursorBound);

		if (levelIdx < 2) {
			// Level 0/1: fresh attrs only, orientation = 0.
			state.orientation = 0;
			uint8 pickedSlot = slotShuffle[slotCursor];
			uint8 pickedValue = valueShuffle[valueCursor];
			if (_seenAttrA[pickedSlot] != pickedValue) {
				state.attrs[pickedSlot] = pickedValue;
				_level1AttrHistory[4 * levelIdx + pickedSlot] = pickedValue;
				_seenAttrA[pickedSlot] = pickedValue;
				--randBudget;
			}
		} else if (levelIdx == 2 || levelIdx == 3) {
			// Level 2/3: orientation = 2, reuse previously-seen attrs ~34% of the time.
			state.orientation = 2;
			int16 slotIdx = _vm->_rnd->getRandomNumber(0, 3);
			// IDA: if rand(100,0) > 65 (i.e. rand in [66..99], ~34%) OR last-iter fallback.
			bool reuseRoll = (_vm->_rnd->getRandomNumber(0, 99) > 65);
			bool lastIterFallback = (randBudget == initialBudget && i == 3);

			if (_seenAttrA[slotIdx]) {
				if (reuseRoll || lastIterFallback) {
					// REUSE path.
					state.attrs[slotIdx] = _seenAttrA[slotIdx];
					_level2AttrHistory[4 * levelIdx + slotIdx] = _seenAttrA[slotIdx];
					_seenAttrB[slotIdx] = _seenAttrA[slotIdx];
					--randBudget;
				}
				// Else: no action — outer loop continues without consuming budget.
			} else {
				// FRESH: draw from _questionAttrs (persistent starter set).
				uint8 starterAttr = _questionAttrs[slotIdx];
				state.attrs[slotIdx] = starterAttr;
				_level2AttrHistory[4 * levelIdx + slotIdx] = starterAttr;
				_seenAttrB[slotIdx] = starterAttr;
				--randBudget;
			}
		}

		// Shrink shuffle arrays (IDA: shift down then decrement bound).
		for (int16 j = slotCursor; j < slotCursorBound; ++j)
			slotShuffle[j] = slotShuffle[j + 1];
		--slotCursorBound;

		for (int16 k = valueCursor; k < valueCursorBound; ++k)
			valueShuffle[k] = valueShuffle[k + 1];
		--valueCursorBound;

		if (slotCursorBound < 0 || valueCursorBound < 0)
			break;
	}

	// Persist newly-seen attrs back into _questionAttrs for the next call (IDA end-of-function).
	for (int16 j = 0; j < 4; ++j) {
		if (_seenAttrA[j])
			_questionAttrs[j] = _seenAttrA[j];
	}

	state.attrCyclePos = 4;
}

void ZoombiniPuzzleSmoke::initAllRunnerAttrs(int16 param) {
	// IDA: smoke_initAllRunnerAttrs — reinit grid runners for L3-4
	for (int16 i = 1; i < _gridRunnerCount && i < 7; ++i) {
		ZmbFeature *runner = _gridRunners[i];
		if (!runner)
			continue;
		ZmbSmokeRunnerState *state = findRunnerState(runner);
		if (!state)
			continue;
		generateAttrGrid(i, *state);
	}
}

void ZoombiniPuzzleSmoke::generateAttrGrid(int16 rowIndex, ZmbSmokeRunnerState &state) {
	// IDA: smoke_generateAttrGrid (0x44E181)
	// Generates attribute grid for L3-4 puzzle logic.
	// Row 1 trigger initializes the entire grid; all rows write to state at end.

	if (rowIndex == 1) {
		memset(_attrGridPrimary, 0, sizeof(_attrGridPrimary));
		memset(_attrGridSecondary, 0, sizeof(_attrGridSecondary));
		memset(_attrGridMatchFlags, 0, sizeof(_attrGridMatchFlags));

		// Row 0: copy from question attrs
		_attrGridPrimary[0] = _questionAttrs[0];
		_attrGridPrimary[1] = _questionAttrs[1];
		_attrGridPrimary[2] = _questionAttrs[2];
		_attrGridPrimary[3] = _questionAttrs[3];

		_attrGridSecondary[0] = _questionAttrs[4];
		_attrGridSecondary[1] = _questionAttrs[5];
		_attrGridSecondary[2] = _questionAttrs[6];
		_attrGridSecondary[3] = _questionAttrs[7];

		// Generate rows 1..2 with random attrs and match probability
		for (int16 row = 1; row < 3; ++row) {
			int16 matchCount = 0;
			int16 matchDone = 0;
			int16 randTarget = _vm->_rnd->getRandomNumber(3);

			for (int16 col = 0; col < 4; ++col) {
				if (matchCount >= 2)
					continue;

				uint8 randVal = _vm->_rnd->getRandomNumber(1, 5);

				if (col == randTarget && !matchDone && _vm->_rnd->getRandomNumber(100) > 70) {
					matchDone = 1;
					uint8 src = _attrGridPrimary[4 * (row - 1) + col];
					if (!src)
						src = _questionAttrs[col];
					_attrGridPrimary[4 * row + col] = (src % 5) + 1;
					_attrGridMatchFlags[4 * row + col] = _attrGridPrimary[4 * row + col];
				} else if (_vm->_rnd->getRandomNumber(100) > 40 || (col == 3 && matchCount == 0)) {
					_attrGridPrimary[4 * row + col] = randVal;
					_attrGridMatchFlags[4 * row + col] = 0;
				}

				if (_attrGridPrimary[4 * row + col])
					++matchCount;
			}
		}

		// L3-4: extended rows 3..4
		if (_difficultyLevel >= kPuzzleDiffLevel3) {
			for (int16 row = 3; row < 5; ++row) {
				int16 matchCount = 0;
				int16 matchDone = 0;

				for (int16 col = 0; col < 4; ++col) {
					if (matchCount >= 2)
						continue;

					uint8 randVal = _vm->_rnd->getRandomNumber(1, 5);

					if (!matchDone && _vm->_rnd->getRandomNumber(100) > 70) {
						matchDone = 1;
						uint8 src = _attrGridPrimary[4 * (row - 2) + col];
						if (!src)
							src = _questionAttrs[col];
						_attrGridPrimary[4 * row + col] = src;
						_attrGridMatchFlags[4 * row + col] = _attrGridPrimary[4 * row + col];
					} else {
						_attrGridPrimary[4 * row + col] = randVal;
						_attrGridMatchFlags[4 * row + col] = 0;
					}

					if (_attrGridPrimary[4 * row + col])
						++matchCount;
				}
			}
		}

		// L3: rows 5..6
		if (_difficultyLevel == kPuzzleDiffLevel3) {
			for (int16 row = 5; row < 7; ++row) {
				int16 matchCount = 0;
				int16 randTarget2 = _vm->_rnd->getRandomNumber(3);

				for (int16 col = 0; col < 4; ++col) {
					if (matchCount >= 2)
						continue;

					uint8 randVal = _vm->_rnd->getRandomNumber(1, 5);

					if (col == randTarget2 && _vm->_rnd->getRandomNumber(100) > 70) {
						_attrGridPrimary[4 * row + col] = randVal;
						_attrGridMatchFlags[4 * row + col] = randVal;
					} else if (_vm->_rnd->getRandomNumber(100) > 40 || (col == 3 && matchCount == 0)) {
						_attrGridPrimary[4 * row + col] = randVal;
						_attrGridMatchFlags[4 * row + col] = 0;
					}

					if (_attrGridPrimary[4 * row + col])
						++matchCount;
				}
			}
		} else if (_difficultyLevel == kPuzzleDiffLevel4) {
			// L4: rows 5..6 from existing rows with mutation
			bool coinFlip = _vm->_rnd->getRandomNumber(1) != 0;
			int16 srcRow, dstRow;

			if (coinFlip) {
				srcRow = _vm->_rnd->getRandomNumber(1, 2);
				dstRow = 5;
			} else {
				srcRow = _vm->_rnd->getRandomNumber(3, 4);
				dstRow = 6;
			}

			for (int16 col = 0; col < 4; ++col) {
				if (_attrGridMatchFlags[4 * srcRow + col]) {
					_attrGridPrimary[4 * dstRow + col] = _attrGridPrimary[4 * srcRow + col];
					_attrGridMatchFlags[4 * dstRow + col] = _attrGridPrimary[4 * srcRow + col];
				} else if (_attrGridPrimary[4 * srcRow + col]) {
					_attrGridPrimary[4 * dstRow + col] = (_attrGridPrimary[4 * srcRow + col] % 5) + 1;
					_attrGridMatchFlags[4 * dstRow + col] = 0;
				}
			}

			int16 otherDst = (dstRow == 5) ? 6 : 5;
			int16 otherSrc;
			if (coinFlip)
				otherSrc = _vm->_rnd->getRandomNumber(3, 4);
			else
				otherSrc = _vm->_rnd->getRandomNumber(1, 2);

			int16 randMutateCol = _vm->_rnd->getRandomNumber(3);
			for (int16 col = 0; col < 4; ++col) {
				if (col == randMutateCol) {
					_attrGridPrimary[4 * otherDst + col] = _vm->_rnd->getRandomNumber(1, 5);
					_attrGridMatchFlags[4 * otherDst + col] = 0;
				} else if (_attrGridMatchFlags[4 * otherSrc + col]) {
					_attrGridPrimary[4 * otherDst + col] = _attrGridPrimary[4 * otherSrc + col];
					_attrGridMatchFlags[4 * otherDst + col] = _attrGridPrimary[4 * otherSrc + col];
				} else if (_attrGridPrimary[4 * otherSrc + col]) {
					_attrGridPrimary[4 * otherDst + col] = (_attrGridPrimary[4 * otherSrc + col] % 5) + 1;
					_attrGridMatchFlags[4 * otherDst + col] = 0;
				}
			}
		}

		// Row 7 (target)
		for (int16 col = 0; col < 4; ++col) {
			if (_attrGridPrimary[col + 16]) {
				_attrGridPrimary[col + 28] = _vm->_rnd->getRandomNumber(1, 5);
			} else if (_attrGridPrimary[col + 12]) {
				_attrGridPrimary[col + 28] = _attrGridPrimary[col + 12] - 1;
				if (_attrGridPrimary[col + 28] < 1)
					_attrGridPrimary[col + 28] = 5;
			} else {
				_attrGridPrimary[col + 28] = _questionAttrs[col];
			}
		}
	}

	// Write attrs for this row to the runner state
	int16 mc = 0;
	for (int16 col = 0; col < 4; ++col) {
		if (rowIndex == 8) {
			state.attrs[col] = _questionAttrs[4 + col] ? _attrGridSecondary[col] : 0;
		} else {
			state.attrs[col] = _attrGridPrimary[4 * rowIndex + col];
		}

		if (_attrGridMatchFlags[4 * rowIndex + col])
			mc = col + 1;
	}

	state.hasMatch = (mc > 0) ? 1 : 0;
	state.matchCount = mc;
}

// =========================================================================
// Rejection animation
// =========================================================================

void ZoombiniPuzzleSmoke::playRejectedAnimation() {
	// IDA: smoke_playZmbRejectedAnim (0x44CA52)
	if (_rejectionFeature) {
		loadScrbOntoFeature(_rejectionFeature, _placedZmbCount + 11036, true);
		// IDA: runner_linkRelativeToParent(background, 0, rejection) — NO-OP
	}

	ZmbSnoid *zmb = _currentDragZmb;
	if (zmb && _rejectionFeature) {
		zmb->setPointLoc(kRejectPosition);
		playZmbScript(false, nullptr, _placedZmbCount + 12020, zmb);
		// IDA: runner_linkRelativeToParent(rejection, 1, currentDragZmb) — NO-OP
	}
}

// =========================================================================
// Stack building / spawning
// =========================================================================

void ZoombiniPuzzleSmoke::buildRunnerStacks() {
	// IDA: smoke_buildRunnerStacks (0x44DBE2)
	clearAllRunnerSlots();

	switch (_difficultyLevel) {
	case kPuzzleDiffLevel1:
		spawnStackRunners((_zmbCount < 8) ? _zmbCount : 8, 1);
		spawnStackRunners(2, 4);
		spawnStackRunners(2, 5);
		break;
	case kPuzzleDiffLevel2:
		spawnStackRunners((_zmbCount < 4) ? _zmbCount : 4, 2);
		spawnStackRunners((_zmbCount < 8) ? _zmbCount : 8, 1);
		spawnStackRunners(2, 4);
		spawnStackRunners(2, 5);
		break;
	case kPuzzleDiffLevel3:
		spawnStackRunners((_zmbCount < 7) ? _zmbCount : 7, 3);
		spawnStackRunners(1, 4);
		spawnStackRunners(2, 5);
		break;
	case kPuzzleDiffLevel4:
		spawnStackRunners((_zmbCount < 8) ? _zmbCount : 8, 3);
		spawnStackRunners(1, 4);
		spawnStackRunners(2, 5);
		break;
	default:
		break;
	}
}

void ZoombiniPuzzleSmoke::spawnStackRunners(int16 count, int16 runnerType) {
	// IDA: smoke_spawnStackRunners (0x44DC7B)
	if (count <= 0)
		return;

	int16 randTarget = _vm->_rnd->getRandomNumber(count - 1);

	for (int16 i = 0; i < count; ++i) {
		Common::Point pos;
		ZmbSmokeRunnerState tempState = {};

		switch (runnerType) {
		case 1: // Cliff runners
			for (int16 j = 0; j < 4; ++j)
				tempState.attrs[j] = _vm->_rnd->getRandomNumber(1, 5);
			tempState.orientation = 1;
			tempState.attrCyclePos = 4;
			pos = kCliffRunnerPositions[i];

			if (i == randTarget) {
				tempState.attrs[0] = _questionAttrs[0] ? _questionAttrs[0] : _vm->_rnd->getRandomNumber(1, 5);
				tempState.attrs[1] = _questionAttrs[1] ? _questionAttrs[1] : _vm->_rnd->getRandomNumber(1, 5);
				tempState.attrs[2] = _questionAttrs[2] ? _questionAttrs[2] : _vm->_rnd->getRandomNumber(1, 5);
				tempState.attrs[3] = _questionAttrs[3] ? _questionAttrs[3] : _vm->_rnd->getRandomNumber(1, 5);
			}
			break;

		case 2: // Level 2 runners
			assignRunnerAttrsForLevel(i, tempState);
			pos = kCliffRunnerPositions[i];
			break;

		case 3: // Grid runners
			generateAttrGrid(i + 1, tempState);
			if (i + 1 < 7)
				tempState.orientation = 7;
			if (i == 6)
				tempState.orientation = 5;
			if (i == 7)
				tempState.orientation = 3;
			tempState.attrCyclePos = 4;
			pos = kGridRunnerPositions[i];
			break;

		case 4: // Exit runners
			memset(tempState.attrs, 0, 4);
			tempState.attrCyclePos = 4;
			if (i == 0) {
				tempState.orientation = 8;
				pos = kExitRunnerPositions[0];
			} else if (i == 1) {
				tempState.orientation = 5;
				pos = kExitRunnerPositions[1];
			}
			break;

		case 5: // Bottom runners
			memset(tempState.attrs, 0, 4);
			tempState.attrCyclePos = 4;
			pos = kBottomRunnerPositions[i];
			tempState.orientation = (i == 0) ? 5 : 3;
			break;

		default:
			continue;
		}

		ZmbFeature *runner = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 11000, 6, pos,
			ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00000002_TYPE_TOWN_ENTITY);

		if (!runner)
			continue;

		switch (runnerType) {
		case 1:
			if (_cliffRunnerCount < 20) {
				_cliffRunners[_cliffRunnerCount] = runner;
				_cliffRunnerStates[_cliffRunnerCount] = tempState;
				_cliffRunnerCount++;
			}
			// IDA: runner_linkRelativeToParent(secondAnim, 0, runner) — NO-OP
			break;

		case 2:
			if (_level2RunnerCount < 6) {
				_level2Runners[_level2RunnerCount] = runner;
				_level2RunnerStates[_level2RunnerCount] = tempState;
				_level2RunnerCount++;
			}
			// IDA: runner_linkRelativeToParent(compareA, 1, runner) — NO-OP
			break;

		case 3:
			if (_gridRunnerCount < 9) {
				_gridRunners[_gridRunnerCount] = runner;
				_gridRunnerStates[_gridRunnerCount] = tempState;

				if (_gridRunnerCount == 7 && _difficultyLevel >= kPuzzleDiffLevel3) {
					_targetZmbRunner = runner;
					assignZmbAttrsFromSrc(7, nullptr);
					cacheZmbAttrs(7, nullptr);
					_bMatchReady = true;
				}
				if (_gridRunnerCount == 8 && _difficultyLevel >= kPuzzleDiffLevel3) {
					_sourceZmbRunner = runner;
					assignZmbAttrsFromSrc(8, nullptr);
				}

				_gridRunnerCount++;
			}
			// IDA: runner_linkRelativeToParent(secondAnim, 0, runner) — NO-OP
			break;

		case 4:
			if (_exitRunnerCount < 4) {
				_exitRunners[_exitRunnerCount] = runner;
				_exitRunnerStates[_exitRunnerCount] = tempState;
				_exitRunnerCount++;
			}
			// IDA: runner_linkRelativeToParent(secondAnim, 1, runner) — NO-OP
			break;

		case 5:
			if (_bottomRunnerCount < 2) {
				_bottomRunners[_bottomRunnerCount] = runner;
				_bottomRunnerStates[_bottomRunnerCount] = tempState;
				_bottomRunnerCount++;
			}
			// IDA: runner_linkRelativeToParent(mainAnim, 1, runner) — NO-OP
			runner->deactivateRender();
			runner->setNeedsRedraw(true);
			break;
		}
	}
}

// =========================================================================
// Reset
// =========================================================================

void ZoombiniPuzzleSmoke::resetAndReinitLevel() {
	// IDA: smoke_resetAndReinitLevel (0x44BBF0)
	clearZmbAttrs(0);
	if (_difficultyLevel <= kPuzzleDiffLevel2)
		clearZmbAttrs(1);
	else
		clearZmbAttrs(7);
	clearDisplayRunners();
	clearAllRunnerSlots();

	switch (_difficultyLevel) {
	case kPuzzleDiffLevel1:
		initQuestionRunners(_questionResult);
		break;
	case kPuzzleDiffLevel2:
		assignAllRunnersAttrs();
		initQuestionRunners(_questionResult);
		break;
	case kPuzzleDiffLevel3:
		initAllRunnerAttrs(0);
		break;
	case kPuzzleDiffLevel4:
		if (_targetZmbRunner) {
			ZmbSmokeRunnerState *targetState = findRunnerState(_targetZmbRunner);
			if (targetState) {
				targetState->attrs[0] = 0;
				targetState->attrCyclePos = 2;
			}
		}
		if (_sourceZmbRunner) {
			ZmbSmokeRunnerState *srcState = findRunnerState(_sourceZmbRunner);
			if (srcState)
				srcState->attrs[0] = 0;
		}
		if (_overlayAnimFeature) {
			loadScrbOntoFeature(_overlayAnimFeature, _scrbTransitionResId, true);
			_bWord4A43B8 = false;
		}
		_transitionPhase = 3;
		break;
	default:
		break;
	}
	_answerState = 2;
}

// =========================================================================
// Drag evaluation
// =========================================================================

int16 ZoombiniPuzzleSmoke::evaluateRunnerDrop(ZmbFeature *runner, const Common::Point &dropPos) {
	// IDA: smoke_dragZmbRunner (0x44F2B0) — converted from blocking to event-driven.
	// Returns the slot index if dropped in a valid rect, -1 otherwise.

	if (_difficultyLevel >= kPuzzleDiffLevel3) {
		if (_dragSlotIdxA < 3) {
			for (int16 j = 0; j < 3; ++j) {
				int16 rectIdx = 3 * _dragSlotIdxA + j;
				if (kDragRectsA[rectIdx].contains(dropPos))
					return j;
			}
		}
		if (_dragSlotIdxB < 3) {
			for (int16 j = 0; j < 3; ++j) {
				int16 rectIdx = 3 * _dragSlotIdxB + j;
				if (kDragRectsB[rectIdx].contains(dropPos))
					return j + 3;
			}
		}
		return -1;
	}

	// L1-2
	if (kCliffDropRect.contains(dropPos) && !_bDragLocked)
		return 4;

	return -1;
}

// =========================================================================
// L4 frame transition handler
// =========================================================================

void ZoombiniPuzzleSmoke::handleFrameTransition(int16 eventCode) {
	// IDA: smoke_handleFrameTransition (0x44D281)
	switch (eventCode) {
	case 17:
		if (_sourceZmbRunner) {
			ZmbSmokeRunnerState *state = findRunnerState(_sourceZmbRunner);
			if (state) {
				--_transitionPhase;
				state->orientation = 4;
				state->attrCyclePos = 4;
			}
		}
		break;

	case 18:
		if (_sourceZmbRunner) {
			_sourceZmbRunner->setFrameInterval(0);
			ZmbSmokeRunnerState *state = findRunnerState(_sourceZmbRunner);
			if (state) {
				--_transitionPhase;
				state->orientation = 5;
				state->attrCyclePos = 4;
			}
			cacheZmbAttrs(7, nullptr);
		}
		loadZmbAttrsToCache();
		cycleZmbAttrDisplay();
		cacheAnswerRunnerAttrs();
		advanceAnswerRunnerFrames();
		initMatchCompareRunners();
		if (_bWord4A43B8)
			_bWord4A43B8 = false;
		break;

	case 19:
		initAllRunnerAttrs(0);
		_bReloadScrb = true;
		break;

	default:
		break;
	}
}

// =========================================================================
// Animation dispatch
// =========================================================================

void ZoombiniPuzzleSmoke::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// L4 overlay transitions use handleFrameTransition
	if (feature == _overlayAnimFeature && _difficultyLevel == kPuzzleDiffLevel4 && _transitionPhase < 3) {
		handleFrameTransition(eventCode);
		return;
	}

	// All other events go to the central dispatch
	processAnimDispatchEvent(feature, eventCode);
}

void ZoombiniPuzzleSmoke::processAnimDispatchEvent(ZmbFeature *feature, int16 eventCode) {
	// IDA: smoke_scrbAnimDispatch (0x44CB72)

	if (eventCode > 30) {
		if (eventCode > 38) {
			switch (eventCode) {
			case '2': // 0x32 = 50
				_bPlaceZmb = true;
				break;
			case '3': // 0x33 = 51
				_bLinkRunners = true;
				break;
			case '<': // 0x3C = 60
				_bDragLocked = false;
				break;
			case 251:
				if (feature && feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
					static_cast<ZmbSnoid *>(feature)->setBodyArrangement(1);
				break;
			default:
				break;
			}
		} else {
			switch (eventCode) {
			case 31:
				if (_smokeStackBFeature) {
					// IDA: runner_linkRelativeToParent(stackA, 1, stackB) — NO-OP
					_smokeStackBFeature->addFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00008000_LOOP_ANIM));
					_smokeStackBFeature->removeFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER));
					loadScrbOntoFeature(_smokeStackBFeature, _scrbSmokeStackResB, true);
				}
				break;

			case 35:
				if (_currentZmbIdx + 1 < _zmbCount && _zmbQueue[_currentZmbIdx + 1]) {
					ZmbSnoid *nextZmb = getSnoid(_zmbQueue[_currentZmbIdx + 1]);
					if (nextZmb) {
						nextZmb->deactivateRender();
						nextZmb->setPointLoc(kHidePosition);
						playZmbScript(false, feature, _scrbDropResId, nextZmb);
						// IDA: runner_linkRelativeToParent — NO-OP
					}
				}
				break;

			case 36:
				if (_bFirstAttrAssign) {
					_bFirstAttrAssign = false;
					assignZmbAttrsFromSrc(0, _currentDragZmb);
					cacheZmbAttrs(0, _currentDragZmb);
				}
				break;

			case 37:
				if (_currentZmbIdx < _zmbCount && _zmbQueue[_currentZmbIdx]) {
					ZmbSnoid *zmb = getSnoid(_zmbQueue[_currentZmbIdx]);
					_currentDragZmb = zmb;
					if (zmb) {
						playZmbScript(false, feature, _scrbWalkResId, zmb);
						// IDA: runner_linkRelativeToParent — NO-OP
					}
				}

				{
					ZmbFeature *toggleStack = _bRunnerToggle ? _smokeStackAFeature : _smokeStackBFeature;
					if (toggleStack) {
						toggleStack->addFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00008000_LOOP_ANIM));
						toggleStack->removeFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER));
						loadScrbOntoFeature(toggleStack, _scrbSmokeStackResB, true);
					}
				}
				break;

			case 38:
				if (_difficultyLevel == kPuzzleDiffLevel4) {
					if (_transitionPhase == 3) {
						if (_currentDragZmb) {
							assignZmbAttrsFromSrc(0, _currentDragZmb);
							cacheZmbAttrs(0, _currentDragZmb);
							loadZmbAttrsToCache();
							cycleZmbAttrDisplay();
							clearDisplayRunners();

							if (_targetZmbRunner) {
								ZmbSmokeRunnerState *targetState = findRunnerState(_targetZmbRunner);
								if (targetState)
									targetState->attrs[0] = 0;
							}

							if (_currentZmbIdx < _zmbCount && _zmbQueue[_currentZmbIdx]) {
								if (_overlayAnimFeature) {
									loadScrbOntoFeature(_overlayAnimFeature, _scrbOverlayResId, true);
									_bWord4A43B8 = false;
								}
							}
						}
					} else if (_transitionPhase == 1 && _questionResult && _placedZmbCount <= _zmbCount) {
						_answerState = 1;
						loadScrbOnAnswerRunner(11005);
					}
				}
				break;

			default:
				break;
			}
		}
		return;
	}

	if (eventCode == 30) {
		if (_currentZmbIdx < _zmbCount && _zmbQueue[_currentZmbIdx]) {
			ZmbSnoid *zmb = getSnoid(_zmbQueue[_currentZmbIdx]);
			if (zmb) {
				zmb->deactivateRender();
				_currentDragZmb = zmb;
				zmb->setPointLoc(kHidePosition);
				playZmbScript(false, feature, _scrbPickupResId, zmb);
				// IDA: runner_linkRelativeToParent — NO-OP
			}
		}
		return;
	}

	switch (eventCode) {
	case 0:
		if (feature && feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			if (feature->isRenderActivated())
				feature->deactivateRender();
			else
				feature->activateRender();
		}
		break;

	case 1:
		_bReloadMainRunner = true;
		break;

	case 2:
		initMatchCompareRunners();
		break;

	case 3:
		if (_difficultyLevel >= kPuzzleDiffLevel1 && _difficultyLevel < kPuzzleDiffLevel4)
			_bResetLevel = true;
		break;

	case 4:
		startNextCompareSequence();
		break;

	case 10:
	case 11:
	case 13:
	case 14:
		if (_currentDragZmb) {
			uint8 orient = _currentDragZmb->_trait._foot;
			uint16 scrsId = _scrbZmbAnimIdArr[_bCompareSwapped ? 1 : 0] + orient;
			playZmbScript(true, feature, scrsId, _currentDragZmb);
		}
		break;

	case 16:
		if (_bottomRunners[0]) {
			ZmbSmokeRunnerState *state0 = findRunnerState(_bottomRunners[0]);
			if (state0) {
				_bottomRunners[0]->setFrameInterval(0);
				state0->attrCyclePos = 4;
			}
		}
		if (_bottomRunners[1]) {
			ZmbSmokeRunnerState *state1 = findRunnerState(_bottomRunners[1]);
			if (state1) {
				_bottomRunners[1]->setFrameInterval(0);
				state1->attrCyclePos = 4;
			}
		}

		if (_displayPairIdx < 12) {
			Common::Point posA, posB;
			if (_bCompareSwapped) {
				posA = kDisplayPairSwappedA[_displayPairIdx];
				posB = kDisplayPairSwappedB[_displayPairIdx];
			} else {
				posA = kDisplayPairNormalA[_displayPairIdx];
				posB = kDisplayPairNormalB[_displayPairIdx];
			}

			if (_bottomRunners[0])
				_bottomRunners[0]->setPointLoc(posA);
			if (_bottomRunners[1])
				_bottomRunners[1]->setPointLoc(posB);

			++_displayPairIdx;
		}
		break;

	case 17:
		if (_currentDragZmb) {
			for (int16 i = 0; i < _zmbCount; ++i) {
				if (_zmbQueue[i] && getSnoid(_zmbQueue[i]) == _currentDragZmb && _difficultyLevel != kPuzzleDiffLevel4) {
					_zmbQueue[i] = 0;
					break;
				}
			}
			if (_compareIdx)
				_currentDragZmb = nullptr;
		}
		playRejectedAnimation();
		break;

	default:
		break;
	}
}

// =========================================================================
// Per-frame update
// =========================================================================

void ZoombiniPuzzleSmoke::onEveryFrame() {
	// IDA: smoke_onHover (0x44A62F)
	if (_processingFrame || !_puzzleActive)
		return;
	_processingFrame = true;

	// Pending Go departure
	if (_pendingGoDepart) {
		_processingFrame = false;
		return;
	}

	// --- Event flag handlers ---

	// Reload main runner
	if (_bReloadMainRunner) {
		debugC(1, kZmbDebugAnimation, "Smoke: reload main runner SCRB 11017");
		_bReloadMainRunner = false;
		if (_mainAnimFeature) {
			_mainAnimFeature->addFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE));
			_mainAnimFeature->removeFlag(ZmbFeature::FLAG_01000000_DEFER_RENDER);
			loadScrbOntoFeature(_mainAnimFeature, 11017, true);
		}
	}

	// Link runners — runner_linkRelativeToParent = all NO-OPs
	if (_bLinkRunners) {
		_bLinkRunners = false;
	}

	// Reload overlay SCRB
	if (_bReloadScrb) {
		debugC(1, kZmbDebugAnimation, "Smoke: reload overlay SCRB %d", _scrbOverlayResId);
		_bReloadScrb = false;
		if (_overlayAnimFeature) {
			loadScrbOntoFeature(_overlayAnimFeature, _scrbOverlayResId, false);
			_bShowAnswer = true;
			_answerState = 2;
			loadScrbOnAnswerRunner(11003);
			_bWord4A43B8 = true;
		}
	}

	// Reset level
	if (_bResetLevel) {
		debugC(1, kZmbDebugAnimation, "Smoke: reset level, diff=%d", _difficultyLevel);
		_bResetLevel = false;
		if (_difficultyLevel <= kPuzzleDiffLevel2) {
			_bShowAnswer = false;
			loadScrbOnWellRunner(11002);
		} else if (_difficultyLevel == kPuzzleDiffLevel3) {
			_bShowAnswer = true;
			_answerState = 2;
			loadScrbOnAnswerRunner(11003);
		}
	}

	// Place zoombini
	if (_bPlaceZmb) {
		debugC(1, kZmbDebugAnimation, "Smoke: place zmb, placed=%d/%d cliffCount=%d",
			_placedZmbCount, _zmbCount, _loadedOnCliffCount);
		_bPlaceZmb = false;

		uint16 columnScrbId = 11071 - _placedZmbCount;
		ZmbFeature *columnRunner = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), columnScrbId, 6,
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);
		if (_placedZmbCount < 20)
			_smokeColumnRunners[_placedZmbCount] = columnRunner;

		if (!_compareIdx && _currentDragZmb) {
			if (_placedZmbCount < 20)
				_zmbOnCliff[_placedZmbCount] = _currentDragZmb;

			_currentDragZmb->addFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_04000000_OVERLAY));

			if (_loadedOnCliffCount > 2) {
				// IDA smoke_onHover @ 0x44a92e:
				//   snoidScript_initAndPlay(0, 0, p_core188[1].u.s.pcStr1[3] + 12044, ...)
				// pcStr1[3] is byte +191 of the snoid runner = cFoot (0-indexed 0-4).
				// ScummVM stores _trait._foot 1-indexed (1-5), so subtract 1 to match
				// IDA's SCRS range 12044..12048 (was off-by-one as 12045..12049).
				int16 footIdx = (int16)_currentDragZmb->_trait._foot - 1;
				if (footIdx < 0) footIdx = 0;
				if (footIdx > 4) footIdx = 4;
				playZmbScript(false, nullptr, (uint16)(12044 + footIdx), _currentDragZmb);
			}

			if (++_loadedOnCliffCount == 1) {
				_bExitGateEnabled = true;
			}
		}

		// IDA: runner_linkRelativeToParent — NO-OP

		++_placedZmbCount;
		if (_placedZmbCount == _zmbCount) {
			// Count loaded snoids on cliff
			int16 loadedCount = 0;
			for (int16 c = 0; c < 20; ++c) {
				if (_zmbOnCliff[c])
					loadedCount++;
			}

			if (loadedCount == _zmbCount) {
				_bIdleAnimActive = true;
				_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem,
					_vm->_rnd->getRandomNumber(20055, 20063)));
			} else if (loadedCount < _zmbCount) {
				if (_vm->_rnd->getRandomNumber(4) > static_cast<uint16>(_difficultyLevel - 1) ||
					(_vm->_state->_f._pageFlagSmoke & 0xFFF) <= 3) {
					_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem,
						_vm->_rnd->getRandomNumber(20045, 20048)));
				}
			}
		}

		_currentDragZmb = nullptr;

		// Reload smoke stack
		if (_difficultyLevel <= kPuzzleDiffLevel2) {
			if (_smokeStackAFeature) {
				_smokeStackAFeature->addFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE));
				_smokeStackAFeature->removeFlag(
			static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER));
				loadScrbOntoFeature(_smokeStackAFeature, 11032, true);
			}
		} else {
			++_currentZmbIdx;
			ZmbFeature *toggleStack = _bRunnerToggle ? _smokeStackBFeature : _smokeStackAFeature;
			if (toggleStack)
				loadScrbOntoFeature(toggleStack, _scrbTravelResId, true);
		}

		// Update match readiness
		if (_difficultyLevel <= kPuzzleDiffLevel2)
			_bMatchReady = false;

		if (_difficultyLevel < kPuzzleDiffLevel4 || (_difficultyLevel == kPuzzleDiffLevel4 && _transitionPhase == 1)) {
			if (_difficultyLevel >= kPuzzleDiffLevel3)
				_questionResult = copyPairToCompareBuffer();
			else
				selectQuestionZmb();

			if (_questionResult && _placedZmbCount <= _zmbCount && _difficultyLevel < kPuzzleDiffLevel4) {
				_answerState = 1;
				loadScrbOnAnswerRunner(11005);
			} else {
				_answerState = 0;
				loadScrbOnWellRunner(11004);
			}
		}
	}

	// Show results
	if (_bShowResults) {
		_bShowResults = false;
		for (int16 i = 0; i < 3 && i < _placedZmbCount; ++i) {
			ZmbFeature *colRunner = _smokeColumnRunners[i];
			if (!colRunner)
				continue;

			loadScrbOntoFeature(colRunner, i + 11072, true);

			ZmbSnoid *cliffZmb = _zmbOnCliff[i];
			if (cliffZmb)
				playZmbScript(false, nullptr, i + 12041, cliffZmb);
		}
	}

	// Idle animations
	if (_bIdleAnimActive && _idleAnimCount < _zmbCount - 1) {
		if (getCurrentFrameCounter() - _lastIdleFrameTime > 30) {
			_lastIdleFrameTime = getCurrentFrameCounter();
			for (int16 j = 0; j < _zmbCount; ++j) {
				uint32 idleMask = _idlePoolState;
				uint16 randIdx = _vm->_rnd->getNonRepeatRandom(_zmbCount - 1, idleMask);
				_idlePoolState = idleMask;

				if (_zmbQueue[randIdx] && getSnoid(_zmbQueue[randIdx])) {
					ZmbSnoid *idleZmb = getSnoid(_zmbQueue[randIdx]);
					// IDA smoke_onHover @ 0x44AD61 idle-pool exclusion:
					//   if (runner == zmbOnCliff[0] || runner == word_4B1CB2
					//       || runner == word_4B1CB4) skip;
					// word_4B1CB2/B4 are the active compare-slot runners. An
					// idle animation fired on a snoid currently being compared
					// would break the compare visual.
					bool excluded = false;
					if (_zmbOnCliff[0] == idleZmb) excluded = true;
					if (_compareSlotRunnerA == idleZmb) excluded = true;
					if (_compareSlotRunnerB == idleZmb) excluded = true;
					if (idleZmb && idleZmb->isRenderActivated() && !excluded) {
						// IDA: snoidScript_initAndPlay(0, 0, byte+239 + 12044, ...)
						// byte+239 is 0-based foot trait (0..4). ScummVM stores
						// _trait._foot 1-based — subtract 1 to match SCRS range
						// 12044..12048 (was off-by-one as 12045..12049).
						int16 footIdx = (int16)idleZmb->_trait._foot - 1;
						if (footIdx < 0) footIdx = 0;
						if (footIdx > 4) footIdx = 4;
						playZmbScript(false, nullptr, (uint16)(12044 + footIdx), idleZmb);
						_idleAnimCount = (_idleAnimCount > 0) ? _idleAnimCount + 1 : 4;
						break;
					}
				}
			}
		}
	} else if (_idleAnimCount >= _zmbCount - 1) {
		_idlePoolState = 0;
		_lastIdleFrameTime = 0;
		_bIdleAnimActive = false;
		_idleAnimCount = 0;
	}

	_processingFrame = false;
}

// =========================================================================
// Click / Drag handlers
// =========================================================================

ZmbEventHandleResult ZoombiniPuzzleSmoke::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// IDA: smoke_onClickHandler (0x44AE29)

	// Let base class handle Go/Map/Help
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	if (_placedZmbCount >= _zmbCount || _bDragLocked || !_puzzleActive)
		return ZmbEventHandleResult::kPassthrough;

	// Check answer zone click
	if (_answerState == 1 && _answerZoneFeature &&
		_answerZoneFeature->getClickRect().contains(absPos)) {
		resetAndReinitLevel();
		loadScrbOnWellRunner(11004);
		return ZmbEventHandleResult::kConsumed;
	}

	if (_answerState == 2 && _bShowAnswer && _answerZoneFeature &&
		_answerZoneFeature->getClickRect().contains(absPos)) {
		_bShowAnswer = false;
		_answerState = 0;
		loadScrbOnWellRunner(11002);
		return ZmbEventHandleResult::kConsumed;
	}

	// Try to find a snoid to drag
	ZmbSnoid *clickedSnoid = findSnoidAtPoint(absPos);
	if (clickedSnoid) {
		bool onCliff = false;
		for (int16 i = 0; i < _loadedOnCliffCount; ++i) {
			if (_zmbOnCliff[i] == clickedSnoid) {
				onCliff = true;
				break;
			}
		}

		if (!onCliff && clickedSnoid->isRenderActivated()) {
			if (clickedSnoid == _currentDragZmb) {
				_currentDragZmb = nullptr;
				clearZmbAttrs(0);
				clearRunnerSlot(0);
				_bShowAnswer = false;
				loadScrbOnWellRunner(11002);
			}

			beginSnoidDrag(clickedSnoid);
			_currentDragZmb = clickedSnoid;
			assignZmbAttrsFromSrc(0, _currentDragZmb);
			cacheZmbAttrs(0, _currentDragZmb);
			if (_bMatchReady) {
				_bShowAnswer = true;
				loadScrbOnAnswerRunner(11003);
			}
			return ZmbEventHandleResult::kConsumed;
		}
	}

	// L3-4: grid runner drag
	if (_difficultyLevel >= kPuzzleDiffLevel3) {
		for (int16 m = 1; m < _gridRunnerCount && m < 7; ++m) {
			ZmbFeature *gridRunner = _gridRunners[m];
			if (!gridRunner || !gridRunner->getClickRect().contains(absPos))
				continue;

			_draggedRunner = gridRunner;
			_dragRunnerOrigPos = gridRunner->getPointLoc();
			_dragRunnerSavedInterval = gridRunner->getFrameInterval();
			_bRunnerDragActive = true;
			gridRunner->setFrameInterval(3);

			loadScrbOnWellRunner(11002);
			_bShowAnswer = false;

			for (int16 d = 0; d < 6; ++d) {
				if (_displayRunnerArr[d] == gridRunner) {
					_displayRunnerArr[d] = nullptr;
					break;
				}
			}

			return ZmbEventHandleResult::kConsumed;
		}
	}

	// L1-2: cliff runner drag
	if (_difficultyLevel <= kPuzzleDiffLevel2) {
		for (int16 k = 0; k < _cliffRunnerCount; ++k) {
			ZmbFeature *cliffRunner = _cliffRunners[k];
			if (!cliffRunner || !cliffRunner->getClickRect().contains(absPos))
				continue;

			_draggedRunner = cliffRunner;
			_dragRunnerOrigPos = cliffRunner->getPointLoc();
			_dragRunnerSavedInterval = cliffRunner->getFrameInterval();
			_bRunnerDragActive = true;
			cliffRunner->setFrameInterval(3);

			if (!_bMatchReady)
				loadTimerScrb();

			return ZmbEventHandleResult::kConsumed;
		}

		// Re-drag from match area
		if (_bMatchReady && kCliffDropRect.contains(absPos)) {
			_bShowAnswer = false;
			loadScrbOnWellRunner(11002);
			clearRunnerSlot(7);
			clearZmbAttrs(1);
			_bMatchReady = false;
			return ZmbEventHandleResult::kConsumed;
		}
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniPuzzleSmoke::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	// Handle end of non-snoid runner drag
	if (_bRunnerDragActive && _draggedRunner) {
		_bRunnerDragActive = false;
		_draggedRunner->setFrameInterval(_dragRunnerSavedInterval);

		int16 dropSlot = evaluateRunnerDrop(_draggedRunner, absPos);

		if (_difficultyLevel >= kPuzzleDiffLevel3) {
			if (dropSlot >= 0 && dropSlot < 6) {
				if (dropSlot < 3) {
					ZmbSmokeRunnerState *state = findRunnerState(_draggedRunner);
					if (state)
						state->orientation = 0;
					if (!_displayRunnerArr[dropSlot])
						_displayRunnerArr[dropSlot] = _draggedRunner;
					++_dragSlotIdxA;
					loadZmbAttrsToCache();
					cycleZmbAttrDisplay();
				} else {
					ZmbSmokeRunnerState *state = findRunnerState(_draggedRunner);
					if (state)
						state->orientation = 2;
					if (!_displayRunnerArr[dropSlot])
						_displayRunnerArr[dropSlot] = _draggedRunner;
					++_dragSlotIdxB;
					cacheAnswerRunnerAttrs();
					advanceAnswerRunnerFrames();
				}
			} else {
				_draggedRunner->setPointLoc(_dragRunnerOrigPos);
				ZmbSmokeRunnerState *state = findRunnerState(_draggedRunner);
				if (state)
					state->attrCyclePos = 4;
			}

			loadScoreDisplayScrbs();
			_bShowAnswer = true;
			loadScrbOnAnswerRunner(11003);
		} else {
			// L1-2
			if (dropSlot == 4) {
				_draggedRunner->setPointLoc(kCliffDropSnapPosition);
				ZmbSmokeRunnerState *state = findRunnerState(_draggedRunner);
				if (state)
					state->attrCyclePos = 4;
				_bMatchReady = true;
				assignZmbAttrsFromSrc(1, nullptr);
				cacheZmbAttrs(7, nullptr);
			} else {
				_draggedRunner->setPointLoc(_dragRunnerOrigPos);
				ZmbSmokeRunnerState *state = findRunnerState(_draggedRunner);
				if (state)
					state->attrCyclePos = 4;
			}

			if (_bMatchReady && _currentDragZmb) {
				_bShowAnswer = true;
				loadScrbOnAnswerRunner(11003);
			}
			unloadTimerScrb();
		}

		_draggedRunner = nullptr;
		_dragRunnerMatchIdx = -1;
		return ZmbEventHandleResult::kConsumed;
	}

	// Handle end of snoid drag
	if (_currentDragZmb) {
		endSnoidDrag(_currentDragZmb);

		Common::Point pos = _currentDragZmb->getPointLoc();
		if (kCliffDropRect.contains(pos)) {
			int16 bestX = kColumnSnapX[0];
			int16 bestDist = ABS(pos.x - bestX);
			for (int16 j = 1; j < 5; ++j) {
				int16 dist = ABS(pos.x - kColumnSnapX[j]);
				if (dist < bestDist) {
					bestDist = dist;
					bestX = kColumnSnapX[j];
				}
			}
			_currentDragZmb->setPointLoc(Common::Point(bestX, pos.y));
		}

		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniPuzzleSmoke::onMouseMove(const Common::Point &absPos, const Common::Point &relPos) {
	if (_bRunnerDragActive && _draggedRunner) {
		_draggedRunner->setPointLoc(absPos);
		_draggedRunner->setNeedsRedraw(true);
		return ZmbEventHandleResult::kConsumed;
	}
	return ZmbEventHandleResult::kPassthrough;
}
} // End of namespace Mohawk
