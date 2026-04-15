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

#include "mohawk/zoombini_pages/puzzle_caves.h"
#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"
#include "mohawk/graphics.h"

namespace Mohawk {

// IDA: pedestal positions at 0x4A0A70 (20 POINTS)
const Common::Point ZoombiniPuzzleCaves::kSnoidPositions[20] = {
	Common::Point(180, 110),
	Common::Point(160, 136),
	Common::Point(130, 167),
	Common::Point(106, 193),
	Common::Point(86, 232),
	Common::Point(140, 100),
	Common::Point(120, 126),
	Common::Point(100, 157),
	Common::Point(76, 183),
	Common::Point(46, 222),
	Common::Point(100, 90),
	Common::Point(80, 116),
	Common::Point(60, 147),
	Common::Point(36, 173),
	Common::Point(60, 80),
	Common::Point(40, 106),
	Common::Point(20, 137),
	Common::Point(10, 167),
	Common::Point(20, 90),
	Common::Point(20, 116),
};

// IDA: DRAW_ON_REG positions at off_4A09BC+1 thru +20 for SCRB 7000-7019
// Cave entrance positions forming a spiral path through the cave system
const Common::Point ZoombiniPuzzleCaves::kCaveEntrancePositions[20] = {
	Common::Point(254, 140),
	Common::Point(296, 148),
	Common::Point(340, 146),
	Common::Point(373, 163),
	Common::Point(364, 187),
	Common::Point(337, 212),
	Common::Point(316, 234),
	Common::Point(301, 263),
	Common::Point(314, 292),
	Common::Point(346, 311),
	Common::Point(388, 316),
	Common::Point(429, 301),
	Common::Point(458, 281),
	Common::Point(482, 261),
	Common::Point(521, 247),
	Common::Point(556, 263),
	Common::Point(567, 290),
	Common::Point(543, 314),
	Common::Point(529, 342),
	Common::Point(554, 359),
};

// IDA: unk_4A0B24 + unk_4A0B38 — per-slot glyph Y positions (11 entries)
const int16 ZoombiniPuzzleCaves::kGlyphYPositions[11] = {
	0, 326, 348, 375, 397, 423, 324, 347, 373, 395, 422
};

// IDA: unk_4A0B3A + unk_4A0B4E — per-slot glyph X positions (11 entries)
const int16 ZoombiniPuzzleCaves::kGlyphXPositions[11] = {
	0, 36, 39, 42, 44, 46, 77, 80, 83, 86, 90
};

// IDA: byte_4A0AC8 (16-bit stride) — entrance type per 0-based entrance index.
// Type 1 or 2, giving SCRS ID = kEntranceType[idx] + 12999.
// Original uses 1-based indexing; these are shifted to 0-based.
const uint8 ZoombiniPuzzleCaves::kEntranceType[20] = {
	2, 2, 2, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1
};

ZoombiniPuzzleCaves::ZoombiniPuzzleCaves(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kCaves) {
}

ZoombiniPuzzleCaves::~ZoombiniPuzzleCaves() {
}

void ZoombiniPuzzleCaves::open() {
	// MIDIMPC.MHK contains MIDI BGM (tMID 30025-30028) — Broderbund v1.x only.
	if (!_vm->isGameVariant(GF_ZMB_TLC))
		openArchive(ZMB_MHK_MIDIMPC);
	openArchive(ZMB_MHK_CAVES);
}

void ZoombiniPuzzleCaves::setBackgroundMusic() {
	// IDA: caves_funcInit (0x416978) at 0x4172ad:
	//   if (caves_difficultyLevel < 4) scrb_enqueueSoundResource(30025 + routeDiffLevel)
	// Plays MIDI BGM for difficulty levels 1-3 (routeLevel 0-2); silent at level 4 (routeLevel 3).
	// TLC v2.0 has no MIDI resources.
	if (!_vm->isGameVariant(GF_ZMB_TLC)) {
		int16 routeLevel = _vm->_state->readActivePageRouteLevel();
		if (routeLevel < 3)
			_vm->_midi->playZmbMidi(ZmbResource(ZmbArchiveKind::kPage, (uint16)(30025 + routeLevel)));
	}
}

void ZoombiniPuzzleCaves::setBackgroundBitmap() {
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniPuzzleCaves::loadFeatures() {
	// IDA: caves_funcInit (0x416978)
	_difficultyLevel = static_cast<ZmbPuzzleDifficultyLevel>(_vm->_state->readActivePageRouteLevel() + 1);

	// Preload shape images
	// IDA: shape_loadSubShapesFromArchive(&stru_4A08C0, 0x2AF8u) — shapes at tBMP 11000
	_vm->_gfx->preloadImage(11000);
	_vm->_gfx->preloadImage(6000);
	_vm->_gfx->preloadImage(7000);
	_vm->_gfx->preloadImage(8200);
	_vm->_gfx->preloadImage(9000);
	_vm->_gfx->preloadImage(9025);

	// IDA: getHieroglyphsSprites_418559 — load hieroglyph sprite resource (tBMP 10000)
	// and per-slot X adjustment data (REGS 201)
	_vm->_gfx->preloadImage(10000);

	// Load per-slot glyph X adjustments from REGS 201.
	// IDA: dword_4AB0F4 — REGS resource 201 (big-endian uint16 array, byteswapped to LE)
	{
		Common::SeekableReadStream *regsStream =
			_vm->getResource(MKTAG('R', 'E', 'G', 'S'), ZmbResource(ZmbArchiveKind::kPage, 201));
		if (regsStream) {
			for (int i = 0; i < 11 && regsStream->pos() < regsStream->size(); i++)
				_glyphXAdj[i] = regsStream->readSint16BE();
			delete regsStream;
		}
	}

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
									ZmbResource(ZmbArchiveKind::kPage, 9000), 9000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 20, 7000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 20; i++) {
			parent = loadSubFeature(parent,
									ZmbResource(ZmbArchiveKind::kPage, 7000), 7000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 80, 8200)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 80; i++) {
			parent = loadSubFeature(parent,
									ZmbResource(ZmbArchiveKind::kPage, 8200), 8200 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 4, 9025)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 4; i++) {
			parent = loadSubFeature(parent,
									ZmbResource(ZmbArchiveKind::kPage, 9025), 9025 + i);
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
		ZmbResource(ZmbArchiveKind::kPage, 6000), 6000, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: word_4AB07A — entrance animation SCRB 6001, interval=6
	_entranceAnimFeatures[1] = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 6000), 6001, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: word_4AB07C — entrance animation SCRB 6002, interval=8
	_entranceAnimFeatures[2] = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 6000), 6002, 8,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: 4x cave entrance DRAW_ON_REG — SCRB 7000-7003, interval=7
	// IDA: scrb_drawOnRegRunnerIdxArr[0..3] from dword_4A09C0
	for (uint16 i = 0; i < 4; i++) {
		_doorDrawOnRegFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 7000), 7000 + i, 7,
			kCaveEntrancePositions[i],
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
				ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER |
				ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// IDA: loop v2=5..11 — SCRB 7004-7010 DRAW_ON_REG + glyph overlays SCRB 9004-9010
	for (uint16 i = 0; i < 7; i++) {
		_glyphOverlayFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 9000), 9004 + i, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
				ZmbFeature::FLAG_04000000_OVERLAY);

		// NOTE: Original engine used no-op placeholder runners (word_4AB04C[5+i]) for Z-ordering
		// in its linked-list renderer. ScummVM uses per-frame sorted rendering, so not needed.

		_doorDrawOnRegFeatures[4 + i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 7000), 7004 + i, 7,
			kCaveEntrancePositions[4 + i],
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
				ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER |
				ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// IDA: door panel animations SCRB 9014-9011 (created in reverse order) + glyph DRAW_ON_REG SCRB 7011-7014
	// IDA: word_4AB010 (9014), word_4AB00E (9013), word_4AB00C (9012), word_4AB00A (9011)
	for (uint16 i = 0; i < 4; i++) {
		_doorPanelFeatures[3 - i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 9000), 9014 - i, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
				ZmbFeature::FLAG_04000000_OVERLAY);

		// NOTE: Original engine used no-op placeholder runners (word_4AB064[3-i]) for Z-ordering.
	}

	// IDA: word_4B7B60[0..3] — glyph DRAW_ON_REG SCRB 7011-7014 from corePosUnion
	for (uint16 i = 0; i < 4; i++) {
		_doorDrawOnRegFeatures[11 + i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 7000), 7011 + i, 7,
			kCaveEntrancePositions[11 + i],
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
				ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER |
				ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// NOTE: Original engine called runner_linkRelativeToParent(word_4AB064[i], 1, word_4B7B60[i])
	// for Z-ordering. ScummVM uses per-frame sorted rendering, so not needed.

	// IDA: loop v2=16..20 — SCRB 7015-7019 DRAW_ON_REG + glyph overlays SCRB 9015-9019
	for (uint16 i = 0; i < 5; i++) {
		_extraGlyphOverlayFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 9000), 9015 + i, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
				ZmbFeature::FLAG_04000000_OVERLAY);

		// NOTE: Original engine used no-op placeholder runners (word_4AB04C[16+i]) for Z-ordering.

		_doorDrawOnRegFeatures[15 + i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 7000), 7015 + i, 7,
			kCaveEntrancePositions[15 + i],
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
				ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER |
				ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// IDA 0x417072: word_4AB080 — SCRB 6012 (0x177C), OVERLAY
	// The full glyph renderer draws matching symbols on cave entrances based on puzzle rules.
	// Glyph setup via setupEntranceGlyphs() → initEntranceAttrPattern/countGlyphDistribution/etc.
	_glyphPanelOverlayFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 6000), 6012, 0,
		ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA 0x41709c: word_4AB080 — SCRB unk_4A08F0+1 (_glyphPanelScrbId+1), REGION_TRACK
	// This is created after initDifficultyParams() which sets _glyphPanelScrbId
	initDifficultyParams();

	_glyphPanelRegionFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 6000), _glyphPanelScrbId + 1, 9,
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_08000000_REGION_TRACK);

	// Setup glyph patterns
	setupEntranceGlyphs();

	// IDA 0x4170e7: unk_4A090C — virtual glyph renderer with custom callbacks
	// caves_clearAndInvalidateRect as preRender, caves_renderAllEntranceGlyphs_41846A as render
	{
		ZmbFeature::EventHooks hooks;
		hooks.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(
			&ZoombiniPuzzleCaves::renderEntranceGlyphs));
		_virtualGlyphRenderer = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 6000), 6000, 0,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_04000000_OVERLAY,
			hooks);
	}

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

	// Initialize entrance hit rects from positions
	for (int i = 0; i < 20; i++) {
		int16 x = kCaveEntrancePositions[i].x;
		int16 y = kCaveEntrancePositions[i].y;
		_entranceHitRects[i] = Common::Rect(
			x - kEntranceHitRadius, y - kEntranceHitRadius,
			x + kEntranceHitRadius, y + kEntranceHitRadius);
	}

	_puzzleActive = true;
	_placedZmbCount = 0;
	_pendingWalkInSnoid = nullptr;
	_pendingWalkInScrsId = 0;
	_outOfZoneDrop = false;
	_interactionLocked = false;
	_hintFlashCounter = 0;

	// IDA: caves_funcInit global initialization
	_rejectScrsBaseId = 12004;
	_glyphScrbBaseId = 8200;
	_entranceAnimCounter = 0;
	_activeDoorFeature = nullptr;
	_selectedDoorOverlay = nullptr;
	_matchingDoorOverlay = nullptr;
	_bTransitionPending = false;
	_entranceCompletionFlag = false;
	_nActiveEntranceAnimCount = 0;
	_phaseState = 0;
	_activeDropSnoid = nullptr;
	_selectedEntranceIdx = 0;
	_matchingEntranceIdx = 0;
	_bWrongPlacement = false;
	_bDoorAnimPending = false;
	_bAdvanceClicked = false;
	_bAdvanceEnabled = false;
}

void ZoombiniPuzzleCaves::onGoButtonActivated() {
	// IDA: caves_onClickHandler case 2 -> puzzle_pendingTransitionTarget = 17
	// Route 4: Caves -> Smoke (via Xfer)
	_departXferSrcSiPage = ZMB_SI_CAVES_14;
	ZoombiniInteractive::onGoButtonActivated();
}

void ZoombiniPuzzleCaves::loadZoombinisFromPack() {
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

void ZoombiniPuzzleCaves::initDifficultyParams() {
	// IDA: caves_initDifficultyParams_41896E
	// Initialize difficulty parameters based on route level.
	// The level determines how many entrances are active and which SCRB panel to use.

	// Reset tracking state
	_hoveredEntranceSlot = 0;

	// Clear entrance attribute arrays
	for (int i = 0; i < 11; i++) {
		_entranceAttrReq[i] = 0;
		_entranceAttrOffset[i] = 0;
		_glyphTimingTable[i] = 0;
	}

	// Map level (1-4) to entrance count (4-7) and panel SCRB (6006-6003)
	switch (_difficultyLevel) {
	case kPuzzleDiffLevel1:
		_entranceCount = 4;
		_glyphPanelScrbId = 6006;
		break;
	case kPuzzleDiffLevel2:
		_entranceCount = 5;
		_glyphPanelScrbId = 6005;
		break;
	case kPuzzleDiffLevel3:
		_entranceCount = 6;
		_glyphPanelScrbId = 6004;
		break;
	case kPuzzleDiffLevel4:
	default:
		_entranceCount = 7;
		_glyphPanelScrbId = 6003;
		break;
	}
}

void ZoombiniPuzzleCaves::setupEntranceGlyphs() {
	// IDA: caves_glyphSetupDispatch_418A6E
	// Calls three sub-functions to setup the glyph pattern system:
	initEntranceAttrPattern();
	countGlyphDistribution();
	buildGlyphTimingTable();

	// IDA: caves_entranceAttrDist_418CB1 — distribute attributes to entrances
	distributeEntranceAttributes();
}

void ZoombiniPuzzleCaves::initEntranceAttrPattern() {
	// IDA: caves_initEntranceAttrPattern_418A7E
	// Initializes random attribute patterns using Fisher-Yates shuffle.

	// Guard complexity: 1 or 2 based on difficulty
	// IDA: unk_4A08E4 = (word_4AAF00 <= 2) ? 1 : 2
	_guardComplexity = (_difficultyLevel <= kPuzzleDiffLevel2) ? 1 : 2;

	// Number of attribute columns (typically 5)
	_attrColumnCount = 5;

	// Initialize base attribute types
	_baseAttrTypes[0] = 0;
	_baseAttrTypes[1] = 0;
	_entranceAttrBase = 0;

	// Clear attribute columns
	for (int row = 0; row < 2; row++) {
		for (int col = 0; col < _attrColumnCount; col++) {
			_attrColumns[5 * row + col] = 0;
		}
	}

	// Fisher-Yates shuffle for attribute selection
	int16 attrPool[7];
	int16 attrPoolSize = 3; // Initially 4 attributes (0-3), but we pick with removal

	for (int pass = 0; pass < 2; pass++) {
		// Reset column pool (0-6)
		int16 colPool[7];
		for (int i = 0; i < 7; i++) {
			colPool[i] = i;
		}

		// Reset attribute pool for first pass
		if (pass == 0) {
			for (int i = 0; i < 4; i++) {
				attrPool[i] = i;
			}
			attrPoolSize = 3;

			// Pick base attribute type
			int16 randIdx = _vm->_rnd->getRandomNumber(attrPoolSize);
			_baseAttrTypes[0] = attrPool[randIdx];

			// Remove selected attribute from pool
			for (int i = randIdx; i < attrPoolSize + 1; i++) {
				attrPool[i] = attrPool[i + 1];
			}
			attrPoolSize--;
		} else {
			// Second pass: pick entrance base attribute from remaining pool
			int16 randIdx = _vm->_rnd->getRandomNumber(attrPoolSize);
			_entranceAttrBase = attrPool[randIdx];
		}

		// Shuffle columns using Fisher-Yates
		int16 colPoolSize = 5;
		for (int col = 0; col < _attrColumnCount; col++) {
			int16 randIdx = _vm->_rnd->getRandomNumber(1, colPoolSize);
			_attrColumns[5 * pass + col] = colPool[randIdx];

			// Remove selected column from pool
			for (int i = randIdx; i < colPoolSize + 1; i++) {
				colPool[i] = colPool[i + 1];
			}
			colPoolSize--;
		}
	}
}

void ZoombiniPuzzleCaves::countGlyphDistribution() {
	// IDA: caves_countGlyphDistribution_418BFE
	// Counts glyph attribute distribution across loaded Zoombinis.

	// Get loaded Zoombini count from pack
	ZmbStateFile &f = _vm->_state->_f;
	_loadedZmbCount = 0;
	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount; i++) {
		if (f._zmbPackActive._entries[i]._bIsOccupied) {
			_loadedZmbCount++;
		}
	}

	// Guard complexity based on difficulty
	_guardComplexity = (_difficultyLevel <= kPuzzleDiffLevel2) ? 1 : 2;

	// Clear distribution table
	for (int i = 0; i < 36; i++) {
		_glyphDistribution[i] = 0;
	}

	// Count distribution based on Zoombini traits
	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount; i++) {
		if (!f._zmbPackActive._entries[i]._bIsOccupied)
			continue;

		ZmbTrait &traits = f._zmbPackActive._entries[i]._traits;
		uint8 traitBytes[4] = {
			static_cast<uint8>(traits._head),
			static_cast<uint8>(traits._eye),
			static_cast<uint8>(traits._nose),
			static_cast<uint8>(traits._foot)};

		// Get trait value for base attribute type
		int16 baseTraitVal = traitBytes[_baseAttrTypes[0]];
		_glyphDistribution[baseTraitVal]++;

		// For complex guards, also count cross-product
		if (_guardComplexity > 1) {
			int16 secondTraitVal = traitBytes[_entranceAttrBase];
			_glyphDistribution[6 * secondTraitVal + baseTraitVal]++;
		}
	}
}

void ZoombiniPuzzleCaves::buildGlyphTimingTable() {
	// IDA: caves_buildGlyphTimingTable_418F6C
	// Builds timing tables for glyph animations.

	// Clear timing arrays
	for (int i = 0; i < 21; i++) {
		_frameToSlotMap[i] = 0;
		_crossProductTable[i] = 0;
	}

	// Build frame-to-slot map from attribute columns and distribution
	int timingIdx = 21 - _loadedZmbCount; // IDA: dword_4A08FC
	for (int col = 0; col < _attrColumnCount; col++) {
		int16 slotVal = _attrColumns[col];
		int16 distCount = _glyphDistribution[slotVal];
		for (int j = 0; j < distCount && timingIdx < 21; j++) {
			_frameToSlotMap[timingIdx++] = slotVal;
		}
	}

	// Build cross-product table for complex guards
	if (_guardComplexity > 1) {
		int glyphIdx = 21 - _loadedZmbCount;
		for (int row = 0; row < _attrColumnCount; row++) {
			for (int col = 0; col < _attrColumnCount; col++) {
				int16 rowSlot = _attrColumns[5 + col]; // Second row
				int16 colSlot = _attrColumns[row];
				int16 crossDist = _glyphDistribution[6 * rowSlot + colSlot];
				for (int j = 0; j < crossDist && glyphIdx < 21; j++) {
					if (rowSlot != 0) {
						_crossProductTable[glyphIdx] = rowSlot;
					}
					glyphIdx++;
				}
			}
		}
	}
}

void ZoombiniPuzzleCaves::distributeEntranceAttributes() {
	// IDA: caves_entranceAttrDist_418CB1
	// Distributes attributes to cave entrances based on difficulty level.

	int16 slotPool[7];
	for (int i = 0; i < 7; i++) {
		slotPool[i] = i;
	}

	// Clear entrance requirements
	for (int i = 0; i < 11; i++) {
		_entranceAttrReq[i] = 0;
		_entranceAttrOffset[i] = 0;
	}

	switch (_difficultyLevel) {
	case kPuzzleDiffLevel1:
		// Level 1: All first 5 entrances active
		for (int slot = 1; slot < 6; slot++) {
			_entranceAttrReq[slot] = 1;
		}
		break;

	case kPuzzleDiffLevel2: {
		// Level 2: Random 2-4 of first 5 entrances
		int16 numActive = _vm->_rnd->getRandomNumber(2, 4);
		int16 poolSize = 5;
		for (int i = 0; i < numActive; i++) {
			int16 randIdx = _vm->_rnd->getRandomNumber(1, poolSize);
			_entranceAttrReq[slotPool[randIdx]] = 1;
			// Remove from pool
			for (int j = randIdx; j < poolSize + 1; j++) {
				slotPool[j] = slotPool[j + 1];
			}
			poolSize--;
		}
		break;
	}

	case kPuzzleDiffLevel3: {
		// Level 3: Random selection in two groups (0-4 and 5-9)
		for (int group = 0; group < 2; group++) {
			int16 offset = (group == 0) ? 0 : 5;

			// Reset pool
			for (int i = 0; i < 7; i++) {
				slotPool[i] = i;
			}

			int16 numActive = _vm->_rnd->getRandomNumber(2, 4);
			int16 poolSize = 5;
			for (int i = 0; i < numActive; i++) {
				int16 randIdx = _vm->_rnd->getRandomNumber(1, poolSize);
				_entranceAttrReq[slotPool[randIdx] + offset] = 1;
				// Remove from pool
				for (int j = randIdx; j < poolSize + 1; j++) {
					slotPool[j] = slotPool[j + 1];
				}
				poolSize--;
			}
		}
		break;
	}

	default:
		// Level 4+: Similar to level 1, all entrances active
		for (int slot = 1; slot < _entranceCount + 1; slot++) {
			_entranceAttrReq[slot] = 1;
		}
		break;
	}

	// Set attribute offsets based on base attribute type
	for (int slot = 1; slot < 6; slot++) {
		if (_entranceAttrReq[slot]) {
			switch (_baseAttrTypes[0]) {
			case 0:
				_entranceAttrOffset[slot] = slot; // Hair
				break;
			case 1:
				_entranceAttrOffset[slot] = slot + 5; // Eyes
				break;
			case 2:
				_entranceAttrOffset[slot] = slot + 10; // Nose
				break;
			case 3:
				_entranceAttrOffset[slot] = slot + 15; // Feet
				break;
			default:
				_entranceAttrOffset[slot] = slot;
				break;
			}
		}
	}

	// Set offsets for second group (slots 6-10)
	for (int slot = 6; slot < 11; slot++) {
		if (_entranceAttrReq[slot]) {
			switch (_entranceAttrBase) {
			case 0:
				_entranceAttrOffset[slot] = (slot - 5); // Hair
				break;
			case 1:
				_entranceAttrOffset[slot] = (slot - 5) + 5; // Eyes
				break;
			case 2:
				_entranceAttrOffset[slot] = (slot - 5) + 10; // Nose
				break;
			case 3:
				_entranceAttrOffset[slot] = (slot - 5) + 15; // Feet
				break;
			default:
				_entranceAttrOffset[slot] = (slot - 5);
				break;
			}
		}
	}
}

// =========================================================================
// Glyph rendering
// =========================================================================

ZmbRenderResult ZoombiniPuzzleCaves::renderEntranceGlyphs(ZmbFeature *feature) {
	// IDA: caves_renderAllEntranceGlyphs_41846A
	// Iterates entrance slots 1-10, drawing hieroglyph shapes at each active entrance.
	// At difficulty 1 with hoveredSlot < 6, skip the hovered entrance (hide its glyph).
	ZmbResource glyphRes(ZmbArchiveKind::kPage, 10000);
	bool skipHovered = (_difficultyLevel == kPuzzleDiffLevel1 && _hoveredEntranceSlot < 6);

	for (int slot = 1; slot < 11; slot++) {
		if (!_entranceAttrReq[slot])
			continue;

		if (skipHovered && slot == _hoveredEntranceSlot)
			continue;

		uint8 shapeIdx = _entranceAttrOffset[slot]; // 1-based shape index into tBMP 10000
		if (shapeIdx == 0)
			continue;

		// IDA: caves_renderGlyphHotspot_41837D
		// Get sub-image height for vertical centering.
		MohawkSurface *ms = _vm->_gfx->findShape(glyphRes, shapeIdx);
		if (!ms || !ms->getSurface())
			continue;

		int16 halfHeight = static_cast<int16>(ms->getSurface()->h) / 2;
		int16 x = kGlyphXPositions[slot] - _glyphXAdj[slot];
		int16 y = kGlyphYPositions[slot] - halfHeight;

		_vm->_gfx->drawShape(ZoombiniGraphics::kShapeScreen, glyphRes, shapeIdx,
							 Common::Point(x, y));
	}

	return ZmbRenderResult::kRendered;
}

// =========================================================================
// Gameplay methods
// =========================================================================

int16 ZoombiniPuzzleCaves::getEntranceSlotAtPoint(const Common::Point &pos) const {
	// Check each active entrance's hit rect
	for (int16 i = 0; i < 20; i++) {
		if (_entranceHitRects[i].contains(pos.x, pos.y))
			return i;
	}
	return -1;
}

int16 ZoombiniPuzzleCaves::findMatchingGlyphSlot(const ZmbTrait &traits) const {
	// IDA: caves_findMatchingGlyphSlot
	// Determines which cave entrance matches a Zoombini's attributes.
	// Uses _baseAttrTypes[0] as the primary attribute to match against entrance columns.

	uint8 traitBytes[4] = {
		static_cast<uint8>(traits._head),
		static_cast<uint8>(traits._eye),
		static_cast<uint8>(traits._nose),
		static_cast<uint8>(traits._foot)};

	int16 primaryVal = traitBytes[_baseAttrTypes[0]];

	// Simple matching (complexity 1): match primary attribute against columns
	if (_guardComplexity <= 1) {
		for (int16 col = 0; col < _attrColumnCount; col++) {
			if (_attrColumns[col] == primaryVal) {
				return col;
			}
		}
	} else {
		// Complex matching (complexity 2): match primary + secondary attribute
		int16 secondaryVal = traitBytes[_entranceAttrBase];

		for (int16 col = 0; col < _attrColumnCount; col++) {
			if (_attrColumns[col] == primaryVal) {
				// Check if secondary attribute also matches in second row
				for (int16 row = 0; row < _attrColumnCount; row++) {
					if (_attrColumns[5 + row] == secondaryVal) {
						// Both match — return combined slot
						return col + _attrColumnCount * row;
					}
				}
			}
		}
	}

	// Fallback: return first active entrance (should not normally happen)
	return 0;
}

void ZoombiniPuzzleCaves::handleCorrectPlacement(ZmbSnoid *snoid, int16 entranceSlot) {
	// IDA: caves_funcOnClick_417CDB — MATCH branch (hoverIdx == selectedIdx)
	// Correct match: queue walk-in, do NOT set _activeDropSnoid.

	_placedZmbCount++;
	snoid->_packIsOccupied = false;

	// IDA: priority = 0 — correct match clears activeDropSnoid
	// The walk-in is handled by the queue, not the door animation chain.

	if (_placedZmbCount == 1) {
		// IDA: First placement — enable advance button, merge dirty rect.
		// Original called dirty_mergeDrawnRectIntoRMap for the advance button rect;
		// not needed in ScummVM because the Go button feature re-renders every frame.
		_bAdvanceEnabled = true;
	}

	if (_placedZmbCount == _loadedZmbCount) {
		// IDA: All Zoombinis placed — trigger mass walk-in
		_bDoorAnimPending = true;
		_entranceCompletionFlag = true;
		// IDA: nextRand_410705(20055, 20063) — random cheer sound
		uint16 soundId = static_cast<uint16>(_vm->_rnd->getRandomNumber(20055, 20063));
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, soundId));
	}

	// Queue walk-in for processing in onEveryFrame.
	// IDA: caves_entranceAnimStates_4AB01E[caves_bHoverEnabled_4AB046++] = runnerIdx
	// IDA: pInitPos = &off_4A09BC + selectedIdx
	// SCRS ID from entrance type table: kEntranceType[idx] + 12999
	_pendingWalkInSnoid = snoid;
	_pendingWalkInScrsId = kEntranceType[entranceSlot] + 12999;
}

void ZoombiniPuzzleCaves::handleWrongPlacement(ZmbSnoid *snoid, int16 droppedSlot, int16 correctSlot) {
	// IDA: caves_funcOnClick_417CDB — MISMATCH branch (hoverIdx != selectedIdx)
	// Door animation chain: setupDoorAnimation(0) → event 1 (reject SCRS) → event 5 →
	// setupDoorAnimation(1) → event 2 (redirect SCRS) → event 4 (complete).

	// IDA: word_4AB04A = runnerIdx
	_activeDropSnoid = snoid;
	_selectedEntranceIdx = droppedSlot;
	_matchingEntranceIdx = correctSlot;

	// IDA: word_4AAF62 = caves_entranceSCRBRunnerArr[selectedIdx]
	// IDA: word_4AAF64 = caves_entranceSCRBRunnerArr[hoverIdx]
	_selectedDoorOverlay = getEntranceOverlayFeature(droppedSlot);
	_matchingDoorOverlay = getEntranceOverlayFeature(correctSlot);

	// IDA: ++HIWORD(caves_nTotalSlotCount)
	_placedZmbCount++;
	// IDA: unk_4A08E0 = 1
	_entranceCompletionFlag = true;
	// IDA: word_4AAEFE = 1 — triggers setupDoorAnimation(0) in onEveryFrame
	_bWrongPlacement = true;

	// IDA: clearPendingRunnerSlot_45354C()
	// In ScummVM, pending runner slot is N/A with per-frame sorted rendering.

	if (_placedZmbCount == 1) {
		// IDA: First placement — enable advance button.
		// Original called dirty_mergeDrawnRectIntoRMap for the advance button rect;
		// not needed in ScummVM because the Go button feature re-renders every frame.
		_bAdvanceEnabled = true;
	} else if (_placedZmbCount == _loadedZmbCount) {
		// IDA: All Zoombinis placed — random cheer sound
		uint16 soundId = static_cast<uint16>(_vm->_rnd->getRandomNumber(20055, 20063));
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, soundId));
	}
}

void ZoombiniPuzzleCaves::endDrag(const Common::Point &dropPos) {
	ZmbSnoid *snoid = finishSnoidDrag();
	if (!snoid)
		return;

	Common::Point snoidPos = snoid->getPointLoc();
	int16 droppedSlot = getEntranceSlotAtPoint(snoidPos);

	if (droppedSlot >= 0) {
		// Dropped on a cave entrance — check if it matches
		int16 correctSlot = findMatchingGlyphSlot(snoid->_trait);

		if (droppedSlot == correctSlot ||
			(_guardComplexity <= 1 && _attrColumns[droppedSlot % _attrColumnCount] == _attrColumns[correctSlot % _attrColumnCount])) {
			// Correct entrance
			snoid->_packIsOccupied = false;
			handleCorrectPlacement(snoid, droppedSlot);
		} else {
			// Wrong entrance
			handleWrongPlacement(snoid, droppedSlot, correctSlot);
		}
	} else {
		// Dropped outside any entrance.
		// IDA: caves_funcOnClick_417CDB — out-of-zone branch.
		// Sets entranceCompletionFlag and activeDropSnoid, then calls setupDoorAnimation(2)
		// to play SCRS 12012 (walk-back script) on the snoid.
		_entranceCompletionFlag = true;
		_activeDropSnoid = snoid;
		_outOfZoneDrop = true;
		setupDoorAnimation(2);
	}
}

ZmbEventHandleResult ZoombiniPuzzleCaves::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// Sticky mouse: second click ends drag
	if (isDragging() && _vm->_state->getEnableStickyMouse()) {
		endDrag(absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	// Let base class handle button clicks (Go/Map/Help)
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// IDA: !unk_4A08E0 — don't allow drag during door animation / entrance completion.
	if (_entranceCompletionFlag || !_puzzleActive)
		return ZmbEventHandleResult::kPassthrough;

	// Don't allow drag if already dragging
	if (isDragging())
		return ZmbEventHandleResult::kPassthrough;

	// Find Zoombini at click point
	ZmbSnoid *snoid = findSnoidAtPoint(absPos);
	if (!snoid)
		return ZmbEventHandleResult::kPassthrough;

	// Don't drag snoids that are playing scripts
	SnoidAnimState state = snoid->getAnimState();
	if (state == kSnoidAnimScriptReject || state == kSnoidAnimScriptNormal)
		return ZmbEventHandleResult::kPassthrough;

	// Begin drag
	startSnoidDrag(snoid, absPos);
	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniPuzzleCaves::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	if (!isDragging())
		return ZoombiniInteractive::onLButtonUp(absPos, relPos);

	// Sticky mouse: button-up does NOT end drag
	if (_vm->_state->getEnableStickyMouse())
		return ZmbEventHandleResult::kConsumed;

	endDrag(absPos);
	return ZmbEventHandleResult::kConsumed;
}

void ZoombiniPuzzleCaves::onEveryFrame() {
	if (_processingFrame || !_puzzleActive)
		return;
	_processingFrame = true;

	// [0] Pending Go departure: skip normal logic
	if (_pendingGoDepart) {
		_processingFrame = false;
		return;
	}

	// [0.5] Process entrance callback state flags.
	// IDA: caves_funcMain_41732B — processes nActiveEntranceAnimCount, bWrongPlacement, bTransitionPending.

	// Process nActiveEntranceAnimCount (set by entrance door event 4).
	// IDA: if (caves_nActiveEntranceAnimCount) { load entrance overlay SCRBs, update snoid }
	if (_nActiveEntranceAnimCount > 0) {
		debugC(1, kZmbDebugAnimation, "Caves: entrance anim callback, selectedIdx=%d matchIdx=%d",
			_selectedEntranceIdx, _matchingEntranceIdx);
		_nActiveEntranceAnimCount = 0;

		// IDA: Load SCRB (9000 + selectedIdx) onto word_4AAF62 runner, enable render.
		// scrb_loadOnRunner(0, selectedIdx + 8999, runner); runner->wBoolDoRender = 1;
		if (_selectedDoorOverlay) {
			loadScrbOntoFeature(_selectedDoorOverlay,
				static_cast<uint16>(9000 + _selectedEntranceIdx), false);
			_selectedDoorOverlay->activateRender();
		}

		// IDA: Load SCRB (9000 + hoverIdx) onto word_4AAF64 runner, enable render.
		if (_matchingDoorOverlay) {
			loadScrbOntoFeature(_matchingDoorOverlay,
				static_cast<uint16>(9000 + _matchingEntranceIdx), false);
			_matchingDoorOverlay->activateRender();
		}

		// IDA: Update snoid entrance tracking data from byte_4A0AC8 / word_4A0AF2 tables.
		// These tables map entrance indices to entrance type bytes and walk position IDs.
		// In ScummVM, the snoid positioning is handled by the SCRS playback system, so
		// the raw byte/word writes to the snoid runner struct are not needed.
	}

	// Process wrong placement flag (set by click handler).
	// IDA: if (word_4AAEFE) { setInteractionLock(0); caves_setupDoorAnimation(doorIdx=0) }
	if (_bWrongPlacement) {
		debugC(1, kZmbDebugAnimation, "Caves: wrong placement -> setupDoorAnimation(0)");
		_bWrongPlacement = false;
		_interactionLocked = false;
		setupDoorAnimation(0);
	}

	// Process transition pending (set by entrance door event 5).
	// IDA: if (bTransitionPending) { caves_setupDoorAnimation(doorIdx=1) }
	if (_bTransitionPending) {
		debugC(1, kZmbDebugAnimation, "Caves: transition pending -> setupDoorAnimation(1)");
		_bTransitionPending = false;
		setupDoorAnimation(1);
	}

	// Process advance button click.
	// IDA: if (word_4AAEF8 && !priority) { word_4AAEFA=1, load SCRB 6002 }
	if (_bAdvanceClicked && !_activeDropSnoid) {
		debugC(1, kZmbDebugAnimation, "Caves: advance clicked -> phaseState=1, loading SCRB 6002");
		_bAdvanceClicked = false;
		_phaseState = 1;
		_interactionLocked = false;
		// Load SCRB 6002 onto the success animation feature with glyph panel callback.
		if (_entranceAnimFeatures[2]) {
			loadScrbOntoFeature(_entranceAnimFeatures[2], 6002);
		}
	}

	// Process walk-in queue for correct placements.
	// IDA: while (caves_bHoverEnabled_4AB046) { play SCRS on queued snoid }
	if (_pendingWalkInSnoid) {
		debugC(1, kZmbDebugAnimation, "Caves: walk-in queue, scrsId=%d", _pendingWalkInScrsId);
		ZmbSnoid *walkSnoid = _pendingWalkInSnoid;
		int16 walkScrsId = _pendingWalkInScrsId;
		_pendingWalkInSnoid = nullptr;
		_pendingWalkInScrsId = 0;

		// IDA: runner->bitmask = OVERLAY | SNOID (0x04000001)
		// IDA: snoidScript_initAndPlay(0, pInitPos, scrsId, &core)
		// scriptType=0 → normal (hideOnComplete=true, rejectState=false)
		// IDA: runner->onHotspotShapeOrFrameFunc = caves_noop_incrReturn
		Common::SeekableReadStream *scrsStream =
			_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
							 ZmbResource(ZmbArchiveKind::kPage, static_cast<uint16>(walkScrsId)));
		if (scrsStream)
			walkSnoid->startScrsPlayback(scrsStream, true, false);
	}

	// Process completion flag to trigger phase transition.
	// IDA: caves_funcMain phase management (word_4AAEFA states)
	if (_phaseState == 2) {
		debugC(1, kZmbDebugAnimation, "Caves: phaseState 2 -> 3, playing SND 996");
		_phaseState = 3;
		// IDA: scrb_enqueueSoundResource(0, SND_00996_MOVE_LONG_SFX)
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, 996));
	}

	// [1] Glyph hint blink at difficulty 1.
	// IDA: word_4AAEF4 toggling with 30-frame timer on glyphRenderRunner.
	// At difficulty 1 with hoveredSlot < 6, the glyph renderer toggles visibility.
	// IDA: caves_funcOnHover @ 0x4175DF-0x41767F
	if (_difficultyLevel == kPuzzleDiffLevel1 && _hoveredEntranceSlot < 6) {
		if (_virtualGlyphRenderer) {
			uint32 currFrame = getCurrentFrameCounter();
			if (currFrame >= _glyphBlinkNextFrame) {
				if (_hoveredEntranceSlot != 0) {
					// Currently showing hint -> hide it
					_glyphBlinkNextFrame = currFrame + 30;
					_hoveredEntranceSlot = 0;
				} else {
					// Currently hidden -> show next hint
					_glyphBlinkNextFrame = currFrame + 30;
					_hintFlashCounter++;
					_hoveredEntranceSlot = _hintFlashCounter;
				}
				// Request redraw of glyph panel
				if (_virtualGlyphRenderer->isRenderActivated()) {
					_virtualGlyphRenderer->setNeedsRedraw(true);
				}
			}
		}
	}

	_processingFrame = false;
}

void ZoombiniPuzzleCaves::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
		if (eventCode == kZmbAnimEventM1_End) {
			// End-of-animation — check state and handle completion.
			ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);
			SnoidAnimState state = snoid->getAnimState();

			if (state == kSnoidAnimScriptReject) {
				// Reject script finished — return to idle
				snoid->setAnimState(kSnoidAnimIdle);
				snoid->setupIdleHotspots();

				// IDA: For out-of-zone walk-back (setupDoorAnimation phase 2),
				// clear tracking when the walk-back SCRS (12012) completes.
				// Door-chain rejects (SCRS 12004) are followed by event 5 → setupDoorAnimation(1),
				// so _activeDropSnoid is NOT cleared here for those.
				if (_outOfZoneDrop && snoid == _activeDropSnoid) {
					_outOfZoneDrop = false;
					_activeDropSnoid = nullptr;
					_entranceCompletionFlag = false;
				}
			} else if (state == kSnoidAnimScriptNormal) {
				// Normal script (walk into cave) finished — hide snoid
				snoid->deactivateRender();
				snoid->deactivateAnimate();
			}
		}
		// Intermediate events (0, 240-253, etc.) during SCRS playback
		// are intentionally ignored — the original caves page does not
		// register intermediate event callbacks for snoid features.
	} else {
		// SCRB feature events — dispatch based on which feature fired the event.
		// IDA: Two different callbacks exist:
		//   caves_scrbEntranceCallback (0x417A98) — door entrance/rejection SCRB animations
		//   caves_handleScriptEvent_417BF2 — glyph panel SCRB animations
		if (feature == _glyphPanelRegionFeature || feature == _entranceAnimFeatures[2]) {
			handleGlyphPanelEvent(feature, eventCode);
		} else {
			handleEntranceDoorEvent(feature, eventCode);
		}
	}
}

// =========================================================================
// Entrance callback helpers
// =========================================================================

void ZoombiniPuzzleCaves::playEntranceScript(bool isReject, int16 scrsResId) {
	// IDA: caves_playEntranceScript_417A4E
	// Loads SCRS onto the active drop snoid and starts script playback.
	// In the original, this loads SCRS onto the runner found via priority (= word_4AB04A = snoid runner).
	if (!_activeDropSnoid)
		return;

	Common::SeekableReadStream *scrsStream =
		_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, static_cast<uint16>(scrsResId)));
	if (scrsStream) {
		// IDA: snoidScript_initAndPlay(scriptType, dword_4AAF6C, scrsResId, &runner->core188)
		// scriptType=1 → reject (hideOnComplete=false, rejectState=true)
		// scriptType=0 → normal (hideOnComplete=true, rejectState=false)
		_activeDropSnoid->startScrsPlayback(scrsStream, !isReject, isReject);
	}
}

void ZoombiniPuzzleCaves::loadGlyphPanelFrame(int16 frameIdx) {
	// IDA: caves_loadScrbFrameOnRunner_4186C2
	// Loads SCRB (frameIdx + _glyphPanelScrbId) onto the glyph panel region feature.
	// Validates: feature exists, not currently rendering, frameIdx within bounds.
	if (!_glyphPanelRegionFeature)
		return;
	if (_glyphPanelRegionFeature->isRenderActivated())
		return;
	if (frameIdx > _entranceCount)
		return;

	// IDA: scrb_loadOnRunner(1, runnerIdx + caves_buttonResIdBase_4A08F0, runner)
	loadScrbOntoFeature(_glyphPanelRegionFeature, static_cast<uint16>(frameIdx + _glyphPanelScrbId));
	// The original sets onHotspotShapeOrFrameFunc = caves_handleScriptEvent_417BF2.
	// In ScummVM, onFeatureAnimEvent handles dispatch to handleGlyphPanelEvent.
}

ZmbFeature *ZoombiniPuzzleCaves::getEntranceOverlayFeature(int16 idx) const {
	// Maps 0-based entrance slot index to the corresponding glyph overlay feature.
	// IDA: caves_entranceSCRBRunnerArr_4AAFF2[slotIdx+1]
	//
	// In the original, the overlay runners exist for 1-based slot indices 5-11 and 16-20.
	// ScummVM uses 0-based indexing: 4-10 → _glyphOverlayFeatures[idx-4],
	//                                 15-19 → _extraGlyphOverlayFeatures[idx-15].
	if (4 <= idx && idx <= 10)
		return _glyphOverlayFeatures[idx - 4];
	if (15 <= idx && idx <= 19)
		return _extraGlyphOverlayFeatures[idx - 15];
	return nullptr;
}

void ZoombiniPuzzleCaves::setupDoorAnimation(int16 doorIdx) {
	// IDA: caves_setupDoorAnimation_4177FB
	// Sets up door opening/closing animations for the caves entrance sequence.
	// doorIdx: 0 = open selected entrance, 1 = open matching entrance, 2 = close/reset

	if (!_activeDropSnoid)
		return;

	// Determine which entrance overlay feature to operate on.
	// IDA: doorIdx 0 → runner_findByIndex(word_4AAF62), selects the dropped-on entrance
	//      doorIdx 1 → runner_findByIndex(word_4AAF64), selects the correct entrance
	//      doorIdx 2 → runner_findByIndex(priority),    selects the snoid itself
	ZmbFeature *doorFeature = nullptr;
	if (doorIdx == 0) {
		doorFeature = _selectedDoorOverlay;
	} else if (doorIdx == 1) {
		doorFeature = _matchingDoorOverlay;
	}
	// doorIdx == 2 operates directly on the snoid (no feature needed)

	if (doorIdx < 2 && !doorFeature)
		return;

	if (doorIdx == 0) {
		// Phase 0: Door opening animation for SELECTED entrance (where the snoid was dropped).
		// IDA: 0x41787E..0x417921

		// IDA: dword_4AAF6C = 0
		// Clear position override — no initial position for script playback.

		// Load door-open SCRB onto the entrance overlay feature.
		// IDA: scrb_loadOnRunner(1, glyphBaseId + 4*selectedIdx - 4, doorData)
		// With 0-based idx: glyphBaseId + 4 * idx
		uint16 doorScrbId = static_cast<uint16>(_glyphScrbBaseId + 4 * _selectedEntranceIdx);
		loadScrbOntoFeature(doorFeature, doorScrbId);
		// The callback for this feature is handled by onFeatureAnimEvent → handleEntranceDoorEvent.

		// IDA: runner_freeByIndex(caves_doorScrbId) — destroy old door overlay
		if (_activeDoorFeature) {
			unloadScrbFeature(_activeDoorFeature);
			_activeDoorFeature = nullptr;
		}

		// IDA: runner_linkRelativeToParent(entranceRunnerArr[selectedIdx], 1, priority)
		// NO-OP in ScummVM: Z-ordering is handled by per-frame sorted rendering.

		// Create new door overlay sub-feature with SCRB: glyphBaseId + 4*selectedIdx + 1
		// IDA: runner_registerAndAllocate(priority, 1, 0, 6, glyphBaseId + 4*selectedIdx - 3,
		//       preRenderStandard, postRenderStandard, LOOP_ANIM|PLAY_ONCE|OVERLAY)
		uint16 overlayScrbId = static_cast<uint16>(_glyphScrbBaseId + 4 * _selectedEntranceIdx + 1);
		_activeDoorFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, static_cast<uint16>(_glyphScrbBaseId)),
			overlayScrbId, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
				ZmbFeature::FLAG_04000000_OVERLAY);

		// IDA: scrb_registerHotspotGroup(0, 0, 0, 0, doorScrbId, doorData->wFeatureRunnerIdx)
		// In ScummVM, hotspot groups are parsed from the SCRB data during loadScrbFeature.
		// The overlay and door feature share rendering dispatch via onFeatureAnimEvent.

	} else if (doorIdx == 1) {
		// Phase 1: Door opening animation for MATCHING entrance (redirect after wrong placement).
		// IDA: 0x417932..0x4179E3

		// IDA: caves_setKeyRunnerSlot(hoverEntranceIdx, priority)
		// Sets the snoid's target key position for the walk-to script.
		// NOTE: In ScummVM, snoid position targeting is handled by SCRS data.
		// The original uses input_getKeyPositionFromTable to get the walk target.

		// IDA: dword_4AAF6C = word_4AAF68
		// Set position override for the matching entrance walk script.

		// Load door-open SCRB onto the matching entrance overlay feature.
		// IDA: scrb_loadOnRunner(1, glyphBaseId + 4*hoverIdx - 2, doorData)
		// With 0-based idx: glyphBaseId + 4 * idx + 2
		uint16 doorScrbId = static_cast<uint16>(_glyphScrbBaseId + 4 * _matchingEntranceIdx + 2);
		loadScrbOntoFeature(doorFeature, doorScrbId);

		// IDA: runner_freeByIndex(caves_doorScrbId) — destroy old door overlay
		if (_activeDoorFeature) {
			unloadScrbFeature(_activeDoorFeature);
			_activeDoorFeature = nullptr;
		}

		// IDA: runner_linkRelativeToParent(entranceRunnerArr[hoverIdx], 1, priority)
		// NO-OP in ScummVM.

		// Create new door overlay: glyphBaseId + 4*hoverIdx + 3
		// IDA: runner_registerAndAllocate(priority, 1, 0, 6, glyphBaseId + 4*hoverIdx - 1,
		//       preRenderStandard, postRenderStandard, LOOP_ANIM|PLAY_ONCE|OVERLAY)
		uint16 overlayScrbId = static_cast<uint16>(_glyphScrbBaseId + 4 * _matchingEntranceIdx + 3);
		_activeDoorFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, static_cast<uint16>(_glyphScrbBaseId)),
			overlayScrbId, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
				ZmbFeature::FLAG_04000000_OVERLAY);

		// IDA: scrb_registerHotspotGroup(0, 0, 0, 0, doorScrbId, doorData->wFeatureRunnerIdx)
		// Hotspot groups parsed from SCRB data in ScummVM.

	} else if (doorIdx == 2) {
		// Phase 2: Door close/reset — snoid walks back after out-of-zone drop.
		// IDA: 0x4179EC..0x417A43

		// IDA: setInteractionLock(0) — unlock interaction
		_interactionLocked = false;

		// IDA: dword_4AAF6C = 0
		// IDA: doorData[1].core188.u.h.hsArr[1].shapeid = 0
		// Clear snoid entrance tracking state.

		// IDA: doorData->onHotspotShapeOrFrameFunc = caves_scrbEntranceCallback
		// In ScummVM, the callback is handled by onFeatureAnimEvent dispatch.

		// IDA: snoidScript_initAndPlay(1, dword_4AAF6C=0, 12012, &doorData->core188)
		// Play SCRS 12012 (door close / walk-back script) on the active snoid.
		Common::SeekableReadStream *scrsStream =
			_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, 12012));
		if (scrsStream)
			_activeDropSnoid->startScrsPlayback(scrsStream, false, true);

		// IDA: scrb_registerHotspotGroup(0, 0, 0, 0, doorData->idx, doorData->idx)
		// IDA: runner_linkRelativeToParent(caves_buttonOverlayRunner, 0, doorData->idx)
		// NO-OP in ScummVM: hotspot and Z-ordering handled by the framework.
	}
}

void ZoombiniPuzzleCaves::handleEntranceDoorEvent(ZmbFeature *feature, int16 eventCode) {
	// IDA: caves_scrbEntranceCallback (0x417A98)
	// Events fired from door entrance/rejection SCRB animations.
	switch (eventCode) {
	case 1:
		// Play reject entrance SCRS on the snoid.
		// IDA: caves_playEntranceScript(1, callback, caves_rejectScrbBaseId, endFrame)
		// This event fires from the door SCRB (loaded by setupDoorAnimation phase 0)
		// to trigger the reject walk at the correct animation frame.
		if (_activeDropSnoid)
			playEntranceScript(true, _rejectScrsBaseId);
		break;

	case 2:
		// Play normal walk-into-cave SCRS on the snoid.
		// IDA: caves_playEntranceScript(0, callback, caves_rejectScrbBaseId + 1, endFrame)
		// This event fires from the door SCRB (loaded by setupDoorAnimation phase 1)
		// to trigger the walk-into-correct-entrance animation.
		if (_activeDropSnoid)
			playEntranceScript(false, _rejectScrsBaseId + 1);
		break;

	case 4:
		// Door transition complete.
		// IDA: setRunnerUpdateNeeded(), nActiveEntranceAnimCount=1,
		//      ++entranceAnimCounter, loadGlyphPanelFrame(counter), freeRunner(doorId)
		_nActiveEntranceAnimCount = 1;
		++_entranceAnimCounter;
		loadGlyphPanelFrame(_entranceAnimCounter);
		// IDA: runner_freeByIndex(caves_doorScrbId) — destroy the door overlay feature.
		if (_activeDoorFeature) {
			unloadScrbFeature(_activeDoorFeature);
			_activeDoorFeature = nullptr;
		}
		break;

	case 5:
		// Set transition pending — processed in onEveryFrame.
		// IDA: caves_bTransitionPending_4A08F6 = 1
		_bTransitionPending = true;
		break;

	case 10:
		// Position adjustment: play SCRS 12013 (walk to position script).
		// IDA: dword_4AAF6C = &dword_4AB09C[--word_4AB0EC];
		//      caves_playEntranceScript(0, callback, 12013, endFrame)
		playEntranceScript(false, 12013);
		break;

	case 20:
		// Completion check: all entrances used?
		// IDA: priority = 0; unk_4A08E0 = (caves_entranceAnimCounter == unk_4A08F2)
		_activeDropSnoid = nullptr;
		_entranceCompletionFlag = (_entranceAnimCounter == _entranceCount);
		break;

	case 21:
		// Force completion.
		// IDA: priority = 0; unk_4A08E0 = 1
		_activeDropSnoid = nullptr;
		_entranceCompletionFlag = true;
		break;

	default:
		break;
	}
}

void ZoombiniPuzzleCaves::handleGlyphPanelEvent(ZmbFeature *feature, int16 eventCode) {
	// IDA: caves_handleScriptEvent_417BF2
	// Events fired from glyph panel SCRB animations (loaded by loadGlyphPanelFrame or advance button).
	switch (eventCode) {
	case 10:
		// Phase change to animating state.
		// IDA: word_4AAEFA = 2
		_phaseState = 2;
		break;

	case 20: {
		// Check whether all entrances have been used.
		// IDA: if (entranceAnimCounter == entranceCount) → play sound, set completion
		if (_entranceAnimCounter == _entranceCount) {
			// Count active snoids to check if more remain.
			// IDA: getLoadedZmbRunnerCount_452402() < caves_nLoadedZmbCount
			int16 activeSnoidCount = 0;
			for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
				ZmbSnoid *s = *it;
				if (s->getId() >= 10000 && s->isAnimateActivated())
					++activeSnoidCount;
			}
			// IDA: if (count < nLoadedZmb && (rand(4,0) > diffLevel-1 || puzzleFlag <= 3))
			if (activeSnoidCount < _loadedZmbCount) {
				int16 rollThreshold = _difficultyLevel - 1;
				if (_vm->_rnd->getRandomNumber(0, 4) > rollThreshold ||
					(_vm->_state->_f._pageFlagCaves & 0xFFFu) <= 3) {
					uint16 soundId = static_cast<uint16>(_vm->_rnd->getRandomNumber(20045, 20048));
					_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, soundId));
				}
			}
			_entranceCompletionFlag = true;
		} else {
			_entranceCompletionFlag = false;
		}
		break;
	}

	case 21:
		// Force completion + handle practice level drag lock.
		// IDA: unk_4A08E0 = 1; if (wPracticeLevel) { dragLock=0, dragLockCounter=1 }
		_entranceCompletionFlag = true;
		if (_vm->_state->inPracticeMode()) {
			// IDA: ui_bDragLockActive = 0; ui_dragLockCounter = 1
			// In ScummVM, the drag lock is managed via the walk-in progress counter.
			// Setting walkersInProgress to 0 re-enables dragging.
			_vm->_walkersInProgress = 0;
		}
		break;

	default:
		break;
	}
}

} // End of namespace Mohawk
