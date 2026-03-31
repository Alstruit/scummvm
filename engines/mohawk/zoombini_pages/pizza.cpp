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
#include "mohawk/zoombini_pages/pizza.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions at 0x4A3834 (16 POINTS)
const Common::Point ZoombiniInteractivePizza::kSnoidPositions[16] = {
	Common::Point(288, 389), Common::Point(240, 386), Common::Point(257, 434), Common::Point(202, 396),
	Common::Point(224, 437), Common::Point(186, 443), Common::Point(158, 400), Common::Point(151, 455),
	Common::Point(126, 391), Common::Point(118, 446), Common::Point( 89, 403), Common::Point( 86, 456),
	Common::Point( 48, 396), Common::Point( 51, 440), Common::Point( 20, 416), Common::Point( 18, 457),
};

// IDA: stru_4A381C+8 — DRAW_ON_REG position for answer display
const Common::Point ZoombiniInteractivePizza::kAnswerDisplayPosition = Common::Point(270, 334);

// IDA: base SCRB IDs for topping features per difficulty level (diff 0-3)
const uint16 ZoombiniInteractivePizza::kToppingScrbBase[4] = { 7005, 7015, 7027, 7041 };

ZoombiniInteractivePizza::ZoombiniInteractivePizza(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kPizza) {
}

ZoombiniInteractivePizza::~ZoombiniInteractivePizza() {
}

void ZoombiniInteractivePizza::open() {
	openArchive(ZMB_MHK_PIZZA);
}

void ZoombiniInteractivePizza::setBackgroundMusic() {
	// IDA: sound_activeHandle = 20071 (level 0) or 20072 (level 1+)
	if (_difficultyLevel > 0)
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20072), Audio::Mixer::kMusicSoundType);
	else
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20071), Audio::Mixer::kMusicSoundType);
}

void ZoombiniInteractivePizza::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniInteractivePizza::loadFeatures() {
	// IDA: puzzlePizza_43B394
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// Load NODE and PATH for walk network
	// IDA: node_loadNodeAndPath(0x3E8u)
	loadNODE(ZmbArchiveKind::kPage, 1000);

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(0x64u)
	loadTerrainBitmap(100);

	// Preload shape images
	// IDA: shape_loadSubShapesFromArchive(&stru_4A381C, 0x1770u) — shapes at tBMP 6000
	_vm->_gfx->preloadImage(6000);
	_vm->_gfx->preloadImage(7000);
	_vm->_gfx->preloadImage(8000);
	_vm->_gfx->preloadImage(9000);
	_vm->_gfx->preloadImage(10000);
	_vm->_gfx->preloadImage(12000);

	// Load feature groups
	// IDA: scrb_useFeatureGroup(0, 0, 7000)
	// IDA: scrb_useFeatureGroup(0, 1, 8000)
	// IDA: scrb_useFeatureGroup(0, 2, 12000)

	// Load main features: 69 SCRBs at 7000
	// IDA: scrb_loadMainFeatureSet(69, 7000)
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 36, 8000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 36; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 8000), 8000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 45, 12000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 45; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 12000), 12000 + i);
		}
	}

	// Conditional feature groups for difficulty levels 1+
	if (_difficultyLevel >= 1) {
		// IDA: scrb_useFeatureGroup(0, 3, 9000)
		// IDA: scrb_loadSubFeatureSet(0, 35, 9000)
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 35; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 9000), 9000 + i);
		}
	}

	if (_difficultyLevel >= 2) {
		// IDA: scrb_useFeatureGroup(0, 4, 10000)
		// IDA: scrb_loadSubFeatureSet(0, 39, 10000)
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 39; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 10000), 10000 + i);
		}
	}

	// Load reject pool: 6 reject scripts at SCRS 14000
	// IDA: scrs_loadRejectPool(0, 6, 14000)
	for (uint16 i = 0; i < 6; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  14000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 40 normal scripts at SCRS 13000
	// IDA: scrs_loadNormalPool(0, 40, 13000)
	for (uint16 i = 0; i < 40; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  13000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// IDA: answser display DRAW_ON_REG — SCRB 7063, interval=7
	// IDA: scrb_drawOnRegRunnerIdxArr[0] = runner_registerAndAllocate(..., MEMORY[0x4A3824], 7, 7063, ..., 0x108A000)
	_drawOnRegFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7063, 7,
		kAnswerDisplayPosition,
		ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER);

	// IDA: main tree/interaction animation — SCRB 7000, interval=6
	// IDA: MEMORY[0x4B0CC0] = runner_registerAndAllocate(..., 6, 7000, ..., 0x188000)
	// IDA: scrb_playFrameSounds(0, MEMORY[0x4B0CC0]); scrb_registerHotspotGroup(0, 0, 0, 0, ...)
	_treeAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7000, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE);

	// IDA: topping display features (difficulty-dependent count and base SCRB)
	// diff 0: 5 toppings (7005,7007,...,7013); diff 1: 6 (7015,...,7025)
	// diff 2: 7 (7027,...,7039); diff 3: 8 (7041,...,7055)
	{
		_toppingCount = _difficultyLevel + 5;
		uint16 scrbBase = kToppingScrbBase[_difficultyLevel];
		for (uint16 i = 0; i < _toppingCount; i++) {
			_toppingFeatures[i] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 7000), scrbBase + i * 2, 6,
				ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
				ZmbFeature::FLAG_00100000_PLAY_ONCE);
		}
	}

	// IDA: order display runners (conditional on difficulty)
	// IDA: if pizza_order1State → SCRB 9034 (diff >= 1)
	if (_difficultyLevel >= 1) {
		_order1Feature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 9000), 9034, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_00100000_PLAY_ONCE);
	}

	// IDA: MEMORY[0x4B0CDE] = SCRB 8032 (always)
	_orderBaseFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8032, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE);

	// IDA: if pizza_order2State → SCRB 10038 (diff >= 2)
	if (_difficultyLevel >= 2) {
		_order2Feature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 10038, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_00100000_PLAY_ONCE);
	}

	// IDA: pizza_overlayBaseRunner — SCRB 8033, flags=0x4108000 (OVERLAY|PLAY_ONCE|LOOP_ANIM)
	_overlayFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8033, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// Load Zoombinis from active pack at 16 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, stru_4A3834, 16)
	// IDA: zmb_loadAnimationsFromActivePack(v27, 0)
	loadZoombinisFromPack();

	// Layout and stagger walk-in (200ms walk delay)
	// IDA: zmb_layoutStaticAndWalkInGroups(200)
	layoutStaticAndWalkIn();
	assignStaggeredWalkDelays();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(6000);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagPizza);

	// IDA: sound_activeHandle = 20072 — pizza narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20072);
}

void ZoombiniInteractivePizza::onGoButtonActivated() {
	// IDA: pizza_onClickHandler case 2
	// Stop BGM, play departure SFX, walk snoids to (690, 250), fade out when SFX finishes.
	// IDA: scrb_enqueueSoundResource(0, 0) — stop background music
	_vm->_sound->stopAllSoundQueues();

	_departXferSrcSiPage = ZMB_SI_PIZZA_04;
	// IDA: zmbMoveAnimation_45479D(45, 250, 690)
	startDepartWalkAnimation(Common::Point(690, 250));
	ZoombiniInteractive::onGoButtonActivated();
}

void ZoombiniInteractivePizza::loadZoombinisFromPack() {
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
