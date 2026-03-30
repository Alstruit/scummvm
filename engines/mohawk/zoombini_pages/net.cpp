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
#include "mohawk/zoombini_pages/net.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions from 0x4A286C (16 POINTS)
const Common::Point ZoombiniInteractiveNet::kSnoidPositions[16] = {
	Common::Point(233, 392), Common::Point(209, 378), Common::Point(196, 390), Common::Point(185, 365),
	Common::Point(167, 380), Common::Point(160, 408), Common::Point(135, 397), Common::Point(121, 407),
	Common::Point(115, 368), Common::Point(114, 342), Common::Point( 99, 375), Common::Point( 97, 394),
	Common::Point( 95, 346), Common::Point( 91, 411), Common::Point( 79, 355), Common::Point( 62, 404),
};

ZoombiniInteractiveNet::ZoombiniInteractiveNet(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kNet) {
}

ZoombiniInteractiveNet::~ZoombiniInteractiveNet() {
}

void ZoombiniInteractiveNet::open() {
	openArchive(ZMB_MHK_NET);
}

void ZoombiniInteractiveNet::setBackgroundMusic() {
	// IDA: sound_activeHandle = 20064
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20064), Audio::Mixer::kMusicSoundType);
}

void ZoombiniInteractiveNet::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId((net_difficultyLevel >= 2) + 5000)
	// Background differs based on difficulty: 5000 or 5001
	uint16 bgId = (_difficultyLevel >= 2) ? 5001 : 5000;
	_vm->_gfx->setPalette(bgId);
	_vm->_gfx->drawBackground(bgId);
}

void ZoombiniInteractiveNet::loadFeatures() {
	// IDA: puzzleNet_4361D4 (0x4361d4)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// Preload shape images at tBMP 6000 (0x1770)
	// IDA: shape_loadSubShapesFromArchive(&stru_4A285C, 0x1770u)
	_vm->_gfx->preloadImage(6000);

	// Feature groups
	// IDA: scrb_useFeatureGroup(1, 0, 7000)
	// IDA: scrb_useFeatureGroup(0, 1, 8000)
	// IDA: scrb_useFeatureGroup(1, 2, 9000)
	// IDA: scrb_useFeatureGroup(0, 3, 10000)

	// Load main features: 48 SCRBs at 7000
	// IDA: scrb_loadMainFeatureSet(48, 7000)
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 8, 8000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 8; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 6000), 8000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 154, 9000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 154; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 6000), 9000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 19, 10000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 19; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 6000), 10000 + i);
		}
	}

	// Load reject pool: 3 at SCRS 14000
	// IDA: scrs_loadRejectPool(0, 3, 14000)
	for (uint16 i = 0; i < 3; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  14000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 51 at SCRS 13000
	// IDA: scrs_loadNormalPool(0, 51, 13000)
	for (uint16 i = 0; i < 51; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  13000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Register virtual render feature
	// IDA: runner_registerAndAllocate(0, 0, 0, 0, 0, net_invalidateVisualRects2, fleens_renderAllAttrSlots_436785, 0x1000)
	// Virtual feature for attribute slot rendering (TOPMOST)
	// TODO: Implement net_invalidateVisualRects2 / fleens_renderAllAttrSlots callbacks
	{
		ZmbFeature::EventHooks attrSlotHooks;
		loadVirtualFeature(100, 0, ZmbFeature::FLAG_00001000_TOPMOST, attrSlotHooks);
	}

	// Load Zoombinis at 16 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, v12, 16)
	// IDA: SHPL_copyPaletteSrcToDst(236, 10)
	loadZoombinisFromPack();

	// IDA: net_registerAllSCRBRunners(v10, &unk_4A28AC) — registers all puzzle SCRB runners
	// TODO: Implement net_registerAllSCRBRunners (gameplay code for column/slot runners)

	// Layout and stagger walk-in with walkDelay=30
	// IDA: zmb_layoutStaticAndWalkInGroups(0)
	layoutStaticAndWalkIn();
	// IDA: zmb_assignStaggeredWalkDelays(0, 30) — base class uses default values
	assignStaggeredWalkDelays();

	// Buttons
	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(6000);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagNet);

	// IDA: sound_activeHandle = 20064 — net narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20064);
}

void ZoombiniInteractiveNet::onGoButtonActivated() {
	// IDA: net_onClickHandler case 2
	// Stop BGM, play departure SFX, walk snoids to (600, -100), fade out when SFX finishes.
	// IDA: scrb_enqueueSoundResource(0, 0) — stop background music
	_vm->_sound->stopAllSoundQueues();

	playDepartSfx();

	// IDA: zmbMoveAnimation_45479D(45, -100, 600)
	startDepartWalkAnimation(Common::Point(600, -100));
	_pendingGoDepart = true;
}

void ZoombiniInteractiveNet::onEveryFrame() {
	if (!_pendingGoDepart)
		return;

	if (isDepartSfxDone()) {
		_pendingGoDepart = false;
		_vm->_xferSrcSiPage = ZMB_SI_NET_12;
		_vm->setNextPage(ZoombiniPageType::kXfer);
		close();
	}
}

void ZoombiniInteractiveNet::loadZoombinisFromPack() {
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
