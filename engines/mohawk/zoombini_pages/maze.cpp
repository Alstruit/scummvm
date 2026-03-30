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
#include "mohawk/zoombini_pages/maze.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions from 0x4A1F58 (20 POINTS)
const Common::Point ZoombiniInteractiveMaze::kSnoidPositions[20] = {
	Common::Point(287, 394), Common::Point(260, 426), Common::Point(224, 447), Common::Point(188, 441),
	Common::Point(157, 455), Common::Point(263, 384), Common::Point(219, 397), Common::Point(184, 388),
	Common::Point(155, 402), Common::Point(121, 417), Common::Point(226, 354), Common::Point(189, 349),
	Common::Point(156, 354), Common::Point(131, 375), Common::Point( 85, 394), Common::Point(164, 311),
	Common::Point(125, 324), Common::Point( 79, 352), Common::Point( 29, 318), Common::Point( 15, 285),
};

ZoombiniInteractiveMaze::ZoombiniInteractiveMaze(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kMaze) {
}

ZoombiniInteractiveMaze::~ZoombiniInteractiveMaze() {
}

void ZoombiniInteractiveMaze::open() {
	openArchive(ZMB_MHK_MAZE2);
}

void ZoombiniInteractiveMaze::setBackgroundMusic() {
	// IDA: sound_activeHandle = 20068
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20068), Audio::Mixer::kMusicSoundType);
}

void ZoombiniInteractiveMaze::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniInteractiveMaze::loadFeatures() {
	// IDA: puzzleMaze2_42E47C (0x42e47c)
	// Most complex puzzle init — large grid runner setup
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(100)
	loadTerrainBitmap(100);

	// Feature groups
	// IDA: scrb_useFeatureGroup(0, 0, 7000)
	// IDA: scrb_useFeatureGroup(0, 1, 8000)
	// IDA: scrb_useFeatureGroup(0, 2, 9000)
	// IDA: scrb_useFeatureGroup(0, 3, 10000)
	// IDA: scrb_useFeatureGroup(0, 4, 12000)

	// Preload shape images at tBMP 5100 (0x13EC)
	// IDA: shape_loadSubShapesFromArchive(&stru_4AF294, 0x13ECu)
	_vm->_gfx->preloadImage(5100);

	// Load main features: 28 SCRBs at 7000
	// IDA: scrb_loadMainFeatureSet(28, 7000)
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 14, 8000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 14; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 5100), 8000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 8, 9000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 8; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 5100), 9000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 44, 10000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 44; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 5100), 10000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 2, 12000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 2; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 5100), 12000 + i);
		}
	}

	// Load REGS resources for grid configuration
	// IDA: regs_loadAndByteSwap(&unk_4AF2A8, 16000)
	// IDA: net_loadRegsResource(&unk_4AF2F4, 16501, ...)
	// IDA: regs_loadAndByteSwap(&unk_4AF2B0, 17000)
	// IDA: regs_loadAndByteSwap(&unk_4AF2B4, 17001)
	// IDA: regs_loadAndByteSwap(&MEMORY[0x4AF298], 18000)
	// IDA: regs_loadAndByteSwap(&MEMORY[0x4AF29C], 18001)

	// Load reject pool: 8 at SCRS 14000
	// IDA: scrs_loadRejectPool(0, 8, 14000)
	for (uint16 i = 0; i < 8; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 5100),
				  14000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 96 at SCRS 15000
	// IDA: scrs_loadNormalPool(0, 96, 15000)
	for (uint16 i = 0; i < 96; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 5100),
				  15000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// IDA: SHPL_copyPaletteSrcToDst(236, 10)

	// IDA 0x42ea74: word_4AF2FA - overlay anim feature
	// SCRB 12001, interval=7, OVERLAY|LOOP_ANIM|DEFER_ANIM|PLAY_ONCE
	_overlayAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 5100), 12001, 7,
		ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE);

	// Load Zoombinis at 20 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, v31, 20)
	loadZoombinisFromPack();

	// TODO: Data-driven creature features based on maze_regsDataPtr (tREG config by difficulty).
	// IDA 0x42eae0-0x42ee1b: Creature loops (type 1&3) → SCRB 9005+type
	//   Grid position features → SCRB 7000+idx, OVERLAY|PLAY_ONCE|LOOP_ANIM
	//   DRAW_ON_REG features → SCRB 7014+idx, with positions from word_4A1BD4[]
	//   Obstacle features (cases 6-8) → complex switch with word_4A1CEC[], asc_4A1D0A[]
	// IDA 0x42ee22-0x42ee82: word_4AF314 linking (SCRB 8005 or 8010)

	// IDA 0x42eea8: word_4AF3F6[0] - creature base animation
	// SCRB 9005, interval=7, DEFER_ANIM|PLAY_ONCE|LOOP_ANIM
	_creatureBaseFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 5100), 9005, 7,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// IDA 0x42eed2-0x42ef3c: NoOp runner layers (word_4AF45C[0..10])
	// SCRB 8011, interval=0, noOp callbacks, OVERLAY|LOOP_ANIM
	for (int i = 0; i < 11; i++) {
		_noopFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 5100), 8011, 0,
			ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM);
	}

	// TODO: Data-driven creature/obstacle features (second pass)
	// IDA 0x42ef3e-0x42efa9: Creature loop 2 (type 2) → SCRB 9005+type, DEFER_ANIM|PLAY_ONCE
	// IDA 0x42efab-0x42f2d0: Obstacle loop 2 (cases 3-5, 12-13, default) → complex switch
	// IDA 0x42f2de-0x42f312: unk_4AF316 linking (SCRB 8001)
	// IDA 0x42f321-0x42f355: word_4AF318 linking (SCRB 8008)

	// IDA 0x42f378: final SCRB 8011, OVERLAY|LOOP_ANIM
	loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 5100), 8011, 0,
		ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// IDA 0x42f399: SCRB 8004, OVERLAY
	_finalOverlayA = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 5100), 8004, 0,
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA 0x42f3ba: SCRB 8000, OVERLAY
	_finalOverlayB = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 5100), 8000, 0,
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA 0x42f3bf-0x42f3f4: NoOp runner 11, SCRB 8011, OVERLAY|LOOP_ANIM
	_noopFeatures[11] = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 5100), 8011, 0,
		ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// Layout and stagger walk-in
	// IDA: zmb_layoutStaticAndWalkInGroups(0)
	layoutStaticAndWalkIn();
	// IDA: zmb_assignStaggeredWalkDelays(30, 45)
	assignStaggeredWalkDelays();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(6000);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagMaze);

	// IDA: sound_activeHandle = 20068 — maze narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20068);
}

void ZoombiniInteractiveMaze::onGoButtonActivated() {
	// IDA: maze_onClickHandler case 2 -> puzzle_pendingTransitionTarget = 6 (Town)
	// Route 4: Maze -> Town (via Xfer)
	_vm->_xferSrcSiPage = ZMB_SI_MAZE_16;
	_vm->setNextPage(ZoombiniPageType::kXfer);
	close();
}

void ZoombiniInteractiveMaze::loadZoombinisFromPack() {
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
