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
#include "mohawk/zoombini_pages/tunnels.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions at 0x4A7534 (16 POINTS)
const Common::Point ZoombiniInteractiveTunnels::kSnoidPositions[16] = {
	Common::Point(399, 402), Common::Point(367, 398), Common::Point(337, 397), Common::Point(306, 400),
	Common::Point(274, 400), Common::Point(240, 403), Common::Point(381, 424), Common::Point(351, 424),
	Common::Point(322, 428), Common::Point(292, 422), Common::Point(261, 426), Common::Point(371, 458),
	Common::Point(342, 459), Common::Point(310, 457), Common::Point(277, 457), Common::Point(245, 459),
};

ZoombiniInteractiveTunnels::ZoombiniInteractiveTunnels(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kTunnels) {
}

ZoombiniInteractiveTunnels::~ZoombiniInteractiveTunnels() {
}

void ZoombiniInteractiveTunnels::open() {
	openArchive(ZMB_MHK_TUNNELS);
}

void ZoombiniInteractiveTunnels::setBackgroundMusic() {
	// IDA: sound_activeHandle = nextRand_410705(20069, 20070)
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, _vm->_rnd->getRandomNumber(20069, 20070)), Audio::Mixer::kMusicSoundType);
}

void ZoombiniInteractiveTunnels::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(300)
	_vm->_gfx->setPalette(300);
	_vm->_gfx->drawBackground(300);
}

void ZoombiniInteractiveTunnels::loadFeatures() {
	// IDA: puzzleTunnels_459DCB (0x459dcb)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// Load NODE/PATH waypoints at 1000
	// IDA: node_loadNodeAndPath(0x3E8u)
	loadNODE(ZmbArchiveKind::kPage, 1000);

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(0x64u)
	loadTerrainBitmap(100);

	// Preload shape images at tBMP 400 (0x190)
	// IDA: shape_loadSubShapesFromArchive(&stru_4A750C, 0x190u)
	_vm->_gfx->preloadImage(400);

	// Feature groups
	// IDA: scrb_useFeatureGroup(0, 0, 5000) — main tunnel animations
	// IDA: scrb_useFeatureGroup(0, 1, 6000) — tunnel entrance doors
	// IDA: scrb_useFeatureGroup(0, 2, 7000) — tunnel path effects
	// IDA: scrb_useFeatureGroup(0, 3, 9000) — feedback/hint animations
	// IDA: scrb_useFeatureGroup(0, 4, 4000) — attribute group A
	// IDA: scrb_useFeatureGroup(0, 5, 4200) — attribute group B
	// IDA: scrb_useFeatureGroup(0, 6, 4400) — attribute group C
	// IDA: scrb_useFeatureGroup(0, 7, 4600) — attribute group D

	// Load main features: 4 SCRBs at 5000
	// IDA: scrb_loadMainFeatureSet(4, 5000)
	ZmbFeature *mainFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 400), 5000, 0,
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 12, 6000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 12; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 400), 6000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 5, 7000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 5; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 400), 7000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 7, 9000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 7; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 400), 9000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(2, 39, 4000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 39; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 400), 4000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(2, 27, 4200)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 27; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 400), 4200 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(2, 24, 4400)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 24; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 400), 4400 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(2, 18, 4600)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 18; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 400), 4600 + i);
		}
	}

	// Load reject pool: 8 at SCRS 8000
	// IDA: scrs_loadRejectPool(0, 8, 8000)
	for (uint16 i = 0; i < 8; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 400),
				  8000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 65 at SCRS 8500
	// IDA: scrs_loadNormalPool(5, 65, 8500)
	for (uint16 i = 0; i < 65; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 400),
				  8500 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// IDA: SHPL_copyPaletteSrcToDst(236, 10)

	// Load Zoombinis at 16 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, &stru_4A7534, 16)
	loadZoombinisFromPack();

	// Layout and stagger walk-in
	// IDA: zmb_layoutStaticAndWalkInGroups(100)
	// IDA: zmb_layoutStaticAndWalkInGroups(100)
	layoutStaticAndWalkIn();
	// IDA: zmb_assignStaggeredWalkDelays(30, 45)
	assignStaggeredWalkDelays();

	// Buttons
	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(400);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagTunnels);

	// IDA: sound_activeHandle = nextRand(20069, 20070) — tunnels narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, _vm->_rnd->getRandomNumber(20069, 20070));
}

void ZoombiniInteractiveTunnels::onGoButtonActivated() {
	// IDA: tunnels_onClickHandler case 2
	// Play departure SFX, start walk-off animation, then fade out when SFX finishes.
	// IDA: zmbMoveAnimation_45479D(45, 30, 670) — walk to (670, 30)
	playDepartSfx();
	startDepartWalkAnimation(Common::Point(670, 30));
	_pendingGoDepart = true;
}

void ZoombiniInteractiveTunnels::onEveryFrame() {
	if (!_pendingGoDepart)
		return;

	if (isDepartSfxDone()) {
		_pendingGoDepart = false;
		_vm->_xferSrcSiPage = ZMB_SI_TUNNELS_03;
		_vm->setNextPage(ZoombiniPageType::kXfer);
		close();
	}
}

void ZoombiniInteractiveTunnels::loadZoombinisFromPack() {
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
