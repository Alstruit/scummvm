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
#include "mohawk/zoombini_pages/caves.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions at 0x4A0A70 (20 POINTS)
const Common::Point ZoombiniInteractiveCaves::kSnoidPositions[20] = {
	Common::Point(180, 110), Common::Point(160, 136), Common::Point(130, 167), Common::Point(106, 193),
	Common::Point( 86, 232), Common::Point(140, 100), Common::Point(120, 126), Common::Point(100, 157),
	Common::Point( 76, 183), Common::Point( 46, 222), Common::Point(100,  90), Common::Point( 80, 116),
	Common::Point( 60, 147), Common::Point( 36, 173), Common::Point( 60,  80), Common::Point( 40, 106),
	Common::Point( 20, 137), Common::Point( 10, 167), Common::Point( 20,  90), Common::Point( 20, 116),
};

// IDA: DRAW_ON_REG positions at off_4A09BC+1 thru +20 for SCRB 7000-7019
// Cave entrance positions forming a spiral path through the cave system
const Common::Point ZoombiniInteractiveCaves::kCaveEntrancePositions[20] = {
	Common::Point(254, 140), Common::Point(296, 148), Common::Point(340, 146), Common::Point(373, 163),
	Common::Point(364, 187), Common::Point(337, 212), Common::Point(316, 234), Common::Point(301, 263),
	Common::Point(314, 292), Common::Point(346, 311), Common::Point(388, 316), Common::Point(429, 301),
	Common::Point(458, 281), Common::Point(482, 261), Common::Point(521, 247), Common::Point(556, 263),
	Common::Point(567, 290), Common::Point(543, 314), Common::Point(529, 342), Common::Point(554, 359),
};

ZoombiniInteractiveCaves::ZoombiniInteractiveCaves(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kCaves) {
}

ZoombiniInteractiveCaves::~ZoombiniInteractiveCaves() {
}

void ZoombiniInteractiveCaves::open() {
	openArchive(ZMB_MHK_CAVES);
}

void ZoombiniInteractiveCaves::setBackgroundMusic() {
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20065), Audio::Mixer::kMusicSoundType);
}

void ZoombiniInteractiveCaves::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniInteractiveCaves::loadFeatures() {
	// IDA: caves_funcInit (0x416978)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel() + 1;

	// Preload shape images
	// IDA: shape_loadSubShapesFromArchive(&stru_4A08C0, 0x2AF8u) — shapes at tBMP 11000
	_vm->_gfx->preloadImage(11000);

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(0x64u)
	loadTerrainBitmap(100);

	// Load NODE/PATH for walk network
	// IDA: node_loadNodeAndPath(0x3E8u)
	loadNODE(ZmbArchiveKind::kPage, 1000);

	// Load feature groups
	// IDA: scrb_useFeatureGroup(0, 0, 6000) — entrance animations
	// IDA: scrb_useFeatureGroup(0, 1, 9000) — overlays
	// IDA: scrb_useFeatureGroup(0, 2, 7000) — door animations
	// IDA: scrb_useFeatureGroup(0, 3, 8200) — glyph panels
	// IDA: scrb_useFeatureGroup(0, 4, 9025)

	// Load main features: 13 entrance SCRBs at 6000
	// IDA: scrb_loadMainFeatureSet(13, 6000)
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// Load sub-features chained from main
	// IDA: scrb_loadSubFeatureSet(0, 20, 9000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 20; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 11000), 9000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 20, 7000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 20; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 11000), 7000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 80, 8200)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 80; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 11000), 8200 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 4, 9025)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 4; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 11000), 9025 + i);
		}
	}

	// Load reject pool: 14 reject scripts at SCRS 12000
	// IDA: scrs_loadRejectPool(0, 14, 12000)
	for (uint16 i = 0; i < 14; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 11000),
				  12000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 5 normal scripts at SCRS 13000
	// IDA: scrs_loadNormalPool(0, 5, 13000)
	for (uint16 i = 0; i < 5; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 11000),
				  13000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// === Additional feature runners from IDA caves_funcInit ===

	// IDA: word_4AB078 — entrance animation SCRB 6000, interval=6
	_entranceAnimFeatures[0] = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), 6000, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: word_4AB07A — entrance animation SCRB 6001, interval=6
	_entranceAnimFeatures[1] = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), 6001, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: word_4AB07C — entrance animation SCRB 6002, interval=8
	_entranceAnimFeatures[2] = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 11000), 6002, 8,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: 4x cave entrance DRAW_ON_REG — SCRB 7000-7003, interval=7
	// IDA: scrb_drawOnRegRunnerIdxArr[0..3] from dword_4A09C0
	for (uint16 i = 0; i < 4; i++) {
		_doorDrawOnRegFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 7000 + i, 7,
			kCaveEntrancePositions[i],
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER |
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// IDA: loop v2=5..11 — SCRB 7004-7010 DRAW_ON_REG + glyph overlays SCRB 9004-9010
	for (uint16 i = 0; i < 7; i++) {
		_glyphOverlayFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 9004 + i, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);

		// TODO: no-op placeholder runner (word_4AB04C[5+i]) for render ordering

		_doorDrawOnRegFeatures[4 + i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 7004 + i, 7,
			kCaveEntrancePositions[4 + i],
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER |
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// IDA: door panel animations SCRB 9014-9011 (created in reverse order) + glyph DRAW_ON_REG SCRB 7011-7014
	// IDA: word_4AB010 (9014), word_4AB00E (9013), word_4AB00C (9012), word_4AB00A (9011)
	for (uint16 i = 0; i < 4; i++) {
		_doorPanelFeatures[3 - i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 9014 - i, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);

		// TODO: no-op placeholder runner (word_4AB064[3-i]) for render ordering
	}

	// IDA: word_4B7B60[0..3] — glyph DRAW_ON_REG SCRB 7011-7014 from corePosUnion
	for (uint16 i = 0; i < 4; i++) {
		_doorDrawOnRegFeatures[11 + i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 7011 + i, 7,
			kCaveEntrancePositions[11 + i],
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER |
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// TODO: runner_linkRelativeToParent(word_4AB064[i], 1, word_4B7B60[i]) for render ordering

	// IDA: loop v2=16..20 — SCRB 7015-7019 DRAW_ON_REG + glyph overlays SCRB 9015-9019
	for (uint16 i = 0; i < 5; i++) {
		_extraGlyphOverlayFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 9015 + i, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY);

		// TODO: no-op placeholder runner (word_4AB04C[16+i]) for render ordering

		_doorDrawOnRegFeatures[15 + i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 11000), 7015 + i, 7,
			kCaveEntrancePositions[15 + i],
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER |
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// TODO: Overlay SCRB 6012 (word_4AB080), REGION_TRACK runner, virtual glyph renderer (unk_4A090C)
	// These require caves_initDifficultyParams and caves_glyphSetupDispatch gameplay code

	// Load Zoombinis from active pack at 20 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, posTable, 20)
	// IDA: zmb_loadAnimationsFromActivePack(0)
	loadZoombinisFromPack();

	// Layout and stagger walk-in
	// IDA: zmb_layoutStaticAndWalkInGroups(0)
	layoutStaticAndWalkIn();
	assignStaggeredWalkDelays();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(11000);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagCaves);

	// IDA: sound_activeHandle = 20065 — caves narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20065);
}

void ZoombiniInteractiveCaves::onGoButtonActivated() {
	// IDA: caves_onClickHandler case 2 -> puzzle_pendingTransitionTarget = 17
	// Route 4: Caves -> Smoke (via Xfer)
	_vm->_xferSrcSiPage = ZMB_SI_CAVES_14;
	_vm->setNextPage(ZoombiniPageType::kXfer);
	close();
}

void ZoombiniInteractiveCaves::loadZoombinisFromPack() {
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
