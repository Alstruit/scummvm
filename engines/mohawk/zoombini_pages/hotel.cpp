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
	// IDA: sound_activeHandle = 20081
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20081), Audio::Mixer::kMusicSoundType);
}

void ZoombiniInteractiveHotel::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniInteractiveHotel::loadFeatures() {
	// IDA: hotel_initAndSetupPuzzle (0x41ede4)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

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

	// IDA: word_4AB750 = runner_registerAndAllocate(..., 6, introScrb+5750, standard, standard, 0x108000)
	// Intro animation runner — SCRB adjusted by difficulty
	{
		int16 introAdjust = (_difficultyLevel >= 2) ? _difficultyLevel - 1 : _difficultyLevel;
		_introAnimFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 8000), 5750 + introAdjust, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);
	}

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

	// Load Zoombinis from active pack at 20 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, posData, 20)
	loadZoombinisFromPack();

	// NOTE: Hotel does not call zmb_layoutStaticAndWalkInGroups
	// It positions Zoombinis differently

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(8000);
	loadHelpButtonFeature();

	// IDA: sound_activeHandle = 20081 — hotel narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20081);
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

} // End of namespace Mohawk
