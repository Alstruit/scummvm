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
#include "mohawk/zoombini_pages/hotel.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions from 0x4A13E4 (20 POINTS)
const Common::Point ZoombiniInteractiveHotel::kSnoidPositions[20] = {
	Common::Point(455, 423), Common::Point(432, 421), Common::Point(412, 420), Common::Point(395, 425),
	Common::Point(379, 418), Common::Point(365, 433), Common::Point(352, 412), Common::Point(340, 433),
	Common::Point(328, 418), Common::Point(314, 432), Common::Point(295, 421), Common::Point(279, 430),
	Common::Point(264, 437), Common::Point(259, 421), Common::Point(244, 432), Common::Point(226, 421),
	Common::Point(211, 427), Common::Point(195, 419), Common::Point(176, 423), Common::Point(158, 431),
};

ZoombiniInteractiveHotel::ZoombiniInteractiveHotel(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kHotel) {
}

ZoombiniInteractiveHotel::~ZoombiniInteractiveHotel() {
}

void ZoombiniInteractiveHotel::open() {
	openArchive(ZMB_MHK_HOTEL);
}

void ZoombiniInteractiveHotel::setBackgroundMusic() {
	// IDA: hotel_initAndSetupPuzzle (0x41ede4) has no music playback call on page load.
	// sound_activeHandle = 20081 is stored at end of funcInit for F1 replay only.
}

void ZoombiniInteractiveHotel::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniInteractiveHotel::loadFeatures() {
	// IDA: hotel_initAndSetupPuzzle (0x41ede4)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// IDA 0x41ef17-0x41ef46: Initialize maxStepsPerRound based on difficulty
	// Level 0: 5, Level 1: 2, Level 2: 4, Level 3: 2
	switch (_difficultyLevel) {
	case 0:
		_maxStepsPerRound = 5;
		break;
	case 2:
		_maxStepsPerRound = 4;
		break;
	default: // Levels 1 and 3
		_maxStepsPerRound = 2;
		break;
	}
	_stepCounter = 1;
	debugC(kZmbDebugPage, "Hotel: difficultyLevel=%d, maxStepsPerRound=%d",
	       _difficultyLevel, _maxStepsPerRound);

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(0x64u)
	loadTerrainBitmap(100);

	// Preload shape images — main shapes at tBMP 8000
	// IDA: shape_loadSubShapesFromArchive(&stru_4AB7CC, 0x1F40u)
	_vm->_gfx->preloadImage(8000);
	_vm->_gfx->preloadImage(7000);
	_vm->_gfx->preloadImage(7500);
	_vm->_gfx->preloadImage(10000);
	_vm->_gfx->preloadImage(11500);
	_vm->_gfx->preloadImage(11800);

	// Level-dependent extra shapes
	if (_difficultyLevel == 2) {
		// IDA: shape_loadSubShapesFromArchive(stru_4AB7CC, 0x2AF8u) — tBMP 11000
		_vm->_gfx->preloadImage(11000);
	}
	if (_difficultyLevel == 3) {
		// IDA: shape_loadSubShapesFromArchive(stru_4AB7CC, 0x2EE0u) — tBMP 12000
		_vm->_gfx->preloadImage(12000);
	}

	// Feature groups — main SCRB depends on difficulty
	if (_difficultyLevel == 3) {
		// IDA: scrb_useFeatureGroup(0, 0, 9000)
		// IDA: scrb_loadMainFeatureSet(12, 9000)
	} else {
		// IDA: scrb_useFeatureGroup(0, 0, 6000)
		// IDA: scrb_loadMainFeatureSet(88, 6000)
	}
	// IDA: scrb_useFeatureGroup(0, 1, 7000)
	// IDA: scrb_useFeatureGroup(0, 2, 10000)
	// IDA: scrb_useFeatureGroup(0, 3, 11500)
	// IDA: scrb_useFeatureGroup(0, 4, 11800)
	// IDA: scrb_useFeatureGroup(0, 5, 7500) — not at diff 3

	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(2, 11, 0x1B58) — 11 subs at 7000
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 11; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 7000), 7000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet — subs at 10000 (25 or 125 depending on diff)
	{
		uint16 subCount = (_difficultyLevel == 3) ? 125 : 25;
		uint16 subStart = (_difficultyLevel == 3) ? 10025 : 10000;
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < subCount; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 10000), subStart + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 6, 0x2CEC) — 6 subs at 11500
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 6; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 11500), 11500 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 1, 0x2E18) — 1 sub at 11800
	{
		ZmbFeature *parent = mainFeature;
		parent = loadSubFeature(parent,
			ZmbResource(ZmbArchiveKind::kPage, 11800), 11800);
	}

	// IDA: scrb_loadSubFeatureSet(2, 10, 0x1D4C) — 10 subs at 7500 (not at diff 3)
	if (_difficultyLevel != 3) {
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 10; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 7500), 7500 + i);
		}
	}

	// Load reject/normal pools — different sets at diff 3
	if (_difficultyLevel >= 3) {
		// IDA: scrs_loadRejectPool(5, 25, 14025)
		for (uint16 i = 0; i < 25; i++) {
			loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 8000),
					  14025 + i,
					  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
		}
		// IDA: scrs_loadNormalPool(5, 45, 13025)
		for (uint16 i = 0; i < 45; i++) {
			loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 8000),
					  13025 + i,
					  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
		}
	} else {
		// IDA: scrs_loadRejectPool(5, 25, 14000)
		for (uint16 i = 0; i < 25; i++) {
			loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 8000),
					  14000 + i,
					  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
		}
		// IDA: scrs_loadNormalPool(5, 70, 13000)
		for (uint16 i = 0; i < 70; i++) {
			loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 8000),
					  13000 + i,
					  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
		}
	}

	// --- Puzzle-specific feature runners ---

	// NOTE: IDA references "introScrb+5750" but SCRB 5750 does not exist in HOTEL.MHK.
	// The archive starts at SCRB 6000. Hotel puzzle appears to skip a dedicated intro
	// animation feature. Setup is triggered directly via _bBatchWalkDone flag.

	// IDA 0x41f294: getDifficultyIdFromPuzzleFlag increments the page flag.
	// Must be called before room anim type selection (which reads the updated flag).
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagHotel);

	// IDA: word_4AB742 = runner_registerAndAllocate(..., 6, roomAnimType+7000, standard, standard, 0x8108000)
	// Room animation runner — SCRB depends on difficulty and puzzle flags
	{
		int16 roomAnimType = 0;
		switch (_difficultyLevel) {
		case 1: roomAnimType = 4; break;
		case 2: roomAnimType = 5; break;
		case 3: roomAnimType = 6; break;
		default: {
			// IDA 0x41f2c3: diff=0, type 0 on first play, random on subsequent plays
			uint16 hotelPF = _vm->_state->_f._pageFlagHotel;
			if ((hotelPF & ZMB_PAGE_MASK_0FFF) > 1)
				roomAnimType = _vm->_rnd->getRandomNumber(1, (hotelPF & ZMB_PAGE_MASK_0FFF) - 1);
			break;
		}
		}
		_roomAnimFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 7000), 7000 + roomAnimType, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_08000000_REGION_TRACK);
	}

	// IDA: word_4AB752 = runner_registerAndAllocate(..., 6, 0x2E18, standard, standard, 0x100000)
	// Room SCRB runner (SCRB 11800)
	_roomScrbFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11800), 11800, 6,
		ZmbFeature::FLAG_00100000_PLAY_ONCE);

	// Set total room count based on difficulty
	_totalRoomCount = (_difficultyLevel == 3) ? 125 : 25;

	// Load Zoombinis from active pack at 20 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, posData, 20)
	loadZoombinisFromPack();

	// NOTE: Hotel does not call zmb_layoutStaticAndWalkInGroups
	// It positions Zoombinis differently

	// Generate room rules immediately (IDA: sub_4209AA called during hotel_initAndSetupPuzzle)
	computeTraitVariantCounts();
	generateRoomRules();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(8000);
	loadHelpButtonFeature();

	// IDA: sound_activeHandle = 20081 — hotel narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20081);

	// Trigger game board setup on first frame (no intro animation exists in hotel archive)
	_bBatchWalkDone = true;
}

void ZoombiniInteractiveHotel::onGoButtonActivated() {
	// IDA: hotel_onClickHandler case 2
	// Stop BGM before departure, play SFX 996, fade out when SFX finishes.
	// IDA: scrb_enqueueSoundResource(0, 0) — stop background music
	_vm->_sound->stopAllSoundQueues();

	_departXferSrcSiPage = ZMB_SI_HOTEL_11;
	ZoombiniInteractive::onGoButtonActivated();
}

void ZoombiniInteractiveHotel::loadZoombinisFromPack() {
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
}

// ---------------------------------------------------------------------------
// computeTraitVariantCounts: Count distinct trait values per axis.
// IDA: picker_countAttrVariants_421919
// ---------------------------------------------------------------------------
void ZoombiniInteractiveHotel::computeTraitVariantCounts() {
	const ZmbStateFile &f = _vm->_state->_f;
	_totalZmbCount = 0;
	memset(_traitVariantCounts, 0, sizeof(_traitVariantCounts));

	// attrCounts[axisIdx][value] = how many zmbs have that value on that axis
	// Axis order (packed DWORD byte order): 0=foot, 1=nose, 2=eye, 3=head
	uint8 attrCounts[4][6] = {};

	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount; i++) {
		const ZmbStateActiveEntry &entry = f._zmbPackActive._entries[i];
		if (!entry._bIsOccupied)
			continue;

		byte axisVals[4] = {
			entry._traits._foot,   // axis 0
			entry._traits._nose,   // axis 1
			entry._traits._eye,    // axis 2
			entry._traits._head,   // axis 3
		};
		for (int j = 0; j < 4; j++) {
			if (axisVals[j] >= 1 && axisVals[j] <= 5)
				attrCounts[j][axisVals[j]]++;
		}
		_totalZmbCount++;
	}

	for (int j = 0; j < 4; j++) {
		for (int v = 1; v <= 5; v++) {
			if (attrCounts[j][v])
				_traitVariantCounts[j]++;
		}
	}

	debugC(kZmbDebugPage, "Hotel: totalZmbCount=%d, variantCounts=[%d,%d,%d,%d]",
	       _totalZmbCount, _traitVariantCounts[0], _traitVariantCounts[1],
	       _traitVariantCounts[2], _traitVariantCounts[3]);
}

// ---------------------------------------------------------------------------
// generateRoomRules: Pick random axes and optionally forbidden rooms.
// IDA: sub_4209AA (0x4209AA)
// ---------------------------------------------------------------------------
void ZoombiniInteractiveHotel::generateRoomRules() {
	// Count how many axes have <X variants for axis-selection validation
	auto countLimitedAxes = [&](int threshold) -> int {
		int count = 0;
		for (int i = 0; i < 4; i++)
			if (_traitVariantCounts[i] < threshold)
				count++;
		return count;
	};

	int v4 = 0;
	do {
		_attrAxis1 = _vm->_rnd->getRandomNumber(0, 3);
		_attrAxis2 = _vm->_rnd->getRandomNumber(0, 3);
		_attrAxis3 = _vm->_rnd->getRandomNumber(0, 3);

		if (_difficultyLevel <= 1) {
			// IDA: variant counts must be 5 for both axes; fallback if < 3 axes have full coverage
			int limitedCount = countLimitedAxes(5);
			if (_traitVariantCounts[_attrAxis1] == 5 && _traitVariantCounts[_attrAxis2] == 5 && _attrAxis1 != _attrAxis2) {
				v4 = 1;
			} else if (limitedCount < 3 && _attrAxis1 != _attrAxis2 &&
					   _traitVariantCounts[_attrAxis1] >= 4 && _traitVariantCounts[_attrAxis2] >= 4) {
				v4 = 1;
			} else if (limitedCount >= 3 && _attrAxis1 != _attrAxis2) {
				v4 = 1;
			}
		} else if (_difficultyLevel == 2) {
			int limitedCount = countLimitedAxes(4);
			if (limitedCount >= 3) {
				if (_attrAxis1 != _attrAxis2)
					v4 = 1;
			} else {
				if (_attrAxis1 != _attrAxis2 && _traitVariantCounts[_attrAxis1] >= 4 && _traitVariantCounts[_attrAxis2] >= 4)
					v4 = 1;
			}
		} else { // diff 3: all three axes must be distinct
			if (_attrAxis1 != _attrAxis2 && _attrAxis2 != _attrAxis3 && _attrAxis1 != _attrAxis3)
				v4 = 1;
		}
	} while (!v4);

	debugC(kZmbDebugPage, "Hotel: axes selected: axis1=%d axis2=%d axis3=%d",
	       _attrAxis1, _attrAxis2, _attrAxis3);

	// For diff 2: generate a small number of forbidden rooms.
	// IDA: sub_4209AA picks 1-8 empty slots from the 5×5 grid as obstacles.
	// With a full pack every slot is initially reachable, so the exact IDA logic
	// would find zero "no-match" slots. We approximate by picking a random
	// count (0-3) of random slots as obstacles.
	// TODO: implement exact IDA diff2 forbidden-room generation from sub_4209AA.
	if (_difficultyLevel == 2) {
		// For simplicity: pick 0-3 random rooms as forbidden
		// TODO: replace with exact IDA logic
		int forbiddenCount = _vm->_rnd->getRandomNumber(0, MIN(3, _totalRoomCount / 8));
		for (int fi = 0; fi < forbiddenCount; fi++) {
			int slot = _vm->_rnd->getRandomNumber(0, 24);
			if (_roomGrid[slot] == 0) {
				_roomGrid[slot] = -1;
				_forbiddenRoomIds[fi] = _vm->_rnd->getRandomNumber(0, 2);
			}
		}
	}

	// Reset attribute grids
	memset(_attrGrid1, 0, sizeof(_attrGrid1));
	memset(_attrGrid2, 0, sizeof(_attrGrid2));
	memset(_attrGrid3, 0, sizeof(_attrGrid3));
	_bFirstPlacement = true;
	_stepCounter = 1;
	_placedCount = 0;
	memset(_roomGrid, 0, sizeof(int16) * _totalRoomCount);
	// Re-apply forbidden room marks (diff 2 only — regenerate them if needed after reset)
}

// ---------------------------------------------------------------------------
// setupGameBoard: Called from onEveryFrame when intro animation completes.
// IDA: hotel_onHoverPerFrame priority-3 block (word_4AB77A / word_4AB7C4 branch)
// ---------------------------------------------------------------------------
void ZoombiniInteractiveHotel::setupGameBoard() {
	_setupFrameCount = getCurrentFrameCounter();

	// Deactivate room SCRB runner (used during intro display)
	if (_roomScrbFeature) {
		_roomScrbFeature->deactivateAnimate();
		_roomScrbFeature->deactivateRender();
		_roomScrbFeature = nullptr;
	}

	// Redraw background for main gameplay (IDA: gfx_drawBackgroundFromResId(5001 or 5002))
	int16 bgId = (_difficultyLevel >= 3) ? 5002 : 5001;
	_vm->_gfx->drawBackground(bgId);

	// Register room display runners
	registerDisplayScrbs();

	// Set up guide animation (diff+7500) on the room anim feature
	if (_roomAnimFeature) {
		loadScrbOntoFeature(_roomAnimFeature, (uint16)(7500 + _difficultyLevel));
		_guideAnimPurpose = 1; // prompt
		_guideState = 1;
		_guideComplete = false;
	}

	// For diff 3: reset step counter and play sound
	if (_difficultyLevel == 3) {
		_stepCounter = 1;
		_overflowCounter = 0;
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, 30020 + _difficultyLevel));
	}

	_bPuzzleActive = true;

	debugC(kZmbDebugPage, "Hotel: game board set up, puzzleActive=true");
}

// ---------------------------------------------------------------------------
// registerDisplayScrbs: Register room display and hotspot runners.
// IDA: hotel_registerDisplayScrbs_4203ED (0x4203ED)
// ---------------------------------------------------------------------------
void ZoombiniInteractiveHotel::registerDisplayScrbs() {
	// Free any existing room display features
	for (int i = 0; i < 125; i++) {
		if (_roomDisplayFeatures[i]) {
			_roomDisplayFeatures[i]->deactivateAnimate();
			_roomDisplayFeatures[i]->deactivateRender();
			_roomDisplayFeatures[i] = nullptr;
		}
		if (_forbiddenFeatures[i]) {
			_forbiddenFeatures[i]->deactivateAnimate();
			_forbiddenFeatures[i]->deactivateRender();
			_forbiddenFeatures[i] = nullptr;
		}
	}
	if (_labelFeature) {
		_labelFeature->deactivateAnimate();
		_labelFeature->deactivateRender();
		_labelFeature = nullptr;
	}

	if (_difficultyLevel == 0) {
		// IDA: register rooms only at slots 4, 9, 14, 19, 24 (every 5th)
		for (int jj = 4; jj < _totalRoomCount; jj += 5) {
			_roomDisplayFeatures[jj] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 6000), (uint16)(jj + 6013), 6,
				ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
		}
		// Label SCRB 11504
		_labelFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11500), 11504, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);

	} else if (_difficultyLevel <= 2) {
		// IDA: register all 25 rooms
		for (int m = 0; m < _totalRoomCount; m++) {
			if (_roomGrid[m] != -1) { // skip forbidden
				_roomDisplayFeatures[m] = loadScrbFeature(
					ZmbResource(ZmbArchiveKind::kPage, 6000), (uint16)(m + 6013), 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
			}
		}
		// Register forbidden obstacle runners for diff 2
		if (_difficultyLevel == 2) {
			int fi = 0;
			for (int n = 0; n < _totalRoomCount; n++) {
				if (_roomGrid[n] == -1) {
					uint16 obstScrb = (uint16)(_forbiddenRoomIds[fi++] + 11004);
					_forbiddenFeatures[n] = loadScrbFeature(
						ZmbResource(ZmbArchiveKind::kPage, 11000), obstScrb, 6,
						ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
				}
			}
		}
		// Label SCRB 11503
		_labelFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11500), 11503, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);

	} else { // diff 3
		// IDA: register 125 rooms using column/floor offsets
		for (int i = 0; i < _totalRoomCount; i++) {
			if (_roomGrid[i] != -1) {
				_roomDisplayFeatures[i] = loadScrbFeature(
					ZmbResource(ZmbArchiveKind::kPage, kRoomPositions125[i].x > 0 ? 9000 : 9000),
					(uint16)(i % 5 + 9002), 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
			}
		}
		// Forbidden obstacle runners for diff 3
		int fi = 0;
		for (int k = 0; k < _totalRoomCount; k++) {
			if (_roomGrid[k] == -1) {
				uint16 obstScrb = (uint16)(_forbiddenRoomIds[fi++] + 12000);
				_forbiddenFeatures[k] = loadScrbFeature(
					ZmbResource(ZmbArchiveKind::kPage, 12000), obstScrb, 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
			}
		}
		// Label SCRB 11505
		_labelFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11500), 11505, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY);
	}
}

// ---------------------------------------------------------------------------
// validate2AttrPlacement: Check if a zmb can be placed at slot (diff 0–2).
// IDA: picker_checkAttrFilter_421729 (0x421729)
// Parameters: slot=targetSlot, axis2Val=trait[_attrAxis2], axis1Val=trait[_attrAxis1]
// Returns true = valid placement.
// ---------------------------------------------------------------------------
bool ZoombiniInteractiveHotel::validate2AttrPlacement(int16 slot, int16 axis2Val, int16 axis1Val) const {
	if (_attrBypass)
		return true;

	int16 ax1Constraint = _attrGrid1[slot];
	int16 ax2Constraint = _attrGrid2[slot];

	if (ax1Constraint || ax2Constraint) {
		// Slot has existing constraints
		// Exact match always valid
		if (axis1Val == ax1Constraint && axis2Val == ax2Constraint)
			return true;
		// Must be compatible with both constraints (0 = no constraint on that axis)
		bool ax1ok = (!ax1Constraint || axis1Val == ax1Constraint);
		bool ax2ok = (!ax2Constraint || axis2Val == ax2Constraint);
		if (!ax1ok || !ax2ok)
			return false;
		// Uniqueness check: no other slot may share axis1 or axis2 value
		for (int k = 0; k < _totalRoomCount; k++) {
			if (ax2Constraint && axis2Val == _attrGrid2[k])
				return false;
			if (ax1Constraint && axis1Val == _attrGrid1[k])
				return false;
		}
		return true;
	} else {
		// Slot is empty: pure uniqueness check
		for (int k = 0; k < _totalRoomCount; k++) {
			if (axis2Val && axis2Val == _attrGrid2[k])
				return false;
			if (axis1Val && axis1Val == _attrGrid1[k])
				return false;
		}
		return true;
	}
}

// ---------------------------------------------------------------------------
// validate3AttrPlacement: Check if a zmb can be placed at slot (diff 3).
// IDA: hotel_checkZmbFitsRoom_421E41 (0x421E41)
// slot decomposition: col=slot%5, rowGroup=slot%25/5, floor=slot/25
// Returns true = valid.
// ---------------------------------------------------------------------------
bool ZoombiniInteractiveHotel::validate3AttrPlacement(int16 slot, int16 axis3Val, int16 axis2Val, int16 axis1Val) const {
	if (_attrBypass)
		return true;

	int16 col       = slot % 5;
	int16 rowGroup  = (slot % 25) / 5;
	int16 floor     = slot / 25;

	int16 gRow   = _attrGrid1[rowGroup]; // axis1 for this row-group
	int16 gFloor = _attrGrid2[floor];    // axis2 for this floor
	int16 gCol   = _attrGrid3[col];      // axis3 for this column

	if (gRow || gFloor || gCol) {
		// At least one constraint is set — must satisfy all set constraints
		bool rowOk   = (!gRow   || axis1Val == gRow);
		bool floorOk = (!gFloor || axis2Val == gFloor);
		bool colOk   = (!gCol   || axis3Val == gCol);
		if (!rowOk || !floorOk || !colOk)
			return false;
		// Uniqueness check across the first 5 entries of each grid
		for (int i = 0; i < 5; i++) {
			if (_attrGrid1[i] && axis1Val == _attrGrid1[i]) return false;
			if (_attrGrid2[i] && axis2Val == _attrGrid2[i]) return false;
			if (_attrGrid3[i] && axis3Val == _attrGrid3[i]) return false;
		}
		return true;
	} else {
		// No constraints — uniqueness check only
		for (int i = 0; i < 5; i++) {
			if (_attrGrid1[i] && axis1Val == _attrGrid1[i]) return false;
			if (_attrGrid2[i] && axis2Val == _attrGrid2[i]) return false;
			if (_attrGrid3[i] && axis3Val == _attrGrid3[i]) return false;
		}
		return true;
	}
}

// ---------------------------------------------------------------------------
// fillCellRow: Set axis1/axis2 constraints for an entire row+column.
// IDA: ferry_fillCellRow_4216BC (0x4216BC)
// rowIdx: target slot (0-24); axis2Val: row attribute; axis1Val: col attribute.
// ---------------------------------------------------------------------------
void ZoombiniInteractiveHotel::fillCellRow(int16 rowIdx, int16 axis2Val, int16 axis1Val) {
	for (int k = 0; k < 5; k++) {
		// Fill entire column (all row-groups at position rowIdx%5)
		_attrGrid1[5 * k + rowIdx % 5] = axis1Val;
		// Fill entire row (all columns at the same row-group offset)
		_attrGrid2[k + (rowIdx - rowIdx % 5)] = axis2Val;
	}
}

// ---------------------------------------------------------------------------
// setCellAttrsIn3Grids: Set all three attribute grids for diff-3 placement.
// IDA: maze_setCellAttrsInGrids_422197 (0x422197)
// ---------------------------------------------------------------------------
void ZoombiniInteractiveHotel::setCellAttrsIn3Grids(int16 cellIdx, int16 attrType, int16 attrValue, int16 gridLayer) {
	_attrGrid1[cellIdx % 25 / 5] = gridLayer; // axis1 → row-group
	_attrGrid2[cellIdx / 25]     = attrValue;  // axis2 → floor
	_attrGrid3[cellIdx % 5]      = attrType;   // axis3 → column
}

// ---------------------------------------------------------------------------
// placeZoombiniInRoom: Animate a zoombini entering an assigned room slot.
// IDA: hotel_setupRoomSlotScrb_422534 (0x422534)
// ---------------------------------------------------------------------------
void ZoombiniInteractiveHotel::placeZoombiniInRoom(int16 roomSlot, ZmbSnoid *snoid) {
	const Common::Point *posTable = (_difficultyLevel == 3) ? kRoomPositions125 : kRoomPositions25;
	Common::Point basePos = posTable[roomSlot];
	Common::Point finalPos;
	int16 stacking = _roomGrid[roomSlot]; // depth (0-based from first placed)

	if (_difficultyLevel == 3) {
		// IDA: pInitPos.x += 5; pInitPos.y -= 15; finalPos.x = pInitPos.x - 32; finalPos.y = pInitPos.y
		basePos.x += 5;
		basePos.y -= 15;
		finalPos.x = basePos.x - 32;
		finalPos.y = basePos.y;
		if (stacking > 1) {
			finalPos.x -= 2 * (stacking - 1);
			finalPos.y -= (stacking - 1);
		}
	} else {
		// IDA: pInitPos.x += 24; pInitPos.y -= 7; stacking adjustments
		basePos.x += 24;
		basePos.y -= 7;
		finalPos.y = basePos.y - 2;
		// X offset by column group
		if (roomSlot == 4)
			finalPos.x = basePos.x - 5;
		else if (roomSlot == 9)
			finalPos.x = basePos.x - 7;
		else if (roomSlot >= 10)
			finalPos.x = basePos.x - (3 + ((roomSlot >= 20) ? 2 : 0));
		else
			finalPos.x = basePos.x - 3;
		// Stacking adjustment (1-based depth)
		int16 d = stacking; // already incremented before this call
		if (d % 3 == 0)
			finalPos.x -= 8;
		else if (d % 3 == 2)
			finalPos.x -= 1;
		else
			finalPos.x += 6;
		finalPos.y += d - 1;
	}

	_placedZmbPos = finalPos;

	// Select SCRS ID: eye trait value + base - 1
	// IDA: v7 = SHIBYTE(traitDword) + roomIdx - 1  (SHIBYTE = eye = axis index 2)
	int16 eyeVal = snoid->_trait._eye;
	int16 scrsBase;
	if (_difficultyLevel == 3) {
		scrsBase = (int16)(5 * (roomSlot % 5) + 13045);
	} else {
		if (roomSlot < 10)
			scrsBase = 13030;
		else if (roomSlot < 15)
			scrsBase = 13035;
		else
			scrsBase = 13040;
	}
	uint16 scrsId = (uint16)(eyeVal + scrsBase - 1);

	// Move to initial position (diff 3 uses specified pos; diff 0-2 stays where dropped)
	if (_difficultyLevel == 3)
		snoid->setPointLoc(basePos);

	// Play SCRS normal animation
	Common::SeekableReadStream *scrsStream =
		_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, scrsId));
	if (scrsStream) {
		snoid->startScrsPlayback(scrsStream, false /* hideOnComplete */, false /* NORMAL, not reject */);
		_placedZmbSnoid = snoid;
		_bPlacedZmbAnimDone = false;
	} else {
		// If SCRS resource not available, finalise directly
		snoid->setPointLoc(finalPos);
		if (_difficultyLevel >= 3) {
			snoid->setAnimState(kSnoidAnimIdle);
			snoid->setupIdleHotspots();
		} else {
			snoid->setAnimState(kSnoidAnimDepart);
		}
		_placedZmbSnoid = nullptr;
		_bPlacedZmbAnimDone = false;
		_bInteractionLock = false;
		if (_placedCount >= _totalZmbCount)
			registerWinCheckpoints();
	}
}

// ---------------------------------------------------------------------------
// dimPaletteOnError: Dim palette slightly after a wrong placement.
// IDA: picker_applyBrightnessDim_42185D (0x42185D)
// ---------------------------------------------------------------------------
void ZoombiniInteractiveHotel::dimPaletteOnError() {
	uint8 scalePercent = 92;
	if (_difficultyLevel == 0)
		scalePercent = 88;
	else if (_difficultyLevel == 2)
		scalePercent = 90;
	// Scale palette entries 10..245 (236 entries) — IDA: entries 10..246
	_vm->_gfx->scalePalettePartial(10, 236, scalePercent);
}

// ---------------------------------------------------------------------------
// registerWinCheckpoints: Set up win detection on room hotspot runners.
// IDA: maze_registerCheckpointRunners_422A61 (0x422A61)
// In our system we simply check _placedCount >= _totalZmbCount in onEveryFrame.
// This function serves as a win announcement / SCRB reload trigger.
// ---------------------------------------------------------------------------
void ZoombiniInteractiveHotel::registerWinCheckpoints() {
	// Reload hotspot SCRB i+6063 on all active room hotspot runners
	// IDA: scrb_initRunnerWithScript(0, 0, j+6063, word_4AB54E[j])
	// In our implementation, we trigger win by reloading the guide anim to a cheer SCRB
	// and setting the targetRoomSlot sentinel.
	_targetRoomSlot = 200; // sentinel = "win detected"
}

// ---------------------------------------------------------------------------
// onFeatureAnimEvent: Called when a feature's animation cycle ends.
// ---------------------------------------------------------------------------
void ZoombiniInteractiveHotel::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (eventCode != kZmbAnimEventM1_End)
		return; // Only handle end-of-animation

	if (feature == _roomAnimFeature) {
		switch (_guideAnimPurpose) {
		case 1: // Prompt animation done (guide SCRB 7500+diff)
			_bPromptAnimDone = true;
			_guideComplete = true;
			break;
		case 2: // Cheer animation done
			_bCheerAnimDone = true;
			_guideComplete = true;
			break;
		case 3: // Win animation done
			_bWinAnimDone = true;
			break;
		default:
			break;
		}
		return;
	}

	if (feature == _counterFeature) {
		// IDA: word_4AB784 = 1 (counter hotspot fires)
		_bCounterAnimDone = true;
		return;
	}

	if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
		// A snoid's SCRS animation completed
		ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);
		if (snoid == _placedZmbSnoid) {
			// IDA: word_4AB780 = 1 (placed zmb hotspot fires)
			_bPlacedZmbAnimDone = true;
		} else {
			// Other snoid (reject, etc.): return to idle
			_bRejectAnimActive = false;
			snoid->setAnimState(kSnoidAnimIdle);
			snoid->setupIdleHotspots();
		}
	}
}

// ---------------------------------------------------------------------------
// onEveryFrame: Main per-frame state machine for the hotel puzzle.
// IDA: hotel_onHoverPerFrame_41F6D2 (0x41F6D2)
// ---------------------------------------------------------------------------
void ZoombiniInteractiveHotel::onEveryFrame() {
	if (_bPuzzleComplete)
		return;

	// [Priority 1] Cheer animation done
	if (_bCheerAnimDone) {
		_bCheerAnimDone = false;
		// IDA: play random sound nextRand(v1)+7503 on page SND
		uint16 cheerSnd = (uint16)(_vm->_rnd->getRandomNumber(0, 1) + 7503);
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, cheerSnd));

		// IDA: if diff!=3 && overflowCounter > 0: reload guide with room SCRB
		if (_difficultyLevel != 3 && _overflowCounter > 0) {
			if (_roomAnimFeature) {
				// Reload room display on guide and register win anim hotspot
				loadScrbOntoFeature(_roomAnimFeature, (uint16)(7507 + _vm->_rnd->getRandomNumber(0, 2)));
				_guideAnimPurpose = 3; // win
				_guideComplete = false;
			}
		} else {
			// Trigger win animation done sequence
			_bWinAnimDone = true;
		}
		return;
	}

	// [Priority 2] Win animation done
	if (_bWinAnimDone) {
		_bWinAnimDone = false;
		_bPuzzleComplete = true;
		setGoButtonsEnabled(true);
		debugC(kZmbDebugPage, "Hotel: puzzle complete!");
		return;
	}

	// [Priority 3] Batch walk (intro) done OR out of steps → (re)setup board
	if (_bBatchWalkDone || _bOutOfSteps) {
		_bBatchWalkDone = false;
		_bOutOfSteps = false;
		setupGameBoard();
		return;
	}

	if (!_bPuzzleActive)
		return;

	// [Priority 4a] Prompt (guide) animation done → start counter
	if (_bPromptAnimDone) {
		_bPromptAnimDone = false;
		_guideState = 1;
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, (uint16)(30020 + _difficultyLevel)));

		if (_bOutOfSteps) {
			// Reset step counter; reload counter with full steps
			_bOutOfSteps = false;
			_stepCounter = 1;
			if (_counterFeature) {
				loadScrbOntoFeature(_counterFeature, (uint16)(_maxStepsPerRound + 6000));
				_bCounterAnimDone = false;
			}
		} else if (_difficultyLevel != 3) {
			// Start counter animation (SCRB maxSteps+6000 is the initial/full display)
			if (!_counterFeature) {
				_counterFeature = loadScrbFeature(
					ZmbResource(ZmbArchiveKind::kPage, 6000), (uint16)(_maxStepsPerRound + 6000), 6,
					ZmbFeature::FLAG_00100000_PLAY_ONCE);
			} else {
				loadScrbOntoFeature(_counterFeature, (uint16)(_maxStepsPerRound + 6000));
			}
		}
		return;
	}

	// [Priority 4b] Counter animation step done → advance counter
	if (_bCounterAnimDone && _counterFeature) {
		_bCounterAnimDone = false;
		if (_stepCounter <= _maxStepsPerRound) {
			loadScrbOntoFeature(_counterFeature, (uint16)(_stepCounter + 6000));
			_stepCounter++;
		}
		return;
	}

	// [Priority 4d] Pending placement → animate accept or reject
	if (_pendingPlacementSnoid) {
		ZmbSnoid *snoid = _pendingPlacementSnoid;
		_pendingPlacementSnoid = nullptr;

		if (_pendingAccepted) {
			// --- ACCEPTED PLACEMENT ---
			// IDA: ++hotel_placedCount; update roomGrid depth; placeZoombiniInRoom
			_placedCount++;
			if (_roomGrid[_targetRoomSlot] <= 0)
				_roomGrid[_targetRoomSlot] = 1;
			else
				_roomGrid[_targetRoomSlot] = (int16)MIN<int>(_roomGrid[_targetRoomSlot] + 1, 6);

			// Mark snoid as placed
			_placedSnoidIds.push_back((uint16)snoid->getId());

			placeZoombiniInRoom(_targetRoomSlot, snoid);

			// Play success sound when all zmbs are loaded
			if (_placedCount == _totalZmbCount) {
				uint16 winSnd = (uint16)(175 + _vm->_rnd->getRandomNumber(0, 2));
				_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, winSnd));
			}

		} else {
			// --- REJECTED PLACEMENT ---
			// IDA: hotel_bRejectAnimActive=1; play reject SCRS on snoid; dimPalette
			_bRejectAnimActive = true;

			// Step counter handling
			if (_difficultyLevel >= 3) {
				if (_stepCounter >= 11)
					registerWinCheckpoints();
				_stepCounter++;
			} else if (_stepCounter >= 11) {
				registerWinCheckpoints();
			} else {
				// Reload counter step
				_stepCounter++;
				if (_counterFeature) {
					loadScrbOntoFeature(_counterFeature, (uint16)(_stepCounter + 6000));
				}
			}

			dimPaletteOnError();

			// Play rejection SCRS on the snoid (SCRS from reject pool: 14000+eyeVal-1 etc.)
			// Use a reject SCRS closest to snoid's eye value
			int16 eyeVal = snoid->_trait._eye;
			uint16 rejectScrsBase = (_difficultyLevel >= 3) ? 14025 : 14000;
			uint16 rejectScrsId = (uint16)(rejectScrsBase + (eyeVal - 1));
			Common::SeekableReadStream *rejectStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, rejectScrsId));
			if (rejectStream) {
				snoid->startScrsPlayback(rejectStream, false /* show when done */, true /* reject state */);
			} else {
				// Fallback: just release the reject lock
				_bRejectAnimActive = false;
			}

			// Escalation reactions
			if (_stepCounter == 9) {
				// IDA: play random voice on guide at step 9
				uint16 reactSnd = (uint16)(7503 + _vm->_rnd->getRandomNumber(0, 1));
				_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, reactSnd));
			}
			if (_stepCounter >= 12) {
				_overflowCounter++;
				_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, 6006));
				if (_difficultyLevel >= 3) {
					_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, 7500));
				}
				// Trigger out-of-steps reset
				_bOutOfSteps = true;
			}
		}
		return;
	}

	// [Priority 4e] Placed-zmb SCRS animation done → finalise position + check win
	if (_bPlacedZmbAnimDone) {
		_bPlacedZmbAnimDone = false;
		ZmbSnoid *snoid = _placedZmbSnoid;
		_placedZmbSnoid = nullptr;

		if (snoid) {
			// IDA: if diff3 → kSnoidAnimIdle; else → kSnoidAnimDepart (walk into room)
			if (_difficultyLevel >= 3) {
				snoid->setPointLoc(_placedZmbPos);
				snoid->setAnimState(kSnoidAnimIdle);
				snoid->setupIdleHotspots();
			} else {
				snoid->setAnimState(kSnoidAnimDepart);
			}
		}

		_bInteractionLock = false;

		// Check win condition
		if (_placedCount >= _totalZmbCount) {
			// IDA: hotel_registerWinCheckpoints(); hotel_targetRoomSlot=200;
			// load SCRB random 7507+rand on guide; register win hotspot
			registerWinCheckpoints();
			_targetRoomSlot = 200;

			if (_roomAnimFeature) {
				uint16 cheerScrb = (uint16)(7507 + _vm->_rnd->getRandomNumber(0, 2));
				loadScrbOntoFeature(_roomAnimFeature, cheerScrb);
				_guideAnimPurpose = 2; // cheer
				_guideComplete = false;
			} else {
				// No guide feature: go directly to complete
				_bPuzzleComplete = true;
				setGoButtonsEnabled(true);
			}
		}
		return;
	}

	// [Priority 5] Fidget: if elapsed time > threshold or no guide loaded
	if (!_guideState || (getCurrentFrameCounter() - _setupFrameCount) > 0xB4) {
		if (_roomAnimFeature && _guideState == 0) {
			// Reload guide with prompt animation
			loadScrbOntoFeature(_roomAnimFeature, (uint16)(7500 + _difficultyLevel));
			_guideAnimPurpose = 1;
			_guideState = 1;
			_guideComplete = false;
			_setupFrameCount = getCurrentFrameCounter();
		}
	}
}

// ---------------------------------------------------------------------------
// findSnoidAtPoint: Only return draggable pack snoids (IDs 10000–12999).
// ---------------------------------------------------------------------------
ZmbSnoid *ZoombiniInteractiveHotel::findSnoidAtPoint(const Common::Point &pos) {
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		uint16 id = (*it)->getId();
		// Only pack snoids (10000–12999); skip pool snoids (13000+)
		if (id < 10000 || id >= 13000)
			continue;
		// Skip already-placed snoids
		bool alreadyPlaced = false;
		for (uint32 pi = 0; pi < _placedSnoidIds.size(); pi++) {
			if (_placedSnoidIds[pi] == id) { alreadyPlaced = true; break; }
		}
		if (alreadyPlaced)
			continue;
		ZmbSnoid *snoid = *it;
		if (snoid->findDrawRecordAtPoint(pos))
			return snoid;
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// getDropTargetSlot: Find closest room slot to the drop position.
// ---------------------------------------------------------------------------
int16 ZoombiniInteractiveHotel::getDropTargetSlot(const Common::Point &dropPos) const {
	// IDA: getDropTargetResult_453571 checks registered click-zone runners.
	// We approximate with a proximity check against known room positions.
	const int32 kDropRadiusSq = 55 * 55; // ~55px radius

	if (_difficultyLevel == 3) {
		int16 best = -1;
		int32 bestDist = kDropRadiusSq;
		for (int i = 0; i < 125; i++) {
			int32 dx = dropPos.x - kRoomPositions125[i].x;
			int32 dy = dropPos.y - kRoomPositions125[i].y;
			int32 d = dx * dx + dy * dy;
			if (d < bestDist) { bestDist = d; best = (int16)i; }
		}
		return best;
	} else if (_difficultyLevel == 0) {
		// Only slots 4, 9, 14, 19, 24
		int16 best = -1;
		int32 bestDist = kDropRadiusSq;
		for (int i = 4; i < 25; i += 5) {
			int32 dx = dropPos.x - kRoomPositions25[i].x;
			int32 dy = dropPos.y - kRoomPositions25[i].y;
			int32 d = dx * dx + dy * dy;
			if (d < bestDist) { bestDist = d; best = (int16)i; }
		}
		return best;
	} else {
		// Diff 1–2: all 25 slots
		int16 best = -1;
		int32 bestDist = kDropRadiusSq;
		for (int i = 0; i < 25; i++) {
			int32 dx = dropPos.x - kRoomPositions25[i].x;
			int32 dy = dropPos.y - kRoomPositions25[i].y;
			int32 d = dx * dx + dy * dy;
			if (d < bestDist) { bestDist = d; best = (int16)i; }
		}
		return best;
	}
}

// ---------------------------------------------------------------------------
// getTraitValue: Read trait value for a given axis index.
// IDA: *(char*)(&traitDword + axisIdx) — packed DWORD byte order: 0=foot,1=nose,2=eye,3=head.
// ---------------------------------------------------------------------------
byte ZoombiniInteractiveHotel::getTraitValue(const ZmbTrait &trait, int16 axisIdx) const {
	switch (axisIdx) {
	case 0: return trait._foot;
	case 1: return trait._nose;
	case 2: return trait._eye;
	case 3: return trait._head;
	default: return 0;
	}
}

// ---------------------------------------------------------------------------
// endDrag: Evaluate drop after drag release.
// IDA: hotel_funcOnHover_420DFC case 4 drag evaluation
// ---------------------------------------------------------------------------
void ZoombiniInteractiveHotel::endDrag(const Common::Point &mousePos) {
	ZmbSnoid *snoid = finishSnoidDrag();
	if (!snoid)
		return;

	Common::Point dropPos = snoid->getPointLoc();

	// Guards: don't place during interaction lock or reject animation
	if (_bInteractionLock || _bRejectAnimActive || !_bPuzzleActive) {
		snoid->setPointLoc(_dragOrigPos);
		snoid->setAnimState(kSnoidAnimIdle);
		snoid->setupIdleHotspots();
		return;
	}

	int16 targetSlot = getDropTargetSlot(dropPos);

	// Validate: slot must exist, not be forbidden, not already full
	bool slotOk = (targetSlot >= 0 && targetSlot < _totalRoomCount)
			   && (_roomGrid[targetSlot] >= 0); // -1 = forbidden

	if (!slotOk) {
		// Invalid drop
		snoid->setPointLoc(_dragOrigPos);
		snoid->setAnimState(kSnoidAnimIdle);
		snoid->setupIdleHotspots();
		return;
	}

	_targetRoomSlot = targetSlot;

	// Retrieve trait values for the selected axes
	int16 axis1Val = getTraitValue(snoid->_trait, _attrAxis1);
	int16 axis2Val = getTraitValue(snoid->_trait, _attrAxis2);
	int16 axis3Val = getTraitValue(snoid->_trait, _attrAxis3);

	bool isHovered; // true = conflict/invalid

	if (_bFirstPlacement) {
		// IDA: first placement always valid; set constraints immediately
		_bFirstPlacement = false;
		_stepCounter = _maxStepsPerRound;
		isHovered = false;

		if (_difficultyLevel == 0) {
			_attrGrid1[targetSlot] = axis1Val;
		} else if (_difficultyLevel <= 2) {
			fillCellRow(targetSlot, axis2Val, axis1Val);
		} else {
			setCellAttrsIn3Grids(targetSlot, axis3Val, axis2Val, axis1Val);
		}
	} else {
		// Validate against current constraints
		if (_difficultyLevel == 0) {
			int16 existing = _attrGrid1[targetSlot];
			if (existing) {
				isHovered = (existing != axis1Val);
			} else {
				// Empty slot: check uniqueness across active slots
				isHovered = false;
				for (int i = 4; i < 25; i += 5) {
					if (_attrGrid1[i] && _attrGrid1[i] == axis1Val) {
						isHovered = true;
						break;
					}
				}
			}
		} else if (_difficultyLevel <= 2) {
			isHovered = !validate2AttrPlacement(targetSlot, axis2Val, axis1Val);
			if (!isHovered) {
				fillCellRow(targetSlot, axis2Val, axis1Val);
			}
		} else {
			isHovered = !validate3AttrPlacement(targetSlot, axis3Val, axis2Val, axis1Val);
			if (!isHovered) {
				setCellAttrsIn3Grids(targetSlot, axis3Val, axis2Val, axis1Val);
			}
		}

		// For diff 0: set constraint on valid accept (after validation above)
		if (_difficultyLevel == 0 && !isHovered) {
			_attrGrid1[targetSlot] = axis1Val;
		}
	}

	// Mark placement pending
	_pendingPlacementSnoid = snoid;
	_pendingAccepted = !isHovered;
	_bInteractionLock = true;

	if (!isHovered) {
		// IDA: pcStr1[11]=1; if loaded==total: play sound
		debugC(kZmbDebugPage, "Hotel: accepted placement at slot %d", targetSlot);
	} else {
		// IDA: hotel_bRejectAnimActive=1; word_4AB764=0
		debugC(kZmbDebugPage, "Hotel: rejected placement at slot %d", targetSlot);
	}
}

// ---------------------------------------------------------------------------
// onLButtonDown: Click / drag initiation.
// IDA: hotel_onClickHandler (case 1 mouse-down; case 4 drag logic)
// ---------------------------------------------------------------------------
ZmbEventHandleResult ZoombiniInteractiveHotel::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// Sticky mouse: second click drops the dragged snoid
	if (isDragging() && _vm->_state->getEnableStickyMouse()) {
		endDrag(absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	// Let base handle Go/Map/Help
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Guards
	if (!_bPuzzleActive || _bInteractionLock || _bRejectAnimActive)
		return ZmbEventHandleResult::kPassthrough;
	if (isDragging())
		return ZmbEventHandleResult::kPassthrough;

	ZmbSnoid *snoid = findSnoidAtPoint(absPos);
	if (!snoid)
		return ZmbEventHandleResult::kPassthrough;

	startSnoidDrag(snoid, absPos);
	return ZmbEventHandleResult::kConsumed;
}

// ---------------------------------------------------------------------------
// onLButtonUp: Release drag.
// ---------------------------------------------------------------------------
ZmbEventHandleResult ZoombiniInteractiveHotel::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	if (!isDragging())
		return ZoombiniInteractive::onLButtonUp(absPos, relPos);

	// Sticky mouse: don't drop on button-up
	if (_vm->_state->getEnableStickyMouse())
		return ZmbEventHandleResult::kConsumed;

	endDrag(absPos);
	return ZmbEventHandleResult::kConsumed;
}

// ---------------------------------------------------------------------------
// Static data tables
// ---------------------------------------------------------------------------

// IDA: dword_4A1110[25] — room center positions for diff 0–2 (5 cols × 5 rows)
// Parsed from binary: each entry is (LOWORD=x, HIWORD=y) little-endian int16 pair.
const Common::Point ZoombiniInteractiveHotel::kRoomPositions25[25] = {
	Common::Point(0x87, 0x4e),  // [0]  col0 row0  (135, 78)
	Common::Point(0x8a, 0x8e),  // [1]  col0 row1  (138, 142)
	Common::Point(0x8e, 0xcc),  // [2]  col0 row2  (142, 204)
	Common::Point(0x92, 0x107), // [3]  col0 row3  (146, 263)
	Common::Point(0x95, 0x144), // [4]  col0 row4  (149, 324)
	Common::Point(0xdf, 0x54),  // [5]  col1 row0  (223, 84)
	Common::Point(0xde, 0x93),  // [6]  col1 row1  (222, 147)
	Common::Point(0xe3, 0xd2),  // [7]  col1 row2  (227, 210)
	Common::Point(0xe4, 0x10b), // [8]  col1 row3  (228, 267)
	Common::Point(0xea, 0x148), // [9]  col1 row4  (234, 328)
	Common::Point(0x13b, 0x58), // [10] col2 row0  (315, 88)
	Common::Point(0x137, 0x98), // [11] col2 row1  (311, 152)
	Common::Point(0x139, 0xd5), // [12] col2 row2  (313, 213)
	Common::Point(0x13a, 0x110),// [13] col2 row3  (314, 272)
	Common::Point(0x13a, 0x14d),// [14] col2 row4  (314, 333)
	Common::Point(0x192, 0x5e), // [15] col3 row0  (402, 94)
	Common::Point(0x18e, 0x9d), // [16] col3 row1  (398, 157)
	Common::Point(0x18f, 0xdc), // [17] col3 row2  (399, 220)
	Common::Point(0x18d, 0x115),// [18] col3 row3  (397, 277)
	Common::Point(0x18c, 0x152),// [19] col3 row4  (396, 338)
	Common::Point(0x1eb, 0x64), // [20] col4 row0  (491, 100)
	Common::Point(0x1e9, 0xa4), // [21] col4 row1  (489, 164)
	Common::Point(0x1e9, 0xe2), // [22] col4 row2  (489, 226)
	Common::Point(0x1e8, 0x11c),// [23] col4 row3  (488, 284)
	Common::Point(0x1e5, 0x15a),// [24] col4 row4  (485, 346)
};

// IDA: dword_4A1178[125] — room center positions for diff 3 (5 floors × 5×5 grid)
// 125 entries extracted from binary at 0x4A1178 (500 bytes).
const Common::Point ZoombiniInteractiveHotel::kRoomPositions125[125] = {
	// Floor 0 (entries 0–24)
	Common::Point(0x10, 0x28),  // [0]   (16, 40)
	Common::Point(0x27, 0x32),  // [1]   (39, 50)
	Common::Point(0x3c, 0x36),  // [2]   (60, 54)
	Common::Point(0x56, 0x3a),  // [3]   (86, 58)
	Common::Point(0x6f, 0x3c),  // [4]   (111, 60)
	Common::Point(0x13, 0x73),  // [5]   (19, 115)
	Common::Point(0x2a, 0x7d),  // [6]   (42, 125)
	Common::Point(0x3f, 0x81),  // [7]   (63, 129)
	Common::Point(0x59, 0x85),  // [8]   (89, 133)
	Common::Point(0x72, 0x87),  // [9]   (114, 135)
	Common::Point(0x15, 0xbc),  // [10]  (21, 188)
	Common::Point(0x2c, 0xc6),  // [11]  (44, 198)
	Common::Point(0x41, 0xca),  // [12]  (65, 202)
	Common::Point(0x5b, 0xce),  // [13]  (91, 206)
	Common::Point(0x74, 0xd0),  // [14]  (116, 208)
	Common::Point(0x18, 0x105), // [15]  (24, 261)
	Common::Point(0x2f, 0x10f), // [16]  (47, 271)
	Common::Point(0x44, 0x113), // [17]  (68, 275)
	Common::Point(0x5e, 0x117), // [18]  (94, 279)
	Common::Point(0x77, 0x119), // [19]  (119, 281)
	Common::Point(0x1c, 0x14d), // [20]  (28, 333)
	Common::Point(0x33, 0x157), // [21]  (51, 343)
	Common::Point(0x49, 0x15b), // [22]  (73, 347)
	Common::Point(0x63, 0x15f), // [23]  (99, 351)
	Common::Point(0x7c, 0x161), // [24]  (124, 353)
	// Floor 1 (entries 25–49)
	Common::Point(0x8e, 0x36),  // [25]  (142, 54)
	Common::Point(0xa5, 0x3c),  // [26]  (165, 60)
	Common::Point(0xba, 0x40),  // [27]  (186, 64)
	Common::Point(0xd4, 0x44),  // [28]  (212, 68)
	Common::Point(0xed, 0x46),  // [29]  (237, 70)
	Common::Point(0x91, 0x81),  // [30]  (145, 129)
	Common::Point(0xa8, 0x8b),  // [31]  (168, 139)
	Common::Point(0xbd, 0x8f),  // [32]  (189, 143)
	Common::Point(0xd7, 0x93),  // [33]  (215, 147)
	Common::Point(0xf0, 0x95),  // [34]  (240, 149)
	Common::Point(0x93, 0xca),  // [35]  (147, 202)
	Common::Point(0xaa, 0xd4),  // [36]  (170, 212)
	Common::Point(0xbf, 0xd8),  // [37]  (191, 216)
	Common::Point(0xd9, 0xdc),  // [38]  (217, 220)
	Common::Point(0xf2, 0xde),  // [39]  (242, 222)
	Common::Point(0x96, 0x113), // [40]  (150, 275)
	Common::Point(0xad, 0x11d), // [41]  (173, 285)
	Common::Point(0xc2, 0x121), // [42]  (194, 289)
	Common::Point(0xdc, 0x125), // [43]  (220, 293)
	Common::Point(0xf5, 0x127), // [44]  (245, 295)
	Common::Point(0x9a, 0x15b), // [45]  (154, 347)
	Common::Point(0xb1, 0x165), // [46]  (177, 357)
	Common::Point(0xc6, 0x169), // [47]  (198, 361)
	Common::Point(0xe0, 0x16d), // [48]  (224, 365)
	Common::Point(0xf9, 0x16f), // [49]  (249, 367)
	// Floor 2 (entries 50–74)
	Common::Point(0x10c, 0x3b), // [50]  (268, 59)
	Common::Point(0x123, 0x45), // [51]  (291, 69)
	Common::Point(0x138, 0x49), // [52]  (312, 73)
	Common::Point(0x152, 0x4d), // [53]  (338, 77)
	Common::Point(0x16b, 0x4f), // [54]  (363, 79)
	Common::Point(0x10f, 0x86), // [55]  (271, 134)
	Common::Point(0x126, 0x90), // [56]  (294, 144)
	Common::Point(0x13b, 0x94), // [57]  (315, 148)
	Common::Point(0x155, 0x98), // [58]  (341, 152)
	Common::Point(0x16e, 0x9a), // [59]  (366, 154)
	Common::Point(0x111, 0xcf), // [60]  (273, 207)
	Common::Point(0x128, 0xd9), // [61]  (296, 217)
	Common::Point(0x13c, 0xdd), // [62]  (316, 221)
	Common::Point(0x156, 0xe1), // [63]  (342, 225)
	Common::Point(0x16f, 0xe3), // [64]  (367, 227)
	Common::Point(0x114, 0x118),// [65]  (276, 280)
	Common::Point(0x12b, 0x122),// [66]  (299, 290)
	Common::Point(0x140, 0x126),// [67]  (320, 294)
	Common::Point(0x15a, 0x12a),// [68]  (346, 298)
	Common::Point(0x173, 0x12c),// [69]  (371, 300)
	Common::Point(0x118, 0x163),// [70]  (280, 355)
	Common::Point(0x12f, 0x16d),// [71]  (303, 365)
	Common::Point(0x144, 0x171),// [72]  (324, 369)
	Common::Point(0x15e, 0x175),// [73]  (350, 373)
	Common::Point(0x177, 0x177),// [74]  (375, 375)
	// Floor 3 (entries 75–99)
	Common::Point(0x188, 0x3f), // [75]  (392, 63)
	Common::Point(0x19f, 0x49), // [76]  (415, 73)
	Common::Point(0x1b5, 0x4d), // [77]  (437, 77)
	Common::Point(0x1cf, 0x51), // [78]  (463, 81)
	Common::Point(0x1e8, 0x53), // [79]  (488, 83)
	Common::Point(0x18b, 0x8a), // [80]  (395, 138)
	Common::Point(0x1a2, 0x94), // [81]  (418, 148)
	Common::Point(0x1b7, 0x98), // [82]  (439, 152)
	Common::Point(0x1d1, 0x9c), // [83]  (465, 156)
	Common::Point(0x1ea, 0x9e), // [84]  (490, 158)
	Common::Point(0x18d, 0xd3), // [85]  (397, 211)
	Common::Point(0x1a4, 0xdd), // [86]  (420, 221)
	Common::Point(0x1b9, 0xe1), // [87]  (441, 225)
	Common::Point(0x1d3, 0xe5), // [88]  (467, 229)
	Common::Point(0x1ec, 0xe7), // [89]  (492, 231)
	Common::Point(0x190, 0x11c),// [90]  (400, 284)
	Common::Point(0x1a7, 0x126),// [91]  (423, 294)
	Common::Point(0x1bc, 0x12a),// [92]  (444, 298)
	Common::Point(0x1d6, 0x12e),// [93]  (470, 302)
	Common::Point(0x1ef, 0x130),// [94]  (495, 304)
	Common::Point(0x194, 0x164),// [95]  (404, 356)
	Common::Point(0x1ab, 0x16e),// [96]  (427, 366)
	Common::Point(0x1c0, 0x172),// [97]  (448, 370)
	Common::Point(0x1da, 0x176),// [98]  (474, 374)
	Common::Point(0x1f3, 0x178),// [99]  (499, 376)
	// Floor 4 (entries 100–124)
	Common::Point(0x204, 0x48), // [100] (516, 72)
	Common::Point(0x21b, 0x52), // [101] (539, 82)
	Common::Point(0x230, 0x56), // [102] (560, 86)
	Common::Point(0x24a, 0x5a), // [103] (586, 90)
	Common::Point(0x263, 0x5c), // [104] (611, 92)
	Common::Point(0x207, 0x93), // [105] (519, 147)
	Common::Point(0x21e, 0x9d), // [106] (542, 157)
	Common::Point(0x233, 0xa1), // [107] (563, 161)
	Common::Point(0x24d, 0xa5), // [108] (589, 165)
	Common::Point(0x266, 0xa7), // [109] (614, 167)
	Common::Point(0x209, 0xdc), // [110] (521, 220)
	Common::Point(0x220, 0xe6), // [111] (544, 230)
	Common::Point(0x237, 0xea), // [112] (567, 234)
	Common::Point(0x251, 0xee), // [113] (593, 238)
	Common::Point(0x26a, 0xf0), // [114] (618, 240)
	Common::Point(0x20c, 0x125),// [115] (524, 293)
	Common::Point(0x223, 0x12f),// [116] (547, 303)
	Common::Point(0x238, 0x133),// [117] (568, 307)
	Common::Point(0x252, 0x137),// [118] (594, 311)
	Common::Point(0x26b, 0x139),// [119] (619, 313)
	Common::Point(0x210, 0x16d),// [120] (528, 365)
	Common::Point(0x227, 0x177),// [121] (551, 375)
	Common::Point(0x23c, 0x17b),// [122] (572, 379)
	Common::Point(0x256, 0x17f),// [123] (598, 383)
	Common::Point(0x26f, 0x181),// [124] (623, 385)
};

// IDA: word_4A138C[5] — column X-offsets for diff 3 runner registration
const int16 ZoombiniInteractiveHotel::kColumnOffsetX[5] = { 0, 23, 46, 69, 94 };

// IDA: word_4A1396[5] — column Y-offsets for diff 3 runner registration
const int16 ZoombiniInteractiveHotel::kColumnOffsetY[5] = { 0, 7, 11, 14, 17 };

} // End of namespace Mohawk
