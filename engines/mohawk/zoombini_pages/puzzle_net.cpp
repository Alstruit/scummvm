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
#include "mohawk/zoombini_pages/puzzle_net.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions from 0x4A286C (16 POINTS)
const Common::Point ZoombiniPuzzleNet::kSnoidPositions[16] = {
	Common::Point(233, 392), Common::Point(209, 378), Common::Point(196, 390), Common::Point(185, 365),
	Common::Point(167, 380), Common::Point(160, 408), Common::Point(135, 397), Common::Point(121, 407),
	Common::Point(115, 368), Common::Point(114, 342), Common::Point( 99, 375), Common::Point( 97, 394),
	Common::Point( 95, 346), Common::Point( 91, 411), Common::Point( 79, 355), Common::Point( 62, 404),
};

// IDA: word_4A2586 — low-difficulty slot positions (5x5 = 25 entries)
const Common::Point ZoombiniPuzzleNet::kSlotPositionsLow[25] = {
	Common::Point(102, 117), Common::Point(204, 106), Common::Point(306,  94), Common::Point(409,  79), Common::Point(507,  69),
	Common::Point(102, 157), Common::Point(204, 143), Common::Point(306, 129), Common::Point(407, 115), Common::Point(507, 104),
	Common::Point(102, 195), Common::Point(204, 180), Common::Point(306, 166), Common::Point(407, 151), Common::Point(507, 140),
	Common::Point(102, 232), Common::Point(204, 217), Common::Point(306, 205), Common::Point(407, 191), Common::Point(507, 178),
	Common::Point(102, 272), Common::Point(204, 257), Common::Point(306, 245), Common::Point(407, 229), Common::Point(507, 214),
};

// IDA: word_4A25EA — high-difficulty slot positions (5x25 = 125 entries)
const Common::Point ZoombiniPuzzleNet::kSlotPositionsHigh[125] = {
	// Plane 0 (slots 0-24)
	Common::Point( 74, 121), Common::Point( 94, 119), Common::Point(114, 116), Common::Point(135, 113), Common::Point(156, 111),
	Common::Point(177, 109), Common::Point(197, 107), Common::Point(217, 104), Common::Point(237, 102), Common::Point(257,  99),
	Common::Point(278,  98), Common::Point(298,  95), Common::Point(319,  93), Common::Point(340,  89), Common::Point(360,  87),
	Common::Point(380,  84), Common::Point(398,  82), Common::Point(418,  79), Common::Point(438,  77), Common::Point(458,  74),
	Common::Point(481,  72), Common::Point(499,  70), Common::Point(519,  67), Common::Point(538,  65), Common::Point(559,  63),
	// Plane 1 (slots 25-49)
	Common::Point( 74, 160), Common::Point( 93, 158), Common::Point(113, 155), Common::Point(133, 152), Common::Point(154, 149),
	Common::Point(177, 147), Common::Point(197, 145), Common::Point(217, 143), Common::Point(237, 140), Common::Point(257, 139),
	Common::Point(279, 135), Common::Point(299, 132), Common::Point(319, 130), Common::Point(341, 127), Common::Point(362, 124),
	Common::Point(382, 121), Common::Point(400, 118), Common::Point(420, 115), Common::Point(439, 112), Common::Point(459, 110),
	Common::Point(480, 109), Common::Point(499, 107), Common::Point(518, 105), Common::Point(538, 103), Common::Point(558, 101),
	// Plane 2 (slots 50-74)
	Common::Point( 72, 200), Common::Point( 93, 198), Common::Point(113, 195), Common::Point(134, 192), Common::Point(156, 190),
	Common::Point(177, 188), Common::Point(197, 185), Common::Point(216, 181), Common::Point(236, 178), Common::Point(256, 176),
	Common::Point(279, 173), Common::Point(299, 170), Common::Point(319, 168), Common::Point(338, 164), Common::Point(358, 162),
	Common::Point(380, 158), Common::Point(399, 156), Common::Point(419, 153), Common::Point(438, 150), Common::Point(458, 148),
	Common::Point(479, 147), Common::Point(500, 144), Common::Point(519, 141), Common::Point(538, 138), Common::Point(557, 136),
	// Plane 3 (slots 75-99)
	Common::Point( 74, 239), Common::Point( 94, 236), Common::Point(114, 233), Common::Point(135, 230), Common::Point(156, 227),
	Common::Point(178, 224), Common::Point(198, 221), Common::Point(219, 218), Common::Point(238, 216), Common::Point(256, 214),
	Common::Point(278, 212), Common::Point(298, 209), Common::Point(319, 206), Common::Point(338, 203), Common::Point(359, 200),
	Common::Point(380, 196), Common::Point(399, 194), Common::Point(419, 191), Common::Point(438, 189), Common::Point(459, 186),
	Common::Point(479, 184), Common::Point(499, 181), Common::Point(518, 178), Common::Point(538, 177), Common::Point(556, 175),
	// Plane 4 (slots 100-124)
	Common::Point( 75, 278), Common::Point( 95, 276), Common::Point(115, 273), Common::Point(135, 269), Common::Point(156, 267),
	Common::Point(177, 263), Common::Point(197, 261), Common::Point(217, 258), Common::Point(237, 255), Common::Point(257, 253),
	Common::Point(280, 250), Common::Point(300, 248), Common::Point(319, 245), Common::Point(339, 241), Common::Point(359, 238),
	Common::Point(380, 235), Common::Point(400, 232), Common::Point(419, 229), Common::Point(439, 226), Common::Point(457, 223),
	Common::Point(480, 219), Common::Point(499, 216), Common::Point(519, 214), Common::Point(538, 211), Common::Point(556, 209),
};

// IDA: dword_4A27DE — exit positions (16 packed x,y pairs)
const Common::Point ZoombiniPuzzleNet::kExitPositions[16] = {
	Common::Point( 16, 58), Common::Point( 17, 45), Common::Point( 15, 33), Common::Point( 16, 19),
	Common::Point( 47, 59), Common::Point( 51, 48), Common::Point( 46, 30), Common::Point( 48, 20),
	Common::Point( 77, 63), Common::Point( 74, 47), Common::Point( 76, 32), Common::Point( 77, 18),
	Common::Point(146, 67), Common::Point(143, 58), Common::Point(141, 40), Common::Point(147, 32),
};

// IDA: dword_4A28E8 — entry start positions (snoid event 4)
const Common::Point ZoombiniPuzzleNet::kEntryStartPositions[3] = {
	Common::Point(203, 42), Common::Point(242, 35), Common::Point(283, 28),
};

// IDA: dword_4A28F4 — entry exit positions (snoid event 30)
const Common::Point ZoombiniPuzzleNet::kEntryExitPositions[3] = {
	Common::Point(220, 41), Common::Point(259, 34), Common::Point(300, 27),
};

// IDA: unk_4A28D4 — column offset remapping table 1
const int16 ZoombiniPuzzleNet::kColOffsets1[5] = {2, 3, 0, 1, 4};

// IDA: unk_4A28DE — column offset remapping table 2
const int16 ZoombiniPuzzleNet::kColOffsets2[5] = {4, 0, 2, 1, 3};

// IDA: word_4A2292 (button runner data, entries 4-19, 36-byte stride)
// Fixed click rectangles for the submit button and 3x5 attribute column buttons.
// These match the original engine's CButtonRunner table exactly.
const Common::Rect ZoombiniPuzzleNet::kButtonClickRects[16] = {
	// [0] submit button (hotspot 4)
	Common::Rect(450, 275, 587, 362),
	// [1-5] column 0 values 0-4 (hotspots 5-9, active only at diff>=2)
	Common::Rect(446, 378, 475, 407),
	Common::Rect(476, 375, 498, 402),
	Common::Rect(499, 373, 524, 399),
	Common::Rect(525, 367, 549, 394),
	Common::Rect(550, 363, 573, 390),
	// [6-10] column 1 values 0-4 (hotspots 10-14)
	Common::Rect(446, 408, 475, 433),
	Common::Rect(476, 403, 501, 428),
	Common::Rect(499, 400, 526, 424),
	Common::Rect(525, 395, 550, 421),
	Common::Rect(550, 391, 577, 415),
	// [11-15] column 2 values 0-4 (hotspots 15-19)
	Common::Rect(450, 434, 478, 459),
	Common::Rect(479, 429, 501, 455),
	Common::Rect(502, 425, 527, 451),
	Common::Rect(528, 422, 553, 446),
	Common::Rect(554, 416, 577, 441),
};

ZoombiniPuzzleNet::ZoombiniPuzzleNet(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kNet) {
}

ZoombiniPuzzleNet::~ZoombiniPuzzleNet() {
}

void ZoombiniPuzzleNet::open() {
	openArchive(ZMB_MHK_NET);
}

void ZoombiniPuzzleNet::setBackgroundMusic() {
	// IDA: net_puzzleInit (0x4361d4) has no music playback call on page load.
	// sound_activeHandle = 20064 is stored at end of funcInit for F1 replay only.
}

void ZoombiniPuzzleNet::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId((net_difficultyLevel >= 2) + 5000)
	// Background differs based on difficulty: 5000 or 5001
	// NOTE: setBackgroundBitmap() is called BEFORE loadFeatures(), so _difficultyLevel
	// is not yet initialized. Read the route level directly from state (0-based).
	int16 routeLevel = _vm->_state->readActivePageRouteLevel();
	uint16 bgId = (routeLevel >= 2) ? 5001 : 5000;
	_vm->_gfx->setPalette(bgId);
	_vm->_gfx->drawBackground(bgId);
}

void ZoombiniPuzzleNet::loadFeatures() {
	// IDA: puzzleNet_4361D4 (0x4361d4)
	_difficultyLevel = static_cast<ZmbPuzzleDifficultyLevel>(_vm->_state->readActivePageRouteLevel() + 1);

	// Initialize puzzle state
	// IDA: net_totalSlotCount = 25; if (diff > 1) net_totalSlotCount = 125;
	_totalSlotCount = (_difficultyLevel >= kPuzzleDiffLevel3) ? 125 : 25;
	_columnCount = (_difficultyLevel >= kPuzzleDiffLevel3) ? 3 : 2;
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
		attrSlotHooks.setPreRenderFunc(reinterpret_cast<ZmbFeature::OnPreRenderFunc>(&ZoombiniPuzzleNet::attrSlots_preRender));
		attrSlotHooks.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniPuzzleNet::attrSlots_render));
		loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, 0), 0, 0, ZmbFeature::FLAG_00001000_TOPMOST, attrSlotHooks);
	}

	// Load Zoombinis at 16 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, v12, 16)
	// IDA: SHPL_copyPaletteSrcToDst(236, 10)
	loadZoombinisFromPack();

	// IDA: net_computeColumnSizes() — distributes loaded zoombinis into 3,2,3,2... groups.
	// This OVERWRITES _columnCount with the number of distribution groups.
	computeColumnSizes();

	// IDA: net_remainingExitSteps = net_columnCount + (difficulty==3) + 7
	_remainingExitSteps = _columnCount + (_difficultyLevel == kPuzzleDiffLevel4 ? 1 : 0) + 7;
	// IDA: net_totalExitSteps = net_remainingExitSteps (copy for reference)
	_totalExitSteps = _remainingExitSteps;
	_exitAnimActive = true;
	_labelAnimRunning = true;
	_firstRoundFlag = true;
	_inputLocked = true;
	_exitAnimStep = 0;
	_exitScrbOffset = 16 - _remainingExitSteps;
	_sortedZmbCount = 0;
	_rejectedCount = 0;
	_submitCount = 0;
	_submitActiveFlag = 0;
	_columnMatchCount = 0;
	_columnAnimDone = 0;
	_nextZmbToAssign = 0;
	_pendingZmbIndex = -1;
	_activeWalkCount = 0;
	_activeZmbSnoidId = 0;
	_lastLinkedSnoidId = 0;
	_exitingZmbSnoidId = 0;
	_zmbsAtColumns = 0;
	_allColumnsExhausted = 0;
	_zmbQueueCount = 0;
	_zmbReadyCount = 0;
	_zmbWalkPending = false;
	_attrColumnsReady = false;
	_bAdvanceReady = false;
	_noMatchFlag = false;
	_hintPending = false;
	_slotRunnerCount = -1;
	_currentSlotIndex = -1;
	_pendingColumnSetup = 0;
	_pendingAttrRunning = false;
	_activeAttrRunning = false;
	_activeAttrAnim1Running = false;
	_activeAttrAnim2Running = false;
	_activeAttrAnim3Running = false;
	_columnOpenAnimRunning = false;
	_columnAnimColIdx = 0;
	_zmbEntryAnimRunning = false;
	_sortAnimRunning = false;
	_hotspotPositionFlag = 0;
	_bounceCounter = 0;
	_sortAnimType = 0;
	_exitPositionIdx = 0;
	_lastSubmitFrame = 0;
	_pendingBodyArrangement = 0;
	memset(_columnSlotSnoidIds, 0, sizeof(_columnSlotSnoidIds));
	memset(_walkSlotSnoidIds, 0, sizeof(_walkSlotSnoidIds));
	memset(_slotScrbFeatures, 0, sizeof(_slotScrbFeatures));
	memset(_activeSlotFeatures, 0, sizeof(_activeSlotFeatures));

	// Register column SCRB runners and start initial animations
	// IDA: net_registerAllSCRBRunners(v10, &unk_4A28AC)
	registerColumnRunners();
	// Exit SCRB animation is started during registerColumnRunners()
	// IDA: net_exitRunner = scrb_registerHotspotGroup(0, 0, 0, 0, net_exitScrbRunner, net_exitScrbRunner)
	_exitRunnerActive = true;

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

	// Idle animation state
	// IDA: net_idleAnimMax = 3; if (*(_WORD*)(g_pGameState + 32)) net_idleAnimMax = 2;
	_idleAnimTrigger = false;
	_idleAnimCount = 0;
	_idleAnimMax = 3;
	_idleAnimPoolState = 0;
	_idleAnimLastFrame = 0;
	_roundCompletedFlag = false;
}

void ZoombiniPuzzleNet::onGoButtonActivated() {
	// IDA: net_onClickHandler case 2
	// Stop BGM, play departure SFX, walk snoids to (600, -100), fade out when SFX finishes.
	// IDA: scrb_enqueueSoundResource(0, 0) — stop background music
	_vm->_sound->stopAllSoundQueues();

	_departXferSrcSiPage = ZMB_SI_NET_12;
	// IDA: zmbMoveAnimation_45479D(45, -100, 600)
	startDepartWalkAnimation(Common::Point(600, -100));
	ZoombiniInteractive::onGoButtonActivated();
}

void ZoombiniPuzzleNet::loadZoombinisFromPack() {
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

void ZoombiniPuzzleNet::registerColumnRunners() {
	// IDA: net_registerAllSCRBRunners (0x437733)
	// Registers all the SCRB features needed for column-based sorting puzzle.

	// IDA: scrb_useFeatureGroup(TRUE, groupId, 7000) — loads REGS 7000/7001
	// alongside tBMP 7000 for per-shape registration-point offsets.
	loadREGS(ZmbArchiveKind::kPage, 7000);
	// IDA: scrb_useFeatureGroup(TRUE, groupId, 9000) — loads REGS 9000/9001
	// for slot feature shapes (indicator shapes 151-156 have non-zero offsets).
	loadREGS(ZmbArchiveKind::kPage, 9000);
	
	// 5 column SCRB runners at 8000-8004
	// IDA: for i=0..4: net_columnScrbRunners[i] = registerSCRB(..., 6, i+8000, ..., 0x4180000)
	for (int16 i = 0; i < 5; i++) {
		_columnScrbFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 8000), 8000 + i, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);
	}
	
	// Entry SCRB runner at 8005
	// IDA: net_entryScrbRunner = registerSCRB(..., 6, 8005, ..., 0x4188000)
	_entryScrbFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8005, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);
	
	// Label SCRB runner: 9151 (diff<=1) or 9153 (diff>1)
	// IDA: net_labelScrbRunner = registerSCRB(..., 6, 9151/9153, ..., 0x4108000)
	uint16 labelScrbId = (_difficultyLevel >= kPuzzleDiffLevel3) ? 9153 : 9151;
	_labelScrbFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 9000), labelScrbId, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_04000000_OVERLAY);
	
	// Attribute animation SCRB runner at 7018
	// IDA: net_attrAnimScrbRunner = registerSCRB(..., 6, 7018, ..., 0x4188000)
	_attrAnimScrbFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7018, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);
	if (_attrAnimScrbFeature)
		_attrAnimScrbFeature->setShapeRegs(_regsMap[7000]);
	
	// Feedback SCRB runner at 10018
	// IDA: net_feedbackScrbRunner = registerSCRB(..., 6, 10018, ..., 0x188000)
	_feedbackScrbFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10018, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE);
	
	// Attribute column SCRB runners at random offsets
	// IDA: net_attrCol0ScrbRunner (only if diff>=2), net_attrCol1ScrbRunner, net_attrCol2ScrbRunner
	// IDA flags: 0x4108000 = OVERLAY | PLAY_ONCE | LOOP_ANIM
	if (_difficultyLevel >= kPuzzleDiffLevel3) {
		_attrColScrbFeatures[0] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 10002 + _randAttrColOffset[0], 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);
	}
	_attrColScrbFeatures[1] = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10007 + _randAttrColOffset[1], 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_04000000_OVERLAY);
	_attrColScrbFeatures[2] = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10012 + _randAttrColOffset[2], 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_04000000_OVERLAY);
	
	// Exit SCRB runner at 7000
	// IDA: net_exitScrbRunner = registerSCRB(..., 6, 7000, ..., 0x4188000)
	// IDA: net_exitRunner = scrb_registerHotspotGroup(0, 0, 0, 0, ...) — tracks completion
	// Original uses DEFER_ANIM: SCRB 7000 is loaded as a placeholder but NEVER plays.
	// On the first frame tick, hotspot_ownerRunnerArr[exitRunner]==0 (since no render occurred),
	// so the exit step immediately advances from 0→1, loading SCRB 7001 as the first actual animation.
	_exitScrbFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7000, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);
	if (_exitScrbFeature)
		_exitScrbFeature->setShapeRegs(_regsMap[7000]);
}

bool ZoombiniPuzzleNet::attrSlots_preRender(ZmbFeature *feature) {
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

ZmbRenderResult ZoombiniPuzzleNet::attrSlots_render(ZmbFeature *feature) {
	// IDA: fleens_renderAllAttrSlots_436785 (0x436785)
	// Renders the attribute slot button sprites:
	//   Slot 1: Label area (shape 5 from tBMP 6000) at (600, 403)
	//   Slot 2: Advance button (shape 1=off / 2=ready from tBMP 6000) at (600, 441)
	//
	// IDA: fleens_renderAttrSlotSCRB(0, 0, 1) — slot 1 always shape 5
	//      fleens_renderAttrSlotSCRB(0, 0, 2) — slot 2 shape depends on _bAdvanceReady

	ZmbResource shapeRes(ZmbArchiveKind::kPage, 6000);

	// Slot 1: Map/label button — always shape 5.
	// IDA: word_4A2292[18] = 600 (x), word_4A2294[18] = 403 (y), shapeIdx = 5
	_vm->_gfx->drawShape(ZoombiniGraphics::kShapeScreen, shapeRes, 5,
						  Common::Point(600, 403));

	// Slot 2: Go/advance button — shape depends on _bAdvanceReady.
	// IDA: word_4A2292[36] = 600 (x), word_4A2294[36] = 441 (y)
	//      shapeIdx = _bAdvanceReady ? 2 : 1
	uint16 goShapeIdx = _bAdvanceReady ? 2 : 1;
	_vm->_gfx->drawShape(ZoombiniGraphics::kShapeScreen, shapeRes, goShapeIdx,
						  Common::Point(600, 441));

	// Clear dirty flags after render
	_advanceButtonDirty = false;
	_columnLabelDirty = false;

	return ZmbRenderResult::kRendered;
}

void ZoombiniPuzzleNet::remapHotspotFramesByAttr(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	// IDA: net_remapHotspotFramesByAttr (0x438761)
	// Remaps hotspot shape indices based on current attribute column offsets.
	// Uses kColOffsets1 (unk_4A28D4) and kColOffsets2 (unk_4A28DE) lookup tables.
	// Also adjusts hotspot x/y positions when _hotspotPositionFlag is set (bounce animation).

	// Map current and previous column offsets through lookup tables
	int16 mappedCurCol1 = -1;  // v2: kColOffsets1[randAttrColOffset[1]]
	int16 mappedCurCol2 = -1;  // mappedOffset2: kColOffsets2[randAttrColOffset[2]]
	int16 mappedCurCol0 = -1;  // v1: kColOffsets2[randAttrColOffset[0]]
	int16 mappedPrevCol1 = -1; // mappedOffsetA: kColOffsets1[prevAttrColOffset[1]]
	int16 mappedPrevCol2 = -1; // mappedOffsetB: kColOffsets2[prevAttrColOffset[2]]
	int16 mappedPrevCol0 = -1; // mappedOffset0: kColOffsets2[prevAttrColOffset[0]]

	if (_randAttrColOffset[1] != -1)
		mappedCurCol1 = kColOffsets1[_randAttrColOffset[1]];
	if (_randAttrColOffset[2] != -1)
		mappedCurCol2 = kColOffsets2[_randAttrColOffset[2]];
	if (_randAttrColOffset[0] != -1)
		mappedCurCol0 = kColOffsets2[_randAttrColOffset[0]];
	if (_prevAttrColOffset[1] != -1)
		mappedPrevCol1 = kColOffsets1[_prevAttrColOffset[1]];
	if (_prevAttrColOffset[2] != -1)
		mappedPrevCol2 = kColOffsets2[_prevAttrColOffset[2]];
	if (_prevAttrColOffset[0] != -1)
		mappedPrevCol0 = kColOffsets2[_prevAttrColOffset[0]];

	for (uint i = 0; i < hotspots.size(); i++) {
		int16 shapeIdx = hotspots[i]._shapeIdx;
		if (shapeIdx == 0)
			break;

		if (1 <= shapeIdx && shapeIdx < 185) {
			// Range 1-5: prev column offsets (both col1 and col0 mapped)
			if (shapeIdx < 6 && mappedPrevCol1 >= 0 && mappedPrevCol0 != -1) {
				hotspots[i]._shapeIdx = mappedPrevCol1 + 12 * mappedPrevCol0 + 6;
			}
			// Range 1-5: prev column offset (only col1 mapped, no col0)
			else if (shapeIdx < 6 && mappedPrevCol1 != -1 && mappedPrevCol0 == -1) {
				hotspots[i]._shapeIdx = mappedPrevCol1 + 1;
			}
			// Range 6-10: current column offsets (both col1 and col0 mapped)
			else if (6 <= shapeIdx && shapeIdx < 11 && mappedCurCol1 >= 0 && mappedCurCol0 != -1) {
				hotspots[i]._shapeIdx = shapeIdx + mappedCurCol1 + 12 * mappedCurCol0;
			}
			// Range 6-10: current column offset (only col1 mapped)
			else if (6 <= shapeIdx && shapeIdx < 11 && mappedCurCol1 != -1) {
				hotspots[i]._shapeIdx = mappedCurCol1 + 1;
			}
			// Range 11-17: cube plane offset (col0 mapped)
			else if (11 <= shapeIdx && shapeIdx < 18 && mappedCurCol0 != -1) {
				hotspots[i]._shapeIdx = shapeIdx + 12 * mappedCurCol0;
			}
			// Range 66-87: row offset (col2 mapped)
			else if (66 <= shapeIdx && shapeIdx < 88 && mappedCurCol2 >= 0) {
				hotspots[i]._shapeIdx = shapeIdx + 22 * mappedCurCol2;
			}
			// Range 176+: prev row offset (prevCol2 mapped)
			else if (176 <= shapeIdx && mappedPrevCol2 != -1) {
				hotspots[i]._shapeIdx = 22 * mappedPrevCol2 + 66;
			}

			// Position adjustment when bouncing or placing at slot
			if (_hotspotPositionFlag) {
				if (i == 0) {
					// First hotspot: base position
					hotspots[i]._x = _bounceX;
					hotspots[i]._y = _bounceY;
				} else if (_bounceCounter) {
					// During bounce animation
					hotspots[i]._x = _bounceX + 4;
					hotspots[i]._y = _bounceY + 3;
				} else {
					// At rest in slot
					if (_difficultyLevel >= kPuzzleDiffLevel3)
						hotspots[i]._x = _bounceX + 3;
					else
						hotspots[i]._x = _bounceX + 21;
					hotspots[i]._y = _bounceY + 7;
				}
			}
		}
	}
}

void ZoombiniPuzzleNet::slotPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	// IDA: net_saveRunnerPosition (0x438736)
	// Adds the column indicator shape to the slot feature's hotspot list.
	//
	// In the original engine, this callback writes directly into the inline
	// hotspot array: hsArr[1].shapeid = shapeOffset, hsArr[1].pos = hsArr[0].pos,
	// hsArr[2].shapeid = 0 (terminator).
	//
	// In ScummVM, the hotspots array is a per-frame copy, so we append a
	// new hotspot with the indicator shape at the same position as the
	// SCRB's first hotspot (the slot background/dot).

	for (int16 i = 0; i < _totalSlotCount; i++) {
		if (_slotScrbFeatures[i] == feature && _slotColumnAssign[i] > 0) {
			int16 shapeOffset = (_difficultyLevel >= kPuzzleDiffLevel3)
				? _slotColumnAssign[i] + 153
				: _slotColumnAssign[i] + 150;
			if (!hotspots.empty()) {
				ZmbHotspot indicator(
					static_cast<uint16>(hotspots.size()),
					shapeOffset,
					hotspots[0]._frame,
					hotspots[0]._x, hotspots[0]._y);
				hotspots.push_back(indicator);
			}
			return;
		}
	}
}

// ---------------------------------------------------------------------------
// Core puzzle logic functions
// ---------------------------------------------------------------------------

void ZoombiniPuzzleNet::computeColumnSizes() {
	// IDA: net_computeColumnSizes (0x4393C4)
	// Distributes zoombinis into groups of 3,2,3,2... pattern.
	// Overwrites _columnCount with the number of groups.
	for (int16 i = 0; i < 12; i++)
		_columnSizes[i] = 0;

	int16 remaining = _loadedZmbCount;
	int16 size = 4;
	int16 groupCount = 0;

	do {
		if (--size < 1)
			size = 3;
		_columnSizes[groupCount++] = size;
		remaining -= size;
	} while (remaining > 0);

	_columnCount = groupCount;

	// If we overshot (remaining < 0), trim sizes from columns >= 2
	if (remaining < 0) {
		int16 excess = -remaining;
		while (excess > 0) {
			for (int16 i = 0; i < _columnCount && excess > 0; i++) {
				if (_columnSizes[i] >= 2) {
					_columnSizes[i]--;
					excess--;
				}
			}
			// Safety: if no column has size >= 2, set all to 1
			if (excess > 0) {
				int16 hasLarge = 0;
				for (int16 i = 0; i < _columnCount; i++) {
					if (_columnSizes[i] > 1)
						hasLarge++;
				}
				if (!hasLarge) {
					_columnCount = _loadedZmbCount;
					for (int16 i = 0; i < _columnCount; i++)
						_columnSizes[i] = 1;
					excess = 0;
				}
			}
		}
	}
}

void ZoombiniPuzzleNet::generateAttrRules() {
	// IDA: net_generateAttrRules (0x437A36)
	// Generates the rule grids that define the puzzle solution.

	// Clear slot column assignments
	memset(_slotColumnAssign, 0, sizeof(_slotColumnAssign));

	// Track used attribute values for uniqueness
	int16 usedA[5] = {0, 0, 0, 0, 0};
	int16 usedB[5] = {0, 0, 0, 0, 0};
	int16 usedC[5] = {0, 0, 0, 0, 0};

	// Generate 5 unique attribute combos
	for (int16 i = 0; i < 5; i++) {
		bool unique = false;
		int16 valA, valB, valC;
		do {
			valA = _vm->_rnd->getRandomNumber(0, 4);
			valB = _vm->_rnd->getRandomNumber(0, 4);
			valC = _vm->_rnd->getRandomNumber(0, 4);

			if (_difficultyLevel >= kPuzzleDiffLevel3) {
				// Diff 3+: no two combos share any single trait value
				if (!usedA[valA] && !usedB[valB] && !usedC[valC])
					unique = true;
			} else {
				// Diff 1-2: no two combos share both A AND B
				if (!usedA[valA] && !usedB[valB])
					unique = true;
			}

			if (unique) {
				usedA[valA]++;
				usedB[valB]++;
				usedC[valC]++;
			}
		} while (!unique);

		// Populate grids
		if (_difficultyLevel < kPuzzleDiffLevel3) {
			// 5x5 grid
			for (int16 j = 0; j < 5; j++) {
				_ruleGridA[5 * i + j] = valA;
				_ruleGridB[j * 5 + i] = valB;
			}
		} else {
			// 5x5x5 cube
			for (int16 j = 0; j < 25; j++)
				_ruleGridA[25 * i + j] = valA;
			for (int16 j = 0; j < 5; j++) {
				for (int16 k = 0; k < 5; k++)
					_ruleGridB[25 * j + 5 * i + k] = valB;
			}
			for (int16 j = 0; j < 5; j++) {
				for (int16 k = 0; k < 5; k++)
					_ruleGridC[25 * k + 5 * j + i] = valB;
			}
		}
	}

	// Apply rotations for difficulty 1 (shift rows of gridB)
	if (_difficultyLevel == kPuzzleDiffLevel2) {
		int16 shift = _vm->_rnd->getRandomNumber(0, 1) + 2;
		int16 tempBuf[5];
		for (int16 row = 0; row < 5; row++)
			tempBuf[row] = _ruleGridB[row];

		for (int16 row = 1; row < 5; row++) {
			for (int16 s = 0; s < shift; s++) {
				int16 last = tempBuf[4];
				for (int16 k = 4; k > 0; k--)
					tempBuf[k] = tempBuf[k - 1];
				tempBuf[0] = last;
			}
			for (int16 col = 0; col < 5; col++)
				_ruleGridB[5 * row + col] = tempBuf[col];
		}
	}
	// Apply rotations for difficulty 3 (shift columns of gridA across 3D cube)
	else if (_difficultyLevel == kPuzzleDiffLevel4) {
		int16 shift = _vm->_rnd->getRandomNumber(0, 1) + 2;
		_vm->_rnd->getRandomNumber(0, 1); // extra random call to match original
		int16 tempBuf[5];

		for (int16 plane = 0; plane < 5; plane++) {
			for (int16 col = 1; col < 5; col++) {
				// IDA: reads via net_columnCount[idx] which is ruleGridA[idx-1]
				// due to net_columnCount being 2 bytes before ruleGridA in memory.
				// This cascading shift reads z=col-1 and writes to z=col.
				for (int16 k = 0; k < 5; k++)
					tempBuf[k] = _ruleGridA[25 * k + 5 * plane + col - 1];
				for (int16 s = 0; s < shift; s++) {
					int16 last = tempBuf[4];
					for (int16 k = 4; k > 0; k--)
						tempBuf[k] = tempBuf[k - 1];
					tempBuf[0] = last;
				}
				for (int16 k = 0; k < 5; k++)
					_ruleGridA[25 * k + col + 5 * plane] = tempBuf[k];
			}
		}
	}

	// Distribute slots to columns — assign column sizes to random slot positions
	int16 slotBase = (_difficultyLevel >= kPuzzleDiffLevel3) ? 25 : 0;
	for (int16 i = 0; i < _columnCount; i++) {
		int16 pos;
		do {
			pos = (_difficultyLevel >= kPuzzleDiffLevel3)
				? _vm->_rnd->getRandomNumber(0, 124)
				: _vm->_rnd->getRandomNumber(0, 24);
		} while (_slotColumnAssign[pos] != 0);
		_slotColumnAssign[pos] = _columnSizes[i];
	}

	// Register slot SCRB runners for non-empty slots
	for (int16 i = 0; i < _totalSlotCount; i++) {
		_slotScrbFeatures[i] = nullptr;
		if (_slotColumnAssign[i] != 0) {
			uint16 scrbId = i + slotBase + 9000;
			int16 shapeOffset = (_difficultyLevel >= kPuzzleDiffLevel3)
				? _slotColumnAssign[i] + 153
				: _slotColumnAssign[i] + 150;

			// IDA: flags = 0x4188000 = OVERLAY | PLAY_ONCE | DEFER_ANIM | LOOP_ANIM
			// Original uses DEFER_ANIM + runner_linkRelativeToParent(column[0], 0, slot)
			// to make slots visible only when linked to the column.
			// Since runner_linkRelativeToParent is not implemented, we use LOOP_ANIM | OVERLAY
			// (auto-activate) and immediately freeze the animation on frame 0 so the slot
			// feature renders its indicator shape statically without flickering.
			_slotScrbFeatures[i] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 9000), scrbId, 6,
				ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);

			if (_slotScrbFeatures[i]) {
				// IDA: v25[5] = net_saveRunnerPosition; *(WORD*)(v25+44) = shapeOffset;
				// Set preRenderShape callback to add column indicator shape at slot position.
				_slotScrbFeatures[i]->setPreRenderShapeFunc(
					reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleNet::slotPreRenderShape));
				// IDA: scrb_useFeatureGroup(1, 2, 9000) — REGS 9000/9001 provides
				// registration-point offsets for indicator shapes (151-156).
				_slotScrbFeatures[i]->setShapeRegs(_regsMap[9000]);
				// Freeze animation on frame 0: slot shows its static indicator permanently.
				// LOOP_ANIM activateAnimate() sets _frameTimingReady = true;
				// deactivateAnimate() keeps _frameTimingReady true but stops frame advance.
				_slotScrbFeatures[i]->deactivateAnimate();
			}
			(void)shapeOffset; // shapeOffset is derived on-the-fly in slotPreRenderShape
		}
	}

	// Generate attribute labels
	if (_difficultyLevel >= kPuzzleDiffLevel4) {
		// All three labels must be distinct (0, 1, 2 in some order)
		int16 attempts = 0;
		do {
			_attrRowLabel = _vm->_rnd->getRandomNumber(0, 2);
			_attrColLabel = _vm->_rnd->getRandomNumber(0, 2);
			_seed = _vm->_rnd->getRandomNumber(0, 2);
			attempts++;
		} while (_attrRowLabel == _attrColLabel ||
				 _attrColLabel == _seed ||
				 _seed == _attrRowLabel);
	} else {
		// Two labels: randomly swap row and column
		if (_vm->_rnd->getRandomNumber(0, 1)) {
			_attrRowLabel = 2;
			_attrColLabel = 1;
		} else {
			_attrRowLabel = 1;
			_attrColLabel = 2;
		}
		_seed = 0;
	}

	// Attribute permutation for difficulty 3
	if (_difficultyLevel >= kPuzzleDiffLevel4)
		_attrPermutationIdx = _vm->_rnd->getRandomNumber(0, 5);
	else
		_attrPermutationIdx = 0;
}

int16 ZoombiniPuzzleNet::findSlotByAttrColumns() {
	// IDA: net_findSlotByAttrColumns (0x438C47)
	// Searches grids for the slot matching current column offsets.

	if (_difficultyLevel < kPuzzleDiffLevel3) {
		// 2D lookup (two attributes)
		for (int16 i = 0; i < _totalSlotCount; i++) {
			if (_attrRowLabel == 2) {
				if (_ruleGridA[i] == _randAttrColOffset[2] &&
					_ruleGridB[i] == _randAttrColOffset[1])
					return i;
			} else {
				if (_ruleGridB[i] == _randAttrColOffset[2] &&
					_ruleGridA[i] == _randAttrColOffset[1])
					return i;
			}
		}
	} else {
		// 3D lookup (three attributes) with 6 permutations
		for (int16 i = 0; i < _totalSlotCount; i++) {
			bool match = false;
			switch (_attrPermutationIdx) {
			case 0:
				match = (_ruleGridA[i] == _randAttrColOffset[2] &&
						 _ruleGridB[i] == _randAttrColOffset[1] &&
						 _ruleGridC[i] == _randAttrColOffset[0]);
				break;
			case 1:
				match = (_ruleGridA[i] == _randAttrColOffset[1] &&
						 _ruleGridB[i] == _randAttrColOffset[2] &&
						 _ruleGridC[i] == _randAttrColOffset[0]);
				break;
			case 2:
				match = (_ruleGridA[i] == _randAttrColOffset[0] &&
						 _ruleGridB[i] == _randAttrColOffset[2] &&
						 _ruleGridC[i] == _randAttrColOffset[1]);
				break;
			case 3:
				match = (_ruleGridA[i] == _randAttrColOffset[2] &&
						 _ruleGridB[i] == _randAttrColOffset[0] &&
						 _ruleGridC[i] == _randAttrColOffset[1]);
				break;
			case 4:
				match = (_ruleGridA[i] == _randAttrColOffset[1] &&
						 _ruleGridB[i] == _randAttrColOffset[0] &&
						 _ruleGridC[i] == _randAttrColOffset[2]);
				break;
			case 5:
				match = (_ruleGridA[i] == _randAttrColOffset[0] &&
						 _ruleGridB[i] == _randAttrColOffset[1] &&
						 _ruleGridC[i] == _randAttrColOffset[2]);
				break;
			default:
				break;
			}
			if (match)
				return i;
		}
	}
	return -1;
}

void ZoombiniPuzzleNet::updateAttrColumnOffset(int16 value, int16 columnGroup) {
	// IDA: net_updateAttrColumnOffset (0x438108)
	// Updates column selector and triggers animations.

	// Check if all required columns are set (pre-check for submit validation)
	bool allColumnsSet = false;
	if (_difficultyLevel <= kPuzzleDiffLevel2) {
		allColumnsSet = (_randAttrColOffset[1] >= 0 && _randAttrColOffset[2] >= 0);
	} else {
		allColumnsSet = (_randAttrColOffset[0] >= 0 && _randAttrColOffset[1] >= 0 && _randAttrColOffset[2] >= 0);
	}

	if (columnGroup == 0) {
		// Submit — process current selection
		_prevAttrColOffset[2] = _randAttrColOffset[2];
		_prevAttrColOffset[1] = _randAttrColOffset[1];
		_prevAttrColOffset[0] = _randAttrColOffset[0];

		if (allColumnsSet) {
			// Play submit feedback sound
			loadScrbOntoFeature(_feedbackScrbFeature, 10001);

			_attrColumnsReady = false;
			_currentSlotIndex = findSlotByAttrColumns();

			// Compute column index and sort animation type
			int16 colIdx;
			if (_difficultyLevel >= kPuzzleDiffLevel3)
				colIdx = _currentSlotIndex % 25 / 5;
			else
				colIdx = _currentSlotIndex % 5;

			// IDA: MEMORY[0x4A2860] lookup: col0→1, col1-3→0, col4→2
			static const int16 kSortScrbLookup[3] = {1, 0, 2};
			if (colIdx == 0)
				_sortAnimType = 1;
			else if (colIdx >= 1 && colIdx < 4)
				_sortAnimType = 0;
			else
				_sortAnimType = 2;

			// Load submit sort animation SCRB
			uint16 sortScrbId = 7028 + kSortScrbLookup[_sortAnimType];
			loadScrbOntoFeature(_attrAnimScrbFeature, sortScrbId);
			if (_attrAnimScrbFeature) {
				_hotspotPositionFlag = 0;
				// IDA: v10[5] = net_remapHotspotFramesByAttr
				_attrAnimScrbFeature->setPreRenderShapeFunc(
					reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleNet::remapHotspotFramesByAttr));
				_attrAnimScrbFeature->setFrameInterval(6);
				_sortAnimRunning = true;
			}
		}
	} else if (columnGroup == 1) {
		// Column 0 selector (only at difficulty 3+)
		if (_difficultyLevel >= kPuzzleDiffLevel3) {
			_prevAttrColOffset[1] = _randAttrColOffset[1];
			_prevAttrColOffset[0] = _randAttrColOffset[0];
			_randAttrColOffset[0] = value;

			if (_prevAttrColOffset[0] != value) {
				loadScrbOntoFeature(_attrColScrbFeatures[0], 10002 + value);
				if (!_rejectedCount && allColumnsSet) {
					loadScrbOntoFeature(_attrAnimScrbFeature, 7027);
					if (_attrAnimScrbFeature) {
						_hotspotPositionFlag = 0;
						// IDA: v4[5] = net_remapHotspotFramesByAttr
						_attrAnimScrbFeature->setPreRenderShapeFunc(
							reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleNet::remapHotspotFramesByAttr));
					}
					if (!_submitCount)
						loadScrbOntoFeature(_feedbackScrbFeature, 10018);
				}
			}
		}
	} else if (columnGroup == 2) {
		// Column 1 selector
		_prevAttrColOffset[1] = _randAttrColOffset[1];
		_randAttrColOffset[1] = value;
		_prevAttrColOffset[0] = _randAttrColOffset[0];

		if (_prevAttrColOffset[1] != value) {
			loadScrbOntoFeature(_attrColScrbFeatures[1], 10007 + value);
			if (!_rejectedCount && allColumnsSet) {
				loadScrbOntoFeature(_attrAnimScrbFeature, 7019);
				if (_attrAnimScrbFeature) {
					_attrAnimScrbFeature->setFrameInterval(3);
					_hotspotPositionFlag = 0;
					// IDA: v7[5] = net_remapHotspotFramesByAttr
					_attrAnimScrbFeature->setPreRenderShapeFunc(
						reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleNet::remapHotspotFramesByAttr));
				}
				if (!_submitCount)
					loadScrbOntoFeature(_feedbackScrbFeature, 10018);
			}
		}
	} else if (columnGroup == 3) {
		// Column 2 selector
		_prevAttrColOffset[2] = _randAttrColOffset[2];
		_prevAttrColOffset[1] = _randAttrColOffset[1];
		_prevAttrColOffset[0] = _randAttrColOffset[0];
		_randAttrColOffset[2] = value;

		if (_prevAttrColOffset[2] != value) {
			loadScrbOntoFeature(_attrColScrbFeatures[2], 10012 + value);
			if (!_rejectedCount && allColumnsSet) {
				loadScrbOntoFeature(_attrAnimScrbFeature, 7026);
				if (_attrAnimScrbFeature) {
					_hotspotPositionFlag = 0;
					// IDA: v8[5] = net_remapHotspotFramesByAttr
					_attrAnimScrbFeature->setPreRenderShapeFunc(
						reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleNet::remapHotspotFramesByAttr));
				}
				if (!_submitCount)
					loadScrbOntoFeature(_feedbackScrbFeature, 10018);
			}
		}
	}

	// If columns not yet ready, trigger column setup
	if (!_attrColumnsReady && !_rejectedCount) {
		_pendingColumnSetup++;
		_attrColumnsReady = true;
	}
}

void ZoombiniPuzzleNet::assignNextZmbToColumn() {
	// IDA: net_assignNextZmbToColumn (0x438017)
	// Finds an empty column slot and assigns the next zoombini.

	for (int16 i = 2; i >= 0; i--) {
		if (_columnSlotSnoidIds[i] != 0)
			continue;

		if (_nextZmbToAssign >= _loadedZmbCount) {
			_allColumnsExhausted++;
			continue;
		}

		if (!_zmbReadyCount)
			return;

		if (--_zmbQueueCount <= 0)
			_zmbReadyCount = 0;

		uint16 snoidId = 10000 + _nextZmbToAssign;
		ZmbSnoid *snoid = getSnoid(snoidId);
		if (snoid) {
			snoid->setAnimState(kSnoidAnimArrivalMotion);
			_pendingZmbIndex = _nextZmbToAssign;
			_columnSlotSnoidIds[i] = snoidId;
			_nextZmbToAssign++;
			_activeColumnIdx = i;
			_zmbWalkPending = true;
		}
		return;
	}

	_zmbQueueCount = 0;
	_zmbReadyCount = 0;
}

void ZoombiniPuzzleNet::registerZmbAtSlot(int16 slotIndex) {
	// IDA: net_registerZmbAtSlot (0x438A84)
	// Positions and registers a zoombini display at the given net slot.

	if (slotIndex < 0)
		return;

	// Get position from appropriate table
	if (_difficultyLevel >= kPuzzleDiffLevel3) {
		_bounceX = kSlotPositionsHigh[slotIndex].x;
		_bounceY = kSlotPositionsHigh[slotIndex].y;
	} else {
		_bounceX = kSlotPositionsLow[slotIndex].x;
		_bounceY = kSlotPositionsLow[slotIndex].y;
	}

	_hotspotPositionFlag++;

	// Load slot display SCRB
	uint16 scrbId = (_difficultyLevel >= kPuzzleDiffLevel3) ? 7024 : 7023;
	if (_activeSlotFeatures[_slotRunnerCount]) {
		loadScrbOntoFeature(_activeSlotFeatures[_slotRunnerCount], scrbId);
	} else {
		_activeSlotFeatures[_slotRunnerCount] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 7000), scrbId, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_04000000_OVERLAY);
		if (_activeSlotFeatures[_slotRunnerCount])
			_activeSlotFeatures[_slotRunnerCount]->setShapeRegs(_regsMap[7000]);
	}

	// Score tracking
	_noMatchFlag = false;
	_columnMatchCount = _slotColumnAssign[_currentSlotIndex];
	if (_columnMatchCount >= 1) {
		_idleAnimTrigger = true;
	} else {
		_columnMatchCount = 0;
		_submitCount = 0;
		_noMatchFlag = true;
	}
	_sortedZmbCount += _columnMatchCount;
	_slotColumnAssign[slotIndex] = -1;
}

void ZoombiniPuzzleNet::spawnZmbAtSlot(int16 slotIndex) {
	// IDA: net_spawnZmbAtSlot (0x439489)
	// Initiates the bounce animation to spawn a zoombini at a net slot.

	if (_bounceCounter)
		return;

	_currentSlotIndex = slotIndex;
	_bounceCounter = 1;

	// Get target position
	if (_difficultyLevel >= kPuzzleDiffLevel3) {
		_bounceX = kSlotPositionsHigh[slotIndex].x;
		_bounceY = kSlotPositionsHigh[slotIndex].y;
	} else {
		_bounceX = kSlotPositionsLow[slotIndex].x;
		_bounceY = kSlotPositionsLow[slotIndex].y;
	}

	// Compute bounce deltas from start point (484, 318)
	_bounceDeltaX = (484 - _bounceX) / 6;
	_bounceDeltaY = (318 - _bounceY) / 6;
	_bounceX = 484;
	_bounceY = 318;

	_slotRunnerCount++;

	// Load sort display SCRB with bounce pre-render
	static const int16 kSortScrbLookup[3] = {1, 0, 2};
	uint16 sortScrbId = 7020 + kSortScrbLookup[_sortAnimType];

	// IDA: flags 0x04100000 = OVERLAY | PLAY_ONCE
	_activeSlotFeatures[_slotRunnerCount] = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), sortScrbId, 6,
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);

	if (_activeSlotFeatures[_slotRunnerCount]) {
		_activeSlotFeatures[_slotRunnerCount]->setShapeRegs(_regsMap[7000]);
		_hotspotPositionFlag++;
		// IDA: v2[5] = net_remapHotspotFramesByAttr
		_activeSlotFeatures[_slotRunnerCount]->setPreRenderShapeFunc(
			reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleNet::remapHotspotFramesByAttr));
		_activeSlotFeatures[_slotRunnerCount]->setFrameInterval(3);
	}
}

// ---------------------------------------------------------------------------
// Click handling
// ---------------------------------------------------------------------------

ZmbEventHandleResult ZoombiniPuzzleNet::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// IDA: net_funcOnClick_43747F — original dispatches by hotspot IDs 1-19
	// using CButtonRunner fixed click rects (word_4A2292, 36-byte stride).
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	if (_inputLocked)
		return ZmbEventHandleResult::kPassthrough;

	// Hit-test against fixed click rects (matching original button runner table)
	int16 buttonIdx = -1;
	for (int16 i = 0; i < 16; i++) {
		if (kButtonClickRects[i].contains(absPos)) {
			buttonIdx = i;
			break;
		}
	}

	if (buttonIdx < 0)
		return ZmbEventHandleResult::kPassthrough;

	if (buttonIdx == 0) {
		// Submit button (hotspot 4)
		if (!_submitCount && !_submitActiveFlag && !_rejectedCount) {
			_submitCount++;
			_submitActiveFlag++;
			_lastSubmitFrame = getCurrentFrameCounter();
			updateAttrColumnOffset(0, 0);
		}
		return ZmbEventHandleResult::kConsumed;
	}

	// Column selector buttons (hotspots 5-19)
	// buttonIdx 1-5 = column 0, 6-10 = column 1, 11-15 = column 2
	int16 colIdx = (buttonIdx - 1) / 5;  // 0, 1, or 2
	int16 value = (buttonIdx - 1) % 5;   // 0-4

	// Column 0 only active at difficulty >= 3
	if (colIdx == 0 && _difficultyLevel <= kPuzzleDiffLevel2)
		return ZmbEventHandleResult::kPassthrough;

	if (_submitActiveFlag && !_rejectedCount)
		return ZmbEventHandleResult::kConsumed;

	updateAttrColumnOffset(value, colIdx + 1);
	return ZmbEventHandleResult::kConsumed;
}

// ---------------------------------------------------------------------------
// Animation event dispatch
// ---------------------------------------------------------------------------

void ZoombiniPuzzleNet::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// IDA: net_zmbAnimCallback (0x438EA1) — unified callback for all NET features.
	// NOTE: The ASCII-event traversal callback (0x43105B, net_scrbAnimCallback) belongs
	// to MAZE2 (Bubblewonder Abyss), NOT NET (Mudball Wall). NET uses only this callback.
	if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
		processSnoidAnimEvent(feature, eventCode);
	} else {
		processZmbScrbAnimEvent(feature, eventCode);
	}
}

void ZoombiniPuzzleNet::processSnoidAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// IDA: net_zmbAnimCallback (0x438EA1) — snoid-specific events
	ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);

	if (eventCode == kZmbAnimEventM1_End) {
		// IDA: Event -1 — end of animation
		if (_zmbsAtColumns) {
			// Exit phase: position zoombini at exit point
			if (_exitPositionIdx < 16) {
				Common::Point exitPos = kExitPositions[_exitPositionIdx++];
				ZmbSnoid *activeSnoid = getSnoid(_activeZmbSnoidId);
				if (activeSnoid) {
					// IDA: animateZoombini(0, 7u, ...) then *(int*)(v8+278) = exitPos
					// initWalkToTarget sets _animTargetPos AND calls setAnimState(kSnoidAnimDepart)
					activeSnoid->initWalkToTarget(exitPos);
					activeSnoid->_packIsOccupied = true;
				}
				_exitingZmbSnoidId = _activeZmbSnoidId;
				_bAdvanceReady = true;
				_zmbsAtColumns = 0;

				if (_sortedZmbCount >= _loadedZmbCount || _allColumnsExhausted) {
					_submitCount = 0;
					_submitActiveFlag = 0;
				}

				// Check if all zoombinis are done
				if (!_zmbQueueCount &&
					!_columnSlotSnoidIds[0] && !_columnSlotSnoidIds[1] && !_columnSlotSnoidIds[2]) {
					if (_sortedZmbCount >= _loadedZmbCount) {
						_vm->_sound->playZmbSound(
							ZmbResource(ZmbArchiveKind::kSystem,
								_vm->_rnd->getRandomNumber(20055, 20063)));
					}
					_roundCompletedFlag = true;
					_idleAnimTrigger = true;
					_idleAnimMax = _loadedZmbCount;
				}

				// Process pending hint
				if (_hintPending) {
					_hintPending = false;
					if (_sortedZmbCount >= 1 && _sortedZmbCount < _loadedZmbCount) {
						_vm->_sound->playZmbSound(
							ZmbResource(ZmbArchiveKind::kSystem,
								_vm->_rnd->getRandomNumber(20045, 20048)));
					}
				}
			}
		} else {
			// Walk-off cleanup: find matching walkSlot, clear it, turn snoid left
			for (int16 i = 2; i >= 0; i--) {
				if (_walkSlotSnoidIds[i] == feature->getId()) {
					ZmbSnoid *walkSnoid = getSnoid(_walkSlotSnoidIds[i]);
					_walkSlotSnoidIds[i] = 0;
					if (walkSnoid) {
						walkSnoid->setAnimState(kSnoidAnimTurnLeft);
						if (!_walkSlotSnoidIds[0] && !_walkSlotSnoidIds[1] && !_walkSlotSnoidIds[2]) {
							loadScrbOntoFeature(_feedbackScrbFeature, 10018);
							_submitCount = 0;
							_submitActiveFlag = 0;
						}
					}
					return;
				}
			}
			// Fallback: check if queue is empty
			if (!_zmbQueueCount && !_columnSlotSnoidIds[0]) {
				_submitCount = 0;
				_submitActiveFlag = 0;
			}
		}
		return;
	}

	switch (eventCode) {
	case 0:
		// Toggle render visibility
		if (feature->isRenderActivated())
			feature->deactivateRender();
		else
			feature->activateRender();
		// Apply pending body arrangement
		if (_pendingBodyArrangement) {
			snoid->setBodyArrangement(_pendingBodyArrangement - 1);
			_pendingBodyArrangement = 0;
		}
		break;

	case 2:
		// Spawn zoombini SCRB at the current slot.
		// IDA: net_zmbAnimCallback case 2 — same handler for snoid and SCRB callers.
		// Seating SCRS scripts (13016-13030) can fire this from snoid context.
		spawnZmbAtSlot(_currentSlotIndex);
		break;

	case 4: {
		// Start snoid travel to column — play entry SCRS.
		// IDA: net_zmbAnimCallback case 4 — fires from seating SCRS (13016-13018)
		// via 0xFF05 terminator (raw 5, adjusted 4) at the last frame.
		// Operates on _activeZmbSnoidId (the column snoid), NOT the calling feature.
		ZmbSnoid *activeSnoid = getSnoid(_activeZmbSnoidId);
		if (activeSnoid) {
			Common::Point entryPos = kEntryStartPositions[_activeColumnIdx];
			uint16 scrsId = _activeColumnIdx + 14000;
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
					ZmbResource(ZmbArchiveKind::kPage, scrsId));
			if (scrsStream) {
				activeSnoid->setPointLoc(entryPos);
				activeSnoid->startScrsPlayback(scrsStream, false, false);
			}
		}
		// IDA: 4x runner_linkRelativeToParent chain (not implemented)
		break;
	}

	case 30:
		// Column exit: play exit SCRS at entry exit position
		if (_activeZmbSnoidId) {
			Common::Point exitPos = kEntryExitPositions[_activeColumnIdx];
			ZmbSnoid *activeSnoid = getSnoid(_activeZmbSnoidId);
			if (activeSnoid) {
				uint16 scrsId = _activeColumnIdx + 3 * (activeSnoid->_trait._foot - 1) + 13031;
				Common::SeekableReadStream *scrsStream =
					_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
						ZmbResource(ZmbArchiveKind::kPage, scrsId));
				if (scrsStream) {
					activeSnoid->setPointLoc(exitPos);
					activeSnoid->startScrsPlayback(scrsStream, false, false);
				}
			}
			_zmbsAtColumns++;
			_lastLinkedSnoidId = _activeZmbSnoidId;
		}
		break;

	default:
		if (eventCode >= kZmbAnimEvent240_BodyArrangePendFirst &&
			eventCode <= kZmbAnimEvent243_BodyArrangePendLast) {
			_pendingBodyArrangement = eventCode - 239;
		} else if (eventCode >= kZmbAnimEvent250_BodyArrangeDirectFirst &&
				   eventCode <= kZmbAnimEvent253_BodyArrangeDirectLast) {
			snoid->setBodyArrangement(eventCode - 250);
		}
		break;
	}
}

void ZoombiniPuzzleNet::processZmbScrbAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// IDA: net_zmbAnimCallback (0x438EA1) — events from SCRB features with zmb routing
	// These events operate on global state (activeZmbSnoidId, columnSlotSnoidIds)
	// rather than on the calling feature itself.

	switch (eventCode) {
	case 0:
		// Toggle visibility on the calling SCRB feature
		if (feature->isRenderActivated())
			feature->deactivateRender();
		else
			feature->activateRender();
		break;

	case 2:
		// Spawn zoombini SCRB at the current slot
		spawnZmbAtSlot(_currentSlotIndex);
		break;

	case 4: {
		// Start snoid travel to column — play entry SCRS
		ZmbSnoid *activeSnoid = getSnoid(_activeZmbSnoidId);
		if (activeSnoid) {
			Common::Point entryPos = kEntryStartPositions[_activeColumnIdx];
			uint16 scrsId = _activeColumnIdx + 14000;
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
					ZmbResource(ZmbArchiveKind::kPage, scrsId));
			if (scrsStream) {
				activeSnoid->setPointLoc(entryPos);
				activeSnoid->startScrsPlayback(scrsStream, false, false);
			}
		}
		break;
	}

	case 20: {
		// Activate runner at column slot — play positioning SCRS
		_activeZmbSnoidId = _columnSlotSnoidIds[_activeColumnIdx];
		ZmbSnoid *colSnoid = getSnoid(_activeZmbSnoidId);
		if (colSnoid) {
			uint16 scrsId = 3 * (colSnoid->_trait._foot - 1) + 2 - _activeColumnIdx + 13016;
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
					ZmbResource(ZmbArchiveKind::kPage, scrsId));
			if (scrsStream) {
				colSnoid->_packIsOccupied = true;
				colSnoid->startScrsPlayback(scrsStream, false, false);
			}
			_pendingZmbIndex = _nextZmbToAssign;
			_activeZmbSnoidId = _columnSlotSnoidIds[_activeColumnIdx];
			_columnSlotSnoidIds[_activeColumnIdx] = 0;
			if (!_allColumnsExhausted && _nextZmbToAssign < _loadedZmbCount)
				_zmbQueueCount++;
		}
		break;
	}

	case 30: {
		// Column exit from SCRB context — delegates to snoid event 30 logic
		if (_activeZmbSnoidId) {
			Common::Point exitPos = kEntryExitPositions[_activeColumnIdx];
			ZmbSnoid *activeSnoid = getSnoid(_activeZmbSnoidId);
			if (activeSnoid) {
				uint16 scrsId = _activeColumnIdx + 3 * (activeSnoid->_trait._foot - 1) + 13031;
				Common::SeekableReadStream *scrsStream =
					_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
						ZmbResource(ZmbArchiveKind::kPage, scrsId));
				if (scrsStream) {
					activeSnoid->setPointLoc(exitPos);
					activeSnoid->startScrsPlayback(scrsStream, false, false);
				}
			}
			_zmbsAtColumns++;
			_lastLinkedSnoidId = _activeZmbSnoidId;
		}
		break;
	}

	case kZmbAnimEventM1_End:
		// End of SCRB animation — same as snoid event -1 but from SCRB context
		// (Handled via hasAnimEndCallbackFired polling in onEveryFrame)
		break;

	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// onEveryFrame: Per-frame idle animation scheduling.
// IDA: net_onFrameTick @ 0x43728B
// ---------------------------------------------------------------------------
void ZoombiniPuzzleNet::onEveryFrame() {
	// IDA: net_onFrameTick (0x436861) — main per-frame state machine.

	// Phase 1: Exit animation sequence (opening animation)
	// IDA: SCRB 7000 is loaded with DEFER_ANIM — never plays.
	// On the first frame tick, hotspot_ownerRunnerArr[exitRunner]==0 (no render occurred),
	// so exitAnimStep immediately advances from 0→1, loading SCRB 7001.
	// Subsequent SCRBs play with PLAY_ONCE and advance when complete.
	if (_exitAnimActive && _exitRunnerActive) {
		if (getCurrentFrameCounter() % 60 == 0)
			debugC(1, kZmbDebugAnimation, "NET Phase1: exitStep=%d/%d animEndFired=%d isAnimActivated=%d isRenderActivated=%d frameIdx=%d maxFrame=%d flags=0x%08X timingReady=%d curFrame=%u",
				_exitAnimStep, _remainingExitSteps,
				_exitScrbFeature->hasAnimEndCallbackFired() ? 1 : 0,
				_exitScrbFeature->isAnimateActivated() ? 1 : 0,
				_exitScrbFeature->isRenderActivated() ? 1 : 0,
				_exitScrbFeature->getLastFrameIdx(),
				_exitScrbFeature->getMaxFrameIdx(),
				_exitScrbFeature->getFlags(),
				_exitScrbFeature->isFrameTimingReady() ? 1 : 0,
				getCurrentFrameCounter());
	}
	if (_exitAnimActive && _exitRunnerActive) {
		// IDA: step 0 is deferred (SCRB 7000 placeholder), advance immediately.
		// Steps >= 1 wait for PLAY_ONCE animation to complete.
		bool shouldAdvance = (_exitAnimStep == 0) || _exitScrbFeature->hasAnimEndCallbackFired();

		if (shouldAdvance) {
			debugC(1, kZmbDebugAnimation, "NET Phase1 DONE step %d/%d, loading next SCRB %d",
				_exitAnimStep, _remainingExitSteps, _exitAnimStep + 1 + 7000);
			_exitRunnerActive = false;
			if (++_exitAnimStep >= _remainingExitSteps) {
				debugC(1, kZmbDebugAnimation, "NET Phase1 ALL DONE - exit animation complete");
				_exitAnimActive = false;
				_zmbQueueCount = 3;
				_zmbReadyCount++;
				assignNextZmbToColumn();
				_pendingColumnSetup = 1;
				_attrColumnsReady = true;
			} else {
				loadScrbOntoFeature(_exitScrbFeature, _exitAnimStep + 7000);
				_exitRunnerActive = true;
			}
		}
	}

	// Phase 2: Label animation → generate rules
	if (_labelAnimRunning) {
		if (getCurrentFrameCounter() % 60 == 0)
			debugC(1, kZmbDebugAnimation, "NET Phase2: animEndFired=%d isAnimActivated=%d isRenderActivated=%d frameIdx=%d maxFrame=%d",
				_labelScrbFeature->hasAnimEndCallbackFired() ? 1 : 0,
				_labelScrbFeature->isAnimateActivated() ? 1 : 0,
				_labelScrbFeature->isRenderActivated() ? 1 : 0,
				_labelScrbFeature->getLastFrameIdx(),
				_labelScrbFeature->getMaxFrameIdx());
	}
	if (_labelAnimRunning && _labelScrbFeature->hasAnimEndCallbackFired()) {
		debugC(1, kZmbDebugAnimation, "NET Phase2 DONE - generating rules");
		_labelAnimRunning = false;
		generateAttrRules();
	}

	// Phase 3: Sort animation completion (submit result)
	if (_sortAnimRunning) {
		if (_attrAnimScrbFeature->hasAnimEndCallbackFired()) {
			_sortAnimRunning = false;
			if (--_remainingExitSteps < 0) {
				_rejectedCount++;
				if (_vm->_rnd->getRandomNumber(0, 4) > (_difficultyLevel - 1) ||
					(_vm->_state->_f._pageFlagNet & 0xFFF) <= 3) {
					if (_columnMatchCount >= 1) {
						if (_nextZmbToAssign < _loadedZmbCount)
							_hintPending = true;
					} else if (_sortedZmbCount >= 1 && _sortedZmbCount < _loadedZmbCount) {
						_vm->_sound->playZmbSound(
							ZmbResource(ZmbArchiveKind::kSystem,
								_vm->_rnd->getRandomNumber(20045, 20048)));
					}
				}
			}
			if (_columnMatchCount < 1)
				_columnMatchCount = 0;
			else
				_columnAnimDone++;
		}
		goto label_postColumn;
	}

	// Phase 4: Pending column setup
	if (_pendingColumnSetup && !_rejectedCount) {
		bool allSet;
		if (_difficultyLevel <= kPuzzleDiffLevel2)
			allSet = (_randAttrColOffset[1] >= 0 && _randAttrColOffset[2] >= 0);
		else
			allSet = (_randAttrColOffset[0] >= 0 && _randAttrColOffset[1] >= 0 && _randAttrColOffset[2] >= 0);

		if (getCurrentFrameCounter() % 60 == 0)
			debugC(1, kZmbDebugAnimation, "NET Phase4: pendingColumnSetup=%d rejectedCount=%d allSet=%d diff=%d offsets=[%d,%d,%d]",
				_pendingColumnSetup, _rejectedCount, allSet ? 1 : 0, _difficultyLevel,
				_randAttrColOffset[0], _randAttrColOffset[1], _randAttrColOffset[2]);

		if (allSet) {
			_pendingColumnSetup = 0;
			debugC(1, kZmbDebugAnimation, "NET Phase4 DONE: loading SCRB %d, pendingAttrRunning=true", _exitScrbOffset + 7031);
			loadScrbOntoFeature(_exitScrbFeature, _exitScrbOffset + 7031);
			_pendingAttrRunning = true;
			if (++_exitScrbOffset > 16)
				_exitScrbOffset = 16;
		}
		goto label_postColumn;
	}

	// Phase 5: Pending attr runner (one-shot trigger)
	if (_pendingAttrRunning) {
		debugC(1, kZmbDebugAnimation, "NET Phase5: pendingAttrRunning -> loading 7018");
		_pendingAttrRunning = false;
		_prevAttrColOffset[1] = _randAttrColOffset[1];
		_prevAttrColOffset[2] = -1;
		_prevAttrColOffset[0] = -1;
		if (_difficultyLevel <= kPuzzleDiffLevel2)
			_randAttrColOffset[0] = -1;
		if (!_rejectedCount) {
			loadScrbOntoFeature(_attrAnimScrbFeature, 7018);
			if (_attrAnimScrbFeature) {
				_hotspotPositionFlag = 0;
				_attrAnimScrbFeature->setFrameInterval(3);
				// IDA: v1->onPreRenderShapeFunc = net_remapHotspotFramesByAttr
				_attrAnimScrbFeature->setPreRenderShapeFunc(
					reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleNet::remapHotspotFramesByAttr));
				_activeAttrRunning = true;
			}
		}
		goto label_postColumn;
	}

	// Phase 6: Active attr animation
	if (_activeAttrRunning) {
		if (getCurrentFrameCounter() % 120 == 0)
			debugC(1, kZmbDebugAnimation, "NET Phase6: waiting animEnd=%d frameIdx=%d maxFrame=%d",
				_attrAnimScrbFeature->hasAnimEndCallbackFired() ? 1 : 0,
				_attrAnimScrbFeature->getLastFrameIdx(),
				_attrAnimScrbFeature->getMaxFrameIdx());
		if (_attrAnimScrbFeature->hasAnimEndCallbackFired()) {
			debugC(1, kZmbDebugAnimation, "NET Phase6 DONE: loading 7025");
			_activeAttrRunning = false;
			assignNextZmbToColumn();
			loadScrbOntoFeature(_attrAnimScrbFeature, 7025);
			if (_attrAnimScrbFeature) {
				_attrAnimScrbFeature->setFrameInterval(2);
				_activeAttrAnim1Running = true;
			}
		}
		goto label_postColumn;
	}

	// Phase 7: Attr anim 1
	if (_activeAttrAnim1Running) {
		if (getCurrentFrameCounter() % 120 == 0)
			debugC(1, kZmbDebugAnimation, "NET Phase7: waiting animEnd=%d frameIdx=%d maxFrame=%d",
				_attrAnimScrbFeature->hasAnimEndCallbackFired() ? 1 : 0,
				_attrAnimScrbFeature->getLastFrameIdx(),
				_attrAnimScrbFeature->getMaxFrameIdx());
		if (_attrAnimScrbFeature->hasAnimEndCallbackFired()) {
			debugC(1, kZmbDebugAnimation, "NET Phase7 DONE: loading 7026");
			_activeAttrAnim1Running = false;
			loadScrbOntoFeature(_attrAnimScrbFeature, 7026);
			if (_attrAnimScrbFeature) {
				_hotspotPositionFlag = 0;
				// IDA: v3->onPreRenderShapeFunc = net_remapHotspotFramesByAttr
				_attrAnimScrbFeature->setPreRenderShapeFunc(
					reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleNet::remapHotspotFramesByAttr));
				_attrAnimScrbFeature->setFrameInterval(6);
				_activeAttrAnim2Running = true;
			}
		}
		goto label_postColumn;
	}

	// Phase 8: Attr anim 2
	if (_activeAttrAnim2Running) {
		if (getCurrentFrameCounter() % 120 == 0)
			debugC(1, kZmbDebugAnimation, "NET Phase8: waiting animEnd=%d frameIdx=%d maxFrame=%d",
				_attrAnimScrbFeature->hasAnimEndCallbackFired() ? 1 : 0,
				_attrAnimScrbFeature->getLastFrameIdx(),
				_attrAnimScrbFeature->getMaxFrameIdx());
		if (_attrAnimScrbFeature->hasAnimEndCallbackFired()) {
			debugC(1, kZmbDebugAnimation, "NET Phase8 DONE: inputLocked=false");
			_inputLocked = false;
			_activeAttrAnim2Running = false;
			if (_difficultyLevel >= kPuzzleDiffLevel3) {
				_prevAttrColOffset[0] = -1;
				loadScrbOntoFeature(_attrAnimScrbFeature, 7027);
				if (_attrAnimScrbFeature) {
					_activeAttrAnim3Running = true;
					_hotspotPositionFlag = 0;
					// IDA: v4->onPreRenderShapeFunc = net_remapHotspotFramesByAttr
					_attrAnimScrbFeature->setPreRenderShapeFunc(
						reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleNet::remapHotspotFramesByAttr));
				}
				goto label_postColumn;
			}
			_activeAttrAnim3Running = false;
			if (_firstRoundFlag)
				goto label_firstRound;
			if (_noMatchFlag || _sortedZmbCount >= _loadedZmbCount)
				goto label_noMatch;
			_noMatchFlag = false;
			_submitActiveFlag = 0;
		}
		goto label_postColumn;
	}

	// Phase 9: Attr anim 3 (difficulty 2+ only)
	if (_activeAttrAnim3Running) {
		if (getCurrentFrameCounter() % 120 == 0)
			debugC(1, kZmbDebugAnimation, "NET Phase9: waiting animEnd=%d frameIdx=%d maxFrame=%d",
				_attrAnimScrbFeature->hasAnimEndCallbackFired() ? 1 : 0,
				_attrAnimScrbFeature->getLastFrameIdx(),
				_attrAnimScrbFeature->getMaxFrameIdx());
		if (_attrAnimScrbFeature->hasAnimEndCallbackFired()) {
			debugC(1, kZmbDebugAnimation, "NET Phase9 DONE");
			_activeAttrAnim3Running = false;
			if (_firstRoundFlag) {
label_firstRound:
				loadScrbOntoFeature(_feedbackScrbFeature, 10017);
				_firstRoundFlag = false;
				_lastSubmitFrame = getCurrentFrameCounter();
				goto label_postColumn;
			}
			if (_noMatchFlag || _sortedZmbCount >= _loadedZmbCount) {
label_noMatch:
				_noMatchFlag = false;
				_submitActiveFlag = 0;
				loadScrbOntoFeature(_feedbackScrbFeature, 10018);
				goto label_postColumn;
			}
			_noMatchFlag = false;
			_submitActiveFlag = 0;
		}
		goto label_postColumn;
	}

label_postColumn:
	// Phase 10: Column animation done — open column
	if (_columnAnimDone && _columnMatchCount) {
		_columnAnimDone = 0;
		if (_nextZmbToAssign >= _loadedZmbCount)
			_zmbQueueCount = 0;
		if (--_columnMatchCount <= 0) {
			_columnMatchCount = 0;
			if (_pendingZmbIndex < 0) {
				if (_zmbQueueCount)
					assignNextZmbToColumn();
			}
		}
		// Compute column index and load column SCRB
		int16 colIdx;
		if (_difficultyLevel >= kPuzzleDiffLevel3)
			colIdx = _currentSlotIndex % 25 / 5;
		else
			colIdx = _currentSlotIndex % 5;
		loadScrbOntoFeature(_columnScrbFeatures[colIdx], colIdx + 8000);
		_columnAnimColIdx = colIdx;
		_columnOpenAnimRunning = true;
		_submitCount++;
		_lastSubmitFrame = getCurrentFrameCounter();
	} else if (_columnOpenAnimRunning) {
		// Phase 11: Column open animation done → start entry
		// IDA: checks hotspot_ownerRunnerArr[net_columnOpenAnimRunner] — the specific
		// column that was loaded in Phase 10, NOT always column 0.
		if (_columnScrbFeatures[_columnAnimColIdx] && _columnScrbFeatures[_columnAnimColIdx]->hasAnimEndCallbackFired()) {
			_columnOpenAnimRunning = false;
			_activeColumnIdx = 0;
			// Find first active column slot
			for (int16 i = 0; i < 3; i++) {
				if (_columnSlotSnoidIds[i]) {
					_activeColumnIdx = i;
					break;
				}
			}
			if (_columnSlotSnoidIds[_activeColumnIdx]) {
				loadScrbOntoFeature(_entryScrbFeature, _activeColumnIdx + 8005);
				_zmbEntryAnimRunning = true;
			}
		}
	} else if (_zmbEntryAnimRunning) {
		// Phase 12: Entry animation done
		if (_entryScrbFeature->hasAnimEndCallbackFired()) {
			_zmbEntryAnimRunning = false;
			if (_columnMatchCount)
				_columnAnimDone++;
			else
				_zmbReadyCount++;
			if (_sortedZmbCount >= _loadedZmbCount && !_columnMatchCount) {
				_columnAnimDone = 0;
				_columnMatchCount = 0;
			}
		}
	} else if (_pendingZmbIndex >= 0 && !_rejectedCount && _pendingZmbIndex < _loadedZmbCount) {
		// Phase 13: Walk pending zoombini to column
		uint16 snoidId = 10000 + _pendingZmbIndex;
		ZmbSnoid *snoid = getSnoid(snoidId);
		if (snoid) {
			uint16 scrsId = 5 * (2 - _activeColumnIdx) + snoid->_trait._foot - 1 + 13001;
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
					ZmbResource(ZmbArchiveKind::kPage, scrsId));
			if (scrsStream) {
				snoid->startScrsPlayback(scrsStream, false, false);
				_activeWalkCount++;
				_walkSlotSnoidIds[_activeColumnIdx] = _columnSlotSnoidIds[_activeColumnIdx];
				_pendingZmbIndex = -1;
				_zmbWalkPending = false;
			}
		}
	}

	// Queue management
	if (_pendingZmbIndex < 0 && _zmbQueueCount)
		assignNextZmbToColumn();

	// Idle animations
	if (_loadedZmbCount > 0) {
		if (_idleAnimTrigger && _idleAnimCount < _idleAnimMax) {
			if (getCurrentFrameCounter() - _idleAnimLastFrame > 30) {
				bool triggered = false;
				int16 attempts = 0;
				_idleAnimLastFrame = getCurrentFrameCounter();

				do {
					uint16 poolIdx = _vm->_rnd->getNonRepeatRandom(_loadedZmbCount, _idleAnimPoolState);
					uint16 snoidId = 10000 + poolIdx;

					// Skip snoids in active column slots
					if (snoidId == _columnSlotSnoidIds[0] ||
						snoidId == _columnSlotSnoidIds[1] ||
						snoidId == _columnSlotSnoidIds[2]) {
						if (++attempts > 20)
							triggered = true;
						continue;
					}

					ZmbSnoid *snoid = getSnoid(snoidId);
					if (snoid && snoid->isRenderActivated() &&
						snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
						// Skip locked snoids unless round is complete
						if (snoid->_packIsOccupied && !_roundCompletedFlag) {
							if (++attempts > 20)
								triggered = true;
							continue;
						}

						uint16 scrsId = snoid->_trait._foot - 1 + 13046;
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
		} else if (_idleAnimCount >= _idleAnimMax && _idleAnimMax > 0) {
			_idleAnimPoolState = 0;
			_idleAnimLastFrame = 0;
			_idleAnimTrigger = false;
			_idleAnimCount = 0;
		}
	}

	// Submit timeout — idle feedback after inactivity
	if (_submitCount) {
		if (getCurrentFrameCounter() - _lastSubmitFrame > 720) {
			_submitActiveFlag = 0;
			_submitCount = 0;
			loadScrbOntoFeature(_feedbackScrbFeature, 10018);
			_lastSubmitFrame = getCurrentFrameCounter();
		}
	} else {
		if (getCurrentFrameCounter() - _lastSubmitFrame > 7200) {
			_lastSubmitFrame = 0;
			_submitActiveFlag = 0;
			_submitCount = 0;
			loadScrbOntoFeature(_feedbackScrbFeature, 10018);
			_lastSubmitFrame = getCurrentFrameCounter();
		}
	}

	// Bounce animation tick
	if (_bounceCounter > 0) {
		if (++_bounceCounter > 5) {
			_bounceCounter = 0;
			registerZmbAtSlot(_currentSlotIndex);
		} else {
			_bounceX -= _bounceDeltaX;
			_bounceY -= _bounceDeltaY;
		}
	}
}

} // End of namespace Mohawk
