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
#include "mohawk/zoombini_pages/fleens.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions from 0x4A0FE8 (16 POINTS)
const Common::Point ZoombiniInteractiveFleens::kSnoidPositions[16] = {
	Common::Point(238, 368), Common::Point(185, 417), Common::Point(155, 448), Common::Point(197, 396),
	Common::Point(160, 357), Common::Point(164, 384), Common::Point(150, 416), Common::Point(116, 357),
	Common::Point(130, 386), Common::Point(109, 418), Common::Point(117, 448), Common::Point( 74, 348),
	Common::Point( 89, 384), Common::Point( 67, 418), Common::Point( 76, 450), Common::Point( 56, 379),
};

// IDA: raft DRAW_ON_REG position at 0x4A1028
const Common::Point ZoombiniInteractiveFleens::kRaftPosition(438, 357);

ZoombiniInteractiveFleens::ZoombiniInteractiveFleens(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kFleens) {
}

ZoombiniInteractiveFleens::~ZoombiniInteractiveFleens() {
}

void ZoombiniInteractiveFleens::open() {
	openArchive(ZMB_MHK_FLEENS);
}

void ZoombiniInteractiveFleens::setBackgroundMusic() {
	// IDA: fleens_initAndSetupPuzzle (0x41c1e0) has no music playback call on page load.
	// sound_activeHandle is stored at end of funcInit for F1 replay only.
}

void ZoombiniInteractiveFleens::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(300)
	_vm->_gfx->setPalette(300);
	_vm->_gfx->drawBackground(300);
}

void ZoombiniInteractiveFleens::loadFeatures() {
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
		attrSlotHooks.setPreRenderFunc(reinterpret_cast<ZmbFeature::OnPreRenderFunc>(&ZoombiniInteractiveFleens::attrSlots_preRender));
		attrSlotHooks.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniInteractiveFleens::attrSlots_render));
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
	_idleAnimTarget = 0; // Set by fleens_onRaftExitComplete when raft exits
	_idleAnimLastFrame = 0;
	_idleAnimInterval = 60; // IDA: dword_4AB43C = s_updateMode ? 120 : 60
	_idleAnimPoolState = 0;
}

void ZoombiniInteractiveFleens::onGoButtonActivated() {
	// IDA: fleens_onClickHandler case 2 -> puzzle_pendingTransitionTarget = 14
	// Route 3: Fleens -> Hotel (via Xfer)
	_departXferSrcSiPage = ZMB_SI_FLEENS_10;
	ZoombiniInteractive::onGoButtonActivated();
}

// ---------------------------------------------------------------------------
// onEveryFrame: Idle celebration animation for waiting Zoombinis.
// IDA: fleens_onHoverPerFrame @ 0x41CA77
// Triggered after first raft exit completes (sets _idleAnimTarget > 0).
// Only targets Zoombinis on the left shore (x <= 270).
// ---------------------------------------------------------------------------
void ZoombiniInteractiveFleens::onEveryFrame() {
	if (_loadedZmbCount <= 0)
		return;

	if (!_bInteractionAllowed || _idleAnimCount >= _idleAnimTarget)
		return;

	if (getCurrentFrameCounter() - _idleAnimLastFrame <= _idleAnimInterval)
		return;
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
			// IDA: fleens_eventToScrsId_41E860 type 5 → foot + 7030
			uint16 scrsId = snoid->_trait._foot + 7030;
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
					ZmbResource(ZmbArchiveKind::kPage, scrsId));
			if (scrsStream) {
				snoid->startScrsPlayback(scrsStream, false, true);
				_idleAnimCount++;
				triggered = true;
			}
		} else if (++attempts > 20) {
			triggered = true;
		}
	} while (!triggered);
}

void ZoombiniInteractiveFleens::loadZoombinisFromPack() {
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

void ZoombiniInteractiveFleens::buildZmbTraitSetup() {
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

bool ZoombiniInteractiveFleens::attrSlots_preRender(ZmbFeature *feature) {
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

ZmbRenderResult ZoombiniInteractiveFleens::attrSlots_render(ZmbFeature *feature) {
	// IDA: fleens_renderAttrSlotSCRB_4366CB
	// Just clear dirty flags for now - actual sprite rendering handled by SCRB features
	_raftButtonDirty = false;
	_attrSlot1Dirty = false;
	_attrSlot2Dirty = false;
	return ZmbRenderResult::kRendered;
}

// ---------------------------------------------------------------------------
// Animation event dispatch
// ---------------------------------------------------------------------------

void ZoombiniInteractiveFleens::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// IDA: Two callbacks share the Fleens event system:
	//   fleens_raftAnimStateMachine (0x41E1BD) — main raft state machine (events 0, 4-9, 28, 30,
	//       132-135, 137, 240-253)
	//   fleens_raftMovementCallback (0x41E075) — sub-feature movement (events -1, 0, 1, 2, 137,
	//       140, 218)
	// In the original, different features have different callbacks. In ScummVM both come here.

	switch (eventCode) {
	case kZmbAnimEventM1_End:
		// End-of-animation.
		// raftAnimStateMachine: reset raft state (word_4A4764=64, clear flags).
		// raftMovementCallback: clear runner field 112.
		// TODO: Implement full raft animation completion logic
		warning("Fleens: event -1 (anim end) not fully implemented");
		break;

	case 0:
		// Toggle render visibility.
		// IDA: *(a4+290) = *(a4+290)==0
		if (feature->isRenderActivated())
			feature->deactivateRender();
		else
			feature->activateRender();
		// Apply pending body arrangement (only for snoid features).
		// IDA: if (word_4AB1A0) { zmb_setBodyLayerShapes(word_4AB1A0-1, a4+48); word_4AB1A0=0; }
		if (_pendingBodyArrangement && feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			static_cast<ZmbSnoid *>(feature)->setBodyArrangement(_pendingBodyArrangement - 1);
			_pendingBodyArrangement = 0;
		}
		// IDA: conditional linking of overlay runner if word_4AB1CC && runner matches word_4AB1BA
		// TODO: Implement overlay runner linking on event 0
		break;

	case 1:
		// Link auxiliary runner (if not already linked).
		// IDA: if (!word_4AB1B6) { link word_4AB1A8 before word_4AB1B8; set flag; re-init SCRB }
		// TODO: Implement auxiliary runner linking
		warning("Fleens: event 1 (aux runner link) not yet implemented");
		break;

	case 2:
		// Link movement runner after raft runner.
		// IDA: runner_linkRelativeToParent(word_4AB1BA, 1, word_4AB1B8)
		// TODO: Implement movement runner linking
		warning("Fleens: event 2 (link after raft) not yet implemented");
		break;

	case 4: case 5: case 6: case 7: case 8: case 9:
		// Raft animation state machine phases.
		// IDA: fleens_raftAnimStateMachine events 4-9 — complex state transitions,
		//      runner SCRB resolving, position setup, linking, mismatch processing.
		// TODO: Implement raft state machine phases
		warning("Fleens: event %d (raft state phase) not yet implemented", eventCode);
		break;

	case 28:
		// Alias for event 8 (self-recursive call in original).
		// IDA: fleens_raftAnimStateMachine(8, a4)
		// TODO: Implement event 28 → 8 forwarding
		warning("Fleens: event 28 (link chain) not yet implemented");
		break;

	case 30:
		// Register new overlay SCRB runner.
		// IDA: register runner with random SCRB 1004-1006, set exit callback.
		// TODO: Implement overlay runner registration
		warning("Fleens: event 30 (register overlay) not yet implemented");
		break;

	case 132:
		// Play snoid SCRS 7016 on departure queue runners.
		// IDA: iterate departure queue, snoidScript_initAndPlay(7016+offset), load SCRBs.
		// TODO: Implement departure queue animation
		warning("Fleens: event 132 (departure queue anim) not yet implemented");
		break;

	case 133: case 134: case 135:
		// Range-check runner offset 288, apply capture SCRB 14 to matching runners.
		// IDA: iterate 16 runners, check byte at offset 288 within range,
		//      scrb_resolveToResourceId(14, runner), register with raftMovementCallback.
		// TODO: Implement capture animation
		warning("Fleens: event %d (capture anim) not yet implemented", eventCode);
		break;

	case 137:
		// Clear runner state (shared by both callbacks).
		// IDA: *(a4+224) = 0
		// TODO: Map runner offset 224 to ScummVM field
		break;

	case 140:
		// Link movement runner before raft runner.
		// IDA: runner_linkRelativeToParent(word_4AB1BA, 0, word_4AB1B8)
		// TODO: Implement pre-link
		warning("Fleens: event 140 (pre-link) not yet implemented");
		break;

	case 218:
		// Play random Fleen sound.
		// IDA: nextRand_410705(4100, 4124) → scrb_enqueueSoundResource
		{
			uint16 sndId = _vm->_rnd->getRandomNumber(4100, 4124);
			_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, sndId),
			                          Audio::Mixer::kSFXSoundType);
		}
		break;

	default:
		if (eventCode >= kZmbAnimEvent240_BodyArrangePendFirst &&
		    eventCode <= kZmbAnimEvent243_BodyArrangePendLast) {
			// IDA: word_4AB1A0 = a5 - 239
			_pendingBodyArrangement = eventCode - 239;
		} else if (eventCode >= kZmbAnimEvent250_BodyArrangeDirectFirst &&
		           eventCode <= kZmbAnimEvent253_BodyArrangeDirectLast) {
			// IDA: zmb_setBodyLayerShapes(a5 - 250, a4 + 48)
			if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
				static_cast<ZmbSnoid *>(feature)->setBodyArrangement(eventCode - 250);
			}
		}
		break;
	}
}

} // End of namespace Mohawk
