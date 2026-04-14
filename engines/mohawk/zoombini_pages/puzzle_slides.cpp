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

	// Initialize the hex grid based on difficulty
	initGridByDifficulty();

	// Build adjacency table
	buildHexAdjacencyTable();

	// Snapshot attributes for matching
	snapshotZmbAttrsToArrays();

	// Generate attribute pairings
	generateAttrPairings();

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
	_isDragging = 0;
	_activeCellCount = 0;
}

void ZoombiniPuzzleSlides::onGoButtonActivated() {
	// IDA: slides_onClickHandler case 2 -> puzzle_pendingTransitionTarget = 5 (BC2)
	// Route 2: Slides -> Basecamp2 (via Xfer)
	_departXferSrcSiPage = ZMB_SI_SLIDES_08;
	ZoombiniInteractive::onGoButtonActivated();
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

	// IDA: Large switch on difficulty level (0-3, but we use 1-4)
	switch (_difficultyLevel) {
	case kPuzzleDiffLevel1:
		// Level 1: Simple 3-slot configuration
		// IDA: 3 slots at cells: 57 (center), 46, 68
		_numSlots = 3;
		_slotCellMap[0] = 57;
		_slotCellMap[1] = 46;
		_slotCellMap[2] = 68;

		for (int16 i = 0; i < _numSlots; i++) {
			int16 cell = _slotCellMap[i];
			_cellGrid[cell * kFieldsPerCell + 1] = _slotBaseState;
		}
		break;

	case kPuzzleDiffLevel2:
		// Level 2: 6-slot horizontal configuration
		// IDA: 6 slots in a row pattern
		_numSlots = 6;
		_slotCellMap[0] = 46;
		_slotCellMap[1] = 48;
		_slotCellMap[2] = 50;
		_slotCellMap[3] = 65;
		_slotCellMap[4] = 67;
		_slotCellMap[5] = 69;

		for (int16 i = 0; i < _numSlots; i++) {
			int16 cell = _slotCellMap[i];
			_cellGrid[cell * kFieldsPerCell + 1] = _slotBaseState;
		}

		// Set up connector cells
		for (int16 i = 0; i < 18; i++) {
			int16 cell = kLeftArmLinkCells[i];
			if (_cellGrid[cell * kFieldsPerCell + 1] == kCellInert) {
				_cellGrid[cell * kFieldsPerCell + 1] = kCellConnector;
			}
		}
		break;

	case kPuzzleDiffLevel3:
		// Level 3: 12-slot grid configuration
		// IDA: 12 slots across two rows
		_numSlots = 12;
		for (int16 i = 0; i < 12; i++) {
			_slotCellMap[i] = kInnerLinkPairs[i];
			int16 cell = _slotCellMap[i];
			_cellGrid[cell * kFieldsPerCell + 1] = _slotBaseState;
		}

		// Set up left and right endpoints as connectors
		for (int16 i = 0; i < 3; i++) {
			_cellGrid[kLeftEndpointCells[i] * kFieldsPerCell + 1] = kCellConnector;
			_cellGrid[kRightEndpointCells[i] * kFieldsPerCell + 1] = kCellConnector;
		}

		// Set up all link cells
		for (int16 i = 0; i < 18; i++) {
			int16 cellL = kLeftArmLinkCells[i];
			int16 cellR = kRightArmLinkCells[i];
			if (_cellGrid[cellL * kFieldsPerCell + 1] == kCellInert) {
				_cellGrid[cellL * kFieldsPerCell + 1] = kCellConnector;
			}
			if (_cellGrid[cellR * kFieldsPerCell + 1] == kCellInert) {
				_cellGrid[cellR * kFieldsPerCell + 1] = kCellConnector;
			}
		}
		break;

	case kPuzzleDiffLevel4:
		// Level 4: Full 26-slot grid (uses kSlotCellIndices)
		// IDA: All primary slot cells active
		_numSlots = 26;
		for (int16 i = 0; i < 26; i++) {
			_slotCellMap[i] = kSlotCellIndices[i];
			int16 cell = _slotCellMap[i];
			_cellGrid[cell * kFieldsPerCell + 1] = _slotBaseState;
		}

		// Set up all interior link cells
		for (int16 i = 0; i < 43; i++) {
			int16 cell = kLinkCellIndices[i];
			if (_cellGrid[cell * kFieldsPerCell + 1] == kCellInert) {
				_cellGrid[cell * kFieldsPerCell + 1] = kCellConnector;
			}
		}

		// At level 4, path cells may be used for walking
		// IDA: if (slotBaseState == 505) use alternate path setup
		if (_slotBaseState == kCellSlotBase2) {
			for (int16 i = 0; i < 20; i++) {
				int16 cell = kEvenRowLinkCells[i];
				if (_cellGrid[cell * kFieldsPerCell + 1] == kCellConnector) {
					_cellGrid[cell * kFieldsPerCell + 1] = kCellPath;
				}
			}
		}
		break;

	default:
		warning("Slides: Unknown difficulty level %d", _difficultyLevel);
		break;
	}

	debugC(kZmbDebugPage, "Slides: initGridByDifficulty level=%d, numSlots=%d",
	       _difficultyLevel, _numSlots);
}

// =============================================================================
// Hex Adjacency Table
// IDA: slides_buildHexAdjacencyTable @ 0x4436E4
// =============================================================================

void ZoombiniPuzzleSlides::buildHexAdjacencyTable() {
	// Build neighbor links for all cells based on hex grid geometry.
	// Row parity determines neighbor offsets:
	// - Even rows (0,2,4,...): cell % 18 is 0-8
	// - Odd rows (1,3,5,...): cell % 18 is 9-17
	//
	// Direction offsets (from IDA analysis):
	// NW: -10 (even), -9 (odd)
	// W:  -1 (always)
	// SW: +8 (even), +9 (odd)
	// SE: +9 (even), +10 (odd)
	// E:  +1 (always)
	// NE: -9 (even), -8 (odd)

	for (int16 cell = 0; cell < kNumCells; cell++) {
		// Only process active cells (not inert)
		if (_cellGrid[cell * kFieldsPerCell + 1] == kCellInert)
			continue;

		int16 row = cell / 9;
		int16 col = cell % 9;
		bool oddRow = (row % 2) != 0;

		int16 base = cell * kFieldsPerCell;

		// NW neighbor (field 3)
		int16 nwCell = -1;
		if (row > 0) {
			if (oddRow) {
				nwCell = cell - 9;  // Odd row: directly above
			} else if (col > 0) {
				nwCell = cell - 10; // Even row: above-left
			}
		}
		if (nwCell >= 0 && nwCell < kNumCells &&
			_cellGrid[nwCell * kFieldsPerCell + 1] != kCellInert) {
			_cellGrid[base + 3] = nwCell;
			_adjBitFlags[cell] |= kAdjNW;
		}

		// W neighbor (field 4)
		int16 wCell = -1;
		if (col > 0) {
			wCell = cell - 1;
		}
		if (wCell >= 0 && wCell < kNumCells &&
			_cellGrid[wCell * kFieldsPerCell + 1] != kCellInert) {
			_cellGrid[base + 4] = wCell;
			_adjBitFlags[cell] |= kAdjW;
		}

		// SW neighbor (field 5)
		int16 swCell = -1;
		if (row < 12) {
			if (oddRow) {
				swCell = cell + 9;  // Odd row: directly below
			} else if (col > 0) {
				swCell = cell + 8;  // Even row: below-left
			}
		}
		if (swCell >= 0 && swCell < kNumCells &&
			_cellGrid[swCell * kFieldsPerCell + 1] != kCellInert) {
			_cellGrid[base + 5] = swCell;
			_adjBitFlags[cell] |= kAdjSW;
		}

		// SE neighbor (field 6)
		int16 seCell = -1;
		if (row < 12) {
			if (oddRow && col < 8) {
				seCell = cell + 10; // Odd row: below-right
			} else if (!oddRow) {
				seCell = cell + 9;  // Even row: directly below
			}
		}
		if (seCell >= 0 && seCell < kNumCells &&
			_cellGrid[seCell * kFieldsPerCell + 1] != kCellInert) {
			_cellGrid[base + 6] = seCell;
			_adjBitFlags[cell] |= kAdjSE;
		}

		// E neighbor (field 7)
		int16 eCell = -1;
		if (col < 8) {
			eCell = cell + 1;
		}
		if (eCell >= 0 && eCell < kNumCells &&
			_cellGrid[eCell * kFieldsPerCell + 1] != kCellInert) {
			_cellGrid[base + 7] = eCell;
			_adjBitFlags[cell] |= kAdjE;
		}

		// NE neighbor (field 8)
		int16 neCell = -1;
		if (row > 0) {
			if (oddRow && col < 8) {
				neCell = cell - 8;  // Odd row: above-right
			} else if (!oddRow) {
				neCell = cell - 9;  // Even row: directly above
			}
		}
		if (neCell >= 0 && neCell < kNumCells &&
			_cellGrid[neCell * kFieldsPerCell + 1] != kCellInert) {
			_cellGrid[base + 8] = neCell;
			_adjBitFlags[cell] |= kAdjNE;
		}
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
	// Generate attribute type pairings for matching.
	// Each pair determines which attribute type connects two slots.

	_numPairs = 0;
	memset(_pairTypeArray, 0, sizeof(_pairTypeArray));
	memset(_usedFlags, 0, sizeof(_usedFlags));

	// Simple pairing: cycle through attribute types
	// IDA: more complex logic involving overlap counting, but simplified here
	for (int16 i = 0; i < _numSlots && _numPairs < 16; i++) {
		// Assign attribute type (510-513) cycling
		_pairTypeArray[_numPairs] = kAttrHair + (i % 4);
		_numPairs++;
	}

	debugC(kZmbDebugPage, "Slides: generateAttrPairings numPairs=%d", _numPairs);
}

// =============================================================================
// Per-Frame Update
// IDA: slides_puzzleHoverUpdate @ 0x4427B7
// =============================================================================

void ZoombiniPuzzleSlides::onEveryFrame() {
	if (_loadedZmbCount <= 0)
		return;

	// Celebration scheduling.
	// Once _celebrationActive is set, it stays set (one celebration per match event).
	// Resets when _celebrationIndex reaches _celebrationTarget (= loaded zmb count).
	if (_celebrationActive || !_matchCount || _celebrationIndex >= _celebrationTarget) {
		if (_celebrationIndex >= _celebrationTarget) {
			_celebrationPoolState = 0;
			_celebrationLastFrame = 0;
			_matchCount = 0;
			_celebrationIndex = 0;
		}
	} else {
		debugC(1, kZmbDebugAnimation, "Slides: celebration triggered, matchCount=%d", _matchCount);
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

	debugC(kZmbDebugPage, "Slides: Assigned snoid %d to cell %d", snoid->getId(), cellIdx);

	// Check for attribute matches
	int16 matchResult = validateChainAndMarkMatched();
	if (matchResult > 0) {
		_matchCount += matchResult;
		debugC(kZmbDebugPage, "Slides: Found %d matches, total matchCount=%d", matchResult, _matchCount);
	}
}

void ZoombiniPuzzleSlides::moveZmbToCell(ZmbSnoid *snoid, int16 cellIdx) {
	// IDA: slides_moveZmbToCell @ 0x4481FE
	snoid->setPointLoc(kCellPositions[cellIdx]);
}

void ZoombiniPuzzleSlides::clearCellToEmpty(int16 cellIdx) {
	// IDA: slides_clearCellToEmpty @ 0x448955
	int16 base = cellIdx * kFieldsPerCell;
	_cellGrid[base + 1] = kCellConnector;
	_cellGrid[base + 2] = 0;
}

void ZoombiniPuzzleSlides::resetCellToEmpty(int16 cellIdx) {
	// IDA: slides_resetCellToEmpty @ 0x4496BC
	int16 base = cellIdx * kFieldsPerCell;
	_cellGrid[base + 1] = kCellInert;

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
}

void ZoombiniPuzzleSlides::clearCellLinkBits(int16 cellIdx, uint16 bitsToClear) {
	// IDA: slides_clearCellLinkBits @ 0x449048
	_adjBitFlags[cellIdx] &= ~bitsToClear;
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

int16 ZoombiniPuzzleSlides::validateChainAndMarkMatched() {
	int16 matchCount = 0;

	// Scan all slots for attribute matches with neighbors
	for (int16 slot = 0; slot < _numSlots; slot++) {
		int16 cellIdx = _slotCellMap[slot];
		if (cellIdx < 0)
			continue;

		int16 base = cellIdx * kFieldsPerCell;
		int16 state = _cellGrid[base + 1];

		// Only check occupied cells
		if (state != kCellOccupied && state != kCellLocked)
			continue;

		int16 zmbId = _cellGrid[base + 2];
		ZmbSnoid *snoid = getSnoid(zmbId);
		if (!snoid)
			continue;

		// Check all 6 neighbors for matching attributes
		for (int16 dir = 0; dir < 6; dir++) {
			int16 neighborCell = _cellGrid[base + 3 + dir];
			if (neighborCell < 0)
				continue;

			int16 neighborBase = neighborCell * kFieldsPerCell;
			int16 neighborState = _cellGrid[neighborBase + 1];

			if (neighborState != kCellOccupied && neighborState != kCellLocked)
				continue;

			int16 neighborZmbId = _cellGrid[neighborBase + 2];
			ZmbSnoid *neighborSnoid = getSnoid(neighborZmbId);
			if (!neighborSnoid)
				continue;

			// Check if any attribute matches
			bool matched = false;
			if (snoid->_trait._head == neighborSnoid->_trait._head)
				matched = true;
			if (snoid->_trait._eye == neighborSnoid->_trait._eye)
				matched = true;
			if (snoid->_trait._nose == neighborSnoid->_trait._nose)
				matched = true;
			if (snoid->_trait._foot == neighborSnoid->_trait._foot)
				matched = true;

			if (matched) {
				// Mark both cells as matched/locked
				_cellGrid[base + 1] = kCellLocked;
				_cellGrid[neighborBase + 1] = kCellLocked;
				matchCount++;
			}
		}
	}

	return matchCount;
}

int16 ZoombiniPuzzleSlides::buildChainSequence(int16 startCell, int16 *outChain, int16 maxLen) {
	// IDA: slides_buildChainSequence @ 0x444C16
	if (!outChain || maxLen <= 0)
		return 0;

	int16 count = 0;
	int16 currentCell = startCell;
	uint16 visited[kNumCells];
	memset(visited, 0, sizeof(visited));

	while (currentCell >= 0 && count < maxLen) {
		if (visited[currentCell])
			break;

		visited[currentCell] = 1;
		outChain[count++] = currentCell;

		// Find next unvisited neighbor in chain
		int16 base = currentCell * kFieldsPerCell;
		int16 nextCell = -1;

		for (int16 dir = 0; dir < 6; dir++) {
			int16 neighbor = _cellGrid[base + 3 + dir];
			if (neighbor >= 0 && !visited[neighbor]) {
				int16 neighborState = _cellGrid[neighbor * kFieldsPerCell + 1];
				if (neighborState == kCellOccupied || neighborState == kCellLocked ||
					neighborState == kCellConnector) {
					nextCell = neighbor;
					break;
				}
			}
		}

		currentCell = nextCell;
	}

	return count;
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

int16 ZoombiniPuzzleSlides::findRunnerByMatchingAttr(int16 attrType, int16 attrValue) const {
	// IDA: slides_findRunnerByMatchingAttr @ 0x444DC8
	for (int16 i = 0; i < _loadedZmbCount; i++) {
		ZmbSnoid *snoid = getSnoid(10000 + i);
		if (!snoid)
			continue;

		int16 snoidAttrVal = 0;
		switch (attrType) {
		case kAttrHair: snoidAttrVal = snoid->_trait._head; break;
		case kAttrEyes: snoidAttrVal = snoid->_trait._eye; break;
		case kAttrNose: snoidAttrVal = snoid->_trait._nose; break;
		case kAttrLegs: snoidAttrVal = snoid->_trait._foot; break;
		default: break;
		}

		if (snoidAttrVal == attrValue)
			return snoid->getId();
	}
	return -1;
}

void ZoombiniPuzzleSlides::sortZmbsByOverlapCount() {
	// IDA: slides_sortZmbsByOverlapCount @ 0x444FBF
	// Sort _sortedZmbIndices by number of matching attributes with neighbors
	for (int16 i = 0; i < _loadedZmbCount; i++) {
		_sortedZmbIndices[i] = i;
	}
	// Simple bubble sort by overlap count (simplified)
	// In original: calculates overlap counts and sorts descending
}

void ZoombiniPuzzleSlides::placeMatchingZmbInCell(int16 cellIdx, int16 attrType) {
	// IDA: slides_placeMatchingZmbInCell @ 0x4450A3
	// Find a Zoombini with matching attribute and place in cell
	// Simplified: just find first unused Zoombini
	for (int16 i = 0; i < _loadedZmbCount; i++) {
		if (_usedFlags[i])
			continue;

		ZmbSnoid *snoid = getSnoid(10000 + i);
		if (!snoid)
			continue;

		_usedFlags[i] = 1;
		assignZmbToSlot(snoid, cellIdx);
		break;
	}
}

int16 ZoombiniPuzzleSlides::pickRandomMatchingAttr(int16 snoidIdx) const {
	// IDA: slides_pickRandomMatchingAttr @ 0x44533D
	// Return a random attribute type (510-513)
	return kAttrHair + _vm->_rnd->getRandomNumber(0, 3);
}

void ZoombiniPuzzleSlides::activateChainLink(int16 linkIdx) {
	// IDA: slides_activateChainLink @ 0x445527
	// Activate visual feedback for chain link
	debugC(kZmbDebugPage, "Slides: activateChainLink %d", linkIdx);
}

void ZoombiniPuzzleSlides::confirmEndpointMatches() {
	// IDA: slides_confirmEndpointMatches @ 0x445700
	// Confirm matches at chain endpoints
}

bool ZoombiniPuzzleSlides::checkFirstAttrMatch(int16 cellIdx, int16 snoidIdx) const {
	// IDA: slides_checkFirstAttrMatch @ 0x448119
	ZmbSnoid *snoid = getSnoid(10000 + snoidIdx);
	if (!snoid)
		return false;

	int16 base = cellIdx * kFieldsPerCell;

	// Check neighbors for any attribute match
	for (int16 dir = 0; dir < 6; dir++) {
		int16 neighbor = _cellGrid[base + 3 + dir];
		if (neighbor < 0)
			continue;

		int16 neighborBase = neighbor * kFieldsPerCell;
		if (_cellGrid[neighborBase + 1] != kCellOccupied &&
			_cellGrid[neighborBase + 1] != kCellLocked)
			continue;

		int16 neighborZmbId = _cellGrid[neighborBase + 2];
		ZmbSnoid *neighborSnoid = getSnoid(neighborZmbId);
		if (!neighborSnoid)
			continue;

		if (snoid->_trait._head == neighborSnoid->_trait._head)
			return true;
		if (snoid->_trait._eye == neighborSnoid->_trait._eye)
			return true;
		if (snoid->_trait._nose == neighborSnoid->_trait._nose)
			return true;
		if (snoid->_trait._foot == neighborSnoid->_trait._foot)
			return true;
	}

	return false;
}

void ZoombiniPuzzleSlides::evalAttrMatchAndAdvance(int16 cellIdx) {
	// IDA: slides_evalAttrMatchAndAdvance @ 0x445A1B
	// Evaluate and advance chain propagation
}

void ZoombiniPuzzleSlides::evalNeighborStates(int16 cellIdx) {
	// IDA: slides_evalNeighborStates @ 0x445880
	// Evaluate neighbor cell states for chain propagation
}

void ZoombiniPuzzleSlides::propagateMatchChain() {
	// IDA: slides_propagateMatchChain @ 0x446073
	// Propagate match status through connected cells
}

int16 ZoombiniPuzzleSlides::checkAttrMatchOutcome(int16 cellIdx, int16 snoidIdx) const {
	// IDA: slides_checkAttrMatchOutcome @ 0x448D1C
	return checkFirstAttrMatch(cellIdx, snoidIdx) ? 1 : 0;
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
	// IDA: slides_updateWaterLevelSFX @ 0x44664B
	// Update water level sound effect based on progress
}

void ZoombiniPuzzleSlides::triggerSwapAnimation(int16 cellA, int16 cellB) {
	// IDA: slides_triggerSwapAnimation @ 0x449509
	debugC(kZmbDebugPage, "Slides: triggerSwapAnimation cells %d <-> %d", cellA, cellB);
}

void ZoombiniPuzzleSlides::loadRunnerSCRB(uint16 runnerId, int16 scrbId) {
	// IDA: slides_loadRunnerSCRB @ 0x44BA68
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
	for (int16 i = 0; i < _numSlots; i++) {
		int16 cellIdx = _slotCellMap[i];
		if (cellIdx >= 0) {
			int16 state = _cellGrid[cellIdx * kFieldsPerCell + 1];
			if (state == kCellLocked) {
				_cellGrid[cellIdx * kFieldsPerCell + 1] = kCellOccupied;
			}
		}
	}
}

void ZoombiniPuzzleSlides::placeNextZmbInCell(int16 cellIdx) {
	// IDA: slides_placeNextZmbInCell @ 0x445F75
	// Find next available Zoombini and place in cell
	for (int16 i = 0; i < _loadedZmbCount; i++) {
		if (_usedFlags[i])
			continue;

		ZmbSnoid *snoid = getSnoid(10000 + i);
		if (snoid && snoid->getAnimState() == kSnoidAnimIdle) {
			_usedFlags[i] = 1;
			assignZmbToSlot(snoid, cellIdx);
			break;
		}
	}
}

bool ZoombiniPuzzleSlides::hasPendingZmb() const {
	// IDA: slides_hasPendingZmb @ 0x4484AA
	for (int16 i = 0; i < _loadedZmbCount; i++) {
		if (!_usedFlags[i])
			return true;
	}
	return false;
}

void ZoombiniPuzzleSlides::scanAndResetActiveCells() {
	// IDA: slides_scanAndResetActiveCells @ 0x4484CF
	_activeCellCount = 0;
	for (int16 i = 0; i < _numSlots; i++) {
		int16 cellIdx = _slotCellMap[i];
		if (cellIdx >= 0) {
			int16 state = _cellGrid[cellIdx * kFieldsPerCell + 1];
			if (state == kCellOccupied || state == kCellLocked) {
				_activeCellList[_activeCellCount++] = cellIdx;
			}
		}
	}
}

int16 ZoombiniPuzzleSlides::findMatchingZmbForCell(int16 cellIdx) const {
	// IDA: slides_findMatchingZmbForCell @ 0x448760
	// Find a Zoombini that would match neighbors at cellIdx
	return -1; // Simplified
}

void ZoombiniPuzzleSlides::reassignDeadSlots() {
	// IDA: slides_reassignDeadSlots @ 0x44899D
	// Reassign any disconnected slots
}

int16 ZoombiniPuzzleSlides::pickNextCellForLink(int16 currentCell, uint16 dirMask) const {
	// IDA: slides_pickNextCellForLink @ 0x4495C2
	int16 base = currentCell * kFieldsPerCell;

	for (int16 dir = 0; dir < 6; dir++) {
		if (!(dirMask & (1 << dir)))
			continue;

		int16 neighbor = _cellGrid[base + 3 + dir];
		if (neighbor >= 0 && neighbor < kNumCells) {
			int16 neighborState = _cellGrid[neighbor * kFieldsPerCell + 1];
			if (neighborState == kCellConnector || neighborState == kCellSlotBase1 ||
				neighborState == kCellSlotBase2) {
				return neighbor;
			}
		}
	}

	return -1;
}

void ZoombiniPuzzleSlides::markMatchedRunnersDone() {
	// IDA: slides_markMatchedRunnersDone @ 0x4447E2
	for (int16 i = 0; i < _numSlots; i++) {
		int16 cellIdx = _slotCellMap[i];
		if (cellIdx >= 0 && _cellGrid[cellIdx * kFieldsPerCell + 1] == kCellLocked) {
			// Mark the Zoombini as matched/done
			// int16 zmbId = _cellGrid[cellIdx * kFieldsPerCell + 2];
			// In original, this would set a completion flag
		}
	}
}

// =============================================================================
// Victory Checking
// IDA: slides_checkVictoryCondition @ 0x44943A
// =============================================================================

bool ZoombiniPuzzleSlides::checkVictoryCondition() const {
	// All slots must be filled and locked (matched)
	for (int16 i = 0; i < _numSlots; i++) {
		int16 cellIdx = _slotCellMap[i];
		if (cellIdx < 0)
			continue;

		int16 state = _cellGrid[cellIdx * kFieldsPerCell + 1];
		if (state != kCellLocked)
			return false;
	}
	return true;
}

// =============================================================================
// Callback Functions
// =============================================================================

void ZoombiniPuzzleSlides::filterHotspotScript(ZmbFeature *feature, ZmbHotspotGroup *hsGroup,
                                               Common::Array<ZmbHotspot> &hotspots) {
	// IDA: slides_filterHotspotScript @ 0x443D75
	// SCRB preRenderShapeFunc callback that filters hotspot commands based on cell state.
	//
	// Original logic extracts cell index from SCRB metadata: v2 = scriptData[4].pos.x - cellGrid[0]
	// Then for each hotspot (shapeid/opcode):
	//   - opcode 4:  remove if (adjBitFlags[cell] & 0x01) == 0
	//   - opcode 8:  remove if (adjBitFlags[cell] & 0x02) == 0
	//   - opcode 24: remove if (adjBitFlags[cell] & 0x20) == 0
	//   - opcode 73: remove if cellState[cell*9+2] != 513 (kAttrLegs)
	//   - opcode 74: remove if cellState[cell*9+2] != 510 (kAttrHair)
	//   - opcode 75: remove if cellState[cell*9+2] != 512 (kAttrNose)
	//   - opcode 76: remove if cellState[cell*9+2] != 511 (kAttrEyes)
	//   - opcode 103: remove if cellState[cell*9+1] != 506 (kCellConnector)
	//   - opcode 109: remove if state(502/504/508) && _slotBaseState != 505
	//   - opcode 110: remove if state(502/505/508) && _slotBaseState == 505
	//
	// This callback is NOT currently wired to any feature. The gameplay works without it,
	// but extra animation frames may display when cells are in certain states.
	//
	// TODO: Implement full logic when the feature needing this callback is identified.
}

bool ZoombiniPuzzleSlides::filterCommandByFlags(int16 cmd, int16 flags) const {
	// IDA: slides_filterCommandByFlags @ 0x444028
	// Filters a command based on adjacency bit flags.
	// Original adjusts hotspot positions (x -= 22, y += 6) before checking.
	//
	// Opcode mapping to flag bit:
	//   - opcode 4:  check adjBitFlags[cell] & 0x01
	//   - opcode 8:  check adjBitFlags[cell] & 0x02
	//   - opcode 12: check adjBitFlags[cell] & 0x04
	//   - opcode 16: check adjBitFlags[cell] & 0x08
	//   - opcode 20: check adjBitFlags[cell] & 0x10
	//   - opcode 24: check adjBitFlags[cell] & 0x20
	//
	// Returns true if the command passes the filter (flag bit set), false to remove.
	// Currently returns true (no filtering) - gameplay unaffected.
	return true;
}

void ZoombiniPuzzleSlides::processCommandQueue() {
	// IDA: slides_processCommandQueue @ 0x444144
	// Processes command queue, similar to filterHotspotScript but also modifies shapeid.
	//
	// For opcodes 4/8/24: if flag bit set, adds _cellSpacing to shapeid
	// For opcodes 109/110: removes if cell state conditions match (same as filterHotspotScript)
	//
	// This modifies the SCRB command queue in-place to apply cell-spacing offsets
	// to animation frame indices. Without it, the puzzle may show slightly wrong
	// frame offsets but gameplay is unaffected.
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
