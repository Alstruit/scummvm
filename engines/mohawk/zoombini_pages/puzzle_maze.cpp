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
#include "mohawk/zoombini_pages/puzzle_maze.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions from 0x4A1F58 (20 POINTS)
const Common::Point ZoombiniInteractiveMaze::kSnoidPositions[20] = {
	Common::Point(287, 394), Common::Point(260, 426), Common::Point(224, 447), Common::Point(188, 441),
	Common::Point(157, 455), Common::Point(263, 384), Common::Point(219, 397), Common::Point(184, 388),
	Common::Point(155, 402), Common::Point(121, 417), Common::Point(226, 354), Common::Point(189, 349),
	Common::Point(156, 354), Common::Point(131, 375), Common::Point( 85, 394), Common::Point(164, 311),
	Common::Point(125, 324), Common::Point( 79, 352), Common::Point( 29, 318), Common::Point( 15, 285),
};

// IDA: word_4A1CB4 - has shadow flag for each creature slot (14 entries)
// 0 = no shadow, 1 = has shadow
const int16 ZoombiniInteractiveMaze::kCreatureHasShadow[14] = {
	0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1
};

// IDA: word_4A1CD0 - creature type ID for each slot (14 entries)
// 0 = base type (goes in word_4AF3F6[0])
// 1 = type 1 bank (goes in word_4AF3F6[1])
// 2 = type 2 bank (goes in word_4AF3F6[2])
const int16 ZoombiniInteractiveMaze::kCreatureTypeId[14] = {
	0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2
};

// IDA: word_4A1CEC - SCRB resource ID for each creature slot (14 entries)
// These are the base SCRB IDs; some creatures use SCRB_ID+1 for shadows
const int16 ZoombiniInteractiveMaze::kCreatureScrbId[14] = {
	9000, 9000, 9000, 9001, 9001, 9001, 9001, 9001, 9001, 9000, 9000, 9000, 9003, 9003
};

ZoombiniInteractiveMaze::ZoombiniInteractiveMaze(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kMaze) {
}

ZoombiniInteractiveMaze::~ZoombiniInteractiveMaze() {
}

void ZoombiniInteractiveMaze::open() {
	// MIDIMPC.MHK contains MIDI BGM (tMID 30035-30038) — Broderbund v1.x only.
	if (!_vm->isGameVariant(GF_ZMB_TLC))
		openArchive(ZMB_MHK_MIDIMPC);
	openArchive(ZMB_MHK_MAZE2);
}

void ZoombiniInteractiveMaze::setBackgroundMusic() {
	// IDA: maze2_initAndSetup (0x42e47c) at 0x42f573:
	//   scrb_enqueueSoundResource(30035 + routeDiffLevel)
	// Always plays MIDI BGM — no difficulty check.
	// Note: maze _difficultyLevel == routeLevel (no +1 offset).
	// TLC v2.0 has no MIDI resources.
	if (!_vm->isGameVariant(GF_ZMB_TLC)) {
		int16 routeLevel = _vm->_state->readActivePageRouteLevel();
		_vm->_midi->playZmbMidi(ZmbResource(ZmbArchiveKind::kPage, (uint16)(30035 + routeLevel)));
	}
}

void ZoombiniInteractiveMaze::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniInteractiveMaze::loadFeatures() {
	// IDA: puzzleMaze2_42E47C (0x42e47c)
	// Most complex puzzle init — large grid runner setup
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(100)
	loadTerrainBitmap(100);

	// Feature groups
	// IDA: scrb_useFeatureGroup(0, 0, 7000)
	// IDA: scrb_useFeatureGroup(0, 1, 8000)
	// IDA: scrb_useFeatureGroup(0, 2, 9000)
	// IDA: scrb_useFeatureGroup(0, 3, 10000)
	// IDA: scrb_useFeatureGroup(0, 4, 12000)

	// Preload shape images at tBMP 5100 (0x13EC)
	// IDA: shape_loadSubShapesFromArchive(&stru_4AF294, 0x13ECu)
	_vm->_gfx->preloadImage(5100);
	_vm->_gfx->preloadImage(7000);
	_vm->_gfx->preloadImage(8000);
	_vm->_gfx->preloadImage(9000);
	_vm->_gfx->preloadImage(10000);
	_vm->_gfx->preloadImage(12000);

	// Load main features: 28 SCRBs at 7000
	// IDA: scrb_loadMainFeatureSet(28, 7000)
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 14, 8000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 14; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 8000), 8000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 8, 9000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 8; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 9000), 9000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 44, 10000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 44; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 10000), 10000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 2, 12000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 2; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 12000), 12000 + i);
		}
	}

	// Load REGS resources for grid configuration
	// IDA: regs_loadAndByteSwap(&unk_4AF2A8, 16000)
	// IDA: net_loadRegsResource(&unk_4AF2F4, 16501, ...)
	// IDA: regs_loadAndByteSwap(&unk_4AF2B0, 17000)
	// IDA: regs_loadAndByteSwap(&unk_4AF2B4, 17001)
	// IDA: regs_loadAndByteSwap(&MEMORY[0x4AF298], 18000)
	// IDA: regs_loadAndByteSwap(&MEMORY[0x4AF29C], 18001)

	// Load reject pool: 8 at SCRS 14000
	// IDA: scrs_loadRejectPool(0, 8, 14000)
	for (uint16 i = 0; i < 8; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 5100),
				  14000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 96 at SCRS 15000
	// IDA: scrs_loadNormalPool(0, 96, 15000)
	for (uint16 i = 0; i < 96; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 5100),
				  15000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// IDA: SHPL_copyPaletteSrcToDst(236, 10)

	// IDA 0x42ea74: word_4AF2FA - overlay anim feature
	// SCRB 12001, interval=7, OVERLAY|LOOP_ANIM|DEFER_ANIM|PLAY_ONCE
	_overlayAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 12000), 12001, 7,
		ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);

	// Load Zoombinis at 20 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, v31, 20)
	loadZoombinisFromPack();

	// IDA: maze_loadRegsConfigByLevel — select REGS resource for creature/obstacle config
	loadRegsConfigByLevel();
	
	// Load and parse REGS data, then create creature features
	// IDA: maze_regsDataPtr = maze_loadRegsConfigByLevel(unk_4AF302)
	// IDA: Multiple loops creating creature runners based on REGS data
	loadAndParseRegsData();
	createCreatureFeatures();

	// IDA 0x42eea8: word_4AF3F6[0] - creature base animation
	// SCRB 9005, interval=7, DEFER_ANIM|PLAY_ONCE|LOOP_ANIM
	_creatureBaseFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 9000), 9005, 7,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// IDA 0x42eed2-0x42ef3c: NoOp runner layers (word_4AF45C[0..10])
	// SCRB 8011, interval=0, noOp callbacks, OVERLAY|LOOP_ANIM
	for (int i = 0; i < 11; i++) {
		_noopFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 8000), 8011, 0,
			ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM);
	}

	// IDA 0x42f378: final SCRB 8011, OVERLAY|LOOP_ANIM
	loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8011, 0,
		ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// IDA 0x42f399: SCRB 8004, OVERLAY
	_finalOverlayA = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8004, 0,
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA 0x42f3ba: SCRB 8000, OVERLAY
	_finalOverlayB = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8000, 0,
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA 0x42f3bf-0x42f3f4: NoOp runner 11, SCRB 8011, OVERLAY|LOOP_ANIM
	_noopFeatures[11] = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8011, 0,
		ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// Layout and stagger walk-in
	// IDA: zmb_layoutStaticAndWalkInGroups(0)
	layoutStaticAndWalkIn();
	// IDA: zmb_assignStaggeredWalkDelays(30, 45)
	assignStaggeredWalkDelays();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(6000);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagMaze);

	// IDA: sound_activeHandle = 20068 — maze narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20068);

	// Celebration state init (IDA: maze2_initAndSetup @ 0x42E47C)
	_celebrationTrigger = false;
	_celebrationsPlayed = 0;
	// IDA: word_4B0408 set from word_4AF306 during completion handler
	_celebrationTarget = 0;
	_celebrationPoolState = 0;
	_celebrationLastFrame = 0;
}

void ZoombiniInteractiveMaze::onGoButtonActivated() {
	// IDA: maze_onClickHandler case 2 -> puzzle_pendingTransitionTarget = 6 (Town)
	// Route 4: Maze -> Town (via Xfer)
	_departXferSrcSiPage = ZMB_SI_MAZE_16;
	ZoombiniInteractive::onGoButtonActivated();
}

void ZoombiniInteractiveMaze::loadZoombinisFromPack() {
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
		}
		posIdx++;
	}

	_loadedZmbCount = posIdx;
}

void ZoombiniInteractiveMaze::loadRegsConfigByLevel() {
	// IDA: maze_loadRegsConfigByLevel (0x4319C9)
	// Selects REGS resource based on difficulty level and random variant.
	// REGS resources configure creature/obstacle placement on the maze grid.
	//
	// Level 0: REGS 16600 + variant (0-1), 2 variants
	// Level 1: REGS 16602 + variant (0-1), 2 variants
	// Level 2: REGS 16604 + variant (0-1), 2 variants
	// Level 3: REGS 16606 + variant (0-2), 3 variants
	// Default (level 4): REGS 16609, fixed debug layout
	
	switch (_difficultyLevel) {
	case 0:
		_levelVariantIdx = _vm->_rnd->getRandomNumber(0, 1);
		_regsResourceId = 16600 + _levelVariantIdx;
		break;
	case 1:
		_levelVariantIdx = _vm->_rnd->getRandomNumber(0, 1);
		_regsResourceId = 16602 + _levelVariantIdx;
		break;
	case 2:
		_levelVariantIdx = _vm->_rnd->getRandomNumber(0, 1);
		_regsResourceId = 16604 + _levelVariantIdx;
		break;
	case 3:
		_levelVariantIdx = _vm->_rnd->getRandomNumber(0, 2);
		_regsResourceId = 16606 + _levelVariantIdx;
		break;
	default:
		_regsResourceId = 16609;
		_levelVariantIdx = 0;
		break;
	}
	
	debugC(kZmbDebugPage, "Maze: level %d, variant %d, REGS %d",
	       _difficultyLevel, _levelVariantIdx, _regsResourceId);
}

void ZoombiniInteractiveMaze::loadAndParseRegsData() {
	// IDA: regs_loadAndByteSwap (0x452374) + maze parsing in puzzleMaze2_42E47C
	// Load REGS resource and parse creature slot assignments.
	//
	// REGS format (big-endian int16 array):
	// - regs[0]: Total creature count (stored in word_4AFF80)
	// - regs[1-9]: Creature slot index for each maze column (0 = no creature)
	// - regs[10+]: Per-creature configuration records (10 words each)
	
	Common::SeekableReadStream *stream = _vm->getResource(ID_REGS, ZmbResource(ZmbArchiveKind::kPage, _regsResourceId));
	if (!stream) {
		warning("ZoombiniInteractiveMaze: Failed to load REGS %d", _regsResourceId);
		return;
	}
	
	// REGS data is big-endian int16 array
	uint32 dataSize = stream->size();
	uint32 wordCount = dataSize / 2;
	
	_regsData.clear();
	_regsData.resize(wordCount);
	
	for (uint32 i = 0; i < wordCount; i++) {
		_regsData[i] = stream->readSint16BE();
	}
	delete stream;
	
	// Parse header
	if (wordCount < 10) {
		warning("ZoombiniInteractiveMaze: REGS %d too small (%u words)", _regsResourceId, wordCount);
		return;
	}
	
	// regs[0] = total creature count (IDA: word_4AFF80)
	_totalCreatureCount = _regsData[0];
	
	// regs[1-9] = creature slot for each maze column
	// Clear slots first
	for (int i = 0; i < 10; i++) {
		_creatureSlots[i] = 0;
	}
	
	// Copy slots 1-9 (column 0 is unused)
	for (int col = 1; col <= 9; col++) {
		_creatureSlots[col] = _regsData[col];
	}
	
	debugC(1, kZmbDebugScript, "Maze REGS %d: count=%d, slots=[%d %d %d %d %d %d %d %d %d]",
		_regsResourceId, _totalCreatureCount,
		_creatureSlots[1], _creatureSlots[2], _creatureSlots[3],
		_creatureSlots[4], _creatureSlots[5], _creatureSlots[6],
		_creatureSlots[7], _creatureSlots[8], _creatureSlots[9]);
}

void ZoombiniInteractiveMaze::createCreatureFeatures() {
	// IDA: Multiple creature creation loops in puzzleMaze2_42E47C
	// Creates creature features based on parsed REGS data.
	//
	// Loop 1 (0x42eae0-0x42eb49): Creates type 1 creatures (word_4A1CD0[] == 1)
	// Loop 2 (0x42eb5d-0x42eba7): Creates grid cell creatures (word_4AF362[])
	// Loop 3 (0x42eba9-0x42ec1a): Creates draw-on-region runners
	// Loop 4 (0x42ec1c-0x42ee1b): Creates obstacle features for slots 7-9
	// Loop 5 (0x42efab-0x42f2d0): Creates remaining obstacle features
	
	// Phase 1: Create type 1 creature runners (IDA: word_4AF3F6[1..2])
	// Loop through columns 1-9, check if creature type == 1
	for (int col = 1; col <= 9; col++) {
		int16 slot = _creatureSlots[col];
		if (slot == 0)
			continue;
		
		int16 slotIdx = slot - 1;  // 0-based index
		if (slotIdx < 0 || slotIdx >= 14)
			continue;
		
		int16 typeId = kCreatureTypeId[slotIdx];
		
		// Type 1 creatures go into _creatureSlotFeatures[1]
		if (typeId == 1 && _creatureSlotFeatures[1] == nullptr) {
			// IDA: word_4AF3F6[word_4A1CD0[v4]] = runner_registerAndAllocate(...)
			// SCRB = typeId + 9005 = 9006
			_creatureSlotFeatures[1] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 9000), 9006, 7,
				ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
				ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
		}
	}
	
	// Phase 2: Create grid cell creature features (IDA: word_4AF362[])
	for (int col = 1; col <= 9; col++) {
		int16 slot = _creatureSlots[col];
		if (slot == 0)
			continue;
		
		int16 slotIdx = slot - 1;
		if (slotIdx < 0 || slotIdx >= 14)
			continue;
		
		// IDA: word_4AF362[v6-1] = runner_registerAndAllocate(..., v6-1+7000, ...)
		// SCRB = slotIdx + 7000
		_gridCreatureFeatures[slotIdx] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 7000), 7000 + slotIdx, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY |
			ZmbFeature::FLAG_00100000_PLAY_ONCE);
	}
	
	// Phase 3: Create obstacle features for matching slots
	for (int col = 1; col <= 9; col++) {
		int16 slot = _creatureSlots[col];
		if (slot == 0)
			continue;
		
		int16 slotIdx = slot - 1;
		if (slotIdx < 0 || slotIdx >= 14)
			continue;
		
		int16 scrbId = kCreatureScrbId[slotIdx];
		bool hasShadow = (kCreatureHasShadow[slotIdx] != 0);
		
		// IDA: word_4AF3FC[v16] = runner_registerAndAllocate(..., word_4A1CEC[v16], ...)
		_creatureObstacleFeatures[slotIdx] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 9000), scrbId, 7,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY |
			ZmbFeature::FLAG_00080000_DEFER_ANIM);
		
		// Create shadow feature if needed
		// IDA: word_4AF418[v9] = runner_registerAndAllocate(..., word_4A1CEC[v9]+1, ...)
		if (hasShadow) {
			_creatureShadowFeatures[slotIdx] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 9000), scrbId + 1, 7,
				ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY |
				ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00800000_POS_DELTA);
		}
	}
	
	// Phase 4: Create type 2 creature runners (IDA: word_4AF3F6[2])
	for (int col = 1; col <= 9; col++) {
		int16 slot = _creatureSlots[col];
		if (slot == 0)
			continue;
		
		int16 slotIdx = slot - 1;
		if (slotIdx < 0 || slotIdx >= 14)
			continue;
		
		int16 typeId = kCreatureTypeId[slotIdx];
		
		// Type 2 creatures go into _creatureSlotFeatures[2]
		if (typeId == 2 && _creatureSlotFeatures[2] == nullptr) {
			// SCRB = typeId + 9005 = 9007
			_creatureSlotFeatures[2] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 9000), 9007, 7,
				ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
				ZmbFeature::FLAG_00008000_LOOP_ANIM);
		}
	}
}

// ---------------------------------------------------------------------------
// Animation event dispatch
// ---------------------------------------------------------------------------

void ZoombiniInteractiveMaze::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// IDA: Multiple callbacks share the maze event system:
	//   maze_obstacleAnimCallback (0x4222FA) — obstacle/room animations (shared with Hotel)
	//   maze_scriptEventHandler (0x425D55) — grid SCRB events 3-5 (shared with Lilly)
	//   maze_runnerExitCallback (0x425CCA) — runner exit events 1-3
	// Only standard snoid events are implemented here; grid events require full
	// grid state machine which is not yet implemented.

	switch (eventCode) {
	case kZmbAnimEventM1_End:
		// End-of-animation.
		// IDA (maze_obstacleAnimCallback): find runner, calculate target position
		//      based on difficulty and room slot, play SCRS 14000+offset.
		// TODO: Implement maze completion/reject animation
		warning("Maze: event -1 (anim end) not fully implemented");
		break;

	case 0:
		// Toggle render visibility.
		// IDA: *(timerData+290) = *(timerData+290)==0;
		if (feature->isRenderActivated())
			feature->deactivateRender();
		else
			feature->activateRender();
		// Apply pending body arrangement (only for snoid features).
		// IDA: if (word_4AB7C6) { zmb_setBodyLayerShapes(word_4AB7C6-1, ...); word_4AB7C6=0; }
		if (_pendingBodyArrangement && feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			static_cast<ZmbSnoid *>(feature)->setBodyArrangement(_pendingBodyArrangement - 1);
			_pendingBodyArrangement = 0;
		}
		break;

	case 1: case 2: case 3: case 4: case 5:
		// Grid runner events from maze_scriptEventHandler / maze_runnerExitCallback.
		// These drive the grid state machine for runner arrivals, exits, and cell swaps.
		// TODO: Implement maze grid state machine
		warning("Maze: event %d (grid runner) not yet implemented", eventCode);
		break;

	case 15:
		// Link overlay runner, set obstacle flags.
		// IDA (maze_obstacleAnimCallback): link headerOverlayRunner before word_4AB7BE,
		//      set flags, store scroll data.
		// TODO: Implement obstacle overlay linking
		warning("Maze: event 15 (obstacle overlay) not yet implemented");
		break;

	default:
		if (eventCode >= kZmbAnimEvent240_BodyArrangePendFirst &&
		    eventCode <= kZmbAnimEvent243_BodyArrangePendLast) {
			// IDA: word_4AB7C6 = timerIdx - 239
			_pendingBodyArrangement = eventCode - 239;
		} else if (eventCode >= kZmbAnimEvent250_BodyArrangeDirectFirst &&
		           eventCode <= kZmbAnimEvent253_BodyArrangeDirectLast) {
			// IDA: zmb_setBodyLayerShapes(timerIdx - 250, timerData + 48)
			if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
				static_cast<ZmbSnoid *>(feature)->setBodyArrangement(eventCode - 250);
			}
		}
		break;
	}
}

// ---------------------------------------------------------------------------
// onEveryFrame: Per-frame celebration scheduling.
// IDA: maze2_onHover_frameUpdate @ 0x42FF46
// ---------------------------------------------------------------------------
void ZoombiniInteractiveMaze::onEveryFrame() {
	if (_loadedZmbCount <= 0)
		return;

	if (_celebrationTrigger && _celebrationsPlayed < _celebrationTarget) {
		if (getCurrentFrameCounter() - _celebrationLastFrame > 30) {
			bool triggered = false;
			_celebrationLastFrame = getCurrentFrameCounter();

			for (int16 i = 0; i < _loadedZmbCount && !triggered; i++) {
				uint16 poolIdx = _vm->_rnd->getNonRepeatRandom(_loadedZmbCount, _celebrationPoolState);
				uint16 snoidId = 10000 + poolIdx;
				ZmbSnoid *snoid = getSnoid(snoidId);

				if (snoid && snoid->isRenderActivated()) {
					// IDA: snoidScript_initAndPlay(0, 0, byte_239 + 15090, core)
					uint16 scrsId = snoid->_trait._foot + 15090;
					Common::SeekableReadStream *scrsStream =
						_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
							ZmbResource(ZmbArchiveKind::kPage, scrsId));
					if (scrsStream) {
						snoid->startScrsPlayback(scrsStream, false, true);
						_celebrationsPlayed++;
						triggered = true;
					}
				}
			}
		}
	} else if (_celebrationsPlayed >= _celebrationTarget && _celebrationTarget > 0) {
		_celebrationPoolState = 0;
		_celebrationLastFrame = 0;
		_celebrationTrigger = false;
		_celebrationsPlayed = 0;
	}
}

} // End of namespace Mohawk
