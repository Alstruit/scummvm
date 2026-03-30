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
#include "mohawk/zoombini_pages/smoke.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions from 0x4A4368 (20 POINTS)
const Common::Point ZoombiniInteractiveSmoke::kSnoidPositions[20] = {
	Common::Point(214, 128), Common::Point(175, 126), Common::Point(135, 127), Common::Point( 94, 126),
	Common::Point( 53, 128), Common::Point(237, 176), Common::Point(196, 177), Common::Point(150, 178),
	Common::Point(110, 176), Common::Point( 69, 178), Common::Point(234,  36), Common::Point(195,  37),
	Common::Point(155,  36), Common::Point(114,  35), Common::Point( 73,  38), Common::Point(237,  79),
	Common::Point(196,  78), Common::Point(150,  80), Common::Point(110,  78), Common::Point( 69,  79),
};

// IDA: DRAW_ON_REG position at stru_4A400C
const Common::Point ZoombiniInteractiveSmoke::kDrawOnRegPosition = Common::Point(43, 258);

ZoombiniInteractiveSmoke::ZoombiniInteractiveSmoke(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kSmoke) {
}

ZoombiniInteractiveSmoke::~ZoombiniInteractiveSmoke() {
}

void ZoombiniInteractiveSmoke::open() {
	openArchive(ZMB_MHK_SMOKE);
}

void ZoombiniInteractiveSmoke::setBackgroundMusic() {
	// IDA: sound_activeHandle = nextRand_410705(20067, 20066)
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, _vm->_rnd->getRandomNumber(20066, 20067)), Audio::Mixer::kMusicSoundType);
}

void ZoombiniInteractiveSmoke::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniInteractiveSmoke::loadFeatures() {
	// IDA: smoke_init (0x44983c)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel() + 1;
	if (_difficultyLevel > 4)
		_difficultyLevel = 4;

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(0x64u)
	loadTerrainBitmap(100);

	// Preload shape images at tBMP 10000 (0x2710)
	// IDA: shape_loadSubShapesFromArchive(&stru_4B1D0C, 0x2710u)
	_vm->_gfx->preloadImage(10000);

	// Load REGS resources
	// IDA: regs_loadAndByteSwap(&dword_4B1D24, 0x2710u) — REGS 10000
	// IDA: regs_loadAndByteSwap(&dword_4B1D28, 0x2711u) — REGS 10001

	// Feature groups — single main SCRB at 11000
	// IDA: scrb_useFeatureGroup(0, 0, 11000)
	// IDA: scrb_loadMainFeatureSet(78, 11000)
	createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// Load reject pool: 1 at SCRS 11999
	// IDA: scrs_loadRejectPool(0, 1, 11999)
	loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 10000),
			  11999,
			  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);

	// Load normal pool: 50 at SCRS 12000
	// IDA: scrs_loadNormalPool(0, 50, 12000)
	for (uint16 i = 0; i < 50; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 10000),
				  12000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// === Additional feature runners from IDA smoke_init ===

	// Difficulty-dependent SCRB IDs
	uint16 scrbOverlayResId = (_difficultyLevel == 4) ? 11011 : 11013;
	uint16 scrbAnimId0, scrbAnimId1, scrbSmokeStackResA;
	if (_difficultyLevel <= 2) {
		scrbAnimId0 = 11024;
		scrbAnimId1 = 11025;
		scrbSmokeStackResA = 11032;
	} else {
		scrbAnimId0 = 11028;
		scrbAnimId1 = 11029;
		scrbSmokeStackResA = 11033;
	}

	// IDA: smoke_scrbOverlayAnim — overlay animation, interval=10
	_overlayAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), scrbOverlayResId, 10,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbLevel12Extra — SCRB 11076, diff 1/2 only, interval=10
	if (_difficultyLevel <= 2) {
		_level12ExtraFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 11076, 10,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER |
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// IDA: smoke_scrbCliffLeft — SCRB 11006, interval=10
	_cliffLeftFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 11006, 10,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbCliffRight — SCRB 11007, interval=10
	_cliffRightFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 11007, 10,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbMainAnim — main animation, interval=6
	_mainAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), scrbAnimId0, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbSmokeStackA — smoke stack animation, interval=6
	_smokeStackAFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), scrbSmokeStackResA, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbSmokeStackB — diff 3/4 only, SCRB 11034
	if (_difficultyLevel >= 3) {
		_smokeStackBFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 11034, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// IDA: smoke_scrbSecondAnim — second animation, interval=6
	_secondAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), scrbAnimId1, 6,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_04000000_OVERLAY |
		ZmbFeature::FLAG_08000000_REGION_TRACK);

	// IDA: smoke_scrbCompareA — SCRB 11018, interval=6
	_compareAFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 11018, 6,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbCompareB — SCRB 11019, interval=6
	_compareBFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 11019, 6,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_04000000_OVERLAY |
		ZmbFeature::FLAG_08000000_REGION_TRACK);

	// IDA: smoke_scrbBgOverlay — SCRB 11009, interval=6
	_bgOverlayFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 11009, 6,
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbRejection — SCRB 11036, interval=6
	_rejectionFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 11036, 6,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbBackground — SCRB 11008, interval=0
	_backgroundFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 11008, 0,
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbAnswerZone — SCRB 11002, interval=5
	_answerZoneFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 11002, 5,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: smoke_scrbHoldingArea — SCRB 11077, interval=0
	_holdingAreaFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 10000), 11077, 0,
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);

	// Load Zoombinis at 20 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, pointsArr_4A4368, 20)
	loadZoombinisFromPack();

	// NOTE: Smoke does NOT call zmb_layoutStaticAndWalkInGroups.
	// Uses smoke_buildRunnerStacks() for custom stack-based positioning.

	// IDA: scrb_drawOnRegRunnerIdxArr[0] — SCRB 11001, diff < 3 only, interval=7
	if (_difficultyLevel < 3) {
		_drawOnRegFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 11001, 7,
			kDrawOnRegPosition,
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER);
	}

	// IDA: SHPL_copyPaletteSrcToDst(236, 10)

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(10000);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagSmoke);

	// IDA: sound_activeHandle = nextRand(20066, 20067) — smoke narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, _vm->_rnd->getRandomNumber(20066, 20067));
}

void ZoombiniInteractiveSmoke::onGoButtonActivated() {
	// IDA: smoke_onClickHandler case 2
	// Route 4: Smoke -> Maze (via Xfer)
	_vm->_xferSrcSiPage = ZMB_SI_SMOKE_15;
	_vm->setNextPage(ZoombiniPageType::kXfer);
	close();
}

void ZoombiniInteractiveSmoke::loadZoombinisFromPack() {
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
