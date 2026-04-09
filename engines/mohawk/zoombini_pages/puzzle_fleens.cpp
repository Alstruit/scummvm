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
#include "mohawk/zoombini_pages/puzzle_fleens.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions from 0x4A0FE8 (16 POINTS)
const Common::Point ZoombiniPuzzleFleens::kSnoidPositions[16] = {
	Common::Point(238, 368), Common::Point(185, 417), Common::Point(155, 448), Common::Point(197, 396),
	Common::Point(160, 357), Common::Point(164, 384), Common::Point(150, 416), Common::Point(116, 357),
	Common::Point(130, 386), Common::Point(109, 418), Common::Point(117, 448), Common::Point( 74, 348),
	Common::Point( 89, 384), Common::Point( 67, 418), Common::Point( 76, 450), Common::Point( 56, 379),
};

// IDA: raft DRAW_ON_REG position at 0x4A1028
const Common::Point ZoombiniPuzzleFleens::kRaftPosition(438, 357);

ZoombiniPuzzleFleens::ZoombiniPuzzleFleens(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kFleens) {
}

ZoombiniPuzzleFleens::~ZoombiniPuzzleFleens() {
}

void ZoombiniPuzzleFleens::open() {
	openArchive(ZMB_MHK_FLEENS);
}

void ZoombiniPuzzleFleens::setBackgroundMusic() {
	// IDA: fleens_initAndSetupPuzzle (0x41c1e0) has no music playback call on page load.
	// sound_activeHandle is stored at end of funcInit for F1 replay only.
}

void ZoombiniPuzzleFleens::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(300)
	_vm->_gfx->setPalette(300);
	_vm->_gfx->drawBackground(300);
}

void ZoombiniPuzzleFleens::loadFeatures() {
	// IDA: fleens_initAndSetupPuzzle (0x41C3DC)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// Initialize puzzle state
	_bRaftReady = false;
	_bInteractionAllowed = false;
	_mismatchCount = 0;
	_raftButtonDirty = false;
	_attrSlot1Dirty = false;
	_attrSlot2Dirty = false;

	// Load terrain barrier bitmap (tBMP 500)
	// IDA: rmap_loadTerrainArchive(0x1F4u)
	loadTerrainBitmap(500);

	// Preload shape images
	// IDA: shape_loadSubShapesFromArchive(stru_4AB20C, 0xFA0u) — shapes at tBMP 4000
	_vm->_gfx->preloadImage(4000);

	// IDA: shape_loadSubShapesFromArchive(&stru_4AB20C, 0x190u) — shapes at tBMP 400
	_vm->_gfx->preloadImage(400);
	_vm->_gfx->preloadImage(1000);
	_vm->_gfx->preloadImage(1100);
	_vm->_gfx->preloadImage(1200);

	// Load feature groups
	// IDA: scrb_useFeatureGroup(0, 0, 1000)
	// IDA: scrb_useFeatureGroup(0, 1, 1100)
	// IDA: scrb_useFeatureGroup(0, 2, 1200)

	// Load REGS resources
	// IDA: regs_loadAndByteSwap(0xFA0u) — REGS 4000
	// IDA: regs_loadAndByteSwap(0xFA1u) — REGS 4001

	// Load main features: 7 SCRBs at 1000
	// IDA: scrb_loadMainFeatureSet(7, 1000)
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 1, 0x44C) — 1 sub at 1100
	{
		ZmbFeature *parent = mainFeature;
		parent = loadSubFeature(parent,
			ZmbResource(ZmbArchiveKind::kPage, 1100), 1100);
	}

	// IDA: scrb_loadSubFeatureSet(0, 7, 0x4B0) — 7 subs at 1200
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 7; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 1200), 1200 + i);
		}
	}

	// Load reject pool: 5 reject scripts at SCRS 6000
	// IDA: scrs_loadRejectPool(0, 5, 6000)
	for (uint16 i = 0; i < 5; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 4000),
				  6000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 46 normal scripts at SCRS 7000
	// IDA: scrs_loadNormalPool(26, 46, 7000)
	for (uint16 i = 0; i < 46; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 4000),
				  7000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// --- Puzzle-specific feature runners ---

	// IDA: word_4AB1A4 = runner_registerAndAllocate(..., 6, 0x3E8, standard, standard, 0x108000)
	// Animation runner (SCRB 1000), LOOP_ANIM | PLAY_ONCE
	_animFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 1000), 1000, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);

	// IDA: scrb_drawOnRegRunnerIdxArr[0] = runner_registerAndAllocate(..., &raftPos, 7, 0x44C, standard, standard, 0x108A000)
	// Raft DRAW_ON_REG runner (SCRB 1100) at raft position
	_raftFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 1100), 1100, 7,
		kRaftPosition,
		ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER);

	// IDA: runner_registerAndAllocate(0, 0, 0, 0, 0, caves_invalidateEntranceRectsC, caves_renderAllAttrSlots, 0x1000)
	// Virtual feature for attribute slot rendering (TOPMOST)
	{
		ZmbFeature::EventHooks attrSlotHooks;
		attrSlotHooks.setPreRenderFunc(reinterpret_cast<ZmbFeature::OnPreRenderFunc>(&ZoombiniPuzzleFleens::attrSlots_preRender));
		attrSlotHooks.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniPuzzleFleens::attrSlots_render));
		loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, 0), 0, 0, ZmbFeature::FLAG_00001000_TOPMOST, attrSlotHooks);
	}

	// Load Zoombinis from active pack at 16 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, pPosArr, 16)
	loadZoombinisFromPack();

	// IDA: ferry_buildZmbRunners_41D9F4 — builds zoombini trait runners
	// Sets up trait transformation data for puzzle matching logic.
	// The visual trait runners (fleens_spawnRunner) are not fully implemented yet.
	buildZmbTraitSetup();

	// IDA: 7× word_4AA848[scrbId] = runner_registerAndAllocate(..., 6, scrbId, standard, standard, flags)
	// Overlay runners (SCRB 1200-1206)
	for (int16 i = 0; i < 7; i++) {
		uint32 flags = ZmbFeature::FLAG_04000000_OVERLAY;
		if (i == 0) {
			// SCRB 1200 gets additional DEFER_ANIM | PLAY_ONCE
			flags |= ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE;
		}
		_overlayFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 1200), 1200 + i, 6, flags);
	}

	// Layout and stagger walk-in (200ms walk delay)
	// IDA: zmb_layoutStaticAndWalkInGroups(0)
	// IDA: zmb_assignStaggeredWalkDelays(200, 45)
	layoutStaticAndWalkIn();
	assignStaggeredWalkDelays();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(4000);
	loadHelpButtonFeature();

	// IDA: v2 = getDifficultyIdFromPuzzleFlag(FLEENS_FLAG)
	//   v2==2 (LEVEL2)         → 20080 (hard voice)
	//   routeLevel==1 || ==3   → random(20079, 20080)
	//   else                   → 20079
	{
		ZMB_DIFFICULTY_ID diffId = _vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagFleens);
		uint16 helpSoundId;
		if (diffId == ZMB_DIFFICULTY_LEVEL2_02) {
			helpSoundId = 20080;
		} else if (_difficultyLevel == 1 || _difficultyLevel == 3) {
			helpSoundId = _vm->_rnd->getRandomNumber(20079, 20080);
		} else {
			helpSoundId = 20079;
		}
		_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, helpSoundId);
	}

	// Idle animation state init (IDA: fleens_clearAllPuzzleState @ 0x41C0B4)
	_idleAnimCount = 0;
	_idleAnimTarget = 0;
	_idleAnimLastFrame = 0;
	_idleAnimInterval = 60;
	_idleAnimPoolState = 0;
	_idleAnimDelayCounter = 64;

	// Additional state init (IDA: fleens_clearAllPuzzleState)
	_bPuzzleActive = false;
	_bRaftAnimPlaying = false;
	_bBoardingInProgress = false;
	_bAuxLinked = false;
	_bOverlayLinkPending = false;
	_bRaftDepartPending = false;
	_bScriptDComplete = false;
	_bScriptEComplete = false;
	_bScriptAComplete = false;
	_pendingTransitionTarget = 0;
	_pendingBoardSnoidId = 0;
	_boardingSnoidFoot = 0;
	_activeRaftAnimSnoidId = 0;
	_activeRaftSnoidRunner = 0;
	_capturePhaseRunner = 0;
	_departQueueCount = 0;
	_deferredScrsCountdown = 8;

	for (int i = 0; i < 16; i++) {
		_seatOccupied[i] = false;
		_seatSnoidId[i] = 0;
	}
	for (int i = 0; i < 7; i++) {
		_departQueue[i] = 0;
		_departFleenQueue[i] = 0;
	}

	// Start initial raft arrival animation
	// IDA: fleens_initAndSetupPuzzle tail (0x41C4CA-0x41C52C)
	startInitialRaftAnim();

	// IDA: fleens_bPuzzleActive = 1
	_bPuzzleActive = true;

	// IDA: fleens_totalZmbCount = zmb_countFeatureRunners()
	_totalZmbCount = _loadedZmbCount;
}

void ZoombiniPuzzleFleens::onGoButtonActivated() {
	// IDA: fleens_onClickHandler case 2
	// Guard: must be ready and interaction allowed
	if (!_bRaftReady || !_bInteractionAllowed)
		return;

	// IDA: play move SFX, set word_4AB1C8=1, puzzle_pendingTransitionTarget=14
	_bRaftDepartPending = true;
	_pendingTransitionTarget = 14;
	_departXferSrcSiPage = ZMB_SI_FLEENS_10;
	ZoombiniInteractive::onGoButtonActivated();
}

// ---------------------------------------------------------------------------
// onEveryFrame: Complete per-frame state machine.
// IDA: fleens_onHoverPerFrame @ 0x41C81B
// ---------------------------------------------------------------------------
void ZoombiniPuzzleFleens::onEveryFrame() {
	if (!_bPuzzleActive)
		return;

	// --- Phase 1: Pending transition departure ---
	// IDA @ 0x41C86B: if puzzle_pendingTransitionTarget set
	if (_pendingTransitionTarget != 0) {
		debugC(1, kZmbDebugAnimation, "Fleens: pending transition target=%d raftAnim=%d boarding=%d",
			_pendingTransitionTarget, _bRaftAnimPlaying ? 1 : 0, _bBoardingInProgress ? 1 : 0);
		// Departure has been initiated — wait for raft animation
		if (!_bRaftAnimPlaying && !_bBoardingInProgress) {
			// All animations finished — execute departure
			_bInteractionAllowed = false;
			executeDeparture();
			return;
		}
	}

	// --- Phase 2: Raft boarding sequence ---
	// IDA @ 0x41C9xx: if word_4AB1F8 (pending board snoid) set and no active raft anim
	if (_pendingBoardSnoidId != 0 && !_bBoardingInProgress) {
		debugC(1, kZmbDebugAnimation, "Fleens: starting boarding for snoid %d", _pendingBoardSnoidId);
		startBoardingAnimation();
	}

	// --- Phase 3: Script completion flags processing ---
	// IDA @ 0x41CA27: departure script processing
	// These flags are set by the various script event handlers (A/D/E) when
	// their SCRS animations complete (event -1).

	if (_bScriptDComplete) {
		debugC(1, kZmbDebugAnimation, "Fleens: script D complete, departQueue=%d", _departQueueCount);
		_bScriptDComplete = false;
		// Script D complete — signal raft departure if queue has items
		if (_departQueueCount > 0)
			_bRaftDepartPending = true;
	}

	if (_bScriptEComplete) {
		debugC(1, kZmbDebugAnimation, "Fleens: script E complete, departQueue=%d", _departQueueCount);
		_bScriptEComplete = false;
		// Script E complete — signal raft departure if queue has items
		if (_departQueueCount > 0)
			_bRaftDepartPending = true;
	}

	if (_bScriptAComplete) {
		debugC(1, kZmbDebugAnimation, "Fleens: script A complete, departQueue=%d", _departQueueCount);
		_bScriptAComplete = false;
		// Script A complete — signal raft departure if queue has items
		if (_departQueueCount > 0)
			_bRaftDepartPending = true;
	}

	// --- Phase 4: Process raft departure queue ---
	// IDA @ 0x41CA6F: if word_4AB1C8 set, call processRaftDeparture
	if (_bRaftDepartPending) {
		debugC(1, kZmbDebugAnimation, "Fleens: raft departure pending");
		_bRaftDepartPending = false;
		processRaftDeparture();
	}

	// --- Phase 5: Idle celebration animations ---
	// IDA @ 0x41CA77: when interaction allowed and below target count
	if (_bInteractionAllowed && _idleAnimCount < _idleAnimTarget &&
	    _loadedZmbCount > 0) {

		if (getCurrentFrameCounter() - _idleAnimLastFrame > _idleAnimInterval) {
			_idleAnimLastFrame = getCurrentFrameCounter();

			bool triggered = false;
			int16 attempts = 0;

			do {
				uint16 poolIdx = _vm->_rnd->getNonRepeatRandom(_loadedZmbCount, _idleAnimPoolState);
				uint16 snoidId = 10000 + poolIdx;
				ZmbSnoid *snoid = getSnoid(snoidId);

				if (snoid && snoid->isRenderActivated() &&
				    snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID) &&
				    snoid->getPointLoc().x <= 270) {
					// IDA: fleens_mapEventToScrsId type 5 → foot + 7030
					uint16 scrsId = mapEventToScrsId(5, snoid);
					if (scrsId != 0) {
						Common::SeekableReadStream *scrsStream =
							_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
								ZmbResource(ZmbArchiveKind::kPage, scrsId));
						if (scrsStream) {
							snoid->startScrsPlayback(scrsStream, false, true);
							_idleAnimCount++;
							triggered = true;
						}
					}
				} else if (++attempts > 20) {
					triggered = true;
				}
			} while (!triggered);
		}
	}

	// --- Phase 6: Deferred SCRS loading ---
	// IDA: fleens_deferredScrsCountdown decrements each frame, loads SCRS when 0
	if (_deferredScrsCountdown > 0) {
		_deferredScrsCountdown--;
	}

	// --- Phase 7: Go button state ---
	// IDA: fleens_bRaftReady enables/disables go button
	setGoButtonsEnabled(_bRaftReady && _bInteractionAllowed && _departQueueCount > 0);
}

// ---------------------------------------------------------------------------
// Mouse handlers
// IDA: fleens_onClickHandler @ 0x41CC8F
// ---------------------------------------------------------------------------
ZmbEventHandleResult ZoombiniPuzzleFleens::onLButtonDown(
		const Common::Point &absPos, const Common::Point &relPos) {
	// Let base class handle Go/Map/Help buttons first
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return ZmbEventHandleResult::kConsumed;

	// Guard: must be active and interaction allowed
	if (!_bPuzzleActive || !_bInteractionAllowed)
		return ZmbEventHandleResult::kPassthrough;

	// Cannot drag during boarding or if already dragging
	if (_bBoardingInProgress || isDragging())
		return ZmbEventHandleResult::kPassthrough;

	// IDA: fleens_onClickHandler case 4 — Zoombini drag
	// Find snoid under cursor (must be on left shore, x<=270)
	ZmbSnoid *snoid = findSnoidAtPoint(absPos);
	if (!snoid)
		return ZmbEventHandleResult::kPassthrough;

	// Only drag idle snoids on the shore
	if (snoid->getPointLoc().x > 270)
		return ZmbEventHandleResult::kPassthrough;

	// Save origin and start drag
	_savedDragOrigin = snoid->getPointLoc();
	startSnoidDrag(snoid, absPos);
	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniPuzzleFleens::onLButtonUp(
		const Common::Point &absPos, const Common::Point &relPos) {
	if (!isDragging())
		return ZoombiniInteractive::onLButtonUp(absPos, relPos);

	endDrag(absPos);
	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniPuzzleFleens::onMouseMove(
		const Common::Point &absPos, const Common::Point &relPos) {
	return ZoombiniInteractive::onMouseMove(absPos, relPos);
}

// ---------------------------------------------------------------------------
// endDrag: Process drop target after releasing a dragged Zoombini.
// IDA: fleens_onClickHandler case 4 drop logic
// ---------------------------------------------------------------------------
void ZoombiniPuzzleFleens::endDrag(const Common::Point &mousePos) {
	ZmbSnoid *snoid = finishSnoidDrag();
	if (!snoid)
		return;

	Common::Point dropPos = snoid->getPointLoc();

	// IDA: check if dropped in raft area (right side of screen, x > 270)
	if (dropPos.x > 270) {
		// Find available seat on the raft
		int16 seatIdx = findAvailableRaftSeat();
		if (seatIdx >= 0) {
			// Mark seat as occupied
			_seatOccupied[seatIdx] = true;
			_seatSnoidId[seatIdx] = snoid->getId();

			// Add to departure queue
			if (_departQueueCount < 7) {
				_departQueue[_departQueueCount] = snoid->getId();
				_departQueueCount++;
			}

			// Set pending boarding
			// IDA: word_4AB1F6 (foot trait), word_4AB1F8 (runner to board)
			_boardingSnoidFoot = snoid->_trait._foot;
			_pendingBoardSnoidId = snoid->getId();

			// Mark snoid as occupied (passed this puzzle)
			snoid->_packIsOccupied = true;

			// Set raft ready if we have boarded snoids
			if (_departQueueCount > 0)
				_bRaftReady = true;
		} else {
			// No seats available — return to origin
			snoid->setPointLoc(_savedDragOrigin);
			snoid->setAnimState(kSnoidAnimIdle);
			snoid->setupIdleHotspots();
		}
	} else {
		// Dropped back on shore — validate terrain and keep position
		if (!validateTerrainDrop(snoid)) {
			snoid->setPointLoc(_savedDragOrigin);
		}
		snoid->setAnimState(kSnoidAnimIdle);
		snoid->setupIdleHotspots();
	}
}

// ---------------------------------------------------------------------------
// mapEventToScrsId: Map an event type to SCRS resource ID.
// IDA: fleens_mapEventToScrsId @ 0x41E860
// ---------------------------------------------------------------------------
uint16 ZoombiniPuzzleFleens::mapEventToScrsId(int16 eventType, const ZmbSnoid *snoid) const {
	uint8 foot = snoid->_trait._foot;

	switch (eventType) {
	case 1:
		return foot + 7035;
	case 2:
		if (!_bAuxLinked) {
			return foot + 7041 - 1;
		} else if (_mismatchCount == 3) {
			return foot + 7005 - 1;
		} else {
			return foot + 7000 - 1;
		}
	case 3:
		return 7010;
	case 4:
		return foot + 7010;
	case 5:
		return foot + 7030;
	case 8:
		return foot + 7025;
	case 9:
		return foot + 5999;
	case 7016:
		return foot + 7015;
	case 7021:
		return foot + 7020;
	default:
		return 0;
	}
}

// ---------------------------------------------------------------------------
// processRaftDeparture: Process the departure queue.
// IDA: fleens_processRaftDeparture @ 0x41EAF3
// ---------------------------------------------------------------------------
void ZoombiniPuzzleFleens::processRaftDeparture() {
	for (int16 i = 0; i < _departQueueCount; i++) {
		ZmbSnoid *snoid = getSnoid(_departQueue[i]);
		if (!snoid)
			continue;

		if (i == _departQueueCount - 1) {
			// Last runner in queue: play exit SCRS type 8 (foot+7025)
			uint16 scrsId = mapEventToScrsId(8, snoid);
			if (scrsId != 0) {
				Common::SeekableReadStream *scrsStream =
					_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
						ZmbResource(ZmbArchiveKind::kPage, scrsId));
				if (scrsStream) {
					snoid->startScrsPlayback(scrsStream, false, true);
				}
			}
		} else {
			// Other runners: play SCRS type 7021 (foot+7020)
			uint16 scrsId = mapEventToScrsId(7021, snoid);
			if (scrsId != 0) {
				Common::SeekableReadStream *scrsStream =
					_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
						ZmbResource(ZmbArchiveKind::kPage, scrsId));
				if (scrsStream) {
					snoid->startScrsPlayback(scrsStream, false, true);
				}
			}
		}
	}

	// Decrement queue count after processing
	if (_departQueueCount > 0)
		_departQueueCount--;
}

// ---------------------------------------------------------------------------
// startInitialRaftAnim: Play the initial raft arrival animation.
// IDA: fleens_initAndSetupPuzzle tail (0x41C4CA-0x41C52C)
// ---------------------------------------------------------------------------
void ZoombiniPuzzleFleens::startInitialRaftAnim() {
	if (_loadedZmbCount <= 0)
		return;

	// Find the first pack snoid (the raft leader)
	ZmbSnoid *firstSnoid = getSnoid(10000);
	if (!firstSnoid)
		return;

	// IDA: play SCRS type 1 (foot + 7035) — initial boarding animation
	uint16 scrsId = mapEventToScrsId(1, firstSnoid);
	if (scrsId == 0)
		return;

	Common::SeekableReadStream *scrsStream =
		_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
			ZmbResource(ZmbArchiveKind::kPage, scrsId));
	if (scrsStream) {
		_bRaftAnimPlaying = true;
		firstSnoid->startScrsPlayback(scrsStream, false, true);
	}
}

// ---------------------------------------------------------------------------
// startBoardingAnimation: Start a boarding animation for the pending snoid.
// IDA: fleens_onHoverPerFrame boarding block
// ---------------------------------------------------------------------------
void ZoombiniPuzzleFleens::startBoardingAnimation() {
	if (_pendingBoardSnoidId == 0)
		return;

	ZmbSnoid *snoid = getSnoid(_pendingBoardSnoidId);
	if (!snoid) {
		_pendingBoardSnoidId = 0;
		return;
	}

	// IDA: play SCRS type 2 — boarding animation
	// Type 2: if !_bAuxLinked → foot+7041-1, else if mismatch==3 → foot+7005-1, else foot+7000-1
	uint16 scrsId = mapEventToScrsId(2, snoid);
	if (scrsId == 0) {
		_pendingBoardSnoidId = 0;
		return;
	}

	Common::SeekableReadStream *scrsStream =
		_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
			ZmbResource(ZmbArchiveKind::kPage, scrsId));
	if (scrsStream) {
		_bBoardingInProgress = true;
		_activeRaftAnimSnoidId = _pendingBoardSnoidId;
		snoid->startScrsPlayback(scrsStream, false, true);
	}
	_pendingBoardSnoidId = 0;
}

// ---------------------------------------------------------------------------
// onRaftExitComplete: Handle raft exit completion.
// IDA: fleens_onRaftExitComplete @ 0x41ED04
// ---------------------------------------------------------------------------
void ZoombiniPuzzleFleens::onRaftExitComplete() {
	_bInteractionAllowed = true;
	_bRaftAnimPlaying = false;
	_bBoardingInProgress = false;
	_activeRaftAnimSnoidId = 0;

	// IDA: if loadedCount == totalCount → celebration sound 20055-20063
	if (_loadedZmbCount == _totalZmbCount && _loadedZmbCount > 0) {
		uint16 sndId = _vm->_rnd->getRandomNumber(20055, 20063);
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, sndId),
		                          Audio::Mixer::kSFXSoundType);
	} else if (_loadedZmbCount > 0) {
		// IDA: with probability or first few attempts, play guidance 20045-20048
		ZmbStateFile &f = _vm->_state->_f;
		int16 randCheck = _vm->_rnd->getRandomNumber(0, 4);
		if (randCheck > _difficultyLevel || (f._pageFlagFleens & 0xFFF) <= 3) {
			uint16 sndId = _vm->_rnd->getRandomNumber(20045, 20048);
			_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, sndId),
			                          Audio::Mixer::kSFXSoundType, 1);
		}
	}

	// IDA: set idle target based on loaded count
	if (_loadedZmbCount == 16) {
		_idleAnimTarget = 13;
	} else if (_loadedZmbCount > 8) {
		_idleAnimTarget = _loadedZmbCount - 8;
	}
}

// ---------------------------------------------------------------------------
// isMismatchSnoid: Check if a snoid index is one of the mismatched ones.
// ---------------------------------------------------------------------------
bool ZoombiniPuzzleFleens::isMismatchSnoid(uint16 snoidIdx) const {
	for (int i = 0; i < 3; i++) {
		if (_mismatchIdx[i] != 0 && _mismatchIdx[i] == static_cast<int16>(snoidIdx))
			return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
// findAvailableRaftSeat: Find the next empty seat on the raft.
// IDA: byte_4AB24A[] scan
// ---------------------------------------------------------------------------
int16 ZoombiniPuzzleFleens::findAvailableRaftSeat() const {
	for (int16 i = 0; i < 16; i++) {
		if (!_seatOccupied[i])
			return i;
	}
	return -1;
}

void ZoombiniPuzzleFleens::loadZoombinisFromPack() {
	ZmbStateFile &f = _vm->_state->_f;
	uint16 posIdx = 0;

	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount && posIdx < 16; i++) {
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
		}
		posIdx++;
	}

	_loadedZmbCount = posIdx;
}

void ZoombiniPuzzleFleens::buildZmbTraitSetup() {
	// IDA: ferry_buildZmbRunners_41D9F4
	// Selects "mismatch" zoombinis and generates mod-5 trait transformation offsets.
	// The transformed traits determine which Zoombinis will be captured by Fleens.

	ZmbStateFile &f = _vm->_state->_f;
	
	// Count occupied zoombinis (IDA: countOccupiedInActivePack_452875)
	int16 zmbCount = 0;
	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount; i++) {
		if (f._zmbPackActive._entries[i]._bIsOccupied)
			zmbCount++;
	}
	
	if (zmbCount == 0)
		return;
	
	// Reset mismatch indices
	_mismatchIdx[0] = 0;
	_mismatchIdx[1] = 0;
	_mismatchIdx[2] = 0;
	
	// Pick first mismatch zoombini randomly (1-based index)
	_mismatchIdx[0] = _vm->_rnd->getRandomNumber(1, zmbCount);
	
	// Set mismatch count based on zoombini count
	if (zmbCount == 1) {
		_mismatchCount = 2;
	} else if (zmbCount == 2) {
		_mismatchCount = 1;
	}
	
	// Pick second mismatch zoombini (different from first)
	if (zmbCount >= 2) {
		do {
			_mismatchIdx[1] = _vm->_rnd->getRandomNumber(1, zmbCount);
		} while (_mismatchIdx[1] == _mismatchIdx[0]);
	}
	
	// Pick third mismatch zoombini (different from first two)
	if (zmbCount >= 3) {
		do {
			_mismatchIdx[2] = _vm->_rnd->getRandomNumber(1, zmbCount);
		} while (_mismatchIdx[2] == _mismatchIdx[0] || _mismatchIdx[2] == _mismatchIdx[1]);
	}
	
	// Generate trait transformation offsets (1-5) for first 4 slots
	// These determine how traits are transformed for puzzle matching
	// IDA: if (!wTransitionsDisable[1] || fleens_routeLevel == 1 || fleens_routeLevel == 3)
	if (_traitOffsets[0] == 0 || _difficultyLevel == 1 || _difficultyLevel == 3) {
		for (int i = 0; i < 4; i++) {
			_traitOffsets[i] = static_cast<uint8>(_vm->_rnd->getRandomNumber(1, 5));
		}
	}
	
	// For difficulty <= 1, clear the slot order array
	if (_difficultyLevel <= 1) {
		for (int i = 0; i < 4; i++) {
			_traitSlotOrder[i] = 0;
		}
	} else if (_traitSlotOrder[0] == 0 || _difficultyLevel == 3) {
		// For higher difficulty, generate slot order using non-repeat random
		// IDA: e2GetPoolValue_nonRepeatRandom with 4 positions
		_traitSlotOrder[0] = static_cast<uint8>(_vm->_rnd->getRandomNumber(2, 4));
		// Simplified: just assign 1-4 for slot ordering
		uint32 usedMask = 1 << (_traitSlotOrder[0] - 1);
		for (int i = 1; i < 4; i++) {
			uint8 slot;
			do {
				slot = static_cast<uint8>(_vm->_rnd->getRandomNumber(1, 4));
			} while (usedMask & (1 << (slot - 1)));
			_traitSlotOrder[i] = slot;
			usedMask |= (1 << (slot - 1));
		}
	}
}

bool ZoombiniPuzzleFleens::attrSlots_preRender(ZmbFeature *feature) {
	// IDA: fleens_renderAttrSlotSCRB_4366CB
	// Toggle dirty flags when raft state changes
	if (_bRaftReady && _bInteractionAllowed) {
		if (!_raftButtonDirty) {
			_raftButtonDirty = true;
			_attrSlot1Dirty = true;
			_attrSlot2Dirty = true;
		}
	} else {
		if (_raftButtonDirty) {
			_raftButtonDirty = false;
			_attrSlot1Dirty = false;
			_attrSlot2Dirty = false;
		}
	}
	return true; // Continue to render
}

ZmbRenderResult ZoombiniPuzzleFleens::attrSlots_render(ZmbFeature *feature) {
	// IDA: fleens_renderAttrSlotSCRB_4366CB
	// Just clear dirty flags for now - actual sprite rendering handled by SCRB features
	_raftButtonDirty = false;
	_attrSlot1Dirty = false;
	_attrSlot2Dirty = false;
	return ZmbRenderResult::kRendered;
}

// ---------------------------------------------------------------------------
// Animation event dispatch
// IDA: fleens_raftAnimStateMachine (0x41E1BD) — main raft state machine
//      fleens_raftMovementCallback (0x41E075) — sub-feature movement
//      fleens_scriptEventHandler{A,B,C,D,E} — script completion handlers
// ---------------------------------------------------------------------------
void ZoombiniPuzzleFleens::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	switch (eventCode) {
	case kZmbAnimEventM1_End:
		// End-of-animation. IDA: fleens_raftAnimStateMachine event -1
		// Reset raft animation state
		_idleAnimDelayCounter = 64;
		_bRaftAnimPlaying = false;
		_bBoardingInProgress = false;
		_activeRaftAnimSnoidId = 0;
		_activeRaftSnoidRunner = 0;

		// If this was a SCRS-playing snoid, return to idle
		if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);
			snoid->setAnimState(kSnoidAnimIdle);
			snoid->setupIdleHotspots();

			// Check if this snoid was the active raft animation snoid
			if (snoid->getId() == _activeRaftAnimSnoidId) {
				// Boarding animation complete
				onRaftExitComplete();
			}
		}
		break;

	case 0:
		// Toggle render visibility.
		// IDA: *(a4+290) = *(a4+290)==0
		if (feature->isRenderActivated())
			feature->deactivateRender();
		else
			feature->activateRender();

		// Apply pending body arrangement
		// IDA: if (word_4AB1A0) { zmb_setBodyLayerShapes(word_4AB1A0-1, a4+48); word_4AB1A0=0; }
		if (_pendingBodyArrangement != 0 && feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			static_cast<ZmbSnoid *>(feature)->setBodyArrangement(_pendingBodyArrangement - 1);
			_pendingBodyArrangement = 0;
		}
		break;

	case 1:
		// IDA: fleens_raftMovementCallback event 1
		// Link auxiliary runner, init SCRB, set _bAuxLinked
		if (!_bAuxLinked) {
			_bAuxLinked = true;
			// NOTE: Original engine runner_linkRelativeToParent for Z-order.
			// ScummVM uses dirty-rect rendering which handles Z naturally.
		}
		break;

	case 2:
		// IDA: fleens_raftMovementCallback event 2
		// Link movement runner after raft runner.
		// NOTE: Original engine runner_linkRelativeToParent for Z-order.
		break;

	case 4:
		// IDA: raftAnimStateMachine event 4
		// First boarding phase: resolve SCRB for boarding position
		if (_raftFeature) {
			loadScrbOntoFeature(_raftFeature, 1100);
		}
		break;

	case 5:
		// IDA: raftAnimStateMachine event 5
		// Set boarding position from REGS data
		// Marks transition to next boarding phase
		break;

	case 6:
		// IDA: raftAnimStateMachine event 6
		// Mark boarding in progress
		_bBoardingInProgress = true;
		break;

	case 7:
		// IDA: raftAnimStateMachine event 7
		// Link snoid runner after raft for visual layering
		// NOTE: Original engine runner_linkRelativeToParent for Z-order.
		break;

	case 8:
		// IDA: raftAnimStateMachine event 8
		// Process next departure queue item / chain linking
		if (_departQueueCount > 0) {
			// Process the departure queue
			processRaftDeparture();
		}
		break;

	case 9:
		// IDA: raftAnimStateMachine event 9
		// Initialize departure queue processing
		break;

	case 28:
		// IDA: raftAnimStateMachine event 28 → forward to event 8
		onFeatureAnimEvent(feature, 8);
		break;

	case 30:
		// IDA: raftAnimStateMachine event 30
		// Register new overlay SCRB runner (random 1004-1006) with exit callback
		{
			uint16 overlayScrbId = 1004 + _vm->_rnd->getRandomNumber(0, 2);
			// Use overlay feature slot 4, 5, or 6 (indices after the main 4: 1200-1203)
			for (int i = 4; i < 7; i++) {
				if (_overlayFeatures[i] && !_overlayFeatures[i]->isAnimateActivated()) {
					loadScrbOntoFeature(_overlayFeatures[i], overlayScrbId);
					break;
				}
			}
		}
		break;

	case 60:
		// IDA: fleens_scriptEventHandlerA event 60
		// Find runner at _pendingExitRunner, run SCRB 13, init runner state
		break;

	case 131:
		// IDA: fleens_scriptEventHandlerB event 131
		// If departure queue active, set raft depart pending
		if (_departQueueCount > 0)
			_bRaftDepartPending = true;
		break;

	case 132:
		// IDA: raftAnimStateMachine event 132
		// Departure queue processing: play SCRS 7016+offset on each queue runner
		for (int16 i = 0; i < _departQueueCount; i++) {
			ZmbSnoid *snoid = getSnoid(_departQueue[i]);
			if (!snoid)
				continue;
			uint16 scrsId = mapEventToScrsId(7016, snoid);
			if (scrsId != 0) {
				Common::SeekableReadStream *scrsStream =
					_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
						ZmbResource(ZmbArchiveKind::kPage, scrsId));
				if (scrsStream) {
					snoid->startScrsPlayback(scrsStream, false, true);
				}
			}
		}
		break;

	case 133: case 134: case 135:
		// IDA: raftAnimStateMachine events 133-135
		// Capture animation: check each snoid for mismatch, play capture SCRB 14
		{
			int16 captureRange = eventCode - 132; // 1, 2, or 3
			for (int16 i = 0; i < _loadedZmbCount; i++) {
				if (!_seatOccupied[i])
					continue;
				ZmbSnoid *snoid = getSnoid(_seatSnoidId[i]);
				if (!snoid)
					continue;

				// Check if this snoid is a mismatch within the capture range
				uint16 snoidPosIdx = snoid->getId() - 10000;
				if (isMismatchSnoid(snoidPosIdx + 1) && captureRange <= _mismatchCount) {
					// Play capture SCRS (type 9 → foot + 5999)
					uint16 scrsId = mapEventToScrsId(9, snoid);
					if (scrsId != 0) {
						Common::SeekableReadStream *scrsStream =
							_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
								ZmbResource(ZmbArchiveKind::kPage, scrsId));
						if (scrsStream) {
							snoid->startScrsPlayback(scrsStream, false, true);
						}
					}

					// Free the seat
					_seatOccupied[i] = false;
					_seatSnoidId[i] = 0;
					snoid->_packIsOccupied = false;
				}
			}
		}
		break;

	case 136:
		// IDA: fleens_flagRunnersCompleted event 136
		// Mark active raft anim runners as completed (set render)
		if (_activeRaftAnimSnoidId != 0) {
			ZmbSnoid *snoid = getSnoid(_activeRaftAnimSnoidId);
			if (snoid)
				snoid->activateRender();
		}
		break;

	case 137:
		// IDA: clear runner state — disable render
		// Shared by raftAnimStateMachine and raftMovementCallback
		if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			feature->deactivateRender();
		}
		break;

	case 140:
		// IDA: fleens_raftMovementCallback event 140
		// Link movement runner before raft runner.
		// NOTE: Original engine runner_linkRelativeToParent for Z-order.
		break;

	case 218:
		// IDA: fleens_raftMovementCallback event 218
		// Play random Fleen sound
		{
			uint16 sndId = _vm->_rnd->getRandomNumber(4100, 4124);
			_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, sndId),
			                          Audio::Mixer::kSFXSoundType);
		}
		break;

	default:
		if (eventCode >= kZmbAnimEvent240_BodyArrangePendFirst &&
		    eventCode <= kZmbAnimEvent243_BodyArrangePendLast) {
			// IDA: word_4AB1A0 = eventCode - 239
			_pendingBodyArrangement = eventCode - 239;
		} else if (eventCode >= kZmbAnimEvent250_BodyArrangeDirectFirst &&
		           eventCode <= kZmbAnimEvent253_BodyArrangeDirectLast) {
			// IDA: zmb_setBodyLayerShapes(eventCode - 250, a4 + 48)
			if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
				static_cast<ZmbSnoid *>(feature)->setBodyArrangement(eventCode - 250);
			}
		}
		break;
	}
}

} // End of namespace Mohawk
