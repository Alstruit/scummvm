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
#include "mohawk/zoombini_pages/puzzle_tunnels.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// =========================================================================
// Static Data Tables
// =========================================================================

// IDA: pedestal positions at 0x4A7534 (16 POINTS)
const Common::Point ZoombiniPuzzleTunnels::kSnoidPositions[16] = {
	Common::Point(399, 402), Common::Point(367, 398), Common::Point(337, 397), Common::Point(306, 400),
	Common::Point(274, 400), Common::Point(240, 403), Common::Point(381, 424), Common::Point(351, 424),
	Common::Point(322, 428), Common::Point(292, 422), Common::Point(261, 426), Common::Point(371, 458),
	Common::Point(342, 459), Common::Point(310, 457), Common::Point(277, 457), Common::Point(245, 459),
};

// IDA: tunnel entry positions at 0x4A7674 (4 POINTS)
const Common::Point ZoombiniPuzzleTunnels::kTunnelEntryPositions[4] = {
	Common::Point(98, 424), Common::Point(178, 415), Common::Point(453, 421), Common::Point(533, 430),
};

// IDA: door index mapping at 0x4A7684
const int16 ZoombiniPuzzleTunnels::kDoorIndices[4] = { 1, 2, 0, 3 };

// IDA: SCRS replay positions at 0x4A76AA
const Common::Point ZoombiniPuzzleTunnels::kScrsReplayPositions[4] = {
	Common::Point(145, 455), Common::Point(210, 434), Common::Point(430, 434), Common::Point(476, 455),
};

// IDA: Gate positions at 0x4A7574, 0x4A75B4, 0x4A75F4, 0x4A7634 (4 gates x 16 positions)
const Common::Point ZoombiniPuzzleTunnels::kGatePositions[4][16] = {
	// Gate 0 (0x4A7574)
	{
		Common::Point(277, 62),  Common::Point(264, 63),  Common::Point(247, 64),  Common::Point(230, 66),
		Common::Point(274, 84),  Common::Point(255, 86),  Common::Point(236, 90),  Common::Point(214, 92),
		Common::Point(273, 102), Common::Point(255, 104), Common::Point(235, 108), Common::Point(215, 112),
		Common::Point(258, 120), Common::Point(239, 128), Common::Point(220, 130), Common::Point(200, 133),
	},
	// Gate 1 (0x4A75B4)
	{
		Common::Point(403, 60),  Common::Point(381, 61),  Common::Point(362, 64),  Common::Point(346, 69),
		Common::Point(412, 80),  Common::Point(392, 84),  Common::Point(372, 87),  Common::Point(353, 93),
		Common::Point(414, 98),  Common::Point(401, 103), Common::Point(382, 107), Common::Point(363, 110),
		Common::Point(415, 118), Common::Point(403, 121), Common::Point(387, 123), Common::Point(370, 127),
	},
	// Gate 2 (0x4A75F4)
	{
		Common::Point(288, 213), Common::Point(273, 219), Common::Point(257, 223), Common::Point(238, 226),
		Common::Point(222, 230), Common::Point(283, 235), Common::Point(268, 239), Common::Point(252, 245),
		Common::Point(237, 248), Common::Point(221, 252), Common::Point(287, 257), Common::Point(270, 260),
		Common::Point(253, 262), Common::Point(240, 265), Common::Point(220, 270), Common::Point(259, 280),
	},
	// Gate 3 (0x4A7634)
	{
		Common::Point(414, 217), Common::Point(389, 223), Common::Point(373, 228), Common::Point(357, 233),
		Common::Point(415, 238), Common::Point(399, 247), Common::Point(382, 249), Common::Point(362, 255),
		Common::Point(419, 259), Common::Point(400, 263), Common::Point(381, 267), Common::Point(363, 276),
		Common::Point(420, 271), Common::Point(401, 278), Common::Point(387, 283), Common::Point(371, 268),
	},
};

// IDA: hoverData to gate type mapping at stru_4A750C (kHoverDataToGateType)
const int16 ZoombiniPuzzleTunnels::kHoverDataToGateType[8] = {
	0, 1, 0, 2, 2, 3, 1, 3
};

// IDA: spawn origin X positions at 0x4A76A2
const int16 ZoombiniPuzzleTunnels::kSpawnOriginX[4] = { 141, 198, 426, 479 };

// IDA: preferred staging slots at 0x4A76BA/0x4A76BE
// [0] = preferred slots for side=0 (left), [1] = preferred slots for side=1 (right)
const int16 ZoombiniPuzzleTunnels::kPreferredSlots[2][2] = {
	{ 15, 10 },  // side 0 at 0x4A76BA
	{ 11, 6 },   // side 1 at 0x4A76BE
};

// =========================================================================
// Feedback / Hint Sound Pools
// =========================================================================

// IDA: Rejection pool for gate type 0 at 0x4A73B0 (SCRB 4000–4009)
const int16 ZoombiniPuzzleTunnels::kWrongPool0[10] = {
	4000, 4001, 4002, 4003, 4004, 4005, 4006, 4007, 4008, 4009
};

// IDA: Correct hint pool (small) at 0x4A73C8 (SCRB 4010–4020)
const int16 ZoombiniPuzzleTunnels::kCorrectHintSmall[11] = {
	4010, 4011, 4012, 4013, 4014, 4015, 4016, 4017, 4018, 4019, 4020
};

// IDA: Rejection pool for gate type 2 at 0x4A73E4 (SCRB 4200–4207)
const int16 ZoombiniPuzzleTunnels::kWrongPool1[8] = {
	4200, 4201, 4202, 4203, 4204, 4205, 4206, 4207
};

// IDA: Correct hint pool (medium) for hoverData 3,4 at 0x4A73F8 (SCRB 4208–4215)
const int16 ZoombiniPuzzleTunnels::kWrongPool2[8] = {
	4208, 4209, 4210, 4211, 4212, 4213, 4214, 4215
};

// IDA: Rejection pool for gate type 3 at 0x4A740C (SCRB 4404,4403,4413–4416,4400)
const int16 ZoombiniPuzzleTunnels::kWrongPool3[7] = {
	4404, 4403, 4413, 4414, 4415, 4416, 4400
};

// IDA: Correct hint pool (large) at 0x4A7420 (SCRB 4417,4405–4407,4401,4418)
const int16 ZoombiniPuzzleTunnels::kCorrectHintLarge[6] = {
	4417, 4405, 4406, 4407, 4401, 4418
};

// IDA: Rejection pool for gate type 1 at 0x4A7430 (SCRB 4600–4603)
const int16 ZoombiniPuzzleTunnels::kRejectPoolGate1[4] = {
	4600, 4601, 4602, 4603
};

// IDA: Correct hint pool for hoverData 1,6 at 0x4A743C (SCRB 4604–4609)
const int16 ZoombiniPuzzleTunnels::kWrongPool4[6] = {
	4604, 4605, 4606, 4607, 4608, 4609
};

// =========================================================================
// Constructor / Destructor
// =========================================================================

ZoombiniPuzzleTunnels::ZoombiniPuzzleTunnels(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kTunnels) {
}

ZoombiniPuzzleTunnels::~ZoombiniPuzzleTunnels() {
}

// =========================================================================
// Page Lifecycle
// =========================================================================

void ZoombiniPuzzleTunnels::open() {
	openArchive(ZMB_MHK_TUNNELS);
}

void ZoombiniPuzzleTunnels::setBackgroundMusic() {
	// IDA: puzzleTunnels_459DCB has no music playback on page load.
}

void ZoombiniPuzzleTunnels::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(300)
	_vm->_gfx->setPalette(300);
	_vm->_gfx->drawBackground(300);
}

void ZoombiniPuzzleTunnels::loadFeatures() {
	// IDA: puzzleTunnels_459DCB

	initPuzzleState();

	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// IDA: node_loadNodeAndPath(1000)
	loadNODE(ZmbArchiveKind::kPage, 1000);

	// IDA: rmap_loadTerrainArchive(100)
	loadTerrainBitmap(100);

	// Preload shape images
	_vm->_gfx->preloadImage(400);
	_vm->_gfx->preloadImage(4000);
	_vm->_gfx->preloadImage(4200);
	_vm->_gfx->preloadImage(4400);
	_vm->_gfx->preloadImage(4600);
	_vm->_gfx->preloadImage(5000);
	_vm->_gfx->preloadImage(6000);
	_vm->_gfx->preloadImage(7000);
	_vm->_gfx->preloadImage(9000);

	// Main feature head
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 12, 6000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 12; i++)
			parent = loadSubFeature(parent, ZmbResource(ZmbArchiveKind::kPage, 6000), 6000 + i);
	}

	// IDA: scrb_loadSubFeatureSet(0, 5, 7000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 5; i++)
			parent = loadSubFeature(parent, ZmbResource(ZmbArchiveKind::kPage, 7000), 7000 + i);
	}

	// IDA: scrb_loadSubFeatureSet(0, 7, 9000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 7; i++)
			parent = loadSubFeature(parent, ZmbResource(ZmbArchiveKind::kPage, 9000), 9000 + i);
	}

	// IDA: scrb_loadSubFeatureSet(2, 39, 4000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 39; i++)
			parent = loadSubFeature(parent, ZmbResource(ZmbArchiveKind::kPage, 4000), 4000 + i);
	}

	// IDA: scrb_loadSubFeatureSet(2, 27, 4200)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 27; i++)
			parent = loadSubFeature(parent, ZmbResource(ZmbArchiveKind::kPage, 4200), 4200 + i);
	}

	// IDA: scrb_loadSubFeatureSet(2, 24, 4400)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 24; i++)
			parent = loadSubFeature(parent, ZmbResource(ZmbArchiveKind::kPage, 4400), 4400 + i);
	}

	// IDA: scrb_loadSubFeatureSet(2, 18, 4600)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 18; i++)
			parent = loadSubFeature(parent, ZmbResource(ZmbArchiveKind::kPage, 4600), 4600 + i);
	}

	// Load reject pool: 8 snoids at SCRS 8000
	for (uint16 i = 0; i < 8; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 400), 8000 + i,
		          ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 65 snoids at SCRS 8500
	for (uint16 i = 0; i < 65; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 400), 8500 + i,
		          ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Feedback animation runner (SCRB 9000)
	_feedbackFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 9000), 9000, 0,
		ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// 4 tunnel entrance runners at predefined positions
	for (int16 i = 0; i < 4; i++) {
		_tunnelEntryFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 5000), 5000 + i, 6,
			kTunnelEntryPositions[i],
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER);
	}

	// Path effect runner (SCRB 7001)
	_pathEffectFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7001, 6,
		ZmbFeature::FLAG_08000000_REGION_TRACK | ZmbFeature::FLAG_04000000_OVERLAY |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00080000_DEFER_ANIM);

	// Door animation runners
	for (int16 i = 0; i < 4; i++) {
		int16 doorIdx = kDoorIndices[i];
		_doorAnimFeatures[doorIdx] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 6000), 6000 + doorIdx, 6,
			ZmbFeature::FLAG_08000000_REGION_TRACK | ZmbFeature::FLAG_04000000_OVERLAY |
			ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00080000_DEFER_ANIM);
	}

	// Anonymous visual feedback runners (SCRB 9001-9006)
	for (uint16 i = 0; i < 6; i++) {
		loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 9000), 9001 + i, 6,
			ZmbFeature::FLAG_00000000_TYPE_SHAPES);
	}

	// Main path runner (SCRB 7000) — topmost overlay
	_mainPathFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7000, 6,
		ZmbFeature::FLAG_08000000_REGION_TRACK | ZmbFeature::FLAG_04000000_OVERLAY |
		ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00001000_TOPMOST);

	// Load Zoombinis at 16 pedestal positions
	loadZoombinisFromPack();

	// Layout and stagger walk-in
	layoutStaticAndWalkIn();
	assignStaggeredWalkDelays();

	// Buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(400);
	loadHelpButtonFeature();

	// Read difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagTunnels);

	// IDA: sound_activeHandle = nextRand(20069, 20070)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, _vm->_rnd->getRandomNumber(20069, 20070));

	// Generate tunnel rules
	generateRules();

	// Disable Go button until at least one Zoombini has entered
	setGoButtonsEnabled(false);

	_puzzleActive = true;
}

// =========================================================================
// Zoombini Loading
// =========================================================================

void ZoombiniPuzzleTunnels::loadZoombinisFromPack() {
	ZmbStateFile &f = _vm->_state->_f;
	uint16 posIdx = 0;

	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount && posIdx < 16; i++) {
		ZmbStateActiveEntry &entry = f._zmbPackActive._entries[i];
		if (!entry._bIsOccupied)
			continue;

		Common::Point pos = kSnoidPositions[posIdx];
		uint16 snoidId = 10000 + posIdx;

		ZmbSnoid *snoid = loadSnoidFromPack(snoidId, pos, ZmbFeature::FLAG_00000001_TYPE_SNOID);
		if (snoid) {
			snoid->_trait = entry._traits;
			snoid->_name = entry.getU32Name(_vm);
			snoid->_packIsOccupied = true;
			snoid->setupIdleHotspots();
		}
		posIdx++;
	}

	_totalZmbCount = posIdx;
	_remainingCount = posIdx;
}

// =========================================================================
// initPuzzleState
// IDA: tunnels_initPuzzleState @ 0x459C5C
// =========================================================================

void ZoombiniPuzzleTunnels::initPuzzleState() {
	_puzzleActive = false;
	_processingFrame = false;
	_enteredCount = 0;
	_remainingCount = 0;
	_totalZmbCount = 0;
	_postGameStarted = false;
	_goButtonReady = false;
	_animLocked = false;
	_setupPhase = 0;

	// Reset rule system
	_guardCount = 0;
	for (int i = 0; i < 2; i++) {
		_guards[i] = TunnelGuard();
	}

	// Reset per-gate state
	for (int gate = 0; gate < 4; gate++) {
		_gateOccupancy[gate] = 0;
		for (int slot = 0; slot < 16; slot++)
			_gateSlots[gate][slot] = 0;
	}
	for (int z = 0; z < 5; z++)
		_gateCorrectStreak[z] = 0;

	_wrongCountZone0 = 0;
	_wrongCountZone2 = 0;

	// Random seed for level-0 gate bias
	_level0GateBias = _vm->_rnd->getRandomNumber(0, 1);

	// Reset animation queue
	_animQueueCount = 0;
	for (int i = 0; i < 5; i++)
		_animQueue[i] = AnimQueueEntry();

	_pendingSoundRunner = 0;
	_pendingSoundScrbId = 0;
	_pendingSoundHasCallback = false;
	_pendingBodyArrangement = 0;

	// Reset ambient animation
	_idleAnimDeadline = 0;
	_celebrationTarget = 0;
	_celebrationsPlayed = 0;
	_celebrationTimer = 0;
	_celebrationInterval = _vm->_rnd->getRandomNumber(1000, 2000);
	_countdownVoiceId = 0;
	_countdownVoicePlaying = false;
	_zmbEnteredVoiceId = 0;
	_activeSoundResId = 0;

	// Reset pool state bitmasks
	_poolStateWrongZone0 = 0;
	_poolStateCorrectSmall = 0;
	_poolStateWrongZone1 = 0;
	_poolStateWrongZone2 = 0;
	_poolStateWrongZone3 = 0;
	_poolStateCorrectLarge = 0;
	_poolStateRejectGate1 = 0;
	_poolStateWrongZone4 = 0;
	_poolStateWrongZone4b = 0;
	_poolStateIdleRunners = 0;
	_poolStateInitRunners = 0;
	_poolStateEndGameRunners = 0;
	_poolStateAdvanceA = 0;
	_poolStateAdvanceB = 0;
	_poolStateAdvanceGo = 0;
	_poolStateCelebration = 0;

	for (int i = 0; i < 16; i++)
		_sortedRunnerIds[i] = 0;

	// Features
	_feedbackFeature = nullptr;
	for (int i = 0; i < 4; i++) {
		_tunnelEntryFeatures[i] = nullptr;
		_doorAnimFeatures[i] = nullptr;
	}
	_pathEffectFeature = nullptr;
	_mainPathFeature = nullptr;
}

// =========================================================================
// Rule Generation
// =========================================================================

void ZoombiniPuzzleTunnels::generateRules() {
	switch (_difficultyLevel) {
	case 0:  setupLevel0_singleAttr(); break;
	case 1:  setupLevel1_dualSingleAttr(); break;
	case 2:  setupLevel2_dualDoubleAttr(); break;
	case 3:  setupLevel3_crossCategoryAttr(); break;
	default: setupLevel0_singleAttr(); break;
	}
}

// ---------------------------------------------------------------------------
// setupLevel0_singleAttr
// IDA: tunnels_setupLevel1_singleAttr @ 0x45C859
// ---------------------------------------------------------------------------
void ZoombiniPuzzleTunnels::setupLevel0_singleAttr() {
	Common::Array<ZmbTrait> traits;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *snoid = *it;
		if (snoid && snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			traits.push_back(snoid->_trait);
	}

	if (traits.empty()) {
		_guardCount = 1;
		_guards[0].sideFlag = false;
		_guards[0].condCount = 1;
		_guards[0].attrType[0] = 1;
		_guards[0].attrValue[0] = 1;
		return;
	}

	// Build 20-element match count table: 4 categories x 5 values
	int16 matchCounts[20] = {};
	for (const ZmbTrait &trait : traits) {
		uint8 hairVal = trait._head & 0x0F;
		if (hairVal >= 1 && hairVal <= 5)
			matchCounts[hairVal - 1]++;
		uint8 eyeVal = trait._eye & 0x0F;
		if (eyeVal >= 1 && eyeVal <= 5)
			matchCounts[5 + (eyeVal - 1)]++;
		uint8 noseVal = trait._nose & 0x0F;
		if (noseVal >= 1 && noseVal <= 5)
			matchCounts[10 + (noseVal - 1)]++;
		uint8 footVal = trait._foot & 0x0F;
		if (footVal >= 1 && footVal <= 5)
			matchCounts[15 + (footVal - 1)]++;
	}

	// Spiral search from target (50%) outward
	int16 targetCount = traits.size() / 2;
	Common::Array<int16> candidates;
	int16 step = 0;
	int16 checkVal = targetCount;

	for (int iter = 0; iter < 32 && candidates.empty(); iter++) {
		if (checkVal >= 1 && checkVal < 16) {
			for (int slot = 0; slot < 20; slot++) {
				if (matchCounts[slot] == checkVal)
					candidates.push_back(slot);
			}
		}
		step++;
		checkVal += (step & 1) ? step : -step;
	}

	if (candidates.empty()) {
		for (int slot = 0; slot < 20; slot++) {
			if (matchCounts[slot] > 0) {
				candidates.push_back(slot);
				break;
			}
		}
	}

	int16 bestSlot = 0;
	if (!candidates.empty())
		bestSlot = candidates[_vm->_rnd->getRandomNumber(0, candidates.size() - 1)];

	uint8 attrType = (bestSlot / 5) + 1;
	uint8 attrValue = (bestSlot % 5) + 1;

	_guardCount = 1;
	_guards[0].sideFlag = _vm->_rnd->getRandomNumber(0, 1) != 0;
	_guards[0].condCount = 1;
	_guards[0].attrType[0] = attrType;
	_guards[0].attrValue[0] = attrValue;

	debug(3, "Tunnels Level 0 Rule: Guard 0 side=%d, type=%d, value=%d",
	      _guards[0].sideFlag ? 1 : 0, attrType, attrValue);
}

// ---------------------------------------------------------------------------
// setupLevel1_dualSingleAttr
// IDA: tunnels_setupLevel2_dualSingleAttr @ 0x45CB51
// ---------------------------------------------------------------------------
void ZoombiniPuzzleTunnels::setupLevel1_dualSingleAttr() {
	Common::Array<ZmbTrait> traits;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *snoid = *it;
		if (snoid && snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			traits.push_back(snoid->_trait);
	}

	if (traits.empty()) {
		_guardCount = 2;
		_guards[0].sideFlag = false;
		_guards[0].condCount = 1;
		_guards[0].attrType[0] = 1;
		_guards[0].attrValue[0] = 1;
		_guards[1].sideFlag = true;
		_guards[1].condCount = 1;
		_guards[1].attrType[0] = 2;
		_guards[1].attrValue[0] = 1;
		return;
	}

	// Build match count table
	int16 matchCounts[20] = {};
	for (const ZmbTrait &trait : traits) {
		uint8 vals[4] = {
			static_cast<uint8>(trait._head & 0x0F),
			static_cast<uint8>(trait._eye & 0x0F),
			static_cast<uint8>(trait._nose & 0x0F),
			static_cast<uint8>(trait._foot & 0x0F)
		};
		for (int cat = 0; cat < 4; cat++) {
			if (vals[cat] >= 1 && vals[cat] <= 5)
				matchCounts[cat * 5 + (vals[cat] - 1)]++;
		}
	}

	// Find two rules from different categories with good match counts
	Common::Array<int16> goodSlots;
	uint totalCount = traits.size();
	for (int slot = 0; slot < 20; slot++) {
		if (matchCounts[slot] > 0 && static_cast<uint>(matchCounts[slot]) <= totalCount)
			goodSlots.push_back(slot);
	}

	int16 slot0 = 0, slot1 = 5;
	if (goodSlots.size() >= 2) {
		slot0 = goodSlots[_vm->_rnd->getRandomNumber(0, goodSlots.size() - 1)];
		int16 cat0 = slot0 / 5;

		Common::Array<int16> diffCatSlots;
		for (int16 s : goodSlots) {
			if ((s / 5) != cat0)
				diffCatSlots.push_back(s);
		}

		if (!diffCatSlots.empty()) {
			slot1 = diffCatSlots[_vm->_rnd->getRandomNumber(0, diffCatSlots.size() - 1)];
		} else {
			for (int16 s : goodSlots) {
				if (s != slot0) { slot1 = s; break; }
			}
		}
	}

	_guardCount = 2;
	_guards[0].sideFlag = _vm->_rnd->getRandomNumber(0, 1) != 0;
	_guards[0].condCount = 1;
	_guards[0].attrType[0] = (slot0 / 5) + 1;
	_guards[0].attrValue[0] = (slot0 % 5) + 1;
	_guards[1].sideFlag = _vm->_rnd->getRandomNumber(0, 1) != 0;
	_guards[1].condCount = 1;
	_guards[1].attrType[0] = (slot1 / 5) + 1;
	_guards[1].attrValue[0] = (slot1 % 5) + 1;

	debug(3, "Tunnels Level 1: Guard 0 type=%d val=%d, Guard 1 type=%d val=%d",
	      _guards[0].attrType[0], _guards[0].attrValue[0],
	      _guards[1].attrType[0], _guards[1].attrValue[0]);
}

// ---------------------------------------------------------------------------
// setupLevel2_dualDoubleAttr
// IDA: tunnels_setupLevel3_dualDoubleAttr @ 0x45CCCD
// ---------------------------------------------------------------------------
void ZoombiniPuzzleTunnels::setupLevel2_dualDoubleAttr() {
	// IDA: tunnels_attrDescriptors10 @ 0x4A76E4
	static const uint32 kAttrDescriptors10[10] = {
		0x12, 0x13, 0x14, 0x15, 0x23, 0x24, 0x25, 0x34, 0x35, 0x45
	};

	Common::Array<ZmbTrait> traits;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *snoid = *it;
		if (snoid && snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			traits.push_back(snoid->_trait);
	}

	if (traits.empty()) {
		setupLevel1_dualSingleAttr();
		return;
	}

	// Build 40 descriptors: 10 per category
	uint32 descriptors[40];
	int descIdx = 0;
	for (int cat = 0; cat < 4; cat++) {
		for (int i = 0; i < 10; i++)
			descriptors[descIdx++] = kAttrDescriptors10[i] << (cat * 8);
	}

	// Bucket arrays for pair scoring
	int16 bothMatch[1600] = {};
	int16 aOnlyMatch[1600] = {};
	int16 bOnlyMatch[1600] = {};
	int16 neitherMatch[1600] = {};

	for (const ZmbTrait &trait : traits) {
		uint32 traitPacked = (trait._foot & 0x0F) |
		                     ((trait._nose & 0x0F) << 8) |
		                     ((trait._eye & 0x0F) << 16) |
		                     ((trait._head & 0x0F) << 24);

		for (int pairIdx = 0; pairIdx < 1600; pairIdx++) {
			int descA = pairIdx / 40;
			int descB = pairIdx % 40;
			if (descA == descB)
				continue;

			auto matchDesc = [&](uint32 desc) -> bool {
				for (int b = 0; b < 4; b++) {
					uint8 descByte = (desc >> (b * 8)) & 0xFF;
					uint8 traitByte = (traitPacked >> (b * 8)) & 0x0F;
					if (descByte != 0) {
						uint8 val0 = descByte & 0x0F;
						uint8 val1 = (descByte >> 4) & 0x0F;
						return (traitByte == val0 || traitByte == val1);
					}
				}
				return false;
			};

			bool mA = matchDesc(descriptors[descA]);
			bool mB = matchDesc(descriptors[descB]);

			if (mA && mB)        bothMatch[pairIdx]++;
			else if (mA)         aOnlyMatch[pairIdx]++;
			else if (mB)         bOnlyMatch[pairIdx]++;
			else                 neitherMatch[pairIdx]++;
		}
	}

	// Find best diversity
	int16 bestDiversity = 0;
	for (int p = 0; p < 1600; p++) {
		if ((p / 40) == (p % 40)) continue;
		int16 diversity = (bothMatch[p] > 0 ? 1 : 0) + (aOnlyMatch[p] > 0 ? 1 : 0) +
		                  (bOnlyMatch[p] > 0 ? 1 : 0) + (neitherMatch[p] > 0 ? 1 : 0);
		if (diversity > bestDiversity) bestDiversity = diversity;
	}

	// Calculate balance scores
	int16 balanceScore[1600];
	for (int p = 0; p < 1600; p++) {
		balanceScore[p] = -1;
		if ((p / 40) == (p % 40)) continue;
		int16 diversity = (bothMatch[p] > 0 ? 1 : 0) + (aOnlyMatch[p] > 0 ? 1 : 0) +
		                  (bOnlyMatch[p] > 0 ? 1 : 0) + (neitherMatch[p] > 0 ? 1 : 0);
		if (diversity < bestDiversity) continue;

		int16 a = bothMatch[p], b = aOnlyMatch[p], c = bOnlyMatch[p], d = neitherMatch[p];
		balanceScore[p] = ABS(a - b) + ABS(a - c) + ABS(a - d) +
		                  ABS(b - c) + ABS(b - d) + ABS(c - d);
	}

	int16 minBalance = 32000;
	for (int p = 0; p < 1600; p++) {
		if (balanceScore[p] >= 0 && balanceScore[p] < minBalance)
			minBalance = balanceScore[p];
	}

	int16 candidateCount = 0;
	for (int p = 0; p < 1600; p++) {
		if (balanceScore[p] == minBalance) candidateCount++;
	}

	int16 selection = _vm->_rnd->getRandomNumber(1, candidateCount);
	int16 selectedPair = 0;
	for (int p = 0; p < 1600; p++) {
		if (balanceScore[p] == minBalance) {
			if (--selection == 0) { selectedPair = p; break; }
		}
	}

	uint32 descA = descriptors[selectedPair / 40];
	uint32 descB = descriptors[selectedPair % 40];

	_guardCount = 2;
	for (int g = 0; g < 2; g++) {
		uint32 desc = (g == 0) ? descA : descB;
		_guards[g].sideFlag = _vm->_rnd->getRandomNumber(0, 1) != 0;
		_guards[g].condCount = 2;
		for (int b = 0; b < 4; b++) {
			uint8 descByte = (desc >> (b * 8)) & 0xFF;
			if (descByte != 0) {
				_guards[g].attrType[0] = 4 - b;
				_guards[g].attrValue[0] = descByte & 0x0F;
				_guards[g].attrType[1] = 4 - b;
				_guards[g].attrValue[1] = (descByte >> 4) & 0x0F;
				break;
			}
		}
	}

	debug(3, "Tunnels Level 2: Guard 0 type=%d vals=%d/%d, Guard 1 type=%d vals=%d/%d",
	      _guards[0].attrType[0], _guards[0].attrValue[0], _guards[0].attrValue[1],
	      _guards[1].attrType[0], _guards[1].attrValue[0], _guards[1].attrValue[1]);
}

// ---------------------------------------------------------------------------
// setupLevel3_crossCategoryAttr
// IDA: tunnels_setupLevel4_crossCategoryAttr @ 0x45D608
// ---------------------------------------------------------------------------
void ZoombiniPuzzleTunnels::setupLevel3_crossCategoryAttr() {
	static const uint32 kAttrBaseTable6[6] = {
		0x00000001, 0x00000001, 0x00000001,
		0x00000100, 0x00000100,
		0x00010000
	};
	static const uint32 kAttrStepTable6[6] = {
		0x00000100, 0x00010000, 0x01000000,
		0x00010000, 0x01000000,
		0x01000000
	};

	Common::Array<ZmbTrait> traits;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *snoid = *it;
		if (snoid && snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			traits.push_back(snoid->_trait);
	}

	if (traits.empty()) {
		setupLevel1_dualSingleAttr();
		return;
	}

	// Build 150 cross-category descriptors
	uint32 descriptors[150];
	int descIdx = 0;
	for (int pair = 0; pair < 6; pair++) {
		for (int v1 = 1; v1 <= 5; v1++) {
			for (int v2 = 1; v2 <= 5; v2++)
				descriptors[descIdx++] = (kAttrBaseTable6[pair] * v1) + (kAttrStepTable6[pair] * v2);
		}
	}

	// Bucket arrays (heap-allocated for 22500 entries)
	Common::Array<int16> bothMatch(22500, 0);
	Common::Array<int16> aOnlyMatch(22500, 0);
	Common::Array<int16> bOnlyMatch(22500, 0);
	Common::Array<int16> neitherMatch(22500, 0);

	for (const ZmbTrait &trait : traits) {
		uint32 traitPacked = (trait._foot & 0x0F) |
		                     ((trait._nose & 0x0F) << 8) |
		                     ((trait._eye & 0x0F) << 16) |
		                     ((trait._head & 0x0F) << 24);

		for (int pairIdx = 0; pairIdx < 22500; pairIdx++) {
			int descAIdx = pairIdx / 150;
			int descBIdx = pairIdx % 150;
			if (descAIdx == descBIdx) continue;

			auto matchDesc = [&](uint32 desc) -> bool {
				for (int b = 0; b < 4; b++) {
					uint8 descVal = (desc >> (b * 8)) & 0x0F;
					uint8 traitVal = (traitPacked >> (b * 8)) & 0x0F;
					if (descVal != 0 && traitVal == descVal)
						return true;
				}
				return false;
			};

			bool mA = matchDesc(descriptors[descAIdx]);
			bool mB = matchDesc(descriptors[descBIdx]);

			if (mA && mB)        bothMatch[pairIdx]++;
			else if (mA)         aOnlyMatch[pairIdx]++;
			else if (mB)         bOnlyMatch[pairIdx]++;
			else                 neitherMatch[pairIdx]++;
		}
	}

	int16 bestDiversity = 0;
	for (int p = 0; p < 22500; p++) {
		if ((p / 150) == (p % 150)) continue;
		int16 diversity = (bothMatch[p] > 0 ? 1 : 0) + (aOnlyMatch[p] > 0 ? 1 : 0) +
		                  (bOnlyMatch[p] > 0 ? 1 : 0) + (neitherMatch[p] > 0 ? 1 : 0);
		if (diversity > bestDiversity) bestDiversity = diversity;
	}

	Common::Array<int16> balanceScore(22500, -1);
	for (int p = 0; p < 22500; p++) {
		if ((p / 150) == (p % 150)) continue;
		int16 diversity = (bothMatch[p] > 0 ? 1 : 0) + (aOnlyMatch[p] > 0 ? 1 : 0) +
		                  (bOnlyMatch[p] > 0 ? 1 : 0) + (neitherMatch[p] > 0 ? 1 : 0);
		if (diversity < bestDiversity) continue;

		int16 a = bothMatch[p], b = aOnlyMatch[p], c = bOnlyMatch[p], d = neitherMatch[p];
		balanceScore[p] = ABS(a - b) + ABS(a - c) + ABS(a - d) +
		                  ABS(b - c) + ABS(b - d) + ABS(c - d);
	}

	int16 minBalance = 32000;
	for (int p = 0; p < 22500; p++) {
		if (balanceScore[p] >= 0 && balanceScore[p] < minBalance)
			minBalance = balanceScore[p];
	}

	int16 candidateCount = 0;
	for (int p = 0; p < 22500; p++) {
		if (balanceScore[p] == minBalance) candidateCount++;
	}

	int16 sel = _vm->_rnd->getRandomNumber(1, candidateCount);
	int16 selectedPair = 0;
	for (int p = 0; p < 22500; p++) {
		if (balanceScore[p] == minBalance) {
			if (--sel == 0) { selectedPair = p; break; }
		}
	}

	uint32 descA = descriptors[selectedPair / 150];
	uint32 descB = descriptors[selectedPair % 150];

	_guardCount = 2;
	for (int g = 0; g < 2; g++) {
		uint32 desc = (g == 0) ? descA : descB;
		_guards[g].sideFlag = _vm->_rnd->getRandomNumber(0, 1) != 0;
		_guards[g].condCount = 2;
		int condIdx = 0;
		for (int b = 0; b < 4 && condIdx < 2; b++) {
			uint8 descVal = (desc >> (b * 8)) & 0x0F;
			if (descVal != 0) {
				_guards[g].attrType[condIdx] = 4 - b;
				_guards[g].attrValue[condIdx] = descVal;
				condIdx++;
			}
		}
	}

	debug(3, "Tunnels Level 3: Guard 0 types=%d/%d vals=%d/%d, Guard 1 types=%d/%d vals=%d/%d",
	      _guards[0].attrType[0], _guards[0].attrType[1], _guards[0].attrValue[0], _guards[0].attrValue[1],
	      _guards[1].attrType[0], _guards[1].attrType[1], _guards[1].attrValue[0], _guards[1].attrValue[1]);
}

// =========================================================================
// evaluateRule
// IDA: tunnels_evalAttrRule @ 0x45C65D
//
// Returns true if the rule is NOT satisfied (= rejection).
// guardAMatch: output for which side guard A matched (zone-dependent).
// =========================================================================

bool ZoombiniPuzzleTunnels::evaluateRule(ZmbSnoid *snoid, int16 dropZone, bool &guardAMatch) {
	guardAMatch = false;
	if (!snoid || dropZone < 1 || dropZone > 4)
		return true;

	// Trait values indexed by attribute type (1=hair, 2=eyes, 3=nose, 4=feet)
	uint8 traitVals[5] = {0};
	traitVals[1] = snoid->_trait._head & 0x0F;
	traitVals[2] = snoid->_trait._eye & 0x0F;
	traitVals[3] = snoid->_trait._nose & 0x0F;
	traitVals[4] = snoid->_trait._foot & 0x0F;

	// Evaluate a single guard's conditions
	auto evaluateGuard = [&](const TunnelGuard &guard) -> bool {
		if (guard.condCount == 0)
			return false;
		if (guard.condCount == 1) {
			uint8 type = guard.attrType[0];
			uint8 value = guard.attrValue[0];
			return (type >= 1 && type <= 4 && traitVals[type] == value);
		}
		// 2 conditions
		uint8 type0 = guard.attrType[0], value0 = guard.attrValue[0];
		uint8 type1 = guard.attrType[1], value1 = guard.attrValue[1];
		bool match0 = (type0 >= 1 && type0 <= 4 && traitVals[type0] == value0);
		bool match1 = (type1 >= 1 && type1 <= 4 && traitVals[type1] == value1);
		if (type0 == type1)
			return match0 || match1;  // Same category: OR (level 2)
		else
			return match0 && match1;  // Cross-category: AND (level 3)
	};

	// Evaluate guard A (v12 in IDA)
	bool v12 = false;
	if (_guardCount >= 1 && _guards[0].condCount >= 1) {
		v12 = evaluateGuard(_guards[0]);
		if (!_guards[0].sideFlag)
			v12 = !v12;
	}

	// Single-guard mode (level 0)
	if (_guardCount == 1) {
		guardAMatch = v12;
		// Zone 1: match guard A → accepted
		// Zone 2: NOT match guard A → accepted
		bool result;
		if (dropZone == 1)
			result = v12;
		else
			result = !v12;
		return !result;  // Return true = rejection (rule NOT satisfied)
	}

	// Evaluate guard B (v11 in IDA)
	bool v11 = false;
	if (_guardCount >= 2 && _guards[1].condCount >= 1) {
		v11 = evaluateGuard(_guards[1]);
		if (!_guards[1].sideFlag)
			v11 = !v11;
	}

	// Zone logic per IDA 0x45C65D
	bool result;
	switch (dropZone) {
	case 1:
		guardAMatch = v12;
		result = (_guardCount >= 2) ? (v12 && v11) : v12;
		break;
	case 2:
		guardAMatch = v12;
		result = v12 && !v11;
		break;
	case 3:
		guardAMatch = !v12;
		result = !v12 && !v11;
		break;
	case 4:
		guardAMatch = !v12;
		result = !v12 && v11;
		break;
	default:
		result = false;
		break;
	}

	return !result;  // true = rejection
}

// =========================================================================
// getDropZone
// =========================================================================

int16 ZoombiniPuzzleTunnels::getDropZone(const Common::Point &pos) {
	for (int16 i = 0; i < 4; i++) {
		int16 dx = pos.x - kTunnelEntryPositions[i].x;
		int16 dy = pos.y - kTunnelEntryPositions[i].y;
		int32 distSq = dx * dx + dy * dy;
		if (distSq <= kClickZoneRadius * kClickZoneRadius)
			return i + 1;
	}
	return 0;
}

// =========================================================================
// handleZoombiniPlacement
// IDA: tunnels_funcOnHover case 4 @ 0x45A9CF
// =========================================================================

void ZoombiniPuzzleTunnels::handleZoombiniPlacement(ZmbSnoid *snoid, int16 zone,
                                                          bool isRejection, bool guardAMatch, bool wasInSlot) {
	if (!snoid || zone < 1 || zone > 4)
		return;

	// Compute hoverData from zone + guardAMatch (IDA: 0x45ACB0)
	int16 hoverData = 0;
	switch (zone) {
	case 1: hoverData = guardAMatch ? 1 : 0; break;
	case 2: hoverData = guardAMatch ? 3 : 2; break;
	case 3: hoverData = guardAMatch ? 4 : 5; break;
	case 4: hoverData = guardAMatch ? 6 : 7; break;
	}

	// Level 0 bias (IDA: 0x45AD14)
	if (!isRejection && _difficultyLevel == 0) {
		if (_level0GateBias) {
			if (zone == 1 || zone == 4)
				isRejection = true;
		} else {
			if (zone == 2 || zone == 3)
				isRejection = true;
		}
	}

	int16 gateType = kHoverDataToGateType[hoverData];
	int16 variant = snoid->getVariant();

	// Hint animation (set regardless of rejection — IDA: 0x45AD5C)
	int16 hintRunner = 0;      // secondaryRunner: door feature index to load hint onto
	int16 hintScrb = 0;        // secondaryScrb1: hint SCRB ID
	if (guardAMatch) {
		if (hoverData >= 4) {
			hintRunner = 3;  // _doorAnimFeatures[3]
			hintScrb = kCorrectHintLarge[_vm->_rnd->getNonRepeatRandom(6, _poolStateCorrectLarge)];
		} else {
			hintRunner = 0;  // _doorAnimFeatures[0]
			hintScrb = kCorrectHintSmall[_vm->_rnd->getNonRepeatRandom(11, _poolStateCorrectSmall)];
		}
	}

	int16 walkScrsId = 0;      // walking/approach SCRS
	int16 rejectScrsId = 0;    // snoid SCRS for gate interaction
	int16 primaryRunner = gateType;  // door feature index
	int16 primaryScrb = 0;     // door animation SCRB
	int16 secondaryScrb = 0;   // secondary feedback SCRB
	int16 secondaryRunner = hintRunner;
	int16 secondaryScrb1 = hintScrb;
	int16 secondaryScrb2 = 0;

	if (isRejection) {
		// === REJECTION PATH === (IDA: 0x45ADC6)
		_gateCorrectStreak[zone] = 0;// Reset streak for this zone

		primaryScrb = hoverData + 6004;  // Door rejection animation SCRB

		// Select from gate-specific rejection pool
		switch (gateType) {
		case 0:
			_wrongCountZone0++;
			do {
				secondaryScrb = kWrongPool0[_vm->_rnd->getNonRepeatRandom(10, _poolStateWrongZone0)];
			} while (secondaryScrb == 4005 && _wrongCountZone0 < 3);
			break;
		case 1:
			secondaryScrb = kRejectPoolGate1[_vm->_rnd->getNonRepeatRandom(4, _poolStateRejectGate1)];
			break;
		case 2:
			secondaryScrb = kWrongPool1[_vm->_rnd->getNonRepeatRandom(8, _poolStateWrongZone1)];
			break;
		case 3:
			_wrongCountZone2++;
			do {
				secondaryScrb = kWrongPool3[_vm->_rnd->getNonRepeatRandom(7, _poolStateWrongZone3)];
			} while (secondaryScrb == 4416 && _wrongCountZone2 < 3);
			break;
		}

		walkScrsId = variant + 5 * hoverData + 8519;
		rejectScrsId = hoverData + 8000;
	} else {
		// === SUCCESS PATH === (IDA: 0x45AED8)
		_wrongCountZone0 = 0;
		_wrongCountZone2 = 0;
		_gateCorrectStreak[zone]++;

		// Select correct feedback SCRB
		if (hoverData == 1 || hoverData == 6) {
			primaryScrb = kWrongPool4[_vm->_rnd->getNonRepeatRandom(6, _poolStateWrongZone4b)];
		} else if (hoverData == 3 || hoverData == 4) {
			primaryScrb = kWrongPool2[_vm->_rnd->getNonRepeatRandom(8, _poolStateWrongZone2)];
		}

		walkScrsId = hoverData / 2 + 4 * variant + 8496;
		rejectScrsId = 0;

		// If this would be the last zoombini, clear hint fields
		if (_enteredCount + 1 >= _totalZmbCount) {
			secondaryScrb2 = 0;
			secondaryScrb1 = 0;
			secondaryRunner = 0;
		}
	}

	// Build animation queue entry (IDA: 0x45AF84)
	AnimQueueEntry entry;
	entry.runnerIdx = snoid->getId();
	entry.isRejection = isRejection ? 1 : 0;
	entry.stepCounter = 0;
	entry.pos = snoid->getPointLoc();
	entry.walkScrsId = walkScrsId;
	entry.rejectScrsId = rejectScrsId;
	entry.primaryRunner = primaryRunner;
	entry.primaryScrb = primaryScrb;
	entry.secondaryScrb = secondaryScrb;
	entry.secondaryRunner = secondaryRunner;
	entry.secondaryScrb1 = secondaryScrb1;
	entry.secondaryScrb2 = secondaryScrb2;
	entry.zoneIdx = zone;

	appendAnimQueueEntry(entry);

	// If dragged from a slot, reassign idle positions (IDA: 0x45B031)
	if (wasInSlot) {
		Common::Point origPos = _dragOrigPos;
		Common::Point curPos = snoid->getPointLoc();
		if (origPos.x != curPos.x || origPos.y != curPos.y) {
			// The zoombini was moved; let idle position assignment handle reassignment
		}
	}

	debug(3, "Tunnels: Placed snoid %d at zone %d, rejection=%d, hoverData=%d, gateType=%d",
	      snoid->getId(), zone, isRejection ? 1 : 0, hoverData, gateType);
}

// =========================================================================
// Animation Queue Management
// =========================================================================

void ZoombiniPuzzleTunnels::appendAnimQueueEntry(const AnimQueueEntry &entry) {
	if (_animQueueCount >= 5)
		return;
	_animQueue[_animQueueCount] = entry;
	_animQueueCount++;
}

void ZoombiniPuzzleTunnels::popAnimQueueEntry() {
	if (_animQueueCount <= 0)
		return;

	// IDA: tunnels_removeRunnerFromList (0x45BE7D) — shift entries down by 14-word stride
	for (int i = 0; i < _animQueueCount - 1; i++)
		_animQueue[i] = _animQueue[i + 1];
	_animQueueCount--;
	_animQueue[_animQueueCount] = AnimQueueEntry();
}

// =========================================================================
// advanceAnimStep
// IDA: tunnels_advanceAnimSequenceStep @ 0x45BF8D
// =========================================================================

void ZoombiniPuzzleTunnels::advanceAnimStep() {
	if (_animQueueCount <= 0)
		return;

	AnimQueueEntry &head = _animQueue[0];
	head.stepCounter++;

	int16 runner = 0;
	int16 scrb = 0;

	switch (head.stepCounter) {
	case 1:
		runner = head.primaryRunner;
		scrb = head.primaryScrb;
		break;
	case 2:
		runner = head.secondaryRunner;
		scrb = head.secondaryScrb1;
		if (!scrb) {
			head.stepCounter = 3;
			// Fall through to step 3
			runner = head.primaryRunner;
			scrb = head.secondaryScrb;
			if (!scrb) {
				head.stepCounter = 4;
				runner = head.secondaryRunner;
				scrb = head.secondaryScrb2;
			}
		}
		break;
	case 3:
		runner = head.primaryRunner;
		scrb = head.secondaryScrb;
		if (!scrb) {
			head.stepCounter = 4;
			runner = head.secondaryRunner;
			scrb = head.secondaryScrb2;
		}
		break;
	case 4:
		runner = head.secondaryRunner;
		scrb = head.secondaryScrb2;
		break;
	default:
		// Beyond step 4: pop entry
		popAnimQueueEntry();
		_animLocked = false;
		return;
	}

	if (scrb > 0 && runner >= 0 && runner < 4 && _doorAnimFeatures[runner]) {
		// IDA: scrb_initRunnerWithScript(1, tunnels_advanceRunnerState, scrb, runner)
		loadScrbOntoFeature(_doorAnimFeatures[runner], scrb);
		_animLocked = true;
	} else {
		// No valid runner/scrb: pop queue entry
		// IDA: callIfNonZero_45BF72
		popAnimQueueEntry();
		_animLocked = false;
	}
}

// =========================================================================
// selectLevelRunners
// IDA: tunnels_selectLevelRunners @ 0x45C05E
// =========================================================================

void ZoombiniPuzzleTunnels::selectLevelRunners(int16 mode) {
	int16 primaryRunner = 0;    // door feature index (v3 in IDA)
	int16 primaryScrb = 0;     // SCRB ID (v1 in IDA)
	int16 secondaryRunner = 0;  // secondary door feature index (v2 in IDA)
	int16 secondaryScrb1 = 0;  // (v22 in IDA)
	int16 secondaryScrb = 0;   // (setupBuf in IDA)
	int16 secondaryScrb2 = 0;  // (v21 in IDA)

	switch (mode) {
	case 0: {
		// Idle animations: 10-pool at dword_4A76C8
		uint16 idx = _vm->_rnd->getNonRepeatRandom(10, _poolStateIdleRunners);
		switch (idx) {
		case 0: primaryRunner = 1; primaryScrb = 4610; secondaryRunner = 2; secondaryScrb1 = 4216; break;
		case 1: primaryRunner = 2; primaryScrb = 4217; break;
		case 2: primaryRunner = 2; primaryScrb = 4218; break;
		case 3: primaryRunner = 2; primaryScrb = 4219; break;
		case 4: primaryRunner = 2; primaryScrb = 4220; break;
		case 5: primaryRunner = 0; primaryScrb = 4021; secondaryRunner = 3; secondaryScrb1 = 4408; break;
		case 6: primaryRunner = 0; primaryScrb = 4022; secondaryRunner = 3; secondaryScrb1 = 4408; break;
		case 7: primaryRunner = 3; primaryScrb = 4402; secondaryRunner = 0; secondaryScrb1 = 4023; secondaryScrb2 = 4029; break;
		case 8: primaryRunner = 3; primaryScrb = 4402; secondaryRunner = 0; secondaryScrb1 = 4023; secondaryScrb2 = 4030; break;
		case 9: primaryRunner = 3; primaryScrb = 4419; break;
		}
		break;
	}
	case 1: {
		// Init animations
		uint16 visitCount = _vm->_state->_f._pageFlagTunnels & 0xFFF;
		if (visitCount == 1) {
			// First visit: 4-pool at dword_4A76CC
			uint16 idx = _vm->_rnd->getNonRepeatRandom(4, _poolStateInitRunners);
			switch (idx) {
			case 0: primaryRunner = 2; primaryScrb = 4221; break;
			case 1: primaryRunner = 0; primaryScrb = 4024; secondaryScrb = 4025; break;
			case 2: primaryRunner = 1; primaryScrb = 4614; break;
			case 3: primaryRunner = 3; primaryScrb = 4409; break;
			}
		} else {
			// Subsequent visits: 8-pool
			uint16 idx = _vm->_rnd->getNonRepeatRandom(8, _poolStateInitRunners);
			switch (idx) {
			case 0: primaryRunner = 1; primaryScrb = 4611; secondaryRunner = 0; secondaryScrb1 = 4026; break;
			case 1: primaryRunner = 1; primaryScrb = 4611; secondaryRunner = 2; secondaryScrb1 = 4224; break;
			case 2: primaryRunner = 3; primaryScrb = 4411; secondaryRunner = 1; secondaryScrb1 = 4612; break;
			case 3: primaryRunner = 1; primaryScrb = 4613; secondaryRunner = 2; secondaryScrb1 = 4223; break;
			case 4: primaryRunner = 1; primaryScrb = 4613; secondaryRunner = 2; secondaryScrb1 = 4222; break;
			case 5: primaryRunner = 3; primaryScrb = 4410; secondaryRunner = 2; secondaryScrb1 = 4222; break;
			case 6: primaryRunner = 0; primaryScrb = 4027; secondaryRunner = 3; secondaryScrb1 = 4412; break;
			case 7: primaryRunner = 0; primaryScrb = 4028; secondaryRunner = 2; secondaryScrb1 = 4223; break;
			}
		}
		break;
	}
	case 2: {
		// End-game: 3-pool at dword_4A76D4
		uint16 idx = _vm->_rnd->getNonRepeatRandom(3, _poolStateEndGameRunners);
		switch (idx) {
		case 0: primaryRunner = 3; primaryScrb = 4420; break;
		case 1: primaryRunner = 3; primaryScrb = 4421; break;
		case 2: primaryRunner = 3; primaryScrb = 4422; break;
		}
		break;
	}
	case 3: {
		// Advance: depends on goButtonReady and zmb count
		if (_goButtonReady) {
			// 7-pool at dword_4A76E0
			uint16 idx = _vm->_rnd->getNonRepeatRandom(7, _poolStateAdvanceGo);
			switch (idx) {
			case 0: primaryRunner = 0; primaryScrb = 4035; break;
			case 1: primaryRunner = 3; primaryScrb = 4423; break;
			case 2: primaryRunner = 0; primaryScrb = 4034; break;
			case 3: primaryRunner = 0; primaryScrb = 4036; break;
			case 4: primaryRunner = 0; primaryScrb = 4037; break;
			case 5: primaryRunner = 0; primaryScrb = 4032; break;
			case 6: primaryRunner = 0; primaryScrb = 4033; break;
			}
		} else {
			// Count idle snoids on pedestals
			int16 idleCount = 0;
			for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
				ZmbSnoid *s = *it;
				if (s && s->getId() >= 10000 && s->getAnimState() == kSnoidAnimIdle)
					idleCount++;
			}

			if (idleCount == _totalZmbCount) {
				// All at pedestals: 8-pool at dword_4A76D8
				uint16 idx = _vm->_rnd->getNonRepeatRandom(8, _poolStateAdvanceA);
				switch (idx) {
				case 0: primaryRunner = 0; primaryScrb = 4031; break;
				case 1: primaryRunner = 1; primaryScrb = 4617; break;
				case 2: primaryRunner = 0; primaryScrb = 4038; break;
				case 3: primaryRunner = 0; primaryScrb = 4034; break;
				case 4: primaryRunner = 0; primaryScrb = 4036; break;
				case 5: primaryRunner = 0; primaryScrb = 4037; break;
				case 6: primaryRunner = 0; primaryScrb = 4032; break;
				case 7: primaryRunner = 0; primaryScrb = 4033; break;
				}
			} else {
				// Partial: 9-pool at dword_4A76DC
				uint16 idx = _vm->_rnd->getNonRepeatRandom(9, _poolStateAdvanceB);
				switch (idx) {
				case 0: primaryRunner = 2; primaryScrb = 4225; break;
				case 1: primaryRunner = 2; primaryScrb = 4226; break;
				case 2: primaryRunner = 1; primaryScrb = 4615; break;
				case 3: primaryRunner = 1; primaryScrb = 4616; break;
				case 4: primaryRunner = 0; primaryScrb = 4034; break;
				case 5: primaryRunner = 0; primaryScrb = 4036; break;
				case 6: primaryRunner = 0; primaryScrb = 4037; break;
				case 7: primaryRunner = 0; primaryScrb = 4032; break;
				case 8: primaryRunner = 0; primaryScrb = 4033; break;
				}
			}
		}
		break;
	}
	}

	// Build animation queue entry
	AnimQueueEntry entry;
	entry.primaryRunner = primaryRunner;
	entry.primaryScrb = primaryScrb;
	entry.secondaryScrb = secondaryScrb;
	entry.secondaryRunner = secondaryRunner;
	entry.secondaryScrb1 = secondaryScrb1;
	entry.secondaryScrb2 = secondaryScrb2;
	appendAnimQueueEntry(entry);
}

// =========================================================================
// assignSlotWithPush
// IDA: tunnels_assignSlotWithPush @ 0x45BA3D
// =========================================================================

int16 ZoombiniPuzzleTunnels::assignSlotWithPush(int16 side) {
	// Collect idle positions and find nearest runners per slot
	// IDA: snoid_collectIdlePositions(500, 0, &dword_4A76C2)
	for (int16 i = 0; i < 16; i++) {
		// Find nearest idle snoid to each pedestal position
		uint16 bestId = 0;
		int32 bestDist = 0x7FFFFFFF;

		for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
			ZmbSnoid *s = *it;
			if (!s || s->getId() < 10000 || s->getAnimState() != kSnoidAnimIdle)
				continue;

			Common::Point sPos = s->getPointLoc();
			int32 dx = sPos.x - kSnoidPositions[i].x;
			int32 dy = sPos.y - kSnoidPositions[i].y;
			int32 dist = dx * dx + dy * dy;

			// Check not already assigned to a previous slot
			bool alreadyUsed = false;
			for (int16 j = 0; j < i; j++) {
				if (_sortedRunnerIds[j] == s->getId()) {
					alreadyUsed = true;
					break;
				}
			}
			if (alreadyUsed)
				continue;

			if (dist < bestDist) {
				bestDist = dist;
				bestId = s->getId();
			}
		}

		_sortedRunnerIds[i] = bestId;
	}

	// Check preferred slots first
	int16 result = -1;
	for (int16 k = 0; k < 2 && result == -1; k++) {
		int16 prefSlot = kPreferredSlots[side][k];
		if (prefSlot >= 0 && prefSlot < 16 && _sortedRunnerIds[prefSlot] == 0)
			result = prefSlot;
	}

	if (result != -1)
		return result;

	// Cascading push: find empty slot and shift occupants
	for (int16 v10 = 0; v10 < 15; v10++) {
		int16 slotIdx = v10;
		// IDA: if slot >= 11 && !side → mirror slot
		if (slotIdx >= 11 && side == 0)
			slotIdx = 26 - slotIdx;

		if (_sortedRunnerIds[slotIdx] != 0)
			continue;

		// Determine push offsets (cascading chain)
		int16 pushOff1 = 0, pushOff2 = 0, pushOff3 = 0;

		if (slotIdx == 0) {
			pushOff1 = 6;
		} else if (slotIdx >= 1 && slotIdx <= 4) {
			pushOff1 = 5;
			pushOff2 = 6;
		} else if (slotIdx == 5 || slotIdx == 6) {
			pushOff1 = 5;
		} else if (slotIdx >= 7 && slotIdx <= 10) {
			pushOff1 = 4;
			pushOff2 = 5;
		} else if (slotIdx >= 11 && slotIdx <= 15) {
			if (side) {
				pushOff1 = (slotIdx < 15) ? 1 : 0;
				if (slotIdx < 14) pushOff2 = 2;
				if (slotIdx < 13) pushOff3 = 3;
			} else {
				if (slotIdx > 11) pushOff1 = -1;
				if (slotIdx > 12) pushOff2 = -2;
				if (slotIdx > 13) pushOff3 = -3;
			}
		}

		// Try cascading push
		bool pushed = false;
		int16 offsets[3] = { pushOff1, pushOff2, pushOff3 };
		for (int step = 0; step < 3 && !pushed; step++) {
			int16 off = offsets[step];
			if (off == 0)
				break;

			int16 neighborSlot = slotIdx + off;
			if (neighborSlot < 0 || neighborSlot >= 16)
				continue;

			if (_sortedRunnerIds[neighborSlot] != 0) {
				// Push the occupant from neighborSlot to slotIdx
				ZmbSnoid *occupant = getSnoid(_sortedRunnerIds[neighborSlot]);
				if (occupant) {
					occupant->setAnimTargetPos(kSnoidPositions[slotIdx]);
					occupant->setAnimState(kSnoidAnimDepart);
					_sortedRunnerIds[slotIdx] = _sortedRunnerIds[neighborSlot];
					_sortedRunnerIds[neighborSlot] = 0;
					pushed = true;
				}
			}

			// Cascade: try next offset
			offsets[0] = offsets[1];
			offsets[1] = offsets[2];
			offsets[2] = 0;
		}

		// After push attempts, don't break — continue looking
	}

	// Re-check preferred slots after push
	result = -1;
	for (int16 k = 0; k < 2 && result == -1; k++) {
		int16 prefSlot = kPreferredSlots[side][k];
		if (prefSlot >= 0 && prefSlot < 16 && _sortedRunnerIds[prefSlot] == 0)
			result = prefSlot;
	}

	// Fallback: find any empty slot
	if (result == -1) {
		for (int16 j = 0; j < 16; j++) {
			if (_sortedRunnerIds[j] == 0) {
				result = j;
				break;
			}
		}
	}

	return (result != -1) ? result : 0;
}

// =========================================================================
// spawnPendingZoombinis
// IDA: tunnels_spawnPendingZmbs @ 0x45B3E5
// =========================================================================

int16 ZoombiniPuzzleTunnels::spawnPendingZoombinis() {
	int16 spawned = 0;

	for (int16 i = 0; i < 4 && _animQueueCount > 0; i++) {
		AnimQueueEntry &head = _animQueue[0];
		ZmbSnoid *snoid = getSnoid(head.runnerIdx);
		if (snoid) {
			// Position at spawn origin (off-screen bottom)
			int16 spawnX = kSpawnOriginX[head.zoneIdx > 0 ? head.zoneIdx - 1 : 0];
			snoid->setPointLoc(Common::Point(spawnX, 460));
			snoid->_packIsOccupied = false;
			snoid->removeFlag(ZmbFeature::FLAG_04000000_OVERLAY);
			snoid->setAnimState(kSnoidAnimDepart);
			spawned++;
		}
		popAnimQueueEntry();
	}

	return spawned;
}

// =========================================================================
// playAmbientSound
// IDA: tunnels_playAmbientSound @ 0x45B4BF
//
// Called as a SCRB frame callback on _mainPathFeature.
// On soundIdx==-1 (end): set goButtonReady, maybe play ambient SND 20045-20048.
// =========================================================================

void ZoombiniPuzzleTunnels::playAmbientSound() {
	_goButtonReady = true;

	// Probability check: rand(0,4) > difficultyLevel OR puzzleFlag <= 3
	bool shouldPlay = (_vm->_rnd->getRandomNumber(0, 4) > _difficultyLevel) ||
	                  ((_vm->_state->_f._pageFlagTunnels & 0xFFF) <= 3);

	if (shouldPlay && _enteredCount < _totalZmbCount && _enteredCount > 0) {
		_vm->_sound->playZmbSound(
			ZmbResource(ZmbArchiveKind::kSystem, _vm->_rnd->getRandomNumber(20045, 20048)));
	}
}

// =========================================================================
// clearGateRenderFlag
// IDA: tunnels_clearGateRenderFlag @ 0x45DD11
// =========================================================================

void ZoombiniPuzzleTunnels::clearGateRenderFlag() {
	// Clear OVERLAY flag on appropriate door features for gate types 1 and 4
	if (_animQueueCount > 0) {
		int16 gateType = kHoverDataToGateType[_animQueue[0].zoneIdx > 0 ? _animQueue[0].zoneIdx - 1 : 0];
		if (gateType == 0 || gateType == 3) {
			// Gate types 1 and 4 (original indexing) need overlay cleared
			for (int i = 0; i < 4; i++) {
				if (_doorAnimFeatures[i] && _doorAnimFeatures[i]->isRenderActivated()) {
					_doorAnimFeatures[i]->removeFlag(ZmbFeature::FLAG_04000000_OVERLAY);
					break;
				}
			}
		}
	}
}

// =========================================================================
// findIdlePackSnoid
// =========================================================================

ZmbSnoid *ZoombiniPuzzleTunnels::findIdlePackSnoid(uint16 snoidId) {
	ZmbSnoid *snoid = getSnoid(snoidId);
	if (snoid && snoid->getAnimState() == kSnoidAnimIdle)
		return snoid;

	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *s = *it;
		if (s && s->getId() >= 10000 && s->getAnimState() == kSnoidAnimIdle)
			return s;
	}
	return nullptr;
}

// =========================================================================
// processSnoidAnimEvent
// IDA: tunnels_scrbAnimCallback @ 0x45B56C
// =========================================================================

void ZoombiniPuzzleTunnels::processSnoidAnimEvent(ZmbSnoid *snoid, int16 eventCode) {
	if (!snoid)
		return;

	if (eventCode > 13) {
		// Events 240-243: pending body arrangement
		if (eventCode >= kZmbAnimEvent240_BodyArrangePendFirst && eventCode <= kZmbAnimEvent243_BodyArrangePendLast) {
			_pendingBodyArrangement = eventCode - (kZmbAnimEvent240_BodyArrangePendFirst - 1);
		}
		// Events 250-253: direct body arrangement
		else if (eventCode >= kZmbAnimEvent250_BodyArrangeDirectFirst && eventCode <= kZmbAnimEvent253_BodyArrangeDirectLast) {
			snoid->setBodyArrangement(eventCode - kZmbAnimEvent250_BodyArrangeDirectFirst);
		}
		return;
	}

	if (eventCode == 13) {
		// === Exit sequence ===
		// IDA: load door SCRB via scrb_initRunnerWithScript, copy sortRect, unlock, stop sound
		AnimQueueEntry &head = _animQueue[0];
		if (head.primaryScrb > 0 && head.primaryRunner >= 0 && head.primaryRunner < 4) {
			ZmbFeature *doorFeature = _doorAnimFeatures[head.primaryRunner];
			if (doorFeature) {
				loadScrbOntoFeature(doorFeature, head.primaryScrb);
				doorFeature->setSortRect(snoid->getSortRect());
			}
		}
		_animLocked = false;

		// Stop active sound
		if (_activeSoundResId > 0) {
			_vm->_sound->stopZmbSound(ZmbResource(ZmbArchiveKind::kPage, _activeSoundResId));
			_activeSoundResId = 0;
		}
		return;
	}

	if (eventCode == kZmbAnimEventM1_End) {
		// === Animation end (event -1) ===
		// IDA: 0x45B702
		_pendingSoundRunner = 0;
		_pendingSoundHasCallback = true;

		if (!_animQueue[0].isRejection && _animQueueCount > 0) {
			// === SUCCESS: gate arrival ===
			AnimQueueEntry &head = _animQueue[0];

			// Pending sound runner for streak feedback
			if (head.primaryScrb > 0 && head.runnerIdx == snoid->getId()) {
				if (_gateCorrectStreak[head.zoneIdx] < 2) {
					_pendingSoundRunner = head.primaryRunner;
					_pendingSoundScrbId = head.primaryScrb;
				}
			}

			int16 gateType = kHoverDataToGateType[head.zoneIdx > 0 ? head.zoneIdx - 1 : 0];
			if (head.zoneIdx > 0) {
				// Set OVERLAY + LOOP flags on snoid
				snoid->addFlag(static_cast<ZmbFeature::Flag>(ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM));

				// Store snoid in gate slot and set target position
				int16 &occupancy = _gateOccupancy[gateType];

				// Link relative to previous snoid in gate (for Z-ordering)
				// IDA: runner_linkRelativeToParent
				if (occupancy > 0 && occupancy < 16) {
					// Previous snoid in this gate for Z-linking (handled by dirty-rect rendering)
				}

				if (occupancy < 16) {
					_gateSlots[gateType][occupancy] = snoid->getId();
					snoid->setAnimTargetPos(kGatePositions[gateType][occupancy]);
					occupancy++;
				}

				// Set snoid flags and animate arrival
				snoid->_packIsOccupied = true;
				snoid->setAnimState(kSnoidAnimArrivalMotion);

				// Celebration milestones
				_enteredCount++;
				if (_enteredCount == _totalZmbCount) {
					_celebrationTarget += 2;
					_zmbEnteredVoiceId = _vm->_rnd->getRandomNumber(20055, 20063);
				} else if (_enteredCount == 10) {
					_celebrationTarget++;
				} else if (_enteredCount == 12) {
					_celebrationTarget++;
				} else if (_enteredCount == 14) {
					_celebrationTarget += 2;
				}
			}

			// Set initial entered count on first entry
			if (!_remainingCount)
				_remainingCount = _totalZmbCount;
		} else if (_animQueue[0].isRejection && _animQueueCount > 0) {
			// === REJECTION: return to staging area ===
			AnimQueueEntry &head = _animQueue[0];

			// Play countdown voice if queued
			if (!_countdownVoicePlaying && _countdownVoiceId > 0) {
				_countdownVoicePlaying = true;
				_vm->_sound->playZmbSound(
					ZmbResource(ZmbArchiveKind::kPage, _countdownVoiceId),
					Audio::Mixer::kSpeechSoundType);

				// Play rejection face SCRS
				if (_pathEffectFeature) {
					int16 faceScrs = 7001 + _vm->_rnd->getRandomNumber(0, 3);
					loadScrbOntoFeature(_pathEffectFeature, faceScrs);
				}
			}

			// Determine side for slot assignment
			int16 side;
			int16 gateType = kHoverDataToGateType[head.zoneIdx > 0 ? head.zoneIdx - 1 : 0];
			if (gateType == 0 || gateType == 1)
				side = 1;
			else
				side = 0;

			// Assign staging slot and walk back
			int16 slotResult = assignSlotWithPush(side);
			snoid->removeFlag(ZmbFeature::FLAG_04000000_OVERLAY);
			snoid->setAnimTargetPos(kSnoidPositions[slotResult]);
			snoid->setAnimState(kSnoidAnimDepart);

			clearGateRenderFlag();
		}

		// Pop queue entry if no pending sound
		if (_pendingSoundRunner == 0) {
			popAnimQueueEntry();
			_animLocked = false;
		}
		return;
	}

	if (eventCode == 0) {
		// === Toggle render + pending body arrangement ===
		if (snoid->isRenderActivated())
			snoid->deactivateRender();
		else
			snoid->activateRender();

		if (_pendingBodyArrangement > 0) {
			snoid->setBodyArrangement(_pendingBodyArrangement - 1);
			_pendingBodyArrangement = 0;
		}
		return;
	}

	if (eventCode == 10) {
		// === SCRS replay at gate entrance ===
		// IDA: variant = (word_4B7A52 - 8000) / 2 % 4
		if (_animQueueCount > 0) {
			int16 scrsId = _animQueue[0].rejectScrsId;
			if (scrsId > 0) {
				int16 variantIdx = ((scrsId - 8000) / 2) & 3;
				clearGateRenderFlag();
				snoid->setPointLoc(kScrsReplayPositions[variantIdx]);

				Common::SeekableReadStream *scrsStream =
					_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
					                 ZmbResource(ZmbArchiveKind::kPage, scrsId));
				if (scrsStream) {
					bool hideOnComplete = !_animQueue[0].isRejection;
					snoid->startScrsPlayback(scrsStream, hideOnComplete, _animQueue[0].isRejection != 0);
				}
			}
		}
		return;
	}
}

// =========================================================================
// processGateAnimEvent
// =========================================================================

void ZoombiniPuzzleTunnels::processGateAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (eventCode == kZmbAnimEventM1_End) {
		// Gate animation complete
		_animLocked = false;
		if (_setupPhase == 1)
			_setupPhase = 2;
	}
}

// =========================================================================
// onFeatureAnimEvent
// =========================================================================

void ZoombiniPuzzleTunnels::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
		processSnoidAnimEvent(static_cast<ZmbSnoid *>(feature), eventCode);
	} else {
		processGateAnimEvent(feature, eventCode);
	}
}

// =========================================================================
// onEveryFrame
// IDA: puzzleTunnels_onHover @ 0x45A460
// =========================================================================

void ZoombiniPuzzleTunnels::onEveryFrame() {
	if (_processingFrame || !_puzzleActive)
		return;
	_processingFrame = true;

	// Check pending departure
	if (_pendingGoDepart) {
		_processingFrame = false;
		return;
	}

	// Check sound status (IDA: wActiveSndResId_4B7B3E)
	bool isGateSoundPlaying = false;
	bool isAmbientSoundPlaying = false;
	if (_activeSoundResId >= 4000 && _activeSoundResId <= 4699) {
		if (_vm->_system->getMixer()->isSoundHandleActive(_activeSndHandle))
			isGateSoundPlaying = true;
	} else if (_activeSoundResId >= 7000 && _activeSoundResId <= 7099) {
		if (_vm->_system->getMixer()->isSoundHandleActive(_activeSndHandle))
			isAmbientSoundPlaying = true;
	}

	// Countdown voice management (IDA: word_4B7AE4, word_4B7AE2)
	if (_countdownVoiceId > 0) {
		debugC(2, kZmbDebugAnimation, "Tunnels: countdown voice id=%d playing=%d", _countdownVoiceId, _countdownVoicePlaying ? 1 : 0);
		if (_countdownVoicePlaying) {
			// Check if finished
			if (!_vm->_system->getMixer()->isSoundHandleActive(_activeSndHandle)) {
				_countdownVoiceId = 0;
				_countdownVoicePlaying = false;
			}
		}
	}

	// Pending sound runner (IDA: word_4B7A28, word_4B7A2A, word_4B7A2C)
	if (_pendingSoundRunner > 0) {
		debugC(2, kZmbDebugAnimation, "Tunnels: pending sound runner=%d scrbId=%d gateSnd=%d ambientSnd=%d",
			_pendingSoundRunner, _pendingSoundScrbId, isGateSoundPlaying ? 1 : 0, isAmbientSoundPlaying ? 1 : 0);
		if (!isGateSoundPlaying && !isAmbientSoundPlaying) {
			int16 runner = _pendingSoundRunner;
			_pendingSoundRunner = 0;

			if (_zmbEnteredVoiceId > 0) {
				// Play entered voice and advance
				if (_pendingSoundHasCallback) {
					// IDA: tunnels_onAnimSeqComplete(-1, 0)
					popAnimQueueEntry();
					_animLocked = false;
				}
			} else {
				// Load SCRB onto runner
				if (runner >= 0 && runner < 4 && _doorAnimFeatures[runner] && _pendingSoundScrbId > 0) {
					if (_pendingSoundHasCallback) {
						loadScrbOntoFeature(_doorAnimFeatures[runner], _pendingSoundScrbId);
					} else {
						loadScrbOntoFeature(_doorAnimFeatures[runner], _pendingSoundScrbId);
					}
				}
			}
		}
	} else if (_zmbEnteredVoiceId > 0 && !isGateSoundPlaying && !isAmbientSoundPlaying) {
		// Play entered voice SFX
		_vm->_sound->playZmbSound(
			ZmbResource(ZmbArchiveKind::kSystem, _zmbEnteredVoiceId),
			Audio::Mixer::kSpeechSoundType);
		_zmbEnteredVoiceId = 0;
	}

	// Animation queue processing
	if (_remainingCount > 0) {
		debugC(2, kZmbDebugAnimation, "Tunnels: animQueue=%d locked=%d remaining=%d",
			_animQueueCount, _animLocked ? 1 : 0, _remainingCount);
		if (_countdownVoicePlaying || !_animQueueCount || _animLocked)
			goto postAnimQueue;

		if (_animQueue[0].runnerIdx > 0) {
			// Active entry with runner
			if (_animQueue[0].stepCounter == 0 && !isGateSoundPlaying && !isAmbientSoundPlaying) {
				ZmbSnoid *snoid = getSnoid(_animQueue[0].runnerIdx);
				if (snoid) {
					_animQueue[0].stepCounter = 1;

					// Start secondary animation (hint) if available
					if (_animQueue[0].secondaryScrb1 > 0) {
						if (_gateCorrectStreak[_animQueue[0].zoneIdx] < 2 &&
						    _animQueue[0].secondaryRunner >= 0 && _animQueue[0].secondaryRunner < 4) {
							loadScrbOntoFeature(_doorAnimFeatures[_animQueue[0].secondaryRunner],
							                    _animQueue[0].secondaryScrb1);
						}
					}

					// Clear overlay on snoid
					snoid->removeFlag(ZmbFeature::FLAG_04000000_OVERLAY);
					if (snoid->getAnimState() < kSnoidAnimArrive)
						snoid->activateRender();

					// Start SCRS playback
					int16 scrsId = _animQueue[0].walkScrsId;
					if (scrsId > 0) {
						Common::SeekableReadStream *scrsStream =
							_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
							                 ZmbResource(ZmbArchiveKind::kPage, scrsId));
						if (scrsStream) {
							snoid->startScrsPlayback(scrsStream,
							                         !_animQueue[0].isRejection,
							                         _animQueue[0].isRejection != 0);
						}
					}

					// Handle rejection countdown
					if (_animQueue[0].isRejection) {
						_remainingCount--;
						switch (_remainingCount) {
						case 1: _countdownVoiceId = 4703; break;
						case 2: _countdownVoiceId = 4702; break;
						case 3: _countdownVoiceId = 4701; break;
						case 4: _countdownVoiceId = 4700; break;
						}
}

					_animLocked = true;
				}
			}
			goto postAnimQueue;
		}

		// No runner in head entry — advance animation step
		advanceAnimStep();
	} else {
		// All zoombinis placed — post-game phase
		if (!_postGameStarted && !_animLocked) {
			debugC(1, kZmbDebugAnimation, "Tunnels: all placed, starting post-game");
			if (!isGateSoundPlaying) {
				_postGameStarted = true;
				spawnPendingZoombinis();
				selectLevelRunners(2);
				_setupPhase = 1;
			}
			goto postAnimQueue;
		}

		// Process remaining queue entries after post-game
		if (_animQueueCount > 0 && !_animLocked && !_animQueue[0].runnerIdx && !isGateSoundPlaying) {
			advanceAnimStep();
		}
	}

postAnimQueue:
	// Idle animation scheduling (IDA: dword_4B7A34)
	if (!_postGameStarted && getCurrentFrameCounter() > _idleAnimDeadline) {
		selectLevelRunners(0);
		_idleAnimDeadline = getCurrentFrameCounter() + _vm->_rnd->getRandomNumber(5400, 10800);
	}

	// Setup phase: load SCRB onto mainPathFeature (IDA: word_4B7A42)
	if (_setupPhase == 2) {
		debugC(1, kZmbDebugAnimation, "Tunnels: setup phase 2 -> 3, loading SCRB 7000");
		_setupPhase = 3;
		if (_mainPathFeature) {
			loadScrbOntoFeature(_mainPathFeature, 7000);
			_mainPathFeature->removeFlag(ZmbFeature::FLAG_01000000_DEFER_RENDER);
			// Animation complete callback will call playAmbientSound
		}
		setGoButtonsEnabled(true);
	}

	// Celebration fidget system (IDA: word_4B7AEE < word_4B7AEC)
	if (_celebrationsPlayed < _celebrationTarget &&
	    getCurrentFrameCounter() - _celebrationTimer > _celebrationInterval) {

		_celebrationTimer = getCurrentFrameCounter();
		bool triggered = false;
		int16 attempts = 0;

		while (!triggered && attempts < 16) {
			attempts++;
			uint16 poolIdx = _vm->_rnd->getNonRepeatRandom(_totalZmbCount, _poolStateCelebration);
			uint16 snoidId = 10000 + poolIdx;

			ZmbSnoid *snoid = findIdlePackSnoid(snoidId);
			if (snoid && snoid->_packIsOccupied && snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
				// Play celebration SCRS (8559 + variant)
				Common::SeekableReadStream *scrsStream =
					_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
					                 ZmbResource(ZmbArchiveKind::kPage, 8559 + snoid->getVariant()));
				if (scrsStream) {
					snoid->startScrsPlayback(scrsStream, false, true);
					_celebrationsPlayed++;
					triggered = true;
				}
			}
		}
	}

	_processingFrame = false;
}

// =========================================================================
// Button Handlers
// =========================================================================

void ZoombiniPuzzleTunnels::onGoButtonActivated() {
	// IDA: tunnels_onClickHandler case 2
	_departXferSrcSiPage = ZMB_SI_TUNNELS_03;
	startDepartWalkAnimation(Common::Point(670, 30));

	// Stop active sound
	if (_activeSoundResId > 0) {
		_vm->_sound->stopZmbSound(ZmbResource(ZmbArchiveKind::kPage, _activeSoundResId));
		_activeSoundResId = 0;
	}

	selectLevelRunners(3);

	ZoombiniInteractive::onGoButtonActivated();
}

ZmbEventHandleResult ZoombiniPuzzleTunnels::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// Sticky mouse: second click ends drag
	if (isDragging() && _vm->_state->getEnableStickyMouse()) {
		endDrag(absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Check if clicking on a Zoombini to start drag
	if (!_postGameStarted && !_animLocked) {
		ZmbSnoid *snoid = findSnoidAtPoint(absPos);
		if (snoid && snoid->getAnimState() == kSnoidAnimIdle) {
			// IDA: clear stale queue head if no active animation
			if (_animQueueCount > 0 && !_animQueue[0].runnerIdx && !_animLocked) {
				_animQueue[0] = AnimQueueEntry();
				_animQueueCount = 0;
				_animLocked = false;
			}

			startSnoidDrag(snoid, absPos);
			return ZmbEventHandleResult::kConsumed;
		}
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniPuzzleTunnels::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	if (!isDragging())
		return ZoombiniInteractive::onLButtonUp(absPos, relPos);

	if (_vm->_state->getEnableStickyMouse())
		return ZmbEventHandleResult::kConsumed;

	endDrag(absPos);
	return ZmbEventHandleResult::kConsumed;
}

// =========================================================================
// endDrag
// =========================================================================

void ZoombiniPuzzleTunnels::endDrag(const Common::Point &dropPos) {
	ZmbSnoid *snoid = finishSnoidDrag();
	if (!snoid)
		return;

	int16 zone = getDropZone(dropPos);

	// For level 0, only zones 1 and 2 are valid
	if (_difficultyLevel == 0 && zone > 2)
		zone = 0;

	if (zone > 0) {
		bool guardAMatch = false;
		bool isRejection = evaluateRule(snoid, zone, guardAMatch);

		// Check if snoid was dragged from animation queue (wasInSlot)
		bool wasInSlot = false;
		// IDA: tunnels_removeRunnerFromList checks if the snoid was in the queue
		for (int i = 0; i < _animQueueCount; i++) {
			if (_animQueue[i].runnerIdx == snoid->getId()) {
				wasInSlot = true;
				break;
			}
		}

		// Set overlay flag on snoid for gate approach
		snoid->addFlag(ZmbFeature::FLAG_04000000_OVERLAY);

		handleZoombiniPlacement(snoid, zone, isRejection, guardAMatch, wasInSlot);
		return;
	}

	// Dropped outside valid zones — return to original position
	snoid->setPointLoc(_dragOrigPos);
	snoid->setAnimState(kSnoidAnimIdle);
}

} // End of namespace Mohawk
