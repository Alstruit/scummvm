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

#include "common/config-manager.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_pages/puzzle_hotel.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions from 0x4A13E4 (20 POINTS)
const Common::Point ZoombiniPuzzleHotel::kSnoidPositions[20] = {
	Common::Point(455, 423), Common::Point(432, 421), Common::Point(412, 420), Common::Point(395, 425),
	Common::Point(379, 418), Common::Point(365, 433), Common::Point(352, 412), Common::Point(340, 433),
	Common::Point(328, 418), Common::Point(314, 432), Common::Point(295, 421), Common::Point(279, 430),
	Common::Point(264, 437), Common::Point(259, 421), Common::Point(244, 432), Common::Point(226, 421),
	Common::Point(211, 427), Common::Point(195, 419), Common::Point(176, 423), Common::Point(158, 431),
};

ZoombiniPuzzleHotel::ZoombiniPuzzleHotel(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kHotel) {
}

ZoombiniPuzzleHotel::~ZoombiniPuzzleHotel() {
}

void ZoombiniPuzzleHotel::open() {
	// MIDIMPC.MHK contains MIDI BGM (tMID 30020-30023) — Broderbund v1.x only.
	// TLC v2.0 removed all MIDI resources.
	if (!_vm->isGameVariant(GF_ZMB_TLC))
		openArchive(ZMB_MHK_MIDIMPC);
	openArchive(ZMB_MHK_HOTEL);

	// DELIBERATE DIVERGENCE from original engine:
	// The original Windows engine had a bug where WAV SFX playback via MCI
	// killed any active MIDI playback. Hotel's MIDI BGM (tMID 30020-30023)
	// was effectively inaudible because SCRB frame SFX stopped it immediately.
	// When "fix_hotel_midi_bgm" is enabled (default), MIDI persists during SFX.
	// When disabled, we replicate the original bug via _stopMidiOnSfx.
	if (!_vm->isGameVariant(GF_ZMB_TLC))
		_vm->_sound->setStopMidiOnSfx(!ConfMan.getBool("fix_hotel_midi_bgm"));
}

void ZoombiniPuzzleHotel::setBackgroundMusic() {
	// IDA: hotel_initAndSetupPuzzle (0x41ede4) has no music playback call on page load.
	// sound_activeHandle = 20081 is stored at end of funcInit for F1 replay only.
}

void ZoombiniPuzzleHotel::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniPuzzleHotel::loadFeatures() {
	// IDA: hotel_initAndSetupPuzzle (0x41ede4)
	_difficultyLevel = static_cast<ZmbPuzzleDifficultyLevel>(_vm->_state->readActivePageRouteLevel() + 1);

	// IDA 0x41ef17-0x41ef46: Initialize maxStepsPerRound based on difficulty
	// Level 1: 5, Level 2: 2, Level 3: 4, Level 4: 2
	switch (_difficultyLevel) {
	case kPuzzleDiffLevel1:
		_maxStepsPerRound = 5;
		break;
	case kPuzzleDiffLevel3:
		_maxStepsPerRound = 4;
		break;
	default: // Levels 2 and 4
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
	if (_difficultyLevel == kPuzzleDiffLevel3) {
		// IDA: shape_loadSubShapesFromArchive(stru_4AB7CC, 0x2AF8u) — tBMP 11000
		_vm->_gfx->preloadImage(11000);
	}
	if (_difficultyLevel == kPuzzleDiffLevel4) {
		// IDA: shape_loadSubShapesFromArchive(stru_4AB7CC, 0x2EE0u) — tBMP 12000
		_vm->_gfx->preloadImage(12000);
	}

	// Feature groups — main SCRB depends on difficulty
	if (_difficultyLevel == kPuzzleDiffLevel4) {
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
		uint16 subCount = (_difficultyLevel == kPuzzleDiffLevel4) ? 125 : 25;
		uint16 subStart = (_difficultyLevel == kPuzzleDiffLevel4) ? 10025 : 10000;
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
	if (_difficultyLevel != kPuzzleDiffLevel4) {
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 10; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 7500), 7500 + i);
		}
	}

	// Load reject/normal pools — different sets at diff 4
	if (_difficultyLevel >= kPuzzleDiffLevel4) {
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

	// IDA 0x41f24f-0x41f27c: Intro overlay feature (SCRB 11500+adj_diff)
	// Decompiler showed "v0+5750" but assembly reveals: add esi, 0x2CEC (=11500)
	// diff 1→11500, diff 2→11501, diff 3→11501 (dec'd), diff 4→11502 (dec'd)
	{
		int16 introScrb = _difficultyLevel - 1;
		if (_difficultyLevel >= kPuzzleDiffLevel3)
			introScrb--;
		introScrb += 11500;
		_introFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11500), (uint16)introScrb, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);
	}

	// IDA 0x41f294: getDifficultyIdFromPuzzleFlag increments the page flag.
	// Must be called before room anim type selection (which reads the updated flag).
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagHotel);

	// IDA 0x41f3ec-0x41f3fe: Room animation runner (SCRB 7000+type)
	// word_4AB746 = intro anim type (0-6), controls guide prompt branching
	{
		_introAnimType = 0;
		switch (_difficultyLevel) {
		case kPuzzleDiffLevel2: _introAnimType = 4; break;
		case kPuzzleDiffLevel3: _introAnimType = 5; break;
		case kPuzzleDiffLevel4: _introAnimType = 6; break;
		default: {
			// IDA 0x41f2c3: diff=1, type 0 on first play, random on subsequent plays
			uint16 hotelPF = _vm->_state->_f._pageFlagHotel;
			if ((hotelPF & ZMB_PAGE_MASK_0FFF) > 1)
				_introAnimType = _vm->_rnd->getRandomNumber(1, (hotelPF & ZMB_PAGE_MASK_0FFF) - 1);
			break;
		}
		}
		// IDA 0x41f306: word_4AB748 = 1 when type is 0 or 4
		_bIntroNeedsGuide = (!_introAnimType || _introAnimType == 4);

		_roomAnimFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 7000), (uint16)(7000 + _introAnimType), 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_08000000_REGION_TRACK);
	}

	// IDA: word_4AB752 = runner_registerAndAllocate(..., 6, 0x2E18, standard, standard, 0x100000)
	// Room SCRB runner (SCRB 11800)
	_roomScrbFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11800), 11800, 6,
		ZmbFeature::FLAG_00100000_PLAY_ONCE);

	// Set total room count based on difficulty
	_totalRoomCount = (_difficultyLevel == kPuzzleDiffLevel4) ? 125 : 25;

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

	// IDA 0x41f465-0x41f48b: Register hotspot on room anim (intro completion trigger)
	// and activate the puzzle.
	// The board setup is triggered in onEveryFrame when the room anim feature
	// completes playing its intro SCRB (7000+type).
	_bPuzzleActive = true;
}

void ZoombiniPuzzleHotel::onGoButtonActivated() {
	// IDA: hotel_onClickHandler case 2
	// Stop BGM before departure, play SFX 996, fade out when SFX finishes.
	// IDA: scrb_enqueueSoundResource(0, 0) — stop background music
	_vm->_sound->stopAllSoundQueues();

	_departXferSrcSiPage = ZMB_SI_HOTEL_11;
	ZoombiniInteractive::onGoButtonActivated();
}

Common::String ZoombiniPuzzleHotel::debugGetAnswer() const {
	// Hotel axis: 0=foot, 1=nose, 2=eye, 3=head
	static const char *kAxisNames[] = {"foot", "nose", "eye", "head"};

	const char *axis1Name = (0 <= _attrAxis1 && _attrAxis1 <= 3) ? kAxisNames[_attrAxis1] : "?";
	const char *axis2Name = (0 <= _attrAxis2 && _attrAxis2 <= 3) ? kAxisNames[_attrAxis2] : "?";
	const char *axis3Name = (0 <= _attrAxis3 && _attrAxis3 <= 3) ? kAxisNames[_attrAxis3] : "?";

	Common::String s = Common::String::format("Hotel (level %d):\n", _difficultyLevel);
	s += Common::String::format("  Axes: dim1=%s(%d) dim2=%s(%d) dim3=%s(%d)\n",
		axis1Name, _attrAxis1, axis2Name, _attrAxis2, axis3Name, _attrAxis3);
	static const ZmbTrait::TraitCategory kPackedAxisToCategory[] = {
		ZmbTrait::kTraitFeet,
		ZmbTrait::kTraitNose,
		ZmbTrait::kTraitEyes,
		ZmbTrait::kTraitHair
	};
	ZmbTrait::TraitCategory axis1Category = (0 <= _attrAxis1 && _attrAxis1 <= 3) ? kPackedAxisToCategory[_attrAxis1] : ZmbTrait::kTraitHair;
	ZmbTrait::TraitCategory axis2Category = (0 <= _attrAxis2 && _attrAxis2 <= 3) ? kPackedAxisToCategory[_attrAxis2] : ZmbTrait::kTraitHair;
	ZmbTrait::TraitCategory axis3Category = (0 <= _attrAxis3 && _attrAxis3 <= 3) ? kPackedAxisToCategory[_attrAxis3] : ZmbTrait::kTraitHair;
	s += "  Grid1 (row): ";
	for (int i = 0; i < 25; i++) {
		if (_attrGrid1[i] != 0)
			s += Common::String::format("%d(%s) ", _attrGrid1[i], ZmbTrait::debugTraitValueName(axis1Category, _attrGrid1[i]));
	}
	s += "\n  Grid2 (floor): ";
	for (int i = 0; i < 25; i++) {
		if (_attrGrid2[i] != 0)
			s += Common::String::format("%d(%s) ", _attrGrid2[i], ZmbTrait::debugTraitValueName(axis2Category, _attrGrid2[i]));
	}
	s += "\n  Grid3 (column): ";
	for (int i = 0; i < 5; i++) {
		if (_attrGrid3[i] != 0)
			s += Common::String::format("%d(%s) ", _attrGrid3[i], ZmbTrait::debugTraitValueName(axis3Category, _attrGrid3[i]));
	}
	s += "\n";
	return s;
}

void ZoombiniPuzzleHotel::loadZoombinisFromPack() {
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
void ZoombiniPuzzleHotel::computeTraitVariantCounts() {
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
void ZoombiniPuzzleHotel::generateRoomRules() {
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

		if (_difficultyLevel <= kPuzzleDiffLevel2) {
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
		} else if (_difficultyLevel == kPuzzleDiffLevel3) {
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

	// IDA 0x420B7D: For diff 2, generate forbidden rooms based on which cells
	// have no matching zoombinis in the pre-filled constraint grid.
	if (_difficultyLevel == kPuzzleDiffLevel3) {
		// IDA 0x420B91-0x420BB1: Build a temporary 5×5 constraint grid
		int16 tempGrid[25] = {};
		int16 usedRows[5] = {};
		int16 usedCols[5] = {};

		// IDA 0x420BB3-0x420C02: Fill 5 rows with random unique row/col assignments
		for (int16 i = 0; i < 5; i++) {
			int16 colVal, rowVal;
			do {
				colVal = _vm->_rnd->getRandomNumber(0, 4);
			} while (usedCols[colVal]);
			usedCols[colVal] = 1;

			do {
				rowVal = _vm->_rnd->getRandomNumber(0, 4);
			} while (usedRows[rowVal]);
			usedRows[rowVal] = 1;

			// IDA: ferry_fillCellRow_4216BC(6*i, rowVal+1, colVal+1)
			// This fills the constraint grids for a full row and column
			for (int k = 0; k < 5; k++) {
				_attrGrid1[5 * k + (5 * i) % 5] = colVal + 1;
				_attrGrid2[k + (5 * i - (5 * i) % 5)] = rowVal + 1;
			}
		}

		// IDA 0x420C0E-0x420CAA: Count how many zmbs match each cell
		const ZmbStateFile &f = _vm->_state->_f;
		for (int16 j = 0; j < _totalZmbCount; j++) {
			// Find the j-th occupied entry
			int16 occIdx = -1;
			for (int16 z = 0; z < f._zmbPackActive._wPackZmbCount; z++) {
				if (!f._zmbPackActive._entries[z]._bIsOccupied)
					continue;
				occIdx++;
				if (occIdx == j) {
					const ZmbStateActiveEntry &entry = f._zmbPackActive._entries[z];
					int16 ax1 = getTraitValue(entry._traits, _attrAxis1);
					int16 ax2 = getTraitValue(entry._traits, _attrAxis2);

					for (int16 r = 0; r < 5; r++) {
						for (int16 c = 0; c < 5; c++) {
							int16 slot = c + 5 * r;
							if (ax1 == _attrGrid1[slot] && ax2 == _attrGrid2[slot])
								tempGrid[slot]++;
						}
					}
					break;
				}
			}
		}

		// IDA 0x420CAC-0x420CCF: Collect empty cells (no zmbs match)
		int16 emptyCells[25];
		int16 emptyCount = 0;
		for (int16 n = 0; n < 25; n++) {
			if (!tempGrid[n])
				emptyCells[emptyCount++] = n;
		}

		// IDA 0x420CD1-0x420CE7: Pick random forbidden count (1..emptyCount), cap at 8
		if (emptyCount > 0) {
			int16 forbiddenCount = _vm->_rnd->getRandomNumber(0, emptyCount - 1) + 1;
			if (forbiddenCount > 8)
				forbiddenCount = 8;
			if (emptyCount < forbiddenCount)
				forbiddenCount = emptyCount;

			// IDA 0x420CFD-0x420D42: Pick random empty cells as forbidden
			for (int16 fi = 0; fi < forbiddenCount; fi++) {
				int16 pickIdx;
				do {
					pickIdx = _vm->_rnd->getRandomNumber(0, emptyCount - 1);
				} while (_roomGrid[emptyCells[pickIdx]] < 0);

				_roomGrid[emptyCells[pickIdx]] = -1;
				_forbiddenRoomIds[fi] = _vm->_rnd->getRandomNumber(0, 3);
			}
		}
	}

	// Reset attribute grids (IDA 0x420DE2-0x420DF5)
	memset(_attrGrid1, 0, sizeof(_attrGrid1));
	memset(_attrGrid2, 0, sizeof(_attrGrid2));
	memset(_attrGrid3, 0, sizeof(_attrGrid3));
	_bFirstPlacement = true;
	_stepCounter = 1;
	_placedCount = 0;
}

// ---------------------------------------------------------------------------
// setupGameBoard: Called from onEveryFrame when intro animation completes.
// IDA: hotel_onHoverPerFrame priority-3 block (word_4AB77A / word_4AB7C4 branch)
// ---------------------------------------------------------------------------
void ZoombiniPuzzleHotel::setupGameBoard() {
	_setupFrameCount = getCurrentFrameCounter();

	// IDA 0x41f90b: Free room anim feature if type != 4
	if (_roomAnimFeature && _introAnimType != 4) {
		_roomAnimFeature->deactivateAnimate();
		_roomAnimFeature->deactivateRender();
		_roomAnimFeature = nullptr;
	}

	// IDA 0x41f937: Free intro feature
	if (_introFeature) {
		_introFeature->deactivateAnimate();
		_introFeature->deactivateRender();
		_introFeature = nullptr;
	}

	// IDA 0x41f943: Free room SCRB runner
	if (_roomScrbFeature) {
		_roomScrbFeature->deactivateAnimate();
		_roomScrbFeature->deactivateRender();
		_roomScrbFeature = nullptr;
	}

	// Redraw background for main gameplay (IDA: gfx_drawBackgroundFromResId(5001 or 5002))
	int16 bgId = (_difficultyLevel >= kPuzzleDiffLevel4) ? 5002 : 5001;
	_vm->_gfx->drawBackground(bgId);

	// IDA: fill opaque rects on the background per difficulty (binary data at 0x4A136C)
	switch (_difficultyLevel) {
	case kPuzzleDiffLevel1:
		_vm->_gfx->fillArea(ZoombiniGraphics::kBackScreen,
			Common::Rect(138, 293, 345, 351));
		_vm->_gfx->fillArea(ZoombiniGraphics::kBackScreen,
			Common::Rect(386, 309, 516, 362));
		break;
	case kPuzzleDiffLevel2:
	case kPuzzleDiffLevel3:
		_vm->_gfx->fillArea(ZoombiniGraphics::kBackScreen,
			Common::Rect(120, 45, 526, 362));
		break;
	case kPuzzleDiffLevel4:
		_vm->_gfx->fillArea(ZoombiniGraphics::kBackScreen,
			Common::Rect(11, 1, 638, 396));
		break;
	}

	// IDA 0x41fa59: Reassign pedestal positions (16 on reset, 20 initially handled in loadFeatures)
	// zmb_assignPedestalPositions(1, posData, 16)
	reassignPedestalPositions(16);

	// Register room display runners
	registerDisplayScrbs();

	// IDA 0x41fa8c-0x41fc82: Complex branching for guide/counter setup
	if (_difficultyLevel == kPuzzleDiffLevel4) {
		// IDA 0x41fc76-0x41fca1: diff 4 — reset counters, play sound
		_bGuideSkipped = false;
		_stepCounter = 1;
		_bPromptAnimDone = false;
		if (_roomAnimFeature) {
			_roomAnimFeature->deactivateAnimate();
			_roomAnimFeature->deactivateRender();
			_roomAnimFeature = nullptr;
		}
		if (!_vm->isGameVariant(GF_ZMB_TLC))
			_vm->_midi->playZmbMidi(ZmbResource(ZmbArchiveKind::kPage, (uint16)(30020 + (_difficultyLevel - 1))));
	} else if (_bGuideSkipped) {
		// IDA 0x41faa4-0x41fb0e: Guide-skipped reset
		_bIntroNeedsGuide = false;
		_bBatchWalkDone = false;
		_bGuideSkipped = false;
		_stepCounter = 1;
		_bPromptAnimDone = false;
		if (_roomAnimFeature) {
			_roomAnimFeature->deactivateAnimate();
			_roomAnimFeature->deactivateRender();
			_roomAnimFeature = nullptr;
		}
		// Create counter feature (SCRB maxSteps+6000)
		if (!_counterFeature) {
			_counterFeature = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 6000), (uint16)(_maxStepsPerRound + 6000), 6,
				ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);
		} else {
			loadScrbOntoFeature(_counterFeature, (uint16)(_maxStepsPerRound + 6000));
		}
		if (!_vm->isGameVariant(GF_ZMB_TLC))
			_vm->_midi->playZmbMidi(ZmbResource(ZmbArchiveKind::kPage, (uint16)(30020 + (_difficultyLevel - 1))));
	} else if (!_introAnimType || (_introAnimType == 4 && _bIntroNeedsGuide)) {
		// IDA 0x41fb34-0x41fb7b: Type 0 or (type 4 + needs guide) → play guide prompt
		if (!_roomAnimFeature) {
			_roomAnimFeature = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 7500), (uint16)(7500 + (_difficultyLevel - 1)), 6,
				ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
				ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_08000000_REGION_TRACK);
		} else {
			loadScrbOntoFeature(_roomAnimFeature, (uint16)(7500 + (_difficultyLevel - 1)));
		}
		_guideAnimPurpose = 1; // prompt
		_bPromptAnimDone = false;
	} else if (_introAnimType <= 4 || _bIntroNeedsGuide) {
		// IDA 0x41fc09-0x41fc6d: Skip guide, go directly to counter
		_bGuideSkipped = false;
		_stepCounter = 1;
		_bPromptAnimDone = false;
		// Create counter feature (SCRB 6000)
		_counterFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 6000), 6000, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);
		_bCounterAnimDone = true; // IDA: word_4AB784 = scrb_registerHotspotGroup(...)
		if (!_vm->isGameVariant(GF_ZMB_TLC))
			_vm->_midi->playZmbMidi(ZmbResource(ZmbArchiveKind::kPage, (uint16)(30020 + (_difficultyLevel - 1))));
	} else {
		// IDA 0x41fb9a-0x41fbfc: Type > 4, no guide needed — play guide with sounds
		if (_roomAnimFeature) {
			_roomAnimFeature->deactivateAnimate();
			_roomAnimFeature->deactivateRender();
			_roomAnimFeature = nullptr;
		}
		_roomAnimFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 7500), (uint16)(7500 + (_difficultyLevel - 1)), 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_08000000_REGION_TRACK);
		_guideAnimPurpose = 1; // prompt
		_bPromptAnimDone = false;
	}

	_overflowCounter = 0;

	debugC(kZmbDebugPage, "Hotel: game board set up");
}

// ---------------------------------------------------------------------------
// reassignPedestalPositions: Reposition remaining non-placed snoids to
// the first `count` pedestal positions. IDA: zmb_assignPedestalPositions
// ---------------------------------------------------------------------------
void ZoombiniPuzzleHotel::reassignPedestalPositions(int16 count) {
	int16 posIdx = 0;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end() && posIdx < count; ++it) {
		ZmbSnoid *snoid = *it;
		if (!snoid->_packIsOccupied)
			continue;
		// Skip already-placed snoids
		bool placed = false;
		for (uint32 pi = 0; pi < _placedSnoidIds.size(); pi++) {
			if (_placedSnoidIds[pi] == (uint16)snoid->getId()) {
				placed = true;
				break;
			}
		}
		if (placed)
			continue;
		snoid->setPointLoc(kSnoidPositions[posIdx]);
		snoid->setAnimState(kSnoidAnimIdle);
		posIdx++;
	}
}

// ---------------------------------------------------------------------------
// registerDisplayScrbs: Register room display and hotspot runners.
// IDA: hotel_registerDisplayScrbs_4203ED (0x4203ED)
// ---------------------------------------------------------------------------
void ZoombiniPuzzleHotel::registerDisplayScrbs() {
	// Free any existing features
	for (int i = 0; i < 125; i++) {
		if (_roomDisplayFeatures[i]) {
			_roomDisplayFeatures[i]->deactivateAnimate();
			_roomDisplayFeatures[i]->deactivateRender();
			_roomDisplayFeatures[i] = nullptr;
		}
		if (_roomIconFeatures[i]) {
			_roomIconFeatures[i]->deactivateAnimate();
			_roomIconFeatures[i]->deactivateRender();
			_roomIconFeatures[i] = nullptr;
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

	// IDA 0x4203ED flag constants:
	// Room display (6013+):  0x04180000 = OVERLAY | PLAY_ONCE | DEFER_ANIM
	// Room icon (6038+):     0x0C180000 = REGION_TRACK | OVERLAY | PLAY_ONCE | DEFER_ANIM
	// Label (11503/4/5):     0x04008000 = OVERLAY | LOOP_ANIM
	// Forbidden (11004+):    0x00808000 = POS_DELTA | LOOP_ANIM
	static const uint32 kRoomDisplayFlags =
		ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00080000_DEFER_ANIM;
	static const uint32 kRoomIconFlags =
		ZmbFeature::FLAG_08000000_REGION_TRACK | ZmbFeature::FLAG_04000000_OVERLAY |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00080000_DEFER_ANIM;
	static const uint32 kLabelFlags =
		ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM;
	static const uint32 kForbiddenFlags =
		ZmbFeature::FLAG_00800000_POS_DELTA | ZmbFeature::FLAG_00008000_LOOP_ANIM;

	if (_difficultyLevel == kPuzzleDiffLevel1) {
		// IDA: stride-5 room display (jj=4,9,14,19,24)
		for (int jj = 4; jj < _totalRoomCount; jj += 5) {
			_roomDisplayFeatures[jj] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 6000), (uint16)(jj + 6013), 6,
				kRoomDisplayFlags);
		}
		// IDA: label 11504
		_labelFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11500), 11504, 6, kLabelFlags);
		// IDA: stride-5 room icon (kk=4,9,14,19,24)
		for (int kk = 4; kk < _totalRoomCount; kk += 5) {
			_roomIconFeatures[kk] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 6000), (uint16)(kk + 6038), 3,
				kRoomIconFlags);
		}

	} else if (_difficultyLevel <= kPuzzleDiffLevel3) {
		// IDA: ALL 25 room display features (including forbidden slots)
		for (int m = 0; m < _totalRoomCount; m++) {
			_roomDisplayFeatures[m] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 6000), (uint16)(m + 6013), 6,
				kRoomDisplayFlags);
		}
		// IDA: diff 3 forbidden obstacle runners
		if (_difficultyLevel == kPuzzleDiffLevel3) {
			int fi = 0;
			for (int n = 0; n < _totalRoomCount; n++) {
				if (_roomGrid[n] == -1) {
					uint16 obstScrb = (uint16)(_forbiddenRoomIds[fi++] + 11004);
					_forbiddenFeatures[n] = loadScrbFeature(
						ZmbResource(ZmbArchiveKind::kPage, 11000), obstScrb, 0,
						kForbiddenFlags);
				}
			}
		}
		// IDA: label 11503
		_labelFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11500), 11503, 6, kLabelFlags);
		// IDA: ALL 25 room icon features
		for (int ii = 0; ii < _totalRoomCount; ii++) {
			_roomIconFeatures[ii] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 6000), (uint16)(ii + 6038), 3,
				kRoomIconFlags);
		}

	} else { // diff 4
		// IDA: register 125 position runners (SCRB col%5+9002)
		// Flags: 0x04980000 = OVERLAY | POS_DELTA | PLAY_ONCE | DEFER_ANIM
		static const uint32 kPosFlags =
			ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00800000_POS_DELTA |
			ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00080000_DEFER_ANIM;
		for (int i = 0; i < _totalRoomCount; i++) {
			_roomDisplayFeatures[i] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 9000), (uint16)(i % 5 + 9002), 6,
				kPosFlags);
		}
		// IDA: label 11505
		_labelFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11500), 11505, 6, kLabelFlags);
		// IDA: 125 background runners (SCRB col%5+9007)
		// We skip background runners as they're purely decorative overlays

		// IDA: forbidden obstacle runners for diff 3 (SCRB forbiddenRoomIds[]+12000)
		int fi = 0;
		for (int k = 0; k < _totalRoomCount; k++) {
			if (_roomGrid[k] == -1) {
				uint16 obstScrb = (uint16)(_forbiddenRoomIds[fi++] + 12000);
				_forbiddenFeatures[k] = loadScrbFeature(
					ZmbResource(ZmbArchiveKind::kPage, 12000), obstScrb, 0,
					kForbiddenFlags);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// validate2AttrPlacement: Check if a zmb can be placed at slot (diff 0–2).
// IDA: picker_checkAttrFilter_421729 (0x421729)
// Parameters: slot=targetSlot, axis2Val=trait[_attrAxis2], axis1Val=trait[_attrAxis1]
// Returns true = valid placement.
// ---------------------------------------------------------------------------
bool ZoombiniPuzzleHotel::validate2AttrPlacement(int16 slot, int16 axis2Val, int16 axis1Val) const {
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
//
// The original logic: if NO constraints set, simple uniqueness across 5 entries.
// If ANY constraint set, cascaded pairwise exclusion:
//   1. For each unset constraint, verify the value doesn't already appear in that grid
//   2. Each set constraint must match the provided value
//   3. If all 3 match exactly → valid
//   4. For each pair of axes where one matches and the other's constraint is 0,
//      verify the other axis value doesn't appear in that grid's 5 entries
// ---------------------------------------------------------------------------
bool ZoombiniPuzzleHotel::validate3AttrPlacement(int16 slot, int16 axis3Val, int16 axis2Val, int16 axis1Val) const {
	if (_attrBypass)
		return true;

	int16 col      = slot % 5;
	int16 rowGroup = (slot % 25) / 5;
	int16 floor    = slot / 25;

	int16 gRow   = _attrGrid1[rowGroup]; // axis1 constraint for this row-group
	int16 gFloor = _attrGrid2[floor];    // axis2 constraint for this floor
	int16 gCol   = _attrGrid3[col];      // axis3 constraint for this column

	// IDA 0x421EC4: if no constraints set at all — pure uniqueness check
	if (!gRow && !gFloor && !gCol) {
		for (int16 i = 0; i < 5; i++) {
			if (_attrGrid2[i] && axis2Val == _attrGrid2[i])
				return false;
			if (_attrGrid1[i] && axis1Val == _attrGrid1[i])
				return false;
			if (_attrGrid3[i] && axis3Val == _attrGrid3[i])
				return false;
		}
		return true;
	}

	// IDA 0x421F20-0x421F9E: Cascaded uniqueness for unset constraints
	// For each unset constraint, check uniqueness FIRST
	if (!gRow) {
		for (int16 i = 0; i < 5; i++) {
			if (_attrGrid1[i] && axis1Val == _attrGrid1[i])
				return false;
		}
	}
	if (!gFloor) {
		for (int16 i = 0; i < 5; i++) {
			if (_attrGrid2[i] && axis2Val == _attrGrid2[i])
				return false;
		}
	}
	if (!gCol) {
		for (int16 i = 0; i < 5; i++) {
			if (_attrGrid3[i] && axis3Val == _attrGrid3[i])
				return false;
		}
	}

	// IDA 0x421FBB-0x422003: Direct constraint checks — must match each set constraint
	if (gCol && axis3Val != gCol)
		return false;
	if (gRow && axis1Val != gRow)
		return false;
	if (gFloor && axis2Val != gFloor)
		return false;

	// IDA 0x422031: If all three match exactly → valid
	if (axis1Val == gRow && axis3Val == gCol && axis2Val == gFloor)
		return true;

	// IDA 0x42204D-0x42218B: Pairwise exclusion checks
	// For each pair: if one axis matches its constraint and the other's constraint is 0,
	// verify the other value doesn't already exist in that grid

	// Pair (axis1, axis2): axis1 matches but axis2 constraint is 0
	if (axis1Val == gRow && !gFloor) {
		for (int16 i = 0; i < 5; i++) {
			if (_attrGrid2[i] && axis2Val == _attrGrid2[i])
				return false;
		}
	}

	// Pair (axis2, axis1): axis2 matches but axis1 constraint is 0
	if (axis2Val == gFloor && !gRow) {
		for (int16 i = 0; i < 5; i++) {
			if (_attrGrid1[i] && axis1Val == _attrGrid1[i])
				return false;
		}
	}

	// Pair (axis1, axis3): axis1 matches but axis3 constraint is 0
	if (axis1Val == gRow && !gCol) {
		for (int16 i = 0; i < 5; i++) {
			if (_attrGrid3[i] && axis3Val == _attrGrid3[i])
				return false;
		}
	}

	// Pair (axis2, axis3): axis2 matches but axis3 constraint is 0
	if (axis2Val == gFloor && !gCol) {
		for (int16 i = 0; i < 5; i++) {
			if (_attrGrid3[i] && axis3Val == _attrGrid3[i])
				return false;
		}
	}

	// Pair (axis3, axis2): axis3 matches but axis2 constraint is 0
	if (axis3Val == gCol && !gFloor) {
		for (int16 i = 0; i < 5; i++) {
			if (_attrGrid2[i] && axis2Val == _attrGrid2[i])
				return false;
		}
	}

	// Pair (axis3, axis1): axis3 matches but axis1 constraint is 0
	if (axis3Val == gCol && !gRow) {
		for (int16 i = 0; i < 5; i++) {
			if (_attrGrid1[i] && axis1Val == _attrGrid1[i])
				return false;
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
// fillCellRow: Set axis1/axis2 constraints for an entire row+column.
// IDA: ferry_fillCellRow_4216BC (0x4216BC)
// rowIdx: target slot (0-24); axis2Val: row attribute; axis1Val: col attribute.
// ---------------------------------------------------------------------------
void ZoombiniPuzzleHotel::fillCellRow(int16 rowIdx, int16 axis2Val, int16 axis1Val) {
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
void ZoombiniPuzzleHotel::setCellAttrsIn3Grids(int16 cellIdx, int16 attrType, int16 attrValue, int16 gridLayer) {
	_attrGrid1[cellIdx % 25 / 5] = gridLayer; // axis1 → row-group
	_attrGrid2[cellIdx / 25]     = attrValue;  // axis2 → floor
	_attrGrid3[cellIdx % 5]      = attrType;   // axis3 → column
}

// ---------------------------------------------------------------------------
// placeZoombiniInRoom: Animate a zoombini entering an assigned room slot.
// IDA: hotel_setupRoomSlotScrb_422534 (0x422534)
// ---------------------------------------------------------------------------
void ZoombiniPuzzleHotel::placeZoombiniInRoom(int16 roomSlot, ZmbSnoid *snoid) {
	const Common::Point *posTable = (_difficultyLevel == kPuzzleDiffLevel4) ? kRoomPositions125 : kRoomPositions25;
	Common::Point basePos = posTable[roomSlot];
	Common::Point finalPos;
	int16 depth = _roomGrid[roomSlot]; // depth (already incremented before this call)

	if (_difficultyLevel == kPuzzleDiffLevel4) {
		// IDA 0x422731-0x42274D: base offset + packed DWORD subtract 0x20000
		// 0x20000 = LOWORD:0, HIWORD:2 → x unchanged, y -= 2
		basePos.x += 5;
		basePos.y -= 15;
		finalPos.x = basePos.x;
		finalPos.y = basePos.y - 2;
		if (depth > 1) {
			// IDA 0x422772: x -= 2*(depth-1), y -= (depth-1)
			finalPos.x -= 2 * (depth - 1);
			finalPos.y -= (depth - 1);
		}
	} else {
		// IDA 0x422582: pInitPos.x += 24; pInitPos.y -= 7
		basePos.x += 24;
		basePos.y -= 7;
		finalPos.y = basePos.y - 2;

		// IDA 0x4225A2-0x422684: X offset depends on difficulty and slot
		if (_difficultyLevel == kPuzzleDiffLevel1) {
			// IDA 0x4225A8: switch on targetRoomSlot (only 4,9,14,19,24)
			switch (roomSlot) {
			case 4:  finalPos.x = basePos.x - 5; break;
			case 9:  finalPos.x = basePos.x - 7; break;
			case 14: finalPos.x = basePos.x - 3; break;
			case 19: finalPos.x = basePos.x - 3; break;
			case 24: finalPos.x = basePos.x - 3; break;
			default: finalPos.x = basePos.x - 3; break;
			}
		} else {
			// IDA 0x422622-0x422684: diff 1-2 slot-range offsets
			if (roomSlot <= 4)
				finalPos.x = basePos.x - 8;
			else if (roomSlot <= 9)
				finalPos.x = basePos.x - 6;
			else if (roomSlot <= 14)
				finalPos.x = basePos.x - 5;
			else if (roomSlot <= 19)
				finalPos.x = basePos.x - 4;
			else if (roomSlot <= 24)
				finalPos.x = basePos.x - 5;
			else
				finalPos.x = basePos.x - 3;
		}

		// IDA 0x4226AC-0x42070E: Stacking adjustment based on depth%3
		// depth is the value of roomGrid[slot] (already incremented)
		if (!depth || depth % 3 == 1) {
			// IDA 0x4226AE: depth%3 == 0 or depth%3 == 1 path
			finalPos.x -= 8;
			finalPos.y = finalPos.y + depth - 1;
		} else if (depth % 3 == 2) {
			// IDA 0x4226EA: depth%3 == 2
			finalPos.x -= 1;
			finalPos.y = finalPos.y + depth - 1;
		} else {
			// IDA 0x42070E: depth%3 == 0 (other branch)
			finalPos.x += 6;
			finalPos.y = finalPos.y + depth - 1;
		}
	}

	_placedZmbPos = finalPos;

	// Select SCRS ID: eye trait value + base - 1
	// IDA: v7 = SHIBYTE(traitDword) + roomIdx - 1  (SHIBYTE = eye = axis index 2)
	int16 eyeVal = snoid->_trait._eye;
	int16 scrsBase;
	if (_difficultyLevel == kPuzzleDiffLevel4) {
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

	// Move to initial position (diff 4 uses specified pos; diff 1-3 stays where dropped)
	if (_difficultyLevel == kPuzzleDiffLevel4)
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
		if (_difficultyLevel >= kPuzzleDiffLevel4) {
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
void ZoombiniPuzzleHotel::dimPaletteOnError() {
	uint8 scalePercent = 92;
	if (_difficultyLevel == kPuzzleDiffLevel1)
		scalePercent = 88;
	else if (_difficultyLevel == kPuzzleDiffLevel3)
		scalePercent = 90;
	// Scale palette entries 10..245 (236 entries) — IDA: entries 10..246
	_vm->_gfx->scalePalettePartial(10, 236, scalePercent);
}

// ---------------------------------------------------------------------------
// registerWinCheckpoints: Reload room icon SCRBs to win-state (slot+6063).
// IDA: maze_registerCheckpointRunners_422A61 (0x422A61)
// ---------------------------------------------------------------------------
void ZoombiniPuzzleHotel::registerWinCheckpoints() {
	// IDA: diff 1 → stride-5 (j=4,9,14,19,24); diff 2-3 → all 25; diff 4 → nothing
	if (_difficultyLevel == kPuzzleDiffLevel1) {
		for (int16 j = 4; j < _totalRoomCount; j += 5) {
			if (_roomIconFeatures[j]) {
				loadScrbOntoFeature(_roomIconFeatures[j], (uint16)(j + 6063));
			}
		}
	} else if (_difficultyLevel <= kPuzzleDiffLevel3) {
		for (int16 i = 0; i < _totalRoomCount; i++) {
			if (_roomIconFeatures[i]) {
				loadScrbOntoFeature(_roomIconFeatures[i], (uint16)(i + 6063));
			}
		}
	}
	// diff 4: no win checkpoints registered (IDA falls through with no action)
}

// ---------------------------------------------------------------------------
// onFeatureAnimEvent: Called when a feature's animation cycle ends.
// ---------------------------------------------------------------------------
void ZoombiniPuzzleHotel::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (eventCode != kZmbAnimEventM1_End)
		return; // Only handle end-of-animation

	if (feature == _roomAnimFeature) {
		switch (_guideAnimPurpose) {
		case 0: // Intro room anim (7000+type) completed → trigger board setup
			_bBatchWalkDone = true;
			break;
		case 1: // Prompt animation done (guide SCRB 7500+diff)
			_bPromptAnimDone = true;
			break;
		case 2: // Cheer/win animation done → word_4AB77E fires
			_bWinAnimDone = true;
			break;
		default:
			break;
		}
		return;
	}

	if (feature == _counterFeature) {
		// IDA: word_4AB784 fires (counter animation step done)
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
void ZoombiniPuzzleHotel::onEveryFrame() {
	// IDA: hotel_onHoverPerFrame_41F6D2 (0x41F6D2)
	// hotel_bPuzzleComplete is NOT checked here — onEveryFrame runs always.

	// IDA setNextRenderFrameWithDebug_46EB56 freeze: skip interactive dispatch
	// until the deadline elapses. This holds the overflow visual stable for the
	// 60-frame window without dispatching new state transitions.
	if (_freezeUntilFrame != 0) {
		if (getCurrentFrameCounter() < _freezeUntilFrame)
			return;
		_freezeUntilFrame = 0;
	}

	// [Priority 1] Counter-step-rejection hotspot: word_4AB778
	// Fires after counter animation during rejection. If overflow happened (word_4AB76E > 0)
	// and diff != 3, load SCRB 7503+rand on guide → registers as word_4AB77E.
	if (_bCounterStepDone) {
		debugC(1, kZmbDebugAnimation, "Hotel: counter step done, overflow=%d diff=%d", _overflowCounter, _difficultyLevel);
		_bCounterStepDone = false;
		uint16 guideScrb = (uint16)(_vm->_rnd->getRandomNumber(0, 2) + 7503);
		if (_difficultyLevel != kPuzzleDiffLevel4 && _overflowCounter > 0) {
			// IDA 0x41f80d-0x41f875: Reload guide with SCRB 7503+rand
			if (_roomAnimFeature) {
				loadScrbOntoFeature(_roomAnimFeature, guideScrb);
			} else {
				_roomAnimFeature = loadScrbFeature(
					ZmbResource(ZmbArchiveKind::kPage, 7500), guideScrb, 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
					ZmbFeature::FLAG_08000000_REGION_TRACK);
			}
			_guideAnimPurpose = 2; // cheer-like → fires _bWinAnimDone when done
		}
		return;
	}

	// [Priority 2] Win/cheer hotspot: word_4AB77E
	// Fires when guide celebration SCRB or counter-overflow SCRB completes.
	if (_bWinAnimDone) {
		debugC(1, kZmbDebugAnimation, "Hotel: win anim done -> puzzle complete");
		_bWinAnimDone = false;
		// IDA 0x41f89f-0x41f8a8: hotel_bPuzzleComplete = 1
		_bPuzzleComplete = true;
		setGoButtonsEnabled(true);
		return;
	}

	// [Priority 3] Intro room anim done OR guide skip → (re)setup board
	// IDA: word_4AB77A (hotspot on room anim) fires, OR word_4AB7C4 (user clicked skip)
	if (_bBatchWalkDone || _bGuideSkipped) {
		debugC(1, kZmbDebugAnimation, "Hotel: batch walk done=%d guideSkipped=%d -> setupGameBoard",
			_bBatchWalkDone ? 1 : 0, _bGuideSkipped ? 1 : 0);
		_bBatchWalkDone = false;
		setupGameBoard();
		return;
	}

	if (!_bPuzzleActive)
		return;

	// --- Main gameplay block ---
	// IDA: only entered when (word_4AB746 && word_4AB746 != 4) || !word_4AB748 || word_4AB7C4
	// i.e. most of the time except waiting for type 0/4 intro with guide needed.

	// [Priority 4a] Guide prompt done (word_4AB77C) → start counter
	if (_bPromptAnimDone || _bGuideSkipped) {
		debugC(1, kZmbDebugAnimation, "Hotel: prompt done=%d guideSkipped=%d -> start counter",
			_bPromptAnimDone ? 1 : 0, _bGuideSkipped ? 1 : 0);
		_bPromptAnimDone = false;
		_bClickToSkipEnabled = false;
		if (!_vm->isGameVariant(GF_ZMB_TLC))
			_vm->_midi->playZmbMidi(ZmbResource(ZmbArchiveKind::kPage, (uint16)(30020 + (_difficultyLevel - 1))));

		if (_bGuideSkipped) {
			// IDA 0x41fe3c-0x41fe85: Guide was skipped by user click
			_bGuideSkipped = false;
			if (_roomAnimFeature) {
				_roomAnimFeature->deactivateAnimate();
				_roomAnimFeature->deactivateRender();
				_roomAnimFeature = nullptr;
			}
			if (_difficultyLevel != kPuzzleDiffLevel4) {
				// Create counter feature (SCRB maxSteps+6000)
				if (!_counterFeature) {
					_counterFeature = loadScrbFeature(
						ZmbResource(ZmbArchiveKind::kPage, 6000), (uint16)(_maxStepsPerRound + 6000), 6,
						ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);
				} else {
					loadScrbOntoFeature(_counterFeature, (uint16)(_maxStepsPerRound + 6000));
				}
			}
		} else if (_difficultyLevel != kPuzzleDiffLevel4) {
			// IDA 0x41feeb-0x41fedc: Normal prompt done, start counter
			if (!_counterFeature) {
				_counterFeature = loadScrbFeature(
					ZmbResource(ZmbArchiveKind::kPage, 6000), 6000, 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);
			} else {
				loadScrbOntoFeature(_counterFeature, 6000);
			}
			_bCounterAnimDone = true; // IDA: word_4AB784 registered immediately
		}
		return;
	}

	// [Priority 4b] Counter animation step done → advance counter
	if (_bCounterAnimDone && _counterFeature) {
		debugC(2, kZmbDebugAnimation, "Hotel: counter anim step=%d/%d", _stepCounter, _maxStepsPerRound);
		_bCounterAnimDone = false;
		if (_stepCounter <= _maxStepsPerRound) {
			loadScrbOntoFeature(_counterFeature, (uint16)(_stepCounter + 6000));
			_stepCounter++;
		}
		return;
	}

	// [Priority 4d] Pending placement → animate accept or reject
	if (_pendingPlacementSnoid) {
		debugC(1, kZmbDebugAnimation, "Hotel: pending placement, accepted=%d slot=%d placed=%d/%d",
			_pendingAccepted ? 1 : 0, _targetRoomSlot, _placedCount, _totalZmbCount);
		ZmbSnoid *snoid = _pendingPlacementSnoid;
		_pendingPlacementSnoid = nullptr;

		if (_pendingAccepted) {
			// --- ACCEPTED PLACEMENT ---
			_placedCount++;

			if (_roomGrid[_targetRoomSlot] > 0) {
				_roomGrid[_targetRoomSlot]++;
				if (_roomGrid[_targetRoomSlot] > 6)
					_roomGrid[_targetRoomSlot] = 6;
			}

			_placedSnoidIds.push_back((uint16)snoid->getId());
			placeZoombiniInRoom(_targetRoomSlot, snoid);

			// IDA 0x420053: Go button active after ANY accept
			_bPuzzleComplete = true;
			setGoButtonsEnabled(true);

			// First placement in this room → reload display SCRB.
			// IDA hotel_onHoverPerFrame @ 0x420072:
			//   if (diff >= 3) maze_loadScrbObstacleA(targetSlot)
			//                  → scrb_initRunnerWithScript(0,0, (slot%5)+9002, positionRunnerArr[slot]);
			//   else           scrb_initRunnerWithScript(0,0, slot+6013, roomScrbRunnerArr[slot]);
			if (_roomGrid[_targetRoomSlot] == 0) {
				if (_roomDisplayFeatures[_targetRoomSlot]) {
					uint16 scrbId = (_difficultyLevel < kPuzzleDiffLevel4)
						? (uint16)(_targetRoomSlot + 6013)
						: (uint16)((_targetRoomSlot % 5) + 9002);
					loadScrbOntoFeature(_roomDisplayFeatures[_targetRoomSlot], scrbId);
				}
				_roomGrid[_targetRoomSlot] = 1;
			}

			// IDA 0x421405: if all placed → play victory sound
			if (_placedCount == _totalZmbCount) {
				uint16 winSnd = (uint16)(175 + _vm->_rnd->getRandomNumber(0, 2));
				_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, winSnd));
			}

		} else {
			// --- REJECTED PLACEMENT ---
			_bRejectAnimActive = true;

			// IDA 0x4200D1: Rejection icon/checkpoint handling
			if (_difficultyLevel >= kPuzzleDiffLevel4) {
				// IDA maze_loadScrbObstacleB(targetSlot):
				//   scrb_initRunnerWithScript(0,0, (slot%5)+9007, bgScrbRunnerArr[slot])
				// ScummVM doesn't keep separate bgScrbFeatures, so we reload
				// the (existing) display feature with the rejection-overlay SCRB.
				if (_roomDisplayFeatures[_targetRoomSlot]) {
					uint16 obstBId = (uint16)((_targetRoomSlot % 5) + 9007);
					loadScrbOntoFeature(_roomDisplayFeatures[_targetRoomSlot], obstBId);
				}
				// IDA 0x420125: at step>=11 + !wMoreActionFlag0020, install
				// maze_checkRunnerAtCheckpoint as the bgScrbRunner's
				// onHotspotShapeOrFrameFunc. We approximate via the existing
				// reject-anim active flag — the runner-timer pipeline picks
				// it up next tick.
				if (_stepCounter >= 11) {
					_bRejectAnimActive = true;
				}
			} else if (_stepCounter >= 11) {
				registerWinCheckpoints();
			} else {
				if (_roomIconFeatures[_targetRoomSlot]) {
					loadScrbOntoFeature(_roomIconFeatures[_targetRoomSlot],
						(uint16)(_targetRoomSlot + 6038));
				}
			}

			// Play reject SCRS animation on snoid
			int16 eyeVal = snoid->_trait._eye;
			uint16 rejectScrsBase = (_difficultyLevel >= kPuzzleDiffLevel4) ? 14025 : 14000;
			uint16 rejectScrsId = (uint16)(rejectScrsBase + (eyeVal - 1));
			Common::SeekableReadStream *rejectStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, rejectScrsId));
			if (rejectStream) {
				snoid->startScrsPlayback(rejectStream, false, true);
			} else {
				_bRejectAnimActive = false;
			}

			// IDA 0x420164: Step counter management during rejection
			if (_difficultyLevel == kPuzzleDiffLevel4) {
				_stepCounter++;
			} else if (!_bFirstPlacement) {
				_stepCounter++;
				// Reload counter SCRB + register counter step callback
				if (_counterFeature) {
					loadScrbOntoFeature(_counterFeature, (uint16)(_stepCounter + 6000));
					_bCounterStepDone = false;
					// The counter animation completion will set _bCounterStepDone
					// which triggers the Priority 1 chain (word_4AB778)
				}
			}

			dimPaletteOnError();

			// IDA 0x4201C9: Step 9 escalation
			if (_stepCounter == 9) {
				if (_roomAnimFeature) {
					uint16 escalationScrb = (uint16)(7007 + _vm->_rnd->getRandomNumber(0, 2));
					loadScrbOntoFeature(_roomAnimFeature, escalationScrb);
				}
			}

			// IDA 0x420233: Step >= 12 — overflow (NO reset, just sound + overflow counter)
			if (_stepCounter >= 12) {
				_overflowCounter++;
				_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, 6006));
				// IDA setNextRenderFrameWithDebug_46EB56(1, 0, 60, 0):
				// hold the overflow animation for 60 frames before resuming
				// interactive dispatch.
				_freezeUntilFrame = getCurrentFrameCounter() + 60;
				if (_difficultyLevel == kPuzzleDiffLevel4) {
					_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, 7500));
				}
				// Note: Original does NOT reset the board here.
				// After overflow, the counter step chain (word_4AB778 → 7503+rand SCRB
				// on guide → word_4AB77E → bPuzzleComplete) provides a mercy exit.
			}
		}
		return;
	}

	// [Priority 4e] Placed-zmb SCRS animation done → finalise position + check win
	if (_bPlacedZmbAnimDone) {
		debugC(1, kZmbDebugAnimation, "Hotel: placed zmb anim done, placedCount=%d/%d", _placedCount, _totalZmbCount);
		_bPlacedZmbAnimDone = false;
		ZmbSnoid *snoid = _placedZmbSnoid;
		_placedZmbSnoid = nullptr;

		if (snoid) {
			if (_difficultyLevel == kPuzzleDiffLevel4) {
				snoid->setAnimState(kSnoidAnimIdle);
				snoid->setupIdleHotspots();
			} else {
				snoid->setAnimState(kSnoidAnimDepart);
			}
			snoid->setPointLoc(_placedZmbPos);
		}

		_bInteractionLock = false;

		// Check win condition
		if (_placedCount >= _totalZmbCount) {
			if (_difficultyLevel >= kPuzzleDiffLevel4) {
				// IDA 0x4203BF: diff 4 just sets sentinel
				_targetRoomSlot = 200;
			} else {
				// IDA 0x420332: diff 1-3 — win sequence
				registerWinCheckpoints();
				_targetRoomSlot = 200;

				if (_roomAnimFeature) {
					uint16 cheerScrb = (uint16)(7507 + _vm->_rnd->getRandomNumber(0, 2));
					loadScrbOntoFeature(_roomAnimFeature, cheerScrb);
					_guideAnimPurpose = 2; // fires _bWinAnimDone when done
				}
			}
		}
		return;
	}

	// [Priority 5] Fidget: reload guide prompt after idle timeout
	// IDA: if (game_getFrameCounter - dword_4AB7C8 > 0xB4 || !word_4AB746)
	if (_guideAnimPurpose == 0 || (getCurrentFrameCounter() - _setupFrameCount) > 0xB4) {
		if (_roomAnimFeature && _guideAnimPurpose == 0) {
			// IDA 0x41fd2f-0x41fd79: Reload guide prompt
			_bIntroNeedsGuide = false;
			loadScrbOntoFeature(_roomAnimFeature, (uint16)(7500 + (_difficultyLevel - 1)));
			_guideAnimPurpose = 1;
			_bPromptAnimDone = false;
			_setupFrameCount = getCurrentFrameCounter();
		}
	}
}

// ---------------------------------------------------------------------------
// findSnoidAtPoint: Only return draggable pack snoids (IDs 10000–12999).
// ---------------------------------------------------------------------------
ZmbSnoid *ZoombiniPuzzleHotel::findSnoidAtPoint(const Common::Point &pos) {
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
int16 ZoombiniPuzzleHotel::getDropTargetSlot(const Common::Point &dropPos) const {
	// IDA: beginDragFeatureRunner_45360F slot detection with rect_isPointInside.
	// Original builds a rect of +/-zmb_clickZoneRadius around the snoid position
	// and checks each slot's center point against it (L-inf / Chebyshev distance).
	// IDA: hotel_registerDisplayScrbs sets zmb_clickZoneRadius:
	//   Diff 0-2 (level 1-3): base 15 + 10 = 25
	//   Diff 3   (level 4):   hard-set 10
	const int16 radius = (_difficultyLevel >= kPuzzleDiffLevel4) ? 10 : 25;

	if (_difficultyLevel == kPuzzleDiffLevel4) {
		int16 best = -1;
		int32 bestDist = INT32_MAX;
		for (int i = 0; i < 125; i++) {
			int32 dx = ABS(dropPos.x - kRoomPositions125[i].x);
			int32 dy = ABS(dropPos.y - kRoomPositions125[i].y);
			if (dx <= radius && dy <= radius) {
				int32 d = dx + dy; // Manhattan tiebreak
				if (d < bestDist) { bestDist = d; best = static_cast<int16>(i); }
			}
		}
		return best;
	} else if (_difficultyLevel == kPuzzleDiffLevel1) {
		// Only slots 4, 9, 14, 19, 24
		int16 best = -1;
		int32 bestDist = INT32_MAX;
		for (int i = 4; i < 25; i += 5) {
			int32 dx = ABS(dropPos.x - kRoomPositions25[i].x);
			int32 dy = ABS(dropPos.y - kRoomPositions25[i].y);
			if (dx <= radius && dy <= radius) {
				int32 d = dx + dy;
				if (d < bestDist) { bestDist = d; best = static_cast<int16>(i); }
			}
		}
		return best;
	} else {
		// Diff 2-3: all 25 slots
		int16 best = -1;
		int32 bestDist = INT32_MAX;
		for (int i = 0; i < 25; i++) {
			int32 dx = ABS(dropPos.x - kRoomPositions25[i].x);
			int32 dy = ABS(dropPos.y - kRoomPositions25[i].y);
			if (dx <= radius && dy <= radius) {
				int32 d = dx + dy;
				if (d < bestDist) { bestDist = d; best = static_cast<int16>(i); }
			}
		}
		return best;
	}
}

// ---------------------------------------------------------------------------
// getTraitValue: Read trait value for a given axis index.
// IDA: *(char*)(&traitDword + axisIdx) — packed DWORD byte order: 0=foot,1=nose,2=eye,3=head.
// ---------------------------------------------------------------------------
byte ZoombiniPuzzleHotel::getTraitValue(const ZmbTrait &trait, int16 axisIdx) const {
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
void ZoombiniPuzzleHotel::endDrag(const Common::Point &mousePos) {
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

		if (_difficultyLevel == kPuzzleDiffLevel1) {
			_attrGrid1[targetSlot] = axis1Val;
		} else if (_difficultyLevel <= kPuzzleDiffLevel3) {
			fillCellRow(targetSlot, axis2Val, axis1Val);
		} else {
			setCellAttrsIn3Grids(targetSlot, axis3Val, axis2Val, axis1Val);
		}
	} else {
		// Validate against current constraints
		if (_difficultyLevel == kPuzzleDiffLevel1) {
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
		} else if (_difficultyLevel <= kPuzzleDiffLevel3) {
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

		// For diff 1: set constraint on valid accept (after validation above)
		if (_difficultyLevel == kPuzzleDiffLevel1 && !isHovered) {
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
ZmbEventHandleResult ZoombiniPuzzleHotel::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// Sticky mouse: second click drops the dragged snoid
	if (isDragging() && _vm->_state->getEnableStickyMouse()) {
		endDrag(absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	// Let base handle Go/Map/Help
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// IDA: hotel_onClickHandler 0x420e55 — click-to-skip animation
	if (_bClickToSkipEnabled && _roomAnimFeature) {
		_roomAnimFeature->deactivateAnimate();
		_roomAnimFeature->deactivateRender();
		_roomAnimFeature = nullptr;
		_bGuideSkipped = true;
	}

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
ZmbEventHandleResult ZoombiniPuzzleHotel::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
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
const Common::Point ZoombiniPuzzleHotel::kRoomPositions25[25] = {
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
const Common::Point ZoombiniPuzzleHotel::kRoomPositions125[125] = {
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
const int16 ZoombiniPuzzleHotel::kColumnOffsetX[5] = { 0, 23, 46, 69, 94 };

// IDA: word_4A1396[5] — column Y-offsets for diff 3 runner registration
const int16 ZoombiniPuzzleHotel::kColumnOffsetY[5] = { 0, 7, 11, 14, 17 };

} // End of namespace Mohawk
