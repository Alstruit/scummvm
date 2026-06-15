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
#include "mohawk/zoombini_pages/puzzle_lilly.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

namespace {

const char *lillyPadAttrTypeName(LillyPadAttrType type) {
	switch (type) {
	case kLillyPadAttrPattern:
		return "pattern";
	case kLillyPadAttrShape:
		return "pad-shape";
	case kLillyPadAttrColor:
		return "color";
	default:
		return "none";
	}
}

const char *lillyPadPatternName(LillyPadPattern pattern) {
	switch (pattern) {
	case kLillyPadPatternFlower:
		return "flower";
	case kLillyPadPatternCross:
		return "cross";
	case kLillyPadPatternDiamond:
		return "diamond";
	default:
		return "unknown";
	}
}

const char *lillyPadColorName(LillyPadColor color) {
	switch (color) {
	case kLillyPadColorMagenta:
		return "magenta";
	case kLillyPadColorRed:
		return "red";
	case kLillyPadColorOrange:
		return "orange";
	case kLillyPadColorCyan:
		return "cyan";
	case kLillyPadColorBeige:
		return "beige";
	default:
		return "unknown";
	}
}

const char *lillyPadShapeName(LillyPadShape shape) {
	switch (shape) {
	case kLillyPadShapeOneCut:
		return "oneCut";
	case kLillyPadShapeTwoCut:
		return "twoCut";
	case kLillyPadShapeThreePointed:
		return "threePointed";
	case kLillyPadShapeFourPointed:
		return "fourPointed";
	default:
		return "unknown";
	}
}

Common::String formatLillyPadAttrValue(LillyPadAttrType type, int value) {
	switch (type) {
	case kLillyPadAttrPattern:
		if (0 <= value && value <= 2)
			return Common::String::format("pattern=%s(%d)", lillyPadPatternName(static_cast<LillyPadPattern>(value)), value);
		break;
	case kLillyPadAttrShape:
		if (0 <= value && value <= 3)
			return Common::String::format("padShape=%s(%d)", lillyPadShapeName(static_cast<LillyPadShape>(value)), value);
		break;
	case kLillyPadAttrColor:
		if (0 <= value && value <= 4)
			return Common::String::format("color=%s(%d)", lillyPadColorName(static_cast<LillyPadColor>(value)), value);
		break;
	default:
		break;
	}
	return Common::String::format("value=%d", value);
}

} // namespace

// =================================================================
// Static data tables (from IDA binary data)
// =================================================================

// IDA: word_4A1738/40/48/50 — direction SCRB tables
// Indexed by previous direction (0=up, 1=right, 2=down, 3=left)
const uint16 ZoombiniPuzzleLilly::kDirScrbUp[4]    = {10001, 10010, 10015, 10008};
const uint16 ZoombiniPuzzleLilly::kDirScrbRight[4] = {10005, 10002, 10011, 10016};
const uint16 ZoombiniPuzzleLilly::kDirScrbDown[4]  = {10013, 10006, 10003, 10012};
const uint16 ZoombiniPuzzleLilly::kDirScrbLeft[4]  = {10009, 10014, 10007, 10004};

// IDA: word_4A171E — Y offset per column for cell positions
const int16 ZoombiniPuzzleLilly::kColYOffset[13] = {
	2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 0
};

// IDA: word_4A16EC — preset swap pair column coordinates
const int16 ZoombiniPuzzleLilly::kSwapPairCol[20] = {
	4, 0, 3, 0, 8, 0, 10, 0, 0, 0, 4, 0, 6, 0, 3, 0, 5, 0, 0, 0
};

// IDA: word_4A1700 — preset swap pair row coordinates
const int16 ZoombiniPuzzleLilly::kSwapPairRow[20] = {
	4, 0, 6, 0, 3, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 4, 4, 6
};

// IDA: word_4A1832 — zoombini count → required grid row count
const int16 ZoombiniPuzzleLilly::kZmbToRowCount[21] = {
	1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10
};

// IDA: byte_4A181E — combined attr base: combinedAttr = attr1 + kCombinedAttrBase[attr3]
const byte ZoombiniPuzzleLilly::kCombinedAttrBase[5] = {5, 8, 11, 14, 17};

// IDA: word_4A185C — row/column validity for pattern placement (0=invalid)
const int16 ZoombiniPuzzleLilly::kRowColValidity[13] = {
	0, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0
};

// IDA: word_4A17EA — pattern attr type pool (indices 0-12)
const LillyPadAttrType ZoombiniPuzzleLilly::kPatternAttrType[13] = {
	kLillyPadAttrPattern, kLillyPadAttrPattern, kLillyPadAttrPattern,
	kLillyPadAttrShape, kLillyPadAttrShape, kLillyPadAttrShape, kLillyPadAttrShape,
	kLillyPadAttrColor, kLillyPadAttrColor, kLillyPadAttrColor, kLillyPadAttrColor, kLillyPadAttrColor,
	kLillyPadAttrNone
};

// IDA: word_4A1804 — pattern attr value pool
const int16 ZoombiniPuzzleLilly::kPatternAttrValue[13] = {
	0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4, 0
};

// IDA: word_4A14A6 — pattern extra index pool
const int16 ZoombiniPuzzleLilly::kPatternAttrExtra[13] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0
};

// IDA: dword_4A1650 — entry/exit positions (12 rows)
const Common::Point ZoombiniPuzzleLilly::kEntryPositions[12] = {
	{66, 118}, {60, 147}, {46, 177}, {58, 205}, {44, 232}, {52, 262},
	{39, 289}, {18, 313}, {47, 327}, {17, 345}, {43, 363}, {53, 393}
};

// IDA: unk_4A15A8 — initial staging positions for zoombini runners (20 slots)
// These are the upper-left waiting area positions before grid entry.
const Common::Point ZoombiniPuzzleLilly::kInitialPositions[20] = {
	{101, 27}, {100, 42}, {95, 55}, {88, 69}, {78, 80},
	{88, 24},  {85, 39},  {80, 54}, {72, 67}, {62, 81},
	{74, 25},  {70, 40},  {64, 53}, {56, 67}, {46, 79},
	{59, 25},  {55, 39},  {49, 53}, {41, 65}, {44, 31}
};

// IDA: word_4A14C0 — BFS layer offset by attr type.
// attrType 0=unused, 1→offset 0, 2→offset 3, 3→offset 7
const int16 ZoombiniPuzzleLilly::kObstacleBFSOffset[5] = {0, 0, 3, 7, 0};

// =================================================================
// Construction / Lifecycle
// =================================================================

ZoombiniPuzzleLilly::ZoombiniPuzzleLilly(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kLilly) {
	memset(_gridPattern, 0, sizeof(_gridPattern));
	memset(_gridOccupancy, 0, sizeof(_gridOccupancy));
	memset(_gridExitReservation, 0, sizeof(_gridExitReservation));
	memset(_gridAttr1, 0, sizeof(_gridAttr1));
	memset(_gridAttr2, 0, sizeof(_gridAttr2));
	memset(_gridAttr3, 0, sizeof(_gridAttr3));
	memset(_gridCombinedAttr, 0, sizeof(_gridCombinedAttr));
	memset(_enterQueue, 0, sizeof(_enterQueue));
	memset(_exitQueue, 0, sizeof(_exitQueue));
	memset(_crossQueue, 0, sizeof(_crossQueue));
	memset(_rotateQueue, 0, sizeof(_rotateQueue));
	memset(_arriveQueue, 0, sizeof(_arriveQueue));
	memset(_departQueue, 0, sizeof(_departQueue));
	memset(_readyQueue, 0, sizeof(_readyQueue));
	memset(_moveQueue, 0, sizeof(_moveQueue));
	memset(_pendingMoveQueue, 0, sizeof(_pendingMoveQueue));
	memset(_pathInitQueue, 0, sizeof(_pathInitQueue));
	memset(_obstacleRunners, 0, sizeof(_obstacleRunners));
	memset(_activeObstacles, 0, sizeof(_activeObstacles));
	memset(_freedRunners, 0, sizeof(_freedRunners));
	memset(_obstacleGrid, 0, sizeof(_obstacleGrid));
	for (int i = 0; i < kMaxRunners; i++)
		_runnerStates[i].clear();
}

ZoombiniPuzzleLilly::~ZoombiniPuzzleLilly() {
}

void ZoombiniPuzzleLilly::open() {
	openArchive(ZMB_MHK_LILLY);
}

void ZoombiniPuzzleLilly::setBackgroundMusic() {
	// IDA: lilly_puzzleInit (0x422de4) has no music playback call on page load.
}

void ZoombiniPuzzleLilly::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

// =================================================================
// loadFeatures — full puzzle initialization
// =================================================================

void ZoombiniPuzzleLilly::loadFeatures() {
	// IDA: lilly_puzzleInit (0x422de4)
	_difficultyLevel = static_cast<ZmbPuzzleDifficultyLevel>(_vm->_state->readActivePageRouteLevel() + 1);

	// Preload shape images
	_vm->_gfx->preloadImage(7000);
	_vm->_gfx->preloadImage(13000);

	// Load main features: 1 SCRB at 11000
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 5, 0x36B0) — 5 subs at 14000
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 5; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 14000), 14000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 167, 0x2710) — 167 subs at 10000
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 167; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 10000), 10000 + i);
		}
	}

	// Load REGS resources
	loadREGS(ZmbArchiveKind::kPage, 100);
	loadREGS(ZmbArchiveKind::kPage, 10000);
	loadREGS(ZmbArchiveKind::kPage, 200);

	// Load zoombinis from pack
	loadZoombinisFromPack();

	// Initialize difficulty
	setDifficultyParams();

	// Load REGS coordinate tables for cell positioning and path interpolation
	// NOTE: Must be called BEFORE initGridWithAttributes() because the grid init
	// uses _regsXTable/_regsYTable to compute cell positions.
	loadRegsCoordinateTables();

	// Initialize grid
	initGridWithAttributes();

	// IDA: word_4A14C8 — virtual grid renderer with custom render callback
	// Original: runner_registerAndAllocate(0,0,0,0,0,
	//     maze_clearAndInvalidateRect, maze_renderAllGridSprites_426BFB, FLAGS)
	// scrbId=0: callback-only runner with no SCRB animation data.
	{
		ZmbFeature::EventHooks hooks;
		hooks.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(
			&ZoombiniPuzzleLilly::renderGridSprites));
		_gridRendererFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 0, 0,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY,
			hooks);
	}

	// IDA: lilly_cursorRunnerIdx — cursor indicator with custom render callback
	// Original: runner_registerAndAllocate(0,0,0,5,0,
	//     maze_computeDrawnCellRect, maze_renderCursorIndicator_426DF9, FLAGS)
	// scrbId=0: callback-only runner with no SCRB animation data.
	{
		ZmbFeature::EventHooks hooks;
		hooks.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(
			&ZoombiniPuzzleLilly::renderCursorIndicator));
		_cursorRunnerFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 0, 5,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY,
			hooks);
	}

	// IDA: lilly_cellAnimRunnerA/B — cell animation runners
	// Original: wBoolDoRender=0 initially; activated later by setRunnerClickRect().
	_cellAnimRunnerA = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10002, 4,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
	_cellAnimRunnerA->deactivateRender();
	_cellAnimRunnerB = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10003, 4,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
	_cellAnimRunnerB->deactivateRender();

	// Create per-zoombini runners
	createZoombiniRunners();

	// IDA: maze_registerObstacleRunners_42651C — register 12 initial obstacle
	// runners ("toads") on the left bank at entry positions with SCRB 10043+j.
	createInitialObstacleRunners();

	// 5 overlay features for SCRB 14000-14004
	for (uint16 i = 0; i < 5; i++) {
		_overlayFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 14000), 14000 + i, 0,
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// Frog obstacle (difficulty > 1)
	if (_difficultyLevel >= kPuzzleDiffLevel2) {
		_frogScrbFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 11000, 5,
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);

		// IDA 0x42354A: runner_registerAndAllocate(0, 0, &pFeatureCore, 6, 10078+unlockProgress,
		//     runner_preRenderStandard, runner_postRenderStandard,
		//     FLAG_00000002|FLAG_00080000|FLAG_00100000|FLAG_04000000)
		// After creation: v7->bitmask = 0; v7->core188.wBoolDoRender = 0;
		//     v7->core188.posLoc = (38, 415).
		// Event 4 activation: v5->bitmask = 0x980002 (POS_DELTA|PLAY_ONCE|DEFER_ANIM|TYPE_TOWN).
		//   scrb_loadOnRunner at 0x4604DC: when POS_DELTA set, pos2 = hsArr[0].pos.
		//   ScummVM: initValues() sets _pointRef from SCRB first hotspot when POS_DELTA present.
		// FLAG_01000000_DEFER_RENDER: needed so deactivateRender() actually suppresses blitShapes.
		_frogRunnerFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 10078, 6,
			ZmbFeature::FLAG_00800000_POS_DELTA |
			ZmbFeature::FLAG_01000000_DEFER_RENDER |
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);

		// IDA 0x423567: Cell select state = 4 for difficulty >= 2
		_cellSelectState = 4;
		if (_frogRunnerFeature) {
			// IDA 0x423590: posLoc = (38, 415). With POS_DELTA, getPosDelta() returns
			// posLoc - pointRef = (38,415) - SCRB_first_hotspot(332,272) = (-294, 143).
			// blitShapes adds this delta to shape positions, centering the wand at (38, 415).
			_frogRunnerFeature->setPointLoc(Common::Point(38, 415));
			_frogRunnerFeature->deactivateRender();

			// Apply per-shape REGS for registration point correction.
			auto itRegs = _regsMap.find(10000);
			if (itRegs != _regsMap.end())
				_frogRunnerFeature->setShapeRegs(itRegs->_value);
		}
	}

	// NOTE: Original engine used 12 no-op overlay runners (word_4AE3AA) purely for
	// render ordering. ScummVM's per-frame Z-sorting makes these unnecessary.

	// Buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(7000);
	loadHelpButtonFeature();

	{ // Help sound selection
		ZMB_DIFFICULTY_ID diffId = _vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagLilly);
		uint16 helpSoundId;
		if (diffId == ZMB_DIFFICULTY_LEVEL2_02) {
			helpSoundId = _vm->_rnd->getRandomNumber(20076, 20077);
		} else if (_difficultyLevel <= kPuzzleDiffLevel1) {
			helpSoundId = 20075;
		} else {
			helpSoundId = _vm->_rnd->getRandomNumber(20075, 20077);
		}
		_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, helpSoundId);
	}

	// Initialize obstacle timer
	// IDA: lilly_nextObstacleTimer = current_frame + 600
	_nextObstacleTimer = getCurrentFrameCounter() + 600;

	// IDA 0x4237A1: At end of init, explicitly re-load the frog SCRB to start
	// the intro animation. The original calls scrb_loadOnRunner(1, 11000, frogRunner)
	// which overrides DEFER_ANIM via scheduleRender → activateRender + activateAnimate.
	// Without this, the SCRB 11000 animation never plays and events 3/4/5 never fire.
	if (_difficultyLevel >= kPuzzleDiffLevel2 && _frogScrbFeature) {
		loadScrbOntoFeature(_frogScrbFeature, 11000);
		// IDA: ui_bDragLockActive = totalCount - (totalCount-1) = 1
		_remainingZmbs = 1;
	}

	// Activate puzzle
	_bPuzzleActive = true;
	_bRenderEnabled = true;
}

// =================================================================
// Initialization helpers
// =================================================================

void ZoombiniPuzzleLilly::onGoButtonActivated() {
	// IDA: lilly_onClickHandler case 2
	_departXferSrcSiPage = ZMB_SI_LILLY_08;
	ZoombiniInteractive::onGoButtonActivated();
}

Common::String ZoombiniPuzzleLilly::debugGetAnswer() const {
	// Lilly pad attributes are independent from Zoombini traits:
	// attr1=pattern (flower/X/diamond), attr2=pad shape, attr3=color.
	static const char *kGridStrategyNames[] = {
		"mixed board (no dedicated obstacle family)",
		"mixed board (no dedicated obstacle family)",
		"mixed board (no dedicated obstacle family)",
		"pattern-driven board",
		"pad-shape-driven board",
		"color-driven board"
	};

	int scriptedSlotCount = 0;
	int scriptedPlacementCount = 0;
	for (int i = 1; i < 13; i++) {
		if (_patternMask[i] == 0)
			scriptedSlotCount++;
		if (_patternUsageCount[i] > 0)
			scriptedPlacementCount += _patternUsageCount[i];
	}

	const char *strategyName = (0 <= _gridType && _gridType <= 5) ? kGridStrategyNames[_gridType] : "unknown";
	const char *primaryFamily = (_gridType == 3) ? "pattern" :
		(_gridType == 4) ? "pad-shape" :
		(_gridType == 5) ? "color" : "none";

	Common::String s = Common::String::format("Lilly (level %d): %s\n",
		_difficultyLevel, strategyName);
	s += Common::String::format("  Strategy: gridType=%d, primaryFamily=%s, zoombinis=%d, scriptedSlots=%d, scriptedPlacements=%d, swapThreshold=%d\n",
		_gridType, primaryFamily, _totalZmbCount, scriptedSlotCount, scriptedPlacementCount, _swapThreshold);

	if (_difficultyLevel == kPuzzleDiffLevel1) {
		s += Common::String::format("    level1 uses mixed cells with only %d scripted slot(s) enabled from the 12 pattern slots; remaining attributes are random-filled.\n",
			scriptedSlotCount);
	} else if (_difficultyLevel == kPuzzleDiffLevel2) {
		s += "    level2 enables the full mixed pattern pool, then unlocks cell-swaps; no dedicated obstacle family is used.\n";
	} else {
		s += Common::String::format("    level%d picks one lily-pad attribute family as the obstacle-driving layer; top-row hits from that family become obstacle entry columns.\n",
			_difficultyLevel);
	}

	s += "  Pattern family order (chosen without replacement):\n";
	s += "    pattern   slots 1-3 : ";
	for (int i = 1; i <= 3; i++) {
		s += Common::String::format("[%d:%s placements=%d] ", i,
			formatLillyPadAttrValue(_patternType[i], _patternValue[i]).c_str(), _patternUsageCount[i]);
	}
	s += "\n    pad-shape slots 4-7 : ";
	for (int i = 4; i <= 7; i++) {
		s += Common::String::format("[%d:%s placements=%d] ", i,
			formatLillyPadAttrValue(_patternType[i], _patternValue[i]).c_str(), _patternUsageCount[i]);
	}
	s += "\n    color     slots 8-12: ";
	for (int i = 8; i <= 12; i++) {
		s += Common::String::format("[%d:%s placements=%d] ", i,
			formatLillyPadAttrValue(_patternType[i], _patternValue[i]).c_str(), _patternUsageCount[i]);
	}
	s += "\n";

	s += "  Active scripted slots: ";
	bool hasActiveSlot = false;
	for (int i = 1; i < 13; i++) {
		if (_patternMask[i] != 0 || _patternType[i] == kLillyPadAttrNone)
			continue;
		hasActiveSlot = true;
		LillyPadAttrType t = _patternType[i];
		const char *name = lillyPadAttrTypeName(t);
		s += Common::String::format("%d:%s(%s, placements=%d) ", i, name,
			formatLillyPadAttrValue(t, _patternValue[i]).c_str(), _patternUsageCount[i]);
	}
	if (!hasActiveSlot)
		s += "none";
	s += "\n";

	if (_obstacleEntryCount > 0) {
		const char *obsName = lillyPadAttrTypeName(_obstacleAttrType);
		s += Common::String::format("  Obstacle plan: sharedAttr=%s(%d), entryCount=%d, entryColumns=",
			obsName, static_cast<int>(_obstacleAttrType), _obstacleEntryCount);
		for (int i = 0; i < _obstacleEntryCount && i < 16; i++) {
			if (i != 0)
				s += ", ";
			s += Common::String::format("col%d(%s)", _obstacleEntryCols[i],
				formatLillyPadAttrValue(_obstacleEntryType[i], _obstacleEntryValue[i]).c_str());
		}
		s += "\n";
		s += "    obstacle runners read the chosen lily-pad attribute from row 0 of their entry column and pathfind only through cells with that same value.\n";

		// Intended answer: toad tattoo → path entry column
		s += "  Intended toad paths (place toad with matching tattoo on entry column):\n";
		for (int i = 0; i < _obstacleEntryCount && i < 16; i++) {
			s += Common::String::format("    Entry col %d: toad with %s tattoo\n",
				_obstacleEntryCols[i],
				formatLillyPadAttrValue(_obstacleEntryType[i], _obstacleEntryValue[i]).c_str());
		}
	} else {
		s += "  Obstacle plan: none; this board is solved by reading the mixed grid and, from level 2 onward, using swaps to repair bad routes.\n";
		// For mixed boards, the intended answer is each active scripted slot
		s += "  Intended paths (active scripted slots with placements > 0):\n";
		bool anyActive = false;
		for (int i = 1; i < 13; i++) {
			if (_patternMask[i] != 0 || _patternType[i] == kLillyPadAttrNone || _patternUsageCount[i] == 0)
				continue;
			anyActive = true;
			s += Common::String::format("    Slot %d: toad with %s tattoo (%d placements on board)\n",
				i, formatLillyPadAttrValue(_patternType[i], _patternValue[i]).c_str(), _patternUsageCount[i]);
		}
		if (!anyActive)
			s += "    (no scripted paths established yet)\n";
	}
	return s;
}

void ZoombiniPuzzleLilly::loadZoombinisFromPack() {
	ZmbStateFile &f = _vm->_state->_f;
	const Common::Point offscreenPos(680, 220);
	uint16 posIdx = 0;
	_activeBfsRunnerCount = 0;

	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount; i++) {
		ZmbStateActiveEntry &entry = f._zmbPackActive._entries[i];
		if (!entry._bIsOccupied)
			continue;

		if (posIdx < kMaxRunners)
			_activeBfsRunners[_activeBfsRunnerCount++] = posIdx;

		uint16 snoidId = 10000 + posIdx;
		ZmbSnoid *snoid = loadSnoidFromPack(snoidId, offscreenPos,
		                                    ZmbFeature::FLAG_00000001_TYPE_SNOID);
		if (snoid) {
			snoid->_trait = entry._traits;
			snoid->_name = entry.getU32Name(_vm);
			snoid->_packIsOccupied = true;
			snoid->deactivateRender();
		}
		posIdx++;
	}

	_totalZmbCount = posIdx;
	_remainingZmbs = _totalZmbCount;
}

void ZoombiniPuzzleLilly::setDifficultyParams() {
	// IDA: lilly_setDifficultyParams (0x4264AC)
	switch (_difficultyLevel) {
	case kPuzzleDiffLevel1:
		_obstacleRows = 0;
		break;
	case kPuzzleDiffLevel2:
		_obstacleRows = 0;
		break;
	case kPuzzleDiffLevel3:
		_obstacleRows = 2;
		break;
	case kPuzzleDiffLevel4:
	default:
		_obstacleRows = 3;
		break;
	}
}

void ZoombiniPuzzleLilly::loadRegsCoordinateTables() {
	// Load REGS 100 X/Y arrays for cell position computation
	auto it100 = _regsMap.find(100);
	if (it100 != _regsMap.end()) {
		ZmbRegs *regs100 = it100->_value;
		_regsXTable.clear();
		_regsYTable.clear();
		for (uint i = 0; i < regs100->_offsets.size(); i++) {
			_regsXTable.push_back(regs100->_offsets[i].x);
			_regsYTable.push_back(regs100->_offsets[i].y);
		}
	}

	// Load REGS 200 for path interpolation deltas
	auto it200 = _regsMap.find(200);
	if (it200 != _regsMap.end()) {
		ZmbRegs *regs200 = it200->_value;
		_regsDeltaX.clear();
		_regsDeltaY.clear();
		for (uint i = 0; i < regs200->_offsets.size(); i++) {
			_regsDeltaX.push_back(regs200->_offsets[i].x);
			_regsDeltaY.push_back(regs200->_offsets[i].y);
		}
	}
}

void ZoombiniPuzzleLilly::loadGridPatternRegs(int gridIdx, uint16 resId) {
	// IDA: dword_4AC038/03C/040 — load a single REGS resource as 12x12 int16 grid.
	// Each REGS resource is 288 bytes = 144 big-endian int16 values, stored row-major.
	assert(gridIdx >= 0 && gridIdx < 3);
	memset(_gridPattern[gridIdx], 0, sizeof(_gridPattern[gridIdx]));

	Common::SeekableReadStream *stream = _vm->getResource(ID_REGS,
		ZmbResource(ZmbArchiveKind::kPage, resId));
	if (!stream) {
		warning("ZoombiniPuzzleLilly: Failed to load REGS %d for grid pattern %d", resId, gridIdx);
		return;
	}

	for (int row = 0; row < 12 && !stream->eos(); row++) {
		for (int col = 0; col < 12 && !stream->eos(); col++) {
			_gridPattern[gridIdx][row][col] = stream->readSint16BE();
		}
	}
	delete stream;
}

void ZoombiniPuzzleLilly::rotateGrid(int rotType, int16 grid[12][12]) {
	// IDA: maze_rotateGrid12x12_4283C9
	// rotType 0 = 90 degrees CW, 1 = 180 degrees, 2 = 90 degrees CCW
	int16 temp[12][12];
	memset(temp, 0, sizeof(temp));

	bool didRotate = false;
	switch (rotType) {
	case 0: // 90 degrees CW: temp[col][11 - row] = grid[row][col]
		for (int16 row = 0; row < 12; row++) {
			for (int16 col = 0; col < 12; col++) {
				temp[col][11 - row] = grid[row][col];
			}
		}
		didRotate = true;
		break;
	case 1: // 180 degrees: temp[11 - row][11 - col] = grid[row][col]
		for (int16 row = 0; row < 12; row++) {
			for (int16 col = 0; col < 12; col++) {
				temp[11 - row][11 - col] = grid[row][col];
			}
		}
		didRotate = true;
		break;
	case 2: // 90 degrees CCW: temp[11 - col][row] = grid[row][col]
		for (int16 row = 0; row < 12; row++) {
			for (int16 col = 0; col < 12; col++) {
				temp[11 - col][row] = grid[row][col];
			}
		}
		didRotate = true;
		break;
	default:
		break;
	}

	if (didRotate) {
		memcpy(grid, temp, sizeof(temp));
	}
}

void ZoombiniPuzzleLilly::flipGrid(int flipType, int16 grid[12][12]) {
	// IDA: maze_flipGrid12x12_428555
	// flipType 0 = horizontal (mirror columns), 1 = vertical (mirror rows)
	int16 temp[12][12];
	memset(temp, 0, sizeof(temp));

	bool didFlip = false;
	switch (flipType) {
	case 0: // Horizontal flip: temp[row][11 - col] = grid[row][col]
		for (int16 row = 0; row < 12; row++) {
			for (int16 col = 0; col < 12; col++) {
				temp[row][11 - col] = grid[row][col];
			}
		}
		didFlip = true;
		break;
	case 1: // Vertical flip: temp[11 - row][col] = grid[row][col]
		for (int16 col = 0; col < 12; col++) {
			for (int16 row = 0; row < 12; row++) {
				temp[11 - row][col] = grid[row][col];
			}
		}
		didFlip = true;
		break;
	default:
		break;
	}

	if (didFlip) {
		memcpy(grid, temp, sizeof(temp));
	}
}

void ZoombiniPuzzleLilly::generateChallengePatterns() {
	// IDA: fleens_generateChallengePatterns (0x427719)
	// Generates 12 challenge pattern triplets (type, value, extra) by
	// random selection without replacement from 3 pools.

	// Pool A: indices 0-2 (attr type 1 = lily-pad pattern, 3 values: flower/X/diamond)
	LillyPadAttrType poolAType[4] = {};
	int16 poolAValue[4] = {};
	int16 poolAExtra[4] = {};
	for (int i = 0; i < 3; i++) {
		poolAType[i] = kPatternAttrType[i];
		poolAValue[i] = kPatternAttrValue[i];
		poolAExtra[i] = kPatternAttrExtra[i];
	}

	// Pool B: indices 3-6 (attr type 2 = lily-pad color, 4 values)
	LillyPadAttrType poolBType[5] = {};
	int16 poolBValue[5] = {};
	int16 poolBExtra[5] = {};
	for (int i = 0; i < 4; i++) {
		poolBType[i] = kPatternAttrType[3 + i];
		poolBValue[i] = kPatternAttrValue[3 + i];
		poolBExtra[i] = kPatternAttrExtra[3 + i];
	}

	// Pool C: indices 7-11 (attr type 3 = lily-pad shape, 5 values)
	LillyPadAttrType poolCType[6] = {};
	int16 poolCValue[6] = {};
	int16 poolCExtra[6] = {};
	for (int i = 0; i < 5; i++) {
		poolCType[i] = kPatternAttrType[7 + i];
		poolCValue[i] = kPatternAttrValue[7 + i];
		poolCExtra[i] = kPatternAttrExtra[7 + i];
	}

	int16 poolASize = 2; // 0-based max index (3 entries: indices 0,1,2)
	int16 poolBSize = 3; // 4 entries
	int16 poolCSize = 4; // 5 entries

	for (int16 i = 1; i < 13; i++) {
		LillyPadAttrType *curType;
		int16 *curValue, *curExtra;
		int16 *curSize;

		if (static_cast<uint>(i - 1) < 3) {
			curType = poolAType;
			curValue = poolAValue;
			curExtra = poolAExtra;
			curSize = &poolASize;
		} else if (static_cast<uint>(i - 4) < 4) {
			curType = poolBType;
			curValue = poolBValue;
			curExtra = poolBExtra;
			curSize = &poolBSize;
		} else {
			curType = poolCType;
			curValue = poolCValue;
			curExtra = poolCExtra;
			curSize = &poolCSize;
		}

		// Random pick from current pool (0..*curSize)
		int16 pick = _vm->_rnd->getRandomNumber(0, *curSize);
		_patternType[i] = curType[pick];
		_patternValue[i] = curValue[pick];
		_patternExtra[i] = curExtra[pick];

		// Remove picked element by shifting left
		for (int16 j = pick; j < *curSize + 1; j++) {
			curType[j] = curType[j + 1];
			curValue[j] = curValue[j + 1];
			curExtra[j] = curExtra[j + 1];
		}
		--(*curSize);
	}
}

void ZoombiniPuzzleLilly::initGridWithAttributes() {
	// IDA: fleens_initGridWithAttributes (0x427955) — 2465 bytes.
	// Full grid initialization: loads REGS patterns, selects grid type by difficulty,
	// rotates/flips randomly, generates challenge patterns, assigns cell attributes.

	// --- Phase 1: Clear pattern tracking arrays ---
	memset(_patternUsageCount, 0, sizeof(_patternUsageCount));
	for (int16 i = 0; i < 13; i++) {
		_patternMask[i] = i;
		_rowShuffle[i] = i;
	}

	// --- Phase 2: Clear occupancy grid ---
	for (int16 row = 0; row < 12; row++) {
		for (int16 col = 0; col < 13; col++) {
			_gridOccupancy[row][col] = 0;
			_gridExitReservation[row][col] = 0;
		}
	}

	// --- Phase 3: Load grid pattern REGS ---
	loadGridPatternRegs(0, 15000);
	loadGridPatternRegs(1, 15001);
	loadGridPatternRegs(2, 15002);

	// --- Phase 4: Select grid type and rotate primary pattern by difficulty ---
	int16 v41 = 0; // max obstacle entry count (for diff 3/4)
	int16 rowsToRemove = 12; // how many rows to mask out from pattern

	if (_difficultyLevel != kPuzzleDiffLevel1 && _difficultyLevel != kPuzzleDiffLevel2) {
		if (_difficultyLevel == kPuzzleDiffLevel3) {
			v41 = 2;
			if (kZmbToRowCount[_totalZmbCount] < 8) {
				int16 rndGrid = _vm->_rnd->getRandomNumber(3, 5);
				if (rndGrid == 3) {
					_gridType = 3;
					rotateGrid(0, _gridPattern[0]);
				} else if (rndGrid == 4) {
					_gridType = 4;
					rotateGrid(0, _gridPattern[1]);
				} else {
					_gridType = 5;
					rotateGrid(0, _gridPattern[2]);
				}
			} else {
				_gridType = 4;
				rotateGrid(0, _gridPattern[1]);
			}
		} else if (_difficultyLevel == kPuzzleDiffLevel4) {
			v41 = 3;
			if (kZmbToRowCount[_totalZmbCount] < 8 &&
			    _vm->_rnd->getRandomNumber(4, 5) != 4) {
				_gridType = 5;
				rotateGrid(0, _gridPattern[2]);
			} else {
				_gridType = 4;
				rotateGrid(0, _gridPattern[1]);
			}
		}
		rowsToRemove = 12; // diff 3/4: no row removal (all rows active)
	} else {
		// Difficulty 1 or 2: no pattern grid used (gridType=0)
		v41 = 0;
		_gridType = 0;
		if (_difficultyLevel == kPuzzleDiffLevel1) {
			rowsToRemove = 12 - kZmbToRowCount[_totalZmbCount];
		} else {
			rowsToRemove = 12; // diff 2: remove all rows
		}
	}

	// --- Phase 5: Random rotation/flip of all 3 grids ---
	int16 transformType = _vm->_rnd->getRandomNumber(0, 2);
	if (transformType == 0) {
		// 180 degree rotation on all grids
		rotateGrid(1, _gridPattern[0]);
		rotateGrid(1, _gridPattern[1]);
		rotateGrid(1, _gridPattern[2]);
	} else if (transformType == 1) {
		// Random flip on each grid
		flipGrid(_vm->_rnd->getRandomNumber(0, 1), _gridPattern[0]);
		flipGrid(_vm->_rnd->getRandomNumber(0, 1), _gridPattern[1]);
		flipGrid(_vm->_rnd->getRandomNumber(0, 1), _gridPattern[2]);
	}
	// transformType == 2: no additional transform

	// --- Phase 6: Row removal shuffle ---
	// Randomly select `rowsToRemove` rows to mask out.
	// IDA: For each iteration, pick random index from shrinking shuffle array,
	// set mask[shuffle[pick]]=0, then compact shuttle array.
	int16 shuffleMax = 12; // starts at 12
	for (int16 j = 0; j < rowsToRemove; j++) {
		int16 pick = _vm->_rnd->getRandomNumber(1, shuffleMax);
		_patternMask[_rowShuffle[pick]] = 0;
		// Shift shuffle array left to remove picked entry
		for (int16 s = pick; s < shuffleMax + 1; s++) {
			_rowShuffle[s] = _rowShuffle[s + 1];
		}
		shuffleMax--;
	}

	// --- Phase 7: Generate challenge patterns ---
	generateChallengePatterns();

	// --- Phase 8: Fill grid attributes ---
	int16 v37 = 0; // obstacle entry count
	int16 patternPlacedCount = 0;
	_obstacleEntryCount = 0;

	for (int16 k = 0; k < 12; k++) {
		for (int16 m = 0; m < 12; m++) {
			_gridOccupancy[k][m] = 0;
			_gridExitReservation[k][m] = 0;
			_gridAttr1[k][m] = 0;
			_gridAttr2[k][m] = 0;
			_gridAttr3[k][m] = 0;

			bool hasAttr1 = false;
			bool hasAttr2 = false;
			bool hasAttr3 = false;

			// Process each of 3 pattern grids
			int16 patternIdx = 0; // final pattern index ('a1' in IDA)
			int16 adjustedIdx = 0; // v42 in IDA

			for (int16 n = 0; n < 3; n++) {
				int16 rawVal = _gridPattern[n][k][m];
				if (rawVal == 0)
					continue;

				// Adjust based on grid type
				if (n == 0) {
					// Grid 0: attr type 1 (pattern), indices 1-3
					if (_gridType == 3) {
						adjustedIdx = rawVal;
					} else {
						adjustedIdx = rawVal + 1;
					}
					if (adjustedIdx > 3)
						adjustedIdx = 1;
				} else if (n == 1) {
					// Grid 1: attr type 2 (color), indices 4-7
					if (_gridType == 4) {
						adjustedIdx = rawVal;
					} else {
						adjustedIdx = rawVal + 1;
					}
					if (adjustedIdx > 7)
						adjustedIdx = 4;
				} else { // n == 2
					// Grid 2: attr type 3 (pad shape), indices 8-12
					if (_gridType == 5) {
						adjustedIdx = rawVal;
					} else {
						adjustedIdx = rawVal + 1;
					}
					if (adjustedIdx > 12)
						adjustedIdx = 8;
				}

				patternIdx = rawVal; // keep raw for the mask/usage checks

				// Apply challenge pattern if valid
				if (rawVal != 0 && kRowColValidity[k] != 0 && kRowColValidity[m] != 0 &&
				    _patternMask[rawVal] == 0 && _patternUsageCount[rawVal] < 2) {
					// 75% random chance, or forced on last row if usage is 0
					int16 rndCheck = _vm->_rnd->getRandomNumber(0, 100);
					if (rndCheck > 75 || (k == 11 && _patternUsageCount[rawVal] == 0)) {
						_patternUsageCount[rawVal]++;
						patternIdx = adjustedIdx;
						patternPlacedCount++;
					}
				}
			}

			// Apply pattern attributes if valid index
			if (patternIdx > 0 && patternIdx < 13) {
				LillyPadAttrType pType = _patternType[patternIdx];
				int16 pValue = _patternValue[patternIdx];

				if (pType == kLillyPadAttrPattern) {
					hasAttr1 = true;
					_gridAttr1[k][m] = static_cast<byte>(pValue);
				} else if (pType == kLillyPadAttrShape) {
					hasAttr2 = true;
					_gridAttr2[k][m] = static_cast<byte>(pValue);
				} else if (pType == kLillyPadAttrColor) {
					hasAttr3 = true;
					_gridAttr3[k][m] = static_cast<byte>(pValue);
				}

				// Record obstacle entry points (difficulty 3/4, first row only)
				if ((_difficultyLevel == kPuzzleDiffLevel3 || _difficultyLevel == kPuzzleDiffLevel4) &&
				    v37 < v41 && k == 0) {
					bool isObstaclePattern = false;
					if (_gridType == 3 && patternIdx <= 3)
						isObstaclePattern = true;
					else if (_gridType == 4 && patternIdx >= 4 && patternIdx <= 7)
						isObstaclePattern = true;
					else if (_gridType == 5 && patternIdx >= 8)
						isObstaclePattern = true;

					if (isObstaclePattern) {
						_obstacleEntryCols[v37] = m;
						_obstacleEntryType[v37] = _patternType[patternIdx];
						_obstacleEntryValue[v37] = _patternValue[patternIdx];
						_obstacleEntryExtra[v37] = _patternExtra[patternIdx];
						v37++;
					}
				}
			}

			// Random fill for unset attributes
			if (!hasAttr1)
				_gridAttr1[k][m] = static_cast<byte>(_vm->_rnd->getRandomNumber(0, 2));
			if (!hasAttr2)
				_gridAttr2[k][m] = static_cast<byte>(_vm->_rnd->getRandomNumber(0, 3));
			if (!hasAttr3)
				_gridAttr3[k][m] = static_cast<byte>(_vm->_rnd->getRandomNumber(0, 4));

			// Compute combined attr: attr1 + kCombinedAttrBase[attr3]
			// IDA: unk_4AC688[row][col] = byte_4AC685[row][col] + byte_4A181E[2 * byte_4AC687[row][col]]
			_gridCombinedAttr[k][m] = _gridAttr1[k][m] +
				kCombinedAttrBase[_gridAttr3[k][m]];

			// Compute cell positions from REGS 100
			// IDA: posX = 35 * col + REGS_X[row+1], posY = kColYOffset[col] + REGS_Y[row+1]
			int16 baseX = (k + 1 < static_cast<int16>(_regsXTable.size())) ?
				_regsXTable[k + 1] : (50 + k * 45);
			int16 baseY = (k + 1 < static_cast<int16>(_regsYTable.size())) ?
				_regsYTable[k + 1] : 100;

			int16 cellX = 35 * m + baseX;
			int16 cellY = kColYOffset[m] + baseY;

			_gridCellPos[k][m] = Common::Point(cellX, cellY);
			_gridCellRect[k][m] = Common::Rect(cellX, cellY, cellX + 36, cellY + 30);
		}
	}

	_obstacleEntryCount = v37;

	// IDA: word_4AE370 byte access — obstacle attr type used by all obstacles.
	// Taken from the first obstacle entry's type.
	if (_obstacleEntryCount > 0)
		_obstacleAttrType = _obstacleEntryType[0];
	else
		_obstacleAttrType = kLillyPadAttrNone;

	// --- Phase 9: Initial cell swaps (difficulty > 1) ---
	if (_difficultyLevel >= kPuzzleDiffLevel2) {
		// First swap pair: indices 0,2
		_swapCellACol = kSwapPairCol[0];
		_swapCellARow = kSwapPairRow[0];
		_swapCellBCol = kSwapPairCol[2];
		_swapCellBRow = kSwapPairRow[2];
		swapCellsAndUpdateRunners(_swapCellACol, _swapCellARow, _swapCellBCol, _swapCellBRow);

		// Second swap pair: indices 4,6
		_swapCellACol = kSwapPairCol[4];
		_swapCellARow = kSwapPairRow[4];
		_swapCellBCol = kSwapPairCol[6];
		_swapCellBRow = kSwapPairRow[6];
		swapCellsAndUpdateRunners(_swapCellACol, _swapCellARow, _swapCellBCol, _swapCellBRow);
	}

	// --- Phase 10: Compute swap unlock threshold ---
	// IDA: v36 = patternPlacedCount + 5; threshold = ceil(v36/6)
	int16 v36 = patternPlacedCount + 5;
	_swapThreshold = (v36 + 5) / 6; // equivalent to ceil(v36/6)
}

void ZoombiniPuzzleLilly::createZoombiniRunners() {
	// IDA: word_4AE3C2[] — per-zoombini runners at SCRB 10109+i
	// Original: runner_registerAndAllocate(0, 0, (POINTS *)&unk_4A15A8 + i, 4, i+10109, ...)
	for (int16 i = 0; i < _totalZmbCount && i < kMaxRunners; i++) {
		_zmbRunners[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 10109 + i, 4,
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);

		if (_zmbRunners[i]) {
			// IDA: position from unk_4A15A8 table.
			// Original: non-last-2 runners are VISIBLE at icon positions with
			// DEFER_ANIM (first frame shown, animation deferred). Last 2 runners
			// have wBoolDoRender=0 set explicitly (then walk-in for diff==1).
			_zmbRunners[i]->setPointLoc(kInitialPositions[i]);
			_zmbRunners[i]->deactivateAnimate();  // DEFER_ANIM: first frame only
			if (i >= _totalZmbCount - 2) {
				// IDA: v4->core188.wBoolDoRender = 0 for last 2
				_zmbRunners[i]->deactivateRender();
			}
		}

		// IDA: Exit animation child runner — separate feature for exit SCRB 10129+row.
		// Allocated during init, hidden until handleArriveAtNode activates it.
		// Original: runner_registerAndAllocate(0, 0, &posTable[i], 4, 10109+i, ...)
		// The exit animation feature shares the same SCRB resource base but is inactive.
		_exitAnimFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 10129 + i, 4,
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

		if (_exitAnimFeatures[i]) {
			_exitAnimFeatures[i]->deactivateRender();
		}
	}

	// IDA: For difficulty 1, last 2 runners get exit animation at SCRBs 10089+i
	// This is handled in handleScriptEvent case 3 for difficulty > 1
	if (_difficultyLevel == kPuzzleDiffLevel1) {
		for (int16 i = MAX(0, _totalZmbCount - 2); i < _totalZmbCount; i++) {
			if (_zmbRunners[i]) {
				loadScrbOntoFeature(_zmbRunners[i], 10089 + i);
				_runnerStates[i].callbackMode = kCBLillyDepart;
			}
		}
		// IDA: ui_bDragLockActive = totalCount - (totalCount-1) = 1
		_remainingZmbs = 1;
	}
}

void ZoombiniPuzzleLilly::createInitialObstacleRunners() {
	// IDA: maze_registerObstacleRunners_42651C (0x42651C) — 659 bytes.
	// Registers 12 obstacle runners ("toads") on the left bank at entry positions.
	// Each gets SCRB 10043+j (entry idle), bitmask 0x980002 (clickable).
	// Lane assignment is randomized via shuffle-without-replacement.
	static const int kObstacleCount = 12;

	// IDA: v47[0..11] = {0,1,...,11} — shuffle deck (lane indices)
	int16 deck[kObstacleCount];
	for (int16 i = 0; i < kObstacleCount; i++)
		deck[i] = i;
	int16 deckSize = kObstacleCount - 1; // IDA: v50 = 11

	for (int16 j = 0; j < kObstacleCount; j++) {
		// IDA: Pick random lane from remaining deck
		int16 randIdx = _vm->_rnd->getRandomNumber(0, deckSize);
		int16 lane = deck[randIdx];

		// IDA: v37 = v46[2*lane] (attrType), v38 = v45[2*lane] (attrValue)
		// v46 = kPatternAttrType, v45 = kPatternAttrValue (same tables at 0x4A177A, 0x4A1792)
		// word_4ABFE2[j] = v37, word_4ABFFA[j] = v38 — write-only globals, not needed in ScummVM

		// IDA: Remove picked lane from deck (shift left)
		for (int16 k = randIdx; k < deckSize; k++)
			deck[k] = deck[k + 1];
		--deckSize;

		// IDA: runner_registerAndAllocate(0, 0, v11, 7, j+10043, pre, post, 0x180002)
		// Priority = rand(3,6), SCRB = j+10043
		int16 runnerIdx = _totalZmbCount + j;
		if (runnerIdx >= kMaxRunners)
			break;

		_zmbRunners[runnerIdx] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 10043 + j, 7,
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);

		if (!_zmbRunners[runnerIdx])
			continue;

		// IDA: scrb_loadOnRunner(1, j+10043, runner) — load entry idle SCRB
		loadScrbOntoFeature(_zmbRunners[runnerIdx], 10043 + j);

		// IDA: runner[8] = 0x980002 — make clickable on left bank
		// In ScummVM, clickability is handled by hasClickRect() + isRenderActivated() checks.
		// The SCRB 10043+j provides the click rect via hotspot groups.

		_obstacleRunners[j] = runnerIdx;

		// Initialize runner state for this obstacle
		ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
		rs.clear();
		rs.isObstacle = true;
		rs.entryPointIdx = lane; // IDA: v39 = lane (entry position index)

		// IDA: runner->onPreRenderShapeFunc = maze_updateMultiLegPath_429E72
		// Adds lane offset to the pattern overlay shape_id before rendering.
		_zmbRunners[runnerIdx]->setPreRenderShapeFunc(
			reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzleLilly::obstaclePreRenderShape));

		// IDA: scrb_useFeatureGroup(0, 2, 10000) — obstacle runners use tBMP 10000
		// with REGS 10000/10001 for per-shape position offsets.
		auto itRegs = _regsMap.find(10000);
		if (itRegs != _regsMap.end())
			_zmbRunners[runnerIdx]->setShapeRegs(itRegs->_value);
	}

	_obstacleRunnerCount = kObstacleCount;

}

void ZoombiniPuzzleLilly::obstaclePreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	// IDA: maze_updateMultiLegPath (0x429E72)
	// Adds lane index (runner[272]) to hsArr[1].shapeid (pattern overlay).
	// This selects one of 12 distinct toad back patterns from tBMP 10000
	// sub-images 78-89 (shape_id 79-90).
	for (int16 j = 0; j < _obstacleRunnerCount; j++) {
		int16 ri = _obstacleRunners[j];
		if (ri >= 0 && ri < kMaxRunners && _zmbRunners[ri] == feature) {
			int16 lane = _runnerStates[ri].entryPointIdx;
			if (hotspots.size() >= 2 && hotspots[1]._shapeIdx > 0) {
				hotspots[1]._shapeIdx += lane;
			}
			return;
		}
	}
}

// =================================================================
// onEveryFrame — main per-frame queue processing pipeline
// IDA: lilly_mainFrameUpdate (0x423A0D)
// =================================================================

void ZoombiniPuzzleLilly::onEveryFrame() {
	if (!_bPuzzleActive)
		return;

	debugC(2, kZmbDebugAnimation, "Lilly: frame tick enterQ=%d exitQ=%d rotateQ=%d crossQ=%d departQ=%d arriveQ=%d moveQ=%d",
		_enterQueueSize, _exitQueueSize, _rotateQueueSize, _crossQueueSize, _departQueueSize, _arriveQueueSize, _moveQueueSize);

	// Process animation queues in strict order (matching original)
	processEnterQueue();
	processExitQueue();
	processCompletedExitRunner();
	processRotateQueue();
	processCrossQueue();
	processCompletedCrossRunner();
	processDepartQueue();
	processArriveQueue();

	// IDA: Cell swap animation when state == 6 (both cells selected)
	if (_cellSelectState == 6) {
		processCellSwapAnimation();
	}

	// Move phase alternates 0/1 each frame
	processMovePhase();
	_movePhaseFlag = 1 - _movePhaseFlag;

	// Clean up freed runners
	processFreedRunners();

	// Per-frame path interpolation for all active movers
	for (int16 i = 0; i < _moveQueueSize; i++) {
		advanceRunnerStep(_moveQueue[i]);
	}

	// Update Go button state
	setGoButtonsEnabled(_bAdvanceEnabled);
}

// --- Queue processing ---

void ZoombiniPuzzleLilly::processEnterQueue() {
	// IDA: 0x423B40. Serialized — only one enter runner active at a time.
	// Pops from back (LIFO). Loads SCRB 10057 (enter animation).
	// Callbacks: runner[5]=lilly_zmbExitPathOffsetStep, runner[4]=maze_runnerExitGridStep
	while (_enterQueueSize > 0 && _activeEnterRunner < 0) {
		int16 runnerIdx = _enterQueue[--_enterQueueSize];
		_activeEnterRunner = runnerIdx;
		if (runnerIdx >= 0 && runnerIdx < kMaxRunners && _zmbRunners[runnerIdx]) {
			_runnerStates[runnerIdx].callbackMode = kCBLillyEnter;
			loadScrbOntoFeature(_zmbRunners[runnerIdx], 10057);
		}
	}
}

void ZoombiniPuzzleLilly::processExitQueue() {
	// IDA: 0x423B9B. Serialized — only one exit runner active at a time.
	// Pops from back (LIFO). Loads SCRB 10058 (exit animation).
	// Callbacks: runner[5]=maze_updateRunnerPathStep, runner[4]=maze_runnerSnapAndRegisterCell
	// Also sets runner flags (runner[8]=0x04980002) and links to exit parent runner.
	while (_exitQueueSize > 0 && _activeExitRunner < 0) {
		int16 runnerIdx = _exitQueue[--_exitQueueSize];
		_activeExitRunner = runnerIdx;
		if (runnerIdx >= 0 && runnerIdx < kMaxRunners && _zmbRunners[runnerIdx]) {
			_runnerStates[runnerIdx].callbackMode = kCBLillyExit;
			loadScrbOntoFeature(_zmbRunners[runnerIdx], 10058);
			// IDA: runner[8] = 0x04980002 (flags)
			// IDA also re-links to a no-op parent runner for list ordering.
			// ScummVM rebuilds render order every frame, so no persistent link is required here.
		}
	}
}

void ZoombiniPuzzleLilly::processCompletedExitRunner() {
	// IDA: 0x423BFA. When exit animation completes, snap runner back to its
	// entry position and reload the idle SCRB 10043+entryPointIdx.
	// Clears both completedExitRunner and activeExitRunner.
	if (_completedExitRunner < 0)
		return;

	int16 runnerIdx = _completedExitRunner;
	_completedExitRunner = -1;
	_activeExitRunner = -1;

	if (runnerIdx >= 0 && runnerIdx < kMaxRunners && _zmbRunners[runnerIdx]) {
		ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
		// IDA: *(int*)(runner+214) = dword_4A1650[entryPointIdx] — snap position
		if (rs.entryPointIdx >= 0 && rs.entryPointIdx < 12) {
			_zmbRunners[runnerIdx]->setPointLoc(kEntryPositions[rs.entryPointIdx]);
		}
		// IDA: scrb_loadOnRunner(1, entryPointIdx + 10043, runner)
		loadScrbOntoFeature(_zmbRunners[runnerIdx], 10043 + rs.entryPointIdx);
	}
}

void ZoombiniPuzzleLilly::processRotateQueue() {
	// IDA: lilly_funcMain at 0x423C62 — Rotate animation queue.
	// Processes rotation ONLY when activeEnterRunner is set.
	// SCRB selection depends on runner's attrType:
	//   pad-shape attrType → odd SCRBs: 10061/10063/10065
	//   other attrTypes   → even SCRBs: 10060/10062/10064
	if (_rotateQueueSize <= 0 || _activeEnterRunner < 0)
		return;

	while (_rotateQueueSize > 0 && _activeEnterRunner >= 0) {
		int16 runnerIdx = _rotateQueue[--_rotateQueueSize];
		if (runnerIdx < 0 || runnerIdx >= kMaxRunners || !_zmbRunners[runnerIdx])
			continue;

		ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
		int16 randVal = _vm->_rnd->getRandomNumber(0, 2);

		rs.callbackMode = kCBLillyRotate;
		if (rs.attrType == kLillyPadAttrShape) {
			loadScrbOntoFeature(_zmbRunners[runnerIdx], 2 * randVal + 10061);
		} else {
			loadScrbOntoFeature(_zmbRunners[runnerIdx], 2 * randVal + 10060);
		}
	}
}

void ZoombiniPuzzleLilly::processCrossQueue() {
	// IDA: 0x423D02. Serialized — only one cross runner active at a time.
	// Pops from back (LIFO). Loads SCRB 10059 (cross animation).
	// Callbacks: runner[5]=maze_zmbPathOffsetStep, runner[4]=maze_runnerSnapPosAndClearCell
	while (_crossQueueSize > 0 && _activeCrossRunner < 0) {
		int16 runnerIdx = _crossQueue[--_crossQueueSize];
		_activeCrossRunner = runnerIdx;
		if (runnerIdx >= 0 && runnerIdx < kMaxRunners && _zmbRunners[runnerIdx]) {
			_runnerStates[runnerIdx].callbackMode = kCBLillyCross;
			loadScrbOntoFeature(_zmbRunners[runnerIdx], 10059);
		}
	}
}

void ZoombiniPuzzleLilly::processCompletedCrossRunner() {
	// IDA: 0x423D51. Free the cross runner (zoombini has successfully
	// crossed the pond). Clears both activeCrossRunner and completedCrossRunner.
	if (_completedCrossRunner < 0)
		return;

	int16 runnerIdx = _completedCrossRunner;
	_completedCrossRunner = -1;
	_activeCrossRunner = -1;

	// IDA: runner_freeByIndex(completedCrossRunner)
	if (runnerIdx >= 0 && runnerIdx < kMaxRunners && _zmbRunners[runnerIdx]) {
		_zmbRunners[runnerIdx]->deactivateRender();
		_zmbRunners[runnerIdx]->deactivateAnimate();
	}
}

void ZoombiniPuzzleLilly::processDepartQueue() {
	// IDA: 0x423D91. Processes all depart queue entries (not serialized).
	// Pops from back (LIFO). Loads SCRB 10141+dirByte (departure animation).
	// Callbacks: runner[5]=maze_advanceRunnerStep, runner[4]=maze_runnerExitCallback
	// Also sets flag: runner[8] |= 0x04000000
	while (_departQueueSize > 0) {
		int16 runnerIdx = _departQueue[--_departQueueSize];
		if (runnerIdx >= 0 && runnerIdx < kMaxRunners && _zmbRunners[runnerIdx]) {
			ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
			rs.callbackMode = kCBLillyDepart;
			loadScrbOntoFeature(_zmbRunners[runnerIdx], 10141 + rs.dirByte);
			// IDA 0x423D91: v14[8] |= 0x04000000 (FLAG_04000000_OVERLAY) — places
			// departing runners in the OVERLAY z-layer for proper compositing.
			_zmbRunners[runnerIdx]->addFlag(ZmbFeature::FLAG_04000000_OVERLAY);
		}
	}
}

void ZoombiniPuzzleLilly::processArriveQueue() {
	// IDA: 0x423DEA. Processes all arrive queue entries (not serialized).
	// Pops from back (LIFO). Loads SCRB 10019+dirByte (arrival animation).
	// Callbacks in IDA: runner[5]=maze_advanceRunnerStep, runner[4]=maze_handlePathBuildEvent
	// maze_handlePathBuildEvent event 20 re-enqueues runner into readyQueue.
	while (_arriveQueueSize > 0) {
		int16 runnerIdx = _arriveQueue[--_arriveQueueSize];
		if (runnerIdx >= 0 && runnerIdx < kMaxRunners && _zmbRunners[runnerIdx]) {
			ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
			rs.callbackMode = kCBLillyArrive;
			loadScrbOntoFeature(_zmbRunners[runnerIdx], 10019 + rs.dirByte);
		}
	}
}

void ZoombiniPuzzleLilly::processMovePhase() {
	// IDA: lilly_mainFrameUpdate at 0x423E32.
	// movePhaseFlag alternates each frame between two branches:
	//   1 → 0: Process readyQueue (zoombini path steps via advancePathOnGrid)
	//   0 → 1: Process pathInit, obstacle spawning, moveQueue (obstacle steps)
	// Skipped entirely when cell selection is active.

	if (_bCellSelectActive)
		return;

	if (_movePhaseFlag == 1) {
		// === BRANCH A: Zoombini path advancement ===
		_movePhaseFlag = 0;

		// IDA: Drain pendingReady (word_4AC3FE/word_4AC426) into readyQueue (word_4AC3D4)
		while (_pendingReadyCount > 0) {
			if (_readyQueueSize < kMaxMoveQueueSize)
				_readyQueue[_readyQueueSize++] = _pendingReadyQueue[--_pendingReadyCount];
			else
				--_pendingReadyCount; // Overflow protection (drop oldest)
		}

		// IDA: Process readyQueue — each runner advances one path step
		while (_readyQueueSize > 0) {
			int16 runnerIdx = _readyQueue[--_readyQueueSize];
			if (runnerIdx < 0 || runnerIdx >= kMaxRunners || !_zmbRunners[runnerIdx])
				continue;

			ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

			// IDA: Timer check — game_getFrameCounterOrDelta() < runner[9]
			if (getCurrentFrameCounter() < rs.moveTimer) {
				// Not yet ready — put back in pendingReady for next frame
				_pendingReadyQueue[_pendingReadyCount++] = _readyQueue[_readyQueueSize];
				continue;
			}

			// IDA re-links to column parents for linked-list ordering only.
			// ScummVM's per-frame Z-sort makes that ordering implicit.

			// Advance one path step
			uint16 nextScrb = advancePathOnGrid(runnerIdx);

			if (nextScrb == 10031) {
				// IDA: Exit reached — load col-offset exit SCRB
				// IDA: scrb_loadOnRunner(1, core188[195] + 10031, runner) — +195 = rs.col
				rs.callbackMode = kCBLillyReadyExit;
				loadScrbOntoFeature(_zmbRunners[runnerIdx],
					static_cast<uint16>(rs.col + 10031));
				// IDA: runner[5]=maze_updateMultiLegPath, runner[4]=maze_runnerArriveAtNode
			} else if (nextScrb != 0) {
				// IDA: Normal step — load direction SCRB
				rs.callbackMode = kCBLillyReadyMove;
				loadScrbOntoFeature(_zmbRunners[runnerIdx], nextScrb);
				// IDA: runner[5]=maze_updateMultiLegPath, runner[4]=maze_zmbMoveFinalizeStep
			} else {
				// No valid move — put back for retry next frame
				_pendingReadyQueue[_pendingReadyCount++] = _readyQueue[_readyQueueSize];
			}
		}
		return; // IDA: goto LABEL_110 (cleanup — handled at end of onEveryFrame)
	}

	// === BRANCH B: Obstacle spawning + obstacle movement ===
	_movePhaseFlag = 1;

	// IDA: Skip obstacles for easy difficulty or during cell selection
	if (static_cast<int16>(_difficultyLevel) <= 2 || _bCellSelectActive)
		return; // IDA: goto LABEL_110

	// IDA: Process path init queue (re-compute BFS paths after grid changes)
	while (_pathInitQueueSize > 0) {
		int16 runnerIdx = _pathInitQueue[--_pathInitQueueSize];
		if (runnerIdx >= 0 && runnerIdx < kMaxRunners && _zmbRunners[runnerIdx])
			initRunnerBFSPath(runnerIdx);
	}

	// IDA: Timer-based obstacle spawning every 480 frames (max 20 active)
	if (getCurrentFrameCounter() > _nextObstacleTimer && _activeObstacleCount < 20) {
		spawnObstacleRunner();
		// NOTE: spawnObstacleRunner sets _nextObstacleTimer internally
	}

	// IDA: Drain pendingMove (word_4AC2B2) into moveQueue (word_4AC06E)
	while (_pendingMoveCount > 0) {
		if (_moveQueueSize < kMaxMoveQueueSize)
			_moveQueue[_moveQueueSize++] = _pendingMoveQueue[--_pendingMoveCount];
		else
			--_pendingMoveCount;
	}

	// IDA: Process moveQueue — advance each obstacle/runner one step
	while (_moveQueueSize > 0) {
		int16 runnerIdx = _moveQueue[--_moveQueueSize];
		if (runnerIdx < 0 || runnerIdx >= kMaxRunners || !_zmbRunners[runnerIdx])
			continue;

		ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

		// IDA: Timer check
		if (getCurrentFrameCounter() < rs.moveTimer) {
			_pendingMoveQueue[_pendingMoveCount++] = _moveQueue[_moveQueueSize];
			continue;
		}

		// IDA: Advance based on runner mode
		// runner.word49 ? maze_zmbAdvanceForwardStep : fleens_advancePathStepAlt
		uint16 v31;
		if (rs.advanceMode != 0) {
			v31 = advanceObstacleForwardStep(runnerIdx);
		} else {
			v31 = advanceObstaclePathStepAlt(runnerIdx);
		}

		if (v31 == 10069) {
			// IDA: Obstacle exit animation
			// runner_linkRelativeToParent(word_4AE3AA[runner.col], 0, runner.idx)
			rs.callbackMode = kCBLillyMoveObstacle;
			loadScrbOntoFeature(_zmbRunners[runnerIdx], 10069);
			rs.direction = 0; // IDA: *(byte+242) = 0
			// IDA: runner[5]=maze_updateRunnerPath, runner[4]=maze_runnerArriveOrDepartCallback
		} else if (v31 == 0) {
			// No valid move — put back for retry
			_pendingMoveQueue[_pendingMoveCount++] = _moveQueue[_moveQueueSize];
		} else {
			// IDA re-links to row/column helper runners before the step SCRB.
			// ScummVM does not preserve runner-list ordering between frames, so the
			// direction-specific parent link is not modeled directly.

			rs.callbackMode = kCBLillyMoveStep;
			loadScrbOntoFeature(_zmbRunners[runnerIdx], v31);
			// IDA: runner[5]=maze_zmbMoveStep, runner[4]=maze_runnerArriveOrDepartCallback
		}
	}
}

void ZoombiniPuzzleLilly::processFreedRunners() {
	// IDA: LABEL_110, 0x424586. Clean up freed runners at end of mainFrameUpdate.
	// Uses runner_freeByIndex (deactivate and remove from runner list).
	while (_freedRunnerCount > 0) {
		int16 runnerIdx = _freedRunners[--_freedRunnerCount];
		if (runnerIdx >= 0 && runnerIdx < kMaxRunners && _zmbRunners[runnerIdx]) {
			_zmbRunners[runnerIdx]->deactivateRender();
			_zmbRunners[runnerIdx]->deactivateAnimate();
		}
	}

	// IDA: Also free the exitedRunnerIdx if set
	if (_exitedRunnerIdx >= 0) {
		int16 runnerIdx = _exitedRunnerIdx;
		_exitedRunnerIdx = -1;
		if (runnerIdx >= 0 && runnerIdx < kMaxRunners && _zmbRunners[runnerIdx]) {
			_zmbRunners[runnerIdx]->deactivateRender();
			_zmbRunners[runnerIdx]->deactivateAnimate();
		}
	}
}

// =================================================================
// Per-frame path interpolation
// IDA: maze_advanceRunnerStep (0x425C85)
// =================================================================

void ZoombiniPuzzleLilly::advanceRunnerStep(int16 runnerIdx) {
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
	if (rs.pathStepIdx == 0)
		return;

	// Advance step index by direction
	rs.pathStepIdx += rs.pathStepDir;

	// Apply position deltas from REGS 200 lookup tables
	if (rs.pathStepIdx >= 0 && rs.pathStepIdx < (int16)_regsDeltaX.size()) {
		ZmbFeature *runner = _zmbRunners[runnerIdx];
		if (runner) {
			Common::Point pos = runner->getPointLoc();
			pos.x -= _regsDeltaX[rs.pathStepIdx];
			pos.y -= _regsDeltaY[rs.pathStepIdx];
			runner->setPointLoc(pos);
		}
	}
}

// =================================================================
// Custom render callbacks
// IDA: maze_renderAllGridSprites (0x426BFB), maze_renderCursorIndicator (0x426DF9)
// =================================================================

ZmbRenderResult ZoombiniPuzzleLilly::renderGridSprites(ZmbFeature *feature) {
	// IDA: maze_renderAllGridSprites_426BFB — renders the 12x12 lily pad lattice.
	// Two layers per cell:
	//   Layer 1: attr2-based shape — shapeIdx = attr2 + 1 (1-21), REGS200[attr2 + 1]
	//   Layer 2: combinedAttr shape — shapeIdx = combinedAttr (1-21), REGS200[combinedAttr]
	// Sprites are from tBMP 13000, offsets from REGS 200.

	ZmbResource shapeRes(ZmbArchiveKind::kPage, 13000);

	for (int16 row = 0; row < 12; row++) {
		for (int16 col = 0; col < 12; col++) {
			int16 posX = _gridCellPos[row][col].x;
			int16 posY = _gridCellPos[row][col].y;

			// Layer 1: attr2 shape (lily pad base color)
			int16 attr2ShapeIdx = _gridAttr2[row][col] + 1;
			if (attr2ShapeIdx > 0 && attr2ShapeIdx < 22) {
				int16 regsX = (attr2ShapeIdx < static_cast<int16>(_regsDeltaX.size())) ?
					_regsDeltaX[attr2ShapeIdx] : 0;
				int16 regsY = (attr2ShapeIdx < static_cast<int16>(_regsDeltaY.size())) ?
					_regsDeltaY[attr2ShapeIdx] : 0;
				_vm->_gfx->drawShape(ZoombiniGraphics::kShapeScreen, shapeRes,
					static_cast<uint16>(attr2ShapeIdx),
					Common::Point(posX - regsX, posY - regsY));
			}

			// Layer 2: combinedAttr shape (combined attribute overlay)
			int16 combinedShapeIdx = _gridCombinedAttr[row][col];
			if (combinedShapeIdx > 0 && combinedShapeIdx < 22) {
				int16 regsX = (combinedShapeIdx < static_cast<int16>(_regsDeltaX.size())) ?
					_regsDeltaX[combinedShapeIdx] : 0;
				int16 regsY = (combinedShapeIdx < static_cast<int16>(_regsDeltaY.size())) ?
					_regsDeltaY[combinedShapeIdx] : 0;
				_vm->_gfx->drawShape(ZoombiniGraphics::kShapeScreen, shapeRes,
					static_cast<uint16>(combinedShapeIdx),
					Common::Point(posX - regsX, posY - regsY));
			}
		}
	}

	return ZmbRenderResult::kRendered;
}

ZmbRenderResult ZoombiniPuzzleLilly::renderCursorIndicator(ZmbFeature *feature) {
	// IDA: maze_renderCursorIndicator_426DF9 — renders highlighting on the
	// selected grid cell with a 4-frame blink cycle overlay.
	// Shape index = kCursorHighlightBase[attr2] + kCursorBlinkOffset[blinkFrame]
	// Two layers: cursor attr2 overlay + cursor combinedAttr overlay.

	if (!feature->isRenderActivated())
		return ZmbRenderResult::kSkipped;

	if (_highlightRow < 0 || _highlightRow >= 12 ||
	    _highlightCol < 0 || _highlightCol >= 12)
		return ZmbRenderResult::kSkipped;

	// IDA: word_4A17C4 — cursor highlight base shape per attr2 value (9 entries)
	static const int16 kCursorHighlightBase[9] = {20, 22, 24, 26, 0, 28, 30, 32, 34};
	// IDA: word_4A17D6 — blink cycle offset (4 frames: 0,0,1,1)
	static const int16 kCursorBlinkOffset[4] = {0, 0, 1, 1};

	ZmbResource shapeRes(ZmbArchiveKind::kPage, 13000);
	int16 posX = _gridCellPos[_highlightRow][_highlightCol].x;
	int16 posY = _gridCellPos[_highlightRow][_highlightCol].y;

	// Layer 1: attr2-based cursor highlight
	byte attr2Val = _gridAttr2[_highlightRow][_highlightCol];
	if (attr2Val < 9) {
		int16 shapeIdx = kCursorHighlightBase[attr2Val] + kCursorBlinkOffset[_cursorBlinkFrame];
		if (shapeIdx > 0 && shapeIdx < 36) {
			int16 regsX = (attr2Val + 1 < static_cast<int16>(_regsDeltaX.size())) ?
				_regsDeltaX[attr2Val + 1] : 0;
			int16 regsY = (attr2Val + 1 < static_cast<int16>(_regsDeltaY.size())) ?
				_regsDeltaY[attr2Val + 1] : 0;
			_vm->_gfx->drawShape(ZoombiniGraphics::kShapeScreen, shapeRes,
				static_cast<uint16>(shapeIdx),
				Common::Point(posX - regsX, posY - regsY));
		}
	}

	// Layer 2: combinedAttr cursor highlight (same shape index, no blink offset)
	int16 combinedVal = _gridCombinedAttr[_highlightRow][_highlightCol];
	if (combinedVal > 0 && combinedVal < 36) {
		int16 regsX = (combinedVal < static_cast<int16>(_regsDeltaX.size())) ?
			_regsDeltaX[combinedVal] : 0;
		int16 regsY = (combinedVal < static_cast<int16>(_regsDeltaY.size())) ?
			_regsDeltaY[combinedVal] : 0;
		_vm->_gfx->drawShape(ZoombiniGraphics::kShapeScreen, shapeRes,
			static_cast<uint16>(combinedVal),
			Common::Point(posX - regsX, posY - regsY));
	}

	// Advance blink frame on interval (5 ticks per the feature's frameInterval)
	uint32 curFrame = getCurrentFrameCounter();
	if (curFrame >= _cursorBlinkTimer) {
		_cursorBlinkTimer = curFrame + 5;
		_cursorBlinkFrame = (_cursorBlinkFrame + 1) % 4;
	}

	return ZmbRenderResult::kRendered;
}

// =================================================================
// Pathfinding
// =================================================================

// IDA fleens_advancePathStep_425F3D writes the new visit-count to four per-direction
// scratch grids at offsets +216 (LEFT), +240 (RIGHT), +244 (DOWN), +268 (UP) on the
// SOURCE cell, plus the main BFS grid at +242 on the TARGET cell. The 4 scratch
// grids are now mirrored as visitGridLeft/Right/Down/Up in ZmbLillyRunnerState so
// later readers (collision/anti-loop checks) have per-direction history.
uint16 ZoombiniPuzzleLilly::advancePathOnGrid(int16 runnerIdx) {
	// IDA: fleens_advancePathStep (0x425F3D) — 1168 bytes.
	// Extends active path by one step. Checks 4 adjacent cells starting from
	// current direction, selects cell with lowest visit count that matches
	// attribute constraint. Updates visit grid and occupancy.
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return 0;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	// Anti-loop: if current cell visit count >= 10000, reset all
	if (rs.visitGrid[rs.row][rs.col] >= 10000) {
		for (int16 i = 0; i < 12; i++) {
			for (int16 j = 0; j < 12; j++) {
				rs.visitGrid[i][j] = 0;
			}
		}
		rs.visitGrid[rs.row][rs.col] = 1;
	}

	int16 curVisit = rs.visitGrid[rs.row][rs.col];
	if (curVisit == 0) {
		curVisit = 1;
		rs.visitGrid[rs.row][rs.col] = 1;
	}
	int16 originalVisit = curVisit;

	// Try 4 directions starting from current direction
	byte checkDir = rs.direction;
	byte dirCount = 0;
	int16 bestDir = 5; // 5 = no valid direction found
	bool exitFound = false;
	int16 bestVisit = curVisit;
	int16 bestCol = 0, bestRow = 0;

	while (dirCount < 4 && !exitFound) {
		int16 v5 = 1; // valid move flag
		int16 newCol = rs.col;
		int16 newRow = rs.row;

		switch (checkDir) {
		case 0: // UP: row - 1
			newRow--;
			if (newRow < 0) {
				newRow = 0;
				v5 = 0;
			}
			break;
		case 1: // RIGHT: col + 1
			newCol++;
			if (newCol > 11) {
				newCol = 11;
				v5 = 0;
				exitFound = true;
			}
			break;
		case 2: // DOWN: row + 1
			newRow++;
			if (newRow > 11) {
				newRow = 11;
				v5 = 0;
			}
			break;
		case 3: // LEFT: col - 1
			newCol--;
			if (newCol < 0) {
				newCol = 0;
				v5 = 0;
			}
			break;
		}

		if (v5) {
			// Check occupancy
			if (_gridOccupancy[newRow][newCol] != 0) {
				v5 = 0;
			} else {
				// Check attribute constraint
				if (rs.attrType == kLillyPadAttrPattern) {
					if (_gridAttr1[newRow][newCol] != rs.attrValue)
						v5 = 0;
				} else if (rs.attrType == kLillyPadAttrColor) {
					if (_gridAttr2[newRow][newCol] != rs.attrValue)
						v5 = 0;
				} else if (rs.attrType == kLillyPadAttrShape) {
					if (_gridAttr3[newRow][newCol] != rs.attrValue)
						v5 = 0;
				}
			}

			// Pick cell with lowest visit count (below current)
			if (v5 && rs.visitGrid[newRow][newCol] < bestVisit) {
				bestDir = checkDir;
				bestVisit = rs.visitGrid[newRow][newCol];
				bestCol = newCol;
				bestRow = newRow;
			}
		}

		checkDir++;
		if (checkDir > 3)
			checkDir = 0;
		dirCount++;
	}

	// Handle exit case: moving right past col 11
	if (exitFound) {
		// IDA: Check byte_4AC691 on the current cell. This reserves the
		// enter/rotate/cross handoff so only one runner exits from a cell at a time.
		if (_gridExitReservation[rs.row][rs.col] != 0) {
			bestDir = 5; // no valid move
		} else {
			_gridExitReservation[rs.row][rs.col] = 1;
			bestDir = 4; // exit
		}
	}

	// IDA fleens_advancePathStep_425F3D writes to per-direction scratch grids
	// at the SOURCE cell (not target). Each direction has a distinct grid:
	//   case 0 (LEFT-ish in IDA, here UP): visitGridLeft  at SOURCE [row][col]
	//   case 1 (DOWN-ish here RIGHT):      visitGridDown  at SOURCE [row][col]
	//   case 2 (UP-ish here DOWN):         visitGridUp    at SOURCE [row][col]
	//   case 3 (RIGHT-ish here LEFT):      visitGridRight at SOURCE [row][col]
	// Plus the main `visitGrid` at TARGET cell — kept for legacy BFS readers.
	const int16 srcRow = rs.row;
	const int16 srcCol = rs.col;
	switch (bestDir) {
	case 0: // UP — IDA case 0 writes to runner+216 (LEFT scratch)
		rs.direction = 0;
		rs.scrbOrDirKey = kDirScrbUp[rs.direction];
		rs.visitGridLeft[srcRow][srcCol] = originalVisit + 1;
		rs.visitGrid[bestRow][bestCol] = originalVisit + 1;
		_gridOccupancy[bestRow][bestCol] = 1;
		return static_cast<uint16>(rs.scrbOrDirKey);

	case 1: // RIGHT — IDA case 1 writes to runner+244 (DOWN scratch)
		rs.direction = 1;
		rs.scrbOrDirKey = kDirScrbRight[rs.direction];
		rs.visitGridDown[srcRow][srcCol] = originalVisit + 1;
		rs.visitGrid[bestRow][bestCol] = originalVisit + 1;
		_gridOccupancy[bestRow][bestCol] = 1;
		return static_cast<uint16>(rs.scrbOrDirKey);

	case 2: // DOWN — IDA case 2 writes to runner+268 (UP scratch)
		rs.direction = 2;
		rs.scrbOrDirKey = kDirScrbDown[rs.direction];
		rs.visitGridUp[srcRow][srcCol] = originalVisit + 1;
		rs.visitGrid[bestRow][bestCol] = originalVisit + 1;
		_gridOccupancy[bestRow][bestCol] = 1;
		return static_cast<uint16>(rs.scrbOrDirKey);

	case 3: // LEFT — IDA case 3 writes to runner+240 (RIGHT scratch)
		rs.direction = 3;
		rs.scrbOrDirKey = kDirScrbLeft[rs.direction];
		rs.visitGridRight[srcRow][srcCol] = originalVisit + 1;
		rs.visitGrid[bestRow][bestCol] = originalVisit + 1;
		_gridOccupancy[bestRow][bestCol] = 1;
		return static_cast<uint16>(rs.scrbOrDirKey);

	case 4: // EXIT
		rs.direction = 1;
		rs.scrbOrDirKey = 10031;
		return 10031;

	default: // No valid direction
		return 0;
	}
}

void ZoombiniPuzzleLilly::computeShortestPath(byte targetRow, int16 runnerIdx) {
	// IDA: maze_computeShortestPathGrid (0x42990A) — Dijkstra-style greedy fill.
	// From current position, try 4 directions (rotating from last best dir).
	// Validates against attribute grid. Tracks best cost in visit grid.
	// 200 iteration limit.
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	// Determine starting position based on directionMode
	int16 maxProgress;
	if (rs.directionMode != 0)
		maxProgress = rs.row;
	else
		maxProgress = rs.col;

	int16 curRow, curCol;
	if (rs.targetRow == 11) {
		curRow = rs.col;
		curCol = rs.row;
	} else {
		curRow = rs.frontierCol;
		curCol = rs.frontierRow;
	}

	int16 bestRow = curRow;
	int16 bestCol = curCol;

	// Seed the starting cell with current step cost
	rs.visitGrid[curCol][curRow] = rs.stepCount;
	int16 bestCost = rs.stepCount;
	int16 curCost = bestCost;
	byte curDir = rs.direction;

	for (int16 iter = 0; iter < 200; iter++) {
		if (maxProgress >= targetRow)
			break;

		byte dirIdx = curDir;
		for (uint16 d = 0; d < 4 && maxProgress < targetRow; d++) {
			int16 v5 = 1;
			int16 testRow = curRow;
			int16 testCol = curCol;

			switch (dirIdx) {
			case 0: // col-1
				testCol--;
				if (testCol < 0) {
					testCol = curCol;
					v5 = 0;
				}
				break;
			case 1: // row+1
				testRow++;
				if (testRow > 11) {
					testRow--;
					v5 = 0;
					if (rs.directionMode == 0)
						maxProgress = testRow;
				}
				break;
			case 2: // col+1
				testCol++;
				if (testCol > 11) {
					testCol = curCol;
					v5 = 0;
					if (rs.directionMode == 1)
						maxProgress = curCol;
				}
				break;
			case 3: // row-1
				testRow--;
				if (testRow < 0) {
					testRow++;
					v5 = 0;
				}
				break;
			}

			// Attribute validation
			if (v5) {
				if (rs.attrType == kLillyPadAttrPattern) {
					if (_gridAttr1[testCol][testRow] != rs.attrValue)
						v5 = 0;
				} else if (rs.attrType == kLillyPadAttrColor) {
					if (_gridAttr2[testCol][testRow] != rs.attrValue)
						v5 = 0;
				} else if (rs.attrType == kLillyPadAttrShape) {
					if (_gridAttr3[testCol][testRow] != rs.attrValue)
						v5 = 0;
				}
			} else {
				v5 = 0;
			}

			// Pick cell with cost below best
			if (v5 && rs.visitGrid[testCol][testRow] < bestCost) {
				curDir = dirIdx;
				bestCost = rs.visitGrid[testCol][testRow];
				bestRow = testRow;
				bestCol = testCol;

				// Track progress
				if (rs.directionMode != 0) {
					if (testCol > maxProgress)
						maxProgress = testCol;
					if (testCol < rs.frontierRow) {
						rs.frontierCol = testRow;
						rs.frontierRow = testCol;
					}
				} else {
					if (testRow > maxProgress)
						maxProgress = testRow;
					if (testRow < rs.frontierCol) {
						rs.frontierCol = testRow;
						rs.frontierRow = testCol;
					}
				}
			}

			dirIdx++;
			if (dirIdx > 3)
				dirIdx = 0;
		}

		// Update best cell cost
		rs.visitGrid[bestCol][bestRow] = curCost + 1;
		curRow = bestRow;
		curCol = bestCol;
		curCost++;
		bestCost = curCost;
	}

	// Store final state
	rs.stepCount = rs.visitGrid[curCol][curRow];
	rs.targetRow = static_cast<byte>(maxProgress);
}

void ZoombiniPuzzleLilly::traversePathBFS(byte targetRow, int16 runnerIdx) {
	// IDA: fleens_traversePathBFS (0x429C2D) — Greedy BFS traversal.
	// Starts from current position, zeros visited cells.
	// At each step checks 4 neighbors, moves to neighbor with highest cost value.
	// Tracks max progress toward targetRow. Returns when reached or 200 iterations.
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	int16 curRow, curCol;
	if (rs.directionMode != 0) {
		// Horizontal mode: start from (col, row) and clear
		rs.visitGrid[rs.row][rs.col] = 0;
		curRow = rs.col;
		curCol = rs.row;
	} else {
		// Vertical mode: start from frontier and clear
		rs.visitGrid[rs.frontierRow][rs.frontierCol] = 0;
		curRow = rs.frontierCol;
		curCol = rs.frontierRow;
	}

	int16 bestRow = curRow;
	int16 bestCol = curCol;

	int16 maxProgress;
	if (rs.directionMode != 0)
		maxProgress = rs.row;
	else
		maxProgress = rs.col;

	int16 bestCost = 0;
	int16 iterCount = 0;

	while (true) {
		if (maxProgress >= targetRow || iterCount >= 200)
			break;

		for (byte i = 0; i < 4; i++) {
			int16 testRow = curRow;
			int16 testCol = curCol;
			int16 neighborCost = 0;

			switch (i) {
			case 0: // col-1
				testCol--;
				if (testCol >= 0)
					neighborCost = rs.visitGrid[testCol][testRow];
				else {
					testCol++;
					neighborCost = 0;
				}
				break;
			case 1: // row+1
				testRow++;
				if (testRow <= 11)
					neighborCost = rs.visitGrid[testCol][testRow];
				else {
					testRow = curRow;
					neighborCost = 0;
				}
				break;
			case 2: // col+1
				testCol++;
				if (testCol <= 11)
					neighborCost = rs.visitGrid[testCol][testRow];
				else {
					testCol--;
					neighborCost = 0;
				}
				break;
			case 3: // row-1
				testRow--;
				if (testRow >= 0)
					neighborCost = rs.visitGrid[testCol][testRow];
				else {
					testRow = curRow;
					neighborCost = 0;
				}
				break;
			}

			if (neighborCost > bestCost) {
				bestCost = neighborCost;
				bestRow = testRow;
				bestCol = testCol;

				// IDA: directionMode==0 tracks v4 (testRow/gridCol), ==1 tracks v2 (testCol/gridRow)
				if (rs.directionMode == 0) {
					if (testRow > maxProgress)
						maxProgress = testRow;
				} else if (rs.directionMode == 1) {
					if (testCol > maxProgress)
						maxProgress = testCol;
				}
			}
		}

		// Zero current cell and advance
		rs.visitGrid[bestCol][bestRow] = 0;
		curRow = bestRow;
		curCol = bestCol;
		iterCount++;
	}
}

void ZoombiniPuzzleLilly::initRunnerBFSPath(int16 runnerIdx) {
	// IDA: maze_initRunnerBFSPath (0x429440)
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	// Clear visit grid
	memset(rs.visitGrid, 0, sizeof(rs.visitGrid));

	// Copy start position to frontier
	rs.frontierCol = rs.col;
	rs.frontierRow = rs.row;

	// Set target row and initial step
	rs.targetRow = 11;
	rs.stepCount = 1;

	// Run Dijkstra twice and then BFS traverse
	// IDA: calls computeShortestPath twice, then traversePathBFS
	computeShortestPath(rs.targetRow, runnerIdx);
	computeShortestPath(rs.targetRow, runnerIdx);
	traversePathBFS(rs.targetRow, runnerIdx);

	// Mark starting cell in visit grid
	rs.visitGrid[rs.row][rs.col] = rs.stepCount;
}

// =================================================================
// Click handling
// IDA: lilly_onClickHandler (0x4245CC)
// =================================================================

ZmbEventHandleResult ZoombiniPuzzleLilly::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// Let base class handle standard buttons (Go/Map/Help)
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Guard: check puzzle state
	if (!_bPuzzleActive)
		return ZmbEventHandleResult::kPassthrough;

	// IDA: Cell-pair swap clicks must pass through even while _bCellSelectActive
	// is set, because the swap handler is what advances cellSelectState 4→5→6.
	// In the original, this runs inside the blocking modal fleens_runAttrSelectLoop.
	// Only runner clicks (zoombini/obstacle/frog) are gated by _bCellSelectActive.

	// --- Runner clicks: blocked during active cell selection ---
	if (!_bCellSelectActive) {

		// IDA: Case 4 — Zoombini/Frog click
		// Original: click_findRunnerAtPoint(1, 0x980002, pPos) finds the frog or
		// a returned zoombini (bitmask 0x980002). ScummVM equivalent: check
		// isRenderActivated() + hasClickRect() + clickRect.contains().
		//
		// Gate: ui_bDragLockActive (mapped to _remainingZmbs) > 0 blocks zoombini
		// clicks but still allows obstacles/frog (type == 2). After auto-entered
		// runners fire their SCRB event 1, _remainingZmbs reaches 0.

		// --- Zoombini runner click (only when _remainingZmbs <= 0) ---
		if (_remainingZmbs <= 0) {
			for (int16 i = 0; i < _totalZmbCount; i++) {
				if (!_zmbRunners[i] || !_zmbRunners[i]->isRenderActivated())
					continue;

				ZmbLillyRunnerState &rs = _runnerStates[i];
				if (rs.placed)
					continue;

				// Hit test against runner feature
				if (_zmbRunners[i]->hasClickRect() &&
				    _zmbRunners[i]->getClickRect().contains(absPos)) {
					// IDA fleens_interactiveCellSelectLoop (mouse mode): the player's
					// click position selects which entry lane the snoid takes.
					// Pick the nearest entry-point row based on the click's Y coordinate.
					int16 nearestEntry = 0;
					int32 bestDist = INT32_MAX;
					for (int16 e = 0; e < 12; e++) {
						int32 dy = (int32)absPos.y - kEntryPositions[e].y;
						int32 d = dy * dy;
						if (d < bestDist) { bestDist = d; nearestEntry = e; }
					}
					_runnerStates[i].entryPointIdx = nearestEntry;
					handleZoombiniClick(_zmbRunners[i]);

					// IDA 0x424974: Chain mechanism — activate next hidden runner from pool.
					// In the original, word_4AE3C2[placedZmbCount] (child runner) gets
					// SCRB 10109+idx loaded with exit callback → auto-enters via event chain.
					// The body stays at icon cluster. In ScummVM single-runner architecture,
					// re-load the icon idle SCRB with deferred animation so the next runner
					// is visible at its icon position but doesn't auto-enter. Player must
					// click it to start grid entry.
					if (_placedZmbCount < _totalZmbCount) {
						int16 nextIdx = _placedZmbCount;
						if (nextIdx >= 0 && nextIdx < kMaxRunners && _zmbRunners[nextIdx]) {
							loadScrbOntoFeature(_zmbRunners[nextIdx], 10109 + nextIdx);
							_zmbRunners[nextIdx]->deactivateAnimate(); // DEFER_ANIM: show first frame only
							_runnerStates[nextIdx].callbackMode = kCBLillyNone;
						}
						_placedZmbCount++;
					}

					return ZmbEventHandleResult::kConsumed;
				}
			}
		}

		// --- Obstacle (toad) click — enter cell swap mode ---
		// IDA: click_findRunnerAtPoint(1, 0x980002, pPos) matches obstacle runners
		// (attrType != 0). For obstacles, fleens_runAttrSelectLoop enters swap mode
		// (fleens_cellSelectState=4, lilly_cellSelectState=4). Obstacle clicks bypass
		// the _remainingZmbs gate (original: ui_bDragLockActive bypass for type==2).
		for (int16 j = 0; j < _obstacleRunnerCount; j++) {
			int16 ri = _obstacleRunners[j];
			if (ri < 0 || ri >= kMaxRunners || !_zmbRunners[ri])
				continue;
			if (_zmbRunners[ri]->hasClickRect() &&
			    _zmbRunners[ri]->getClickRect().contains(absPos)) {
				// IDA: fleens_runAttrSelectLoop at 0x4287B2: when attrType != 0,
				// sets fleens_cellSelectState=4, lilly_cellSelectState=4 (swap mode).
				// Only consume click when state can change (state 0→4). Otherwise,
				// let click fall through to cell swap handler (state 4/5 below).
				if (_cellSelectState == 0 && _swapLevel < 6) {
					_cellSelectState = 4;
					return ZmbEventHandleResult::kConsumed;
				}
				// When swap mode is already active, clicking a toad on the left
				// bank should not cancel swap mode. Consume the click to prevent
				// the "outside grid" else branch from resetting cellSelectState.
				return ZmbEventHandleResult::kConsumed;
			}
		}
		debugC(1, kZmbDebugAnimation, "Lilly: click at (%d,%d), obstCount=%d, cellSel=%d",
			absPos.x, absPos.y, _obstacleRunnerCount, _cellSelectState);
		for (int16 j = 0; j < _obstacleRunnerCount; j++) {
			int16 ri = _obstacleRunners[j];
			if (ri < 0 || ri >= kMaxRunners || !_zmbRunners[ri])
				continue;
			bool hc = _zmbRunners[ri]->hasClickRect();
			Common::Rect cr = hc ? _zmbRunners[ri]->getClickRect() : Common::Rect();
			debugC(1, kZmbDebugAnimation, "  obst[%d] ri=%d hasClick=%d rect=(%d,%d,%d,%d) renderAct=%d",
				j, ri, hc ? 1 : 0, cr.left, cr.top, cr.right, cr.bottom,
				_zmbRunners[ri]->isRenderActivated() ? 1 : 0);
		}

		// --- Frog click — start cell swap selection (difficulty >= 2) ---
		// Original: clicking frog enters modal loop (fleens_interactiveCellSelectLoop).
		// Non-blocking: clicking frog sets _cellSelectState=4 to enable cell clicks.
		if (_frogRunnerFeature && _frogRunnerFeature->isRenderActivated() &&
		    _cellSelectState == 0 && _swapLevel < 6) {
			if (_frogRunnerFeature->hasClickRect() &&
			    _frogRunnerFeature->getClickRect().contains(absPos)) {
				_cellSelectState = 4;
				return ZmbEventHandleResult::kConsumed;
			}
		}

	} // end !_bCellSelectActive

	// IDA: Cell-pair swap selection (from fleens_interactiveCellSelectLoop, 0x4286A5)
	// When cellSelectState is 4 or 5, clicking an unoccupied cell selects it for swap.
	if ((_cellSelectState == 4 || _cellSelectState == 5) && _swapLevel < 6) {
		// IDA: 4px inset margin on cell rects for hit test
		int16 hitCol = -1, hitRow = -1;
		for (int16 row = 0; row < 12; row++) {
			for (int16 col = 0; col < 12; col++) {
				Common::Rect insetRect = _gridCellRect[row][col];
				insetRect.left += 4;
				insetRect.top += 4;
				insetRect.right -= 4;
				insetRect.bottom -= 4;
				// IDA: Offset mouse position by (27, 22) before hit-testing
				Common::Point testPos(absPos.x + 27, absPos.y + 22);
				if (insetRect.contains(testPos) && _gridOccupancy[row][col] == 0) {
					hitCol = col;
					hitRow = row;
					row = 12; // break outer
					break;
				}
			}
		}

		if (hitCol >= 0 && hitRow >= 0) {
			if (_cellSelectState == 4) {
				// IDA at 0x428CC4: Select first cell
				_swapCellACol = hitCol;
				_swapCellARow = hitRow;
				// IDA: Play sound 12000+idx (cycling 0-3)
				_vm->_sound->playZmbSound(
					ZmbResource(ZmbArchiveKind::kPage, 12000 + _swapSoundIdx));
				_swapSoundIdx = (_swapSoundIdx + 1) > 3 ? 0 : (_swapSoundIdx + 1);
				setRunnerClickRect(_swapCellACol, _swapCellARow, _cellAnimRunnerA);
				_cellSelectState = 5;
				_bCellSelectActive = true;
				return ZmbEventHandleResult::kConsumed;
			} else if (_cellSelectState == 5) {
				// IDA at 0x428CC7: Select second cell
				_swapCellBCol = hitCol;
				_swapCellBRow = hitRow;
				_vm->_sound->playZmbSound(
					ZmbResource(ZmbArchiveKind::kPage, 12000 + _swapSoundIdx));
				_swapSoundIdx = (_swapSoundIdx + 1) > 3 ? 0 : (_swapSoundIdx + 1);
				setRunnerClickRect(_swapCellBCol, _swapCellBRow, _cellAnimRunnerB);
				_cellSelectState = 6;

				// IDA at 0x428DB8: Check unlock threshold
				if ((_swapCellBCol != _swapCellACol || _swapCellBRow != _swapCellARow) &&
				    ++_swapCounter >= _swapThreshold && _swapLevel < 6) {
					_swapLevel++;
					_swapCounter = 0;
					// IDA: Load frog SCRB 10078 + unlockProgress
					if (_frogRunnerFeature) {
						loadScrbOntoFeature(_frogRunnerFeature,
							10078 + _swapLevel);
					}
					if (_swapLevel == 6 && _placedZmbCount == _totalZmbCount) {
						countMatchesAndPlaySound();
					}
				}
				return ZmbEventHandleResult::kConsumed;
			}
		} else {
			// IDA: Click outside grid — cancel cell selection
			if (_cellSelectState == 5) {
				initCellRunnerPosition(_swapCellACol, _swapCellARow, _cellAnimRunnerA);
			}
			_cellSelectState = 0;
			_bCellSelectActive = false;
			return ZmbEventHandleResult::kConsumed;
		}
	}

	// Check for cell selection during interactive mode
	if (_bCellSelectActive && _selectingRunnerIdx >= 0) {
		int16 clickCol, clickRow;
		if (findCellAtPoint(absPos, clickCol, clickRow) >= 0) {
			if (isCellValidForRunner(clickCol, clickRow, _selectingRunnerIdx)) {
				_selectedCellIdx = clickRow * 13 + clickCol;
				_bCellSelectActive = false;

				// Finalize placement
				ZmbLillyRunnerState &rs = _runnerStates[_selectingRunnerIdx];
				rs.col = clickCol;
				rs.row = clickRow;
				rs.placed = true;

				// Mark cell occupied
				_gridOccupancy[clickRow][clickCol] = 1;

				// Set runner position
				Common::Point cellPos = _gridCellPos[clickRow][clickCol];
				_zmbRunners[_selectingRunnerIdx]->setPointLoc(cellPos);

				// Initialize BFS path
				initRunnerBFSPath(_selectingRunnerIdx);

				// Load movement SCRB
				loadScrbOntoFeature(_zmbRunners[_selectingRunnerIdx], 10109 + _selectingRunnerIdx);

				_placedZmbCount++;

				// IDA: Check for completion
				if (_placedZmbCount == _totalZmbCount && _swapLevel == 6) {
					countMatchesAndPlaySound();
				}

				// Enable advance if all placed
				if (_placedZmbCount >= _totalZmbCount) {
					_bAdvanceEnabled = true;
				}

				_selectingRunnerIdx = -1;
				return ZmbEventHandleResult::kConsumed;
			}
		}
	}

	return ZmbEventHandleResult::kPassthrough;
}

void ZoombiniPuzzleLilly::handleZoombiniClick(ZmbFeature *clickedRunner) {
	// IDA: lilly_onClickHandler case 4
	// In the original engine, clicking a zoombini runner launches
	// fleens_interactiveCellSelectLoop (a blocking drag loop that also
	// handles the cell-pair swap mechanic for diff >= 2). Since the
	// zoombini runners have attrType == 0, the mouse-mode cell selection
	// NEVER finds a valid cell — so the zoombini is always placed at an
	// entry position (left side) and auto-traverses the grid.
	//
	// The cell-pair swap mechanic is handled separately via
	// onLButtonDown cell click detection when _cellSelectState is 4/5.

	// Find runner index
	int16 runnerIdx = -1;
	for (int16 i = 0; i < _totalZmbCount; i++) {
		if (_zmbRunners[i] == clickedRunner) {
			runnerIdx = i;
			break;
		}
	}

	if (runnerIdx < 0)
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
	if (rs.placed)
		return;

	// IDA handleZoombiniClick (lilly interactive cell-select result):
	// Sets per-runner attrType + attrValue + combinedAttr based on the
	// selected entry cell's grid attributes. Without these, advancePathOnGrid
	// treats every cell as valid, which defeats the attribute-matching
	// puzzle. The entry-point index picks which lily-pad column the snoid
	// enters from, and the grid cell at that column determines the attribute
	// constraint path it must follow.
	//
	// If onLButtonDown pre-populated `entryPointIdx` based on click position,
	// honor it. Otherwise fall back to runnerIdx modulo 12 (auto-assign).
	if (rs.entryPointIdx < 0 || rs.entryPointIdx >= 12)
		rs.entryPointIdx = runnerIdx % 12;

	// Set position from entry point table
	if (rs.entryPointIdx >= 0 && rs.entryPointIdx < 12) {
		Common::Point entryPos = kEntryPositions[rs.entryPointIdx];
		clickedRunner->setPointLoc(entryPos);
	}

	// Pull attribute constraint from the entry cell. IDA reads the grid
	// attribute type from `grid_attrLayer{1,2,3}[169*col + 13*row]` for the
	// runner's entry cell. We use row 0 (top of lily pad) and the entry
	// column.
	int16 entryCol = rs.entryPointIdx;
	if (entryCol >= 0 && entryCol < 12) {
		// Use grid attribute layer based on current puzzle attribute type.
		// _obstacleAttrType encodes which layer this difficulty uses.
		rs.attrType = _obstacleAttrType != kLillyPadAttrNone ? _obstacleAttrType : kLillyPadAttrPattern;
		switch (rs.attrType) {
		case kLillyPadAttrPattern: rs.attrValue = _gridAttr1[entryCol][0]; break;
		case kLillyPadAttrShape: rs.attrValue = _gridAttr2[entryCol][0]; break;
		case kLillyPadAttrColor: rs.attrValue = _gridAttr3[entryCol][0]; break;
		default: rs.attrValue = 0; break;
		}
		rs.obsCombinedAttr = rs.attrValue;
	}

	// IDA: Set dirByte for arrival animation. processArriveQueue loads
	// SCRB 10019 + rs.dirByte. In the original, the chain activation stores
	// lilly_clickedCellIdx (= entry column 0-11) into hsArr[5].shapeid,
	// which processArriveQueue reads as the direction byte.
	rs.dirByte = static_cast<byte>(rs.entryPointIdx);

	// IDA: Load entry SCRB (10109 + runnerIdx) on the child runner.
	// In the original, SCRB 10109+idx fires event 2 (adjusted from 0xFF03)
	// at frame 5, which routes through exitCallback → arriveQueue.
	// SCRB 10043+entryIdx was loaded on the separate SNOID for visual walk-in,
	// but we don't display the snoid walk-in — the child drives everything.
	clickedRunner->activateRender();
	loadScrbOntoFeature(clickedRunner, 10109 + runnerIdx);

	// Reset selected cell state
	// IDA: fleens_resetRunnerIdxState (0x42937A) — clears draw flag
	_selectedCellIdx = 0;

	// Mark as placed (entered grid)
	rs.placed = true;
	rs.col = 0;
	rs.row = static_cast<byte>(rs.entryPointIdx);

	// IDA: runner+243 = column position (initially 0 = left edge of grid)
	// IDA: runner+244 = row position (initially = entry lane index)
	// NOTE: Field naming is inverted: obstRow is at +243 (=col), obstCol is at +244 (=row).
	rs.obstRow = 0;                                    // +243 = initial column (left edge)
	rs.obstCol = static_cast<byte>(rs.entryPointIdx);  // +244 = entry row lane

	// Initialize BFS path for this runner
	initRunnerBFSPath(runnerIdx);

	// NOTE: _placedZmbCount is NOT incremented here. In the original
	// (0x4249D9), lilly_placedZmbCount is incremented in the click handler
	// AFTER activating the next pool runner, not during placement.
	// The increment is in onLButtonDown's chain activation code.

	// IDA: Set kCBLillyDepart callback mode. SCRB 10109+idx fires event 2
	// (0xFF03 terminator, adjusted=2) at frame 5, which chains through:
	// exitCallback event 2 → arriveQueue → processArriveQueue (SCRB 10019+dir)
	// → event 20 → readyQueue → grid traversal begins.
	rs.moveTimer = 0;
	rs.callbackMode = kCBLillyDepart;
}

int16 ZoombiniPuzzleLilly::findCellAtPoint(const Common::Point &pos, int16 &outCol, int16 &outRow) const {
	// Hit-test all grid cells against the given point
	for (int row = 0; row < 12; row++) {
		for (int col = 0; col < 12; col++) {
			if (_gridCellRect[row][col].contains(pos)) {
				outCol = col;
				outRow = row;
				return row * 13 + col;
			}
		}
	}
	outCol = -1;
	outRow = -1;
	return -1;
}

bool ZoombiniPuzzleLilly::isCellValidForRunner(int16 col, int16 row, int16 runnerIdx) const {
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return false;

	const ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	// Cell must be unoccupied
	if (_gridOccupancy[row][col] != 0)
		return false;

	// Cell must match attribute constraint
	switch (rs.attrType) {
	case kLillyPadAttrPattern: return _gridAttr1[row][col] == rs.attrValue;
	case kLillyPadAttrShape: return _gridAttr2[row][col] == rs.attrValue;
	case kLillyPadAttrColor: return _gridAttr3[row][col] == rs.attrValue;
	default: return true;
	}
}

// =================================================================
// Animation event dispatch
// Dispatches SCRB animation events based on per-runner callbackMode.
// Each queue processing function sets the callbackMode when loading a
// SCRB, so the event dispatch knows which IDA callback to emulate.
// =================================================================

void ZoombiniPuzzleLilly::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (!feature)
		return;

	// --- Standard body arrangement events (240-253) ---
	if (eventCode >= kZmbAnimEvent240_BodyArrangePendFirst &&
	    eventCode <= kZmbAnimEvent253_BodyArrangeDirectLast) {
		if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);
			if (eventCode >= kZmbAnimEvent250_BodyArrangeDirectFirst) {
				snoid->setBodyArrangement(eventCode - kZmbAnimEvent250_BodyArrangeDirectFirst);
			}
		}
		return;
	}

	// --- Event 0: Toggle render visibility ---
	if (eventCode == 0) {
		if (feature->isRenderActivated())
			feature->deactivateRender();
		else
			feature->activateRender();
		return;
	}

	// --- Frog/script events ---
	if (feature == _frogScrbFeature || feature == _frogRunnerFeature) {
		handleScriptEvent(eventCode, feature);
		return;
	}

	// --- Find runner index from feature pointer ---
	int16 runnerIdx = -1;
	for (int16 i = 0; i < kMaxRunners; i++) {
		if (_zmbRunners[i] == feature) {
			runnerIdx = i;
			break;
		}
	}

	// --- exitAnimFeature event routing ---
	// In the original, SCRB 10000 is loaded on the grandchild runner (runner+72
	// of the child). Event 20 from SCRB 10000 fires on the grandchild, whose
	// callback (maze_handlePathBuildEvent) pushes to readyQueue. In ScummVM,
	// SCRB 10000 is loaded on _exitAnimFeatures[], which are not in _zmbRunners[].
	// Route their events back to the corresponding runner index.
	if (runnerIdx < 0) {
		for (int16 i = 0; i < kMaxRunners; i++) {
			if (_exitAnimFeatures[i] == feature) {
				runnerIdx = i;
				break;
			}
		}
		if (runnerIdx >= 0) {
			// IDA: maze_handlePathBuildEvent event 20 → readyQueue push
			if (eventCode == 20) {
				if (_readyQueueSize < kMaxMoveQueueSize)
					_readyQueue[_readyQueueSize++] = runnerIdx;
			}
			return;
		}
	}

	if (runnerIdx < 0)
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	switch (rs.callbackMode) {
	case kCBLillyEnter:
		// IDA: maze_runnerExitGridStep (0x4253C1) — event 44 only
		if (eventCode == 44) {
			// Snap position to cell center
			if (rs.obstCol < 12 && rs.obstRow < 13)
				feature->setPointLoc(_gridCellPos[rs.obstCol][rs.obstRow]);
			// Clear Lilly's exit reservation for this cell.
			_gridExitReservation[rs.obstCol][rs.obstRow] = 0;
			// Push to rotateQueue
			if (_rotateQueueSize < kMaxQueueSize)
				_rotateQueue[_rotateQueueSize++] = runnerIdx;
		}
		break;

	case kCBLillyRotate:
		// IDA: maze_recordRunnerPlacement (0x42547D) — event 60 only
		if (eventCode == 60) {
			// Snap position
			if (rs.obstCol < 12 && rs.obstRow < 13)
				feature->setPointLoc(_gridCellPos[rs.obstCol][rs.obstRow]);
			// Clear Lilly's exit reservation for this cell.
			_gridExitReservation[rs.obstCol][rs.obstRow] = 0;
			// Push to exitQueue
			if (_exitQueueSize < kMaxQueueSize)
				_exitQueue[_exitQueueSize++] = runnerIdx;
			// Clear active enter runner (allows next enter to proceed)
			_activeEnterRunner = -1;
		}
		break;

	case kCBLillyExit:
		// IDA: maze_runnerSnapAndRegisterCell (0x425A7C) — event 49 only
		if (eventCode == 49) {
			// Snap position
			if (rs.obstCol < 12 && rs.obstRow < 13)
				feature->setPointLoc(_gridCellPos[rs.obstCol][rs.obstRow]);
			// IDA: *(byte*)(runner+242) = 0 — clear placed flag so runner is clickable again
			rs.placed = false;
			rs.direction = 0;
			// IDA: runner[8] = 0x980002 (flags) — runner becomes clickable on left bank
			_completedExitRunner = runnerIdx;
		}
		break;

	case kCBLillyCross:
		// IDA: maze_runnerSnapPosAndClearCell (0x425781) — event 54 only
		if (eventCode == 54) {
			// Snap position
			if (rs.obstCol < 12 && rs.obstRow < 13)
				feature->setPointLoc(_gridCellPos[rs.obstCol][rs.obstRow]);
			// Clear Lilly's exit reservation for this cell.
			_gridExitReservation[rs.obstCol][rs.obstRow] = 0;
			_completedCrossRunner = runnerIdx;
			removeActiveBfsRunner(runnerIdx);
		}
		break;

	case kCBLillyDepart:
		// IDA: maze_runnerExitCallback (0x425CCA) — events 1, 2, 3
		// Depart SCRB uses same callback as child path runner.
		handleRunnerExitCallback(eventCode, runnerIdx);
		break;

	case kCBLillyArrive:
		// IDA: maze_handlePathBuildEvent (0x42A17D) — events 20, 26
		if (eventCode == 20) {
			if (_readyQueueSize < kMaxMoveQueueSize)
				_readyQueue[_readyQueueSize++] = runnerIdx;
		} else if (eventCode == 26 && _exitAnimFeatures[runnerIdx]) {
			// IDA reloads the linked child runner with SCRB 10000 here.
			// Reuse the dedicated child feature so the extra Lilly path visual stays
			// separate from the primary path runner in the current architecture.
			_exitAnimFeatures[runnerIdx]->setPointLoc(feature->getPointLoc());
			_exitAnimFeatures[runnerIdx]->activateRender();
			loadScrbOntoFeature(_exitAnimFeatures[runnerIdx], 10000);
		}
		break;

	case kCBLillyReadyMove:
		// IDA: maze_zmbMoveFinalizeStep (0x424A5B) — events 10-15
		handleMoveFinalizeStep(eventCode, runnerIdx);
		break;

	case kCBLillyReadyExit:
		// IDA: maze_runnerArriveAtNode (0x425ADB) — event 30
		if (eventCode == 30)
			handleArriveAtNode(runnerIdx);
		break;

	case kCBLillyMoveObstacle:
	case kCBLillyMoveStep:
		// IDA: maze_runnerArriveOrDepartCallback (0x424D3E) — events 70, 80
		handleRunnerArriveOrDepart(eventCode, runnerIdx);
		break;

	default:
		break;
	}
}

// =================================================================
// Callbacks
// =================================================================

void ZoombiniPuzzleLilly::handleRunnerExitCallback(int16 exitCode, int16 runnerIdx) {
	// IDA: maze_runnerExitCallback (0x425CCA)
	switch (exitCode) {
	case 1:
		// IDA: call maze_setRunnerIdxVar(5), decrement ui_bDragLockActive
		_swapBlinkMax = 5; // IDA: lilly_stateVar6 = 5
		if (--_remainingZmbs < 0)
			_remainingZmbs = 0;
		break;

	case 2:
		// IDA: Push to arrive queue
		if (_arriveQueueSize < kMaxQueueSize) {
			_arriveQueue[_arriveQueueSize++] = runnerIdx;
		}
		break;

	case 3:
		// IDA: Push to depart queue
		if (_departQueueSize < kMaxQueueSize) {
			_departQueue[_departQueueSize++] = runnerIdx;
		}
		break;

	default:
		break;
	}
}

void ZoombiniPuzzleLilly::handleRunnerArriveOrDepart(int16 eventCode, int16 runnerIdx) {
	// IDA: maze_runnerArriveOrDepartCallback (0x424D3E)
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	if (eventCode == 70) {
		// Arrive: snapshot position, push to move queue
		// IDA: runner+214..220 = runner+50..52 (position snapshot)
		if (_moveQueueSize < kMaxMoveQueueSize) {
			_moveQueue[_moveQueueSize++] = runnerIdx;
		}
	} else if (eventCode == 80) {
		// Depart: clear occupancy, push to freed list, remove from active obstacles
		_gridOccupancy[rs.obstCol][rs.obstRow] = 0;

		if (_freedRunnerCount < kMaxRunners) {
			_freedRunners[_freedRunnerCount++] = runnerIdx;
		}

		// Remove from active obstacles array
		for (int16 i = 0; i < _activeObstacleCount; i++) {
			if (_activeObstacles[i] == runnerIdx) {
				for (int16 j = i; j < _activeObstacleCount - 1; j++)
					_activeObstacles[j] = _activeObstacles[j + 1];
				_activeObstacleCount--;
				break;
			}
		}
	}
}

void ZoombiniPuzzleLilly::handleMoveFinalizeStep(int16 stepIdx, int16 runnerIdx) {
	// IDA: maze_zmbMoveFinalizeStep (0x424A5B)
	// Handles events 10-15 during zoombini grid traversal.
	// Events fire at specific SCRB animation frames during direction SCRBs.
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners || !_zmbRunners[runnerIdx])
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
	ZmbFeature *feature = _zmbRunners[runnerIdx];

	switch (stepIdx) {
	case 10:
		// IDA: Snap position to destination, push to readyQueue with 30-frame timer.
		// runner+214..220 = runner+253..255 (all hotspot positions = dest position)
		feature->setPointLoc(Common::Point(rs.destX, rs.destY));
		// Push to readyQueue
		if (_readyQueueSize < kMaxMoveQueueSize)
			_readyQueue[_readyQueueSize++] = runnerIdx;
		// Set timer for 30 frames delay before next step
		rs.moveTimer = getCurrentFrameCounter() + 30;
		break;

	case 11: {
		// IDA: Save old row/col to +245/+246, update row/col from direction,
		// compute destination pixel coordinates.
		// runner+245 = runner+243, runner+246 = runner+244
		rs.prevRow = rs.obstRow;
		rs.prevCol = rs.obstCol;

		// IDA: direction 0=up(row--), 1=right(col++), 2=down(row++), 3=left(col--)
		// NOTE: obstRow(+243)=col, obstCol(+244)=row due to naming convention
		switch (rs.dirByte) {
		case 0: // Up: row-- (obstCol is the row field at +244)
			if (rs.obstCol > 0)
				rs.obstCol--;
			break;
		case 1: // Right: col++ (obstRow is the col field at +243)
			if (rs.obstRow < 12)
				rs.obstRow++;
			break;
		case 2: // Down: row++ (obstCol is the row field at +244)
			if (rs.obstCol < 11)
				rs.obstCol++;
			break;
		case 3: // Left: col-- (obstRow is the col field at +243)
			if (rs.obstRow > 0)
				rs.obstRow--;
			break;
		}

		// IDA: Compute destination pixel position from grid cell position
		// posX = 35 * runner[243] + REGS_X[runner[244]+1]
		// posY = kColYOffset[runner[243]] + REGS_Y[runner[244]+1]
		if (rs.obstCol < 12 && rs.obstRow < 13) {
			rs.destX = _gridCellPos[rs.obstCol][rs.obstRow].x;
			rs.destY = _gridCellPos[rs.obstCol][rs.obstRow].y;
		}

		// Sync path position (at +195/196) with animation position (at +243/244).
		// In the original engine, +195/196 is updated by the SCRB animation system
		// and maze_advanceRunnerStep. In ScummVM, we sync explicitly so that
		// advancePathOnGrid sees the correct current position.
		// obstRow(+243) = col dimension, obstCol(+244) = row dimension
		rs.col = rs.obstRow;
		rs.row = rs.obstCol;
		break;
	}

	case 12: {
		// IDA: Direction-dependent hotspot interpolation + clear old cell occupancy.
		// Half-step interpolation toward destination for walking visual effect.
		// After interpolation, clears occupancy at the previous cell.

		// IDA: Clear old cell occupancy using saved +245/+246 position
		// lilly_zmbRunnerIdxArr_4AC684[169*runner[246] + 13*runner[245]] = 0
		if (rs.prevCol < 12 && rs.prevRow < 13)
			_gridOccupancy[rs.prevCol][rs.prevRow] = 0;
		break;
	}

	case 13:
	case 14:
		// IDA: Snap hotspot positions based on direction.
		// Dir 0/2 (vertical): set all hotspot X = destX
		// Dir 1/3 (horizontal): set all hotspot Y = destY
		// In ScummVM the feature position is handled by the animation system,
		// so we just ensure the feature is at the correct intermediate position.
		break;

	case 15:
		// IDA: Push to readyQueue for immediate next step (no timer).
		if (_readyQueueSize < kMaxMoveQueueSize)
			_readyQueue[_readyQueueSize++] = runnerIdx;
		break;

	default:
		break;
	}
}

void ZoombiniPuzzleLilly::handleArriveAtNode(int16 runnerIdx) {
	// IDA: maze_runnerArriveAtNode (0x425ADB) — event 30
	// Runner reached the exit column, arriving at the other side.
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners || !_zmbRunners[runnerIdx])
		return;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
	ZmbFeature *feature = _zmbRunners[runnerIdx];

	// IDA: Snap position using delta tables.
	// finalPos = baseHotspot + deltaTable[stepIdx]
	// In ScummVM we simplify to the grid cell position.
	if (rs.obstCol < 12 && rs.obstRow < 13)
		feature->setPointLoc(_gridCellPos[rs.obstCol][rs.obstRow]);

	// IDA: lilly_stateVar8++. First arrival enables advance button.
	if (++_arrivedCount == 1)
		_bAdvanceEnabled = true;

	// IDA: runner+267++ (crossedCount)
	rs.crossedCount++;

	// IDA: runner+262 = 0 (clear BFS maxProgress — runner has arrived, no longer "path valid")
	rs.targetRow = 0;

	// IDA: Dispatch based on crossedCount
	if (rs.crossedCount == 2) {
		// Crossed twice — done, add to crossQueue (will be freed)
		if (_crossQueueSize < kMaxQueueSize)
			_crossQueue[_crossQueueSize++] = runnerIdx;
	} else {
		// First crossing — re-enter for second pass
		if (_enterQueueSize < kMaxQueueSize)
			_enterQueue[_enterQueueSize++] = runnerIdx;
	}

	// IDA: Clear grid occupancy at current cell
	_gridOccupancy[rs.obstCol][rs.obstRow] = 0;

	// IDA: runner+60 = 0 (clear core188+0x0C field — animation state)
	rs.pathStepIdx = 0;

	// IDA: Load exit animation SCRB 10129+obstCol on a separate exit anim feature.
	// In the original engine, this is loaded on a child runner (word_4AE3C2[]).
	// Since _zmbRunners[] get their SCRB overwritten by next queue processing,
	// we use dedicated _exitAnimFeatures[] for the exit walk-off animation.
	if (_exitAnimFeatures[runnerIdx]) {
		_exitAnimFeatures[runnerIdx]->activateRender();
		// Position at the arrival cell
		if (rs.obstCol < 12 && rs.obstRow < 13)
			_exitAnimFeatures[runnerIdx]->setPointLoc(_gridCellPos[rs.obstCol][rs.obstRow]);
		loadScrbOntoFeature(_exitAnimFeatures[runnerIdx], 10129 + rs.obstCol);
	}

	// IDA: Find parent snoid runner, increment completedZmbCount, set matched.
	// In the original, done via child.core188[0x1A] → runner_findByIndex → snoid.
	// We track completion directly since _zmbRunners[] serve as the child runners.
	_completedZmbCount++;
	rs.matched = true;

	// IDA: word_4AE772 check — when all zoombinis completed, play sound.
	if (_completedZmbCount == _totalZmbCount) {
		uint16 soundId = _vm->_rnd->getRandomNumber(20055, 20063);
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, soundId));
	}
}

void ZoombiniPuzzleLilly::handleScriptEvent(int16 eventId, ZmbFeature *eventFeature) {
	// IDA: maze_scriptEventHandler (0x425D55)
	switch (eventId) {
	case 3:
		// IDA 0x425D55 case 3: Activate last 2 zoombini runners with exit SCRBs.
		// Original uses the ZOOMBINI RUNNERS (word_4AE3C2[]), NOT separate features.
		// Sets wBoolDoRender=1, loads SCRB 10089+i, and sets depart callbacks
		// (preRender=maze_advanceRunnerStep, hotspot=maze_runnerExitCallback).
		if (_difficultyLevel >= kPuzzleDiffLevel2) {
			for (int16 i = MAX((int16)0, (int16)(_totalZmbCount - 2)); i < _totalZmbCount; i++) {
				if (_zmbRunners[i]) {
					_zmbRunners[i]->activateRender();
					loadScrbOntoFeature(_zmbRunners[i], 10089 + i);
					_runnerStates[i].callbackMode = kCBLillyDepart;
				}
			}
		}
		break;

	case 4:
		// IDA: Frog SCRB done — free it and activate the frog interactive runner.
		// lilly_exitedRunnerIdx = eventData.runnerIndex (deferred free)
		if (eventFeature) {
			eventFeature->deactivateRender();
			eventFeature->deactivateAnimate();
		}
		if (_frogRunnerFeature) {
			_frogRunnerFeature->activateRender();
		}
		// IDA: Init obstacle BFS grids for difficulty >= 3.
		// maze_initBFSGrid_4294EB(j, word_4AE370) for each j in [0, gridType)
		if (_difficultyLevel >= kPuzzleDiffLevel3) {
			for (int16 j = 0; j < _gridType; j++) {
				initBFSGrid(j, _obstacleAttrType);
			}
		}
		break;

	case 5:
		// IDA: Cell swap state machine
		if (_cellSelectState == 4) {
			// Select first swap cell
			_swapCellACol = kSwapPairCol[2 * _swapPairIdx];
			_swapCellARow = kSwapPairRow[2 * _swapPairIdx];
			setRunnerClickRect(_swapCellACol, _swapCellARow, _cellAnimRunnerA);
			_cellSelectState = 5;
			_swapPairIdx++;
		} else if (_cellSelectState == 5) {
			// Select second swap cell
			_swapCellBCol = kSwapPairCol[2 * _swapPairIdx];
			_swapCellBRow = kSwapPairRow[2 * _swapPairIdx];
			setRunnerClickRect(_swapCellBCol, _swapCellBRow, _cellAnimRunnerB);
			_cellSelectState = 6;
			_swapPairIdx++;
		}
		break;

	default:
		break;
	}
}

// =================================================================
// Helpers
// =================================================================

void ZoombiniPuzzleLilly::removeActiveBfsRunner(int16 runnerIdx) {
	for (int16 i = 0; i < _activeBfsRunnerCount; i++) {
		if (_activeBfsRunners[i] != runnerIdx)
			continue;

		for (int16 j = i + 1; j < _activeBfsRunnerCount; j++)
			_activeBfsRunners[j - 1] = _activeBfsRunners[j];

		_activeBfsRunners[--_activeBfsRunnerCount] = -1;
		break;
	}
}

void ZoombiniPuzzleLilly::countMatchesAndPlaySound() {
	// IDA: fleens_countAttrMatchAndEnqueueSound (0x429395)
	// Iterates live zoombini BFS runners (word_4AE3EC[] in original).
	// For each: checks placed (core188+0xC2 != 0) AND targetRow == 11 (core188+0xD6 == 11).
	// targetRow == 11 means the BFS path successfully reaches the exit row.
	// Runners that already exited have targetRow cleared to 0 by handleArriveAtNode
	// and are removed from the live list during event 54 cross completion.
	int16 matchCount = _completedZmbCount;

	for (int16 i = 0; i < _activeBfsRunnerCount; i++) {
		int16 runnerIdx = _activeBfsRunners[i];
		if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
			continue;

		ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
		if (rs.placed && rs.targetRow == 11) {
			matchCount++;
			rs.matched = true;
		}
	}

	if (matchCount < _totalZmbCount) {
		// IDA: Play fail sound. Gated by random check OR low page-flag count.
		// Random 0-4 > difficultyLevel-1: always-play at easy difficulty.
		// pageFlagLilly & 0xFFF <= 3: always-play early in the puzzle progression.
		int16 rndCheck = _vm->_rnd->getRandomNumber(0, 4);
		uint16 pageFlagLilly = _vm->_state->getPageFlagFromPageType(ZoombiniPageType::kLilly);
		if (rndCheck > _difficultyLevel - 1 || (pageFlagLilly & 0xFFF) <= 3) {
			uint16 soundId = _vm->_rnd->getRandomNumber(20045, 20048);
			_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, soundId));
		}
	}
}

void ZoombiniPuzzleLilly::setRunnerClickRect(int16 col, int16 row, ZmbFeature *feature) {
	// IDA: maze_setRunnerClickRect (0x429196)
	if (!feature)
		return;

	if (col >= 0 && col < 13 && row >= 0 && row < 12) {
		Common::Point cellPos = _gridCellPos[row][col];
		feature->setPointLoc(cellPos);
		feature->setClickRect(_gridCellRect[row][col]);
		feature->activateRender();
	}
}

void ZoombiniPuzzleLilly::swapCellsAndUpdateRunners(int16 colA, int16 rowA, int16 colB, int16 rowB) {
	// IDA: maze_swapCellsAndUpdateRunners (0x4273BC)
	// Swap cell attributes (offsets 9-12 of the 13-byte cell struct) between two grid cells.
	// NOTE: Does NOT swap occupancy (offset 8) or positions (offsets 0-7).

	// Swap attr1 (offset 9)
	byte tmp1 = _gridAttr1[rowA][colA];
	_gridAttr1[rowA][colA] = _gridAttr1[rowB][colB];
	_gridAttr1[rowB][colB] = tmp1;

	// Swap attr2 (offset 10)
	byte tmp2 = _gridAttr2[rowA][colA];
	_gridAttr2[rowA][colA] = _gridAttr2[rowB][colB];
	_gridAttr2[rowB][colB] = tmp2;

	// Swap attr3 (offset 11)
	byte tmp3 = _gridAttr3[rowA][colA];
	_gridAttr3[rowA][colA] = _gridAttr3[rowB][colB];
	_gridAttr3[rowB][colB] = tmp3;

	// Swap combinedAttr (offset 12)
	byte tmpC = _gridCombinedAttr[rowA][colA];
	_gridCombinedAttr[rowA][colA] = _gridCombinedAttr[rowB][colB];
	_gridCombinedAttr[rowB][colB] = tmpC;

	// IDA: First loop — Clear visit grids at swapped cells for live zoombini BFS runners.
	for (int16 i = 0; i < _activeBfsRunnerCount; i++) {
		int16 runnerIdx = _activeBfsRunners[i];
		if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
			continue;

		ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
		if (!rs.placed)
			continue;

		rs.visitGrid[rowA][colA] = 0;
		rs.visitGrid[rowB][colB] = 0;

		if (rs.attrType != kLillyPadAttrNone) {
			byte attrAtA = getGridAttrByType(rs.attrType, rowA, colA);
			byte attrAtB = getGridAttrByType(rs.attrType, rowB, colB);
			if (attrAtA == rs.attrValue || attrAtB == rs.attrValue)
				initRunnerBFSPath(runnerIdx);
		}
	}

	// IDA: Track which BFS layers need re-initialization via cellData[] indexed by attrValue.
	// cellData[2*k] = attrType, cellData[2*k+1] = attrValue for layer k.
	int16 cellData[10] = {};

	for (int16 i = 0; i < _activeObstacleCount; i++) {
		int16 runnerIdx = _activeObstacles[i];
		if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
			continue;
		ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
		if (!rs.placed)
			continue;

		rs.visitGrid[rowA][colA] = 0;
		rs.visitGrid[rowB][colB] = 0;

		if (rs.attrType != kLillyPadAttrNone) {
			byte attrAtA = getGridAttrByType(rs.attrType, rowA, colA);
			byte attrAtB = getGridAttrByType(rs.attrType, rowB, colB);
			if (attrAtA == rs.attrValue || attrAtB == rs.attrValue) {
				initRunnerBFSPath(runnerIdx);
				// IDA: Record this runner's BFS layer for global re-init
				cellData[2 * rs.attrValue] = rs.attrType;
				cellData[2 * rs.attrValue + 1] = rs.attrValue;
			}
		}
	}

	// IDA: Re-initialize BFS grids for all affected layers
	for (int16 k = 0; k < _gridType; k++) {
		if (cellData[2 * k] != 0)
			initBFSGrid(cellData[2 * k + 1], static_cast<LillyPadAttrType>(cellData[2 * k]));
	}

	_bCellSelectActive = false;
}

void ZoombiniPuzzleLilly::processCellSwapAnimation() {
	// IDA: maze_cellSwapAnimTick (0x4272B4) + maze_tickCellAnimFrame (0x42720F)
	// Called from onEveryFrame when _cellSelectState == 6.
	// First frame (blinkFrame==0): perform swap + re-render cells.
	// Then blink for _swapBlinkMax frames.
	// When done: reset to state 4 (awaiting next cell selection).

	if (_swapBlinkFrame == 0) {
		// IDA at 0x4272D3: First call — perform actual swap
		swapCellsAndUpdateRunners(_swapCellACol, _swapCellARow, _swapCellBCol, _swapCellBRow);
		// IDA: re-render both cells (in ScummVM, the grid renderer handles this automatically)
	}

	if (_swapBlinkFrame < _swapBlinkMax) {
		// IDA: Blink animation phase
		_swapBlinkFrame++;
		// IDA: toggle byte_4A17E8/4A17E9 (0/1) for visual blink
		// In ScummVM, the cellAnimRunnerA/B features handle the visual blink
		// as long as they remain render-activated
	} else {
		// IDA at 0x427311: Animation done — reset state
		_swapBlinkFrame = 0;
		_cellSelectState = 4;
		// Deactivate cell blink runners
		if (_cellAnimRunnerA)
			_cellAnimRunnerA->deactivateRender();
		if (_cellAnimRunnerB)
			_cellAnimRunnerB->deactivateRender();
	}
}

void ZoombiniPuzzleLilly::initCellRunnerPosition(int16 col, int16 row, ZmbFeature *feature) {
	// IDA: maze_initCellRunnerPos (0x429222)
	// Deactivates rendering and sets position for a cell anim runner.
	if (!feature)
		return;

	feature->deactivateRender();

	if (col >= 0 && col < 13 && row >= 0 && row < 12) {
		Common::Point cellPos = _gridCellPos[row][col];
		feature->setPointLoc(cellPos);
		feature->setClickRect(_gridCellRect[row][col]);
	}
}

// =================================================================
// BFS Obstacle Pathfinding
// =================================================================

byte ZoombiniPuzzleLilly::getGridAttrByType(LillyPadAttrType attrType, int16 row0, int16 col) const {
	switch (attrType) {
	case kLillyPadAttrPattern: return _gridAttr1[row0][col];
	case kLillyPadAttrShape: return _gridAttr2[row0][col];
	case kLillyPadAttrColor: return _gridAttr3[row0][col];
	default: return 0;
	}
}

void ZoombiniPuzzleLilly::bfsExpandCell(int16 col, int16 row1, int16 attrValue, LillyPadAttrType attrType) {
	// IDA: maze_bfsExpandCell (0x42971D)
	// Expand one cell in 4 directions. row1 is 1-based.
	// Dir 0=up(row-1, record dir→2), 1=right(col+1, dir→3), 2=down(row+1, dir→0), 3=left(col-1, dir→1).

	int16 srcIdx = kBFSEntriesPerLayer * attrValue + 13 * row1 + col;
	if (srcIdx < 0 || srcIdx >= kMaxBFSEntries)
		return;
	if (!_bfsVisited[srcIdx])
		return;

	for (int16 dir = 0; dir < 4; dir++) {
		bool valid = true;
		int16 nc = col;    // neighbor column
		int16 nr = row1;   // neighbor row (1-based)
		int16 recordDir = 0;

		switch (dir) {
		case 0: // up: row - 1, record dir = 2
			nr = row1 - 1;
			if (nr < 1) {
				nr = row1;
				valid = false;
			}
			recordDir = 2;
			break;
		case 1: // right: col + 1, record dir = 3
			nc = col + 1;
			if (nc > 11) {
				nc = col;
				valid = false;
			}
			recordDir = 3;
			break;
		case 2: // down: row + 1, record dir = 0
			nr = row1 + 1;
			if (nr > 12) {
				nr = row1;
				valid = false;
			}
			recordDir = 0;
			break;
		case 3: // left: col - 1, record dir = 1
			nc = col - 1;
			if (nc < 0) {
				nc = col;
				valid = false;
			}
			recordDir = 1;
			break;
		}

		if (!valid)
			continue;

		// Check: attr matches AND cell not yet visited
		int16 nRow0 = nr - 1; // convert 1-based to 0-based for grid arrays
		if (nRow0 < 0 || nRow0 >= 12 || nc < 0 || nc >= 12)
			continue;

		if (getGridAttrByType(attrType, nRow0, nc) != attrValue)
			continue;

		int16 nIdx = kBFSEntriesPerLayer * attrValue + 13 * nr + nc;
		if (nIdx < 0 || nIdx >= kMaxBFSEntries)
			continue;
		if (_bfsVisited[nIdx])
			continue;

		// Enqueue
		if (_bfsQueueHead < kBFSQueueMax) {
			_bfsQueueCol[_bfsQueueHead] = nc;
			_bfsQueueRow[_bfsQueueHead] = nr;
			_bfsQueueHead++;
		}

		// Record direction and distance
		_bfsDirection[nIdx] = recordDir;
		_bfsDistance[nIdx] = _bfsDistance[srcIdx] + 1;
		_bfsVisited[nIdx] = _bfsVisited[srcIdx];
	}
}

void ZoombiniPuzzleLilly::initBFSGrid(int16 attrValue, LillyPadAttrType attrType) {
	// IDA: maze_initBFSGrid (0x4294EB)
	// Initialize BFS arrays for one layer (one attr value).
	// Seeds from grid cells where the attribute matches attrValue.
	// Rows are 1-based (1..12) in BFS arrays; convert to 0-based for grid access.

	// --- Phase 1: Clear arrays for this layer ---
	for (int16 row1 = 0; row1 < 13; row1++) {
		for (int16 col = 0; col < 12; col++) {
			int16 idx = kBFSEntriesPerLayer * attrValue + 13 * row1 + col;
			if (idx >= 0 && idx < kMaxBFSEntries) {
				_bfsVisited[idx] = 0;
				_bfsDirection[idx] = 44; // unvisited marker
				_bfsDistance[idx] = 0;
			}
		}
	}

	// --- Phase 2: Reset queue ---
	_bfsQueueHead = 0;
	_bfsQueueTail = 0;
	memset(_bfsQueueCol, 0, sizeof(_bfsQueueCol));
	memset(_bfsQueueRow, 0, sizeof(_bfsQueueRow));

	// --- Phase 3: Seed from grid cells matching attrValue, iterate rows 12..1 ---
	for (int16 row1 = 12; row1 >= 1; row1--) {
		// Seed matching cells in this row
		for (int16 col = 0; col < 12; col++) {
			int16 row0 = row1 - 1; // 0-based for grid arrays

			if (getGridAttrByType(attrType, row0, col) != attrValue)
				continue;

			int16 idx = kBFSEntriesPerLayer * attrValue + 13 * row1 + col;
			if (idx < 0 || idx >= kMaxBFSEntries)
				continue;
			if (_bfsVisited[idx])
				continue;

			// Enqueue
			if (_bfsQueueHead < kBFSQueueMax) {
				_bfsQueueCol[_bfsQueueHead] = col;
				_bfsQueueRow[_bfsQueueHead] = row1;
				_bfsQueueHead++;
			}

			// IDA: direction = 2 for seed cells (pointing "down" = toward exit)
			_bfsDirection[idx] = 2;
			_bfsDistance[idx]++;
			_bfsVisited[idx] = row1; // visited marker = source row
		}

		// Expand queued cells that belong to this row pass
		for (int16 qi = _bfsQueueTail; qi < _bfsQueueHead && _bfsQueueTail < kBFSQueueMax && qi < kBFSQueueMax; qi++) {
			int16 qIdx = kBFSEntriesPerLayer * attrValue + 13 * _bfsQueueRow[qi] + _bfsQueueCol[qi];
			if (qIdx >= 0 && qIdx < kMaxBFSEntries && _bfsVisited[qIdx]) {
				bfsExpandCell(_bfsQueueCol[qi], _bfsQueueRow[qi], attrValue, attrType);
				_bfsQueueTail++;
			}
		}
	}
}

void ZoombiniPuzzleLilly::spawnObstacleRunner() {
	// IDA: Obstacle spawning at 0x424007 in lilly_mainFrameUpdate.
	// Two branches based on whether the target cell is already occupied.
	// Both create a feature via maze_registerObstacleRunner (0x4267AF) with SCRB 10067.
	if (_activeObstacleCount >= 20)
		return;

	// IDA: Read entry column from interleaved obstacle entry table.
	// word_4AE36E[4 * nextObstacleIdx] = obstacle entry column.
	if (_nextObstacleIdx >= _obstacleEntryCount)
		return;

	byte entryCol = static_cast<byte>(_obstacleEntryCols[_nextObstacleIdx]);

	// IDA: Create new obstacle feature with SCRB 10067
	ZmbFeature *obstacle = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 10067, 8,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_04000000_OVERLAY);

	if (!obstacle)
		return;

	// Assign to obstacle runner slot
	int16 runnerIdx = _totalZmbCount + _obstacleRunnerCount;
	if (runnerIdx >= kMaxRunners) {
		obstacle->deactivateRender();
		return;
	}

	_zmbRunners[runnerIdx] = obstacle;
	_obstacleRunners[_obstacleRunnerCount] = runnerIdx;
	_activeObstacles[_activeObstacleCount] = runnerIdx;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];
	rs.clear();
	rs.isObstacle = true;
	rs.placed = true;

	// IDA: Set grid position. obstRow(+243) = entry column, obstCol(+244) = 0.
	rs.obstRow = entryCol;
	rs.obstCol = 0;
	rs.entryPointIdx = _nextObstacleIdx;

	// IDA: Check if target cell is occupied
	if (_gridOccupancy[rs.obstCol][rs.obstRow] != 0) {
		// === BRANCH 2: Cell occupied — reuse mode (advanceMode=1) ===
		// IDA: runner+98 = 1 (advanceMode), runner+100 = 0 (bfsReinitFlag)
		rs.advanceMode = 1;
		rs.bfsReinitFlag = 0;

		// IDA: Set obstacle placement marker and visit grid
		rs.direction = 1; // runner+242 = 1

		// IDA: Set visit grid at starting cell
		if (rs.obstCol < 12 && rs.obstRow < 13)
			rs.visitGrid[rs.obstCol][rs.obstRow] = 1;

		// IDA: Compute pixel position
		if (rs.obstCol < 12 && rs.obstRow < 13) {
			int16 posX = 35 * rs.obstRow + 2;
			if (rs.obstCol + 1 < static_cast<int16>(_regsXTable.size()))
				posX += _regsXTable[rs.obstCol + 1];
			int16 posY = kColYOffset[rs.obstRow] - 17;
			if (rs.obstCol + 1 < static_cast<int16>(_regsYTable.size()))
				posY += _regsYTable[rs.obstCol + 1];
			obstacle->setPointLoc(Common::Point(posX, posY));
		}
	} else {
		// === BRANCH 1: Cell not occupied — fresh BFS mode (advanceMode=0) ===
		// IDA: runner+98 = 0 (advanceMode), runner+100 = 0 (bfsReinitFlag)
		rs.advanceMode = 0;
		rs.bfsReinitFlag = 0;

		// IDA: runner+270 = word_4AE370 (obstacle attr type)
		rs.attrType = _obstacleAttrType;

		// IDA: runner+271 = grid attr value at [obstCol][obstRow] for attrType
		switch (rs.attrType) {
		case kLillyPadAttrPattern: rs.attrValue = _gridAttr1[rs.obstCol][rs.obstRow]; break;
		case kLillyPadAttrShape: rs.attrValue = _gridAttr2[rs.obstCol][rs.obstRow]; break;
		case kLillyPadAttrColor: rs.attrValue = _gridAttr3[rs.obstCol][rs.obstRow]; break;
		default: rs.attrValue = 0; break;
		}

		// IDA: runner+272 = attrValue + kPatternAttrExtra[kObstacleBFSOffset[attrType]]
		if (static_cast<byte>(rs.attrType) < 5) {
			int16 offset = kObstacleBFSOffset[static_cast<byte>(rs.attrType)];
			if (offset >= 0 && offset < 13)
				rs.obsCombinedAttr = static_cast<byte>(rs.attrValue + kPatternAttrExtra[offset]);
		}

		// IDA: Initialize BFS path
		initRunnerBFSPath(runnerIdx);

		// IDA: Mark cell occupied
		_gridOccupancy[rs.obstCol][rs.obstRow] = 1;

		// IDA: Set direction and visit grid flags
		rs.direction = 1; // runner+242 = 1

		if (rs.obstCol < 12 && rs.obstRow < 13)
			rs.visitGrid[rs.obstCol][rs.obstRow] = 1;

		// IDA: Mark obstacle placement grid
		_obstacleGrid[rs.obstCol][rs.obstRow] = 1;

		// IDA: Compute pixel position
		if (rs.obstCol < 12 && rs.obstRow < 13) {
			int16 posX = 35 * rs.obstRow + 2;
			if (rs.obstCol + 1 < static_cast<int16>(_regsXTable.size()))
				posX += _regsXTable[rs.obstCol + 1];
			int16 posY = kColYOffset[rs.obstRow] - 17;
			if (rs.obstCol + 1 < static_cast<int16>(_regsYTable.size()))
				posY += _regsYTable[rs.obstCol + 1];
			obstacle->setPointLoc(Common::Point(posX, posY));
		}
	}

	++_activeObstacleCount;
	++_obstacleRunnerCount;

	// IDA: Load SCRB 10067 on obstacle and set callbacks
	rs.callbackMode = kCBLillyMoveStep;
	loadScrbOntoFeature(obstacle, 10067);

	// IDA: Set next obstacle timer = current + 480 frames
	_nextObstacleTimer = getCurrentFrameCounter() + 480;

	// IDA: Advance nextObstacleIdx with wraparound
	if (++_nextObstacleIdx >= _obstacleEntryCount)
		_nextObstacleIdx = 0;
}

uint16 ZoombiniPuzzleLilly::advanceObstacleForwardStep(int16 runnerIdx) {
	// IDA: maze_zmbAdvanceForwardStep (0x42A485) — 377 bytes.
	// Obstacle advance: always tries to move one column to the RIGHT (obstCol++).
	// Returns: 10073 (normal advance), 10069 (exit boundary), 0 (blocked/pending).
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return 0;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	// IDA: If bfsReinitFlag set, push to pathInitQueue, reset mode, return 0
	if (rs.bfsReinitFlag != 0) {
		if (_pathInitQueueSize < kMaxQueueSize)
			_pathInitQueue[_pathInitQueueSize++] = runnerIdx;
		rs.advanceMode = 0;
		rs.bfsReinitFlag = 0;
		return 0;
	}

	// IDA: Read current position. obstCol(+244) = grid row, obstRow(+243) = grid col.
	byte curCol = rs.obstCol;  // grid row
	byte curRow = rs.obstRow;  // grid col
	byte nextCol = curCol + 1; // try next row
	bool atBoundary = false;

	if (nextCol > 11) {
		nextCol = 11;
		atBoundary = true;
	}

	if (!atBoundary) {
		// IDA: Check occupancy at target cell [nextCol][curRow]
		if (_gridOccupancy[nextCol][curRow] != 0) {
			// Cell is occupied — check if there's an obstacle placement marker
			if (_obstacleGrid[nextCol][curRow] == 0)
				return 0; // No obstacle marker → blocked, can't advance
			// Has obstacle marker → fall through to advance (overlapping allowed)
		} else {
			// Cell is NOT occupied — set obstacle attributes from grid
			// IDA: runner+270 = word_4AE370 (global obstacle attr type)
			rs.attrType = _obstacleAttrType;

			// IDA: runner+271 = grid attr value at target cell for this type
			switch (rs.attrType) {
			case kLillyPadAttrPattern: rs.attrValue = _gridAttr1[nextCol][curRow]; break;
			case kLillyPadAttrShape: rs.attrValue = _gridAttr2[nextCol][curRow]; break;
			case kLillyPadAttrColor: rs.attrValue = _gridAttr3[nextCol][curRow]; break;
			default: rs.attrValue = 0; break;
			}

			// IDA: runner+272 = attrValue + kPatternAttrExtra[kObstacleBFSOffset[attrType]]
			if (static_cast<byte>(rs.attrType) < 5) {
				int16 bfsOffset = kObstacleBFSOffset[static_cast<byte>(rs.attrType)];
				if (bfsOffset >= 0 && bfsOffset < 13)
					rs.obsCombinedAttr = static_cast<byte>(rs.attrValue + kPatternAttrExtra[bfsOffset]);
			}

			// IDA: Mark target in visit grid
			if (nextCol < 12 && curRow < 13)
				rs.visitGrid[nextCol][curRow] = 1;

			// IDA: Set pending BFS reinit
			rs.bfsReinitFlag = 1;
		}
	}

	if (atBoundary) {
		// IDA: Exit boundary — set SCRB 10069
		rs.scrbOrDirKey = 10069;
		return 10069;
	}

	// IDA: Mark target cell occupied and return forward move SCRB
	_gridOccupancy[nextCol][curRow] = 1;
	rs.scrbOrDirKey = 10073;
	return 10073;
}

uint16 ZoombiniPuzzleLilly::advanceObstaclePathStepAlt(int16 runnerIdx) {
	// IDA: fleens_advancePathStepAlt (0x42A1E6) — 657 bytes.
	// Tries 4 directions from current position. Each iteration resets test coordinates.
	// Uses 1-based row indexing (matching BFS arrays).
	// Dir 0: backward (row-1), Dir 1: down (col+1), Dir 2: forward (row+1), Dir 3: up (col-1)
	// Selects first direction where: in-bounds, unoccupied, attr matches, BFS distance decreases.
	if (runnerIdx < 0 || runnerIdx >= kMaxRunners)
		return 0;

	ZmbLillyRunnerState &rs = _runnerStates[runnerIdx];

	bool exitReached = false;
	int16 bestDir = 5; // 5 = no valid direction found
	int16 markRow0 = 0; // 0-based row for occupancy marking (IDA: v14)
	int16 markCol = 0;  // column for occupancy marking (IDA: v15)
	byte savedRow = rs.obstRow; // save original obstRow (grid column). IDA: v13

	// IDA: Current cell BFS index uses 1-based row (obstCol+1).
	// unk_4ACE82 = unk_4ACE68 + 13 words, so current index = 507*attrValue + 13*(obstCol+1) + obstRow.
	int16 curBfsIdx = kBFSEntriesPerLayer * rs.attrValue + 13 * (rs.obstCol + 1) + rs.obstRow;

	for (int dir = 0; dir < 4 && !exitReached; dir++) {
		// IDA: Reset test coordinates each iteration
		int16 testRow1 = rs.obstCol + 1; // 1-based row (grid row)
		int16 testCol = rs.obstRow;       // column (grid col)
		bool valid = true;

		switch (dir) {
		case 0: // Backward: row-1
			testRow1--;
			if (testRow1 < 1) {
				testRow1 = 1;
				valid = false;
			}
			break;
		case 1: // Down: col+1
			testCol = savedRow + 1;
			if (testCol > 11) {
				testCol = 11;
				valid = false;
			}
			break;
		case 2: // Forward: row+1 (or exit if past boundary)
			testRow1++;
			if (testRow1 > 12) {
				testRow1 = 12;
				valid = false;
				exitReached = true;
			}
			break;
		case 3: // Up: col-1
			testCol = savedRow - 1;
			if (testCol < 0) {
				testCol = 0;
				valid = false;
			}
			break;
		}

		if (!valid)
			continue;

		// Convert 1-based row to 0-based for grid array access
		int16 testRow0 = testRow1 - 1;
		if (testRow0 < 0 || testRow0 >= 12 || testCol < 0 || testCol >= 12)
			continue;

		// IDA: Check target cell: not occupied
		if (_gridOccupancy[testRow0][testCol] != 0)
			continue;

		// IDA: Check attribute matches
		if (getGridAttrByType(rs.attrType, testRow0, testCol) != rs.attrValue)
			continue;

		// IDA: BFS distance check — target distance < current distance.
		int16 tgtBfsIdx = kBFSEntriesPerLayer * rs.attrValue + 13 * testRow1 + testCol;
		if (tgtBfsIdx >= 0 && tgtBfsIdx < kMaxBFSEntries &&
		    curBfsIdx >= 0 && curBfsIdx < kMaxBFSEntries) {
			if (_bfsDistance[tgtBfsIdx] < _bfsDistance[curBfsIdx]) {
				bestDir = dir;
				markRow0 = testRow0;  // 0-based row for marking
				markCol = testCol;
				break; // IDA: sets v4 = 4 to exit loop on first valid
			}
		}
	}

	if (exitReached) {
		rs.scrbOrDirKey = 10069;
		return 10069;
	}

	if (bestDir >= 4)
		return 0; // No valid move

	// IDA: Direction -> SCRB mapping + mark target cell occupied.
	// Dir 0=backward: SCRB 10071, Dir 1=down: SCRB 10077,
	// Dir 2=forward: SCRB 10073, Dir 3=up: SCRB 10075
	static const uint16 kObsDirScrb[4] = {10071, 10077, 10073, 10075};
	rs.dirByte = static_cast<byte>(bestDir);
	rs.scrbOrDirKey = kObsDirScrb[bestDir];

	// IDA: Mark target cell occupied.
	// lilly_zmbRunnerIdxArr_4AC684[169 * v14 + 13 * v15] = 1
	if (markRow0 >= 0 && markRow0 < 12 && markCol >= 0 && markCol < 13)
		_gridOccupancy[markRow0][markCol] = 1;

	return kObsDirScrb[bestDir];
}

} // End of namespace Mohawk
