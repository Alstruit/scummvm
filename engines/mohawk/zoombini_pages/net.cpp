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
#include "mohawk/zoombini_pages/net.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions from 0x4A286C (16 POINTS)
const Common::Point ZoombiniInteractiveNet::kSnoidPositions[16] = {
	Common::Point(233, 392), Common::Point(209, 378), Common::Point(196, 390), Common::Point(185, 365),
	Common::Point(167, 380), Common::Point(160, 408), Common::Point(135, 397), Common::Point(121, 407),
	Common::Point(115, 368), Common::Point(114, 342), Common::Point( 99, 375), Common::Point( 97, 394),
	Common::Point( 95, 346), Common::Point( 91, 411), Common::Point( 79, 355), Common::Point( 62, 404),
};

ZoombiniInteractiveNet::ZoombiniInteractiveNet(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kNet) {
}

ZoombiniInteractiveNet::~ZoombiniInteractiveNet() {
}

void ZoombiniInteractiveNet::open() {
	openArchive(ZMB_MHK_NET);
}

void ZoombiniInteractiveNet::setBackgroundMusic() {
	// IDA: net_puzzleInit (0x4361d4) has no music playback call on page load.
	// sound_activeHandle = 20064 is stored at end of funcInit for F1 replay only.
}

void ZoombiniInteractiveNet::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId((net_difficultyLevel >= 2) + 5000)
	// Background differs based on difficulty: 5000 or 5001
	uint16 bgId = (_difficultyLevel >= 2) ? 5001 : 5000;
	_vm->_gfx->setPalette(bgId);
	_vm->_gfx->drawBackground(bgId);
}

void ZoombiniInteractiveNet::loadFeatures() {
	// IDA: puzzleNet_4361D4 (0x4361d4)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// Initialize puzzle state
	// IDA: net_totalSlotCount = 25; if (diff > 1) net_totalSlotCount = 125;
	_totalSlotCount = (_difficultyLevel > 1) ? 125 : 25;
	_columnCount = (_difficultyLevel > 1) ? 3 : 2;
	_bAdvanceReady = false;
	_advanceButtonDirty = false;
	_columnLabelDirty = false;
	
	// Random attribute column offsets (0-4)
	// IDA: net_randAttrColOffset0 = nextRand(4); etc.
	_randAttrColOffset[0] = _vm->_rnd->getRandomNumber(0, 4);
	_randAttrColOffset[1] = _vm->_rnd->getRandomNumber(0, 4);
	_randAttrColOffset[2] = _vm->_rnd->getRandomNumber(0, 4);
	_prevAttrColOffset[0] = -1;
	_prevAttrColOffset[1] = -1;
	_prevAttrColOffset[2] = -1;

	// Preload shape images at tBMP 6000 (0x1770)
	// IDA: shape_loadSubShapesFromArchive(&stru_4A285C, 0x1770u)
	_vm->_gfx->preloadImage(6000);
	_vm->_gfx->preloadImage(7000);
	_vm->_gfx->preloadImage(8000);
	_vm->_gfx->preloadImage(9000);
	_vm->_gfx->preloadImage(10000);

	// Feature groups
	// IDA: scrb_useFeatureGroup(1, 0, 7000)
	// IDA: scrb_useFeatureGroup(0, 1, 8000)
	// IDA: scrb_useFeatureGroup(1, 2, 9000)
	// IDA: scrb_useFeatureGroup(0, 3, 10000)

	// Load main features: 48 SCRBs at 7000
	// IDA: scrb_loadMainFeatureSet(48, 7000)
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 8, 8000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 8; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 8000), 8000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 154, 9000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 154; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 9000), 9000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 19, 10000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 19; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 10000), 10000 + i);
		}
	}

	// Load reject pool: 3 at SCRS 14000
	// IDA: scrs_loadRejectPool(0, 3, 14000)
	for (uint16 i = 0; i < 3; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  14000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 51 at SCRS 13000
	// IDA: scrs_loadNormalPool(0, 51, 13000)
	for (uint16 i = 0; i < 51; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  13000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Register virtual render feature for attribute slot buttons
	// IDA: runner_registerAndAllocate(0, 0, 0, 0, 0, net_invalidateVisualRects2, fleens_renderAllAttrSlots_436785, 0x1000)
	{
		ZmbFeature::EventHooks attrSlotHooks;
		attrSlotHooks.setPreRenderFunc(reinterpret_cast<ZmbFeature::OnPreRenderFunc>(&ZoombiniInteractiveNet::attrSlots_preRender));
		attrSlotHooks.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniInteractiveNet::attrSlots_render));
		loadVirtualFeature(100, 0, ZmbFeature::FLAG_00001000_TOPMOST, attrSlotHooks);
	}

	// Load Zoombinis at 16 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, v12, 16)
	// IDA: SHPL_copyPaletteSrcToDst(236, 10)
	loadZoombinisFromPack();

	// Register column SCRB runners
	// IDA: net_registerAllSCRBRunners(v10, &unk_4A28AC)
	registerColumnRunners();

	// Layout and stagger walk-in with walkDelay=30
	// IDA: zmb_layoutStaticAndWalkInGroups(0)
	layoutStaticAndWalkIn();
	// IDA: zmb_assignStaggeredWalkDelays(0, 30) — base class uses default values
	assignStaggeredWalkDelays();

	// Buttons
	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(6000);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagNet);

	// IDA: sound_activeHandle = 20064 — net narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20064);
}

void ZoombiniInteractiveNet::onGoButtonActivated() {
	// IDA: net_onClickHandler case 2
	// Stop BGM, play departure SFX, walk snoids to (600, -100), fade out when SFX finishes.
	// IDA: scrb_enqueueSoundResource(0, 0) — stop background music
	_vm->_sound->stopAllSoundQueues();

	_departXferSrcSiPage = ZMB_SI_NET_12;
	// IDA: zmbMoveAnimation_45479D(45, -100, 600)
	startDepartWalkAnimation(Common::Point(600, -100));
	ZoombiniInteractive::onGoButtonActivated();
}

void ZoombiniInteractiveNet::loadZoombinisFromPack() {
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
}

void ZoombiniInteractiveNet::registerColumnRunners() {
	// IDA: net_registerAllSCRBRunners (0x437733)
	// Registers all the SCRB features needed for column-based sorting puzzle.
	
	// 5 column SCRB runners at 8000-8004
	// IDA: for i=0..4: net_columnScrbRunners[i] = registerSCRB(..., 6, i+8000, ..., 0x4180000)
	for (int16 i = 0; i < 5; i++) {
		_columnScrbFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 8000), 8000 + i, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_04000000_OVERLAY);
	}
	
	// Entry SCRB runner at 8005
	// IDA: net_entryScrbRunner = registerSCRB(..., 6, 8005, ..., 0x4180000)
	_entryScrbFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8005, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_04000000_OVERLAY);
	
	// Label SCRB runner: 9151 (diff<=1) or 9153 (diff>1)
	// IDA: net_labelScrbRunner = registerSCRB(..., 6, 9151/9153, ..., 0x4100000)
	uint16 labelScrbId = (_difficultyLevel > 1) ? 9153 : 9151;
	_labelScrbFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 9000), labelScrbId, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
	
	// Attribute animation SCRB runner at 7018
	// IDA: net_attrAnimScrbRunner = registerSCRB(..., 6, 7018, ..., 0x4188000)
	_attrAnimScrbFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7018, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);
	
	// Feedback SCRB runner at 10018
	// IDA: net_feedbackScrbRunner = registerSCRB(..., 6, 10018, ..., 0x188000)
	_feedbackScrbFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10018, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE);
	
	// Attribute column SCRB runners at random offsets
	// IDA: net_attrCol0ScrbRunner (only if diff>=2), net_attrCol1ScrbRunner, net_attrCol2ScrbRunner
	if (_difficultyLevel >= 2) {
		_attrColScrbFeatures[0] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 10002 + _randAttrColOffset[0], 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
	}
	_attrColScrbFeatures[1] = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10007 + _randAttrColOffset[1], 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
	_attrColScrbFeatures[2] = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10012 + _randAttrColOffset[2], 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
	
	// Exit SCRB runner at 7000
	// IDA: net_exitScrbRunner = registerSCRB(..., 6, 7000, ..., 0x4180000)
	_exitScrbFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7000, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_04000000_OVERLAY);
}

bool ZoombiniInteractiveNet::attrSlots_preRender(ZmbFeature *feature) {
	// IDA: net_invalidateVisualRects2 (0x4367A4)
	// Toggles dirty flags based on puzzle state and marks rects for redraw.
	//
	// The original function manages dirty rect invalidation:
	// - If net_advanceReady changes, toggle _advanceButtonDirty and invalidate rect
	// - Always set _columnLabelDirty and invalidate that rect
	//
	// In ScummVM we use simpler per-feature dirty tracking, so this just
	// manages our internal flags.

	// Check if advance button state changed
	if (_bAdvanceReady) {
		if (!_advanceButtonDirty) {
			_advanceButtonDirty = true;
		}
	} else {
		if (_advanceButtonDirty) {
			_advanceButtonDirty = false;
		}
	}

	// Column label is always marked dirty (feature always renders)
	_columnLabelDirty = true;
	
	// Return true to continue with rendering
	return true;
}

ZmbRenderResult ZoombiniInteractiveNet::attrSlots_render(ZmbFeature *feature) {
	// IDA: fleens_renderAllAttrSlots_436785 (0x436785)
	// Renders the attribute slot button sprites.
	//
	// The original calls:
	//   fleens_renderAttrSlotSCRB(0, 0, 1) — always render slot 1 (label area)
	//   fleens_renderAttrSlotSCRB(0, 0, 2) — render slot 2 (advance button, state-dependent)
	//
	// fleens_renderAttrSlotSCRB maps slot types:
	//   slot 1 -> SCRB shape 5/6 (label)
	//   slot 2 -> SCRB shape 1/2/3 (advance button, depends on net_advanceReady)
	//
	// For now, we rely on:
	// - Label feature (_labelScrbFeature) renders the column labels
	// - Go button (base class) handles advance state
	// So this callback is effectively a no-op until full puzzle logic is added.
	
	// Clear dirty flags after render
	_advanceButtonDirty = false;
	_columnLabelDirty = false;
	
	return ZmbRenderResult::kRendered;
}

// ---------------------------------------------------------------------------
// Animation event dispatch
// ---------------------------------------------------------------------------

void ZoombiniInteractiveNet::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
		processSnoidAnimEvent(feature, eventCode);
	} else {
		processScrbAnimEvent(feature, eventCode);
	}
}

void ZoombiniInteractiveNet::processSnoidAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// IDA: net_zmbAnimCallback (0x438EA1)
	// Handles Zoombini snoid animation events during net sorting.
	ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);

	if (eventCode == kZmbAnimEventM1_End) {
		// End-of-animation completion handler.
		// IDA: Two branches based on net_zmbsAtColumns:
		//   nonzero → exit animation: play anim at dword_4A27DE[exitPositionIdx++],
		//             set advanceReady, check completion.
		//   zero → walk-off cleanup: find walkSlotRunners match, clear slot,
		//          animateZoombini(0, 2), trigger feedback SCRB when all empty.
		// TODO: Implement full net sorting completion logic
		warning("Net: snoid event -1 (anim end) not fully implemented");
		return;
	}

	switch (eventCode) {
	case 0:
		// Toggle render visibility.
		// IDA: *(animData+290) = *(animData+290)==0;
		if (feature->isRenderActivated())
			feature->deactivateRender();
		else
			feature->activateRender();
		// Apply pending body arrangement.
		// IDA: if (net_pendingAnimShape) { zmb_setBodyLayerShapes(net_pendingAnimShape-1, ...); net_pendingAnimShape=0; }
		if (_pendingBodyArrangement) {
			snoid->setBodyArrangement(_pendingBodyArrangement - 1);
			_pendingBodyArrangement = 0;
		}
		break;

	case 2:
		// Spawn Zoombini at slot.
		// IDA: net_spawnZmbAtSlot(a1, net_currentSlotIndex)
		// TODO: Implement net_spawnZmbAtSlot
		warning("Net: snoid event 2 (spawn at slot) not yet implemented");
		break;

	case 4:
		// Start snoid travel to column with runner linking.
		// IDA: find runner, play SCRS (14000+colIdx) at kEntryPositions[colIdx],
		//      link 4-runner chain: activeSlotRunners → columnScrbRunners[0]
		//      → word_4B098C → word_4B098E → activeZmbRunner
		// TODO: Implement column travel start
		warning("Net: snoid event 4 (column travel) not yet implemented");
		break;

	case 20:
		// Activate runner at column slot, play arrival SCRS.
		// IDA: activate columnSlotRunners[colIdx], play SCRS (13016+offset),
		//      set pending zmb index, clear column slot, queue management
		// TODO: Implement column arrival
		warning("Net: snoid event 20 (column arrival) not yet implemented");
		break;

	case 30:
		// Play column exit SCRS, link runner chain.
		// IDA: play SCRS (13031+offset) at kExitPositions[colIdx],
		//      increment zmbsAtColumns, link lastLinkedRunner → activeZmbRunner
		// TODO: Implement column exit
		warning("Net: snoid event 30 (column exit) not yet implemented");
		break;

	default:
		if (eventCode >= kZmbAnimEvent240_BodyArrangePendFirst &&
		    eventCode <= kZmbAnimEvent243_BodyArrangePendLast) {
			// IDA: net_pendingAnimShape = a2 - 239
			_pendingBodyArrangement = eventCode - 239;
		} else if (eventCode >= kZmbAnimEvent250_BodyArrangeDirectFirst &&
		           eventCode <= kZmbAnimEvent253_BodyArrangeDirectLast) {
			// IDA: zmb_setBodyLayerShapes(a2 - 250, a3 + 48)
			snoid->setBodyArrangement(eventCode - 250);
		}
		break;
	}
}

void ZoombiniInteractiveNet::processScrbAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// IDA: net_scrbAnimCallback (0x43105B)
	// Handles SCRB feature traversal events using ASCII char-based codes.
	switch (eventCode) {
	case 50:  // '2': Load SCRB on row runner
		// IDA: find runner by word_4AF3F6[runnerPtr[69]],
		//      scrb_loadOnRunner(1, word_4A1D08[offset], runner)
		// TODO: Implement row SCRB loading
		warning("Net: SCRB event '2'/50 (load row SCRB) not yet implemented");
		break;

	case 61:  // '=': Play traversal script
	case 71:  // 'G': Play traversal script
	case 81:  // 'Q': Play traversal script
		// IDA: net_playTraversalScript(0, callback, runnerPtr[111], runnerPtr)
		// TODO: Implement traversal script playback
		warning("Net: SCRB event traversal script (%d) not yet implemented", eventCode);
		break;

	case 62:  // '>': Setup traversal step
		// IDA: net_setupTraversalStep(0, callback, runnerPtr[111], runnerPtr)
		// TODO: Implement traversal step setup
		warning("Net: SCRB event '>'/62 (traversal step) not yet implemented");
		break;

	case 64:  // '@': Store runner data
	case 84:  // 'T': Store runner data
		// IDA: word_4AFF88[word_4B00CA++] = runnerPtr[74]
		// TODO: Implement runner data storage
		warning("Net: SCRB event store data (%d) not yet implemented", eventCode);
		break;

	case 65:  // 'A': Spawn traversal runner (param=1)
		// IDA: net_spawnTraversalRunner(1, callback, runnerPtr[111], runnerPtr)
		// TODO: Implement runner spawning
		warning("Net: SCRB event 'A'/65 (spawn runner 1) not yet implemented");
		break;

	case 66:  // 'B': Set slot + clear
	case 76:  // 'L': Set slot + clear
	case 86:  // 'V': Set slot + clear
		// IDA: scrb_setSlotFeatureRunnerIdx(0, runnerPtr[68]),
		//      word_4AF33C[runnerPtr[68]] = 0
		// TODO: Implement slot assignment
		warning("Net: SCRB event set slot (%d) not yet implemented", eventCode);
		break;

	case 72:  // 'H': Setup traversal step (param=1)
	case 82:  // 'R': Setup traversal step (param=1)
		// IDA: net_setupTraversalStep(1, callback, runnerPtr[111], runnerPtr)
		// TODO: Implement traversal step setup (alternate)
		warning("Net: SCRB event traversal step alt (%d) not yet implemented", eventCode);
		break;

	case 74:  // 'J': Store + clear runner
		// IDA: word_4AFF88[word_4B00CA++] = runnerPtr[74]; runnerPtr[74] = 0
		// TODO: Implement store + clear
		warning("Net: SCRB event 'J'/74 (store+clear) not yet implemented");
		break;

	case 75:  // 'K': Spawn traversal runner (param=0)
	case 85:  // 'U': Spawn traversal runner (param=0)
		// IDA: net_spawnTraversalRunner(0, callback, runnerPtr[111], runnerPtr)
		// TODO: Implement runner spawning (param=0)
		warning("Net: SCRB event spawn runner 0 (%d) not yet implemented", eventCode);
		break;

	default:
		break;
	}
}

} // End of namespace Mohawk
