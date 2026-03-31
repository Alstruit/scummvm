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
#include "mohawk/zoombini_pages/slides.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions at 0x4A3CF8 (16 POINTS)
const Common::Point ZoombiniInteractiveSlides::kSnoidPositions[16] = {
	Common::Point(482, 127), Common::Point(428, 128), Common::Point(375, 129), Common::Point(318, 127),
	Common::Point(272, 129), Common::Point(226, 128), Common::Point(184, 127), Common::Point(140, 129),
	Common::Point( 87, 128), Common::Point(110, 170), Common::Point(122, 246), Common::Point( 84, 212),
	Common::Point(140, 327), Common::Point( 77, 293), Common::Point( 40, 157), Common::Point( 44, 232),
};

ZoombiniInteractiveSlides::ZoombiniInteractiveSlides(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kSlides) {
}

ZoombiniInteractiveSlides::~ZoombiniInteractiveSlides() {
}

void ZoombiniInteractiveSlides::open() {
	openArchive(ZMB_MHK_SLIDES);
}

void ZoombiniInteractiveSlides::setBackgroundMusic() {
	// IDA: sound_activeHandle = 20078
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20078), Audio::Mixer::kMusicSoundType);
}

void ZoombiniInteractiveSlides::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniInteractiveSlides::loadFeatures() {
	// IDA: puzzleSlides_441F0C
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// At highest difficulty, load NODE/PATH for walking
	// IDA: if (slides_difficultyLevel == 3) node_loadNodeAndPath(0x3E8u)
	if (_difficultyLevel == 3) {
		loadNODE(ZmbArchiveKind::kPage, 1000);
	}

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(0x64u)
	loadTerrainBitmap(100);

	// Preload shape images
	// IDA: shape_loadSubShapesFromArchive(&stru_4A3B20, 0x1770u) — shapes at tBMP 6000
	_vm->_gfx->preloadImage(6000);
	_vm->_gfx->preloadImage(8000);

	// Load feature groups
	// IDA: scrb_useFeatureGroup(0, 0, 7000)
	// IDA: scrb_useFeatureGroup(0, 1, 8000)

	// Load main features: 14 SCRBs at 7000
	// IDA: scrb_loadMainFeatureSet(14, 7000)
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 3, 8000) — 3 subs at 8000
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 3; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 8000), 8000 + i);
		}
	}

	// Load reject pool: 4 reject scripts at SCRS 14000
	// IDA: scrs_loadRejectPool(0, 4, 14000)
	for (uint16 i = 0; i < 4; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  14000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 6 normal scripts at SCRS 13000
	// IDA: scrs_loadNormalPool(0, 6, 13000)
	for (uint16 i = 0; i < 6; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  13000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load Zoombinis from active pack at 16 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, &stru_4A3CF8, 16)
	loadZoombinisFromPack();

	// Layout and stagger walk-in
	// IDA: zmb_layoutStaticAndWalkInGroups(0)
	layoutStaticAndWalkIn();
	assignStaggeredWalkDelays();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(6000);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagSlides);

	// IDA: sound_activeHandle = 20078 — slides narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20078);
}

void ZoombiniInteractiveSlides::onGoButtonActivated() {
	// IDA: slides_onClickHandler case 2 -> puzzle_pendingTransitionTarget = 5 (BC2)
	// Route 2: Slides -> Fleens (via Xfer)
	_departXferSrcSiPage = ZMB_SI_SLIDES_09;
	ZoombiniInteractive::onGoButtonActivated();
}

void ZoombiniInteractiveSlides::loadZoombinisFromPack() {
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
