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

// =================================================================
// Static data tables (from IDA binary data)
// =================================================================

// IDA: pedestal positions from 0x4A1F58 (20 POINTS)
const Common::Point ZoombiniPuzzleMaze::kSnoidPositions[20] = {
	Common::Point(287, 394), Common::Point(260, 426), Common::Point(224, 447), Common::Point(188, 441),
	Common::Point(157, 455), Common::Point(263, 384), Common::Point(219, 397), Common::Point(184, 388),
	Common::Point(155, 402), Common::Point(121, 417), Common::Point(226, 354), Common::Point(189, 349),
	Common::Point(156, 354), Common::Point(131, 375), Common::Point( 85, 394), Common::Point(164, 311),
	Common::Point(125, 324), Common::Point( 79, 352), Common::Point( 29, 318), Common::Point( 15, 285),
};

// IDA: word_4A1CB4 - has shadow flag for each creature slot
const int16 ZoombiniPuzzleMaze::kCreatureHasShadow[14] = {
	0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1
};

// IDA: word_4A1CD0 - creature type ID per slot (0=base, 1=type1, 2=type2)
const int16 ZoombiniPuzzleMaze::kCreatureTypeId[14] = {
	0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2
};

// IDA: word_4A1CEC - SCRB resource ID per creature slot
const int16 ZoombiniPuzzleMaze::kCreatureScrbId[14] = {
	9000, 9000, 9000, 9001, 9001, 9001, 9001, 9001, 9001, 9000, 9000, 9000, 9003, 9003
};

// IDA: word_4A1BD4/4A1BD6 - entry pixel position per seat (14 entries, packed x/y pairs)
const Common::Point ZoombiniPuzzleMaze::kSeatPositions[14] = {
	Common::Point(101, 283), Common::Point(148, 282), Common::Point(188, 280),
	Common::Point(195, 275), Common::Point(203, 297), Common::Point(210, 316),
	Common::Point( 95,  65), Common::Point(100,  81), Common::Point(104,  96),
	Common::Point(622, 271), Common::Point(576, 288), Common::Point(545, 287),
	Common::Point(543, 286), Common::Point(554, 308),
};

// IDA: word_4A1D46/4A1D48 - grid coordinates (row, col) per seat
const Common::Point ZoombiniPuzzleMaze::kSeatGridCoords[14] = {
	Common::Point(0, 9), Common::Point(1, 9), Common::Point(2, 9),
	Common::Point(4, 10), Common::Point(4, 11), Common::Point(4, 12),
	Common::Point(3, 0), Common::Point(3, 1), Common::Point(3, 2),
	Common::Point(12, 10), Common::Point(11, 10), Common::Point(10, 10),
	Common::Point(8, 11), Common::Point(8, 12),
};

// IDA: word_4A1C0C - facing direction per seat (0-3)
const int16 ZoombiniPuzzleMaze::kSeatDirection[14] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1
};

// IDA: word_4A1C60 - movement entry direction per seat (0=decCol, 1=incRow, 2=incCol, 3=decRow)
const int16 ZoombiniPuzzleMaze::kSeatMoveDirection[14] = {
	0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 3, 3
};

// IDA: word_4A1C44 - animation shape per seat (0-3)
const int16 ZoombiniPuzzleMaze::kSeatAnimShape[14] = {
	0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 2, 2
};

// IDA: word_4A1D7E - base node cell types (18 entries: types 20-23)
const int16 ZoombiniPuzzleMaze::kBaseNodeTypes[18] = {
	20, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23, 23
};

// IDA: word_4A1DA2 - base node grid coordinates (row, col) as 18 pairs
const Common::Point ZoombiniPuzzleMaze::kBaseNodeCoords[18] = {
	Common::Point( 0, 10), Common::Point( 1, 10), Common::Point( 2, 10),
	Common::Point( 2, 11), Common::Point( 2, 12),
	Common::Point( 0,  2), Common::Point( 1,  2), Common::Point( 1,  1),
	Common::Point( 1,  0),
	Common::Point(10, 12), Common::Point(10, 11), Common::Point(11, 11),
	Common::Point(12, 10),
	Common::Point(10,  0), Common::Point(10,  1), Common::Point(10,  2),
	Common::Point(11,  2), Common::Point(12,  3),
};

// IDA: word_4A2018 - attribute offset table for Hair/Eyes/Nose/Feet
const int16 ZoombiniPuzzleMaze::kAttrOffsets[4] = {0, 5, 10, 15};

// IDA: dword_4A1DEA - arrival positions per direction (4 dirs x 20 positions, packed x/y)
const Common::Point ZoombiniPuzzleMaze::kArrivalPositions[80] = {
	// Direction 0 - left/center exit
	Common::Point(287, 394), Common::Point(260, 426), Common::Point(224, 447), Common::Point(188, 441),
	Common::Point(157, 455), Common::Point(263, 384), Common::Point(219, 397), Common::Point(184, 388),
	Common::Point(155, 402), Common::Point(121, 417), Common::Point(226, 354), Common::Point(189, 349),
	Common::Point(156, 354), Common::Point(131, 375), Common::Point( 85, 394), Common::Point(164, 311),
	Common::Point(125, 324), Common::Point( 79, 352), Common::Point( 29, 318), Common::Point( 15, 285),
	// Direction 1 - top exit
	Common::Point(  4,  86), Common::Point( 29,  70), Common::Point( 50,  68), Common::Point( 70,  68),
	Common::Point( 94,  63), Common::Point(  7, 102), Common::Point( 30,  86), Common::Point( 55,  79),
	Common::Point( 72,  79), Common::Point(  8, 116), Common::Point( 33, 100), Common::Point( 56,  93),
	Common::Point( 73,  93), Common::Point(  9, 131), Common::Point( 34, 115), Common::Point( 99, 108),
	Common::Point( 74, 108), Common::Point( 57, 108), Common::Point( 97,  79), Common::Point( 98,  93),
	// Direction 2 - right exit
	Common::Point(633, 311), Common::Point(613, 311), Common::Point(590, 336), Common::Point(571, 340),
	Common::Point(551, 345), Common::Point(632, 297), Common::Point(612, 297), Common::Point(589, 321),
	Common::Point(570, 325), Common::Point(550, 330), Common::Point(629, 281), Common::Point(609, 281),
	Common::Point(588, 307), Common::Point(569, 311), Common::Point(549, 316), Common::Point(589, 291),
	Common::Point(566, 296), Common::Point(546, 300), Common::Point(634, 336), Common::Point(614, 336),
	// Direction 3 - right-top exit (completion direction)
	Common::Point(621,  18), Common::Point(624,  40), Common::Point(624,  64), Common::Point(625,  84),
	Common::Point(594,  20), Common::Point(598,  40), Common::Point(593,  60), Common::Point(594,  75),
	Common::Point(594,  90), Common::Point(556,  32), Common::Point(560,  50), Common::Point(555,  72),
	Common::Point(563,  94), Common::Point(511,  42), Common::Point(515,  60), Common::Point(521,  80),
	Common::Point(529, 100), Common::Point(476,  67), Common::Point(484,  89), Common::Point(594, 104),
};

// IDA: word_4A1D08[0..2] - creature type SCRB IDs
const int16 ZoombiniPuzzleMaze::kCreatureTypeScrbs[3] = {9005, 9006, 9007};

// IDA: word_4A204A - path selection threshold table (21 entries)
const int16 ZoombiniPuzzleMaze::kPathSelectThresholds[20] = {
	0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 0, 1, 0
};

// IDA: word_4A2020 - slot-to-category mapping. IDA verified: {0,1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,4,4,4,4,4}
const int16 ZoombiniPuzzleMaze::kSlotToCategory[21] = {
	0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4
};

// IDA: word_4A208E - score-to-loop-count mapping (17 entries). IDA verified.
const int16 ZoombiniPuzzleMaze::kScoreToLoopCount[17] = {
	1, 1, 1, 1, 4, 4, 4, 7, 7, 7, 10, 10, 10, 13, 13, 13, 16
};

// IDA: word_4A1FAE - static path pool (11 entries). IDA verified.
const int16 ZoombiniPuzzleMaze::kStaticPathPool[11] = {
	0, 31, 52, 73, 94, 115, 136, 157, 178, 0, 0
};

// IDA: word_4A1C7C - seat flag value per seat (14 entries). IDA verified.
const int16 ZoombiniPuzzleMaze::kSeatFlagValue[14] = {
	0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 2, 2
};

// Static variant cycling counters (persistent across plays). IDA: maze_levelVariantIdx0..3
int16 ZoombiniPuzzleMaze::s_variantIdx0 = 0;
int16 ZoombiniPuzzleMaze::s_variantIdx1 = 0;
int16 ZoombiniPuzzleMaze::s_variantIdx2 = 0;
int16 ZoombiniPuzzleMaze::s_variantIdx3 = 0;

// IDA: word_4A1FC4[2*i] / word_4A1FC6[2*i] — Attribute slot mapping tables
// Maps path slot index (0-20) to (trait category offset, trait value).
// Usage: slot type maps to ZmbTrait::TraitCategory, traitValue = kAttrSlotValue[slotIdx] (0-5)
const int16 ZoombiniPuzzleMaze::kAttrSlotType[21] = {
	0, 0, 0, 0, 0, 0,     // 0-5: hair (category 0)
	1, 1, 1, 1, 1,        // 6-10: eyes (category 1)
	2, 2, 2, 2, 2,        // 11-15: nose (category 2)
	3, 3, 3, 3, 3         // 16-20: feet (category 3)
};

const int16 ZoombiniPuzzleMaze::kAttrSlotValue[21] = {
	0, 1, 2, 3, 4, 5,     // 0-5
	1, 2, 3, 4, 5,        // 6-10
	1, 2, 3, 4, 5,        // 11-15
	1, 2, 3, 4, 5         // 16-20
};

// =================================================================
// Construction / Lifecycle
// =================================================================

ZoombiniPuzzleMaze::ZoombiniPuzzleMaze(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kMaze) {
	memset(_cellTypes, 0, sizeof(_cellTypes));
	memset(_cellRunnerIdx, 0, sizeof(_cellRunnerIdx));
	memset(_cellAttrType, 0, sizeof(_cellAttrType));
	memset(_cellAttrValue, 0, sizeof(_cellAttrValue));
	memset(_nodeDirFlags, 0, sizeof(_nodeDirFlags));
	memset(_nodeDirection, 0, sizeof(_nodeDirection));
	memset(_nodeCycleFlag, 0, sizeof(_nodeCycleFlag));
	memset(_collisionCount, 0, sizeof(_collisionCount));
	memset(_collisionRunnerIdx, 0, sizeof(_collisionRunnerIdx));
	memset(_zmbRunnerSnoidIds, 0, sizeof(_zmbRunnerSnoidIds));
	memset(_scrsAnimTable, 0, sizeof(_scrsAnimTable));
	memset(_setupNodeQueue, 0, sizeof(_setupNodeQueue));
	memset(_moveQueue, 0, sizeof(_moveQueue));
	memset(_linkQueue, 0, sizeof(_linkQueue));
	memset(_columnLinkQueue, 0, sizeof(_columnLinkQueue));
	memset(_scrsPlayQueue, 0, sizeof(_scrsPlayQueue));
	memset(_arrivalQueue, 0, sizeof(_arrivalQueue));
	memset(_rowChangeQueue, 0, sizeof(_rowChangeQueue));
	memset(_crossAssignQueue, 0, sizeof(_crossAssignQueue));
	memset(_arrivalPosCounter, 0, sizeof(_arrivalPosCounter));
	memset(_zmbTraitAssign, 0, sizeof(_zmbTraitAssign));
	memset(_slotScores, 0, sizeof(_slotScores));
	memset(_connectionTable, 0, sizeof(_connectionTable));
	memset(_freeColumnList, 0, sizeof(_freeColumnList));
	memset(_reachableSlots, 0, sizeof(_reachableSlots));
	memset(_activeColumns, 0, sizeof(_activeColumns));
	memset(_selectedPathSlots, 0, sizeof(_selectedPathSlots));
	memset(_uniqueCheckArr, 0, sizeof(_uniqueCheckArr));
	memset(_committedTraitArr, 0, sizeof(_committedTraitArr));
	memset(_shuffledPathPool, 0, sizeof(_shuffledPathPool));
	memset(_placedRunnerIds, 0, sizeof(_placedRunnerIds));
	memset(_seatAssignment, 0, sizeof(_seatAssignment));
	for (int i = 0; i < kMaxRunners; i++)
		_runnerStates[i].clear();
}

ZoombiniPuzzleMaze::~ZoombiniPuzzleMaze() {
}

void ZoombiniPuzzleMaze::open() {
	// MIDIMPC.MHK contains MIDI BGM (tMID 30035-30038) - Broderbund v1.x only.
	if (!_vm->isGameVariant(GF_ZMB_TLC))
		openArchive(ZMB_MHK_MIDIMPC);
	openArchive(ZMB_MHK_MAZE2);
}

void ZoombiniPuzzleMaze::setBackgroundMusic() {
	// IDA: maze2_initAndSetup (0x42e47c) at 0x42f573:
	//   scrb_enqueueSoundResource(30035 + routeDiffLevel)
	if (!_vm->isGameVariant(GF_ZMB_TLC)) {
		int16 routeLevel = _vm->_state->readActivePageRouteLevel();
		_vm->_midi->playZmbMidi(ZmbResource(ZmbArchiveKind::kPage, static_cast<uint16>(30035 + routeLevel)));
	}
}

void ZoombiniPuzzleMaze::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniPuzzleMaze::loadFeatures() {
	// IDA: puzzleMaze2_42E47C (0x42e47c)
	_difficultyLevel = static_cast<ZmbPuzzleDifficultyLevel>(_vm->_state->readActivePageRouteLevel() + 1); // 1-based (1-4)

	// Load terrain barrier bitmap (tBMP 100)
	loadTerrainBitmap(100);

	// Preload shape images
	_vm->_gfx->preloadImage(5100);
	_vm->_gfx->preloadImage(7000);
	_vm->_gfx->preloadImage(8000);
	_vm->_gfx->preloadImage(9000);
	_vm->_gfx->preloadImage(10000);
	_vm->_gfx->preloadImage(12000);

	// Load main features: 28 SCRBs at 7000
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

	loadRegsCoordinateTables();

	// Load reject pool: 8 at SCRS 14000
	for (uint16 i = 0; i < 8; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 5100),
				  14000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 96 at SCRS 15000
	for (uint16 i = 0; i < 96; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 5100),
				  15000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// IDA 0x42ea74: overlay anim feature
	_overlayAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 12000), 12001, 7,
		ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);

	loadZoombinisFromPack();

	// IDA 0x42ea24: if difficulty 3 (0-based = kPuzzleDiffLevel4) and fewer than 5 zoombinis,
	// bump to difficulty 4 (0-based), which uses the fixed single REGS 16609 layout.
	if (_difficultyLevel == kPuzzleDiffLevel4 && _loadedZmbCount < 5)
		_difficultyLevel = static_cast<ZmbPuzzleDifficultyLevel>(kPuzzleDiffLevel4 + 1);

	loadRegsConfigByLevel();
	loadAndParseRegsData();
	createCreatureFeatures();

	// IDA 0x42eea8: creature base animation
	_creatureBaseFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 9000), 9005, 7,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// IDA 0x42eed2-0x42ef3c: NoOp runner layers (word_4AF45C[0..10])
	for (int i = 0; i < 11; i++) {
		_noopFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 8000), 8011, 0,
			ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM);
	}

	// IDA 0x42f378: final SCRB 8011
	loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8011, 0,
		ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// IDA 0x42f399: SCRB 8004
	_finalOverlayA = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8004, 0,
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA 0x42f3ba: SCRB 8000
	_finalOverlayB = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8000, 0,
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA 0x42f3bf: NoOp runner 11
	_noopFeatures[11] = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8011, 0,
		ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM);

	layoutStaticAndWalkIn();
	assignStaggeredWalkDelays();

	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(6000);
	loadHelpButtonFeature();

	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagMaze);

	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20068);

	_celebrationTrigger = false;
	_celebrationsPlayed = 0;
	_celebrationTarget = 0;
	_celebrationPoolState = 0;
	_celebrationLastFrame = 0;

	// IDA: lilly_bAdvanceEnabled = 0 / lilly_stateVar8 = 0 at puzzle init.
	// Go button starts disabled until the first Zoombini reaches a path node.
	_bAdvanceEnabled = false;
	_nodeArrivalCount = 0;
}

void ZoombiniPuzzleMaze::onGoButtonActivated() {
	_departXferSrcSiPage = ZMB_SI_MAZE_16;
	ZoombiniInteractive::onGoButtonActivated();
}

Common::String ZoombiniPuzzleMaze::debugGetAnswer() const {
	// Bubblewonder Abyss: snoids are launched into a 13x13 hex grid.
	// _selectedPathSlots[] holds the trait-slot indices for each of the 2-3 chosen paths.
	// Slot encoding: kAttrOffsets = {0,5,10,15} → slot = offset[j] + traitValue
	//   slots 1-5 = hair 1-5, slots 6-10 = eye 1-5, slots 11-15 = nose 1-5, slots 16-20 = feet 1-5
	// Cell type 2 = attribute routing: snoid with matching trait gets routed to nodeDirection;
	//   otherwise continues straight.
	// _nodeCycleFlag[r][c] != 0 = colored arrow that CYCLES direction on each snoid pass.
	static const char *kAttrTypeNames[] = {"", "hair", "eyes", "nose", "feet"};
	static const ZmbTrait::TraitCategory kAttrTypeCat[] = {
		ZmbTrait::kTraitHair, ZmbTrait::kTraitHair,
		ZmbTrait::kTraitEyes, ZmbTrait::kTraitNose, ZmbTrait::kTraitFeet
	};
	static const char *kDirNames[] = {"decCol(W)", "incRow(S)", "incCol(E)", "decRow(N)"};

	auto decodeSlot = [&](int16 slot) -> Common::String {
		if (slot < 1 || slot > 20) return "?";
		int t = 0;
		int v = slot;
		for (int j = 3; j >= 0; j--) {
			if (slot > kAttrOffsets[j]) { t = j + 1; v = slot - kAttrOffsets[j]; break; }
		}
		if (t < 1 || t > 4) return "?";
		ZmbTrait::TraitCategory cat = kAttrTypeCat[t];
		return Common::String::format("%s=%d(%s)", kAttrTypeNames[t], v,
			ZmbTrait::debugTraitValueName(cat, v));
	};

	Common::String s = Common::String::format("Bubblewonder Abyss (level %d):\n", _difficultyLevel);

	// Show the selected path slots (2-3 distinct trait values)
	Common::Array<int16> uniqueSlots;
	for (int i = 0; i < _pathSlotWriteIdx && i < 20; i++) {
		int16 sl = _selectedPathSlots[i];
		if (!sl) continue;
		bool dup = false;
		for (uint j = 0; j < uniqueSlots.size(); j++)
			if (uniqueSlots[j] == sl) { dup = true; break; }
		if (!dup) uniqueSlots.push_back(sl);
	}

	s += Common::String::format("  %d active path(s):\n", uniqueSlots.size());
	const ZmbStateFile &f = _vm->_state->_f;
	for (uint pi = 0; pi < uniqueSlots.size(); pi++) {
		int16 sl = uniqueSlots[pi];
		s += Common::String::format("  Path %d: matching %s →", (int)(pi + 1),
			decodeSlot(sl).c_str());

		// Derive attr type/value from slot
		int t = 0, v = sl;
		for (int j = 3; j >= 0; j--) {
			if (sl > kAttrOffsets[j]) { t = j + 1; v = sl - kAttrOffsets[j]; break; }
		}

		// Find snoids with this trait — use the loaded snoid's cached name
		for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount; i++) {
			const ZmbStateActiveEntry &e = f._zmbPackActive._entries[i];
			if (!e._bIsOccupied) continue;
			uint8 traitVal = 0;
			switch (t) {
			case 1: traitVal = e._traits._head; break;
			case 2: traitVal = e._traits._eye;  break;
			case 3: traitVal = e._traits._nose; break;
			case 4: traitVal = e._traits._foot; break;
			}
			if (traitVal == v) {
				// Fetch name from the already-loaded snoid (avoids non-const getU32Name)
				uint16 snoidId = static_cast<uint16>(10000 + i);
				ZmbSnoid *snoid = getSnoid(snoidId);
				Common::String snName = snoid ? snoid->_name.encode() : Common::String("?");
				s += Common::String::format(" %s-%s-%s-%s (%s)",
					ZmbTrait::debugTraitValueName(ZmbTrait::kTraitHair, e._traits._head),
					ZmbTrait::debugTraitValueName(ZmbTrait::kTraitEyes, e._traits._eye),
					ZmbTrait::debugTraitValueName(ZmbTrait::kTraitNose, e._traits._nose),
					ZmbTrait::debugTraitValueName(ZmbTrait::kTraitFeet, e._traits._foot),
					snName.c_str());
			}
		}
		s += "\n";
	}

	// Show attribute routing cells (cell type 2) - these are the key sorting nodes
	s += "  Attribute routing cells (cell type 2):\n";
	bool anyRouting = false;
	for (int r = 0; r < kGridRows; r++) {
		for (int c = 0; c < kGridCols; c++) {
			if (_cellTypes[r][c] != 2) continue;
			anyRouting = true;
			int16 t = _cellAttrType[r][c];
			int16 v = _cellAttrValue[r][c];
			const char *dir = (_nodeDirection[r][c] >= 0 && _nodeDirection[r][c] < 4)
				? kDirNames[_nodeDirection[r][c]] : "?";
			const char *cycle = _nodeCycleFlag[r][c] ? " [CYCLE arrow changes each pass]" : "";
			ZmbTrait::TraitCategory cat = (t >= 1 && t <= 4) ? kAttrTypeCat[t] : ZmbTrait::kTraitHair;
			s += Common::String::format("    [%2d,%2d] if %s=%d(%s) → route %s%s\n",
				r, c, kAttrTypeNames[t], v,
				ZmbTrait::debugTraitValueName(cat, v), dir, cycle);
		}
	}
	if (!anyRouting)
		s += "    (none yet — grid not initialized)\n";

	// Summary of cycle (colored-arrow) nodes
	bool anyCycle = false;
	for (int r = 0; r < kGridRows && !anyCycle; r++)
		for (int c = 0; c < kGridCols && !anyCycle; c++)
			if (_nodeCycleFlag[r][c]) anyCycle = true;
	if (anyCycle) {
		s += "  Colored arrows (cycle direction on each pass):\n";
		for (int r = 0; r < kGridRows; r++) {
			for (int c = 0; c < kGridCols; c++) {
				if (!_nodeCycleFlag[r][c]) continue;
				const char *dir = (_nodeDirection[r][c] >= 0 && _nodeDirection[r][c] < 4)
					? kDirNames[_nodeDirection[r][c]] : "?";
				s += Common::String::format("    [%2d,%2d] currently pointing %s (changes each pass)\n",
					r, c, dir);
			}
		}
	}
	return s;
}

void ZoombiniPuzzleMaze::loadZoombinisFromPack() {
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

void ZoombiniPuzzleMaze::loadRegsConfigByLevel() {
	// IDA: maze_loadRegsConfigByLevel (0x4319C9)
	// REGS resource ID is base + s_variantIdxN (cycled deterministically across plays).
	// NOT random — initGridAndSelectPaths bumps the matching s_variantIdxN counter
	// after each play to step through the variant set.
	switch (_difficultyLevel) {
	case kPuzzleDiffLevel1:
		_levelVariantIdx = s_variantIdx0;
		_regsResourceId = 16600 + _levelVariantIdx;
		break;
	case kPuzzleDiffLevel2:
		_levelVariantIdx = s_variantIdx1;
		_regsResourceId = 16602 + _levelVariantIdx;
		break;
	case kPuzzleDiffLevel3:
		_levelVariantIdx = s_variantIdx2;
		_regsResourceId = 16604 + _levelVariantIdx;
		break;
	case kPuzzleDiffLevel4:
		_levelVariantIdx = s_variantIdx3;
		_regsResourceId = 16606 + _levelVariantIdx;
		break;
	default:
		// IDA case 4 (raw): difficulty bumped to max — fixed REGS 16609.
		_regsResourceId = 16609;
		_levelVariantIdx = 0;
		break;
	}

	debugC(kZmbDebugPage, "Maze: level %d, variant %d, REGS %d",
	       _difficultyLevel, _levelVariantIdx, _regsResourceId);
}

void ZoombiniPuzzleMaze::loadAndParseRegsData() {
	Common::SeekableReadStream *stream = _vm->getResource(ID_REGS, ZmbResource(ZmbArchiveKind::kPage, _regsResourceId));
	if (!stream) {
		warning("ZoombiniInteractiveMaze: Failed to load REGS %d", _regsResourceId);
		return;
	}
	
	uint32 dataSize = stream->size();
	uint32 wordCount = dataSize / 2;
	
	_regsData.clear();
	_regsData.resize(wordCount);
	
	for (uint32 i = 0; i < wordCount; i++) {
		_regsData[i] = stream->readSint16BE();
	}
	delete stream;
	
	if (wordCount < 10) {
		warning("ZoombiniInteractiveMaze: REGS %d too small (%u words)", _regsResourceId, wordCount);
		return;
	}
	
	_totalCreatureCount = _regsData[0];
	
	for (int i = 0; i < 10; i++)
		_creatureSlots[i] = 0;
	
	for (int col = 1; col <= 9; col++)
		_creatureSlots[col] = _regsData[col];
	
	debugC(1, kZmbDebugAnimation, "Maze REGS %d: count=%d, slots=[%d %d %d %d %d %d %d %d %d]",
		_regsResourceId, _totalCreatureCount,
		_creatureSlots[1], _creatureSlots[2], _creatureSlots[3],
		_creatureSlots[4], _creatureSlots[5], _creatureSlots[6],
		_creatureSlots[7], _creatureSlots[8], _creatureSlots[9]);
}

void ZoombiniPuzzleMaze::createCreatureFeatures() {
	// Phase 1: Create type 1 creature runners
	for (int col = 1; col <= 9; col++) {
		int16 slot = _creatureSlots[col];
		if (slot == 0)
			continue;
		int16 slotIdx = slot - 1;
		if (slotIdx < 0 || slotIdx >= 14)
			continue;
		int16 typeId = kCreatureTypeId[slotIdx];
		if (typeId == 1 && _creatureSlotFeatures[1] == nullptr) {
			_creatureSlotFeatures[1] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 9000), 9006, 7,
				ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
				ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
		}
	}
	
	// Phase 2: Create grid cell creature features
	for (int col = 1; col <= 9; col++) {
		int16 slot = _creatureSlots[col];
		if (slot == 0)
			continue;
		int16 slotIdx = slot - 1;
		if (slotIdx < 0 || slotIdx >= 14)
			continue;
		_gridCreatureFeatures[slotIdx] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 7000), 7000 + slotIdx, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY |
			ZmbFeature::FLAG_00100000_PLAY_ONCE);
	}
	
	// Phase 3: Create obstacle features
	for (int col = 1; col <= 9; col++) {
		int16 slot = _creatureSlots[col];
		if (slot == 0)
			continue;
		int16 slotIdx = slot - 1;
		if (slotIdx < 0 || slotIdx >= 14)
			continue;
		int16 scrbId = kCreatureScrbId[slotIdx];
		bool hasShadow = (kCreatureHasShadow[slotIdx] != 0);
		_creatureObstacleFeatures[slotIdx] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 9000), scrbId, 7,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY |
			ZmbFeature::FLAG_00080000_DEFER_ANIM);
		if (hasShadow) {
			_creatureShadowFeatures[slotIdx] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 9000), scrbId + 1, 7,
				ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY |
				ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00800000_POS_DELTA);
		}
	}
	
	// Phase 4: Create type 2 creature runners
	for (int col = 1; col <= 9; col++) {
		int16 slot = _creatureSlots[col];
		if (slot == 0)
			continue;
		int16 slotIdx = slot - 1;
		if (slotIdx < 0 || slotIdx >= 14)
			continue;
		int16 typeId = kCreatureTypeId[slotIdx];
		if (typeId == 2 && _creatureSlotFeatures[2] == nullptr) {
			_creatureSlotFeatures[2] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 9000), 9007, 7,
				ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
				ZmbFeature::FLAG_00008000_LOOP_ANIM);
		}
	}
}

void ZoombiniPuzzleMaze::loadRegsCoordinateTables() {
	loadREGS(ZmbArchiveKind::kPage, 16000);
	loadREGS(ZmbArchiveKind::kPage, 17000);
	loadREGS(ZmbArchiveKind::kPage, 17001);
	loadREGS(ZmbArchiveKind::kPage, 18000);
	loadREGS(ZmbArchiveKind::kPage, 18001);

	auto it16000 = _regsMap.find(16000);
	if (it16000 != _regsMap.end()) {
		ZmbRegs *regs = it16000->_value;
		_regsGridX.clear();
		_regsGridY.clear();
		for (uint i = 0; i < regs->_offsets.size(); i++) {
			_regsGridX.push_back(regs->_offsets[i].x);
			_regsGridY.push_back(regs->_offsets[i].y);
		}
	}

	for (int row = 0; row < kGridRows; row++) {
		int16 baseX = (row < static_cast<int>(_regsGridX.size())) ? _regsGridX[row] : static_cast<int16>(50 + row * 45);
		int16 baseY = (row < static_cast<int>(_regsGridY.size())) ? _regsGridY[row] : static_cast<int16>(100);

		for (int col = 0; col < kGridCols; col++) {
			int16 cellX = 35 * col + baseX + 18;
			int16 cellY = col * 2 + baseY + 15;
			_gridCellPos[row][col] = Common::Point(cellX, cellY);
			_gridCellRect[row][col] = Common::Rect(cellX - 17, cellY - 17, cellX + 18, cellY + 18);
		}
	}
}

// =================================================================
// Grid initialization
// IDA: net_initGridAndSelectPaths (0x431A88) - Master Dispatcher
// =================================================================

void ZoombiniPuzzleMaze::initGridAndSelectPaths() {
	if (_gridInitialized)
		return;
	_gridInitialized = true;

	// Clear grid state
	memset(_cellTypes, 0, sizeof(_cellTypes));
	memset(_cellRunnerIdx, 0, sizeof(_cellRunnerIdx));
	memset(_cellAttrType, 0, sizeof(_cellAttrType));
	memset(_cellAttrValue, 0, sizeof(_cellAttrValue));
	memset(_nodeDirFlags, 0, sizeof(_nodeDirFlags));
	memset(_nodeDirection, 0, sizeof(_nodeDirection));
	memset(_nodeCycleFlag, 0, sizeof(_nodeCycleFlag));
	memset(_collisionCount, 0, sizeof(_collisionCount));
	memset(_collisionRunnerIdx, 0, sizeof(_collisionRunnerIdx));

	// Place 18 base nodes at fixed grid positions
	generateBaseNodes();

	// Copy and shuffle path pool. IDA: net_initGridAndSelectPaths @ 0x431A88
	for (int i = 0; i < 11; i++)
		_shuffledPathPool[i] = kStaticPathPool[i];

	// Fisher-Yates shuffle of indices 2..8
	int16 v3 = 8;
	for (int16 i = 2; i < 9; i++) {
		int16 randIdx = _vm->_rnd->getRandomNumber(2, v3);
		_shuffledPathPool[i] = _shuffledPathPool[randIdx];
		// Shift remaining down
		for (int16 j = randIdx; j < v3 + 1; j++)
			_shuffledPathPool[j] = _shuffledPathPool[j + 1];
		--v3;
	}

	// Initialize path selection state
	_pathSlotWriteIdx = 0;
	_pathSlotReadIdx = 0;
	_reachableSlotCount = 0;
	_activeColumnCount = 0;
	_activeColumnsExist = 0;
	_committedTraitCount = 0;
	memset(_selectedPathSlots, 0, sizeof(_selectedPathSlots));
	memset(_zmbTraitAssign, 0, sizeof(_zmbTraitAssign));
	memset(_slotScores, 0, sizeof(_slotScores));
	memset(_uniqueCheckArr, 0, sizeof(_uniqueCheckArr));
	memset(_committedTraitArr, 0, sizeof(_committedTraitArr));

	// Dispatch path selection by difficulty. IDA: switch at 0x431B8A
	switch (_difficultyLevel) {
	case kPuzzleDiffLevel1:
		buildZmbAssignmentAlt2();
		if (++s_variantIdx0 > 1)
			s_variantIdx0 = 0;
		break;
	case kPuzzleDiffLevel2:
		if (s_variantIdx1)
			selectPathSlots2();
		else
			buildZmbAssignmentAlt();
		if (++s_variantIdx1 > 1)
			s_variantIdx1 = 0;
		break;
	case kPuzzleDiffLevel3:
		if (s_variantIdx2)
			selectPathSlots();
		else
			selectPathSlots2();
		if (++s_variantIdx2 > 1)
			s_variantIdx2 = 0;
		break;
	case kPuzzleDiffLevel4:
		buildZmbAssignmentList();
		s_variantIdx3 += 2;
		if (s_variantIdx3 > 2)
			s_variantIdx3 = 0;
		break;
	default:
		// IDA case 4 (raw): triggered when difficulty 3 + zmbCount<5 path bumps
		// _difficultyLevel to kPuzzleDiffLevel4+1. Calls net_selectPathSlots without
		// touching any variant counter (REGS 16609 fixed layout).
		selectPathSlots();
		break;
	}

	// Initialize grid runners from REGS data
	initGridRunners();

	_puzzleReady = true;

	debugC(kZmbDebugPage, "Maze: Grid initialized, difficulty %d, %d path slots",
	       _difficultyLevel, _pathSlotWriteIdx);
}

void ZoombiniPuzzleMaze::generateBaseNodes() {
	for (int i = 0; i < 18; i++) {
		int16 row = kBaseNodeCoords[i].x;
		int16 col = kBaseNodeCoords[i].y;
		if (row >= 0 && row < kGridRows && col >= 0 && col < kGridCols) {
			_cellTypes[row][col] = kBaseNodeTypes[i];
		}
	}
}

// =================================================================
// SCRS animation table initialization
// IDA: net_initRunnerAnimTable (0x434528)
// =================================================================

void ZoombiniPuzzleMaze::initRunnerAnimTable(int16 runnerIdx) {
	// IDA: net_initRunnerAnimTable_434528
	// Initialize the SCRS animation lookup table on a runner based on foot trait.
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
	int16 foot = rs.footTrait;

	// Direction walking SCRS (4 directions)
	rs.scrsTable[0] = foot + 15014;  // dir 0: decCol walk
	rs.scrsTable[1] = foot + 15019;  // dir 1: incRow walk
	rs.scrsTable[2] = foot + 15024;  // dir 2: incCol walk
	rs.scrsTable[3] = foot + 15029;  // dir 3: decRow walk

	// Direction alt SCRS (4 directions)
	rs.scrsTable[4] = foot + 15055;  // dir 0 alt
	rs.scrsTable[5] = foot + 15060;  // dir 1 alt
	rs.scrsTable[6] = foot + 15065;  // dir 2 alt
	rs.scrsTable[7] = foot + 15070;  // dir 3 alt

	// Special SCRS
	rs.scrsTable[8] = foot + 14999;  // idle
	rs.scrsTable[9] = foot + 15004;  // special 1
	rs.scrsTable[10] = foot + 15009; // special 2

	rs.scrsTable[11] = foot - 1;     // foot index (0-based)
}

void ZoombiniPuzzleMaze::initAllRunnerAnimTables() {
	// IDA: First-click init loop at 0x430424
	if (_animTablesInitialized)
		return;
	_animTablesInitialized = true;

	for (int16 i = 0; i < _runnerCount; i++) {
		initRunnerAnimTable(i);
	}
}

// =================================================================
// Path selection helpers
// IDA: net_* functions at 0x432052..0x432E6B
// =================================================================

void ZoombiniPuzzleMaze::collectZmbAttrs() {
	// IDA: net_collectZmbAttrs (0x432052)
	// Clear the assign array
	for (int16 i = 0; i < _loadedZmbCount; i++)
		for (int16 j = 0; j < 4; j++)
			_zmbTraitAssign[4 * i + j] = 0;

	// Collect trait bytes from all loaded zoombinis
	for (int16 i = 0; i < _loadedZmbCount; i++) {
		uint16 snoidId = 10000 + i;
		ZmbSnoid *snoid = getSnoid(snoidId);
		if (!snoid)
			continue;
		_zmbTraitAssign[4 * i + 0] = snoid->_trait._head;  // hair (category 1)
		_zmbTraitAssign[4 * i + 1] = snoid->_trait._eye;   // eyes (category 2)
		_zmbTraitAssign[4 * i + 2] = snoid->_trait._nose;  // nose (category 3)
		_zmbTraitAssign[4 * i + 3] = snoid->_trait._foot;  // feet (category 4)
	}
}

int16 ZoombiniPuzzleMaze::countZmbAttrMatches(int16 attrIdx) {
	// IDA: net_countZmbAttrMatches (0x43217C)
	int16 zmbCount = 0;

	// Clear score array
	for (int16 i = 0; i < 21; i++)
		_slotScores[i] = 0;

	for (int16 i = 0; i < _loadedZmbCount; i++) {
		int16 matchCount = 1;
		// Phase 1: Check if zmb matches attrIdx -> remove it
		for (int16 j = 0; j < 4; j++) {
			if (_zmbTraitAssign[4 * i + j]
				&& kAttrOffsets[j] + _zmbTraitAssign[4 * i + j] == attrIdx
				&& attrIdx) {
				matchCount = 0;
				for (int16 k = 0; k < 4; k++)
					_zmbTraitAssign[4 * i + k] = 0;
				break;
			}
		}
		// Phase 2: Tally remaining zmb's traits into score array
		if (matchCount) {
			for (int16 k = 0; k < 4; k++) {
				if (_zmbTraitAssign[4 * i + k]) {
					int16 slot = kAttrOffsets[k] + _zmbTraitAssign[4 * i + k];
					_slotScores[slot]++;
				} else {
					matchCount = 0;
					break;
				}
			}
		}
		if (matchCount)
			zmbCount++;
	}
	return zmbCount;
}

int16 ZoombiniPuzzleMaze::collectMatchingZmbAttrs(int16 attrIdx) {
	// IDA: net_collectMatchingZmbAttrs (0x4320C1)
	// Clear assign array
	for (int16 i = 0; i < _loadedZmbCount; i++)
		for (int16 j = 0; j < 4; j++)
			_zmbTraitAssign[4 * i + j] = 0;

	int16 matchCount = 0;

	// Re-collect trait bytes and filter
	for (int16 i = 0; i < _loadedZmbCount; i++) {
		uint16 snoidId = 10000 + i;
		ZmbSnoid *snoid = getSnoid(snoidId);
		if (!snoid)
			continue;

		int16 traits[4] = {
			snoid->_trait._head, snoid->_trait._eye,
			snoid->_trait._nose, snoid->_trait._foot
		};

		bool found = false;
		for (int16 k = 0; k < 4; k++) {
			if (kAttrOffsets[k] + traits[k] == attrIdx)
				found = true;
		}
		if (found) {
			for (int16 k = 0; k < 4; k++)
				_zmbTraitAssign[4 * i + k] = traits[k];
			matchCount++;
		}
	}
	return matchCount;
}

void ZoombiniPuzzleMaze::initConnectionTable() {
	// IDA: net_initConnectionTable (0x433218)
	memset(_connectionTable, 0, sizeof(_connectionTable));
	for (int16 i = 0; i < 21; i++) {
		if (!_slotScores[i])
			_connectionTable[i] = i;
	}
}

int16 ZoombiniPuzzleMaze::rebuildReachabilityList() {
	// IDA: net_rebuildReachabilityList (0x433249)
	memset(_reachableSlots, 0, sizeof(_reachableSlots));
	_reachableSlotCount = 0;

	for (int16 i = 0; i < 21; i++) {
		if (_connectionTable[i]) {
			_reachableSlots[++_reachableSlotCount] = _connectionTable[i];
		}
	}
	return _reachableSlotCount;
}

void ZoombiniPuzzleMaze::initAllSlotsReachable() {
	// IDA: net_initAllSlotsReachable (0x433338)
	for (int16 i = 0; i < 21; i++)
		_reachableSlots[i] = i;
	_reachableSlotCount = 20;
}

int16 ZoombiniPuzzleMaze::findBestAttrSlotInRange(int16 minScore, int16 maxScore) {
	// IDA: net_findBestAttrSlotInRange (0x432AEC)
	int16 bestSlot = 0;
	int16 bestScore = 0;
	for (int16 slot = 1; slot < 21; slot++) {
		if (_slotScores[slot] >= minScore
			&& _slotScores[slot] <= maxScore
			&& bestScore < _slotScores[slot]) {
			bestScore = _slotScores[slot];
			bestSlot = slot;
		}
	}
	return bestSlot;
}

int16 ZoombiniPuzzleMaze::findHighestScoredSlotInRange(int16 excludeSlot, int16 minScore, int16 maxScore) {
	// IDA: net_findHighestScoredSlotInRange (0x4331A3)
	// NOTE: searches 1..19 (not 1..20!) and requires same category
	int16 bestSlot = 0;
	int16 bestScore = 0;
	for (int16 slot = 1; slot < 20; slot++) {
		if (kSlotToCategory[slot] == kSlotToCategory[excludeSlot]
			&& _slotScores[slot] >= minScore
			&& _slotScores[slot] <= maxScore
			&& bestScore < _slotScores[slot]) {
			bestScore = _slotScores[slot];
			bestSlot = slot;
		}
	}
	return bestSlot;
}

int16 ZoombiniPuzzleMaze::findHighestScoredSlot(int16 excludeSlot) {
	// IDA: net_findHighestScoredSlot (0x432359)
	int16 bestSlot = 0;
	int16 bestScore = 0;
	for (int16 slot = 1; slot < 21; slot++) {
		if (bestScore < _slotScores[slot] && slot != excludeSlot) {
			bestScore = _slotScores[slot];
			bestSlot = slot;
		}
	}
	return bestSlot;
}

int16 ZoombiniPuzzleMaze::getAttrMatchCount(int16 idx) const {
	// IDA: net_getAttrMatchCount (0x432A94)
	if (idx < 0 || idx >= 21)
		return 0;
	return _slotScores[idx];
}

int16 ZoombiniPuzzleMaze::countReachableSlots() {
	// IDA: net_countReachableSlots (0x433184)
	int16 count = 0;
	for (int16 slot = 1; slot < 21; slot++) {
		if (_slotScores[slot])
			count++;
	}
	return count;
}

void ZoombiniPuzzleMaze::initFreeColumnList() {
	// IDA: net_initFreeColumnList (0x4332A8)
	memset(_freeColumnList, 0, sizeof(_freeColumnList));
	for (int16 i = 0; i < 21; i++) {
		if (!_slotScores[i])
			_freeColumnList[i] = i;
	}
}

int16 ZoombiniPuzzleMaze::collectActiveColumns() {
	// IDA: net_collectActiveColumns (0x4332D9)
	_activeColumnsExist = 0;
	memset(_activeColumns, 0, sizeof(_activeColumns));
	_activeColumnCount = 0;

	for (int16 i = 0; i < 21; i++) {
		if (_freeColumnList[i]) {
			_activeColumnsExist = 1;
			_activeColumns[++_activeColumnCount] = _freeColumnList[i];
		}
	}
	return _activeColumnCount;
}

int16 ZoombiniPuzzleMaze::findBestNextSlot(int16 searchIdx) {
	// IDA: net_findBestNextSlot (0x4327D6)
	int16 bestSlot = 0;
	int16 bestZmb = -1;
	int16 minScore = 21;

	for (int16 iterSlot = 1; iterSlot < 21; iterSlot++) {
		if (_slotScores[iterSlot] <= 0 || _slotScores[iterSlot] > minScore || searchIdx == iterSlot)
			continue;

		for (int16 zmbI = 0; zmbI < _loadedZmbCount; zmbI++) {
			bool foundMatch = false;
			for (int16 traitJ = 0; traitJ < 4; traitJ++) {
				if (_zmbTraitAssign[4 * zmbI + traitJ] <= 0)
					continue;
				if (kAttrOffsets[traitJ] + _zmbTraitAssign[4 * zmbI + traitJ] != iterSlot)
					continue;

				// Check uniqueness against committed entries
				int16 isUnique = 1;
				for (int16 k = 0; k < 20 && _committedTraitCount < 4; k++) {
					if (_uniqueCheckArr[4 * k + traitJ] > 0
						&& _uniqueCheckArr[4 * k + traitJ] == _zmbTraitAssign[4 * zmbI + traitJ])
						isUnique = 0;
				}
				if (isUnique) {
					bestSlot = iterSlot;
					bestZmb = zmbI;
					minScore = _slotScores[iterSlot];
					foundMatch = true;
				}
				break; // break traitJ loop
			}
			if (foundMatch)
				break; // break zmbI loop
		}
	}

	if (bestSlot) {
		// Clear scores
		memset(_slotScores, 0, sizeof(_slotScores));

		// Record winning zmb's traits
		if (_committedTraitCount < 20) {
			_uniqueCheckArr[4 * _committedTraitCount + 0] = _zmbTraitAssign[4 * bestZmb + 0];
			_uniqueCheckArr[4 * _committedTraitCount + 1] = _zmbTraitAssign[4 * bestZmb + 1];
			_uniqueCheckArr[4 * _committedTraitCount + 2] = _zmbTraitAssign[4 * bestZmb + 2];
			_uniqueCheckArr[4 * _committedTraitCount + 3] = _zmbTraitAssign[4 * bestZmb + 3];
			_committedTraitCount++;
		}

		// Remove matching zmbs and rebuild scores
		for (int16 m = 0; m < _loadedZmbCount; m++) {
			for (int16 n = 0; n < 4; n++) {
				if (_zmbTraitAssign[4 * m + n]
					&& kAttrOffsets[n] + _zmbTraitAssign[4 * m + n] == bestSlot) {
					for (int16 ii = 0; ii < 4; ii++)
						_zmbTraitAssign[4 * m + ii] = 0;
					break;
				}
			}
			if (_zmbTraitAssign[4 * m] > 0) {
				for (int16 v6 = 0; v6 < 4; v6++) {
					int16 v7 = kAttrOffsets[v6] + _zmbTraitAssign[4 * m + v6];
					_slotScores[v7]++;
				}
			}
		}
	}
	return bestSlot;
}

int16 ZoombiniPuzzleMaze::commitBestAttrSlot(int16 maxThreshold, int16 minThreshold) {
	// IDA: net_commitBestAttrSlot (0x432B44)
	int16 bestSlot = 0;
	int16 bestScore = 0;

	for (int16 iterSlot = 1; iterSlot < 21; iterSlot++) {
		if (_slotScores[iterSlot] <= bestScore
			|| _slotScores[iterSlot] < minThreshold
			|| _slotScores[iterSlot] > maxThreshold)
			continue;

		for (int16 zmbI = 0; zmbI < _loadedZmbCount; zmbI++) {
			bool foundMatch = false;
			for (int16 traitJ = 0; traitJ < 4; traitJ++) {
				if (_zmbTraitAssign[4 * zmbI + traitJ] <= 0)
					continue;
				if (kAttrOffsets[traitJ] + _zmbTraitAssign[4 * zmbI + traitJ] != iterSlot)
					continue;

				int16 isUnique = 1;
				for (int16 k = 0; k < 20; k++) {
					if (_uniqueCheckArr[4 * k + traitJ] > 0
						&& _uniqueCheckArr[4 * k + traitJ] == _zmbTraitAssign[4 * zmbI + traitJ])
						isUnique = 0;
				}
				if (isUnique) {
					bestSlot = iterSlot;
					bestScore = _slotScores[iterSlot];
					foundMatch = true;
				}
				break;
			}
			if (foundMatch)
				break;
		}
	}

	if (bestSlot) {
		memset(_slotScores, 0, sizeof(_slotScores));

		for (int16 m = 0; m < _loadedZmbCount; m++) {
			for (int16 n = 0; n < 4; n++) {
				if (_zmbTraitAssign[4 * m + n]
					&& kAttrOffsets[n] + _zmbTraitAssign[4 * m + n] == bestSlot) {
					// Record in BOTH arrays
					if (_committedTraitCount < 20) {
						_committedTraitArr[4 * _committedTraitCount + 0] = _zmbTraitAssign[4 * m + 0];
						_committedTraitArr[4 * _committedTraitCount + 1] = _zmbTraitAssign[4 * m + 1];
						_committedTraitArr[4 * _committedTraitCount + 2] = _zmbTraitAssign[4 * m + 2];
						_committedTraitArr[4 * _committedTraitCount + 3] = _zmbTraitAssign[4 * m + 3];
						_uniqueCheckArr[4 * _committedTraitCount + 0] = _zmbTraitAssign[4 * m + 0];
						_uniqueCheckArr[4 * _committedTraitCount + 1] = _zmbTraitAssign[4 * m + 1];
						_uniqueCheckArr[4 * _committedTraitCount + 2] = _zmbTraitAssign[4 * m + 2];
						_uniqueCheckArr[4 * _committedTraitCount + 3] = _zmbTraitAssign[4 * m + 3];
						_committedTraitCount++;
					}
					// Clear zmb
					_zmbTraitAssign[4 * m + 0] = 0;
					_zmbTraitAssign[4 * m + 1] = 0;
					_zmbTraitAssign[4 * m + 2] = 0;
					_zmbTraitAssign[4 * m + 3] = 0;
					break;
				}
			}
			// Rebuild scores
			if (_zmbTraitAssign[4 * m] > 0) {
				for (int16 v7 = 0; v7 < 4; v7++) {
					int16 v8 = kAttrOffsets[v7] + _zmbTraitAssign[4 * m + v7];
					_slotScores[v8]++;
				}
			}
		}
	}
	return bestSlot;
}

int16 ZoombiniPuzzleMaze::findAndCommitNextSlot(int16 slotIdx, int16 direction) {
	// IDA: net_findAndCommitNextSlot (0x4323DF)
	int16 bestSlot = 0;
	int16 bestZmb = -1;
	int16 bestTrait = 0;
	int16 minScore = 21;

	for (int16 iterSlot = 1; iterSlot < 21; iterSlot++) {
		if (_slotScores[iterSlot] <= 0 || _slotScores[iterSlot] > minScore || direction == iterSlot)
			continue;

		for (int16 zmbI = 0; zmbI < _loadedZmbCount; zmbI++) {
			bool foundMatch = false;
			for (int16 traitJ = 0; traitJ < 4; traitJ++) {
				if (_zmbTraitAssign[4 * zmbI + traitJ] <= 0)
					continue;
				if (kAttrOffsets[traitJ] + _zmbTraitAssign[4 * zmbI + traitJ] != iterSlot)
					continue;

				// Check ALL 4 columns for uniqueness
				int16 isUnique = 1;
				for (int16 k = 0; k < 20 && _committedTraitCount < 3; k++) {
					if (_uniqueCheckArr[4 * k + 0] > 0 && _uniqueCheckArr[4 * k + 0] == _zmbTraitAssign[4 * zmbI + 0])
						isUnique = 0;
					else if (_uniqueCheckArr[4 * k + 1] > 0 && _uniqueCheckArr[4 * k + 1] == _zmbTraitAssign[4 * zmbI + 1])
						isUnique = 0;
					else if (_uniqueCheckArr[4 * k + 2] > 0 && _uniqueCheckArr[4 * k + 2] == _zmbTraitAssign[4 * zmbI + 2])
						isUnique = 0;
					else if (_uniqueCheckArr[4 * k + 3] > 0 && _uniqueCheckArr[4 * k + 3] == _zmbTraitAssign[4 * zmbI + 3])
						isUnique = 0;
				}
				if (isUnique) {
					bestSlot = iterSlot;
					bestZmb = zmbI;
					bestTrait = traitJ;
					minScore = _slotScores[iterSlot];
					foundMatch = true;
				}
				break;
			}
			if (foundMatch)
				break;
		}
	}

	if (bestSlot) {
		if (_committedTraitCount < 4) {
			if (slotIdx) {
				// Mode 1: store ALL 4 traits
				_committedTraitArr[4 * _committedTraitCount + 0] = _zmbTraitAssign[4 * bestZmb + 0];
				_committedTraitArr[4 * _committedTraitCount + 1] = _zmbTraitAssign[4 * bestZmb + 1];
				_committedTraitArr[4 * _committedTraitCount + 2] = _zmbTraitAssign[4 * bestZmb + 2];
				_committedTraitArr[4 * _committedTraitCount + 3] = _zmbTraitAssign[4 * bestZmb + 3];
				_uniqueCheckArr[4 * _committedTraitCount + 0] = _zmbTraitAssign[4 * bestZmb + 0];
				_uniqueCheckArr[4 * _committedTraitCount + 1] = _zmbTraitAssign[4 * bestZmb + 1];
				_uniqueCheckArr[4 * _committedTraitCount + 2] = _zmbTraitAssign[4 * bestZmb + 2];
				_uniqueCheckArr[4 * _committedTraitCount + 3] = _zmbTraitAssign[4 * bestZmb + 3];
			} else {
				// Mode 0: store only matching trait column
				_uniqueCheckArr[4 * _committedTraitCount + bestTrait] = _zmbTraitAssign[4 * bestZmb + bestTrait];
				_committedTraitArr[4 * _committedTraitCount + 0] = _zmbTraitAssign[4 * bestZmb + 0];
				_committedTraitArr[4 * _committedTraitCount + 1] = _zmbTraitAssign[4 * bestZmb + 1];
				_committedTraitArr[4 * _committedTraitCount + 2] = _zmbTraitAssign[4 * bestZmb + 2];
				_committedTraitArr[4 * _committedTraitCount + 3] = _zmbTraitAssign[4 * bestZmb + 3];
			}
			_committedTraitCount++;
		}

		// Clear zmb and rebuild scores
		_zmbTraitAssign[4 * bestZmb + 0] = 0;
		_zmbTraitAssign[4 * bestZmb + 1] = 0;
		_zmbTraitAssign[4 * bestZmb + 2] = 0;
		_zmbTraitAssign[4 * bestZmb + 3] = 0;
		memset(_slotScores, 0, sizeof(_slotScores));
		for (int16 m = 0; m < _loadedZmbCount; m++) {
			if (_zmbTraitAssign[4 * m]) {
				for (int16 v6 = 0; v6 < 4; v6++) {
					int16 v7 = kAttrOffsets[v6] + _zmbTraitAssign[4 * m + v6];
					_slotScores[v7]++;
				}
			}
		}
	}
	return bestSlot;
}

int16 ZoombiniPuzzleMaze::findAndCommitNewAttrSlot(int16 maxThreshold, int16 minThreshold) {
	// IDA: net_findAndCommitNewAttrSlot (0x432E6B)
	int16 bestSlot = 0;
	int16 bestScore = 0;

	for (int16 iterSlot = 1; iterSlot < 21; iterSlot++) {
		if (_slotScores[iterSlot] <= bestScore
			|| _slotScores[iterSlot] < minThreshold
			|| _slotScores[iterSlot] > maxThreshold)
			continue;

		for (int16 zmbI = 0; zmbI < _loadedZmbCount; zmbI++) {
			bool foundMatch = false;
			for (int16 traitJ = 0; traitJ < 4; traitJ++) {
				if (_zmbTraitAssign[4 * zmbI + traitJ] <= 0)
					continue;
				if (kAttrOffsets[traitJ] + _zmbTraitAssign[4 * zmbI + traitJ] != iterSlot)
					continue;

				// Check ALL 4 columns against _committedTraitArr
				int16 isUnique = 1;
				for (int16 k = 0; k < 20; k++) {
					if (_committedTraitArr[4 * k + 0] > 0 && _committedTraitArr[4 * k + 0] == _zmbTraitAssign[4 * zmbI + 0])
						isUnique = 0;
					else if (_committedTraitArr[4 * k + 1] > 0 && _committedTraitArr[4 * k + 1] == _zmbTraitAssign[4 * zmbI + 1])
						isUnique = 0;
					else if (_committedTraitArr[4 * k + 2] > 0 && _committedTraitArr[4 * k + 2] == _zmbTraitAssign[4 * zmbI + 2])
						isUnique = 0;
					else if (_committedTraitArr[4 * k + 3] > 0 && _committedTraitArr[4 * k + 3] == _zmbTraitAssign[4 * zmbI + 3])
						isUnique = 0;
				}
				if (isUnique) {
					bestSlot = iterSlot;
					bestScore = _slotScores[iterSlot];
					foundMatch = true;
				}
				break;
			}
			if (foundMatch)
				break;
		}
	}

	if (bestSlot) {
		memset(_slotScores, 0, sizeof(_slotScores));

		for (int16 m = 0; m < _loadedZmbCount; m++) {
			for (int16 n = 0; n < 4; n++) {
				if (_zmbTraitAssign[4 * m + n]
					&& kAttrOffsets[n] + _zmbTraitAssign[4 * m + n] == bestSlot) {
					// Record in _committedTraitArr ONLY
					if (_committedTraitCount < 20) {
						_committedTraitArr[4 * _committedTraitCount + 0] = _zmbTraitAssign[4 * m + 0];
						_committedTraitArr[4 * _committedTraitCount + 1] = _zmbTraitAssign[4 * m + 1];
						_committedTraitArr[4 * _committedTraitCount + 2] = _zmbTraitAssign[4 * m + 2];
						_committedTraitArr[4 * _committedTraitCount + 3] = _zmbTraitAssign[4 * m + 3];
						_committedTraitCount++;
					}
					_zmbTraitAssign[4 * m + 0] = 0;
					_zmbTraitAssign[4 * m + 1] = 0;
					_zmbTraitAssign[4 * m + 2] = 0;
					_zmbTraitAssign[4 * m + 3] = 0;
					break;
				}
			}
			if (_zmbTraitAssign[4 * m] > 0) {
				for (int16 v7 = 0; v7 < 4; v7++) {
					int16 v8 = kAttrOffsets[v7] + _zmbTraitAssign[4 * m + v7];
					_slotScores[v8]++;
				}
			}
		}
	}
	return bestSlot;
}

// =================================================================
// Path selection algorithms
// =================================================================

void ZoombiniPuzzleMaze::buildZmbAssignmentAlt2() {
	// IDA: net_buildZmbAssignmentAlt2 (0x43335F) - Difficulty 0
	collectZmbAttrs();
	countZmbAttrMatches(0);
	initConnectionTable();
	rebuildReachabilityList();
	if (_reachableSlotCount <= 0)
		initAllSlotsReachable();

	// Select first slot via tiered search
	int16 firstSlot = findBestAttrSlotInRange(2, 5);
	if (!firstSlot) firstSlot = findBestAttrSlotInRange(6, 9);
	if (!firstSlot) firstSlot = findBestAttrSlotInRange(10, 16);
	if (!firstSlot) firstSlot = findBestAttrSlotInRange(1, 16);
	if (!firstSlot && _loadedZmbCount <= 4)
		firstSlot = findBestAttrSlotInRange(1, 2);
	if (!firstSlot) {
		int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
		firstSlot = _reachableSlots[randIdx];
	}

	_selectedPathSlots[_pathSlotWriteIdx++] = firstSlot;
	_selectedPathSlots[_pathSlotWriteIdx++] = firstSlot; // Duplicate slot[1] = slot[0]

	if (_difficultyLevel == kPuzzleDiffLevel3)
		_selectedPathSlots[_pathSlotWriteIdx++] = firstSlot; // Extra dup at diff 3

	// Count matches for the selected slot
	collectMatchingZmbAttrs(firstSlot);
	countZmbAttrMatches(0);
	initConnectionTable();
	rebuildReachabilityList();

	// Select final 2 slots
	int16 reachCount = countReachableSlots();
	if (reachCount <= 4) {
		// Random fallback
		int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
		int16 slot1 = _reachableSlots[randIdx];
		int16 slot2 = findHighestScoredSlot(slot1);
		if (!slot2) slot2 = slot1;

		if (_difficultyLevel >= kPuzzleDiffLevel3 && _slotScores[slot2] > _slotScores[slot1]) {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
		} else {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
		}
	} else {
		int16 v33 = countZmbAttrMatches(firstSlot);
		int16 slot1 = findBestAttrSlotInRange(1, (v33 < 20) ? kPathSelectThresholds[v33] : 8);
		if (!slot1)
			slot1 = findBestAttrSlotInRange(1, 16);
		if (!slot1) {
			int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
			slot1 = _reachableSlots[randIdx];
		}
		int16 slot2 = findHighestScoredSlotInRange(slot1, 1, 16);
		if (!slot2) slot2 = findHighestScoredSlot(slot1);
		if (!slot2) slot2 = slot1;

		if (_slotScores[slot2] > _slotScores[slot1]) {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
		} else {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
		}
	}
}

void ZoombiniPuzzleMaze::buildZmbAssignmentAlt() {
	// IDA: net_buildZmbAssignmentAlt (0x4335EF) - Difficulty 1 Variant 0
	// Nearly identical to Alt2 but always has TWO forced duplicates
	collectZmbAttrs();
	countZmbAttrMatches(0);
	initConnectionTable();
	rebuildReachabilityList();
	if (_reachableSlotCount <= 0)
		initAllSlotsReachable();

	int16 firstSlot = findBestAttrSlotInRange(2, 5);
	if (!firstSlot) firstSlot = findBestAttrSlotInRange(6, 9);
	if (!firstSlot) firstSlot = findBestAttrSlotInRange(10, 16);
	if (!firstSlot) firstSlot = findBestAttrSlotInRange(1, 16);
	if (!firstSlot && _loadedZmbCount <= 4)
		firstSlot = findBestAttrSlotInRange(1, 2);
	if (!firstSlot) {
		int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
		firstSlot = _reachableSlots[randIdx];
	}

	_selectedPathSlots[_pathSlotWriteIdx++] = firstSlot;
	_selectedPathSlots[_pathSlotWriteIdx++] = firstSlot; // Duplicate
	_selectedPathSlots[_pathSlotWriteIdx++] = firstSlot; // Triple (always)

	collectMatchingZmbAttrs(firstSlot);
	countZmbAttrMatches(0);
	initConnectionTable();
	rebuildReachabilityList();

	int16 reachCount = countReachableSlots();
	if (reachCount <= 4) {
		int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
		int16 slot1 = _reachableSlots[randIdx];
		int16 slot2 = findHighestScoredSlot(slot1);
		if (!slot2) slot2 = slot1;

		if (_difficultyLevel >= kPuzzleDiffLevel3 && _slotScores[slot2] > _slotScores[slot1]) {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
		} else {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
		}
	} else {
		int16 v33 = countZmbAttrMatches(firstSlot);
		int16 slot1 = findBestAttrSlotInRange(1, (v33 < 20) ? kPathSelectThresholds[v33] : 8);
		if (!slot1)
			slot1 = findBestAttrSlotInRange(1, 16);
		if (!slot1) {
			int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
			slot1 = _reachableSlots[randIdx];
		}
		int16 slot2 = findHighestScoredSlotInRange(slot1, 1, 16);
		if (!slot2) slot2 = findHighestScoredSlot(slot1);
		if (!slot2) slot2 = slot1;

		if (_slotScores[slot2] > _slotScores[slot1]) {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
		} else {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
		}
	}
}

void ZoombiniPuzzleMaze::selectPathSlots2() {
	// IDA: net_selectPathSlots2 (0x4338A1)
	_committedTraitCount = 0;
	memset(_uniqueCheckArr, 0, sizeof(_uniqueCheckArr));
	memset(_committedTraitArr, 0, sizeof(_committedTraitArr));

	collectZmbAttrs();
	countZmbAttrMatches(0);
	initConnectionTable();
	rebuildReachabilityList();
	if (_reachableSlotCount <= 0)
		initAllSlotsReachable();

	// Phase 1 - Select first slot
	int16 firstSlot = findBestAttrSlotInRange(2, 5);
	if (!firstSlot) firstSlot = findBestAttrSlotInRange(6, 9);
	if (!firstSlot) firstSlot = findBestAttrSlotInRange(10, 16);
	if (!firstSlot) firstSlot = findBestAttrSlotInRange(1, 16);
	if (!firstSlot && _loadedZmbCount <= 4)
		firstSlot = findBestAttrSlotInRange(1, 2);
	if (!firstSlot) {
		int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
		firstSlot = _reachableSlots[randIdx];
	}

	_selectedPathSlots[_pathSlotWriteIdx++] = firstSlot;
	_selectedPathSlots[_pathSlotWriteIdx++] = firstSlot; // Duplicate

	collectMatchingZmbAttrs(firstSlot);
	countZmbAttrMatches(0);

	_pathSlotWriteIdx = 2; // Reset to write position 2
	initFreeColumnList();
	collectActiveColumns();

	// Phase 2 - Select middle slots (2-5)
	for (int16 phase = 0; phase < 4; phase++) {
		int16 midSlot = findBestNextSlot(firstSlot);
		if (!midSlot) {
			if (_activeColumnsExist) {
				int16 randIdx = _vm->_rnd->getRandomNumber(1, _activeColumnCount);
				midSlot = _activeColumns[randIdx];
			} else {
				int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
				midSlot = _reachableSlots[randIdx];
			}
		}
		if (_unknownFlag == 1 && phase == 0) {
			// Duplicate prev slot (avoid UB from sequence point)
			int16 prevSlot = _selectedPathSlots[_pathSlotWriteIdx - 2];
			_selectedPathSlots[_pathSlotWriteIdx++] = prevSlot;
		}
		_selectedPathSlots[_pathSlotWriteIdx++] = midSlot;
	}

	// Phase 3 - Select final 2 slots
	collectZmbAttrs();
	countZmbAttrMatches(firstSlot);
	initConnectionTable();
	rebuildReachabilityList();

	int16 reachCount = countReachableSlots();
	if (reachCount <= 4) {
		int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
		int16 slot1 = _reachableSlots[randIdx];
		int16 slot2 = findHighestScoredSlot(slot1);
		if (!slot2) slot2 = slot1;

		if (_difficultyLevel >= kPuzzleDiffLevel3 && _slotScores[slot2] > _slotScores[slot1]) {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
		} else {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
		}
	} else {
		int16 v36 = countZmbAttrMatches(firstSlot);
		int16 slot1 = findBestAttrSlotInRange(1, (v36 < 20) ? kPathSelectThresholds[v36] : 8);
		if (!slot1)
			slot1 = findBestAttrSlotInRange(1, 16);
		if (!slot1) {
			if (_activeColumnsExist) {
				int16 randIdx = _vm->_rnd->getRandomNumber(1, _activeColumnCount);
				slot1 = _activeColumns[randIdx];
			} else {
				int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
				slot1 = _reachableSlots[randIdx];
			}
		}
		int16 slot2 = findHighestScoredSlotInRange(slot1, 1, 16);
		if (!slot2) slot2 = findHighestScoredSlot(slot1);
		if (!slot2) slot2 = slot1;

		if (_slotScores[slot2] > _slotScores[slot1]) {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
		} else {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
		}
	}
}

void ZoombiniPuzzleMaze::selectPathSlots() {
	// IDA: net_selectPathSlots (0x433D30)
	_committedTraitCount = 0;
	memset(_uniqueCheckArr, 0, sizeof(_uniqueCheckArr));
	memset(_committedTraitArr, 0, sizeof(_committedTraitArr));

	collectZmbAttrs();
	countZmbAttrMatches(0);
	initConnectionTable();
	rebuildReachabilityList();
	if (_reachableSlotCount <= 0)
		initAllSlotsReachable();

	// Pre-compute v58
	int16 v58 = findBestNextSlot(0);

	// Phase 1: commitBestAttrSlot with tiers
	int16 firstSlot = commitBestAttrSlot(4, 2);
	if (!firstSlot) firstSlot = commitBestAttrSlot(8, 5);
	if (!firstSlot) firstSlot = commitBestAttrSlot(12, 9);
	if (!firstSlot) firstSlot = commitBestAttrSlot(16, 1);
	if (!firstSlot && _loadedZmbCount <= 4)
		firstSlot = commitBestAttrSlot(1, 1);
	if (!firstSlot) {
		int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
		firstSlot = _reachableSlots[randIdx];
	}

	_selectedPathSlots[_pathSlotWriteIdx++] = firstSlot;
	_selectedPathSlots[_pathSlotWriteIdx++] = firstSlot; // Duplicate

	collectMatchingZmbAttrs(firstSlot);
	countZmbAttrMatches(0);
	_pathSlotWriteIdx = 2;
	initFreeColumnList();
	collectActiveColumns();

	// Phase 2: Append v58 twice, then 2x findBestNextSlot with fallbacks
	_selectedPathSlots[_pathSlotWriteIdx++] = v58;
	_selectedPathSlots[_pathSlotWriteIdx++] = v58;

	for (int16 phase = 0; phase < 2; phase++) {
		int16 midSlot = findBestNextSlot(firstSlot);
		if (!midSlot) {
			if (_activeColumnsExist) {
				int16 randIdx = _vm->_rnd->getRandomNumber(1, _activeColumnCount);
				midSlot = _activeColumns[randIdx];
			} else {
				int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
				midSlot = _reachableSlots[randIdx];
			}
		}
		_selectedPathSlots[_pathSlotWriteIdx++] = midSlot;
	}

	// Phase 3: Re-collect and select final 2
	collectZmbAttrs();
	countZmbAttrMatches(firstSlot);
	countZmbAttrMatches(v58);
	initFreeColumnList();
	collectActiveColumns();

	int16 reachCount = countReachableSlots();
	if (reachCount <= 4) {
		int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
		int16 slot1 = _reachableSlots[randIdx];
		int16 slot2 = findHighestScoredSlot(slot1);
		if (!slot2) slot2 = slot1;

		if (_difficultyLevel >= kPuzzleDiffLevel3 && _slotScores[slot2] > _slotScores[slot1]) {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
		} else {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
		}
	} else {
		int16 v36 = countZmbAttrMatches(firstSlot);
		int16 slot1 = findBestAttrSlotInRange(1, (v36 < 20) ? kPathSelectThresholds[v36] : 8);
		if (!slot1)
			slot1 = findBestAttrSlotInRange(1, 16);
		if (!slot1) {
			if (_activeColumnsExist) {
				int16 randIdx = _vm->_rnd->getRandomNumber(1, _activeColumnCount);
				slot1 = _activeColumns[randIdx];
			} else {
				int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
				slot1 = _reachableSlots[randIdx];
			}
		}
		int16 slot2 = findHighestScoredSlotInRange(slot1, 1, 16);
		if (!slot2) slot2 = findHighestScoredSlot(slot1);
		if (!slot2) slot2 = slot1;

		if (_slotScores[slot2] > _slotScores[slot1]) {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
		} else {
			_selectedPathSlots[_pathSlotWriteIdx++] = slot1;
			_selectedPathSlots[_pathSlotWriteIdx++] = slot2;
		}
	}
}

void ZoombiniPuzzleMaze::buildZmbAssignmentList() {
	// IDA: net_buildZmbAssignmentList (0x434159) - Difficulty 3
	_committedTraitCount = 0;
	memset(_uniqueCheckArr, 0, sizeof(_uniqueCheckArr));
	memset(_committedTraitArr, 0, sizeof(_committedTraitArr));

	collectZmbAttrs();
	countZmbAttrMatches(0);
	initConnectionTable();
	rebuildReachabilityList();
	if (_reachableSlotCount <= 0)
		initAllSlotsReachable();

	// Phase 1: First 3 slots via findAndCommitNextSlot
	for (int16 phase = 0; phase < 3; phase++) {
		int16 slot = findAndCommitNextSlot(0, 0);
		if (!slot) {
			int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
			slot = _reachableSlots[randIdx];
		}
		_selectedPathSlots[_pathSlotWriteIdx++] = slot;
	}

	// Phase 2: 4th slot
	int16 v44 = findAndCommitNextSlot(0, 0);
	if (!v44) {
		int16 randIdx = _vm->_rnd->getRandomNumber(1, _reachableSlotCount);
		v44 = _reachableSlots[randIdx];
	}

	// Phase 3: Middle slots using dynamic thresholds
	collectZmbAttrs();
	countZmbAttrMatches(_selectedPathSlots[0]);
	countZmbAttrMatches(_selectedPathSlots[1]);
	countZmbAttrMatches(_selectedPathSlots[2]);

	int16 matchCountForSlot2 = getAttrMatchCount(_selectedPathSlots[2]);
	int16 loopCount = (matchCountForSlot2 < 17) ? kScoreToLoopCount[matchCountForSlot2] : 16;

	int16 v40[20];
	memset(v40, 0, sizeof(v40));
	int16 v40Count = 0;

	for (int16 iter = 0; iter < loopCount; iter++) {
		int16 midSlot = commitBestAttrSlot(16, 1);
		if (!midSlot) {
			int16 randIdx = _vm->_rnd->getRandomNumber(1, 20);
			midSlot = randIdx;
		}
		if (iter == 0) {
			_selectedPathSlots[_pathSlotWriteIdx++] = midSlot;
		} else {
			if (v40Count < 20)
				v40[v40Count++] = midSlot;
		}
	}

	// Append stored slots
	for (int16 i = 0; i < v40Count && _pathSlotWriteIdx < 20; i++)
		_selectedPathSlots[_pathSlotWriteIdx++] = v40[i];

	// Append v44
	if (_pathSlotWriteIdx < 20)
		_selectedPathSlots[_pathSlotWriteIdx++] = v44;

	// Phase 4: Two more via findAndCommitNewAttrSlot
	collectZmbAttrs();
	countZmbAttrMatches(0);
	initConnectionTable();
	rebuildReachabilityList();

	for (int16 phase = 0; phase < 2; phase++) {
		int16 slot = findAndCommitNewAttrSlot(3, 1);
		if (!slot) {
			int16 randIdx = _vm->_rnd->getRandomNumber(1, 20);
			slot = randIdx;
		}
		if (_pathSlotWriteIdx < 20)
			_selectedPathSlots[_pathSlotWriteIdx++] = slot;
	}

	// Phase 5: Two random slots
	for (int16 phase = 0; phase < 2; phase++) {
		int16 randSlot = _vm->_rnd->getRandomNumber(1, 20);
		if (_pathSlotWriteIdx < 20)
			_selectedPathSlots[_pathSlotWriteIdx++] = randSlot;
	}
}

// =================================================================
// Grid runner initialization
// IDA: net_initGridRunners (0x431C3A), net_registerGridRunner (0x431D02)
// =================================================================

void ZoombiniPuzzleMaze::initGridRunners() {
	// IDA: net_initGridRunners_431C3A
	_gridRegsReadIdx = 0;
	_runnerCount = 0;
	memset(_waveGroupCount, 0, sizeof(_waveGroupCount));
	for (int16 i = 0; i < _totalCreatureCount && i < kMaxRunners; i++) {
		registerGridRunner();
	}
}

void ZoombiniPuzzleMaze::registerGridRunner() {
	// IDA: net_registerGridRunner (0x431D02)
	// Read 10 words from REGS data per runner.
	// REGS layout per runner (verified from IDA + hex dump):
	//   [0]=cellType, [1]=row, [2]=col, [3]=waveGroup,
	//   [4]=dirFlag0, [5]=dirFlag1, [6]=dirFlag2, [7]=dirFlag3,
	//   [8]=direction, [9]=cycleFlag
	if (_gridRegsReadIdx >= static_cast<int16>(_regsData.size()) / 10)
		return;

	int16 baseOff = 10 + 10 * _gridRegsReadIdx; // Skip 10-word header
	if (baseOff + 10 > static_cast<int16>(_regsData.size()))
		return;

	int16 cellType  = _regsData[baseOff + 0];
	int16 row       = _regsData[baseOff + 1];
	int16 col       = _regsData[baseOff + 2];
	int16 waveGroup = _regsData[baseOff + 3];
	int16 dirFlag0  = _regsData[baseOff + 4];
	int16 dirFlag1  = _regsData[baseOff + 5];
	int16 dirFlag2  = _regsData[baseOff + 6];
	int16 dirFlag3  = _regsData[baseOff + 7];
	int16 direction = _regsData[baseOff + 8];
	int16 cycleFlag = _regsData[baseOff + 9];

	_gridRegsReadIdx++;

	if (row < 0 || row >= kGridRows || col < 0 || col >= kGridCols)
		return;

	// Store in grid arrays
	_cellTypes[row][col] = cellType;
	_cellRunnerIdx[row][col] = _runnerCount;

	// Store per-node direction data. IDA: core_word[34..39]
	_nodeDirFlags[row][col][0] = dirFlag0;
	_nodeDirFlags[row][col][1] = dirFlag1;
	_nodeDirFlags[row][col][2] = dirFlag2;
	_nodeDirFlags[row][col][3] = dirFlag3;
	_nodeDirection[row][col] = direction;
	_nodeCycleFlag[row][col] = cycleFlag;

	// For type-2 cells (zmb start / attribute routing), assign trait from path selection output.
	// IDA: hsArr[13].posY = word_4A1FC4[2*slotIdx] + 1; hsArr[14].shapeid = word_4A1FC6[2*slotIdx];
	if (cellType == 2) {
		if (_pathSlotReadIdx < _pathSlotWriteIdx) {
			int16 slotIdx = _selectedPathSlots[_pathSlotReadIdx];
			if (slotIdx >= 0 && slotIdx <= 20) {
				_cellAttrType[row][col] = kAttrSlotType[slotIdx] + 1;   // 1=hair, 2=eyes, 3=nose, 4=feet
				_cellAttrValue[row][col] = kAttrSlotValue[slotIdx];     // 1-5
			}
			_pathSlotReadIdx++;
		}
	}

	// Create runner state
	if (_runnerCount < kMaxRunners) {
		ZmbMazeRunnerState &rs = _runnerStates[_runnerCount];
		rs.clear();
		rs.row = row;
		rs.col = col;
		rs.oldRow = row;
		rs.oldCol = col;
		rs.cellTypeAtPos = cellType;
		rs.waveGroup = waveGroup;

		// Assign to wave group. IDA: word_4B00E0[word_4B03CE++] = runnerIdx (switch waveGroup)
		if (waveGroup >= 1 && waveGroup <= 8) {
			int16 g = waveGroup - 1;
			if (_waveGroupCount[g] < kMaxRunners)
				_waveGroupRunners[g][_waveGroupCount[g]++] = _runnerCount;
		}

		_runnerCount++;
	}

	debugC(2, kZmbDebugAnimation, "Maze: Registered grid runner row=%d col=%d type=%d dir=%d cycle=%d wave=%d attr=%d/%d",
	       row, col, cellType, direction, cycleFlag, waveGroup,
	       _cellAttrType[row][col], _cellAttrValue[row][col]);
}

// =================================================================
// Helpers
// =================================================================

byte ZoombiniPuzzleMaze::getTraitByCategory(const ZmbTrait &trait, int16 category) {
	switch (category) {
	case 1: return trait._head;
	case 2: return trait._eye;
	case 3: return trait._nose;
	case 4: return trait._foot;
	default: return 0;
	}
}

int16 ZoombiniPuzzleMaze::getRunnerAnimBase(byte footTrait) const {
	if (footTrait >= 1 && footTrait <= 5)
		return _scrsAnimTable[footTrait - 1];
	return _scrsAnimTable[0];
}

ZmbMazeRunnerState *ZoombiniPuzzleMaze::getRunnerState(int16 idx) {
	if (idx < 0 || idx >= kMaxRunners)
		return nullptr;
	return &_runnerStates[idx];
}

int16 ZoombiniPuzzleMaze::findRunnerBySnoidId(uint16 snoidId) const {
	for (int16 i = 0; i < _runnerCount; i++) {
		if (_zmbRunnerSnoidIds[i] == snoidId)
			return i;
	}
	return -1;
}

int16 ZoombiniPuzzleMaze::findRunnerByFeatureId(uint16 featureId) const {
	for (int16 i = 0; i < _runnerCount; i++) {
		if (_zmbRunnerSnoidIds[i] == featureId)
			return i;
	}
	return -1;
}

int16 ZoombiniPuzzleMaze::findSeatByFeatureId(uint16 featureId) const {
	// IDA: runnerPtr[69] = node index. Grid node features are stored in
	// _creatureObstacleFeatures[i] and _creatureShadowFeatures[i].
	for (int16 i = 0; i < 14; i++) {
		if (_creatureObstacleFeatures[i] && _creatureObstacleFeatures[i]->getId() == featureId)
			return i;
		if (_creatureShadowFeatures[i] && _creatureShadowFeatures[i]->getId() == featureId)
			return i;
	}
	return -1;
}

// =================================================================
// Animation event dispatch
// IDA: net_scrbAnimCallback (0x43105B), net_trackRunnerCollisions (0x431354)
// =================================================================

void ZoombiniPuzzleMaze::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// In the original engine, net_scrbAnimCallback (0x43105B) and
	// net_trackRunnerCollisions (0x431354) are SEPARATE callbacks registered
	// on different features. In ScummVM we have one dispatch point.
	// Collision codes (20-61) only come from snoid features (walking runners).
	// ASCII SCRB codes ('2'=50, '='=61 etc.) come from SCRB features.
	// Since values 50='2' and 61='=' overlap, we dispatch by feature type.

	switch (eventCode) {
	case kZmbAnimEventM1_End:
		// IDA: net_processRunnerExitFrame (0x430F06) — fires when SCRS 14006 ends.
		// If this snoid is exiting (rs.exiting=true), push to arrival queue.
		if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			int16 runnerIdx = findRunnerByFeatureId(feature->getId());
			if (runnerIdx >= 0) {
				ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
				if (rs.exiting) {
					rs.exiting = false;
					if (_arrivalQueueSize < kMaxQueueSize)
						_arrivalQueue[_arrivalQueueSize++] = runnerIdx;
				}
			}
		}
		break;

	case 0:
		// Toggle render + handle pending body arrangement
		if (feature->isRenderActivated())
			feature->deactivateRender();
		else
			feature->activateRender();
		if (_pendingBodyArrangement && feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			static_cast<ZmbSnoid *>(feature)->setBodyArrangement(_pendingBodyArrangement - 1);
			_pendingBodyArrangement = 0;
		}
		break;

	case 15:
		// Obstacle overlay linking
		break;

	default:
		if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			// Snoid features -> collision/row-change codes.
			// IDA: net_trackRunnerCollisions (0x431354)
			switch (eventCode) {
			case 20: case 30: case 40: case 50:
			case 21: case 31: case 41: case 51: case 61:
				{
					int16 runnerIdx = findRunnerByFeatureId(feature->getId());
					if (runnerIdx >= 0)
						handleCollisionTracking(eventCode, runnerIdx);

					// IDA maze_runnerArriveAtNode_425ADB @ 0x425b65: on a path-node
					// arrival event, the first arrival (++lilly_stateVar8 == 1) sets
					// lilly_bAdvanceEnabled = 1, enabling the Go button.  Mirror that
					// here so the button unlocks once a Zoombini starts crossing.
					if (eventCode == 30 && ++_nodeArrivalCount == 1)
						_bAdvanceEnabled = true;
				}
				break;
			default:
				break;
			}
		} else {
			// Non-snoid features -> SCRB frame event ASCII codes.
			// IDA: net_scrbAnimCallback (0x43105B)
			switch (eventCode) {
			case '2': // 50 - Load resting SCRB on companion
			case '=': // 61 - Play traversal script (mode 0)
			case '>': // 62 - Setup traversal step (mode 0)
			case '@': // 64 - Push to column-link queue
			case 'A': // 65 - Spawn traversal runner (mode 1)
			case 'B': // 66 - Clear slot + seat assignment
			case 'G': // 71 - Play traversal script (mode 0)
			case 'H': // 72 - Setup traversal step (mode 1)
			case 'J': // 74 - Push to column-link queue + clear companion
			case 'K': // 75 - Spawn traversal runner (mode 0)
			case 'L': // 76 - Clear slot + seat assignment
			case 'Q': // 81 - Play traversal script (mode 0)
			case 'R': // 82 - Setup traversal step (mode 1)
			case 'T': // 84 - Push to column-link queue
			case 'U': // 85 - Spawn traversal runner (mode 0)
			case 'V': // 86 - Clear slot + seat assignment
				processScrbAnimEvent(feature, eventCode);
				break;
			default:
				break;
			}
		}

		// Body arrangement events (shared by all feature types)
		if (eventCode >= kZmbAnimEvent240_BodyArrangePendFirst &&
		    eventCode <= kZmbAnimEvent243_BodyArrangePendLast) {
			_pendingBodyArrangement = eventCode - 239;
		} else if (eventCode >= kZmbAnimEvent250_BodyArrangeDirectFirst &&
		           eventCode <= kZmbAnimEvent253_BodyArrangeDirectLast) {
			if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
				static_cast<ZmbSnoid *>(feature)->setBodyArrangement(eventCode - 250);
			}
		}
		break;
	}
}

void ZoombiniPuzzleMaze::processScrbAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// IDA: net_scrbAnimCallback (0x43105B)
	// Find which seat index this creature-obstacle feature corresponds to.
	// runnerPtr[68] = slot index; runnerPtr[69] = node index (same value here).
	int16 seatIdx = findSeatByFeatureId(feature->getId());
	if (seatIdx < 0)
		return;

	// Find the zmb snoid runner currently placed at this seat.
	// IDA: runner_findByIndex(word_4AF3F6[runnerPtr[69]]) = companion snoid runner.
	int16 runnerIdx = -1;
	for (int16 i = 0; i < _runnerCount; i++) {
		if (_zmbRunnerSnoidIds[i] != 0 && _runnerStates[i].seatIdx == seatIdx && _runnerStates[i].placed) {
			runnerIdx = i;
			break;
		}
	}

	switch (eventCode) {
	case '2': // Load resting SCRB on companion (shadow) runner
		// IDA: foundRunner = runner_findByIndex(word_4AF3F6[runnerPtr[69]]);
		//      scrb_loadOnRunner(1, word_4A1D08[runnerPtr[69]], foundRunner);
		//      scrb_registerHotspotGroup(0, 0, 0, 0, compIdx, compIdx);
		{
			ZmbFeature *shadowFeature = _creatureShadowFeatures[seatIdx];
			if (shadowFeature) {
				int16 typeId = kCreatureTypeId[seatIdx];
				loadScrbOntoFeature(shadowFeature, static_cast<uint16>(kCreatureTypeScrbs[typeId]));
			}
		}
		break;

	case '=': // Play traversal script (mode 0)
	case 'G':
	case 'Q':
		// IDA: net_playTraversalScript — plays companion traversal SCRS (visual only).
		// In ScummVM's simplified architecture, companion SCRS traversal is not implemented.
		break;

	case '>': // Setup traversal step (mode 0)
		// IDA: net_setupTraversalStep — updates companion position/SCRS (visual only).
		// In ScummVM's simplified architecture, no companion to update.
		break;

	case '@': // Push companion to column-link queue
	case 'T':
		// IDA: word_4AFF88[word_4B00CA++] = runnerPtr[74];
		if (runnerIdx >= 0) {
			ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
			if (rs.companionIdx >= 0 && _columnLinkQueueSize < kMaxQueueSize)
				_columnLinkQueue[_columnLinkQueueSize++] = rs.companionIdx;
		}
		break;

	case 'A': // Spawn traversal runner (mode 1)
		// IDA: net_spawnTraversalRunner — spawns overlay companion runner (visual only).
		// In ScummVM's simplified architecture, companion runners are not spawned.
		break;

	case 'B': // Clear slot + seat assignment
	case 'L':
	case 'V':
		// IDA: scrb_setSlotFeatureRunnerIdx(0, runnerPtr[68]); word_4AF33C[runnerPtr[68]] = 0;
		_seatAssignment[seatIdx] = 0;
		break;

	case 'H': // Setup traversal step (mode 1)
	case 'R':
		// IDA: net_setupTraversalStep — updates companion position/SCRS (visual only).
		// In ScummVM's simplified architecture, no companion to update.
		break;

	case 'J': // Push companion to column-link queue + clear companion link
		// IDA: word_4AFF88[word_4B00CA++] = runnerPtr[74]; runnerPtr[74] = 0;
		if (runnerIdx >= 0) {
			ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
			if (rs.companionIdx >= 0 && _columnLinkQueueSize < kMaxQueueSize)
				_columnLinkQueue[_columnLinkQueueSize++] = rs.companionIdx;
			rs.companionIdx = -1;
		}
		break;

	case 'K': // Spawn traversal runner (mode 0)
	case 'U':
		// IDA: net_spawnTraversalRunner — spawns overlay companion runner (visual only).
		// In ScummVM's simplified architecture, companion runners are not spawned.
		break;

	default:
		break;
	}
}

void ZoombiniPuzzleMaze::handleCollisionTracking(int16 eventCode, int16 runnerIdx) {
	// IDA: net_trackRunnerCollisions (0x431354)
	ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
	int16 row = rs.row;
	int16 col = rs.col;

	if (row < 0 || row >= kGridRows || col < 0 || col >= kGridCols)
		return;

	if (eventCode == 20 || eventCode == 30 || eventCode == 40 || eventCode == 50) {
		// Collision tracking - only at intersections (cell type == 3)
		if (rs.cellTypeAtPos != 3)
			return;

		_collisionCount[row][col]++;
		if (_collisionCount[row][col] == 1) {
			_collisionRunnerIdx[row][col] = runnerIdx;
		} else if (_collisionCount[row][col] == 2) {
			int16 other = _collisionRunnerIdx[row][col];
			if (_crossAssignQueueSize + 2 <= kMaxCrossQueueSize) {
				_crossAssignQueue[_crossAssignQueueSize++] = other;
				_crossAssignQueue[_crossAssignQueueSize++] = runnerIdx;
			}
			_collisionRunnerIdx[row][col] = 0;
			_collisionCount[row][col] = 0;
		} else {
			_collisionRunnerIdx[row][col] = 0;
			_collisionCount[row][col] = 0;
		}
	} else if (eventCode == 21 || eventCode == 31 || eventCode == 41 || eventCode == 51 || eventCode == 61) {
		// Row-change event
		if (_rowChangeQueueSize < kMaxQueueSize)
			_rowChangeQueue[_rowChangeQueueSize++] = runnerIdx;
	}
}

// =================================================================
// Click handling
// IDA: maze_onClickHandler (0x4301EF)
// =================================================================

ZmbEventHandleResult ZoombiniPuzzleMaze::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	if (_reentryGuard)
		return ZmbEventHandleResult::kPassthrough;

	ZmbSnoid *clicked = findIdleSnoidAtPoint(absPos);
	if (!clicked)
		return ZmbEventHandleResult::kPassthrough;

	// First-click initialization
	if (!_gridInitialized)
		initGridAndSelectPaths();

	// Init all anim tables on first click
	if (!_animTablesInitialized)
		initAllRunnerAnimTables();

	// Check placement limit
	if (_placedRunnerCount >= 10)
		return ZmbEventHandleResult::kPassthrough;

	// Check if already placed
	for (int16 i = 0; i < 10; i++) {
		if (_placedRunnerIds[i] == clicked->getId())
			return ZmbEventHandleResult::kPassthrough;
	}

	_isDragging = true;
	_dragSnoid = clicked;
	_dragSavedPos = clicked->getPointLoc();
	beginSnoidDrag(clicked);

	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniPuzzleMaze::onMouseMove(const Common::Point &absPos, const Common::Point &relPos) {
	if (!_isDragging || !_dragSnoid)
		return ZmbEventHandleResult::kPassthrough;

	_dragSnoid->setPointLoc(absPos);
	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniPuzzleMaze::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	if (!_isDragging || !_dragSnoid)
		return ZmbEventHandleResult::kPassthrough;

	endSnoidDrag(_dragSnoid);

	int16 seatIdx = findSeatAtPoint(absPos);

	if (seatIdx >= 0 && validateTerrainDrop(_dragSnoid)) {
		handleGridDrop(seatIdx, _dragSnoid);
	} else {
		_dragSnoid->setPointLoc(_dragSavedPos);
		_dragSnoid->setupIdleHotspots();
	}

	_isDragging = false;
	_dragSnoid = nullptr;
	return ZmbEventHandleResult::kConsumed;
}

int16 ZoombiniPuzzleMaze::findSeatAtPoint(const Common::Point &pos) const {
	for (int i = 0; i < 14; i++) {
		Common::Point seatPos = kSeatPositions[i];
		int16 dx = pos.x - seatPos.x;
		int16 dy = pos.y - seatPos.y;
		if (dx * dx + dy * dy < 40 * 40) {
			// Check if seat is already occupied
			if (_seatAssignment[i])
				continue;
			return i;
		}
	}
	return -1;
}

ZmbSnoid *ZoombiniPuzzleMaze::findIdleSnoidAtPoint(const Common::Point &pos) const {
	for (int16 i = 0; i < _loadedZmbCount; i++) {
		uint16 snoidId = 10000 + i;
		ZmbSnoid *snoid = const_cast<ZoombiniPuzzleMaze *>(this)->getSnoid(snoidId);
		if (!snoid || !snoid->isRenderActivated())
			continue;
		if (!snoid->_packIsOccupied)
			continue;

		int16 runnerIdx = findRunnerBySnoidId(snoidId);
		if (runnerIdx >= 0 && _runnerStates[runnerIdx].placed)
			continue;

		if (snoid->hasClickRect() && snoid->getClickRect().contains(pos))
			return snoid;
	}
	return nullptr;
}

void ZoombiniPuzzleMaze::handleGridDrop(int16 seatIdx, ZmbSnoid *snoid) {
	// IDA: maze_onClickHandler drop section
	if (seatIdx < 0 || seatIdx >= 14)
		return;

	// Set snoid position to seat position
	snoid->setPointLoc(kSeatPositions[seatIdx]);

	// Set facing from seat table
	// IDA: p_core188->chZmbAnimShapeCommonImageIdx = byte_4A1C44[2*dropTarget]
	// IDA: p_core188->chIsFacingLeft = word_4A1C0C[dropTarget]

	// Configure runner state
	int16 runnerIdx = findRunnerBySnoidId(snoid->getId());
	if (runnerIdx < 0) {
		// Create new runner entry
		if (_runnerCount >= kMaxRunners)
			return;
		runnerIdx = _runnerCount++;
		_zmbRunnerSnoidIds[runnerIdx] = snoid->getId();
	}

	ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
	rs.clear();

	// Set grid position from seat table
	rs.row = kSeatGridCoords[seatIdx].x;
	rs.col = kSeatGridCoords[seatIdx].y;
	rs.oldRow = rs.row;
	rs.oldCol = rs.col;
	rs.direction = kSeatMoveDirection[seatIdx];
	rs.seatIdx = seatIdx;
	rs.footTrait = snoid->_trait._foot;
	rs.placed = true;
	rs.moving = false;

	// Init anim table
	initRunnerAnimTable(runnerIdx);

	// Record placed runner
	if (_placedRunnerCount < 10)
		_placedRunnerIds[_placedRunnerCount++] = snoid->getId();

	// Mark seat assignment. IDA: word_4AF33E[dropTarget] = dropTarget + 1
	_seatAssignment[seatIdx] = seatIdx + 1;

	// Check if all seats are filled -> enable GO button
	bool anyPlaced = false;
	for (int i = 0; i < 14; i++) {
		if (_seatAssignment[i])
			anyPlaced = true;
	}
	_runnersArePlaced = anyPlaced;

	// Enqueue to setup-node queue. IDA: word_4B0078[word_4B00D4++] = dropTarget
	if (_setupNodeQueueSize < kMaxQueueSize)
		_setupNodeQueue[_setupNodeQueueSize++] = seatIdx;

	debugC(kZmbDebugPage, "Maze: Placed runner %d at seat %d (row=%d, col=%d, dir=%d)",
	       runnerIdx, seatIdx, rs.row, rs.col, rs.direction);
}

// =================================================================
// Queue processing (called from onEveryFrame)
// IDA: maze2_onHover_frameUpdate (0x42F899)
// =================================================================

void ZoombiniPuzzleMaze::processQueues() {
	// IDA maze2_onHover_frameUpdate @ 0x42F8C1: rowChange (word_4B0028) is
	// processed BEFORE arrival (word_4AFFB0). The previous (arrival → rowChange)
	// order delayed turn-node arrivals by one frame because a runner that
	// performed a row-change in frame N would not have its arrival animation
	// processed until frame N+1.
	processSetupNodeQueue();
	processMoveQueue();
	processLinkQueue();
	processColumnLinkQueue();
	processScrsPlayQueue();
	processReorderFlags();
	processRowChangeQueue();
	processArrivalQueue();
	processCrossAssignQueue();
}

void ZoombiniPuzzleMaze::processSetupNodeQueue() {
	// IDA: while (word_4B00D4) net_setupNodeScrb(word_4B0078[--word_4B00D4]);
	while (_setupNodeQueueSize > 0) {
		int16 nodeIdx = _setupNodeQueue[--_setupNodeQueueSize];
		setupNodeScrb(nodeIdx);
	}
}

void ZoombiniPuzzleMaze::setupNodeScrb(int16 nodeIdx) {
	// IDA: net_setupNodeScrb (0x430707)
	if (nodeIdx < 0 || nodeIdx >= 14)
		return;

	// Find runner placed at this node
	int16 runnerIdx = -1;
	for (int16 i = 0; i < _runnerCount; i++) {
		if (_runnerStates[i].seatIdx == nodeIdx && _runnerStates[i].placed) {
			runnerIdx = i;
			break;
		}
	}
	if (runnerIdx < 0)
		return;

	ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
	uint16 snoidId = _zmbRunnerSnoidIds[runnerIdx];
	ZmbSnoid *snoid = getSnoid(snoidId);
	if (!snoid)
		return;

	// Load the SCRB animation for this node
	// IDA: scrb_loadOnRunner(1, word_4A1CEC[nodeIdx], mainRunner)
	int16 scrbId = kCreatureScrbId[nodeIdx];
	ZmbFeature *nodeFeature = _creatureObstacleFeatures[nodeIdx];
	if (nodeFeature) {
		loadScrbOntoFeature(nodeFeature, static_cast<uint16>(scrbId));
	}

	// Load companion SCRB if two-part node
	if (kCreatureHasShadow[nodeIdx] && _creatureShadowFeatures[nodeIdx]) {
		loadScrbOntoFeature(_creatureShadowFeatures[nodeIdx],
			static_cast<uint16>(scrbId + 1));
	}

	// Play initial walking SCRS on the snoid
	// IDA: direction SCRS from anim table = foot + 15014 + (direction * 5)
	int16 walkScrs = rs.scrsTable[rs.direction]; // Direction walk SCRS
	Common::SeekableReadStream *scrsStream =
		_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
			ZmbResource(ZmbArchiveKind::kPage, static_cast<uint16>(walkScrs)));
	if (scrsStream) {
		snoid->startScrsPlayback(scrsStream, false, false);
		rs.moving = true;
	}
}

void ZoombiniPuzzleMaze::processMoveQueue() {
	// IDA: while (word_4B00CE) fleens_moveZmbOnRaft(findByIndex(word_4B0000[--word_4B00CE]));
	while (_moveQueueSize > 0) {
		int16 runnerIdx = _moveQueue[--_moveQueueSize];
		moveZmbOnGrid(runnerIdx);
	}
}

void ZoombiniPuzzleMaze::moveZmbOnGrid(int16 runnerIdx) {
	// IDA: fleens_moveZmbOnRaft (0x434C7C)
	// Moves runner one cell in current direction. SCRB-driven, NOT BFS.
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];

	// Save old position
	rs.oldRow = rs.row;
	rs.oldCol = rs.col;

	// Move one cell in direction
	switch (rs.direction) {
	case 0: // decCol: col--
		if (--rs.col < 0)
			rs.col++;
		break;
	case 1: // incRow: row++
		if (++rs.row > 12)
			rs.row--;
		break;
	case 2: // incCol: col++
		if (++rs.col > 12)
			rs.col--;
		break;
	case 3: // decRow: row--
		if (--rs.row < 0)
			rs.row++;
		break;
	}

	// Check if destination is a hitchhiker cell (type 5)
	if (rs.row >= 0 && rs.row < kGridRows && rs.col >= 0 && rs.col < kGridCols) {
		if (_cellTypes[rs.row][rs.col] == 5) {
			// Transfer hitchhiker to move queue
			int16 hitchhikerRunner = _cellRunnerIdx[rs.row][rs.col];
			if (hitchhikerRunner >= 0 && hitchhikerRunner < kMaxRunners) {
				ZmbMazeRunnerState &hitchRs = _runnerStates[hitchhikerRunner];
				hitchRs.direction = rs.direction;
				if (_moveQueueSize < kMaxQueueSize)
					_moveQueue[_moveQueueSize++] = hitchhikerRunner;
			}
		}

		// Store cell type at new position
		rs.cellTypeAtPos = _cellTypes[rs.row][rs.col];
	}

	// Compute pixel position from grid position table
	// IDA: *(int32*)(a1+214) = *(unk_4AF2C0 + 4*col + 52*row); X+=4, Y-=38
	if (rs.row >= 0 && rs.row < kGridRows && rs.col >= 0 && rs.col < kGridCols) {
		rs.pixelX = _gridCellPos[rs.row][rs.col].x + 4;
		rs.pixelY = _gridCellPos[rs.row][rs.col].y - 38;

		uint16 snoidId = _zmbRunnerSnoidIds[runnerIdx];
		ZmbSnoid *snoid = getSnoid(snoidId);
		if (snoid) {
			snoid->setPointLoc(Common::Point(rs.pixelX, rs.pixelY));
		}
	}

	// Load direction SCRB (10000 + direction) on companion
	// IDA: scrb_loadOnRunner(1, direction + 10000, companionRunner)
	// Play walking SCRS
	uint16 snoidId = _zmbRunnerSnoidIds[runnerIdx];
	ZmbSnoid *snoid = getSnoid(snoidId);
	if (snoid) {
		int16 walkScrs = rs.scrsTable[rs.direction];
		Common::SeekableReadStream *scrsStream =
			_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
				ZmbResource(ZmbArchiveKind::kPage, static_cast<uint16>(walkScrs)));
		if (scrsStream)
			snoid->startScrsPlayback(scrsStream, false, false);
	}
}

void ZoombiniPuzzleMaze::processLinkQueue() {
	// IDA: while (count) runner_linkRelativeToParent(word_4AF2FA, 0, runner->idx);
	while (_linkQueueSize > 0) {
		int16 runnerIdx = _linkQueue[--_linkQueueSize];
		(void)runnerIdx; // Z-ordering handled by ScummVM render system
	}
}

void ZoombiniPuzzleMaze::processColumnLinkQueue() {
	// IDA: while (count) runner_linkRelativeToParent(word_4AF45C[runner.col], 0, runner->idx);
	while (_columnLinkQueueSize > 0) {
		int16 runnerIdx = _columnLinkQueue[--_columnLinkQueueSize];
		(void)runnerIdx;
	}
}

void ZoombiniPuzzleMaze::processScrsPlayQueue() {
	// IDA: Play foot+15090 celebration/exit SCRS
	while (_scrsPlayQueueSize > 0) {
		int16 runnerIdx = _scrsPlayQueue[--_scrsPlayQueueSize];
		if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
			continue;

		ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
		uint16 snoidId = _zmbRunnerSnoidIds[runnerIdx];
		ZmbSnoid *snoid = getSnoid(snoidId);
		if (!snoid)
			continue;

		// IDA: snoidScript_initAndPlay(0, 0, *(char*)(runner+239) + 15090, runner+48)
		// runner+239 = 0-indexed foot trait (0-4). In ScummVM footTrait is 1-indexed (1-5),
		// so correct SCRS = footTrait - 1 + 15090 = footTrait + 15089 → SCRS 15090-15094.
		uint16 exitScrs = rs.footTrait + 15089;
		Common::SeekableReadStream *scrsStream =
			_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
				ZmbResource(ZmbArchiveKind::kPage, exitScrs));
		if (scrsStream) {
			snoid->startScrsPlayback(scrsStream, false, false);
		}
	}
}

void ZoombiniPuzzleMaze::processReorderFlags() {
	if (_reorderFlag0) {
		_reorderFlag0 = false;
		// ScummVM handles Z-ordering per-frame
	}
	if (_reorderFlag1) {
		_reorderFlag1 = false;
	}
}

void ZoombiniPuzzleMaze::processArrivalQueue() {
	while (_arrivalQueueSize > 0) {
		int16 runnerIdx = _arrivalQueue[--_arrivalQueueSize];
		if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
			continue;

		ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
		handleArrival(rs.direction, runnerIdx);
	}
}

void ZoombiniPuzzleMaze::handleArrival(int16 direction, int16 runnerIdx) {
	// IDA: Arrival queue processing with direction-based bitmask linking
	ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
	rs.moving = false;
	rs.arrived = true;

	uint16 snoidId = _zmbRunnerSnoidIds[runnerIdx];
	ZmbSnoid *snoid = getSnoid(snoidId);

	// Get arrival position from table
	int16 posIdx = _arrivalPosCounter[direction];
	if (posIdx >= 20)
		posIdx = 0;
	_arrivalPosCounter[direction] = posIdx + 1;

	Common::Point arrivalPos = kArrivalPositions[20 * direction + posIdx];

	if (snoid) {
		if (direction == 3) {
			// IDA: animateZoombini(0, 7, runner+48) = initWalkToTarget, then
			// queue 5/10 plays snoidScript_initAndPlay(0, 0, foot+15090, runner+48).
			// In ScummVM: teleport to arrivalPos and play foot+15089 celebration SCRS directly.
			snoid->setPointLoc(arrivalPos);
			uint16 celebScrs = rs.footTrait + 15089;
			Common::SeekableReadStream *celebStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
					ZmbResource(ZmbArchiveKind::kPage, celebScrs));
			if (celebStream)
				snoid->startScrsPlayback(celebStream, false, false);
		} else {
			// IDA: animateZoombini(0, 7, runner+48) = initWalkToTarget
			snoid->initWalkToTarget(arrivalPos);
		}
	}

	// Clear grid cell
	if (rs.row >= 0 && rs.row < kGridRows && rs.col >= 0 && rs.col < kGridCols) {
		_cellRunnerIdx[rs.row][rs.col] = 0;
	}

	// Direction 3 = completion (decRow exit -> right-top)
	if (direction == 3) {
		_arrivedZmbCount++;
		if (_arrivedZmbCount >= _loadedZmbCount) {
			_celebrationTrigger = true;
			_celebrationTarget = _loadedZmbCount;
			_celebrationsPlayed = 0;
			_celebrationPoolState = 0;
			_celebrationLastFrame = getCurrentFrameCounter();

			uint16 sndId = _vm->_rnd->getRandomNumber(20055, 20063);
			_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, sndId));
		}
	}
}

void ZoombiniPuzzleMaze::processRowChangeQueue() {
	// IDA: Dispatch based on cell type
	while (_rowChangeQueueSize > 0) {
		int16 runnerIdx = _rowChangeQueue[--_rowChangeQueueSize];
		if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
			continue;

		ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
		int16 row = rs.row;
		int16 col = rs.col;

		if (row < 0 || row >= kGridRows || col < 0 || col >= kGridCols) {
			if (_arrivalQueueSize < kMaxQueueSize)
				_arrivalQueue[_arrivalQueueSize++] = runnerIdx;
			continue;
		}

		// Re-link to column if column changed (IDA check)
		if (rs.col != rs.oldCol) {
			// Z-ordering update (handled by ScummVM)
		}

		// Clear collision tracking if first runner at intersection
		if (_collisionRunnerIdx[row][col] == runnerIdx) {
			_collisionCount[row][col] = 0;
			_collisionRunnerIdx[row][col] = 0;
		}

		int16 cellType = _cellTypes[row][col];
		handleRowChange(cellType, runnerIdx);
	}
}

void ZoombiniPuzzleMaze::handleRowChange(int16 cellType, int16 runnerIdx) {
	// IDA: Row-change cell type dispatch from maze2_onHover_frameUpdate
	ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
	int16 nodeRunnerIdx = _cellRunnerIdx[rs.row][rs.col];

	switch (cellType) {
	case 1: // Straight: arrive at node
		// IDA: net_zmbArriveAtNodeAlt(word_4AFB98[13*row+col], runner)
		zmbArriveAtNodeAlt(nodeRunnerIdx, runnerIdx);
		break;

	case 2: // Zmb start: move step alt (trait check)
		// IDA: net_moveRunnerStepAlt(0, word_4AFB98[...], runner)
		moveRunnerStepAlt(nodeRunnerIdx, runnerIdx);
		break;

	case 3: // Intersection
	case 4: // Intersection
		// IDA: net_moveRunnerStep(core, word_4AFB98[...], runner)
		moveRunnerStep(nodeRunnerIdx, runnerIdx);
		break;

	case 5: // Hitchhiker: setup collision tracking
		// IDA: net_zmbSetupCollisionTracking(word_4AFB98[...], runner)
		zmbSetupCollisionTracking(nodeRunnerIdx, runnerIdx);
		break;

	case 6: // Exit node: play sound, finalize, then exit-walk (falls through to type 1 in IDA)
		// IDA: scrb_enqueueSoundResource(0, 5103); net_finalizeRunnerAtSlot();
		//      → falls through to case 1: net_zmbArriveAtNodeAlt()
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 5103));
		finalizeRunnerAtSlot(nodeRunnerIdx);
		zmbArriveAtNodeAlt(nodeRunnerIdx, runnerIdx);
		break;

	case 20: case 21: case 22: case 23:
		// Turn nodes: direction = cellType - 20
		// IDA: net_zmbArriveAtNode(cellType, runner)
		zmbArriveAtNode(cellType, runnerIdx);
		break;

	default:
		// Unknown cell type: just continue moving
		// IDA: fleens_moveZmbOnRaft(runner)
		moveZmbOnGrid(runnerIdx);
		break;
	}
}

void ZoombiniPuzzleMaze::processCrossAssignQueue() {
	// IDA: while (count > 1) { r2 = [--count]; r1 = [--count]; assignCross(); }
	while (_crossAssignQueueSize > 1) {
		int16 runner2 = _crossAssignQueue[--_crossAssignQueueSize];
		int16 runner1 = _crossAssignQueue[--_crossAssignQueueSize];
		assignCrossRunnerScrbs(runner1, runner2);
	}
	_crossAssignQueueSize = 0;
}

// =================================================================
// Cell type routing functions
// =================================================================

void ZoombiniPuzzleMaze::zmbArriveAtNode(int16 cellType, int16 runnerIdx) {
	// IDA: net_zmbArriveAtNode (0x430049)
	// Turn node: only update direction. The IDA implementation loads SCRB 10040
	// onto the node runner and starts a snoid idle SCRS — it does NOT call the
	// movement function. Calling moveZmbOnGrid() here causes a double-move per
	// turn (one in the IDA-equivalent SCRS playback, one in C++).
	//
	// Direction is consumed by the next normal moveZmbOnGrid() call from the
	// caller's main update loop, after the SCRS turn-idle animation completes.
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
	int16 turnDir = cellType - 20; // 20->0, 21->1, 22->2, 23->3
	rs.direction = turnDir;
}

void ZoombiniPuzzleMaze::zmbArriveAtNodeAlt(int16 nodeRunnerIdx, int16 runnerIdx) {
	// IDA: net_zmbArriveAtNodeAlt (0x434F8B)
	// Straight-through exit node: play SCRS 14006 (exit walk animation) on the snoid.
	// Sets net_processRunnerExitFrame at snoid runner+16 so that when SCRS 14006 ends
	// (kZmbAnimEventM1_End fires), the snoid is pushed to the arrival queue.
	// IDA: snoidScript_initAndPlay(1, 0, 14006, a2+48)
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
	uint16 snoidId = _zmbRunnerSnoidIds[runnerIdx];
	ZmbSnoid *snoid = getSnoid(snoidId);
	if (!snoid) {
		// No snoid: push directly to arrival queue as fallback
		if (_arrivalQueueSize < kMaxQueueSize)
			_arrivalQueue[_arrivalQueueSize++] = runnerIdx;
		return;
	}

	// Play SCRS 14006 (exit walk, 26 groups). When finished, kZmbAnimEventM1_End fires
	// and processRunnerExitFrame pushes this runner to the arrival queue.
	Common::SeekableReadStream *scrsStream =
		_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
			ZmbResource(ZmbArchiveKind::kPage, 14006));
	if (scrsStream)
		snoid->startScrsPlayback(scrsStream, false, false);

	rs.exiting = true;
	rs.moving = false;
}

void ZoombiniPuzzleMaze::moveRunnerStep(int16 nodeRunnerIdx, int16 runnerIdx) {
	// IDA: net_moveRunnerStep (0x435290)
	// Intersection movement (cell types 3/4).
	// Sets zmb direction from node, then optionally cycles node direction for next zmb.
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
	int16 row = rs.row;
	int16 col = rs.col;

	if (row < 0 || row >= kGridRows || col < 0 || col >= kGridCols)
		return;

	// Set zmb direction from node's current direction
	rs.direction = _nodeDirection[row][col];

	// Direction cycling (IDA: core[39] != 0)
	if (_nodeCycleFlag[row][col]) {
		// Play alternating click sound 5101/5102
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 5101 + _soundAlternator));
		if (++_soundAlternator > 1)
			_soundAlternator = 0;

		// Advance node direction for next zmb, skip unavailable dirs
		int16 &nodeDir = _nodeDirection[row][col];
		if (++nodeDir > 3)
			nodeDir = 0;
		while (!_nodeDirFlags[row][col][nodeDir]) {
			if (++nodeDir > 3)
				nodeDir = 0;
		}
	}

	moveZmbOnGrid(runnerIdx);
}

void ZoombiniPuzzleMaze::moveRunnerStepAlt(int16 nodeRunnerIdx, int16 runnerIdx) {
	// IDA: net_moveRunnerStepAlt (0x4350B0)
	// Attribute routing step (cell type 2).
	// If zoombini's trait matches node's configured trait, route in node's direction.
	// Otherwise keep current direction (continue straight).
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];
	int16 row = rs.row;
	int16 col = rs.col;

	if (row < 0 || row >= kGridRows || col < 0 || col >= kGridCols)
		return;

	// IDA: if (*(char*)(a3 + 48 + (int16)a1[41] + 187) == (int16)a1[42])
	//         *(int16*)(a3 + 88) = a1[38];
	int16 attrType = _cellAttrType[row][col];    // 1=hair, 2=eyes, 3=nose, 4=feet
	int16 attrValue = _cellAttrValue[row][col];   // 1-5

	uint16 snoidId = _zmbRunnerSnoidIds[runnerIdx];
	ZmbSnoid *snoid = getSnoid(snoidId);
	if (snoid && attrType >= 1 && attrType <= 4) {
		byte traitVal = getTraitByCategory(snoid->_trait, attrType);
		if (traitVal == attrValue) {
			// Match: route in configured direction
			rs.direction = _nodeDirection[row][col];
		}
		// No match: keep current direction (zmb continues straight)
	}

	moveZmbOnGrid(runnerIdx);
}

void ZoombiniPuzzleMaze::zmbSetupCollisionTracking(int16 nodeRunnerIdx, int16 runnerIdx) {
	// IDA: net_zmbSetupCollisionTracking (0x4354D8)
	// The zmb arrives at a hitchhiker cell (type 5). It stops here and waits.
	// The hitchhiker node records which zmb is waiting. When the wave exits (finalizeRunnerAtSlot),
	// the stored zmb is ejected into the move queue with the same direction.
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbMazeRunnerState &rs = _runnerStates[runnerIdx];

	// IDA: prevRow = row, prevCol = col
	rs.oldRow = rs.row;
	rs.oldCol = rs.col;

	// IDA: hikerRunner->hsArr[14].pos.x = zmb's snoidRunnerIdx  (link zmb to hitchhiker node)
	if (nodeRunnerIdx >= 0 && nodeRunnerIdx < kMaxRunners)
		_runnerStates[nodeRunnerIdx].linkedZmbRunnerIdx = runnerIdx;

	// IDA: nodeRunner loads SCRB (dir + 10036) and positions at hitchhiker pixel
	// (companion visual handled by SCRB events; position is already at cell)

	// IDA: snoidScript_initAndPlay — keep walking animation playing at rest
	uint16 snoidId = _zmbRunnerSnoidIds[runnerIdx];
	ZmbSnoid *snoid = getSnoid(snoidId);
	if (snoid) {
		// Use the direction walk SCRS (same as in-motion anim) while waiting.
		// IDA: core[50+dir] from the runner's anim table (direction walk series).
		int16 walkScrs = rs.scrsTable[rs.direction];
		if (walkScrs > 0) {
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
					ZmbResource(ZmbArchiveKind::kPage, static_cast<uint16>(walkScrs)));
			if (scrsStream)
				snoid->startScrsPlayback(scrsStream, false, false);
		}
	}

	// Do NOT advance the zmb — it waits at the hitchhiker cell.
	// It will be ejected by finalizeRunnerAtSlot when the wave completes.
}

void ZoombiniPuzzleMaze::finalizeRunnerAtSlot(int16 nodeRunnerIdx) {
	// IDA: net_finalizeRunnerAtSlot (0x434E1D)
	// Called when a zmb reaches an exit cell (type 6). Processes every runner in the
	// same wave group: intersection runners advance their direction frame; hitchhiker
	// runners eject their waiting zmb into the move queue.
	if (nodeRunnerIdx < 0 || nodeRunnerIdx >= kMaxRunners)
		return;

	ZmbMazeRunnerState &nodeRs = _runnerStates[nodeRunnerIdx];
	// IDA: runner->byte292 = 2; runner->word70 = 0 (mark exit node complete)
	nodeRs.arrived = true;
	nodeRs.linkedZmbRunnerIdx = -1;

	// IDA: slotType = runner->hsArr[11].shapeid (wave group 1-8)
	// Wave group 1 uses count=0 in the original — no wave processing for group 1.
	int16 waveGroup = nodeRs.waveGroup;
	if (waveGroup < 2 || waveGroup > kMaxWaveGroups)
		return;

	int16 groupIdx = waveGroup - 1;
	int16 count = _waveGroupCount[groupIdx];

	// IDA: iterate wave group backwards: while (count > 0) { grpRunner = groupArr[--count]; ... }
	for (int16 i = count - 1; i >= 0; i--) {
		int16 memberIdx = _waveGroupRunners[groupIdx][i];
		if (memberIdx < 0 || memberIdx >= kMaxRunners)
			continue;

		ZmbMazeRunnerState &memberRs = _runnerStates[memberIdx];
		int16 row = memberRs.row;
		int16 col = memberRs.col;
		if (row < 0 || row >= kGridRows || col < 0 || col >= kGridCols)
			continue;

		int16 cellType = _cellTypes[row][col];

		// IDA: type 4 (intersection) — advance to next available direction, reactivate
		// Original: if (++frameIdx > 3) frameIdx = 0;
		//           while (!frames[frameIdx]) { if (++frameIdx > 3) frameIdx = 0; }
		//           state = 3;
		if (cellType == 4) {
			int16 &dir = _nodeDirection[row][col];
			if (++dir > 3) dir = 0;
			// Skip directions that are unavailable (dirFlag == 0)
			int16 attempts = 0;
			while (attempts < 4 && !_nodeDirFlags[row][col][dir]) {
				if (++dir > 3) dir = 0;
				attempts++;
			}
			debugC(2, kZmbDebugAnimation, "Maze: Wave %d advanced intersection [%d,%d] to dir %d",
			       waveGroup, row, col, dir);
		}

		// IDA: type 5 (hitchhiker) — eject waiting zmb into move queue
		// Original: word_4B0000[word_4B00CE++] = grpRunner->zmbId; grpRunner->zmbId = 0;
		if (cellType == 5 && memberRs.linkedZmbRunnerIdx >= 0) {
			if (_moveQueueSize < kMaxQueueSize)
				_moveQueue[_moveQueueSize++] = memberRs.linkedZmbRunnerIdx;
			debugC(2, kZmbDebugAnimation, "Maze: Wave %d ejected hitchhiker zmb runner %d from [%d,%d]",
			       waveGroup, memberRs.linkedZmbRunnerIdx, row, col);
			memberRs.linkedZmbRunnerIdx = -1;
		}
	}
}

void ZoombiniPuzzleMaze::assignCrossRunnerScrbs(int16 runner1Idx, int16 runner2Idx) {
	// IDA: net_assignCrossRunnerScrbs (0x43462E)
	// Assigns visual overlays when two runners cross at an intersection.
	// Uses 4x4 direction matrix for SCRB pairs.
	if (runner1Idx < 0 || runner1Idx >= kMaxRunners ||
	    runner2Idx < 0 || runner2Idx >= kMaxRunners)
		return;

	ZmbMazeRunnerState &rs1 = _runnerStates[runner1Idx];
	ZmbMazeRunnerState &rs2 = _runnerStates[runner2Idx];

	// Clear intersection collision tracking
	if (rs2.row >= 0 && rs2.row < kGridRows && rs2.col >= 0 && rs2.col < kGridCols) {
		_collisionCount[rs2.row][rs2.col] = 0;
		_collisionRunnerIdx[rs2.row][rs2.col] = 0;
	}

	// Play crossing scripts with foot offset
	// IDA: Direction matrix selects visual SCRB (15035/15040/15045/15050) and audio SCRB (10004-10027)
	uint16 snoidId1 = _zmbRunnerSnoidIds[runner1Idx];
	uint16 snoidId2 = _zmbRunnerSnoidIds[runner2Idx];
	ZmbSnoid *snoid1 = getSnoid(snoidId1);
	ZmbSnoid *snoid2 = getSnoid(snoidId2);

	// Visual crossing SCRB selection by direction pair
	int16 visualScrb1 = 0, visualScrb2 = 0;
	int16 audioScrb1 = 0, audioScrb2 = 0;

	// Direction matrix lookup (IDA @ 0x43462E)
	switch (rs2.direction) {
	case 0:
		switch (rs1.direction) {
		case 0: visualScrb2 = 15045; audioScrb2 = 10006; visualScrb1 = 15035; audioScrb1 = 10004; break;
		case 1: visualScrb2 = 15050; audioScrb2 = 10008; visualScrb1 = 15045; audioScrb1 = 10006; break;
		case 2: visualScrb2 = 15045; audioScrb2 = 10006; visualScrb1 = 15035; audioScrb1 = 10004; break;
		case 3: visualScrb2 = 15040; audioScrb2 = 10010; visualScrb1 = 15045; audioScrb1 = 10006; break;
		}
		break;
	case 1:
		switch (rs1.direction) {
		case 0: visualScrb2 = 15045; audioScrb2 = 10006; visualScrb1 = 15050; audioScrb1 = 10008; break;
		case 1: visualScrb2 = 15035; audioScrb2 = 10004; visualScrb1 = 15050; audioScrb1 = 10008; break;
		case 2: visualScrb2 = 15040; audioScrb2 = 10005; visualScrb1 = 15050; audioScrb1 = 10007; break;
		case 3: visualScrb2 = 15040; audioScrb2 = 10010; visualScrb1 = 15050; audioScrb1 = 10009; break;
		}
		break;
	case 2:
		switch (rs1.direction) {
		case 0: visualScrb2 = 15045; audioScrb2 = 10006; visualScrb1 = 15035; audioScrb1 = 10004; break;
		case 1: visualScrb2 = 15050; audioScrb2 = 10008; visualScrb1 = 15035; audioScrb1 = 10004; break;
		case 2: visualScrb2 = 15045; audioScrb2 = 10006; visualScrb1 = 15035; audioScrb1 = 10004; break;
		case 3: visualScrb2 = 15040; audioScrb2 = 10011; visualScrb1 = 15035; audioScrb1 = 10004; break;
		}
		break;
	case 3:
		switch (rs1.direction) {
		case 0: visualScrb2 = 15045; audioScrb2 = 10006; visualScrb1 = 15040; audioScrb1 = 10010; break;
		case 1: visualScrb2 = 15045; audioScrb2 = 10006; visualScrb1 = 15040; audioScrb1 = 10010; break;
		case 2: visualScrb2 = 15040; audioScrb2 = 10010; visualScrb1 = 15040; audioScrb1 = 10010; break;
		case 3: visualScrb2 = 15040; audioScrb2 = 10010; visualScrb1 = 15050; audioScrb1 = 10009; break;
		}
		break;
	}

	// Play crossing SCRS with foot offset
	// Note: audioScrb1/2 values are preserved for potential future audio enhancement
	// but currently not used since a single shared sound (5101) is played instead.
	(void)audioScrb1;
	(void)audioScrb2;

	if (visualScrb2 && snoid2) {
		Common::SeekableReadStream *scrs2 =
			_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
				ZmbResource(ZmbArchiveKind::kPage, static_cast<uint16>(rs2.footTrait + visualScrb2)));
		if (scrs2)
			snoid2->startScrsPlayback(scrs2, true, false);
	}
	if (visualScrb1 && snoid1) {
		Common::SeekableReadStream *scrs1 =
			_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
				ZmbResource(ZmbArchiveKind::kPage, static_cast<uint16>(rs1.footTrait + visualScrb1)));
		if (scrs1)
			snoid1->startScrsPlayback(scrs1, true, false);
	}

	// Sound 5101/5102 for intersection crossing
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 5101));
}

// =================================================================
// Obstacle system
// IDA: maze_spawnMovingObstacle (0x42E05D), etc.
// =================================================================

void ZoombiniPuzzleMaze::spawnObstacle(int16 slotIdx) {
	// IDA: maze_spawnMovingObstacle (0x42E05D)
	if (slotIdx < 0 || slotIdx >= kMaxObstacles)
		return;

	ObstacleSlot &obs = _obstacles[slotIdx];
	obs.active = true;

	// Random speed: 2/10 for 8, 2/10 for 16, 6/10 for 12
	uint16 speedRoll = _vm->_rnd->getRandomNumber(1, 10);
	if (speedRoll <= 2)
		obs.speed = 8;
	else if (speedRoll >= 9)
		obs.speed = 16;
	else
		obs.speed = 12;

	// IDA tier assignment by speed/spawn-roll. The score ladder maps speed
	// tiers to the SCRB ID range used by maze_obstacleScore at hit time:
	//   speed 8  → SCRB 1000 (basic)
	//   speed 12 → SCRB 1005 (medium)
	//   speed 16 → SCRB 1016 (hard)
	if (obs.speed == 8)
		obs.scrbId = 1000;
	else if (obs.speed == 12)
		obs.scrbId = 1005;
	else
		obs.scrbId = 1016;

	obs.direction = _vm->_rnd->getRandomNumber(0, 7);
	obs.timer = 0;
	_activeObstacleCount++;
}

void ZoombiniPuzzleMaze::moveObstacles() {
	// IDA: maze_updateObstaclePosition (0x42DC56)
	for (int i = 0; i < kMaxObstacles; i++) {
		ObstacleSlot &obs = _obstacles[i];
		if (!obs.active)
			continue;

		obs.timer++;
		if (obs.timer < obs.speed)
			continue;
		obs.timer = 0;

		// 8-directional velocity
		static const int16 dxTable[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
		static const int16 dyTable[8] = {0, 1, 1, 1, 0, -1, -1, -1};

		int16 dir = obs.direction % 8;
		obs.row += dyTable[dir] * obs.speed;
		obs.col += dxTable[dir] * obs.speed;

		// Wrap at screen borders (pixel-space)
		if (obs.col > 650) obs.col = -10;
		else if (obs.col < -10) obs.col = 650;
		if (obs.row > 490) obs.row = -10;
		else if (obs.row < -10) obs.row = 490;
	}
}

void ZoombiniPuzzleMaze::checkObstacleCollisions() {
	// IDA: maze_projectileTickAndCollide (0x42DE69) + maze_obstacleScore tracking.
	// On hit: score added per obstacle SCRB tier, _bonusCounter increments;
	// _bonusCounter resets on consecutive frames with no collisions.
	bool anyHitThisFrame = false;

	// Refresh active runner rect pool for the IDA-style sized intersection test
	// (dword_4AF264[]). The rect pool replaces single-pair distance-squared.
	_activeRunnerRectCount = 0;
	for (int16 j = 0; j < _runnerCount && _activeRunnerRectCount < 6; j++) {
		const ZmbMazeRunnerState &rs = _runnerStates[j];
		if (!rs.moving)
			continue;
		_activeRunnerRects[_activeRunnerRectCount++] =
			Common::Rect(rs.pixelX - 15, rs.pixelY - 15, rs.pixelX + 15, rs.pixelY + 15);
	}

	for (int i = 0; i < kMaxObstacles; i++) {
		ObstacleSlot &obs = _obstacles[i];
		if (!obs.active)
			continue;

		bool hit = false;
		for (int16 r = 0; r < _activeRunnerRectCount && !hit; r++) {
			if (_activeRunnerRects[r].contains(obs.col, obs.row)) {
				hit = true;
				// IDA tier scoring by obstacle SCRB id.
				int16 tierScore = 1;
				switch (obs.scrbId) {
				case 1000: tierScore = 1; break;
				case 1005: tierScore = 2; break;
				case 1016: tierScore = 3; break;
				case 1021: tierScore = 4; break;
				default:   tierScore = 1; break;
				}

				// IDA maze_projectileTickAndCollide @ 0x42DE69 bonus ladder:
				//   maze_obstacleScore += tierScore;
				//   if (maze_obstacleScore >= maze_nextBonusThreshold) {
				//     ++maze_bonusCounter;
				//     if (maze_bonusCounter > 9) maze_bonusCounter = 9;  // cap
				//     maze_nextBonusThreshold += 100;
				//   }
				// The previous port incremented _bonusCounter on every hit
				// and reset it when no hit occurred in a frame — that's a
				// "combo" semantic, not IDA's "extra life per 100 points"
				// ladder. Fix by gating on the 100-pt threshold and capping
				// at 9.
				_obstacleScore += tierScore;
				if (_obstacleScore >= _scoreThreshold) {
					_bonusCounter++;
					if (_bonusCounter > 9)
						_bonusCounter = 9;
					_scoreThreshold += 100;
				}
				anyHitThisFrame = true;

				_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 3001));
				obs.active = false;
				_activeObstacleCount--;
				if (_lives > 0)
					_lives--;
			}
		}
		(void)hit;
	}
	(void)anyHitThisFrame;
}

// =================================================================
// onEveryFrame: main per-frame pipeline
// IDA: maze2_onHover_frameUpdate (0x42F899)
// =================================================================

void ZoombiniPuzzleMaze::onEveryFrame() {
	if (_loadedZmbCount <= 0)
		return;

	if (_reentryGuard)
		return;
	_reentryGuard = true;

	// IDA maze_invalidateVisualRects @ 0x4238bf: the Go button renders enabled
	// only while lilly_bAdvanceEnabled is set (first node arrival).  Drive the
	// ScummVM Go button from _bAdvanceEnabled each frame so it stays disabled
	// until a Zoombini begins crossing the maze.
	setGoButtonsEnabled(_bAdvanceEnabled);

	if (_puzzleReady)
		processQueues();

	if (_activeObstacleCount > 0) {
		debugC(2, kZmbDebugAnimation, "Maze: moving %d obstacles", _activeObstacleCount);
		moveObstacles();
		checkObstacleCollisions();
	}

	processIdleAnimations();

	_reentryGuard = false;
}

void ZoombiniPuzzleMaze::processIdleAnimations() {
	// IDA: maze2_onHover_frameUpdate @ idle/celebration section
	if (_celebrationTrigger && _celebrationsPlayed < _celebrationTarget) {
		if (getCurrentFrameCounter() - _celebrationLastFrame > 30) {
			bool triggered = false;
			_celebrationLastFrame = getCurrentFrameCounter();

			for (int16 i = 0; i < _loadedZmbCount && !triggered; i++) {
				uint16 poolIdx = _vm->_rnd->getNonRepeatRandom(_loadedZmbCount, _celebrationPoolState);
				uint16 snoidId = 10000 + poolIdx;
				ZmbSnoid *snoid = getSnoid(snoidId);

				if (snoid && snoid->isRenderActivated()) {
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

// ============================================================================
// Player projectile system (IDA maze_spawnProjectile + tickProjectilePosition)
// ============================================================================

void ZoombiniPuzzleMaze::spawnProjectile() {
	// IDA maze_spawnProjectile @ 0x42D8E9: launches a projectile from the
	// launcher at (320, 240) in the current 8-direction at 14 u/tick.
	// Only one projectile active at a time; no-op if already flying.
	if (_projectile.active)
		return;

	// IDA 8-direction velocity table. Dir 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW.
	static const int16 kDx[8] = {   0,  14,  14,  14,   0, -14, -14, -14 };
	static const int16 kDy[8] = { -14, -14,   0,  14,  14,  14,   0, -14 };

	int16 dir = _launcherDirection & 7;
	_projectile.active = true;
	_projectile.x = 320;
	_projectile.y = 240;
	_projectile.dx = kDx[dir];
	_projectile.dy = kDy[dir];
	_projectile.lifeFrames = 0;

	// Launch SFX — IDA enqueues a firing sound resource.
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 3000),
		Audio::Mixer::kSFXSoundType);
}

void ZoombiniPuzzleMaze::tickProjectile() {
	// IDA maze_tickProjectilePosition @ 0x42E2F2: advance position by
	// (dx, dy); deactivate at frame > 15 OR when off-screen. Collision with
	// obstacles handled by checkObstacleCollisions using the rect pool.
	if (!_projectile.active)
		return;

	_projectile.x += _projectile.dx;
	_projectile.y += _projectile.dy;
	_projectile.lifeFrames++;

	if (_projectile.lifeFrames > 15 ||
	    _projectile.x < 0 || _projectile.x > 640 ||
	    _projectile.y < 0 || _projectile.y > 480) {
		_projectile.active = false;
		return;
	}

	// Collision test: if projectile overlaps any active obstacle, score hit.
	for (int i = 0; i < kMaxObstacles; i++) {
		ObstacleSlot &obs = _obstacles[i];
		if (!obs.active)
			continue;
		int16 dx = obs.col - _projectile.x;
		int16 dy = obs.row - _projectile.y;
		if (dx * dx + dy * dy < 25 * 25) {
			// Hit — deactivate both projectile and obstacle; award score.
			_projectile.active = false;
			int16 tierScore = 1;
			switch (obs.scrbId) {
			case 1000: tierScore = 1; break;
			case 1005: tierScore = 2; break;
			case 1016: tierScore = 3; break;
			case 1021: tierScore = 4; break;
			default:   tierScore = 1; break;
			}
			_obstacleScore += tierScore;
			if (_obstacleScore >= _scoreThreshold) {
				_bonusCounter++;
				if (_bonusCounter > 9) _bonusCounter = 9;
				_scoreThreshold += 100;
			}
			_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 3001),
				Audio::Mixer::kSFXSoundType);
			obs.active = false;
			_activeObstacleCount--;
			updateScoreDigits();
			return;
		}
	}
}

void ZoombiniPuzzleMaze::updateScoreDigits() {
	// IDA maze_updateScoreDigits @ 0x42D04D: reloads SCRB 8011 on the score-
	// display runner with the current digit frames. Each digit occupies a
	// hotspot; the SCRB has 10 frames (0-9) per digit slot.
	if (!_scoreDigitFeature)
		return;
	// The SCRB animation frame index equals the current score digit value.
	// ScummVM handles this by reloading the SCRB which re-triggers the
	// animation; the pre-render shape callback would pick the right digit
	// frame per digit slot. Minimal: force reload so the display refreshes.
	loadScrbOntoFeature(_scoreDigitFeature, 8011);
}

} // End of namespace Mohawk
