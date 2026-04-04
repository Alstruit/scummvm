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
#include "mohawk/zoombini_random.h"
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
	// IDA: caves_funcInit (0x416978) has no music playback call on page load.
	// sound_activeHandle = 20065 is stored at end of funcInit for F1 replay only.
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
	_vm->_gfx->preloadImage(6000);
	_vm->_gfx->preloadImage(7000);
	_vm->_gfx->preloadImage(8200);
	_vm->_gfx->preloadImage(9000);
	_vm->_gfx->preloadImage(9025);

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
	// caves_clearAndInvalidateRect as preRender, caves_renderAllEntranceGlyphs as render
	// For now, create a placeholder loop animation feature
	_virtualGlyphRenderer = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 6000), 6000, 0,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00020000_SKIP_RENDER);

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
	_successCount = 0;
	_consecutiveCorrect = 0;
	_rejectAnimActive = false;
	_interactionLocked = false;
	_hintFlashEnabled = false;
	_nextFidgetFrame = getCurrentFrameCounter() + 120;
	_fidgetPlayedCount = 0;
	_fidgetTargetCount = 3;
}

void ZoombiniInteractiveCaves::onGoButtonActivated() {
	// IDA: caves_onClickHandler case 2 -> puzzle_pendingTransitionTarget = 17
	// Route 4: Caves -> Smoke (via Xfer)
	_departXferSrcSiPage = ZMB_SI_CAVES_14;
	ZoombiniInteractive::onGoButtonActivated();
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

void ZoombiniInteractiveCaves::initDifficultyParams() {
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
	case 1:
		_entranceCount = 4;
		_glyphPanelScrbId = 6006;
		break;
	case 2:
		_entranceCount = 5;
		_glyphPanelScrbId = 6005;
		break;
	case 3:
		_entranceCount = 6;
		_glyphPanelScrbId = 6004;
		break;
	case 4:
	default:
		_entranceCount = 7;
		_glyphPanelScrbId = 6003;
		break;
	}
}

void ZoombiniInteractiveCaves::setupEntranceGlyphs() {
	// IDA: caves_glyphSetupDispatch_418A6E
	// Calls three sub-functions to setup the glyph pattern system:
	initEntranceAttrPattern();
	countGlyphDistribution();
	buildGlyphTimingTable();
	
	// IDA: caves_entranceAttrDist_418CB1 — distribute attributes to entrances
	distributeEntranceAttributes();
}

void ZoombiniInteractiveCaves::initEntranceAttrPattern() {
	// IDA: caves_initEntranceAttrPattern_418A7E
	// Initializes random attribute patterns using Fisher-Yates shuffle.
	
	// Guard complexity: 1 or 2 based on difficulty
	// IDA: unk_4A08E4 = (word_4AAF00 <= 2) ? 1 : 2
	_guardComplexity = (_difficultyLevel <= 2) ? 1 : 2;
	
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
	int16 attrPoolSize = 3;  // Initially 4 attributes (0-3), but we pick with removal
	
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

void ZoombiniInteractiveCaves::countGlyphDistribution() {
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
	_guardComplexity = (_difficultyLevel <= 2) ? 1 : 2;
	
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
			static_cast<uint8>(traits._foot)
		};
		
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

void ZoombiniInteractiveCaves::buildGlyphTimingTable() {
	// IDA: caves_buildGlyphTimingTable_418F6C
	// Builds timing tables for glyph animations.
	
	// Clear timing arrays
	for (int i = 0; i < 21; i++) {
		_frameToSlotMap[i] = 0;
		_crossProductTable[i] = 0;
	}
	
	// Build frame-to-slot map from attribute columns and distribution
	int timingIdx = 21 - _loadedZmbCount;  // IDA: dword_4A08FC
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
				int16 rowSlot = _attrColumns[5 + col];  // Second row
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

void ZoombiniInteractiveCaves::distributeEntranceAttributes() {
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
	case 1:
		// Level 1: All first 5 entrances active
		for (int slot = 1; slot < 6; slot++) {
			_entranceAttrReq[slot] = 1;
		}
		break;
		
	case 2: {
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
	
	case 3: {
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
				_entranceAttrOffset[slot] = slot;  // Hair
				break;
			case 1:
				_entranceAttrOffset[slot] = slot + 5;  // Eyes
				break;
			case 2:
				_entranceAttrOffset[slot] = slot + 10;  // Nose
				break;
			case 3:
				_entranceAttrOffset[slot] = slot + 15;  // Feet
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
				_entranceAttrOffset[slot] = (slot - 5);  // Hair
				break;
			case 1:
				_entranceAttrOffset[slot] = (slot - 5) + 5;  // Eyes
				break;
			case 2:
				_entranceAttrOffset[slot] = (slot - 5) + 10;  // Nose
				break;
			case 3:
				_entranceAttrOffset[slot] = (slot - 5) + 15;  // Feet
				break;
			default:
				_entranceAttrOffset[slot] = (slot - 5);
				break;
			}
		}
	}
}

// =========================================================================
// Gameplay methods
// =========================================================================

int16 ZoombiniInteractiveCaves::getEntranceSlotAtPoint(const Common::Point &pos) const {
	// Check each active entrance's hit rect
	for (int16 i = 0; i < 20; i++) {
		if (_entranceHitRects[i].contains(pos.x, pos.y))
			return i;
	}
	return -1;
}

int16 ZoombiniInteractiveCaves::findMatchingGlyphSlot(const ZmbTrait &traits) const {
	// IDA: caves_findMatchingGlyphSlot
	// Determines which cave entrance matches a Zoombini's attributes.
	// Uses _baseAttrTypes[0] as the primary attribute to match against entrance columns.

	uint8 traitBytes[4] = {
		static_cast<uint8>(traits._head),
		static_cast<uint8>(traits._eye),
		static_cast<uint8>(traits._nose),
		static_cast<uint8>(traits._foot)
	};

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

void ZoombiniInteractiveCaves::handleCorrectPlacement(ZmbSnoid *snoid, int16 entranceSlot) {
	// IDA: caves correct placement handler
	// Zoombini enters the correct cave.

	_successCount++;
	_consecutiveCorrect++;

	// Play walk-into-cave animation via SCRS 13000+ (normal pool)
	uint16 scrsIdx = 13000 + (entranceSlot % 5);
	Common::SeekableReadStream *scrsStream =
		_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, scrsIdx));
	if (scrsStream) {
		snoid->startScrsPlayback(scrsStream, true, false);
	}

	// Play entrance door animation — load door open SCRB onto the door feature
	if (entranceSlot < 20 && _doorDrawOnRegFeatures[entranceSlot]) {
		loadScrbOntoFeature(_doorDrawOnRegFeatures[entranceSlot], 7000 + entranceSlot);
	}

	// Play positive sound
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20066));

	// Check if all Zoombinis have been placed
	int16 remainingCount = 0;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if ((*it)->getId() < 10000)
			continue;
		ZmbSnoid *s = *it;
		if (s->getAnimState() == kSnoidAnimIdle && s->_packIsOccupied)
			remainingCount++;
	}

	if (remainingCount <= 0) {
		// All Zoombinis placed — enable Go button
		setGoButtonsEnabled(true);
	}
}

void ZoombiniInteractiveCaves::handleWrongPlacement(ZmbSnoid *snoid, int16 droppedSlot, int16 correctSlot) {
	// IDA: caves wrong placement handler
	// Door rejects the Zoombini and optionally shows a hint.

	_interactionLocked = true;
	_rejectAnimActive = true;
	_rejectSnoid = snoid;
	_rejectTargetSlot = droppedSlot;
	_consecutiveCorrect = 0;

	// Play reject SCRS from reject pool (12000+)
	uint16 scrsIdx = 12000 + _vm->_rnd->getRandomNumber(0, 13);
	Common::SeekableReadStream *scrsStream =
		_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, scrsIdx));
	if (scrsStream) {
		snoid->startScrsPlayback(scrsStream, false, true);
	}

	// Play door rejection animation — reload door panel feature showing rejection
	if (droppedSlot < 4 && _doorPanelFeatures[droppedSlot]) {
		loadScrbOntoFeature(_doorPanelFeatures[droppedSlot], 9011 + droppedSlot);
	}

	// Play rejection sound
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, 20067));

	// Enable hint flash at difficulty level 1
	if (_difficultyLevel == 1 && correctSlot >= 0) {
		_hintFlashEnabled = true;
		_hintFlashSlot = correctSlot;
		_hintFlashStartFrame = getCurrentFrameCounter();
	}
}

void ZoombiniInteractiveCaves::endDrag(const Common::Point &dropPos) {
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
		// Dropped outside any entrance — return to idle
		if (!validateTerrainDrop(snoid)) {
			snoid->setPointLoc(_dragOrigPos);
		}
		snoid->setAnimState(kSnoidAnimIdle);
		snoid->setupIdleHotspots();
	}
}

ZmbEventHandleResult ZoombiniInteractiveCaves::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// Sticky mouse: second click ends drag
	if (isDragging() && _vm->_state->getEnableStickyMouse()) {
		endDrag(absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	// Let base class handle button clicks (Go/Map/Help)
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Don't allow interaction while locked (reject animation playing)
	if (_interactionLocked || !_puzzleActive)
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

ZmbEventHandleResult ZoombiniInteractiveCaves::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	if (!isDragging())
		return ZoombiniInteractive::onLButtonUp(absPos, relPos);

	// Sticky mouse: button-up does NOT end drag
	if (_vm->_state->getEnableStickyMouse())
		return ZmbEventHandleResult::kConsumed;

	endDrag(absPos);
	return ZmbEventHandleResult::kConsumed;
}

void ZoombiniInteractiveCaves::onEveryFrame() {
	if (_processingFrame || !_puzzleActive)
		return;
	_processingFrame = true;

	// [0] Pending Go departure: skip normal logic
	if (_pendingGoDepart) {
		_processingFrame = false;
		return;
	}

	// [1] Process reject animation completion
	if (_rejectAnimActive && _rejectSnoid) {
		SnoidAnimState state = _rejectSnoid->getAnimState();
		if (state == kSnoidAnimIdle) {
			// Reject animation finished — return snoid to idle position
			_rejectSnoid->setupIdleHotspots();
			_rejectSnoid = nullptr;
			_rejectAnimActive = false;
			_interactionLocked = false;
			_rejectTargetSlot = -1;
		}
	}

	// [2] Hint flash timeout (level 1 only)
	if (_hintFlashEnabled) {
		uint32 elapsed = getCurrentFrameCounter() - _hintFlashStartFrame;
		if (elapsed > 90) {
			_hintFlashEnabled = false;
			_hintFlashSlot = -1;
		}
	}

	// [3] Fidget scheduling for idle Zoombinis
	if (_fidgetPlayedCount < _fidgetTargetCount &&
		getCurrentFrameCounter() > _nextFidgetFrame) {

		_nextFidgetFrame = getCurrentFrameCounter() + _vm->_rnd->getRandomNumber(60, 180);
		bool triggered = false;
		int16 attempts = 0;

		do {
			attempts++;
			uint16 poolIdx = _vm->_rnd->getRandomNumber(0, 19);
			uint16 snoidId = 10000 + poolIdx;

			ZmbSnoid *snoid = getSnoid(snoidId);
			if (snoid && snoid->getAnimState() == kSnoidAnimIdle &&
				snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
				Common::SeekableReadStream *scrsStream =
					_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
						ZmbResource(ZmbArchiveKind::kPage, 13000 + _vm->_rnd->getRandomNumber(0, 4)));
				if (scrsStream) {
					snoid->startScrsPlayback(scrsStream, false, true);
					_fidgetPlayedCount++;
					triggered = true;
				}
			}
		} while (!triggered && attempts < 16);
	}

	_processingFrame = false;
}

void ZoombiniInteractiveCaves::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
		// Snoid animation event — check if it's a reject completing
		ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);
		SnoidAnimState state = snoid->getAnimState();

		if (state == kSnoidAnimScriptReject) {
			// Reject script finished — return to idle
			snoid->setAnimState(kSnoidAnimIdle);
			snoid->setupIdleHotspots();
		} else if (state == kSnoidAnimScriptNormal) {
			// Normal script (walk into cave) finished — hide snoid
			snoid->deactivateRender();
			snoid->deactivateAnimate();
		}
	} else {
		// SCRB feature event — door/panel animation completed
		// Check if it's a door panel rejection animation
		for (int i = 0; i < 4; i++) {
			if (feature == _doorPanelFeatures[i]) {
				// Door panel animation done — reset
				return;
			}
		}
	}
}

} // End of namespace Mohawk
