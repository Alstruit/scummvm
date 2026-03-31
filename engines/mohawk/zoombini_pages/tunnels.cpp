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

// IDA: tunnel entry positions at 0x4A7674 (4 POINTS, each packed as DWORD = int16 x, int16 y)
const Common::Point ZoombiniInteractiveTunnels::kTunnelEntryPositions[4] = {
	Common::Point(98, 424), Common::Point(178, 415), Common::Point(453, 421), Common::Point(533, 430),
};

// IDA: door index mapping at 0x4A7684 — selects which of the 12 door SCRBs to use as entrance doors
const int16 ZoombiniInteractiveTunnels::kDoorIndices[4] = { 1, 2, 0, 3 };

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

	// Initialize puzzle state before anything else
	// IDA: tunnels_initPuzzleState @ 0x459C5C
	initPuzzleState();

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
	_vm->_gfx->preloadImage(4000);
	_vm->_gfx->preloadImage(4200);
	_vm->_gfx->preloadImage(4400);
	_vm->_gfx->preloadImage(4600);
	_vm->_gfx->preloadImage(5000);
	_vm->_gfx->preloadImage(6000);
	_vm->_gfx->preloadImage(7000);
	_vm->_gfx->preloadImage(9000);

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
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 12, 6000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 12; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 6000), 6000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 5, 7000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 5; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 7000), 7000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 7, 9000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 7; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 9000), 9000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(2, 39, 4000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 39; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 4000), 4000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(2, 27, 4200)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 27; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 4200), 4200 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(2, 24, 4400)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 24; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 4400), 4400 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(2, 18, 4600)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 18; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 4600), 4600 + i);
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

	// --- Puzzle-specific feature runners ---
	// IDA: word_4B7AE0 = runner_registerAndAllocate(..., 0, 9000, standard, standard, 0x8000)
	// Feedback animation runner (SCRB 9000), interval=0, flags=LOOP_ANIM
	_feedbackFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 9000), 9000, 0,
		ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// IDA: 4× scrb_drawOnRegRunnerIdxArr[i] = runner_registerAndAllocate(..., &pos[i], 6, i+5000, standard, standard, 0x108A000)
	// 4 tunnel entrance DRAW_ON_REG runners at predefined positions
	for (int16 i = 0; i < 4; i++) {
		_tunnelEntryFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 5000), 5000 + i, 6,
			kTunnelEntryPositions[i],
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER);
	}

	// IDA: word_4B7AE6 = runner_registerAndAllocate(..., 0, 6, 7001, standard, standard, 0xC180000)
	// Path effect runner (SCRB 7001)
	_pathEffectFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7001, 6,
		ZmbFeature::FLAG_08000000_REGION_TRACK | ZmbFeature::FLAG_04000000_OVERLAY |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00080000_DEFER_ANIM);

	// IDA: 4× word_4B7A18[doorIdx] = runner_registerAndAllocate(..., 0, 6, doorIdx+6000, standard, standard, 0xC180000)
	// Door animation runners — kDoorIndices maps iteration order to door SCRBs {1, 2, 0, 3}
	for (int16 i = 0; i < 4; i++) {
		int16 doorIdx = kDoorIndices[i];
		_doorAnimFeatures[doorIdx] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 6000), 6000 + doorIdx, 6,
			ZmbFeature::FLAG_08000000_REGION_TRACK | ZmbFeature::FLAG_04000000_OVERLAY |
			ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00080000_DEFER_ANIM);
	}

	// IDA: 6× runner_registerAndAllocate(..., 0, 6, 9001+i, standard, standard, 0)
	// Anonymous visual feedback runners (SCRB 9001-9006), flags=0
	for (uint16 i = 0; i < 6; i++) {
		loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 9000), 9001 + i, 6,
			ZmbFeature::FLAG_00000000_TYPE_SHAPES);
	}

	// IDA: word_4B7A16 = runner_registerAndAllocate(..., 0, 6, 7000, standard, standard, 0xD181000)
	// Main path runner (SCRB 7000) — topmost overlay
	_mainPathFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7000, 6,
		ZmbFeature::FLAG_08000000_REGION_TRACK | ZmbFeature::FLAG_04000000_OVERLAY |
		ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00001000_TOPMOST);

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

	// Generate tunnel rules based on difficulty
	generateRules();

	// Disable Go button until at least one Zoombini has entered
	setGoButtonsEnabled(false);

	// Puzzle is now active
	_puzzleActive = true;
}

void ZoombiniInteractiveTunnels::onGoButtonActivated() {
	// IDA: tunnels_onClickHandler case 2
	// Play departure SFX, start walk-off animation, then fade out when SFX finishes.
	// IDA: zmbMoveAnimation_45479D(45, 30, 670) — walk to (670, 30)
	_departXferSrcSiPage = ZMB_SI_TUNNELS_03;
	startDepartWalkAnimation(Common::Point(670, 30));
	ZoombiniInteractive::onGoButtonActivated();
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

	// Store total count
	_totalZmbCount = posIdx;
	_remainingCount = posIdx;
}

// ---------------------------------------------------------------------------
// initPuzzleState: Reset all puzzle state variables.
// IDA: tunnels_initPuzzleState @ 0x459C5C
// ---------------------------------------------------------------------------
void ZoombiniInteractiveTunnels::initPuzzleState() {
	_puzzleActive = false;
	_processingFrame = false;
	_enteredCount = 0;
	_remainingCount = 0;
	_totalZmbCount = 0;
	_allPlaced = false;
	_lastFrameSnapshot = 0;

	// Reset rule system
	_guardCount = 0;
	for (int i = 0; i < 2; i++) {
		_guards[i].sideFlag = false;
		_guards[i].condCount = 0;
		_guards[i].attrType[0] = 0;
		_guards[i].attrType[1] = 0;
		_guards[i].attrValue[0] = 0;
		_guards[i].attrValue[1] = 0;
	}

	// Reset per-gate state
	for (int gate = 0; gate < 4; gate++) {
		_wrongAttempts[gate] = 0;
		_gateOccupancy[gate] = 0;
		for (int slot = 0; slot < 16; slot++) {
			_gateSlots[gate][slot] = 0;
		}
	}

	// Random seed for level-0 gate bias
	_level0GateBias = _vm->_rnd->getRandomNumber(0, 1);
}

// ---------------------------------------------------------------------------
// generateRules: Generate tunnel rules based on difficulty level.
// ---------------------------------------------------------------------------
void ZoombiniInteractiveTunnels::generateRules() {
	switch (_difficultyLevel) {
	case 0:
		setupLevel0_singleAttr();
		break;
	case 1:
		setupLevel1_dualSingleAttr();
		break;
	case 2:
		// Level 2: Dual guards, dual attributes each (OR within category)
		// TODO: Implement setupLevel2_dualDoubleAttr()
		setupLevel1_dualSingleAttr(); // Fallback to level 1 for now
		break;
	case 3:
		// Level 3: Dual guards, cross-category attributes (AND)
		// TODO: Implement setupLevel3_crossCategoryAttr()
		setupLevel1_dualSingleAttr(); // Fallback to level 1 for now
		break;
	default:
		setupLevel0_singleAttr();
		break;
	}
}

// ---------------------------------------------------------------------------
// setupLevel0_singleAttr: Generate single-attribute rule for level 0.
// IDA: tunnels_setupLevel1_singleAttr @ 0x45C859
//
// Algorithm:
// 1. Collect all Zoombini traits
// 2. Build 20-element table of single-attribute rules (5 values x 4 categories)
// 3. Count matches for each possible rule
// 4. Find rule that splits Zoombinis closest to 50%
// 5. Randomly pick from optimal rules
// ---------------------------------------------------------------------------
void ZoombiniInteractiveTunnels::setupLevel0_singleAttr() {
	// Collect all Zoombini traits
	Common::Array<ZmbTrait> traits;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *snoid = it->second;
		if (snoid && snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			traits.push_back(snoid->_trait);
		}
	}

	if (traits.empty()) {
		// No Zoombinis, set default rule
		_guardCount = 1;
		_guards[0].sideFlag = false;
		_guards[0].condCount = 1;
		_guards[0].attrType[0] = 1; // Hair
		_guards[0].attrValue[0] = 1; // Value 1
		return;
	}

	// Build 20-element table: 4 categories x 5 values
	// Category: 1=hair(head), 2=eyes, 3=nose, 4=feet(foot)
	int16 matchCounts[20] = {};

	for (const ZmbTrait &trait : traits) {
		// Category 1: Hair (head)
		uint8 hairVal = trait._head & 0x0F;
		if (hairVal >= 1 && hairVal <= 5) {
			matchCounts[(hairVal - 1)]++;
		}

		// Category 2: Eyes
		uint8 eyeVal = trait._eye & 0x0F;
		if (eyeVal >= 1 && eyeVal <= 5) {
			matchCounts[5 + (eyeVal - 1)]++;
		}

		// Category 3: Nose
		uint8 noseVal = trait._nose & 0x0F;
		if (noseVal >= 1 && noseVal <= 5) {
			matchCounts[10 + (noseVal - 1)]++;
		}

		// Category 4: Feet
		uint8 footVal = trait._foot & 0x0F;
		if (footVal >= 1 && footVal <= 5) {
			matchCounts[15 + (footVal - 1)]++;
		}
	}

	// Find rule closest to 50% split
	int16 targetCount = traits.size() / 2;
	int16 bestSlot = 0;

	// Spiral search from target outward (IDA algorithm)
	Common::Array<int16> candidates;
	int16 step = 0;
	int16 checkVal = targetCount;

	for (int iter = 0; iter < 32 && candidates.empty(); iter++) {
		if (checkVal >= 1 && checkVal < 16) {
			// Find all slots with this match count
			for (int slot = 0; slot < 20; slot++) {
				if (matchCounts[slot] == checkVal) {
					candidates.push_back(slot);
				}
			}
		}
		// Spiral: +1, -2, +3, -4, ...
		step++;
		checkVal += (step & 1) ? step : -step;
	}

	// If no candidates found, use first non-zero slot
	if (candidates.empty()) {
		for (int slot = 0; slot < 20; slot++) {
			if (matchCounts[slot] > 0) {
				candidates.push_back(slot);
				break;
			}
		}
	}

	// Randomly pick from candidates
	if (!candidates.empty()) {
		bestSlot = candidates[_vm->_rnd->getRandomNumber(0, candidates.size() - 1)];
	}

	// Convert slot to attribute type and value
	// Slots 0-4: Hair (type 1), values 1-5
	// Slots 5-9: Eyes (type 2), values 1-5
	// Slots 10-14: Nose (type 3), values 1-5
	// Slots 15-19: Feet (type 4), values 1-5
	uint8 attrType = (bestSlot / 5) + 1;
	uint8 attrValue = (bestSlot % 5) + 1;

	// Set the rule
	_guardCount = 1;
	_guards[0].sideFlag = _vm->_rnd->getRandomNumber(0, 1) != 0;
	_guards[0].condCount = 1;
	_guards[0].attrType[0] = attrType;
	_guards[0].attrValue[0] = attrValue;

	debug(3, "Tunnels Level 0 Rule: Guard 0 side=%d, type=%d, value=%d",
	      _guards[0].sideFlag ? 1 : 0, attrType, attrValue);
}

// ---------------------------------------------------------------------------
// setupLevel1_dualSingleAttr: Generate dual single-attribute rules for level 1.
// IDA: tunnels_setupLevel2_dualSingleAttr @ 0x45CB51
// ---------------------------------------------------------------------------
void ZoombiniInteractiveTunnels::setupLevel1_dualSingleAttr() {
	// For now, generate two independent single-attribute rules
	// TODO: Implement proper pair selection with diversity/balance scoring

	// Collect all Zoombini traits
	Common::Array<ZmbTrait> traits;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *snoid = it->second;
		if (snoid && snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			traits.push_back(snoid->_trait);
		}
	}

	if (traits.empty()) {
		// Default rules
		_guardCount = 2;
		_guards[0].sideFlag = false;
		_guards[0].condCount = 1;
		_guards[0].attrType[0] = 1;
		_guards[0].attrValue[0] = 1;
		_guards[1].sideFlag = true;
		_guards[1].condCount = 1;
		_guards[1].attrType[0] = 2;
		_guards[1].attrValue[0] = 1;
		return;
	}

	// Build match count table
	int16 matchCounts[20] = {};
	for (const ZmbTrait &trait : traits) {
		uint8 vals[4] = {
			static_cast<uint8>(trait._head & 0x0F),
			static_cast<uint8>(trait._eye & 0x0F),
			static_cast<uint8>(trait._nose & 0x0F),
			static_cast<uint8>(trait._foot & 0x0F)
		};
		for (int cat = 0; cat < 4; cat++) {
			if (vals[cat] >= 1 && vals[cat] <= 5) {
				matchCounts[cat * 5 + (vals[cat] - 1)]++;
			}
		}
	}

	// Find two rules that create good 4-way split
	// Simplified: just pick two rules from different categories with good match counts
	Common::Array<int16> goodSlots;
	uint totalCount = traits.size();

	for (int slot = 0; slot < 20; slot++) {
		if (matchCounts[slot] > 0 && static_cast<uint>(matchCounts[slot]) <= totalCount) {
			goodSlots.push_back(slot);
		}
	}

	// Pick two slots, preferring different categories
	int16 slot0 = 0, slot1 = 5; // Default: hair type 1, eyes type 1

	if (goodSlots.size() >= 2) {
		// Shuffle and pick first two from different categories if possible
		slot0 = goodSlots[_vm->_rnd->getRandomNumber(0, goodSlots.size() - 1)];
		int16 cat0 = slot0 / 5;

		// Find slot from different category
		Common::Array<int16> diffCatSlots;
		for (int16 s : goodSlots) {
			if ((s / 5) != cat0) {
				diffCatSlots.push_back(s);
			}
		}

		if (!diffCatSlots.empty()) {
			slot1 = diffCatSlots[_vm->_rnd->getRandomNumber(0, diffCatSlots.size() - 1)];
		} else if (goodSlots.size() >= 2) {
			// Same category, different value
			for (int16 s : goodSlots) {
				if (s != slot0) {
					slot1 = s;
					break;
				}
			}
		}
	}

	// Set guard 0
	_guardCount = 2;
	_guards[0].sideFlag = _vm->_rnd->getRandomNumber(0, 1) != 0;
	_guards[0].condCount = 1;
	_guards[0].attrType[0] = (slot0 / 5) + 1;
	_guards[0].attrValue[0] = (slot0 % 5) + 1;

	// Set guard 1
	_guards[1].sideFlag = _vm->_rnd->getRandomNumber(0, 1) != 0;
	_guards[1].condCount = 1;
	_guards[1].attrType[0] = (slot1 / 5) + 1;
	_guards[1].attrValue[0] = (slot1 % 5) + 1;

	debug(3, "Tunnels Level 1 Rules: Guard 0 type=%d val=%d, Guard 1 type=%d val=%d",
	      _guards[0].attrType[0], _guards[0].attrValue[0],
	      _guards[1].attrType[0], _guards[1].attrValue[0]);
}

// ---------------------------------------------------------------------------
// evaluateRule: Check if a Zoombini matches a tunnel rule.
// IDA: tunnels_evalAttrRule @ 0x45C65D
//
// For level 0 (1 guard): Simple match check
// For level 1+ (2 guards): Zone determines combination:
//   Zone 1: matchA (accepts if guard A matches)
//   Zone 2: matchA AND NOT matchB
//   Zone 3: NOT matchA AND NOT matchB
//   Zone 4: NOT matchA AND matchB
// ---------------------------------------------------------------------------
bool ZoombiniInteractiveTunnels::evaluateRule(ZmbSnoid *snoid, int16 dropZone) {
	if (!snoid || dropZone < 1 || dropZone > 4) {
		return false;
	}

	// Get Zoombini's traits
	uint8 traitVals[5] = {0}; // Index 0 unused, 1=hair, 2=eyes, 3=nose, 4=feet
	traitVals[1] = snoid->_trait._head & 0x0F;  // Hair
	traitVals[2] = snoid->_trait._eye & 0x0F;   // Eyes
	traitVals[3] = snoid->_trait._nose & 0x0F;  // Nose
	traitVals[4] = snoid->_trait._foot & 0x0F;  // Feet

	// Evaluate guard 0
	bool matchA = false;
	if (_guardCount >= 1 && _guards[0].condCount >= 1) {
		uint8 type = _guards[0].attrType[0];
		uint8 value = _guards[0].attrValue[0];
		if (type >= 1 && type <= 4 && traitVals[type] == value) {
			matchA = true;
		}
		// Apply side flag (negation)
		if (!_guards[0].sideFlag) {
			matchA = !matchA;
		}
	}

	// Level 0: Only 1 guard, 2 zones
	if (_guardCount == 1) {
		// Zone 1 and 2 are the only valid zones for level 0
		// Zone 1: match, Zone 2: no match (or vice versa based on random)
		if (dropZone == 1) {
			return matchA;
		} else if (dropZone == 2) {
			return !matchA;
		}
		return false;
	}

	// Level 1+: 2 guards, 4 zones
	bool matchB = false;
	if (_guardCount >= 2 && _guards[1].condCount >= 1) {
		uint8 type = _guards[1].attrType[0];
		uint8 value = _guards[1].attrValue[0];
		if (type >= 1 && type <= 4 && traitVals[type] == value) {
			matchB = true;
		}
		// Apply side flag (negation)
		if (!_guards[1].sideFlag) {
			matchB = !matchB;
		}
	}

	// Zone logic (IDA: a3 parameter in tunnels_evalAttrRule)
	switch (dropZone) {
	case 1: // matchA
		return matchA;
	case 2: // matchA AND NOT matchB
		return matchA && !matchB;
	case 3: // NOT matchA AND NOT matchB
		return !matchA && !matchB;
	case 4: // NOT matchA AND matchB
		return !matchA && matchB;
	default:
		return false;
	}
}

// ---------------------------------------------------------------------------
// getDropZone: Find which tunnel zone a position corresponds to.
// ---------------------------------------------------------------------------
int16 ZoombiniInteractiveTunnels::getDropZone(const Common::Point &pos) {
	// Check each tunnel entry position
	for (int16 i = 0; i < 4; i++) {
		int16 dx = pos.x - kTunnelEntryPositions[i].x;
		int16 dy = pos.y - kTunnelEntryPositions[i].y;
		int32 distSq = dx * dx + dy * dy;

		if (distSq <= kClickZoneRadius * kClickZoneRadius) {
			return i + 1; // Zones are 1-indexed
		}
	}
	return 0; // No zone hit
}

// ---------------------------------------------------------------------------
// handleZoombiniPlacement: Process a Zoombini being placed in a tunnel.
// ---------------------------------------------------------------------------
void ZoombiniInteractiveTunnels::handleZoombiniPlacement(ZmbSnoid *snoid, int16 zone, bool isCorrect) {
	if (!snoid || zone < 1 || zone > 4) {
		return;
	}

	int16 gateIdx = zone - 1;

	if (isCorrect) {
		// Correct placement
		_wrongAttempts[gateIdx] = 0;
		_enteredCount++;
		_remainingCount--;

		// Add to gate slot
		if (_gateOccupancy[gateIdx] < 16) {
			_gateSlots[gateIdx][_gateOccupancy[gateIdx]] = snoid->getId();
			_gateOccupancy[gateIdx]++;
		}

		// Play success sound
		ZmbResource successSound(ZmbArchiveKind::kPage, 8500 + _vm->_rnd->getRandomNumber(0, 64));
		_vm->_sound->playZmbSound(successSound, Audio::Mixer::kSFXSoundType);

		// Hide the Zoombini (it entered the tunnel)
		snoid->deactivateRender();

		// Check if all Zoombinis placed
		if (_remainingCount <= 0) {
			_allPlaced = true;
			debug(3, "Tunnels: All Zoombinis placed!");
		}
	} else {
		// Wrong placement
		_wrongAttempts[gateIdx]++;

		// Play rejection sound
		ZmbResource rejectSound(ZmbArchiveKind::kPage, 8000 + _vm->_rnd->getRandomNumber(0, 7));
		_vm->_sound->playZmbSound(rejectSound, Audio::Mixer::kSFXSoundType);

		// Return Zoombini to original position
		snoid->setAnimState(kSnoidAnimIdle);
	}
}

// ---------------------------------------------------------------------------
// onEveryFrame: Main per-frame logic.
// IDA: tunnels_onFrameTick @ 0x45A460
// ---------------------------------------------------------------------------
void ZoombiniInteractiveTunnels::onEveryFrame() {
	if (_processingFrame || !_puzzleActive) {
		return;
	}
	_processingFrame = true;

	// Check for pending departure
	if (_pendingGoDepart) {
		_processingFrame = false;
		return;
	}

	// Check if all Zoombinis have been placed
	if (_allPlaced && !_pendingGoDepart) {
		// Enable Go button
		setGoButtonsEnabled(true);
	}

	// Process any pending animation queues
	// TODO: Implement animation queue processing

	_processingFrame = false;
}

// ---------------------------------------------------------------------------
// onLButtonDown: Handle mouse button press.
// ---------------------------------------------------------------------------
ZmbEventHandleResult ZoombiniInteractiveTunnels::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// First check base class handling (buttons, etc.)
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed) {
		return result;
	}

	// Check if clicking on a Zoombini to start drag
	ZmbSnoid *snoid = findSnoidAtPoint(absPos);
	if (snoid && snoid->getAnimState() == kSnoidAnimIdle) {
		startSnoidDrag(snoid, absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

// ---------------------------------------------------------------------------
// onLButtonUp: Handle mouse button release.
// ---------------------------------------------------------------------------
ZmbEventHandleResult ZoombiniInteractiveTunnels::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	// Check if we were dragging a Zoombini
	if (isDragging()) {
		ZmbSnoid *snoid = finishSnoidDrag();
		if (snoid) {
			// Check which drop zone we're in
			int16 zone = getDropZone(absPos);

			if (zone > 0) {
				// For level 0, only zones 1 and 2 are valid
				if (_difficultyLevel == 0 && zone > 2) {
					zone = 0; // Invalid zone for level 0
				}

				if (zone > 0) {
					// Evaluate the rule
					bool isCorrect = evaluateRule(snoid, zone);

					// Level 0 bias: first-try success on appropriate gate
					if (_difficultyLevel == 0 && _enteredCount == 0) {
						// Force success on first attempt
						isCorrect = true;
					}

					handleZoombiniPlacement(snoid, zone, isCorrect);
					return ZmbEventHandleResult::kConsumed;
				}
			}

			// Dropped outside valid zones - return to original position
			snoid->setPointLoc(_dragOrigPos);
			snoid->setAnimState(kSnoidAnimIdle);
		}
		return ZmbEventHandleResult::kConsumed;
	}

	return ZoombiniInteractive::onLButtonUp(absPos, relPos);
}

} // End of namespace Mohawk
