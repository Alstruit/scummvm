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
#include "mohawk/zoombini_pages/puzzle_slides.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// =============================================================================
// Static Data Tables
// =============================================================================

// IDA: pedestal positions at 0x4A3CF8 (16 POINTS)
const Common::Point ZoombiniPuzzleSlides::kSnoidPositions[16] = {
	Common::Point(482, 127), Common::Point(428, 128), Common::Point(375, 129), Common::Point(318, 127),
	Common::Point(272, 129), Common::Point(226, 128), Common::Point(184, 127), Common::Point(140, 129),
	Common::Point( 87, 128), Common::Point(110, 170), Common::Point(122, 246), Common::Point( 84, 212),
	Common::Point(140, 327), Common::Point( 77, 293), Common::Point( 40, 157), Common::Point( 44, 232),
};

// IDA: cell center positions at 0x4A3B24 (117 POINTS)
// Grid: 13 rows x 9 columns. X decreases L-to-R (~42px), Y increases T-to-B (~18px).
// Odd rows shift left ~16px (hex grid offset).
const Common::Point ZoombiniPuzzleSlides::kCellPositions[117] = {
	// Row 0 (cells 0-8)
	Common::Point(477, 152), Common::Point(435, 152), Common::Point(393, 152), Common::Point(351, 152),
	Common::Point(309, 152), Common::Point(267, 152), Common::Point(225, 152), Common::Point(183, 152),
	Common::Point(141, 152),
	// Row 1 (cells 9-17)
	Common::Point(461, 170), Common::Point(419, 170), Common::Point(377, 170), Common::Point(335, 170),
	Common::Point(293, 170), Common::Point(251, 170), Common::Point(209, 170), Common::Point(167, 170),
	Common::Point(125, 170),
	// Row 2 (cells 18-26)
	Common::Point(487, 188), Common::Point(445, 188), Common::Point(403, 188), Common::Point(361, 188),
	Common::Point(319, 188), Common::Point(277, 188), Common::Point(235, 188), Common::Point(193, 188),
	Common::Point(151, 188),
	// Row 3 (cells 27-35)
	Common::Point(471, 206), Common::Point(429, 206), Common::Point(387, 206), Common::Point(345, 206),
	Common::Point(303, 206), Common::Point(261, 206), Common::Point(219, 206), Common::Point(177, 206),
	Common::Point(135, 206),
	// Row 4 (cells 36-44)
	Common::Point(497, 224), Common::Point(455, 224), Common::Point(413, 224), Common::Point(371, 224),
	Common::Point(329, 224), Common::Point(287, 224), Common::Point(245, 224), Common::Point(203, 224),
	Common::Point(161, 224),
	// Row 5 (cells 45-53)
	Common::Point(481, 242), Common::Point(439, 242), Common::Point(397, 242), Common::Point(355, 242),
	Common::Point(313, 242), Common::Point(271, 242), Common::Point(229, 242), Common::Point(187, 242),
	Common::Point(145, 242),
	// Row 6 (cells 54-62)
	Common::Point(507, 260), Common::Point(465, 260), Common::Point(423, 260), Common::Point(381, 260),
	Common::Point(339, 260), Common::Point(297, 260), Common::Point(255, 260), Common::Point(211, 260),
	Common::Point(171, 260),
	// Row 7 (cells 63-71)
	Common::Point(491, 278), Common::Point(449, 278), Common::Point(407, 278), Common::Point(365, 278),
	Common::Point(323, 278), Common::Point(281, 278), Common::Point(239, 278), Common::Point(197, 278),
	Common::Point(155, 278),
	// Row 8 (cells 72-80)
	Common::Point(517, 296), Common::Point(475, 296), Common::Point(433, 296), Common::Point(391, 296),
	Common::Point(349, 296), Common::Point(307, 296), Common::Point(265, 296), Common::Point(223, 296),
	Common::Point(181, 296),
	// Row 9 (cells 81-89)
	Common::Point(501, 314), Common::Point(459, 314), Common::Point(417, 314), Common::Point(375, 314),
	Common::Point(333, 314), Common::Point(291, 314), Common::Point(249, 314), Common::Point(207, 314),
	Common::Point(165, 314),
	// Row 10 (cells 90-98)
	Common::Point(527, 332), Common::Point(485, 332), Common::Point(443, 332), Common::Point(401, 332),
	Common::Point(359, 332), Common::Point(317, 332), Common::Point(275, 332), Common::Point(233, 332),
	Common::Point(191, 332),
	// Row 11 (cells 99-107)
	Common::Point(511, 350), Common::Point(469, 350), Common::Point(427, 350), Common::Point(385, 350),
	Common::Point(343, 350), Common::Point(301, 350), Common::Point(259, 350), Common::Point(217, 350),
	Common::Point(175, 350),
	// Row 12 (cells 108-116)
	Common::Point(537, 368), Common::Point(495, 368), Common::Point(453, 368), Common::Point(411, 368),
	Common::Point(369, 368), Common::Point(327, 368), Common::Point(285, 368), Common::Point(243, 368),
	Common::Point(201, 368),
};

// IDA: 0x4A3D90 - 26 primary slot cell indices (evenly spaced across grid)
const int16 ZoombiniPuzzleSlides::kSlotCellIndices[26] = {
	2, 4, 6, 19, 21, 23, 25, 38, 40, 42, 44, 55, 57, 59, 61,
	74, 76, 78, 80, 91, 93, 95, 97, 110, 112, 114
};

// IDA: 0x4A3DC4 - 43 interior/link cell indices
const int16 ZoombiniPuzzleSlides::kLinkCellIndices[43] = {
	10, 11, 12, 13, 14, 15, 28, 29, 30, 31, 32, 33, 34, 46, 47, 48, 49, 50, 51, 52,
	56, 58, 60, 64, 65, 66, 67, 68, 69, 70, 82, 83, 84, 85, 86, 87, 88, 100, 101, 102, 103, 104, 105
};

// IDA: 0x4A3E1A - 20 even-row link cells
const int16 ZoombiniPuzzleSlides::kEvenRowLinkCells[20] = {
	10, 12, 14, 29, 31, 33, 46, 48, 50, 52, 65, 67, 69, 82, 84, 86, 88, 101, 103, 105
};

// IDA: 0x4A3E42 - 20 odd-row link cells
const int16 ZoombiniPuzzleSlides::kOddRowLinkCells[20] = {
	11, 13, 15, 28, 30, 32, 34, 47, 49, 51, 64, 66, 68, 70, 83, 85, 87, 100, 102, 104
};

// IDA: 0x4A3ECC - 16 pair start offsets
const int16 ZoombiniPuzzleSlides::kPairStartOffsets[16] = {
	0, 54, 45, 36, 27, 18, 9, 0, 18, 18, 9, 9, 0, 0, 0, 0
};

// IDA: 0x4A3EE8 - 16 pair spacing values
const int16 ZoombiniPuzzleSlides::kPairSpacingArray[16] = {
	0, 0, 18, 18, 18, 18, 18, 18, 9, 9, 9, 9, 9, 9, 10, 12
};

// IDA: 0x4A3F04 - 18 left-arm link cells
const int16 ZoombiniPuzzleSlides::kLeftArmLinkCells[18] = {
	10, 12, 14, 28, 30, 32, 46, 48, 50, 64, 66, 68, 82, 84, 86, 100, 102, 104
};

// IDA: 0x4A3F28 - 18 right-arm + diagonal link cells
const int16 ZoombiniPuzzleSlides::kRightArmLinkCells[18] = {
	11, 29, 47, 65, 83, 101, 13, 31, 49, 67, 85, 103, 24, 60, 96, 19, 55, 91
};

// IDA: 0x4A3F4C - 3 left endpoint cells
const int16 ZoombiniPuzzleSlides::kLeftEndpointCells[3] = {
	18, 54, 90
};

// IDA: 0x4A3F52 - 3 right endpoint cells
const int16 ZoombiniPuzzleSlides::kRightEndpointCells[3] = {
	24, 60, 96
};

// IDA: 0x4A3F58 - 12 inner link pair cells
const int16 ZoombiniPuzzleSlides::kInnerLinkPairs[12] = {
	11, 13, 29, 31, 47, 49, 65, 67, 83, 85, 101, 103
};

// Drag constraint rect (left side where Zoombinis start)
const Common::Rect ZoombiniPuzzleSlides::kDragConstraint(0, 110, 540, 400);

// =============================================================================
// Constructor / Destructor
// =============================================================================

ZoombiniPuzzleSlides::ZoombiniPuzzleSlides(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kSlides) {
	// Initialize arrays
	memset(_cellGrid, 0, sizeof(_cellGrid));
	memset(_adjBitFlags, 0, sizeof(_adjBitFlags));
	memset(_slotCellMap, -1, sizeof(_slotCellMap));
	memset(_zmbRunnerIdxArr, 0, sizeof(_zmbRunnerIdxArr));
	memset(_sortedZmbIndices, 0, sizeof(_sortedZmbIndices));
	memset(_zmbHairAttrs, 0, sizeof(_zmbHairAttrs));
	memset(_zmbEyesAttrs, 0, sizeof(_zmbEyesAttrs));
	memset(_zmbNoseAttrs, 0, sizeof(_zmbNoseAttrs));
	memset(_zmbLegsAttrs, 0, sizeof(_zmbLegsAttrs));
	memset(_usedFlags, 0, sizeof(_usedFlags));
	memset(_pairTypeArray, 0, sizeof(_pairTypeArray));
	memset(_activeCellList, 0, sizeof(_activeCellList));
	memset(_activeCellRunnerIds, 0, sizeof(_activeCellRunnerIds));
	memset(_cellFeatures, 0, sizeof(_cellFeatures));
	memset(_layerScrbArr, 0, sizeof(_layerScrbArr));
}

ZoombiniPuzzleSlides::~ZoombiniPuzzleSlides() {
}

// =============================================================================
// Page Lifecycle
// =============================================================================

void ZoombiniPuzzleSlides::open() {
	openArchive(ZMB_MHK_SLIDES);
}

void ZoombiniPuzzleSlides::setBackgroundMusic() {
	// IDA: slides_puzzleInit (0x441f0c) has no music playback call on page load.
	// sound_activeHandle = 20078 is stored at end of funcInit for F1 replay only.
}

void ZoombiniPuzzleSlides::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniPuzzleSlides::loadFeatures() {
	// IDA: puzzleSlides_441F0C
	_difficultyLevel = static_cast<ZmbPuzzleDifficultyLevel>(_vm->_state->readActivePageRouteLevel() + 1); // 1-based (1-4)

	// IDA: slides_initGridByDifficulty (0x4468F8) — initialize grid parameters
	// Default values: slotBaseState=504, cellSpacing=48
	_slotBaseState = 504;
	_cellSpacing = 48;

	// At highest difficulty, randomize grid parameters
	// IDA: if (slides_difficultyLevel == 3) { rand(0,1) -> slotBaseState; if non-zero -> cellSpacing=24 }
	if (_difficultyLevel == kPuzzleDiffLevel4) {
		int16 randVal = _vm->_rnd->getRandomNumber(0, 1);
		_slotBaseState = 504 + randVal;
		if (randVal != 0)
			_cellSpacing = 24;
		debugC(kZmbDebugPage, "Slides Level 4: slotBaseState=%d, cellSpacing=%d",
		       _slotBaseState, _cellSpacing);
	}

	// At highest difficulty, load NODE/PATH for walking
	// IDA: if (slides_difficultyLevel == 3) node_loadNodeAndPath(0x3E8u)
	if (_difficultyLevel == kPuzzleDiffLevel4) {
		loadNODE(ZmbArchiveKind::kPage, 1000);
	}

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(0x64u)
	loadTerrainBitmap(100);

	// Preload shape images
	// IDA: shape_loadSubShapesFromArchive(&stru_4A3B20, 0x1770u) — shapes at tBMP 6000
	_vm->_gfx->preloadImage(6000);
	_vm->_gfx->preloadImage(8000);

	// Load feature groups
	// IDA: scrb_useFeatureGroup(0, 0, 7000)
	// IDA: scrb_useFeatureGroup(0, 1, 8000)

	// Load main features: 14 SCRBs at 7000
	// IDA: scrb_loadMainFeatureSet(14, 7000)
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 3, 8000) — 3 subs at 8000
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 3; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 8000), 8000 + i);
		}
	}

	// Load reject pool: 4 reject scripts at SCRS 14000
	// IDA: scrs_loadRejectPool(0, 4, 14000)
	for (uint16 i = 0; i < 4; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  14000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 6 normal scripts at SCRS 13000
	// IDA: scrs_loadNormalPool(0, 6, 13000)
	for (uint16 i = 0; i < 6; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  13000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load Zoombinis from active pack at 16 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, &stru_4A3CF8, 16)
	loadZoombinisFromPack();

	// Layout and stagger walk-in
	// IDA: zmb_layoutStaticAndWalkInGroups(0)
	layoutStaticAndWalkIn();
	assignStaggeredWalkDelays();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(6000);
	loadHelpButtonFeature();
	setGoButtonsEnabled(false);

	// The original init path consumes attribute pairings during board construction.
	snapshotZmbAttrsToArrays();
	generateAttrPairings();

	// Initialize the hex grid based on difficulty
	initGridByDifficulty();

	// Build adjacency table
	buildHexAdjacencyTable();

	// Active Slides grid cells need SCRB runners so chain logic can reload them.
	for (int16 cellIdx = 0; cellIdx < kNumCells; cellIdx++) {
		if (_cellGrid[cellIdx * kFieldsPerCell + 1] != kCellInert)
			ensureCellFeature(cellIdx);
	}

	// Refresh cached runner ids after grid setup.
	snapshotZmbAttrsToArrays();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagSlides);

	// IDA: sound_activeHandle = 20078 — slides narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20078);

	// Celebration state init (IDA: slides_puzzleInit @ 0x441F0C)
	_celebrationActive = false;
	_celebrationIndex = 0;
	// IDA: slides_celebrationTarget = slides_numZoombinis
	_celebrationTarget = _loadedZmbCount;
	_celebrationPoolState = 0;
	_celebrationLastFrame = 0;
	_matchCount = 0;
	_roundComplete = 0;
	_victoryState = 0;
	_victoryLastFrame = 0;
	_victoryNotified = false;
	_roundInitialized = 0;
	_completionAnimFeature = nullptr;
	_completionSequenceActive = false;
	_isDragging = 0;
	_activeCellCount = 0;
	memset(_activeCellList, 0, sizeof(_activeCellList));
	memset(_activeCellRunnerIds, 0, sizeof(_activeCellRunnerIds));
	debug("Slides: loadFeatures difficulty=%d loaded=%d numSlots=%d practice=%u",
	      _difficultyLevel, _loadedZmbCount, _numSlots, _vm->_state->_practiceLevel);
}

void ZoombiniPuzzleSlides::onGoButtonActivated() {
	// IDA: slides_onClickHandler case 2 -> puzzle_pendingTransitionTarget = 5 (BC2)
	// Route 2: Slides -> Basecamp2 (via Xfer)
	if (_pendingGoDepart || _completionSequenceActive)
		return;
	if (!_roundComplete)
		return;

	beginSolvedDepartureSequence();
}

void ZoombiniPuzzleSlides::executeDeparture() {
	// Locked Slides cells correspond to Zoombinis that passed the puzzle.
	// IDA clears runner+295 here via slides_markMatchedRunnersDone(); with the
	// CFeatureRunner307 layout, runner+295 = 0x30 + core259.unk00F7 (occupied flag).
	// Clear them out of the active pack before the shared container-puzzle save/export runs.
	markMatchedRunnersDone();
	ZoombiniInteractive::executeDeparture();
}

void ZoombiniPuzzleSlides::debugPrepareForDeparture() {
	_isDragging = 0;
	_roundInitialized = 0;
	_completionSequenceActive = false;
	_completionAnimFeature = nullptr;
	_victoryNotified = false;
	_activeCellCount = 0;
	memset(_activeCellList, 0, sizeof(_activeCellList));
	memset(_activeCellRunnerIds, 0, sizeof(_activeCellRunnerIds));

	for (int16 cellIdx = 0; cellIdx < kNumCells; cellIdx++) {
		int16 base = cellIdx * kFieldsPerCell;
		int16 state = _cellGrid[base + 1];

		if (state == kCellLocked || state == kCellOccupied) {
			_cellGrid[base + 1] = kCellConnector;
			_cellGrid[base + 2] = 0;
			syncCellFeatureScript(cellIdx);
		} else if (state == kCellMatched) {
			_cellGrid[base + 1] = kCellPath;
			syncCellFeatureScript(cellIdx);
		}
	}

	int16 placedCount = 0;
	for (int16 i = 0; i < _loadedZmbCount && placedCount < _numSlots; i++) {
		int16 cellIdx = _slotCellMap[placedCount];
		if (cellIdx < 0 || kNumCells <= cellIdx)
			continue;

		ZmbSnoid *snoid = getSnoid(10000 + i);
		if (!snoid)
			continue;

		moveZmbToCell(snoid, cellIdx);
		snoid->setAnimState(kSnoidAnimIdle);
		snoid->setupIdleHotspots();

		int16 base = cellIdx * kFieldsPerCell;
		_cellGrid[base + 1] = kCellLocked;
		_cellGrid[base + 2] = snoid->getId();
		_activeCellList[placedCount] = cellIdx;
		_activeCellRunnerIds[placedCount] = snoid->getId();
		_activeCellCount++;
		syncCellFeatureScript(cellIdx);
		placedCount++;
	}

	_roundComplete = (placedCount >= _loadedZmbCount);
	_victoryState = (_difficultyLevel == kPuzzleDiffLevel4 && _roundComplete) ? 1 : 0;
	if (_victoryState != 0)
		_victoryLastFrame = getCurrentFrameCounter();

	setGoButtonsEnabled(_roundComplete);
	debugC(kZmbDebugPage, "Slides: debug prepared solved board (%d/%d runners)", placedCount, _loadedZmbCount);
}

// =============================================================================
// Grid Initialization
// IDA: slides_initGridByDifficulty @ 0x4468F8
// =============================================================================

void ZoombiniPuzzleSlides::initGridByDifficulty() {
	// Initialize all cells to inert state with invalid links
	for (int16 i = 0; i < kNumCells; i++) {
		int16 base = i * kFieldsPerCell;
		_cellGrid[base + 0] = 0;     // runnerIdx
		_cellGrid[base + 1] = kCellInert;  // state
		_cellGrid[base + 2] = 0;     // data
		_cellGrid[base + 3] = -1;    // linkNW
		_cellGrid[base + 4] = -1;    // linkW
		_cellGrid[base + 5] = -1;    // linkSW
		_cellGrid[base + 6] = -1;    // linkSE
		_cellGrid[base + 7] = -1;    // linkE
		_cellGrid[base + 8] = -1;    // linkNE
	}

	// Clear adjacency flags
	memset(_adjBitFlags, 0, sizeof(_adjBitFlags));

	// Clear slot mapping
	memset(_slotCellMap, -1, sizeof(_slotCellMap));
	_numSlots = 0;

	// IDA: slides_snapshotZmbAttrsToArrays() happens before the difficulty switch.
	snapshotZmbAttrsToArrays();

	// IDA: Large switch on difficulty level (0-3, but we use 1-4)
	switch (_difficultyLevel) {
	case kPuzzleDiffLevel1:
		if (_numPairs < ARRAYSIZE(kPairStartOffsets)) {
			int16 startCell = kPairStartOffsets[_numPairs];
			int16 cellStep = kPairSpacingArray[_numPairs];

			for (int16 pairIdx = 0; pairIdx < _numPairs; pairIdx++) {
				int16 baseCell = startCell + cellStep * pairIdx;
				int16 offsetCell = 0;

				if (7 < _numPairs && (baseCell % 2) != 0) {
					_cellGrid[baseCell * kFieldsPerCell + 1] = _slotBaseState;
					_adjBitFlags[baseCell] = kAdjE;
					for (int16 cell = baseCell + 1; cell < baseCell + 4; cell++) {
						_cellGrid[cell * kFieldsPerCell + 1] = _slotBaseState;
						_adjBitFlags[cell] = kAdjW | kAdjE;
					}
					offsetCell = 3;
				}

				int16 slotBaseCell = baseCell + offsetCell;
				int16 slotCell = slotBaseCell + 1;
				_cellGrid[slotBaseCell * kFieldsPerCell + 1] = _slotBaseState;
				_cellGrid[slotCell * kFieldsPerCell + 1] = kCellConnector;
				_slotCellMap[_numSlots++] = slotCell;

				if (_pairTypeArray[pairIdx] == kCellPath) {
					_adjBitFlags[slotBaseCell] |= kAdjE;
					_adjBitFlags[slotCell] |= kAdjW;
				} else {
					int16 attrCell = slotBaseCell + 2;
					int16 endSlotCell = slotBaseCell + 3;
					_cellGrid[attrCell * kFieldsPerCell + 1] = kCellPath;
					_cellGrid[attrCell * kFieldsPerCell + 2] = _pairTypeArray[pairIdx];
					_cellGrid[endSlotCell * kFieldsPerCell + 1] = kCellConnector;
					_slotCellMap[_numSlots++] = endSlotCell;
					_adjBitFlags[slotBaseCell] |= kAdjE;
					_adjBitFlags[slotCell] = kAdjW | kAdjE;
					_adjBitFlags[attrCell] = kAdjW | kAdjE;
					_adjBitFlags[endSlotCell] |= kAdjW;
				}
			}
		}
		break;

	case kPuzzleDiffLevel2:
		buildChainSequence();
		if (_numPairs < ARRAYSIZE(kPairStartOffsets)) {
			int16 startCell = kPairStartOffsets[_numPairs];
			int16 cellStep = kPairSpacingArray[_numPairs];
			int16 pairTypeIdx = 0;
			int16 placedSlotCount = 0;

			for (int16 groupIdx = 0; groupIdx < _numPairs; groupIdx++) {
				int16 baseCell = startCell + cellStep * groupIdx;
				_cellGrid[baseCell * kFieldsPerCell + 1] = _slotBaseState;
				_cellGrid[(baseCell + 1) * kFieldsPerCell + 1] = kCellConnector;
				_slotCellMap[_numSlots++] = baseCell + 1;
				placedSlotCount++;

				if (_loadedZmbCount <= placedSlotCount) {
					_adjBitFlags[baseCell] = kAdjE;
					_adjBitFlags[baseCell + 1] = kAdjW;
					break;
				}

				if (pairTypeIdx < ARRAYSIZE(_pairTypeArray) &&
					(_pairTypeArray[pairTypeIdx] == 0 || _pairTypeArray[pairTypeIdx] == kCellPath)) {
					if (_pairTypeArray[pairTypeIdx] != 0) {
						_cellGrid[(baseCell + 2) * kFieldsPerCell + 1] = kCellPath;
						_cellGrid[(baseCell + 2) * kFieldsPerCell + 2] = 0;
						_cellGrid[(baseCell + 3) * kFieldsPerCell + 1] = kCellConnector;
						_slotCellMap[_numSlots++] = baseCell + 3;
						_adjBitFlags[baseCell] |= kAdjE;
						_adjBitFlags[baseCell + 1] = kAdjW | kAdjE;
						_adjBitFlags[baseCell + 2] = kAdjW | kAdjE;
						_adjBitFlags[baseCell + 3] |= kAdjW;
						pairTypeIdx++;
						placedSlotCount++;
						if (_loadedZmbCount <= placedSlotCount)
							break;
					}
				} else if (pairTypeIdx < ARRAYSIZE(_pairTypeArray)) {
					_cellGrid[(baseCell + 2) * kFieldsPerCell + 1] = kCellPath;
					_cellGrid[(baseCell + 2) * kFieldsPerCell + 2] = _pairTypeArray[pairTypeIdx];
					_cellGrid[(baseCell + 3) * kFieldsPerCell + 1] = kCellConnector;
					_slotCellMap[_numSlots++] = baseCell + 3;
					_adjBitFlags[baseCell] |= kAdjE;
					_adjBitFlags[baseCell + 1] = kAdjW | kAdjE;
					_adjBitFlags[baseCell + 2] = kAdjW | kAdjE;
					_adjBitFlags[baseCell + 3] |= kAdjW;
					pairTypeIdx++;
					placedSlotCount++;
					if (_loadedZmbCount <= placedSlotCount)
						break;
				}

				if (pairTypeIdx < ARRAYSIZE(_pairTypeArray) &&
					(_pairTypeArray[pairTypeIdx] == 0 || _pairTypeArray[pairTypeIdx] == kCellPath)) {
					if (_pairTypeArray[pairTypeIdx] != 0) {
						_cellGrid[(baseCell + 4) * kFieldsPerCell + 1] = kCellPath;
						_cellGrid[(baseCell + 4) * kFieldsPerCell + 2] = 0;
						_cellGrid[(baseCell + 5) * kFieldsPerCell + 1] = kCellConnector;
						_slotCellMap[_numSlots++] = baseCell + 5;
						_adjBitFlags[baseCell + 3] |= kAdjE;
						_adjBitFlags[baseCell + 4] = kAdjW | kAdjE;
						_adjBitFlags[baseCell + 5] = kAdjW;
						pairTypeIdx++;
						placedSlotCount++;
						if (_loadedZmbCount <= placedSlotCount)
							break;
					}
				} else if (pairTypeIdx < ARRAYSIZE(_pairTypeArray)) {
					_cellGrid[(baseCell + 4) * kFieldsPerCell + 1] = kCellPath;
					_cellGrid[(baseCell + 4) * kFieldsPerCell + 2] = _pairTypeArray[pairTypeIdx];
					_cellGrid[(baseCell + 5) * kFieldsPerCell + 1] = kCellConnector;
					_slotCellMap[_numSlots++] = baseCell + 5;
					_adjBitFlags[baseCell + 3] |= kAdjE;
					_adjBitFlags[baseCell + 4] = kAdjW | kAdjE;
					_adjBitFlags[baseCell + 5] = kAdjW;
					pairTypeIdx++;
					placedSlotCount++;
					if (_loadedZmbCount <= placedSlotCount)
						break;
				}
			}
		}
		break;

	case kPuzzleDiffLevel3: {
		for (int16 i = 0; i < ARRAYSIZE(kLeftEndpointCells); i++) {
			int16 cell = kLeftEndpointCells[i];
			_cellGrid[cell * kFieldsPerCell + 1] = _slotBaseState;
			_adjBitFlags[cell] = kAdjE;
			_adjBitFlags[cell + 1] = kAdjW | kAdjSW | kAdjNE;
			_adjBitFlags[cell - 8] |= kAdjSW;
			_adjBitFlags[cell + 10] |= kAdjNW;
		}

		for (int16 i = 0; i < ARRAYSIZE(kRightEndpointCells); i++) {
			int16 cell = kRightEndpointCells[i];
			_cellGrid[cell * kFieldsPerCell + 1] = kCellConnector;
			_adjBitFlags[cell] = kAdjNW | kAdjSW;
			_adjBitFlags[cell - 10] |= kAdjSE;
			_adjBitFlags[cell + 8] |= kAdjNE;
		}

		for (int16 i = 0; i < ARRAYSIZE(kLeftArmLinkCells); i++) {
			_cellGrid[kLeftArmLinkCells[i] * kFieldsPerCell + 1] = kCellPath;
			_cellGrid[kRightArmLinkCells[i] * kFieldsPerCell + 1] = kCellConnector;
		}

		for (int16 i = 0; i < ARRAYSIZE(kInnerLinkPairs); i++) {
			int16 cell = kInnerLinkPairs[i];
			_adjBitFlags[cell] |= kAdjW | kAdjE;
			_adjBitFlags[cell - 1] |= kAdjE;
			_adjBitFlags[cell + 1] |= kAdjW;
		}

		buildHexAdjacencyTable();
		sortZmbsByOverlapCount();

		if (_vm->_rnd->getRandomNumber(0, 99) >= 50) {
			if (_vm->_rnd->getRandomNumber(0, 99) >= 50) {
				int16 result = placeNextZmbInCell(91);
				result = placeNextZmbInCell(19);
				placeNextZmbInCell(55);
				(void)result;
			} else {
				int16 result = placeNextZmbInCell(55);
				result = placeNextZmbInCell(91);
				placeNextZmbInCell(19);
				(void)result;
			}
		} else {
			int16 result = placeNextZmbInCell(19);
			result = placeNextZmbInCell(55);
			placeNextZmbInCell(91);
			(void)result;
		}

		auto maybeSetMatchAttr = [this](int16 destCell, int16 cellIdx, int16 otherCellIdx) {
			int16 attr = pickRandomMatchingAttr(cellIdx, otherCellIdx);
			if (attr != 0)
				_cellGrid[destCell * kFieldsPerCell + 2] = attr;
		};
		maybeSetMatchAttr(12, 13, 11);
		maybeSetMatchAttr(30, 31, 29);
		maybeSetMatchAttr(48, 49, 47);
		maybeSetMatchAttr(66, 67, 65);
		maybeSetMatchAttr(84, 85, 83);
		maybeSetMatchAttr(102, 103, 101);

		int16 occupiedCount = 0;
		for (int16 i = 0; i < ARRAYSIZE(kRightArmLinkCells); i++) {
			if (_cellGrid[kRightArmLinkCells[i] * kFieldsPerCell + 1] == kCellOccupied)
				occupiedCount++;
		}

		if (_loadedZmbCount < occupiedCount) {
			int16 extraCount = occupiedCount - _loadedZmbCount;
			while (0 < extraCount) {
				bool removed = false;
				for (int16 i = 0; i < 6; i++) {
					int16 cell = kRightArmLinkCells[i];
					int16 base = cell * kFieldsPerCell;
					if (_cellGrid[base + 1] == kCellOccupied && _cellGrid[base - 7] == -1) {
						_cellGrid[base + 1] = kCellPath;
						_cellGrid[base + 2] = -1;
						extraCount--;
						removed = true;
						break;
					}
				}

				if (removed)
					continue;

				for (int16 i = 6; i < 12; i++) {
					int16 cell = kRightArmLinkCells[i];
					int16 base = cell * kFieldsPerCell;
					if (_cellGrid[base + 1] == kCellOccupied && _cellGrid[base + 11] == -1) {
						_cellGrid[base + 1] = kCellPath;
						_cellGrid[base + 2] = -1;
						extraCount--;
						removed = true;
						break;
					}
				}

				if (removed)
					continue;

				for (int16 i = 12; i < 15; i++) {
					int16 cell = kRightArmLinkCells[i];
					int16 base = cell * kFieldsPerCell;
					if (_cellGrid[base + 1] == kCellOccupied &&
						_cellGrid[base - 88] == -1 && _cellGrid[base + 74] == -1) {
						_cellGrid[base + 1] = kCellPath;
						_cellGrid[base + 2] = -1;
						extraCount--;
						removed = true;
						break;
					}
				}

				if (!removed)
					break;
			}
		} else {
			int16 missingCount = _loadedZmbCount - occupiedCount;
			while (0 < missingCount) {
				for (int16 i = 0; i < ARRAYSIZE(kRightArmLinkCells); i++) {
					int16 cell = kRightArmLinkCells[i];
					if (_cellGrid[cell * kFieldsPerCell + 1] == kCellOccupied)
						continue;

					_cellGrid[cell * kFieldsPerCell + 1] = kCellOccupied;
					missingCount--;
					break;
				}
			}
		}

		for (int16 i = 0; i < kNumCells; i++) {
			int16 base = i * kFieldsPerCell;
			if (_cellGrid[base + 1] == kCellConnector) {
				_cellGrid[base + 1] = kCellPath;
				_cellGrid[base + 2] = -1;
			}
		}

		_numSlots = 0;
		for (int16 i = 0; i < ARRAYSIZE(kRightArmLinkCells); i++) {
			int16 cell = kRightArmLinkCells[i];
			if (_cellGrid[cell * kFieldsPerCell + 1] == kCellOccupied)
				_slotCellMap[_numSlots++] = cell;
		}
		break;
	}

	case kPuzzleDiffLevel4: {
		auto setCellStateData = [this](int16 cellIdx, int16 state, int16 data) {
			int16 base = cellIdx * kFieldsPerCell;
			_cellGrid[base + 1] = state;
			_cellGrid[base + 2] = data;
		};
		auto clearBoard = [this]() {
			memset(_adjBitFlags, 0, sizeof(_adjBitFlags));
			for (int16 i = 0; i < kNumCells; i++) {
				int16 base = i * kFieldsPerCell;
				_cellGrid[base + 1] = kCellInert;
				_cellGrid[base + 2] = 0;
				for (int16 linkIdx = 0; linkIdx < 6; linkIdx++)
					_cellGrid[base + 3 + linkIdx] = -1;
			}
		};
		auto rebuildOccupiedSlots = [this]() {
			_numSlots = 0;
			_activeCellCount = 0;
			memset(_slotCellMap, -1, sizeof(_slotCellMap));
			memset(_activeCellList, 0, sizeof(_activeCellList));
			memset(_activeCellRunnerIds, 0, sizeof(_activeCellRunnerIds));
			for (int16 i = 0; i < ARRAYSIZE(kSlotCellIndices); i++) {
				int16 cell = kSlotCellIndices[i];
				if (_cellGrid[cell * kFieldsPerCell + 1] != kCellOccupied)
					continue;
				if (_numSlots < ARRAYSIZE(_slotCellMap))
					_slotCellMap[_numSlots++] = cell;
				if (_activeCellCount < ARRAYSIZE(_activeCellList)) {
					_activeCellList[_activeCellCount] = cell;
					_activeCellRunnerIds[_activeCellCount] = _cellGrid[cell * kFieldsPerCell + 2];
					_activeCellCount++;
				}
			}
		};

		for (int16 i = 0; i < ARRAYSIZE(kLinkCellIndices); i++)
			setCellStateData(kLinkCellIndices[i], kCellPath, 0);
		for (int16 i = 0; i < ARRAYSIZE(kEvenRowLinkCells); i++)
			_adjBitFlags[kEvenRowLinkCells[i]] = 36;
		for (int16 i = 0; i < ARRAYSIZE(kOddRowLinkCells); i++)
			_adjBitFlags[kOddRowLinkCells[i]] = 9;

		setCellStateData(54, _slotBaseState, 0);
		_adjBitFlags[54] = 16;
		_adjBitFlags[55] = 58;
		_adjBitFlags[56] = 18;
		_adjBitFlags[57] = 63;
		_adjBitFlags[58] = 18;
		_adjBitFlags[59] = 63;
		_adjBitFlags[60] = 18;
		_adjBitFlags[61] = 47;
		_adjBitFlags[19] = 40;
		_adjBitFlags[21] = 45;
		_adjBitFlags[23] = 45;
		_adjBitFlags[25] = 13;
		_adjBitFlags[38] = 45;
		_adjBitFlags[40] = 45;
		_adjBitFlags[42] = 45;
		_adjBitFlags[44] = 5;
		_adjBitFlags[74] = 45;
		_adjBitFlags[76] = 45;
		_adjBitFlags[78] = 45;
		_adjBitFlags[80] = 5;
		_adjBitFlags[91] = 40;
		_adjBitFlags[93] = 45;
		_adjBitFlags[95] = 45;
		_adjBitFlags[97] = 37;
		_adjBitFlags[2] = 12;
		_adjBitFlags[4] = 12;
		_adjBitFlags[6] = 12;
		_adjBitFlags[110] = 33;
		_adjBitFlags[112] = 33;
		_adjBitFlags[114] = 33;

		for (int16 i = 0; i < ARRAYSIZE(kSlotCellIndices); i++)
			setCellStateData(kSlotCellIndices[i], kCellConnector, 0);

		buildHexAdjacencyTable();

		if (_loadedZmbCount <= 2) {
			clearBoard();
			setCellStateData(54, _slotBaseState, 0);
			setCellStateData(55, kCellOccupied, _zmbRunnerIdxArr[0]);
			_adjBitFlags[54] = kAdjE;
			_adjBitFlags[55] = kAdjW;

			if (_loadedZmbCount == 2) {
				setCellStateData(56, kCellPath, 0);
				setCellStateData(57, kCellOccupied, _zmbRunnerIdxArr[1]);
				if (checkFirstAttrMatch(1, 0))
					_cellGrid[56 * kFieldsPerCell + 2] = _matchAttrIndex + kAttrHair;
				_adjBitFlags[55] = kAdjW | kAdjE;
				_adjBitFlags[56] = kAdjW | kAdjE;
				_adjBitFlags[57] = kAdjW;
			}

			for (int16 i = 0; i < _loadedZmbCount; i++)
				_sortedZmbIndices[i] = -1;
		} else if (_loadedZmbCount <= 5) {
			clearBoard();
			sortZmbsByOverlapCount();
			setCellStateData(54, _slotBaseState, 0);
			setCellStateData(55, kCellOccupied, _zmbRunnerIdxArr[0]);
			_adjBitFlags[54] = kAdjE;
			_adjBitFlags[55] = kAdjW | kAdjE;

			setCellStateData(56, kCellPath, 0);
			setCellStateData(57, kCellOccupied, _zmbRunnerIdxArr[1]);
			if (checkFirstAttrMatch(1, 0))
				_cellGrid[56 * kFieldsPerCell + 2] = _matchAttrIndex + kAttrHair;
			_adjBitFlags[56] = kAdjW | kAdjE;
			_adjBitFlags[57] = kAdjW;

			if (3 <= _loadedZmbCount) {
				setCellStateData(58, kCellPath, 0);
				setCellStateData(59, kCellOccupied, _zmbRunnerIdxArr[2]);
				if (checkFirstAttrMatch(2, 1))
					_cellGrid[58 * kFieldsPerCell + 2] = _matchAttrIndex + kAttrHair;
				_adjBitFlags[57] = kAdjW | kAdjE;
				_adjBitFlags[58] = kAdjW | kAdjE;
				_adjBitFlags[59] = kAdjW;
			}

			if (4 <= _loadedZmbCount) {
				setCellStateData(60, kCellPath, 0);
				setCellStateData(61, kCellOccupied, _zmbRunnerIdxArr[3]);
				if (checkFirstAttrMatch(3, 2))
					_cellGrid[60 * kFieldsPerCell + 2] = _matchAttrIndex + kAttrHair;
				_adjBitFlags[59] = kAdjW | kAdjE;
				_adjBitFlags[60] = kAdjW | kAdjE;
				_adjBitFlags[61] = kAdjW;
			}

			if (5 <= _loadedZmbCount) {
				setCellStateData(52, kCellPath, 0);
				setCellStateData(44, kCellOccupied, _zmbRunnerIdxArr[4]);
				if (checkFirstAttrMatch(4, 3))
					_cellGrid[52 * kFieldsPerCell + 2] = _matchAttrIndex + kAttrHair;
				_adjBitFlags[61] = kAdjW | kAdjNE;
				_adjBitFlags[52] = kAdjSW | kAdjNE;
				_adjBitFlags[44] = kAdjSW;
			}

			for (int16 i = 0; i < _loadedZmbCount; i++)
				_sortedZmbIndices[i] = -1;
		} else {
			assignZmbToSlot(54);
			if (hasPendingZmb())
				reassignDeadSlots();
			scanAndResetActiveCells();

			pickNextCellForLink(93, 84, 83);
			pickNextCellForLink(21, 30, 29);
			pickNextCellForLink(112, 103, 102);
			pickNextCellForLink(4, 13, 12);
			pickNextCellForLink(95, 86, 85);
			pickNextCellForLink(23, 32, 31);
			pickNextCellForLink(78, 69, 68);
			pickNextCellForLink(42, 51, 50);
			pickNextCellForLink(76, 67, 66);
			pickNextCellForLink(114, 105, 104);
			pickNextCellForLink(6, 15, 14);
			pickNextCellForLink(44, 52, 34);
			pickNextCellForLink(80, 88, 70);

			if (_cellGrid[60 * kFieldsPerCell + 1] == kCellPath &&
				_cellGrid[61 * kFieldsPerCell + 1] == kCellPath &&
				_cellGrid[69 * kFieldsPerCell + 1] == kCellInert &&
				_cellGrid[51 * kFieldsPerCell + 1] == kCellInert &&
				_cellGrid[52 * kFieldsPerCell + 1] == kCellInert &&
				_cellGrid[70 * kFieldsPerCell + 1] == kCellInert) {
				resetCellToEmpty(60);
				resetCellToEmpty(61);
			}

			triggerSwapAnimation();
		}

		rebuildOccupiedSlots();
		break;
	}

	default:
		warning("Slides: Unknown difficulty level %d", _difficultyLevel);
		break;
	}

	for (int16 i = 0; i < kNumCells; i++) {
		if (_cellGrid[i * kFieldsPerCell + 1] == kCellOccupied)
			_cellGrid[i * kFieldsPerCell + 1] = kCellConnector;
	}

	debugC(kZmbDebugPage, "Slides: initGridByDifficulty level=%d, numSlots=%d",
	       _difficultyLevel, _numSlots);
}

// =============================================================================
// Hex Adjacency Table
// IDA: slides_buildHexAdjacencyTable @ 0x4436E4
// =============================================================================

void ZoombiniPuzzleSlides::buildHexAdjacencyTable() {
	for (int16 cellIdx = 0; cellIdx < kNumCells; cellIdx++) {
		int16 base = cellIdx * kFieldsPerCell;
		for (int16 linkIdx = 0; linkIdx < 6; linkIdx++)
			_cellGrid[base + 3 + linkIdx] = -1;

		if (_cellGrid[base + 1] == kCellInert)
			continue;

		int16 row = cellIdx / 9;
		int16 col = cellIdx % 9;
		bool oddRow = (cellIdx % 18) > 8;
		uint16 adjMask = _adjBitFlags[cellIdx];
		int16 nwCell = -1;
		int16 neCell = -1;
		int16 swCell = -1;
		int16 seCell = -1;
		if (0 < row) {
			if (oddRow) {
				nwCell = cellIdx - 9;
				if (col < 8)
					neCell = cellIdx - 8;
			} else {
				if (0 < col)
					nwCell = cellIdx - 10;
				neCell = cellIdx - 9;
			}
		}

		int16 westCell = (0 < col) ? static_cast<int16>(cellIdx - 1) : -1;
		int16 eastCell = (col < 8) ? static_cast<int16>(cellIdx + 1) : -1;

		if (row < 12) {
			if (oddRow) {
				swCell = cellIdx + 9;
				if (col < 8)
					seCell = cellIdx + 10;
			} else {
				if (0 < col)
					swCell = cellIdx + 8;
				seCell = cellIdx + 9;
			}
		}

		// Unported setup branches still rely on the old geometry-derived adjacency.
		if (adjMask == 0) {
			if (0 <= nwCell && _cellGrid[nwCell * kFieldsPerCell + 1] != kCellInert)
				adjMask |= kAdjNW;
			if (0 <= neCell && _cellGrid[neCell * kFieldsPerCell + 1] != kCellInert)
				adjMask |= kAdjNE;
			if (0 <= westCell && _cellGrid[westCell * kFieldsPerCell + 1] != kCellInert)
				adjMask |= kAdjW;
			if (0 <= eastCell && _cellGrid[eastCell * kFieldsPerCell + 1] != kCellInert)
				adjMask |= kAdjE;
			if (0 <= swCell && _cellGrid[swCell * kFieldsPerCell + 1] != kCellInert)
				adjMask |= kAdjSW;
			if (0 <= seCell && _cellGrid[seCell * kFieldsPerCell + 1] != kCellInert)
				adjMask |= kAdjSE;

			_adjBitFlags[cellIdx] = adjMask;
		}

		auto setLinkIfValid = [&](uint16 bit, int16 fieldOffset, int16 neighborCell) {
			if ((adjMask & bit) == 0)
				return;
			if (neighborCell < 0 || kNumCells <= neighborCell)
				return;
			if (_cellGrid[neighborCell * kFieldsPerCell + 1] == kCellInert)
				return;
			_cellGrid[base + fieldOffset] = neighborCell;
		};

		setLinkIfValid(kAdjNW, 3, nwCell);
		setLinkIfValid(kAdjW, 4, westCell);
		setLinkIfValid(kAdjSW, 5, swCell);
		setLinkIfValid(kAdjSE, 6, seCell);
		setLinkIfValid(kAdjE, 7, eastCell);
		setLinkIfValid(kAdjNE, 8, neCell);
	}

	debugC(kZmbDebugPage, "Slides: buildHexAdjacencyTable complete");
}

// =============================================================================
// Attribute Snapshot
// IDA: slides_snapshotZmbAttrsToArrays @ 0x444EE7
// =============================================================================

void ZoombiniPuzzleSlides::snapshotZmbAttrsToArrays() {
	// Copy each loaded Zoombini's attributes into per-type arrays
	for (int16 i = 0; i < _loadedZmbCount && i < 16; i++) {
		ZmbSnoid *snoid = getSnoid(10000 + i);
		if (!snoid)
			continue;

		_zmbHairAttrs[i] = snoid->_trait._head;
		_zmbEyesAttrs[i] = snoid->_trait._eye;
		_zmbNoseAttrs[i] = snoid->_trait._nose;
		_zmbLegsAttrs[i] = snoid->_trait._foot;
		_zmbRunnerIdxArr[i] = snoid->getId();
	}
}

// =============================================================================
// Attribute Pairing
// IDA: slides_generateAttrPairings @ 0x44485A
// =============================================================================

void ZoombiniPuzzleSlides::generateAttrPairings() {
	int16 hairAttrs[16] = {};
	int16 eyeAttrs[16] = {};
	int16 noseAttrs[16] = {};
	int16 legAttrs[16] = {};
	int16 pairUsed[16] = {};

	for (int16 i = 0; i < _loadedZmbCount; i++) {
		hairAttrs[i] = _zmbHairAttrs[i];
		eyeAttrs[i] = _zmbEyesAttrs[i];
		noseAttrs[i] = _zmbNoseAttrs[i];
		legAttrs[i] = _zmbLegsAttrs[i];
	}

	int16 seedAttr = _vm->_rnd->getRandomNumber(0, 3);
	for (int16 attempt = 0; attempt < 10; attempt++) {
		memset(_pairTypeArray, 0, sizeof(_pairTypeArray));
		memset(pairUsed, 0, sizeof(pairUsed));
		_numPairs = 0;
		int16 unpairedCount = 0;
		int16 attrCursor = seedAttr;

		for (int16 i = 0; i < _loadedZmbCount; i++) {
			if (pairUsed[i] != 0)
				continue;

			int16 tries = 4;
			while (tries > 0 && pairUsed[i] == 0) {
				attrCursor++;
				if (attrCursor > 3)
					attrCursor = 0;

				for (int16 j = i + 1; j < _loadedZmbCount; j++) {
					if (pairUsed[j] != 0)
						continue;

					bool matched = false;
					switch (attrCursor) {
					case 0:
						matched = (hairAttrs[i] == hairAttrs[j]);
						break;
					case 1:
						matched = (eyeAttrs[i] == eyeAttrs[j]);
						break;
					case 2:
						matched = (noseAttrs[i] == noseAttrs[j]);
						break;
					case 3:
						matched = (legAttrs[i] == legAttrs[j]);
						break;
					default:
						break;
					}

					if (!matched)
						continue;

					_pairTypeArray[_numPairs++] = kAttrHair + attrCursor;
					pairUsed[j] = 1;
					pairUsed[i] = 1;
					break;
				}

				tries--;
				if (tries == 0 && pairUsed[i] == 0) {
					pairUsed[i] = 99;
					_pairTypeArray[_numPairs++] = kCellPath;
					unpairedCount++;
				}
			}
		}

		if (unpairedCount == 0 || (unpairedCount == 1 && (_loadedZmbCount % 2) == 1))
			break;

		for (int16 i = _loadedZmbCount - 1; i >= 0; i--) {
			if (pairUsed[i] != 99 || pairUsed[0] == 99)
				continue;

			int16 tmp = hairAttrs[i];
			hairAttrs[i] = hairAttrs[0];
			hairAttrs[0] = tmp;

			tmp = eyeAttrs[i];
			eyeAttrs[i] = eyeAttrs[0];
			eyeAttrs[0] = tmp;

			tmp = noseAttrs[i];
			noseAttrs[i] = noseAttrs[0];
			noseAttrs[0] = tmp;

			tmp = legAttrs[i];
			legAttrs[i] = legAttrs[0];
			legAttrs[0] = tmp;
		}
	}

	memset(_usedFlags, 0, sizeof(_usedFlags));

	debugC(kZmbDebugPage, "Slides: generateAttrPairings numPairs=%d", _numPairs);
}

// =============================================================================
// Per-Frame Update
// IDA: slides_puzzleHoverUpdate @ 0x4427B7
// =============================================================================

void ZoombiniPuzzleSlides::onEveryFrame() {
	if (_loadedZmbCount <= 0)
		return;
	if (_pendingGoDepart)
		return;
	if (_completionSequenceActive && isSolvedDepartureSequenceActive()) {
		finishSolvedDepartureSequence();
		return;
	}

	if (_victoryState != 0) {
		if (!_victoryNotified) {
			showNotiBox(Common::U32String(U"you entered"), true);
			_victoryNotified = true;
		}

		// IDA: slides_rotatePaletteEntries_443F9C rotates palette entries 243..245.
		if (6 < getCurrentFrameCounter() - _victoryLastFrame) {
			_vm->_gfx->rotatePaletteRight(243, 3);
			_victoryLastFrame = getCurrentFrameCounter();
		}
	}

	// Celebration scheduling.
	// IDA pattern: re-enter every tick while a match is queued and target not
	// reached. The 30-frame timer paces individual SCRS plays. Previously the
	// `_celebrationActive` flag latched after the first fire and blocked all
	// subsequent celebrations until target was met — but target only advances
	// when celebrations actually play, causing a stall.
	if (!_matchCount || _celebrationIndex >= _celebrationTarget) {
		if (_celebrationIndex >= _celebrationTarget) {
			_celebrationPoolState = 0;
			_celebrationLastFrame = 0;
			_matchCount = 0;
			_celebrationIndex = 0;
			_celebrationActive = false;
		}
	} else {
		debugC(1, kZmbDebugAnimation, "Slides: celebration tick, matchCount=%d, idx=%d/%d",
		       _matchCount, _celebrationIndex, _celebrationTarget);
		_celebrationActive = true;
		if (getCurrentFrameCounter() - _celebrationLastFrame > 30) {
			_celebrationLastFrame = getCurrentFrameCounter();
			bool triggered = false;
			int16 attempts = 0;

			do {
				uint16 poolIdx = _vm->_rnd->getNonRepeatRandom(_loadedZmbCount, _celebrationPoolState);
				uint16 snoidId = 10000 + poolIdx;
				ZmbSnoid *snoid = getSnoid(snoidId);

				if (snoid && snoid->isRenderActivated() &&
					snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
					// IDA: snoidScript_initAndPlay(0, 0, byte_239 - 1 + 13001, core)
					uint16 scrsId = snoid->_trait._foot - 1 + 13001;
					Common::SeekableReadStream *scrsStream =
						_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
							ZmbResource(ZmbArchiveKind::kPage, scrsId));
					if (scrsStream) {
						snoid->startScrsPlayback(scrsStream, false, true);
						_celebrationIndex++;
						triggered = true;
					}
				} else if (++attempts > 20) {
					triggered = true;
				}
			} while (!triggered);
		}
	}
}

void ZoombiniPuzzleSlides::beginSolvedDepartureSequence() {
	if (_completionSequenceActive || !_roundComplete || _roundInitialized != 0)
		return;

	_departXferSrcSiPage = ZMB_SI_SLIDES_09;
	_roundInitialized = 1;
	_completionSequenceActive = true;
	_completionAnimFeature = nullptr;

	if (_matchCount)
		_celebrationActive = true;

	for (int16 cellIdx = 0; cellIdx < kNumCells; cellIdx++) {
		int16 state = _cellGrid[cellIdx * kFieldsPerCell + 1];
		if (state == kCellMatched || state == kCellLocked || state == _slotBaseState) {
			ensureCellFeature(cellIdx);
			ZmbFeature *cellFeature = _cellFeatures[cellIdx];
			if (cellFeature) {
				setCellFeaturePreRenderHook(cellFeature, 7002);
				loadScrbOntoFeature(cellFeature, 7002, true);
				cellFeature->activateAnimate();
				if (_completionAnimFeature == nullptr)
					_completionAnimFeature = cellFeature;
			}
		}

		if (state == kCellLocked) {
			ZmbSnoid *snoid = getSnoid(_cellGrid[cellIdx * kFieldsPerCell + 2]);
			if (!snoid)
				continue;

			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
				                 ZmbResource(ZmbArchiveKind::kPage, 13000));
			if (scrsStream)
				snoid->startScrsPlayback(scrsStream, false, false);
		}
	}

	unlockInteractiveSlots();
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, 7000));
	debugC(kZmbDebugPage, "Slides: began solved departure sequence");
}


bool ZoombiniPuzzleSlides::isSolvedDepartureSequenceActive() const {
	if (!_completionSequenceActive || !_completionAnimFeature)
		return false;

	// IDA gates this handoff on hotspot_ownerRunnerArr[slides_animHotspotId] == 0.
	// ScummVM's generic SCRB renderer falls back from empty frame groups to the last
	// non-empty hotspot group, so draw-record absence is not a reliable proxy here.
	// SCRB 7002 frame 1 is genuinely empty in the original data, so gate on the exact
	// selected frame group becoming empty after animation startup.
	int32 frameIdx = _completionAnimFeature->getLastFrameIdx();
	ZmbHotspotGroup *hsGroup = _completionAnimFeature->getHotspotGroupExact(frameIdx);
	return frameIdx > 0 && hsGroup != nullptr && hsGroup->getHotspotCount() == 0;
}

void ZoombiniPuzzleSlides::finishSolvedDepartureSequence() {
	_completionSequenceActive = false;
	_completionAnimFeature = nullptr;

	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, 7001));
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 996));

	if (_difficultyLevel <= kPuzzleDiffLevel2)
		startDepartWalkAnimation(Common::Point(1280, 240));
	else
		startDepartWalkAnimation(Common::Point(800, 200));

	ZoombiniInteractive::onGoButtonActivated();
	debugC(kZmbDebugPage, "Slides: solved departure sequence complete");
}

// =============================================================================
// Animation Event Handling
// IDA: slides_snoidTravelCallback @ 0x4462BC
// =============================================================================

void ZoombiniPuzzleSlides::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (!feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
		return;

	ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);

	if (eventCode == 0) {
		// Toggle render visibility + apply pending body arrangement.
		// IDA: *(runnerData+290) = *(runnerData+290)==0; if word_4B110E: apply & clear.
		if (snoid->isRenderActivated())
			snoid->deactivateRender();
		else
			snoid->activateRender();

		if (_pendingBodyArrangement != 0) {
			snoid->setBodyArrangement(_pendingBodyArrangement - 1);
			_pendingBodyArrangement = 0;
		}
	} else if (eventCode >= 90 && eventCode <= 93) {
		// Directional travel animations.
		// IDA: events 90-93 initiate SCRS 14000-14003 (left/right/up/down)
		// on the active travel snoid with re-set callback.
		if (_activeTravelSnoidId == 0)
			return;

		ZmbSnoid *travelSnoid = getSnoid(_activeTravelSnoidId);
		if (!travelSnoid)
			return;

		int16 scrsId = 14000 + (eventCode - 90);
		Common::SeekableReadStream *scrsStream =
			_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
							 ZmbResource(ZmbArchiveKind::kPage, scrsId));
		if (scrsStream) {
			travelSnoid->startScrsPlayback(scrsStream, false, false);
			// IDA: events 90-92 set word_4B1112=1 (traveling), event 93 sets 0 (arrived)
			_travelState = (eventCode == 93) ? 0 : 1;
			debug(3, "Slides: Travel SCRS %d on snoid %d", scrsId, _activeTravelSnoidId);
		}
	} else if (eventCode >= kZmbAnimEvent240_BodyArrangePendFirst && eventCode <= kZmbAnimEvent243_BodyArrangePendLast) {
		// Pending body arrangement (applied on next event 0).
		// IDA: word_4B110E = travelIdx - 239 (range 1-4)
		_pendingBodyArrangement = eventCode - (kZmbAnimEvent240_BodyArrangePendFirst - 1);
	} else if (eventCode >= kZmbAnimEvent250_BodyArrangeDirectFirst && eventCode <= kZmbAnimEvent253_BodyArrangeDirectLast) {
		// Direct body arrangement change.
		// IDA: zmb_setBodyLayerShapes(travelIdx - 250, core)
		snoid->setBodyArrangement(eventCode - kZmbAnimEvent250_BodyArrangeDirectFirst);
	}
}

// =============================================================================
// Zoombini Loading
// =============================================================================

void ZoombiniPuzzleSlides::loadZoombinisFromPack() {
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

// =============================================================================
// Input Handling
// IDA: slides_onClickHandler @ 0x442891
// =============================================================================

ZmbEventHandleResult ZoombiniPuzzleSlides::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// In sticky mouse mode, a second click ends the drag
	if (_isDragging && _vm->_state->getEnableStickyMouse()) {
		endDrag(absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	// Let the base class handle button clicks first
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Guard: already dragging
	if (_isDragging)
		return ZmbEventHandleResult::kPassthrough;

	// Find snoid at click position
	ZmbSnoid *snoid = findSnoidAtPoint(absPos);
	if (!snoid)
		return ZmbEventHandleResult::kPassthrough;

	// Don't drag snoids that are playing scripts
	SnoidAnimState state = snoid->getAnimState();
	if (state == kSnoidAnimScriptReject || state == kSnoidAnimScriptNormal)
		return ZmbEventHandleResult::kPassthrough;

	// Begin drag
	startSnoidDrag(snoid, absPos);
	_isDragging = 1;

	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniPuzzleSlides::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	if (!_isDragging) {
		return ZoombiniInteractive::onLButtonUp(absPos, relPos);
	}

	// In sticky mouse mode, button-up does NOT end drag
	if (_vm->_state->getEnableStickyMouse())
		return ZmbEventHandleResult::kConsumed;

	endDrag(absPos);

	return ZmbEventHandleResult::kConsumed;
}

void ZoombiniPuzzleSlides::endDrag(const Common::Point &dropPos) {
	ZmbSnoid *snoid = finishSnoidDrag();
	_isDragging = 0;

	if (!snoid)
		return;

	// Check if dropped on a valid cell
	Common::Point snoidPos = snoid->getPointLoc();
	int16 targetCell = findCellAtPosition(snoidPos);

	if (targetCell >= 0 && isCellValidDropTarget(targetCell)) {
		// Valid drop: assign Zoombini to slot
		assignZmbToSlot(snoid, targetCell);
	} else {
		// Invalid drop: validate terrain and return if needed
		if (!validateTerrainDrop(snoid)) {
			snoid->setPointLoc(_dragOrigPos);
		}
		snoid->setAnimState(kSnoidAnimIdle);
		snoid->setupIdleHotspots();
	}
}

int16 ZoombiniPuzzleSlides::findCellAtPosition(const Common::Point &pos) const {
	// Find the cell whose center is closest to the given position
	for (int16 i = 0; i < kNumCells; i++) {
		if (_cellGrid[i * kFieldsPerCell + 1] == kCellInert)
			continue;

		const Common::Point &cellPos = kCellPositions[i];
		int16 dx = pos.x - cellPos.x;
		int16 dy = pos.y - cellPos.y;
		int16 distSq = dx * dx + dy * dy;

		if (distSq <= kCellHitRadius * kCellHitRadius)
			return i;
	}
	return -1;
}

bool ZoombiniPuzzleSlides::isCellValidDropTarget(int16 cellIdx) const {
	if (cellIdx < 0 || cellIdx >= kNumCells)
		return false;

	int16 state = _cellGrid[cellIdx * kFieldsPerCell + 1];

	// Accept slot base cells (504/505) and connector cells (506)
	return (state == kCellSlotBase1 || state == kCellSlotBase2 || state == kCellConnector);
}

void ZoombiniPuzzleSlides::assignZmbToSlot(ZmbSnoid *snoid, int16 cellIdx) {
	// IDA: slides_assignZmbToSlot @ 0x447FF9
	int16 base = cellIdx * kFieldsPerCell;

	// Move snoid to cell position
	snoid->setPointLoc(kCellPositions[cellIdx]);
	snoid->setAnimState(kSnoidAnimIdle);
	snoid->setupIdleHotspots();

	// Update cell state
	_cellGrid[base + 1] = kCellOccupied;
	_cellGrid[base + 2] = snoid->getId(); // Store runner ID in data field
	syncCellFeatureScript(cellIdx);

	debugC(kZmbDebugPage, "Slides: Assigned snoid %d to cell %d", snoid->getId(), cellIdx);

	// Check for attribute matches
	int16 matchResult = validateChainAndMarkMatched(cellIdx);
	if (matchResult > 0) {
		_matchCount += matchResult;
		debugC(kZmbDebugPage, "Slides: Found %d matches, total matchCount=%d", matchResult, _matchCount);
	}
}

int16 ZoombiniPuzzleSlides::assignZmbToSlot(int16 slotBaseCell) {
	// IDA: slides_assignZmbToSlot @ 0x447FF9
	static const int16 kCascadeCells[12] = {
		40, 76, 23, 95, 42, 78, 38, 74, 21, 93, 19, 91
	};

	sortZmbsByOverlapCount();
	int16 slotCell = slotBaseCell + 1;
	_cellGrid[slotCell * kFieldsPerCell + 1] = kCellOccupied;

	if (_loadedZmbCount == 1) {
		_cellGrid[slotCell * kFieldsPerCell + 2] = _zmbRunnerIdxArr[0];
		_sortedZmbIndices[0] = -1;
		return _zmbRunnerIdxArr[0];
	}

	for (int16 i = 0; i < _loadedZmbCount; i++) {
		if (i + 1 >= _loadedZmbCount)
			break;
		if (!checkFirstAttrMatch(i + 1, i))
			continue;

		int16 sortedIdx = _sortedZmbIndices[i + 1];
		if (sortedIdx >= 0) {
			_cellGrid[slotCell * kFieldsPerCell + 2] = _zmbRunnerIdxArr[sortedIdx];
			_sortedZmbIndices[i + 1] = -1;
		}
		break;
	}

	int16 result = moveZmbToCell(slotCell);
	if (result != 0) {
		if (result != -1)
			result = moveZmbToCell(result);
	} else {
		int16 nextCell = slotCell + 2;
		result = moveZmbToCell(nextCell);
		if (result == 0)
			result = moveZmbToCell(nextCell + 2);
	}

	for (uint i = 0; i < ARRAYSIZE(kCascadeCells) && hasPendingZmb(); i++)
		moveZmbToCell(kCascadeCells[i]);

	return result;
}

void ZoombiniPuzzleSlides::moveZmbToCell(ZmbSnoid *snoid, int16 cellIdx) {
	// IDA: slides_moveZmbToCell @ 0x4481FE
	snoid->setPointLoc(kCellPositions[cellIdx]);
}

int16 ZoombiniPuzzleSlides::moveZmbToCell(int16 moveData) {
	// IDA: slides_moveZmb_4481FE
	if (!hasPendingZmb())
		return -1;

	auto getLink = [this](int16 cellIdx, int16 dir) -> int16 {
		if (cellIdx < 0 || cellIdx >= kNumCells || dir < 0 || dir >= 6)
			return -1;
		return _cellGrid[cellIdx * kFieldsPerCell + 3 + dir];
	};

	if (_cellGrid[moveData * kFieldsPerCell + 1] != kCellOccupied) {
		int16 farNE = getLink(getLink(moveData, 5), 5);
		int16 farSE = getLink(getLink(moveData, 3), 3);
		if (farNE < 0 || _cellGrid[farNE * kFieldsPerCell + 1] != kCellOccupied) {
			if (farSE < 0 || _cellGrid[farSE * kFieldsPerCell + 1] != kCellOccupied)
				return -1;
			if (findMatchingZmbForCell(farSE, 0) == -1)
				return -1;
		} else if (findMatchingZmbForCell(farNE, 2) == -1) {
			return -1;
		}
	}

	int16 farNE = getLink(getLink(moveData, 5), 5);
	int16 farSE = getLink(getLink(moveData, 3), 3);

	if ((moveData == 55 || moveData == 57 || moveData == 59) && findMatchingZmbForCell(moveData, 4) == -1)
		return -1;

	int16 nextCell = moveData + 2;
	if (farNE >= 0 && _cellGrid[farNE * kFieldsPerCell + 1] == kCellConnector) {
		if (!hasPendingZmb())
			return -1;
		if (findMatchingZmbForCell(moveData, 5) == -1) {
			_cellGrid[farNE * kFieldsPerCell + 1] = kCellPath;
			return nextCell;
		}
	}

	if (nextCell < kNumCells && _cellGrid[nextCell * kFieldsPerCell + 1] == kCellOccupied && farNE >= 0) {
		int16 attr = pickRandomMatchingAttr(nextCell, farNE);
		if (attr != 0) {
			int16 middleCell = getLink(farNE, 3);
			if (middleCell >= 0)
				_cellGrid[middleCell * kFieldsPerCell + 2] = attr;
		}
	}

	if (farSE >= 0 && _cellGrid[farSE * kFieldsPerCell + 1] == kCellConnector) {
		if (!hasPendingZmb())
			return -1;
		if (findMatchingZmbForCell(moveData, 3) == -1) {
			_cellGrid[farSE * kFieldsPerCell + 1] = kCellPath;
			return nextCell;
		}
		if (nextCell < kNumCells && _cellGrid[nextCell * kFieldsPerCell + 1] == kCellOccupied) {
			int16 attr = pickRandomMatchingAttr(nextCell, farSE);
			if (attr != 0) {
				int16 middleCell = getLink(farSE, 5);
				if (middleCell >= 0)
					_cellGrid[middleCell * kFieldsPerCell + 2] = attr;
			}
		}
	}

	return 0;
}

void ZoombiniPuzzleSlides::clearCellToEmpty(int16 cellIdx) {
	// IDA: slides_clearCellToEmpty @ 0x448955
	int16 base = cellIdx * kFieldsPerCell;
	_cellGrid[base + 1] = kCellConnector;
	_cellGrid[base + 2] = 0;
	syncCellFeatureScript(cellIdx);
}

void ZoombiniPuzzleSlides::resetCellToEmpty(int16 cellIdx) {
	// IDA: slides_resetCellToEmpty @ 0x4496BC
	int16 base = cellIdx * kFieldsPerCell;
	_cellGrid[base + 1] = kCellInert;
	_cellGrid[base + 2] = 0;

	// Clear all link fields and corresponding adjacency bits
	for (int16 i = 0; i < 6; i++) {
		int16 neighborCell = _cellGrid[base + 3 + i];
		if (neighborCell >= 0 && neighborCell < kNumCells) {
			// Clear the reverse bit on the neighbor
			uint16 reverseBit = 0;
			switch (i) {
			case 0: reverseBit = kAdjSE; break; // NW -> SE
			case 1: reverseBit = kAdjE;  break; // W -> E
			case 2: reverseBit = kAdjNE; break; // SW -> NE
			case 3: reverseBit = kAdjNW; break; // SE -> NW
			case 4: reverseBit = kAdjW;  break; // E -> W
			case 5: reverseBit = kAdjSW; break; // NE -> SW
			default: break;
			}
			_adjBitFlags[neighborCell] &= ~reverseBit;
		}
		_cellGrid[base + 3 + i] = -1;
	}

	_adjBitFlags[cellIdx] = 0;
	syncCellFeatureScript(cellIdx);
}

void ZoombiniPuzzleSlides::clearCellLinkBits(uint16 bitMask, int16 linkField, int16 cellIdx) {
	// IDA: slides_clearCellLinkBits @ 0x449048
	if (cellIdx < 0 || cellIdx >= kNumCells || linkField < 0 || linkField >= 6)
		return;

	_cellGrid[cellIdx * kFieldsPerCell + 3 + linkField] = -1;
	_adjBitFlags[cellIdx] &= ~bitMask;
}

void ZoombiniPuzzleSlides::updateNeighborFlags(int16 cellIdx) {
	// IDA: slides_updateNeighborFlags @ 0x449171
	// Refresh adjacency bits based on current cell states
	int16 base = cellIdx * kFieldsPerCell;

	for (int16 i = 0; i < 6; i++) {
		int16 neighborCell = _cellGrid[base + 3 + i];
		if (neighborCell >= 0 && neighborCell < kNumCells) {
			if (_cellGrid[neighborCell * kFieldsPerCell + 1] != kCellInert) {
				_adjBitFlags[cellIdx] |= (1 << i);
			} else {
				_adjBitFlags[cellIdx] &= ~(1 << i);
			}
		}
	}
}

// =============================================================================
// Chain Building and Matching
// IDA: slides_validateChainAndMarkMatched @ 0x4442A9
// =============================================================================

int16 ZoombiniPuzzleSlides::validateChainAndMarkMatched(int16 startCellIdx) {
	if (startCellIdx < 0 || startCellIdx >= kNumCells)
		return 0;

	int16 matchCount = 0;
	int16 currentCell = startCellIdx;

	if (setCellStateAndReload(startCellIdx, kCellOccupied))
		matchCount++;

	int16 backwardCell = getBackwardChainLink(startCellIdx);
	if (backwardCell >= 0 && _cellGrid[backwardCell * kFieldsPerCell + 1] != _slotBaseState)
		currentCell = backwardCell;

	bool blocked = false;
	while (!blocked && currentCell >= 0) {
		int16 base = currentCell * kFieldsPerCell;
		int16 state = _cellGrid[base + 1];

		if (state == kCellInert || state == kCellConnector) {
			blocked = true;
		} else if (state != kCellPath) {
			if (state == kCellOccupied && setCellStateAndReload(currentCell, kCellLocked))
				matchCount++;
		} else {
			int16 attrType = _cellGrid[base + 2];
			if (attrType >= kAttrHair && attrType <= kAttrLegs) {
				int16 forwardCell = getForwardChainLink(currentCell);
				int16 backwardMatchCell = getBackwardChainLink(currentCell);

				if (!cellStateIs(forwardCell, kCellOccupied, kCellLocked) ||
					!cellStateIs(backwardMatchCell, kCellOccupied, kCellLocked)) {
					blocked = true;
				} else if (cellsMatchAttr(backwardMatchCell, forwardCell, attrType)) {
					if (setCellStateAndReload(currentCell, kCellMatched))
						matchCount++;
				} else {
					blocked = true;
				}
			} else if (_difficultyLevel == kPuzzleDiffLevel2 && attrType == 0 && currentCell > 0 &&
				_cellGrid[(currentCell - 1) * kFieldsPerCell + 1] == kCellLocked) {
				if (setCellStateAndReload(currentCell, kCellMatched))
					matchCount++;
			}
		}

		if (!blocked) {
			currentCell = getForwardChainLink(currentCell);
			blocked = (currentCell < 0);
		}
	}

	return matchCount;
}

void ZoombiniPuzzleSlides::buildChainSequence() {
	// IDA: slides_buildChainSequence @ 0x444C16
	memset(_pairTypeArray, 0, sizeof(_pairTypeArray));
	memset(_usedFlags, 0, sizeof(_usedFlags));
	_numPairs = _loadedZmbCount / 3;
	if ((_loadedZmbCount % 3) != 0)
		_numPairs++;

	int16 pairTypeIdx = 0;
	int16 runnerIdx = 0;
	for (int16 groupIdx = 0; groupIdx < _numPairs; groupIdx++) {
		_usedFlags[runnerIdx] = 1;
		int16 nextRunnerIdx = findRunnerByMatchingAttr(runnerIdx);
		if (nextRunnerIdx == -1) {
			_pairTypeArray[pairTypeIdx++] = kCellPath;
			for (int16 i = 1; i < _loadedZmbCount; i++) {
				if (_usedFlags[i] != 0)
					continue;
				_usedFlags[i] = 1;
				nextRunnerIdx = i;
				break;
			}
		} else {
			_usedFlags[nextRunnerIdx] = 1;
			_pairTypeArray[pairTypeIdx++] = _matchAttrIndex + kAttrHair;
		}

		runnerIdx = nextRunnerIdx;
		int16 remainingRunnerIdx = -1;
		for (int16 i = 1; i < _loadedZmbCount; i++) {
			if (_usedFlags[i] == 0)
				remainingRunnerIdx = i;
		}
		if (remainingRunnerIdx == -1)
			break;

		int16 lastRunnerIdx = findRunnerByMatchingAttr(runnerIdx);
		if (lastRunnerIdx == -1) {
			_pairTypeArray[pairTypeIdx++] = kCellPath;
			for (int16 i = 1; i < _loadedZmbCount; i++) {
				if (_usedFlags[i] == 0)
					lastRunnerIdx = i;
			}
		} else {
			_usedFlags[lastRunnerIdx] = 1;
			_pairTypeArray[pairTypeIdx++] = _matchAttrIndex + kAttrHair;
		}

		runnerIdx = lastRunnerIdx;
		_usedFlags[lastRunnerIdx] = 1;
		remainingRunnerIdx = -1;
		for (int16 i = 1; i < _loadedZmbCount; i++) {
			if (_usedFlags[i] == 0)
				remainingRunnerIdx = i;
		}
		if (remainingRunnerIdx == -1)
			break;
		runnerIdx = remainingRunnerIdx;
	}
}

int16 ZoombiniPuzzleSlides::findRunnerInState508() const {
	// IDA: slides_findRunnerInState508 @ 0x44481C
	for (int16 i = 0; i < _numSlots; i++) {
		int16 cellIdx = _slotCellMap[i];
		if (cellIdx >= 0 && _cellGrid[cellIdx * kFieldsPerCell + 1] == kCellLocked) {
			return _cellGrid[cellIdx * kFieldsPerCell + 2];
		}
	}
	return -1;
}

int16 ZoombiniPuzzleSlides::findRunnerByMatchingAttr(int16 runnerIdx) {
	// IDA: slides_findRunnerByMatchingAttr @ 0x444DC8
	_matchAttrIndex = _vm->_rnd->getRandomNumber(0, 3);
	for (int16 tries = 4; 0 < tries; tries--) {
		_matchAttrIndex++;
		if (3 < _matchAttrIndex)
			_matchAttrIndex = 0;

		for (int16 i = 0; i < _loadedZmbCount; i++) {
			if (i == runnerIdx || _usedFlags[i] != 0)
				continue;

			if ((_matchAttrIndex == 0 && _zmbHairAttrs[runnerIdx] == _zmbHairAttrs[i]) ||
				(_matchAttrIndex == 1 && _zmbEyesAttrs[runnerIdx] == _zmbEyesAttrs[i]) ||
				(_matchAttrIndex == 2 && _zmbNoseAttrs[runnerIdx] == _zmbNoseAttrs[i]) ||
				(_matchAttrIndex == 3 && _zmbLegsAttrs[runnerIdx] == _zmbLegsAttrs[i])) {
				return i;
			}
		}
	}

	return -1;
}

void ZoombiniPuzzleSlides::sortZmbsByOverlapCount() {
	// IDA: slides_sortZmbsByOverlapCount @ 0x444FBF
	int16 overlapCounts[16] = {};

	for (int16 i = 0; i < _loadedZmbCount; i++) {
		for (int16 j = 0; j < _loadedZmbCount; j++) {
			if (_zmbHairAttrs[i] == _zmbHairAttrs[j] ||
				_zmbEyesAttrs[i] == _zmbEyesAttrs[j] ||
				_zmbNoseAttrs[i] == _zmbNoseAttrs[j] ||
				_zmbLegsAttrs[i] == _zmbLegsAttrs[j]) {
				overlapCounts[i]++;
			}
		}
	}

	for (int16 sortedIdx = 0; sortedIdx < _loadedZmbCount; sortedIdx++) {
		int16 bestRunnerIdx = 0;
		int16 bestOverlap = -1;

		for (int16 runnerIdx = 0; runnerIdx < _loadedZmbCount; runnerIdx++) {
			if (bestOverlap < overlapCounts[runnerIdx]) {
				bestOverlap = overlapCounts[runnerIdx];
				bestRunnerIdx = runnerIdx;
			}
		}

		_sortedZmbIndices[sortedIdx] = bestRunnerIdx;
		overlapCounts[bestRunnerIdx] = -1;
	}
}

int16 ZoombiniPuzzleSlides::placeMatchingZmbInCell(int16 matchCellIdx, int16 outSlot) {
	// IDA: slides_placeMatchingZmbInCell @ 0x4450A3
	auto getLink = [this](int16 cellIdx, int16 dir) -> int16 {
		if (cellIdx < 0 || cellIdx >= kNumCells || dir < 0 || dir >= 6)
			return -1;
		return _cellGrid[cellIdx * kFieldsPerCell + 3 + dir];
	};

	int16 midCell = -1;
	int16 destCell = -1;
	if (5 < outSlot) {
		switch (outSlot) {
		case 6:
			midCell = getLink(matchCellIdx, 0);
			destCell = getLink(midCell, 1);
			break;
		case 7:
			midCell = getLink(matchCellIdx, 2);
			destCell = getLink(midCell, 1);
			break;
		case 8:
			midCell = getLink(matchCellIdx, 5);
			destCell = getLink(midCell, 4);
			break;
		case 9:
			midCell = getLink(matchCellIdx, 3);
			destCell = getLink(midCell, 4);
			break;
		default:
			break;
		}
	} else {
		midCell = getLink(matchCellIdx, outSlot);
		destCell = getLink(midCell, outSlot);
	}

	if (midCell < 0 || destCell < 0)
		return -1;

	ZmbSnoid *sourceSnoid = getSnoid(_cellGrid[matchCellIdx * kFieldsPerCell + 2]);
	if (!sourceSnoid)
		return -1;

	int16 attrCursor = _vm->_rnd->getRandomNumber(0, 3);
	for (int16 sortedIdx = _loadedZmbCount - 1; 0 <= sortedIdx; --sortedIdx) {
		int16 runnerListIdx = _sortedZmbIndices[sortedIdx];
		if (runnerListIdx == -1)
			continue;

		ZmbSnoid *candidateSnoid = getSnoid(_zmbRunnerIdxArr[runnerListIdx]);
		if (!candidateSnoid)
			continue;

		bool noMatch = true;
		int16 tries = 4;
		while (noMatch && 0 < tries) {
			if ((attrCursor == 0 && candidateSnoid->_trait._head == sourceSnoid->_trait._head) ||
				(attrCursor == 1 && candidateSnoid->_trait._eye == sourceSnoid->_trait._eye) ||
				(attrCursor == 2 && candidateSnoid->_trait._nose == sourceSnoid->_trait._nose) ||
				(attrCursor == 3 && candidateSnoid->_trait._foot == sourceSnoid->_trait._foot)) {
				noMatch = false;
			} else {
				tries--;
				attrCursor++;
				if (3 < attrCursor)
					attrCursor = 0;
			}
		}

		if (!noMatch) {
			_cellGrid[destCell * kFieldsPerCell + 1] = kCellOccupied;
			_cellGrid[destCell * kFieldsPerCell + 2] = candidateSnoid->getId();
			_cellGrid[midCell * kFieldsPerCell + 1] = kCellPath;
			_cellGrid[midCell * kFieldsPerCell + 2] = attrCursor + kAttrHair;
			_sortedZmbIndices[sortedIdx] = -1;
			syncCellFeatureScript(destCell);
			syncCellFeatureScript(midCell);
			return sortedIdx;
		}
	}

	return -1;
}

int16 ZoombiniPuzzleSlides::pickRandomMatchingAttr(int16 cellIdx, int16 otherCellIdx) const {
	// IDA: slides_pickRandomMatchingAttr @ 0x44533D
	if (!cellStateIs(cellIdx, kCellOccupied, kCellLocked) || !cellStateIs(otherCellIdx, kCellOccupied, kCellLocked))
		return 0;

	int16 leftRunnerId = _cellGrid[cellIdx * kFieldsPerCell + 2];
	int16 rightRunnerId = _cellGrid[otherCellIdx * kFieldsPerCell + 2];
	if (leftRunnerId == 0 || rightRunnerId == 0)
		return 0;

	ZmbSnoid *leftSnoid = getSnoid(leftRunnerId);
	ZmbSnoid *rightSnoid = getSnoid(rightRunnerId);
	if (!leftSnoid || !rightSnoid)
		return 0;

	int16 roll = _vm->_rnd->getRandomNumber(0, 999);
	if (roll < 250) {
		if (leftSnoid->_trait._head == rightSnoid->_trait._head)
			return kAttrHair;
		if (leftSnoid->_trait._eye == rightSnoid->_trait._eye)
			return kAttrEyes;
		if (leftSnoid->_trait._nose == rightSnoid->_trait._nose)
			return kAttrNose;
		if (leftSnoid->_trait._foot == rightSnoid->_trait._foot)
			return kAttrLegs;
	} else if (roll < 500) {
		if (leftSnoid->_trait._eye == rightSnoid->_trait._eye)
			return kAttrEyes;
		if (leftSnoid->_trait._nose == rightSnoid->_trait._nose)
			return kAttrNose;
		if (leftSnoid->_trait._foot == rightSnoid->_trait._foot)
			return kAttrLegs;
		if (leftSnoid->_trait._head == rightSnoid->_trait._head)
			return kAttrHair;
	} else if (roll < 750) {
		if (leftSnoid->_trait._nose == rightSnoid->_trait._nose)
			return kAttrNose;
		if (leftSnoid->_trait._foot == rightSnoid->_trait._foot)
			return kAttrLegs;
		if (leftSnoid->_trait._head == rightSnoid->_trait._head)
			return kAttrHair;
		if (leftSnoid->_trait._eye == rightSnoid->_trait._eye)
			return kAttrEyes;
	} else {
		if (leftSnoid->_trait._foot == rightSnoid->_trait._foot)
			return kAttrLegs;
		if (leftSnoid->_trait._head == rightSnoid->_trait._head)
			return kAttrHair;
		if (leftSnoid->_trait._eye == rightSnoid->_trait._eye)
			return kAttrEyes;
		if (leftSnoid->_trait._nose == rightSnoid->_trait._nose)
			return kAttrNose;
	}

	return 0;
}

void ZoombiniPuzzleSlides::activateChainLink(int16 linkIdx) {
	// IDA: slides_activateChainLink @ 0x445527
	static const int16 kChainCells[12] = { 55, 57, 59, 61, 38, 74, 40, 76, 42, 78, 44, 80 };
	int16 cellIdx = linkIdx + 1;

	if (!cellStateIs(cellIdx, kCellOccupied))
		return;

	setCellStateAndReload(cellIdx, kCellLocked);

	for (uint i = 0; i < ARRAYSIZE(kChainCells); i++) {
		int16 chainCell = kChainCells[i];
		if (!cellStateIs(chainCell, kCellOccupied))
			continue;

		for (int16 dir = 0; dir < 6; dir++) {
			int16 neighborCell = _cellGrid[chainCell * kFieldsPerCell + 3 + dir];
			if (cellStateIs(neighborCell, kCellMatched)) {
				setCellStateAndReload(chainCell, kCellLocked);
				break;
			}
		}
	}

	for (uint i = 0; i < ARRAYSIZE(kChainCells); i++) {
		int16 chainCell = kChainCells[i];
		if (cellStateIs(chainCell, kCellMatched, kCellLocked))
			evalNeighborStates(chainCell);
	}

	checkVictoryCondition();

	if (_difficultyLevel == kPuzzleDiffLevel4 &&
		cellStateIs(57, kCellLocked) && cellStateIs(59, kCellLocked) && cellStateIs(61, kCellLocked)) {
		int16 occupiedSlots = 0;
		for (int16 i = 0; i < _numSlots; i++) {
			int16 slotCell = _slotCellMap[i];
			if (cellStateIs(slotCell, kCellOccupied, kCellLocked))
				occupiedSlots++;
		}

		if (_victoryState == 0 && occupiedSlots == 4) {
			_victoryState = 1;
			_victoryLastFrame = getCurrentFrameCounter();
			_victoryNotified = false;
			_roundComplete = 1;
		} else if (occupiedSlots != 4) {
			_victoryState = 0;
			_victoryNotified = false;
		}
	}
}

void ZoombiniPuzzleSlides::confirmEndpointMatches() {
	// IDA: slides_confirmEndpointMatches @ 0x445700
	static const int16 kEndpointCells[3] = { 19, 55, 91 };

	for (int16 i = 0; i < 3; i++) {
		int16 endpointCell = kEndpointCells[i];
		if (!cellStateIs(endpointCell, kCellOccupied))
			continue;

		setCellStateAndReload(endpointCell, kCellLocked);
		propagateMatchChain(endpointCell);
	}

	checkVictoryCondition();
}

bool ZoombiniPuzzleSlides::checkFirstAttrMatch(int16 leftSortedIdx, int16 rightSortedIdx) {
	// IDA: slides_checkFirstAttrMatch @ 0x448119
	if (leftSortedIdx < 0 || rightSortedIdx < 0 || leftSortedIdx >= _loadedZmbCount || rightSortedIdx >= _loadedZmbCount)
		return false;
	int16 leftRunnerIdx = _sortedZmbIndices[leftSortedIdx];
	int16 rightRunnerIdx = _sortedZmbIndices[rightSortedIdx];
	if (leftRunnerIdx < 0 || rightRunnerIdx < 0)
		return false;

	if (_zmbHairAttrs[leftRunnerIdx] == _zmbHairAttrs[rightRunnerIdx]) {
		_matchAttrIndex = 0;
		return true;
	}
	if (_zmbEyesAttrs[leftRunnerIdx] == _zmbEyesAttrs[rightRunnerIdx]) {
		_matchAttrIndex = 1;
		return true;
	}
	if (_zmbNoseAttrs[leftRunnerIdx] == _zmbNoseAttrs[rightRunnerIdx]) {
		_matchAttrIndex = 2;
		return true;
	}
	if (_zmbLegsAttrs[leftRunnerIdx] == _zmbLegsAttrs[rightRunnerIdx]) {
		_matchAttrIndex = 3;
		return true;
	}

	return false;
}

void ZoombiniPuzzleSlides::evalAttrMatchAndAdvance(int16 leadCellIdx, int16 middleCellIdx, int16 tailCellIdx) {
	// IDA: slides_evalAttrMatchAndAdvance @ 0x445A1B
	if (middleCellIdx < 0 || !cellStateIs(middleCellIdx, kCellPath))
		return;

	int16 middleBase = middleCellIdx * kFieldsPerCell;
	int16 attrType = _cellGrid[middleBase + 2];

	if (attrType >= kAttrHair) {
		if (leadCellIdx >= 0 && cellStateIs(tailCellIdx, kCellOccupied, kCellLocked) &&
			cellStateIs(leadCellIdx, kCellOccupied, kCellLocked)) {
			if (cellStateIs(middleCellIdx, kCellMatched) || cellsMatchAttr(tailCellIdx, leadCellIdx, attrType)) {
				setCellStateAndReload(leadCellIdx, kCellLocked);
				setCellStateAndReload(middleCellIdx, kCellMatched);
			}
		} else if (leadCellIdx >= 0 && cellStateIs(leadCellIdx, kCellMatched, kCellPath)) {
			setCellStateAndReload(leadCellIdx, kCellMatched);
		}
	} else if (cellStateIs(tailCellIdx, kCellOccupied, kCellLocked, kCellMatched)) {
		setCellStateAndReload(middleCellIdx, kCellMatched);
		if (leadCellIdx >= 0) {
			if (cellStateIs(leadCellIdx, kCellOccupied, kCellLocked)) {
				setCellStateAndReload(leadCellIdx, kCellLocked);
			} else if (cellStateIs(leadCellIdx, kCellPath)) {
				setCellStateAndReload(leadCellIdx, kCellMatched);
			}
		}
	}
}

void ZoombiniPuzzleSlides::evalNeighborStates(int16 cellIdx) {
	// IDA: slides_evalNeighborStates @ 0x445880
	if (cellIdx < 0 || cellIdx >= kNumCells)
		return;

	int16 westCell = _cellGrid[cellIdx * kFieldsPerCell + 7];
	if (westCell >= 0)
		evalAttrMatchAndAdvance(_cellGrid[westCell * kFieldsPerCell + 7], westCell, cellIdx);

	int16 linkCell = _cellGrid[cellIdx * kFieldsPerCell + 4];
	if (linkCell >= 0)
		evalAttrMatchAndAdvance(_cellGrid[linkCell * kFieldsPerCell + 4], linkCell, cellIdx);

	int16 northEastCell = _cellGrid[cellIdx * kFieldsPerCell + 8];
	if (northEastCell >= 0) {
		int16 leadCell = _cellGrid[northEastCell * kFieldsPerCell + 8];
		evalAttrMatchAndAdvance(leadCell, northEastCell, cellIdx);
		if (cellStateIs(leadCell, kCellLocked, kCellMatched)) {
			int16 innerLead = _cellGrid[leadCell * kFieldsPerCell + 6];
			if (innerLead >= 0)
				evalAttrMatchAndAdvance(_cellGrid[innerLead * kFieldsPerCell + 6], innerLead, leadCell);

			int16 branchCell = _cellGrid[leadCell * kFieldsPerCell + 3];
			if (branchCell >= 0)
				evalAttrMatchAndAdvance(_cellGrid[branchCell * kFieldsPerCell + 3], branchCell, leadCell);
		}
	}

	int16 southWestCell = _cellGrid[cellIdx * kFieldsPerCell + 6];
	if (southWestCell >= 0) {
		int16 leadCell = _cellGrid[southWestCell * kFieldsPerCell + 6];
		evalAttrMatchAndAdvance(leadCell, southWestCell, cellIdx);
		if (cellStateIs(leadCell, kCellLocked, kCellMatched)) {
			int16 innerLead = _cellGrid[leadCell * kFieldsPerCell + 8];
			if (innerLead >= 0)
				evalAttrMatchAndAdvance(_cellGrid[innerLead * kFieldsPerCell + 8], innerLead, leadCell);

			int16 branchCell = _cellGrid[leadCell * kFieldsPerCell + 5];
			if (branchCell >= 0)
				evalAttrMatchAndAdvance(_cellGrid[branchCell * kFieldsPerCell + 5], branchCell, leadCell);
		}
	}

	int16 northWestCell = _cellGrid[cellIdx * kFieldsPerCell + 3];
	if (northWestCell >= 0)
		evalAttrMatchAndAdvance(_cellGrid[northWestCell * kFieldsPerCell + 3], northWestCell, cellIdx);

	int16 eastCell = _cellGrid[cellIdx * kFieldsPerCell + 5];
	if (eastCell >= 0)
		evalAttrMatchAndAdvance(_cellGrid[eastCell * kFieldsPerCell + 5], eastCell, cellIdx);
}

void ZoombiniPuzzleSlides::propagateMatchChain(int16 chainIdx) {
	// IDA: slides_propagateMatchChain @ 0x446073
	if (chainIdx < 0 || chainIdx >= kNumCells)
		return;

	int16 linkCell = _cellGrid[chainIdx * kFieldsPerCell + 8];
	if (linkCell >= 0) {
		int16 leadCell = _cellGrid[linkCell * kFieldsPerCell + 7];
		evalAttrMatchAndAdvance(leadCell, linkCell, chainIdx);
		if (cellStateIs(leadCell, kCellLocked, kCellMatched)) {
			int16 outerCell = _cellGrid[leadCell * kFieldsPerCell + 7];
			int16 outerLead = (outerCell >= 0) ? _cellGrid[outerCell * kFieldsPerCell + 7] : -1;
			evalAttrMatchAndAdvance(outerLead, outerCell, leadCell);
			if (cellStateIs(outerLead, kCellLocked, kCellMatched)) {
				int16 cornerCell = _cellGrid[outerLead * kFieldsPerCell + 7];
				int16 cornerLead = (cornerCell >= 0) ? _cellGrid[cornerCell * kFieldsPerCell + 6] : -1;
				evalAttrMatchAndAdvance(cornerLead, cornerCell, outerLead);
				if (cellStateIs(cornerLead, kCellLocked, kCellMatched)) {
					int16 branchCell = _cellGrid[cornerLead * kFieldsPerCell + 5];
					int16 branchLead = (branchCell >= 0) ? _cellGrid[branchCell * kFieldsPerCell + 4] : -1;
					evalAttrMatchAndAdvance(branchLead, branchCell, cornerLead);
					if (cellStateIs(branchLead, kCellLocked, kCellMatched)) {
						int16 lastCell = _cellGrid[branchLead * kFieldsPerCell + 4];
						if (lastCell >= 0)
							evalAttrMatchAndAdvance(_cellGrid[lastCell * kFieldsPerCell + 4], lastCell, branchLead);
					}
				}
			}
		}
	}

	linkCell = _cellGrid[chainIdx * kFieldsPerCell + 6];
	if (linkCell >= 0) {
		int16 leadCell = _cellGrid[linkCell * kFieldsPerCell + 7];
		evalAttrMatchAndAdvance(leadCell, linkCell, chainIdx);
		if (cellStateIs(leadCell, kCellLocked, kCellMatched)) {
			int16 outerCell = _cellGrid[leadCell * kFieldsPerCell + 7];
			int16 outerLead = (outerCell >= 0) ? _cellGrid[outerCell * kFieldsPerCell + 7] : -1;
			evalAttrMatchAndAdvance(outerLead, outerCell, leadCell);
			if (cellStateIs(outerLead, kCellLocked, kCellMatched)) {
				int16 cornerCell = _cellGrid[outerLead * kFieldsPerCell + 7];
				int16 cornerLead = (cornerCell >= 0) ? _cellGrid[cornerCell * kFieldsPerCell + 8] : -1;
				evalAttrMatchAndAdvance(cornerLead, cornerCell, outerLead);
				if (cellStateIs(cornerLead, kCellLocked, kCellMatched)) {
					int16 branchCell = _cellGrid[cornerLead * kFieldsPerCell + 3];
					int16 branchLead = (branchCell >= 0) ? _cellGrid[branchCell * kFieldsPerCell + 4] : -1;
					evalAttrMatchAndAdvance(branchLead, branchCell, cornerLead);
					if (cellStateIs(branchLead, kCellLocked, kCellMatched)) {
						int16 lastCell = _cellGrid[branchLead * kFieldsPerCell + 4];
						if (lastCell >= 0)
							evalAttrMatchAndAdvance(_cellGrid[lastCell * kFieldsPerCell + 4], lastCell, branchLead);
					}
				}
			}
		}
	}
}

int16 ZoombiniPuzzleSlides::checkAttrMatchOutcome(int16 leftSortedIdx, int16 rightSortedIdx) {
	// IDA: slides_checkAttrMatchOutcome @ 0x448D1C
	return checkFirstAttrMatch(leftSortedIdx, rightSortedIdx) ? 1 : 0;
}

// =============================================================================
// Animation and Travel
// =============================================================================

void ZoombiniPuzzleSlides::resetAnimStates() {
	// IDA: slides_resetAnimStates @ 0x4457C9
	for (int16 i = 0; i < _loadedZmbCount; i++) {
		ZmbSnoid *snoid = getSnoid(10000 + i);
		if (snoid) {
			snoid->setAnimState(kSnoidAnimIdle);
		}
	}
}

void ZoombiniPuzzleSlides::beginZmbTravel(ZmbSnoid *snoid, int16 targetCell) {
	// IDA: slides_beginZmbTravel @ 0x4464A3
	if (!snoid)
		return;

	_activeTravelSnoidId = snoid->getId();
	_travelState = 1;

	// Start travel animation toward target
	Common::Point targetPos = kCellPositions[targetCell];
	snoid->initWalkToTarget(targetPos);
}

void ZoombiniPuzzleSlides::updateWaterLevelSFX() {
	// IDA slides_updateWaterLevelSFX @ 0x44664B:
	// Plays a 4-tier audio feedback based on the current-vs-target water level:
	//   SFX 8500 — low (empty bucket / bubbling)
	//   SFX 8501 — approaching target (rising)
	//   SFX 8502 — at target (filled)
	//   SFX 8504 — slight overflow (warning)
	//   SFX 8505 — full overflow (round complete)
	// The "water level" in ScummVM maps to the match count relative to the
	// loaded zoombini count (target = _loadedZmbCount for a full solve).
	// Without IDA's `_prevWaterLevel`/`_currWaterLevel` tracking, we pace the
	// SFX via match-count delta so the feedback fires once per filling stage.
	if (_loadedZmbCount <= 0)
		return;
	const int16 curr = _matchCount;
	const int16 target = _loadedZmbCount;
	const int16 prev = _prevWaterLevelSFX;
	if (curr == prev)
		return;

	uint16 sfxId = 0;
	if (curr >= target + 2) {
		sfxId = 8505; // full overflow
		_roundComplete = true;
	} else if (curr > target) {
		sfxId = 8504; // warning overflow
	} else if (curr == target) {
		sfxId = 8502; // at target — filled
	} else if (curr >= (target * 2) / 3) {
		sfxId = 8501; // approaching target
	} else {
		sfxId = 8500; // low
	}

	if (sfxId != 0)
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, sfxId),
			Audio::Mixer::kSFXSoundType);
	_prevWaterLevelSFX = curr;
}

void ZoombiniPuzzleSlides::triggerSwapAnimation() {
	// IDA: slides_triggerSwapAnimation @ 0x449509
	debugC(kZmbDebugPage, "Slides: replaying %d active swap cells", _activeCellCount);

	for (int16 i = 0; i < _activeCellCount; i++) {
		int16 cellIdx = _activeCellList[i];
		int16 runnerId = _activeCellRunnerIds[i];

		if (cellIdx < 0 || cellIdx >= kNumCells || runnerId <= 0)
			continue;

		ZmbSnoid *snoid = getSnoid(runnerId);
		if (!snoid)
			continue;

		Common::Point targetPos = kCellPositions[cellIdx];
		if (_cellFeatures[cellIdx])
			targetPos = _cellFeatures[cellIdx]->getPointLoc();

		snoid->initWalkToTarget(targetPos);
		_cellGrid[cellIdx * kFieldsPerCell + 1] = kCellOccupied;
		_cellGrid[cellIdx * kFieldsPerCell + 2] = runnerId;
		syncCellFeatureScript(cellIdx);
	}
}

void ZoombiniPuzzleSlides::loadRunnerSCRB(uint16 runnerId, int16 scrbId) {
	// IDA: slides_loadRunnerSCRB @ 0x44BA68
	for (int16 cellIdx = 0; cellIdx < kNumCells; cellIdx++) {
		if (_cellGrid[cellIdx * kFieldsPerCell + 0] != static_cast<int16>(runnerId) || !_cellFeatures[cellIdx])
			continue;

		setCellFeaturePreRenderHook(_cellFeatures[cellIdx], scrbId);
		loadScrbOntoFeature(_cellFeatures[cellIdx], scrbId);
		return;
	}

	ZmbSnoid *snoid = getSnoid(runnerId);
	if (snoid) {
		loadScrbOntoFeature(snoid, scrbId);
	}
}

// =============================================================================
// Slot Management
// =============================================================================

void ZoombiniPuzzleSlides::unlockInteractiveSlots() {
	// IDA: slides_unlockInteractiveSlots @ 0x445E20
	// The original helper unlocks interaction and relinks slot/unplaced runners for
	// linked-list ordering. ScummVM does not use runner-link ordering for Slides cell
	// features, so this must not rewrite cell states or reload SCRBs.
}

int16 ZoombiniPuzzleSlides::placeNextZmbInCell(int16 cellIdx) {
	// IDA: slides_placeNextZmbInCell @ 0x445F75
	int16 result = cellIdx * kFieldsPerCell;

	for (int16 i = 0; i < _loadedZmbCount; i++) {
		if (_sortedZmbIndices[i] == -1)
			continue;

		int16 runnerId = _zmbRunnerIdxArr[_sortedZmbIndices[i]];
		_cellGrid[cellIdx * kFieldsPerCell + 1] = kCellOccupied;
		_cellGrid[cellIdx * kFieldsPerCell + 2] = runnerId;
		_sortedZmbIndices[i] = -1;
		syncCellFeatureScript(cellIdx);
		break;
	}

	for (int16 j = 0; j < _loadedZmbCount; j++) {
		if (_sortedZmbIndices[j] == -1)
			continue;

		int16 pairedCell = cellIdx + 5;
		if (pairedCell < kNumCells) {
			int16 runnerId = _zmbRunnerIdxArr[_sortedZmbIndices[j]];
			_cellGrid[pairedCell * kFieldsPerCell + 1] = kCellOccupied;
			_cellGrid[pairedCell * kFieldsPerCell + 2] = runnerId;
			_sortedZmbIndices[j] = -1;
			syncCellFeatureScript(pairedCell);
		}
		break;
	}

	if (cellStateIs(cellIdx, kCellOccupied)) {
		int16 matched = placeMatchingZmbInCell(cellIdx, 8);
		if (matched != -1)
			placeMatchingZmbInCell(cellIdx, 9);
	}

	if (cellIdx + 5 < kNumCells && cellStateIs(cellIdx + 5, kCellOccupied)) {
		result = placeMatchingZmbInCell(cellIdx + 5, 6);
		if (result != -1)
			result = placeMatchingZmbInCell(cellIdx + 5, 7);
	}

	return result;
}

bool ZoombiniPuzzleSlides::hasPendingZmb() const {
	// IDA: slides_hasPendingZmb @ 0x4484AA
	for (int16 i = 0; i < _loadedZmbCount; i++) {
		if (_sortedZmbIndices[i] != -1)
			return true;
	}
	return false;
}

void ZoombiniPuzzleSlides::scanAndResetActiveCells() {
	// IDA: slides_scanAndResetActiveCells @ 0x4484CF
	_activeCellCount = 0;
	for (int16 cellIdx = 0; cellIdx < kNumCells; cellIdx++) {
		int16 &state = _cellGrid[cellIdx * kFieldsPerCell + 1];
		if (state == kCellConnector) {
			state = kCellPath;
			syncCellFeatureScript(cellIdx);
		}

		if (state == kCellOccupied && _activeCellCount < ARRAYSIZE(_activeCellList)) {
			_activeCellList[_activeCellCount++] = cellIdx;
			_activeCellRunnerIds[_activeCellCount - 1] = _cellGrid[cellIdx * kFieldsPerCell + 2];
		}
	}
	debugC(kZmbDebugPage, "Slides: scanAndResetActiveCells collected %d occupied cells", _activeCellCount);

	auto cellState = [this](int16 cellIdx) -> int16 {
		return _cellGrid[cellIdx * kFieldsPerCell + 1];
	};

	if (cellState(2) == kCellPath && cellState(19) == kCellPath) {
		clearCellToEmpty(2);
		clearCellToEmpty(19);
		clearCellToEmpty(10);
		clearCellToEmpty(11);
		clearCellToEmpty(28);
		clearCellLinkBits(kAdjNW, 0, 38);
		clearCellLinkBits(kAdjNW, 0, 21);
	}

	if (cellState(91) == kCellPath && cellState(110) == kCellPath) {
		clearCellToEmpty(91);
		clearCellToEmpty(110);
		clearCellToEmpty(100);
		clearCellToEmpty(82);
		clearCellToEmpty(101);
		clearCellLinkBits(kAdjSW, 2, 74);
		clearCellLinkBits(kAdjSW, 2, 93);
	}

	if (cellState(112) == kCellPath) {
		clearCellToEmpty(112);
		clearCellToEmpty(102);
		clearCellToEmpty(103);
		clearCellLinkBits(kAdjSE, 3, 93);
		clearCellLinkBits(kAdjSW, 2, 95);
	}

	if (cellState(114) == kCellPath) {
		clearCellToEmpty(114);
		clearCellToEmpty(104);
		clearCellToEmpty(105);
		clearCellLinkBits(kAdjSE, 3, 95);
		clearCellLinkBits(kAdjSW, 2, 97);
	}

	if (cellState(4) == kCellPath) {
		clearCellToEmpty(4);
		clearCellToEmpty(12);
		clearCellToEmpty(13);
		clearCellLinkBits(kAdjNE, 5, 21);
		clearCellLinkBits(kAdjNW, 0, 23);
	}

	if (cellState(6) == kCellPath) {
		clearCellToEmpty(6);
		clearCellToEmpty(14);
		clearCellToEmpty(15);
		clearCellLinkBits(kAdjNE, 5, 23);
		clearCellLinkBits(kAdjNW, 0, 25);
	}

	if (cellState(97) == kCellPath && cellState(80) == kCellPath) {
		clearCellToEmpty(97);
		clearCellToEmpty(80);
		clearCellToEmpty(88);
		clearCellToEmpty(87);
		clearCellToEmpty(70);
		clearCellToEmpty(105);
		clearCellLinkBits(kAdjSE, 3, 78);
		clearCellLinkBits(kAdjSE, 3, 61);
		clearCellLinkBits(kAdjNE, 5, 114);
	}

	if (cellState(25) == kCellPath && cellState(44) == kCellPath) {
		clearCellToEmpty(25);
		clearCellToEmpty(44);
		clearCellToEmpty(34);
		clearCellToEmpty(15);
		clearCellToEmpty(33);
		clearCellToEmpty(52);
		clearCellLinkBits(kAdjNE, 5, 42);
		clearCellLinkBits(kAdjNE, 5, 61);
		clearCellLinkBits(kAdjSE, 3, 6);
	}
}

int16 ZoombiniPuzzleSlides::findMatchingZmbForCell(int16 matchCellIdx, int16 outResult) {
	// IDA: slides_findMatchingZmbForCell @ 0x448760
	auto getLink = [this](int16 cellIdx, int16 dir) -> int16 {
		if (cellIdx < 0 || cellIdx >= kNumCells || dir < 0 || dir >= 6)
			return -1;
		return _cellGrid[cellIdx * kFieldsPerCell + 3 + dir];
	};

	int16 midCell = getLink(matchCellIdx, outResult);
	if (midCell < 0)
		return -2;

	int16 destCell = getLink(midCell, outResult);
	if (destCell < 0)
		return -2;

	ZmbSnoid *sourceSnoid = getSnoid(_cellGrid[matchCellIdx * kFieldsPerCell + 2]);
	if (!sourceSnoid)
		return -1;

	int16 attrCursor = _vm->_rnd->getRandomNumber(0, 3);
	for (int16 i = 0; i < _loadedZmbCount; i++) {
		int16 runnerListIdx = _sortedZmbIndices[i];
		if (runnerListIdx == -1)
			continue;

		ZmbSnoid *candidateSnoid = getSnoid(_zmbRunnerIdxArr[runnerListIdx]);
		if (!candidateSnoid)
			continue;

		bool noMatch = true;
		int16 tries = 4;
		while (noMatch && 0 < tries) {
			if ((attrCursor == 0 && candidateSnoid->_trait._head == sourceSnoid->_trait._head) ||
				(attrCursor == 1 && candidateSnoid->_trait._eye == sourceSnoid->_trait._eye) ||
				(attrCursor == 2 && candidateSnoid->_trait._nose == sourceSnoid->_trait._nose) ||
				(attrCursor == 3 && candidateSnoid->_trait._foot == sourceSnoid->_trait._foot)) {
				noMatch = false;
			} else {
				tries--;
				attrCursor++;
				if (3 < attrCursor)
					attrCursor = 0;
			}
		}

		if (!noMatch) {
			_cellGrid[destCell * kFieldsPerCell + 1] = kCellOccupied;
			_cellGrid[destCell * kFieldsPerCell + 2] = candidateSnoid->getId();
			_cellGrid[midCell * kFieldsPerCell + 1] = kCellPath;
			_cellGrid[midCell * kFieldsPerCell + 2] = attrCursor + kAttrHair;
			_sortedZmbIndices[i] = -1;
			syncCellFeatureScript(destCell);
			syncCellFeatureScript(midCell);
			return i;
		}
	}

	return -1;
}

void ZoombiniPuzzleSlides::reassignDeadSlots() {
	// IDA: slides_reassignDeadSlots @ 0x44899D
	static const int16 kReassignCells[22] = {
		55, 57, 59, 61, 38, 74, 40, 76, 42, 78, 44, 80, 21, 93, 23, 95, 25, 97, 4, 112, 19, 25
	};

	for (uint i = 0; i < ARRAYSIZE(kReassignCells); i++) {
		int16 cellIdx = kReassignCells[i];
		if (cellStateIs(cellIdx, kCellOccupied))
			continue;

		for (int16 sortedIdx = 0; sortedIdx < _loadedZmbCount; sortedIdx++) {
			int16 runnerListIdx = _sortedZmbIndices[sortedIdx];
			if (runnerListIdx == -1)
				continue;

			_cellGrid[cellIdx * kFieldsPerCell + 2] = _zmbRunnerIdxArr[runnerListIdx];
			_sortedZmbIndices[sortedIdx] = -1;
			setCellStateAndReload(cellIdx, kCellOccupied);
			break;
		}

		if (!hasPendingZmb())
			break;
	}
}

void ZoombiniPuzzleSlides::pickNextCellForLink(int16 cellIdx, int16 nextCell, int16 direction) {
	// IDA: slides_pickNextCellForLink @ 0x4495C2
	if (cellIdx < 0 || nextCell < 0 || direction < 0 || cellIdx >= kNumCells || nextCell >= kNumCells || direction >= kNumCells)
		return;

	int16 directionData = _cellGrid[direction * kFieldsPerCell + 2];
	int16 nextData = _cellGrid[nextCell * kFieldsPerCell + 2];
	int16 cellState = _cellGrid[cellIdx * kFieldsPerCell + 1];

	if (directionData < kAttrHair && nextData < kAttrHair && cellState == kCellPath) {
		resetCellToEmpty(direction);
		resetCellToEmpty(nextCell);
		resetCellToEmpty(cellIdx);
		return;
	}

	if (directionData < kAttrHair && nextData < kAttrHair && cellState == kCellOccupied) {
		resetCellToEmpty(nextCell);
		return;
	}

	if (directionData < kAttrHair && kAttrHair <= nextData && cellState == kCellOccupied) {
		resetCellToEmpty(direction);
		return;
	}

	if (kAttrHair <= directionData && nextData < kAttrHair && cellState == kCellOccupied)
		resetCellToEmpty(nextCell);
}

void ZoombiniPuzzleSlides::markMatchedRunnersDone() {
	// IDA: slides_markMatchedRunnersDone @ 0x4447E2
	// Writes runner+295 = 1 for each locked Slides cell. That byte is
	// core259.unk00F7, which ScummVM maps to _packIsOccupied.
	for (int16 cellIdx = 0; cellIdx < kNumCells; cellIdx++) {
		if (_cellGrid[cellIdx * kFieldsPerCell + 1] != kCellLocked)
			continue;

		ZmbSnoid *snoid = getSnoid(_cellGrid[cellIdx * kFieldsPerCell + 2]);
		if (!snoid)
			continue;

		// Matched Slides runners leave the active pack on departure.
		snoid->_packIsOccupied = false;
	}
}

// =============================================================================
// Victory Checking
// IDA: slides_checkVictoryCondition @ 0x44943A
// =============================================================================

void ZoombiniPuzzleSlides::checkVictoryCondition() {
	int16 matchedCellCount = 0;
	for (int16 cellIdx = 0; cellIdx < kNumCells; cellIdx++) {
		if (_cellGrid[cellIdx * kFieldsPerCell + 1] == kCellLocked)
			matchedCellCount++;
	}

	setGoButtonsEnabled(matchedCellCount >= _loadedZmbCount);

	if (!_celebrationActive && matchedCellCount >= _loadedZmbCount) {
		_matchCount++;
		uint16 sndId = _vm->_rnd->getRandomNumber(20055, 20063);
		_vm->_sound->playZmbSound(
			ZmbResource(ZmbArchiveKind::kSystem, sndId),
			Audio::Mixer::kSFXSoundType);
	}
}

// =============================================================================
// Callback Functions
// =============================================================================

void ZoombiniPuzzleSlides::ensureCellFeature(int16 cellIdx) {
	if (cellIdx < 0 || cellIdx >= kNumCells || _cellFeatures[cellIdx])
		return;
	if (_cellGrid[cellIdx * kFieldsPerCell + 1] == kCellInert)
		return;

	ZmbFeature::EventHooks slotHooks;
	slotHooks.setPreRenderShapeFunc(
		reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleSlides::filterCommandByFlags));

	ZmbFeature *slotFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7013, 7,
		kCellPositions[cellIdx],
		ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00800000_POS_DELTA |
			ZmbFeature::FLAG_01000000_DEFER_RENDER,
		slotHooks);
	slotFeature->activateRender();
	slotFeature->deactivateAnimate();
	_cellFeatures[cellIdx] = slotFeature;
	_cellGrid[cellIdx * kFieldsPerCell + 0] = static_cast<int16>(slotFeature->getRegistrationIndex());
}

int16 ZoombiniPuzzleSlides::getBackwardChainLink(int16 cellIdx) const {
	if (cellIdx < 0 || cellIdx >= kNumCells)
		return -1;

	int16 base = cellIdx * kFieldsPerCell;
	if (_cellGrid[base + 3] != -1)
		return _cellGrid[base + 3];
	if (_cellGrid[base + 4] != -1)
		return _cellGrid[base + 4];
	return _cellGrid[base + 5];
}

int16 ZoombiniPuzzleSlides::getForwardChainLink(int16 cellIdx) const {
	if (cellIdx < 0 || cellIdx >= kNumCells)
		return -1;

	int16 base = cellIdx * kFieldsPerCell;
	if (_cellGrid[base + 8] != -1)
		return _cellGrid[base + 8];
	if (_cellGrid[base + 7] != -1)
		return _cellGrid[base + 7];
	return _cellGrid[base + 6];
}

bool ZoombiniPuzzleSlides::cellStateIs(int16 cellIdx, int16 stateA, int16 stateB, int16 stateC) const {
	if (cellIdx < 0 || cellIdx >= kNumCells)
		return false;

	int16 state = _cellGrid[cellIdx * kFieldsPerCell + 1];
	return state == stateA || state == stateB || state == stateC;
}

bool ZoombiniPuzzleSlides::cellsMatchAttr(int16 leftCellIdx, int16 rightCellIdx, int16 attrType) const {
	if (!cellStateIs(leftCellIdx, kCellOccupied, kCellLocked) || !cellStateIs(rightCellIdx, kCellOccupied, kCellLocked))
		return false;

	ZmbSnoid *leftSnoid = getSnoid(_cellGrid[leftCellIdx * kFieldsPerCell + 2]);
	ZmbSnoid *rightSnoid = getSnoid(_cellGrid[rightCellIdx * kFieldsPerCell + 2]);
	if (!leftSnoid || !rightSnoid)
		return false;

	switch (attrType) {
	case kAttrHair:
		return leftSnoid->_trait._head == rightSnoid->_trait._head;
	case kAttrEyes:
		return leftSnoid->_trait._eye == rightSnoid->_trait._eye;
	case kAttrNose:
		return leftSnoid->_trait._nose == rightSnoid->_trait._nose;
	case kAttrLegs:
		return leftSnoid->_trait._foot == rightSnoid->_trait._foot;
	default:
		return false;
	}
}

bool ZoombiniPuzzleSlides::setCellStateAndReload(int16 cellIdx, int16 state, int16 scrbId) {
	if (cellIdx < 0 || cellIdx >= kNumCells)
		return false;

	ensureCellFeature(cellIdx);
	int16 base = cellIdx * kFieldsPerCell;
	bool changed = (_cellGrid[base + 1] != state);
	_cellGrid[base + 1] = state;
	loadRunnerSCRB(_cellGrid[base + 0], scrbId);
	return changed;
}

int16 ZoombiniPuzzleSlides::findCellIdxForFeature(const ZmbFeature *feature) const {
	if (!feature)
		return -1;

	for (int16 cellIdx = 0; cellIdx < kNumCells; cellIdx++) {
		if (_cellFeatures[cellIdx] == feature)
			return cellIdx;
	}

	return -1;
}

void ZoombiniPuzzleSlides::setCellFeaturePreRenderHook(ZmbFeature *feature, int16 scrbId) {
	if (!feature)
		return;

	ZmbFeature::OnPreRenderShapeFunc callback =
		reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleSlides::filterCommandByFlags);

	if (scrbId == 7000) {
		callback = reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleSlides::filterHotspotScript);
	} else if (scrbId == 7002) {
		callback = reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleSlides::processCommandQueue);
	}

	feature->setPreRenderShapeFunc(callback);
}

void ZoombiniPuzzleSlides::syncCellFeatureScript(int16 cellIdx) {
	if (cellIdx < 0 || cellIdx >= kNumCells)
		return;

	ensureCellFeature(cellIdx);
	ZmbFeature *feature = _cellFeatures[cellIdx];
	if (!feature)
		return;

	int16 scrbId = 7013;
	int16 state = _cellGrid[cellIdx * kFieldsPerCell + 1];

	if (state == kCellInert) {
		feature->deactivateRender();
		feature->deactivateAnimate();
		return;
	}

	if (state == kCellMatched || state == kCellLocked || state == kCellOccupied) {
		scrbId = 7000;
	}

	setCellFeaturePreRenderHook(feature, scrbId);
	loadScrbOntoFeature(feature, scrbId, false);
	feature->activateRender();
	feature->deactivateAnimate();
}

uint16 ZoombiniPuzzleSlides::getAdjMaskForCommand(int16 cmd) {
	switch (cmd) {
	case 4:
		return kAdjNW;
	case 8:
		return kAdjW;
	case 12:
		return kAdjSW;
	case 16:
		return kAdjSE;
	case 20:
		return kAdjE;
	case 24:
		return kAdjNE;
	default:
		return 0;
	}
}

void ZoombiniPuzzleSlides::filterHotspotScript(ZmbFeature *feature, ZmbHotspotGroup *hsGroup,
                                               Common::Array<ZmbHotspot> &hotspots) {
	// IDA: slides_filterHotspotScript @ 0x443D75
	if (!hsGroup)
		return;

	int16 cellIdx = findCellIdxForFeature(feature);
	if (cellIdx < 0 || hotspots.size() <= 8)
		return;

	int16 base = cellIdx * kFieldsPerCell;
	uint16 adjFlags = _adjBitFlags[cellIdx];
	int16 cellState = _cellGrid[base + 1];
	int16 cellData = _cellGrid[base + 2];

	for (uint32 i = 8; i < hotspots.size();) {
		int16 cmd = hotspots[i]._shapeIdx;
		bool keep = true;

		if ((cmd == 4 || cmd == 8 || cmd == 24) && (adjFlags & getAdjMaskForCommand(cmd)) == 0) {
			keep = false;
		} else if (cmd == 73) {
			keep = (cellData == kAttrLegs);
		} else if (cmd == 74) {
			keep = (cellData == kAttrHair);
		} else if (cmd == 75) {
			keep = (cellData == kAttrNose);
		} else if (cmd == 76) {
			keep = (cellData == kAttrEyes);
		} else if (cmd == 103) {
			keep = (cellState == kCellConnector);
		} else if (cmd == 109) {
			keep = !((cellState == kCellMatched || cellState == kCellSlotBase1 || cellState == kCellLocked) &&
				_slotBaseState != kCellSlotBase2);
		} else if (cmd == 110) {
			keep = !((cellState == kCellMatched || cellState == kCellSlotBase2 || cellState == kCellLocked) &&
				_slotBaseState == kCellSlotBase2);
		}

		if (!keep) {
			hotspots.remove_at(i);
		} else {
			i++;
		}
	}
}

void ZoombiniPuzzleSlides::filterCommandByFlags(ZmbFeature *feature, ZmbHotspotGroup *hsGroup,
                                                Common::Array<ZmbHotspot> &hotspots) {
	// IDA: slides_filterCommandByFlags @ 0x444028
	if (!hsGroup)
		return;

	int16 cellIdx = findCellIdxForFeature(feature);
	if (cellIdx < 0 || hotspots.size() <= 8)
		return;

	uint16 adjFlags = _adjBitFlags[cellIdx];

	for (uint32 i = 8; i < hotspots.size();) {
		ZmbHotspot &hotspot = hotspots[i];
		hotspot._x -= 22;
		hotspot._y += 6;

		uint16 mask = getAdjMaskForCommand(hotspot._shapeIdx);
		if (mask != 0 && (adjFlags & mask) == 0) {
			hotspots.remove_at(i);
		} else {
			i++;
		}
	}
}

void ZoombiniPuzzleSlides::processCommandQueue(ZmbFeature *feature, ZmbHotspotGroup *hsGroup,
                                               Common::Array<ZmbHotspot> &hotspots) {
	// IDA: slides_processCommandQueue @ 0x444144
	if (!hsGroup)
		return;

	int16 cellIdx = findCellIdxForFeature(feature);
	if (cellIdx < 0 || hotspots.size() <= 8)
		return;

	int16 base = cellIdx * kFieldsPerCell;
	uint16 adjFlags = _adjBitFlags[cellIdx];
	int16 cellState = _cellGrid[base + 1];

	for (uint32 i = 8; i < hotspots.size();) {
		int16 cmd = hotspots[i]._shapeIdx;
		bool keep = true;

		if (cmd == 4 || cmd == 8 || cmd == 24) {
			uint16 mask = getAdjMaskForCommand(cmd);
			if ((adjFlags & mask) != 0) {
				hotspots[i]._shapeIdx += _cellSpacing;
			} else {
				keep = false;
			}
		} else if (cmd == 109) {
			keep = !((cellState == kCellMatched || cellState == kCellSlotBase1 || cellState == kCellLocked) &&
				_slotBaseState != kCellSlotBase2);
		} else if (cmd == 110) {
			keep = !((cellState == kCellMatched || cellState == kCellSlotBase2 || cellState == kCellLocked) &&
				_slotBaseState == kCellSlotBase2);
		}

		if (!keep) {
			hotspots.remove_at(i);
		} else {
			i++;
		}
	}
}

void ZoombiniPuzzleSlides::invalidateVisualRects(uint16 rectIdx, ZmbFeature *feature) {
	// IDA: slides_invalidateVisualRects @ 0x4423FD
	// Mark visual regions for redraw
}

// =============================================================================
// Snoid Finding / Constraints
// =============================================================================

ZmbSnoid *ZoombiniPuzzleSlides::findSnoidAtPoint(const Common::Point &pos) {
	// Find a snoid whose draw record contains the given point
	for (int16 i = _loadedZmbCount - 1; i >= 0; i--) {
		ZmbSnoid *snoid = getSnoid(10000 + i);
		if (!snoid || !snoid->isRenderActivated())
			continue;

		if (snoid->findDrawRecordAtPoint(pos))
			return snoid;
	}
	return nullptr;
}

const Common::Rect &ZoombiniPuzzleSlides::getDragConstraintRect() const {
	return kDragConstraint;
}

} // End of namespace Mohawk
