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
#include "mohawk/zoombini_pages/fleens.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions from 0x4A0FE8 (16 POINTS)
const Common::Point ZoombiniInteractiveFleens::kSnoidPositions[16] = {
	Common::Point(238, 368), Common::Point(185, 417), Common::Point(155, 448), Common::Point(197, 396),
	Common::Point(160, 357), Common::Point(164, 384), Common::Point(150, 416), Common::Point(116, 357),
	Common::Point(130, 386), Common::Point(109, 418), Common::Point(117, 448), Common::Point( 74, 348),
	Common::Point( 89, 384), Common::Point( 67, 418), Common::Point( 76, 450), Common::Point( 56, 379),
};

// IDA: raft DRAW_ON_REG position at 0x4A1028
const Common::Point ZoombiniInteractiveFleens::kRaftPosition(438, 357);

ZoombiniInteractiveFleens::ZoombiniInteractiveFleens(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kFleens) {
}

ZoombiniInteractiveFleens::~ZoombiniInteractiveFleens() {
}

void ZoombiniInteractiveFleens::open() {
	openArchive(ZMB_MHK_FLEENS);
}

void ZoombiniInteractiveFleens::setBackgroundMusic() {
	// IDA: diff == 2 -> 20080; routeLevel 1||3 -> random(20079,20080); else 20079
	if (_difficultyLevel == 2)
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20080), Audio::Mixer::kMusicSoundType);
	else if (_difficultyLevel > 0)
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20079), Audio::Mixer::kMusicSoundType);
	else
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20079), Audio::Mixer::kMusicSoundType);
}

void ZoombiniInteractiveFleens::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(300)
	_vm->_gfx->setPalette(300);
	_vm->_gfx->drawBackground(300);
}

void ZoombiniInteractiveFleens::loadFeatures() {
	// IDA: fleens_initAndSetupPuzzle (0x41C3DC)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// Load terrain barrier bitmap (tBMP 500)
	// IDA: rmap_loadTerrainArchive(0x1F4u)
	loadTerrainBitmap(500);

	// Preload shape images
	// IDA: shape_loadSubShapesFromArchive(stru_4AB20C, 0xFA0u) — shapes at tBMP 4000
	_vm->_gfx->preloadImage(4000);

	// IDA: shape_loadSubShapesFromArchive(&stru_4AB20C, 0x190u) — shapes at tBMP 400
	_vm->_gfx->preloadImage(400);

	// Load feature groups
	// IDA: scrb_useFeatureGroup(0, 0, 1000)
	// IDA: scrb_useFeatureGroup(0, 1, 1100)
	// IDA: scrb_useFeatureGroup(0, 2, 1200)

	// Load REGS resources
	// IDA: regs_loadAndByteSwap(0xFA0u) — REGS 4000
	// IDA: regs_loadAndByteSwap(0xFA1u) — REGS 4001

	// Load main features: 7 SCRBs at 1000
	// IDA: scrb_loadMainFeatureSet(7, 1000)
	ZmbFeature *mainFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 4000), 1000, 0,
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 1, 0x44C) — 1 sub at 1100
	{
		ZmbFeature *parent = mainFeature;
		parent = loadSubFeature(parent,
			ZmbResource(ZmbArchiveKind::kPage, 4000), 1100);
	}

	// IDA: scrb_loadSubFeatureSet(0, 7, 0x4B0) — 7 subs at 1200
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 7; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 4000), 1200 + i);
		}
	}

	// Load reject pool: 5 reject scripts at SCRS 6000
	// IDA: scrs_loadRejectPool(0, 5, 6000)
	for (uint16 i = 0; i < 5; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 4000),
				  6000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 46 normal scripts at SCRS 7000
	// IDA: scrs_loadNormalPool(26, 46, 7000)
	for (uint16 i = 0; i < 46; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 4000),
				  7000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// --- Puzzle-specific feature runners ---

	// IDA: word_4AB1A4 = runner_registerAndAllocate(..., 6, 0x3E8, standard, standard, 0x108000)
	// Animation runner (SCRB 1000), LOOP_ANIM | PLAY_ONCE
	_animFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 4000), 1000, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);

	// IDA: scrb_drawOnRegRunnerIdxArr[0] = runner_registerAndAllocate(..., &raftPos, 7, 0x44C, standard, standard, 0x108A000)
	// Raft DRAW_ON_REG runner (SCRB 1100) at raft position
	_raftFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 4000), 1100, 7,
		kRaftPosition,
		ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER);

	// IDA: runner_registerAndAllocate(0, 0, 0, 0, 0, caves_invalidateEntranceRectsC, caves_renderAllAttrSlots, 0x1000)
	// Virtual feature for attribute slot rendering (TOPMOST)
	// TODO: Implement caves_invalidateEntranceRectsC / caves_renderAllAttrSlots callbacks
	{
		ZmbFeature::EventHooks attrSlotHooks;
		loadVirtualFeature(100, 0, ZmbFeature::FLAG_00001000_TOPMOST, attrSlotHooks);
	}

	// Load Zoombinis from active pack at 16 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, pPosArr, 16)
	loadZoombinisFromPack();

	// IDA: ferry_buildZmbRunners_41D9F4 — builds zoombini trait runners
	// TODO: Implement Zoombini trait runner setup (gameplay code)

	// IDA: 7× word_4AA848[scrbId] = runner_registerAndAllocate(..., 6, scrbId, standard, standard, flags)
	// Overlay runners (SCRB 1200-1206)
	for (int16 i = 0; i < 7; i++) {
		uint32 flags = ZmbFeature::FLAG_04000000_OVERLAY;
		if (i == 0) {
			// SCRB 1200 gets additional DEFER_ANIM | PLAY_ONCE
			flags |= ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE;
		}
		_overlayFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 4000), 1200 + i, 6, flags);
	}

	// Layout and stagger walk-in (200ms walk delay)
	// IDA: zmb_layoutStaticAndWalkInGroups(0)
	// IDA: zmb_assignStaggeredWalkDelays(200, 45)
	layoutStaticAndWalkIn();
	assignStaggeredWalkDelays();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(4000);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagFleens);

	// IDA: sound_activeHandle = nextRand(20079, 20080) — fleens narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, _vm->_rnd->getRandomNumber(20079, 20080));
}

void ZoombiniInteractiveFleens::onGoButtonActivated() {
	// IDA: fleens_onClickHandler case 2 -> puzzle_pendingTransitionTarget = 14
	// Route 3: Fleens -> Hotel (via Xfer)
	_vm->_xferSrcSiPage = ZMB_SI_FLEENS_10;
	_vm->setNextPage(ZoombiniPageType::kXfer);
	close();
}

void ZoombiniInteractiveFleens::loadZoombinisFromPack() {
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
}

} // End of namespace Mohawk
