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
#include "mohawk/zoombini_pages/ferry.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions at 0x4A0E5C (20 POINTS)
const Common::Point ZoombiniInteractiveFerry::kSnoidPositions[20] = {
	Common::Point(370, 160), Common::Point(395, 196), Common::Point(332, 156), Common::Point(348, 196),
	Common::Point(294, 168), Common::Point(316, 196), Common::Point(253, 166), Common::Point(276, 196),
	Common::Point(214, 157), Common::Point(237, 196), Common::Point(175, 160), Common::Point(196, 190),
	Common::Point(135, 152), Common::Point(150, 191), Common::Point( 94, 145), Common::Point(110, 186),
	Common::Point( 57, 146), Common::Point( 71, 182), Common::Point( 25, 145), Common::Point( 27, 183),
};

ZoombiniInteractiveFerry::ZoombiniInteractiveFerry(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kFerry) {
}

ZoombiniInteractiveFerry::~ZoombiniInteractiveFerry() {
}

void ZoombiniInteractiveFerry::open() {
	openArchive(ZMB_MHK_FERRY);
}

void ZoombiniInteractiveFerry::setBackgroundMusic() {
	// IDA: diff == 2 -> 20074; routeLevel > 0 -> random(20073,20074); else 20073
	if (_difficultyLevel == 2)
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20074), Audio::Mixer::kMusicSoundType);
	else if (_difficultyLevel > 0)
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20073), Audio::Mixer::kMusicSoundType);
	else
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20073), Audio::Mixer::kMusicSoundType);
}

void ZoombiniInteractiveFerry::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(1300)
	_vm->_gfx->setPalette(1300);
	_vm->_gfx->drawBackground(1300);
}

void ZoombiniInteractiveFerry::loadFeatures() {
	// IDA: ferry_funcInit (0x41a394)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// IDA: ferry_selectSCRB (0x41bc4e) — calculate SCRB ID based on difficulty and zoombini count
	// Formula: scrbBase = 1510 + (level * 5); scrbId = base + (clamp(zmbCount, 16, 20) - 16)
	{
		int16 zmbCount = _vm->_state->_f._zmbPackActive._wPackZmbCount;
		if (zmbCount < 16)
			zmbCount = 16;
		else if (zmbCount > 20)
			zmbCount = 20;

		uint16 scrbBase = 1510 + (_difficultyLevel * 5);
		_seatingSCRB = scrbBase + (zmbCount - 16);
		debugC(kZmbDebugPage, "Ferry: difficultyLevel=%d, zmbCount=%d, seatingSCRB=%d",
		       _difficultyLevel, zmbCount, _seatingSCRB);
	}

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(100u)
	loadTerrainBitmap(100);

	// Preload shape images
	// IDA: shape_loadSubShapesFromArchive(&stru_4A0E58, 1400u)
	_vm->_gfx->preloadImage(1400);
	_vm->_gfx->preloadImage(1450);
	_vm->_gfx->preloadImage(1500);
	_vm->_gfx->preloadImage(1600);
	_vm->_gfx->preloadImage(1700);
	_vm->_gfx->preloadImage(1800);

	// Load feature groups
	// IDA: scrb_useFeatureGroup(0, 0, 1500)
	// IDA: scrb_useFeatureGroup(0, 1, 1600)
	// IDA: scrb_useFeatureGroup(0, 2, 1700)
	// IDA: scrb_useFeatureGroup(0, 3, 1800)
	// IDA: scrb_useFeatureGroup(0, 4, 1450)

	// Load main features: 10 SCRBs at 1500
	// IDA: scrb_loadMainFeatureSet(10, 1500)
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 10, 0x640) — 10 subs at 1600
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 10; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 1600), 1600 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 7, 0x6A4) — 7 subs at 1700
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 7; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 1700), 1700 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(5, 33, 0x708) — 33 subs at 1800
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 33; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 1800), 1800 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 3, 0x5AA) — 3 subs at 1450
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 3; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 1450), 1450 + i);
		}
	}

	// Load reject pool: 8 reject scripts at SCRS 1900
	// IDA: scrs_loadRejectPool(0, 8, 1900)
	for (uint16 i = 0; i < 8; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 1400),
				  1900 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 10 normal scripts at SCRS 1000
	// IDA: scrs_loadNormalPool(1, 10, 1000)
	for (uint16 i = 0; i < 10; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 1400),
				  1000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// --- Puzzle-specific feature runners ---

	// IDA: word_4AB13A = runner_registerAndAllocate(..., 6, 0x641, standard, standard, 0xC000)
	// Landscape overlay animation (SCRB 1601)
	_landscapeFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 1600), 1601, 6,
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// IDA: word_4AB17A = runner_registerAndAllocate(..., 6, boatScrb, standard, standard, 0x188000)
	// Boat animation runner — SCRB 1803 on first visit, random from pool on subsequent visits
	_boatAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 1800), 1803, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE);

	// IDA: conditional on !g_pGameState->wMoreActionFlag0020
	// Boat approach runners — only loaded when "more action" mode is active (lessAction=false)
	if (!_vm->_state->isLessActionEnabled()) {
		// IDA: word_4AB13E = runner_registerAndAllocate(..., 6, 1602, standard, standard, 0x8000)
		_boatApproachA = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 1600), 1602, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM);

		// IDA: word_4AB140 = runner_registerAndAllocate(..., 6, 1603, standard, standard, 0x8000)
		_boatApproachB = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 1600), 1603, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM);

		// NOTE: Original engine called scrb_linkRunnersToHotspotSlot(word_4AB140, word_4AB13E)
		// to pair both features on the same hotspot slot. ScummVM handles hotspot-per-feature
		// independently through findDrawRecordAtPoint(), so shared slots are not needed.
	}

	// IDA: word_4AB142 = runner_registerAndAllocate(..., 6, 0x6A8, standard, standard, 0x1188000)
	// Departure overlay runner (SCRB 1704)
	_departOverlayFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 1700), 1704, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER);

	// IDA: runner_registerAndAllocate(..., 6, 0x640, standard, standard, 0) — anonymous (SCRB 1600)
	loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 1600), 1600, 6,
		ZmbFeature::FLAG_00000000_TYPE_SHAPES);

	// IDA: 3× word_4AB14C[i] = runner_registerAndAllocate(..., 0, 1450+i, standard, standard, 0x4000000)
	// Overlay SCRBs (1450-1452)
	for (int16 i = 0; i < 3; i++) {
		_overlayFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 1450), 1450 + i, 0,
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// Load Zoombinis from active pack at 20 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, stru_4A0E58, 20)
	loadZoombinisFromPack();

	// Layout and stagger walk-in (30ms walk delay)
	// IDA: zmb_layoutStaticAndWalkInGroups(0)
	layoutStaticAndWalkIn();
	assignStaggeredWalkDelays();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(1400);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagFerry);

	// IDA: sound_activeHandle = 20073 — ferry narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20073);
}

void ZoombiniInteractiveFerry::onGoButtonActivated() {
	// IDA: ferry_onClickHandler case 2 -> word_4AB17C=1 -> puzzle_pendingTransitionTarget = 11
	// Route 2: Ferry -> Slides (via Xfer)
	_departXferSrcSiPage = ZMB_SI_FERRY_07;
	ZoombiniInteractive::onGoButtonActivated();
}

void ZoombiniInteractiveFerry::loadZoombinisFromPack() {
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
