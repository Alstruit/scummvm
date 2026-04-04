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
#include "mohawk/zoombini_pages/smoke.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions from 0x4A4368 (20 POINTS)
const Common::Point ZoombiniInteractiveSmoke::kSnoidPositions[20] = {
	Common::Point(214, 128), Common::Point(175, 126), Common::Point(135, 127), Common::Point( 94, 126),
	Common::Point( 53, 128), Common::Point(237, 176), Common::Point(196, 177), Common::Point(150, 178),
	Common::Point(110, 176), Common::Point( 69, 178), Common::Point(234,  36), Common::Point(195,  37),
	Common::Point(155,  36), Common::Point(114,  35), Common::Point( 73,  38), Common::Point(237,  79),
	Common::Point(196,  78), Common::Point(150,  80), Common::Point(110,  78), Common::Point( 69,  79),
};

// IDA: DRAW_ON_REG position at stru_4A400C
const Common::Point ZoombiniInteractiveSmoke::kDrawOnRegPosition = Common::Point(43, 258);

// Drop zone positions for levels 1-2 (single zone) and levels 3-4 (3x3 grid)
// IDA: hardcoded in smoke_dragZmbRunner
static const Common::Rect kDropZoneLevel12 = Common::Rect(300, 200, 400, 300);
static const Common::Point kDropZoneGridOffset = Common::Point(25, 31);

ZoombiniInteractiveSmoke::ZoombiniInteractiveSmoke(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kSmoke) {
}

ZoombiniInteractiveSmoke::~ZoombiniInteractiveSmoke() {
}

void ZoombiniInteractiveSmoke::open() {
	openArchive(ZMB_MHK_SMOKE);
}

void ZoombiniInteractiveSmoke::setBackgroundMusic() {
	// IDA: smoke_init (0x44983c) has no music playback call on page load.
	// sound_activeHandle = random(20067,20066) is stored at end of funcInit for F1 replay only.
}

void ZoombiniInteractiveSmoke::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniInteractiveSmoke::loadFeatures() {
	// IDA: smoke_init (0x44983c)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel() + 1;
	if (_difficultyLevel > 4)
		_difficultyLevel = 4;

	// IDA: smoke_assignRunnerAttrsForLevel (0x44D67C) — orientation and history setup
	// Levels 1-2: orientation=0; Levels 3-4: orientation=2
	_orientation = (_difficultyLevel < 3) ? 0 : 2;

	// At level 1, clear the seen attribute history arrays
	// IDA: if (!levelIdx) { smoke_seenAttrA[i] = 0; smoke_seenAttrB[i] = 0; }
	if (_difficultyLevel == 1) {
		for (int i = 0; i < 4; i++) {
			_seenAttrA[i] = 0;
			_seenAttrB[i] = 0;
		}
	}
	debugC(kZmbDebugPage, "Smoke: difficultyLevel=%d, orientation=%d", _difficultyLevel, _orientation);

	// Initialize all gameplay state
	_puzzleActive = false;
	_processingFrame = false;
	_zmbCount = 0;
	_currentZmbIdx = 0;
	_placedZmbCount = 0;
	_loadedOnCliffCount = 0;
	_answerState = 2;
	_bShowAnswer = false;
	_compareIdx = 0;
	_bCompareSwapped = false;
	_transitionPhase = (_difficultyLevel == 4) ? 3 : 0;

	// Clear event flags
	_bPlaceZmb = false;
	_bLinkRunners = false;
	_bReloadScrb = false;
	_bResetLevel = false;
	_bShowResults = false;
	_bReloadMainRunner = false;
	_celebrationActive = false;
	_interactionLocked = false;
	_currentDragZmb = nullptr;
	_bRunnerToggle = false;
	_bExitGateEnabled = false;
	_exitAnimActive = false;
	_exitAnimStep = 0;
	_celebrationsPlayed = 0;

	// Clear runner arrays
	_smokeColumnCount = 0;
	_cliffRunnerCount = 0;
	_level2RunnerCount = 0;
	_gridRunnerCount = 0;
	_exitRunnerCount = 0;
	_bottomRunnerCount = 0;
	_zmbQueueSize = 0;
	_dropZoneCount = 0;

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

	// Clear attribute grids
	memset(_attrGridPrimary, 0, sizeof(_attrGridPrimary));
	memset(_attrGridSecondary, 0, sizeof(_attrGridSecondary));
	memset(_attrGridMatchFlags, 0, sizeof(_attrGridMatchFlags));
	memset(_runnerAttrs, 0, sizeof(_runnerAttrs));
	memset(_runnerMatchCount, 0, sizeof(_runnerMatchCount));
	memset(_runnerHasMatch, 0, sizeof(_runnerHasMatch));
	memset(_permutation, 0, sizeof(_permutation));
	memset(_level1AttrHistory, 0, sizeof(_level1AttrHistory));
	memset(_level2AttrHistory, 0, sizeof(_level2AttrHistory));
	memset(_questionAttrs, 0, sizeof(_questionAttrs));

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(0x64u)
	loadTerrainBitmap(100);

	// Preload shape images at tBMP 10000 (0x2710)
	// IDA: shape_loadSubShapesFromArchive(&stru_4B1D0C, 0x2710u)
	_vm->_gfx->preloadImage(10000);
	_vm->_gfx->preloadImage(11000);

	// Load REGS resources
	// IDA: regs_loadAndByteSwap(&dword_4B1D24, 0x2710u) — REGS 10000
	// IDA: regs_loadAndByteSwap(&dword_4B1D28, 0x2711u) — REGS 10001

	// Feature groups — single main SCRB at 11000
	// IDA: scrb_useFeatureGroup(0, 0, 11000)
	// IDA: scrb_loadMainFeatureSet(78, 11000)
	createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// Load reject pool: 1 at SCRS 11999
	// IDA: scrs_loadRejectPool(0, 1, 11999)
	loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 10000),
			  11999,
			  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);

	// Load normal pool: 50 at SCRS 12000
	// IDA: scrs_loadNormalPool(0, 50, 12000)
	for (uint16 i = 0; i < 50; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 10000),
				  12000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// === Set up difficulty-dependent SCRB IDs ===
	uint16 scrbOverlayResId = (_difficultyLevel == 4) ? 11011 : 11013;
	uint16 scrbAnimId0, scrbAnimId1;
	if (_difficultyLevel <= 2) {
		scrbAnimId0 = 11024;
		scrbAnimId1 = 11025;
		_scrbSmokeStackResA = 11032;
		_scrbSmokeStackResB = 0;
		// Animation ID arrays for levels 1-2
		_scrbAnimIdArr[0] = 11024;
		_scrbAnimIdArr[1] = 11025;
		_scrbAnimIdArr[2] = 11026;
		_scrbAnimIdArr[3] = 11027;
		_scrbZmbAnimIdArr[0] = 11999;
		_scrbZmbAnimIdArr[1] = 12004;
	} else {
		scrbAnimId0 = 11028;
		scrbAnimId1 = 11029;
		_scrbSmokeStackResA = 11033;
		_scrbSmokeStackResB = 11034;
		// Animation ID arrays for levels 3-4
		_scrbAnimIdArr[0] = 11028;
		_scrbAnimIdArr[1] = 11029;
		_scrbAnimIdArr[2] = 11030;
		_scrbAnimIdArr[3] = 11031;
		_scrbZmbAnimIdArr[0] = 12009;
		_scrbZmbAnimIdArr[1] = 12014;
	}

	// === Additional feature runners from IDA smoke_init ===

	// IDA: smoke_scrbOverlayAnim — overlay animation, interval=10
	_overlayAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), scrbOverlayResId, 10,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbLevel12Extra — SCRB 11076, diff 1/2 only, interval=10
	if (_difficultyLevel <= 2) {
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

	// IDA: smoke_scrbMainAnim — main animation, interval=6
	_mainAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), scrbAnimId0, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbSmokeStackA — smoke stack animation, interval=6
	_smokeStackAFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), _scrbSmokeStackResA, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbSmokeStackB — diff 3/4 only, SCRB 11034
	if (_difficultyLevel >= 3) {
		_smokeStackBFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 11034, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// IDA: smoke_scrbSecondAnim — second animation, interval=6
	_secondAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), scrbAnimId1, 6,
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

	// Load Zoombinis at 20 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, pointsArr_4A4368, 20)
	loadZoombinisFromPack();

	// Build attribute runner stacks for the current difficulty
	// IDA: smoke_buildRunnerStacks(zmbCount, level)
	buildRunnerStacks();

	// IDA: scrb_drawOnRegRunnerIdxArr[0] — SCRB 11001, diff < 3 only, interval=7
	if (_difficultyLevel < 3) {
		_drawOnRegFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 11001, 7,
			kDrawOnRegPosition,
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER);
	}

	// IDA: SHPL_copyPaletteSrcToDst(236, 10)

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(10000);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagSmoke);

	// IDA: sound_activeHandle = nextRand(20066, 20067) — smoke narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, _vm->_rnd->getRandomNumber(20066, 20067));

	// Calculate remaining exit steps
	// IDA: smoke_remainingExitSteps = smoke_columnCount + (difficulty==3) + 7
	_remainingExitSteps = _smokeColumnCount + ((_difficultyLevel == 3) ? 1 : 0) + 7;

	// Select question Zoombini for levels 1-2
	if (_difficultyLevel <= 2) {
		selectQuestionZmb();
	} else {
		copyPairToCompareBuffer();
	}

	// For levels 3-4: show answer display
	if (_difficultyLevel >= 3) {
		loadScrbOnAnswerRunner(11003);
		_bShowAnswer = true;
	}

	// Set idle animation max
	_celebrationTarget = 3;
	_nextCelebrationFrame = getCurrentFrameCounter() + 120;

	_puzzleActive = true;
}

void ZoombiniInteractiveSmoke::onGoButtonActivated() {
	// IDA: smoke_onClickHandler case 2
	// Route 4: Smoke -> Maze (via Xfer)
	// NOTE: Original uses SND_0 (no departure SFX).
	_departXferSrcSiPage = ZMB_SI_SMOKE_15;
	_pendingGoDepart = true;
}

void ZoombiniInteractiveSmoke::loadZoombinisFromPack() {
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
// Rule generation and stack building
// =========================================================================

void ZoombiniInteractiveSmoke::buildRunnerStacks() {
	// IDA: smoke_buildRunnerStacks (0x44DBE2)
	// Dispatches to spawnStackRunners with per-difficulty configurations.
	
	switch (_difficultyLevel) {
	case 1:
		// Level 1: cliff(8) + exit(2) + bottom(2)
		spawnStackRunners(8, 1);   // Cliff runners
		spawnStackRunners(2, 4);   // Exit runners
		spawnStackRunners(2, 5);   // Bottom runners
		break;
		
	case 2:
		// Level 2: level2(4) + cliff(8) + exit(2) + bottom(2)
		spawnStackRunners(4, 2);   // Level 2 runners
		spawnStackRunners(8, 1);   // Cliff runners
		spawnStackRunners(2, 4);   // Exit runners
		spawnStackRunners(2, 5);   // Bottom runners
		break;
		
	case 3:
		// Level 3: grid(7) + exit(1) + bottom(2)
		spawnStackRunners(7, 3);   // Grid runners
		spawnStackRunners(1, 4);   // Exit runners
		spawnStackRunners(2, 5);   // Bottom runners
		break;
		
	case 4:
	default:
		// Level 4: grid(8) + exit(1) + bottom(2)
		spawnStackRunners(8, 3);   // Grid runners
		spawnStackRunners(1, 4);   // Exit runners
		spawnStackRunners(2, 5);   // Bottom runners
		break;
	}
	
	// Generate attributes for all runners
	generateRunnerAttributes();
}

void ZoombiniInteractiveSmoke::spawnStackRunners(int16 count, int16 runnerType) {
	// IDA: smoke_spawnStackRunners (0x44DC7B)
	// Creates runner features of specified type.
	
	for (int16 i = 0; i < count; i++) {
		// SCRB IDs based on runner type
		uint16 scrbId;
		uint32 flags = ZmbFeature::FLAG_00008000_LOOP_ANIM |
		               ZmbFeature::FLAG_00100000_PLAY_ONCE |
		               ZmbFeature::FLAG_04000000_OVERLAY;
		
		switch (runnerType) {
		case 1:  // Cliff runners
			scrbId = 11040 + _cliffRunnerCount;
			break;
		case 2:  // Level 2 runners
			scrbId = 11050 + _level2RunnerCount;
			break;
		case 3:  // Grid runners
			scrbId = 11060 + _gridRunnerCount;
			break;
		case 4:  // Exit runners
			scrbId = 11070 + _exitRunnerCount;
			flags |= ZmbFeature::FLAG_00080000_DEFER_ANIM;
			break;
		case 5:  // Bottom runners
			scrbId = 11074 + _bottomRunnerCount;
			flags |= ZmbFeature::FLAG_00080000_DEFER_ANIM |
			         ZmbFeature::FLAG_01000000_DEFER_RENDER;
			break;
		default:
			continue;
		}
		
		ZmbFeature *feature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), scrbId, 6, flags);
		
		if (!feature)
			continue;
		
		// Add to appropriate array
		switch (runnerType) {
		case 1:
			if (_cliffRunnerCount < 20)
				_cliffRunners[_cliffRunnerCount++] = feature;
			break;
		case 2:
			if (_level2RunnerCount < 6)
				_level2Runners[_level2RunnerCount++] = feature;
			break;
		case 3:
			if (_gridRunnerCount < 9)
				_gridRunners[_gridRunnerCount++] = feature;
			break;
		case 4:
			if (_exitRunnerCount < 4)
				_exitRunners[_exitRunnerCount++] = feature;
			break;
		case 5:
			if (_bottomRunnerCount < 2)
				_bottomRunners[_bottomRunnerCount++] = feature;
			break;
		}
		
		// Track in smoke column array
		if (_smokeColumnCount < 20) {
			_smokeColumnRunners[_smokeColumnCount++] = feature;
		}
	}
}

void ZoombiniInteractiveSmoke::generateRunnerAttributes() {
	// IDA: smoke_assignRunnerAttrsForLevel / smoke_generateAttrGrid
	// Generates attribute patterns for runners based on difficulty.
	
	// For levels 1-2: Simple random assignment with one guaranteed match
	// For levels 3-4: Full 9-row attribute grid with propagation
	
	if (_difficultyLevel <= 2) {
		// Simple attribute generation
		int16 matchRunner = _vm->_rnd->getRandomNumber(0, _cliffRunnerCount - 1);
		
		for (int16 i = 0; i < _cliffRunnerCount; i++) {
			if (i == matchRunner && _zmbCount > 0) {
				// This runner gets the question Zoombini's attributes
				_runnerAttrs[i][0] = _questionAttrs[0];
				_runnerAttrs[i][1] = _questionAttrs[1];
				_runnerAttrs[i][2] = _questionAttrs[2];
				_runnerAttrs[i][3] = _questionAttrs[3];
				_runnerHasMatch[i] = true;
			} else {
				// Random attributes 1-5
				_runnerAttrs[i][0] = _vm->_rnd->getRandomNumber(1, 5);
				_runnerAttrs[i][1] = _vm->_rnd->getRandomNumber(1, 5);
				_runnerAttrs[i][2] = _vm->_rnd->getRandomNumber(1, 5);
				_runnerAttrs[i][3] = _vm->_rnd->getRandomNumber(1, 5);
				_runnerHasMatch[i] = false;
			}
		}
	} else {
		// Full attribute grid generation for levels 3-4
		// Row 0: Copy from question pair
		for (int col = 0; col < 4; col++) {
			_attrGridPrimary[col] = _questionAttrs[col];
			_attrGridSecondary[col] = _questionAttrs[4 + col];
		}
		
		// Rows 1-8: Generate with match propagation
		for (int row = 1; row < 9; row++) {
			for (int col = 0; col < 4; col++) {
				int gridIdx = row * 4 + col;
				
				// 70% chance of matching (increment prev row, wrap 5->1)
				if (_vm->_rnd->getRandomNumber(1, 100) <= 70) {
					int prevIdx = (row - 1) * 4 + col;
					_attrGridPrimary[gridIdx] = (_attrGridPrimary[prevIdx] % 5) + 1;
					_attrGridSecondary[gridIdx] = (_attrGridSecondary[prevIdx] % 5) + 1;
					_attrGridMatchFlags[gridIdx] = 1;
				} else {
					// Random filler
					_attrGridPrimary[gridIdx] = _vm->_rnd->getRandomNumber(1, 5);
					_attrGridSecondary[gridIdx] = _vm->_rnd->getRandomNumber(1, 5);
					_attrGridMatchFlags[gridIdx] = 0;
				}
			}
		}
		
		// Assign grid attributes to grid runners
		for (int16 i = 0; i < _gridRunnerCount && i < 9; i++) {
			for (int col = 0; col < 4; col++) {
				_runnerAttrs[i][col] = _attrGridPrimary[i * 4 + col];
			}
			_runnerMatchCount[i] = 0;
			for (int col = 0; col < 4; col++) {
				if (_attrGridMatchFlags[i * 4 + col])
					_runnerMatchCount[i]++;
			}
			_runnerHasMatch[i] = (_runnerMatchCount[i] > 0);
		}
	}
	
	// Set up drop zones based on runner positions
	// For levels 1-2: single drop zone
	// For levels 3-4: 3x3 grid per smoke stack
	if (_difficultyLevel <= 2) {
		_dropZoneRects[0] = kDropZoneLevel12;
		_dropZoneCount = 1;
	} else {
		// Create 3x3 grid drop zones
		// IDA: smoke_dragZmbRunner checks 3x3 grid per stack
		int16 baseX = 300;
		int16 baseY = 200;
		int16 cellW = 50;
		int16 cellH = 50;
		
		_dropZoneCount = 0;
		for (int row = 0; row < 3 && _dropZoneCount < 9; row++) {
			for (int col = 0; col < 3 && _dropZoneCount < 9; col++) {
				_dropZoneRects[_dropZoneCount++] = Common::Rect(
					baseX + col * cellW,
					baseY + row * cellH,
					baseX + (col + 1) * cellW,
					baseY + (row + 1) * cellH);
			}
		}
	}
}

void ZoombiniInteractiveSmoke::selectQuestionZmb() {
	// IDA: smoke_selectQuestionZmb (0x44D372)
	// Pick a random Zoombini as the "question" template.
	
	if (_zmbCount == 0)
		return;
	
	int16 pickIdx = _vm->_rnd->getRandomNumber(0, _zmbCount - 1);
	ZmbSnoid *snoid = getSnoid(_zmbQueue[pickIdx]);
	
	if (snoid) {
		_questionAttrs[0] = snoid->_trait._head;
		_questionAttrs[1] = snoid->_trait._eye;
		_questionAttrs[2] = snoid->_trait._nose;
		_questionAttrs[3] = snoid->_trait._foot;
		_questionResult = pickIdx;
	}
}

int16 ZoombiniInteractiveSmoke::copyPairToCompareBuffer() {
	// IDA: smoke_copyPairToCompareBuffer (0x44D459)
	// Copy attributes from current pair of Zoombinis.
	
	int16 result = 0;
	
	if (_currentZmbIdx < _zmbCount) {
		ZmbSnoid *snoid1 = getSnoid(_zmbQueue[_currentZmbIdx]);
		if (snoid1) {
			_questionAttrs[0] = snoid1->_trait._head;
			_questionAttrs[1] = snoid1->_trait._eye;
			_questionAttrs[2] = snoid1->_trait._nose;
			_questionAttrs[3] = snoid1->_trait._foot;
			result = 1;
		}
	}
	
	if (_currentZmbIdx + 1 < _zmbCount) {
		ZmbSnoid *snoid2 = getSnoid(_zmbQueue[_currentZmbIdx + 1]);
		if (snoid2) {
			_questionAttrs[4] = snoid2->_trait._head;
			_questionAttrs[5] = snoid2->_trait._eye;
			_questionAttrs[6] = snoid2->_trait._nose;
			_questionAttrs[7] = snoid2->_trait._foot;
			result = 2;
		}
	}
	
	return result;
}

void ZoombiniInteractiveSmoke::loadScrbOnAnswerRunner(uint16 scrbId) {
	// IDA: smoke_loadSCRBOnAnswerRunner (0x44BA3D)
	if (_answerZoneFeature) {
		loadScrbOntoFeature(_answerZoneFeature, scrbId);
	}
}

void ZoombiniInteractiveSmoke::reloadScrbAnimation(ZmbFeature *feature, uint16 scrbId) {
	if (feature) {
		loadScrbOntoFeature(feature, scrbId);
	}
}

// =========================================================================
// Gameplay methods
// =========================================================================

int16 ZoombiniInteractiveSmoke::getDropZoneAtPoint(const Common::Point &pos) const {
	for (int16 i = 0; i < _dropZoneCount; i++) {
		if (_dropZoneRects[i].contains(pos.x, pos.y))
			return i;
	}
	return -1;
}

bool ZoombiniInteractiveSmoke::testColumnMatch(int16 zmbIdx, int16 columnIdx) const {
	// Check if Zoombini's attributes match the column runner's attributes
	if (zmbIdx < 0 || zmbIdx >= _zmbCount)
		return false;
	if (columnIdx < 0 || columnIdx >= _smokeColumnCount)
		return false;
	
	ZmbSnoid *snoid = const_cast<ZoombiniInteractiveSmoke *>(this)->getSnoid(_zmbQueue[zmbIdx]);
	if (!snoid)
		return false;
	
	// For simple matching: check if all 4 attributes match the runner
	uint8 zmbAttrs[4] = {
		static_cast<uint8>(snoid->_trait._head),
		static_cast<uint8>(snoid->_trait._eye),
		static_cast<uint8>(snoid->_trait._nose),
		static_cast<uint8>(snoid->_trait._foot)
	};
	
	// Check attribute match (any matching attribute is sufficient for levels 1-2)
	// For levels 3-4, need to check grid matching
	if (_difficultyLevel <= 2) {
		for (int i = 0; i < 4; i++) {
			if (zmbAttrs[i] == _runnerAttrs[columnIdx][i])
				return true;
		}
		return false;
	} else {
		// Levels 3-4: Return whether this is the correct column for the Zoombini
		return _runnerHasMatch[columnIdx];
	}
}

void ZoombiniInteractiveSmoke::placeZoombiniOnColumn() {
	// IDA: smoke_bPlaceZmb handler
	// Places current drag Zoombini on the smoke column.
	
	if (!_currentDragZmb)
		return;
	
	// Create smoke column runner for the placed Zoombini
	uint16 columnScrbId = 11071 - _placedZmbCount;
	if (_smokeColumnCount < 20) {
		ZmbFeature *columnRunner = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), columnScrbId, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);
		
		if (columnRunner) {
			_smokeColumnRunners[_smokeColumnCount++] = columnRunner;
		}
	}
	
	// Add to cliff array
	if (_loadedOnCliffCount < 20) {
		_zmbOnCliff[_loadedOnCliffCount++] = _currentDragZmb;
	}
	
	// Play snoid script for entering smoke
	uint16 scrsId = 12044 + (_placedZmbCount % 6);
	Common::SeekableReadStream *scrsStream =
		_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, scrsId));
	if (scrsStream) {
		_currentDragZmb->startScrsPlayback(scrsStream, true, false);
	}
	
	_placedZmbCount++;
	
	// Check if all Zoombinis have been placed
	if (_placedZmbCount >= _zmbCount) {
		// All placed — trigger celebration animation mode
		_celebrationActive = true;
		_bExitGateEnabled = true;
		setGoButtonsEnabled(true);
		
		// Play success sound
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 
			_vm->_rnd->getRandomNumber(20055, 20063)));
	}
	
	_currentDragZmb = nullptr;
}

void ZoombiniInteractiveSmoke::playRejectedAnimation() {
	// IDA: smoke_playZmbRejectedAnim (0x44CA52)
	
	if (!_currentDragZmb)
		return;
	
	// Load rejection SCRB
	uint16 rejectScrbId = 11036 + _placedZmbCount;
	reloadScrbAnimation(_rejectionFeature, rejectScrbId);
	
	// Play rejection snoid script
	uint16 scrsId = 12020 + (_placedZmbCount % 6);
	Common::SeekableReadStream *scrsStream =
		_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, scrsId));
	if (scrsStream) {
		_currentDragZmb->startScrsPlayback(scrsStream, false, true);
	}
	
	// Play rejection sound
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20048));
	
	_currentDragZmb = nullptr;
}

void ZoombiniInteractiveSmoke::startNextCompareSequence() {
	// IDA: smoke_startNextCompareSequence (0x44C91A)
	// Starts the next comparison display cycle.
	
	_compareIdx = 0;
	
	// Toggle smoke stack A/B
	_bRunnerToggle = !_bRunnerToggle;
	
	// Load appropriate smoke stack SCRB
	if (_bRunnerToggle && _smokeStackBFeature) {
		reloadScrbAnimation(_smokeStackBFeature, _scrbSmokeStackResB);
	} else {
		reloadScrbAnimation(_smokeStackAFeature, _scrbSmokeStackResA);
	}
	
	// Load animation SCRBs
	reloadScrbAnimation(_mainAnimFeature, _scrbAnimIdArr[_compareIdx % 4]);
	reloadScrbAnimation(_secondAnimFeature, _scrbAnimIdArr[(_compareIdx + 1) % 4]);
}

void ZoombiniInteractiveSmoke::resetAndReinitLevel() {
	// IDA: smoke_resetAndReinitLevel (0x44BBF0)
	// Resets the level for the next round of Zoombinis.
	
	_placedZmbCount = 0;
	_loadedOnCliffCount = 0;
	_currentZmbIdx = 0;
	_celebrationActive = false;
	_celebrationsPlayed = 0;
	
	// Clear cliff array
	for (int i = 0; i < 20; i++) {
		_zmbOnCliff[i] = nullptr;
	}
	
	// Clear event flags
	_bPlaceZmb = false;
	_bLinkRunners = false;
	_bReloadScrb = false;
	_bResetLevel = false;
	_bShowResults = false;
	_bReloadMainRunner = false;
	
	// For levels 3-4, hide answer display
	if (_difficultyLevel >= 3) {
		_bShowAnswer = false;
		_answerState = 0;
	}
}

void ZoombiniInteractiveSmoke::handleFrameTransition() {
	// IDA: smoke_handleFrameTransition (0x44D281)
	// Level 4 multi-phase transition handling.
	
	if (_difficultyLevel != 4)
		return;
	
	_transitionPhase--;
	
	if (_transitionPhase <= 0) {
		// Reset transition phase
		_transitionPhase = 3;
		
		// Reinitialize runner attributes
		generateRunnerAttributes();
		
		_bReloadScrb = true;
	}
}

// =========================================================================
// Event handling
// =========================================================================

ZmbEventHandleResult ZoombiniInteractiveSmoke::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// Let base class handle button clicks (Go/Map/Help)
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;
	
	// Don't allow interaction while locked
	if (_interactionLocked || !_puzzleActive)
		return ZmbEventHandleResult::kPassthrough;
	
	// Check for click on Zoombini to start selection
	ZmbSnoid *snoid = findSnoidAtPoint(absPos);
	if (snoid) {
		// Don't select snoids that are playing scripts
		SnoidAnimState state = snoid->getAnimState();
		if (state == kSnoidAnimScriptReject || state == kSnoidAnimScriptNormal)
			return ZmbEventHandleResult::kPassthrough;
		
		_currentDragZmb = snoid;
		_interactionLocked = true;
		
		// For smoke puzzle, clicking a Zoombini starts the placement sequence
		// The actual placement happens when clicking on a drop zone
		return ZmbEventHandleResult::kConsumed;
	}
	
	// Check for click on smoke column (drop zone)
	if (_currentDragZmb) {
		int16 dropZone = getDropZoneAtPoint(absPos);
		if (dropZone >= 0) {
			// Test if this is a valid placement
			int16 zmbIdx = 0;
			for (int16 i = 0; i < _zmbQueueSize; i++) {
				if (getSnoid(_zmbQueue[i]) == _currentDragZmb) {
					zmbIdx = i;
					break;
				}
			}
			
			if (testColumnMatch(zmbIdx, dropZone)) {
				// Correct placement
				_bPlaceZmb = true;
			} else {
				// Wrong placement — play rejection
				playRejectedAnimation();
				_interactionLocked = false;
			}
			
			return ZmbEventHandleResult::kConsumed;
		}
	}
	
	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniInteractiveSmoke::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	// Smoke uses click-to-place, not drag-and-drop, so button up just passes through
	return ZoombiniInteractive::onLButtonUp(absPos, relPos);
}

void ZoombiniInteractiveSmoke::onEveryFrame() {
	if (_processingFrame || !_puzzleActive)
		return;
	_processingFrame = true;
	
	// [0] Pending Go departure: skip normal logic
	if (_pendingGoDepart) {
		_processingFrame = false;
		return;
	}
	
	// [1] Process event flags set by animation callbacks
	if (_bPlaceZmb) {
		_bPlaceZmb = false;
		placeZoombiniOnColumn();
		_interactionLocked = false;
	}
	
	if (_bReloadMainRunner) {
		_bReloadMainRunner = false;
		reloadScrbAnimation(_mainAnimFeature, 11017);
	}
	
	if (_bLinkRunners) {
		_bLinkRunners = false;
		// Link holding area to rejection runner, chain current drag zmb
		// (linking handled by animation system in original)
	}
	
	if (_bReloadScrb) {
		_bReloadScrb = false;
		// Reload overlay SCRB, show answer, reinitialize level
		if (_difficultyLevel >= 3) {
			loadScrbOnAnswerRunner(11003);
			_bShowAnswer = true;
		}
	}
	
	if (_bResetLevel) {
		_bResetLevel = false;
		resetAndReinitLevel();
	}
	
	if (_bShowResults) {
		_bShowResults = false;
		// Show result animations for smoke columns
	}
	
	// [2] Process exit animation
	if (_exitAnimActive) {
		_exitAnimStep++;
		if (_exitAnimStep >= _remainingExitSteps) {
			_exitAnimActive = false;
			// Assign next Zoombini to column
			if (_currentZmbIdx < _zmbCount) {
				_currentZmbIdx++;
				if (_difficultyLevel >= 3) {
					copyPairToCompareBuffer();
				} else {
					selectQuestionZmb();
				}
			}
		}
	}
	
	// [3] Celebration scheduling (hoorah fidget)
	if (_celebrationActive && _celebrationsPlayed < _celebrationTarget) {
		if (getCurrentFrameCounter() > _nextCelebrationFrame) {
			_nextCelebrationFrame = getCurrentFrameCounter() + 30;
			
			if (_loadedOnCliffCount > 0) {
				int16 pickIdx = _vm->_rnd->getRandomNumber(0, _loadedOnCliffCount - 1);
				ZmbSnoid *idleSnoid = _zmbOnCliff[pickIdx];
				
				if (idleSnoid && idleSnoid->getAnimState() == kSnoidAnimIdle) {
					uint16 scrsId = 12044 + (pickIdx % 6);
					Common::SeekableReadStream *scrsStream =
						_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, scrsId));
					if (scrsStream) {
						idleSnoid->startScrsPlayback(scrsStream, false, true);
						_celebrationsPlayed++;
					}
				}
			}
		}
	}
	
	// [4] Level 4 transition phase handling
	if (_difficultyLevel == 4 && _transitionPhase > 0) {
		// Phase transitions are triggered by animation events
	}
	
	_processingFrame = false;
}

void ZoombiniInteractiveSmoke::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
		// Snoid animation event
		ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);
		SnoidAnimState state = snoid->getAnimState();
		
		if (state == kSnoidAnimScriptReject) {
			// Reject script finished — return to idle
			snoid->setAnimState(kSnoidAnimIdle);
			snoid->setupIdleHotspots();
		} else if (state == kSnoidAnimScriptNormal) {
			// Normal script (entering smoke) finished — hide snoid
			snoid->deactivateRender();
			snoid->deactivateAnimate();
		}
	} else {
		// SCRB feature animation event
		processAnimDispatchEvent(feature, eventCode);
	}
}

void ZoombiniInteractiveSmoke::processAnimDispatchEvent(ZmbFeature *feature, int16 eventCode) {
	// IDA: smoke_scrbAnimDispatch (0x44CB72)
	// Central animation callback for smoke features.
	
	switch (eventCode) {
	case 0:
		// Toggle render flag on runner
		if (feature->hasFlag(ZmbFeature::FLAG_01000000_DEFER_RENDER)) {
			feature->removeFlag(ZmbFeature::FLAG_01000000_DEFER_RENDER);
		} else {
			feature->addFlag(ZmbFeature::FLAG_01000000_DEFER_RENDER);
		}
		break;
		
	case 1:
		// Set reload main runner flag
		_bReloadMainRunner = true;
		break;
		
	case 2:
		// Initialize match comparison runners
		reloadScrbAnimation(_compareAFeature, 11018);
		reloadScrbAnimation(_compareBFeature, 11019);
		break;
		
	case 3:
		// Set reset level flag (levels 1-3)
		if (_difficultyLevel <= 3) {
			_bResetLevel = true;
		}
		break;
		
	case 4:
		// Start next compare sequence
		startNextCompareSequence();
		break;
		
	case 10:
	case 11:
	case 13:
	case 14:
		// Play Zoombini travel script on current drag zmb
		if (_currentDragZmb) {
			uint16 scrsId = 12000 + (eventCode - 10);
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, scrsId));
			if (scrsStream) {
				_currentDragZmb->startScrsPlayback(scrsStream, false, false);
			}
		}
		break;
		
	case 16:
		// Position two bottom display runners
		// (positions from coordinate tables in original)
		break;
		
	case 17:
		// Remove Zoombini from queue, play rejected animation
		if (_difficultyLevel == 4) {
			handleFrameTransition();
		}
		break;
		
	case 30:
		// Pick up Zoombini: set position, play pickup script
		break;
		
	case 31:
		// Link and animate smoke stack B
		if (_smokeStackBFeature) {
			_smokeStackBFeature->activateRender();
			_smokeStackBFeature->activateAnimate();
		}
		break;
		
	case 35:
		// Advance next Zoombini in queue through smoke
		_currentZmbIdx++;
		break;
		
	case 36:
		// First-time attribute assignment from current drag Zoombini
		if (_currentDragZmb) {
			_questionAttrs[0] = _currentDragZmb->_trait._head;
			_questionAttrs[1] = _currentDragZmb->_trait._eye;
			_questionAttrs[2] = _currentDragZmb->_trait._nose;
			_questionAttrs[3] = _currentDragZmb->_trait._foot;
		}
		break;
		
	case 37:
		// Walk current Zoombini, load on stack runner, toggle stacks A/B
		_bRunnerToggle = !_bRunnerToggle;
		break;
		
	case 38:
		// Level 4 specific: attribute refresh cycle, or trigger question display
		if (_difficultyLevel == 4) {
			handleFrameTransition();
		}
		break;
		
	case 50:  // '2'
		// Set place Zoombini flag
		_bPlaceZmb = true;
		break;
		
	case 51:  // '3'
		// Set link runners flag
		_bLinkRunners = true;
		break;
		
	case 60:  // '<'
		// Reset navigation state
		break;
		
	case 251:
		// Set animation shape on Zoombini runner
		break;
		
	default:
		break;
	}
}

} // End of namespace Mohawk
