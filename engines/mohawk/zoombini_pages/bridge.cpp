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

#include "mohawk/mohawk.h"
#include "mohawk/sound.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_pages/bridge.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// Snoid position table for 16 Zoombinis on the left bank.
// IDA: unk_4A07B0 (16 POINTS as x,y int16 pairs)
const Common::Point ZoombiniInteractiveBridge::kSnoidPositions[16] = {
	Common::Point(176, 304), Common::Point(169, 327), Common::Point(144, 283), Common::Point(147, 355),
	Common::Point(124, 318), Common::Point(119, 379), Common::Point(108, 284), Common::Point( 99, 345),
	Common::Point( 88, 414), Common::Point( 69, 262), Common::Point( 79, 303), Common::Point( 78, 370),
	Common::Point( 61, 346), Common::Point( 45, 301), Common::Point( 36, 359), Common::Point( 30, 404),
};

// Bridge segment feature positions (2 entries).
// IDA: dword_4A07F0 / dword_4A07F4
const Common::Point ZoombiniInteractiveBridge::kSegmentPositions[2] = {
	Common::Point(116, 104),
	Common::Point(128, 203),
};

// Lane 1 (top) arrival positions for Zoombinis (16 entries).
// IDA: unk_4A0718
const Common::Point ZoombiniInteractiveBridge::kLane1Positions[16] = {
	Common::Point(618,  45), Common::Point(582,  49), Common::Point(552,  36), Common::Point(524,  32),
	Common::Point(493,  25), Common::Point(464,  27), Common::Point(422,  36), Common::Point(618,  86),
	Common::Point(588,  81), Common::Point(556,  76), Common::Point(615, 129), Common::Point(580, 122),
	Common::Point(550, 116), Common::Point(522, 112), Common::Point(493, 106), Common::Point(530,  69),
};

// Lane 2 (bottom) arrival positions for Zoombinis (16 entries).
// IDA: unk_4A0758
const Common::Point ZoombiniInteractiveBridge::kLane2Positions[16] = {
	Common::Point(615, 342), Common::Point(590, 332), Common::Point(579, 303), Common::Point(549, 290),
	Common::Point(522, 281), Common::Point(492, 271), Common::Point(621, 314), Common::Point(602, 283),
	Common::Point(573, 267), Common::Point(533, 248), Common::Point(622, 257), Common::Point(596, 242),
	Common::Point(561, 235), Common::Point(621, 197), Common::Point(594, 187), Common::Point(566, 178),
};

// Level 1 dual-nibble combo table (10 entries).
// IDA: unk_4A0808 (qmemcpy v26, &unk_4A0808)
// Each entry encodes 2 attribute types as packed nibbles:
// 0x12 = foot+nose, 0x13 = foot+eye, 0x14 = foot+head, 0x15 = foot+hair,
// 0x23 = nose+eye, 0x24 = nose+head, 0x25 = nose+hair,
// 0x34 = eye+head, 0x35 = eye+hair, 0x45 = head+hair
static const uint32 kLevel1ComboTable[10] = {
	0x12, 0x13, 0x14, 0x15, 0x23, 0x24, 0x25, 0x34, 0x35, 0x45,
};

// Level 2 base offset table (6 entries).
// IDA: unk_4A0830 (v25)
static const uint32 kLevel2BaseTable[6] = {
	0x00000001, 0x00000001, 0x00000001, 0x00000100, 0x00000100, 0x00010000,
};

// Level 2 step offset table (6 entries).
// IDA: unk_4A0848 (attrStepTable)
static const uint32 kLevel2StepTable[6] = {
	0x00000100, 0x00010000, 0x01000000, 0x00010000, 0x01000000, 0x01000000,
};

// Drag constraint rect for Zoombini (left bank area).
// IDA: unk_4A07A8 = { 0, 0, 280, 480 }
const Common::Rect ZoombiniInteractiveBridge::kDragConstraint = Common::Rect(0, 0, 280, 480);

ZoombiniInteractiveBridge::ZoombiniInteractiveBridge(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kBridge) {
}

ZoombiniInteractiveBridge::~ZoombiniInteractiveBridge() {
}

const Common::Rect &ZoombiniInteractiveBridge::getDragConstraintRect() const {
	return kDragConstraint;
}

void ZoombiniInteractiveBridge::open() {
	openArchive(ZMB_MHK_BRIDGE);
}

void ZoombiniInteractiveBridge::setBackgroundMusic() {
	// Bridge intentionally has no dedicated BGM in the original game.
	// IDA: bridge_initPuzzleState (0x414C83) has no call to playBgm/loadBgmTrack.
	// Ambient audio comes from water/troll SCRS animations.
}

void ZoombiniInteractiveBridge::setBackgroundBitmap() {
	_vm->_gfx->setPalette(kResBackground1000);
	_vm->_gfx->drawBackground(kResBackground1000);
}

void ZoombiniInteractiveBridge::loadFeatures() {
	// --- Initialize puzzle state ---
	// IDA: bridge_initPuzzleState_414C83
	_anyZmbCrossed = 0;
	_isActive = 0;
	_routeLevel = _vm->_state->readActivePageRouteLevel();
	_puzzleReady = false;
	_reqAttrCount = 0;
	memset(_reqAttrTypes, 0, sizeof(_reqAttrTypes));
	memset(_reqAttrValues, 0, sizeof(_reqAttrValues));
	_trollSlot = 0;
	_successCount = 0;
	_bridgeTransitCount = 0;
	_isRejectPlaying = 0;
	_currentMatchResult = 0;
	_currentDropLane = 0;
	_trailLength = 0;
	memset(_trailDropZone, 0, sizeof(_trailDropZone));
	memset(_trailRunnerIdx, 0, sizeof(_trailRunnerIdx));
	memset(_trailMatchResult, 0, sizeof(_trailMatchResult));
	memset(_lane1ZmbIds, 0, sizeof(_lane1ZmbIds));
	memset(_lane2ZmbIds, 0, sizeof(_lane2ZmbIds));
	_lane1Count = 0;
	_lane2Count = 0;
	_isDragging = 0;
	_activeLaneScrb = -1;
	_activeRejectScrb = -1;
	_trollAttrState = 0;
	_crossingHotspotIdx = 0;
	_pendingLaneEvent = 0;
	_newArrivalReady = 0;
	_trollAnimPending = 0;
	_fidgetScheduleCount = 0;
	_fidgetPlayedCount = 0;
	_fidgetTimer = 0;
	_fidgetInterval = 60;
	_fidgetPoolCursor = 0;

	// Preload images (feature groups)
	_vm->_gfx->preloadImage(kResBitmapShape1100);
	_vm->_gfx->preloadImage(kResBitmapShape1200);
	_vm->_gfx->preloadImage(kResBitmapShape1300);

	// Load terrain barrier bitmap (tBMP 1600) for walkability checks.
	// IDA: rmap_loadTerrainArchive(0x640) — 160x120 mask, pixel==1 means walkable.
	loadTerrainBitmap(kResBitmapTerrain1600);

	// [*] SCRB 1100: Main bridge feature
	// IDA: loadMainFeatureSCRB_45FD3F(7, 1100)
	ZmbFeature *mainFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb1100_Main, 0,
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// [*] SCRB 1200-1248: Troll/bridge animation sub-features (49 chained from main)
	// IDA: loadSubFeatureSCRB_45FE2C(10, 49, 0x4B0)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 49; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1200),
				kResScrb1200_TrollLane1 + i);
		}
	}

	// [*] SCRB 1300-1301: Bridge segment sub-features (2 chained from main)
	// IDA: loadSubFeatureSCRB_45FE2C(0, 2, 0x514)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 2; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1300),
				kResScrb1300_Segment0 + i);
		}
	}

	// [*] SCRS Reject pool: 20 reject scripts (SCRS 1000-1019)
	// IDA: loadSCRS_RejectPool_4524AF(20, 20, 1000)
	for (uint16 i = 0; i < kBridgeRejectScrsCount; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100),
				  kResScrs1000_RejectBase + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// [*] SCRS Normal pool: 5 normal scripts (SCRS 2000-2004)
	// IDA: loadSCRS_NormalPool_45258E(5, 25, 2000)
	for (uint16 i = 0; i < kBridgeNormalScrsCount; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100),
				  kResScrs2000_NormalBase + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// [*] Bridge segment SCRB features at predefined positions (2 entries)
	// IDA: registerSCRB_45F60C loop for v0=0..1, SCRB 1300+v0, at kSegmentPositions
	for (uint16 i = 0; i < 2; i++) {
		loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100),
			kResScrb1300_Segment0 + i, 7,
			kSegmentPositions[i],
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER);
	}

	// [*] Troll gate SCRB (SCRB 1105 = 0x451): main troll feature
	// IDA: word_4AAE6A = registerSCRB_45F60C(0,0,0, 6, 0x451, ..., flags)
	loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100),
		kResScrb1105_Overlay, 0,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00040000_CHAIN_SCRIPT |
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_01000000_DEFER_RENDER | ZmbFeature::FLAG_08000000_REGION_TRACK);

	// [*] Troll animations (SCRB 1202=0x4B2, 1201=0x4B1, 1200=0x4B0)
	// IDA: word_4AAE66, word_4AAE64, word_4AAE68
	loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1200),
		kResScrb1202_TrollGate, 0,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_08000000_REGION_TRACK);

	loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1200),
		kResScrb1201_TrollLane2, 0,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_08000000_REGION_TRACK);

	loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1200),
		kResScrb1200_TrollLane1, 0,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_08000000_REGION_TRACK);

	// [*] Overlay SCRBs (1100-1105, except 1103 handled specially)
	// IDA: for i=1100..1105
	for (uint16 i = kResScrb1100_Main; i <= kResScrb1105_Overlay; i++) {
		if (i == kResScrb1103_Overlay) {
			// Water overlay with PLAY_ONCE flag
			loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100),
				i, 0,
				ZmbFeature::FLAG_00100000_PLAY_ONCE);
		} else if (i != kResScrb1100_Main && i != kResScrb1105_Overlay) {
			// Standard overlays (1101, 1102, 1104)
			loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100),
				i, 0,
				ZmbFeature::FLAG_00000000_TYPE_SHAPES);
		}
		// 1100 and 1105 already loaded above
	}

	// [*] Water animation SCRB 1106 (0x452)
	// IDA: registerSCRB_45F60C(0, 0, 0, 0, 0x452, ..., LOOP_ANIM)
	loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100),
		kResScrb1106_Water, 0,
		ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// [*] Load Zoombinis from active pack at predefined positions
	// IDA: setPosToZmbFeatureRunners_45F8DC(1, posData, 16)
	loadZoombinisFromPack();

	// Apply 75/25 walk-in split and stagger timing.
	// IDA: zmb_layoutStaticAndWalkInGroups(0) + gfx_renderFrame() + zmb_assignStaggeredWalkDelays(0, 45)
	layoutStaticAndWalkIn();
	assignStaggeredWalkDelays();

	// Build the attribute toll table
	// IDA: bridge_buildAttrTollTable_4160EF()
	buildAttrTollTable();

	// Record total pack Zoombini count (IDs 10000+, excludes template snoids)
	_totalZmbCount = 0;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if (it->first >= 10000)
			_totalZmbCount++;
	}

	// Store feature handles for troll animation manipulation.
	// IDA: word_4AAE68 = 0x4B0, word_4AAE64 = 0x4B1, etc.
	_scrbTrollLane1Idx = kResScrb1200_TrollLane1;
	_scrbTrollLane2Idx = kResScrb1201_TrollLane2;
	_scrbTrollGateIdx  = kResScrb1202_TrollGate;
	_scrbTrollMainIdx  = kResScrb1105_Overlay;
	_scrbWaterIdx      = kResScrb1106_Water;
	_scrbSegmentIdx[0] = kResScrb1300_Segment0;
	_scrbSegmentIdx[1] = kResScrb1301_Segment1;

	// [*] Buttons: Go, Map, Help
	// IDA: setButtonRunner_46B910(-16384, 1, (CButtonRunner *)&off_4A06F4)
	// Map button (buttonIdx 1): shapes 5/6 from bridge SHPL
	// Go button (buttonIdx 2): shapes 1(disabled)/2(enabled)/3(pressed) from bridge SHPL
	// Help button (buttonIdx 3): system shapes 24/25
	setGoButton(kGoButtonRect, 1, 2, 3);
	setMapButton(kMapButtonRect, 5, 6);
	setHelpButton(kHelpButtonRect);
	loadGoMapButtonsFeature(1400);
	loadHelpButtonFeature();

	// Mark as active
	_isActive = 1;

	// Play move sound
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, kResSound997_MoveSFX), Audio::Mixer::kSFXSoundType);

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagBridge);
}

void ZoombiniInteractiveBridge::loadZoombinisFromPack() {
	ZmbStateFile &f = _vm->_state->_f;
	uint16 posIdx = 0;

	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount && posIdx < 16; i++) {
		ZmbStateActiveEntry &entry = f._zmbPackActive._entries[i];
		// IDA: zmb_loadAnimationsFromActivePack filter for animFlags=0:
		// bIsOccupied && !bSkipOccupiedAnim (bridge always has bSkipOccupiedAnim=0).
		// Use _bIsOccupied like original, not trait completeness.
		if (!entry._bIsOccupied)
			continue;

		Common::Point pos = kSnoidPositions[posIdx];
		uint16 snoidId = 10000 + posIdx;  // Use a distinct range for pack snoids

		ZmbSnoid *snoid = loadSnoidFromPack(snoidId, pos,
											ZmbFeature::FLAG_00000001_TYPE_SNOID);
		if (snoid) {
			snoid->_trait = entry._traits;
			snoid->_name = entry.getU32Name(_vm);
			snoid->_packIsOccupied = true;  // IDA: pZmb.unk00F7 = bIsOccupied (snoid->_packIsOccupied)
			snoid->setupIdleHotspots();
		}

		posIdx++;
	}
}

void ZoombiniInteractiveBridge::onGoButtonActivated() {	// IDA: bridge_funcOnClick_4157EB case 2
	// Play departing SFX and start walk-off animation, then fade out when SFX finishes.
	if (_anyZmbCrossed) {
		_departXferSrcSiPage = ZMB_SI_BRIDGE_02;

		// IDA: zmbMoveAnimation_45479D(45, 316, 680) — walk to (680, 316), stagger 45
		startDepartWalkAnimation(Common::Point(680, 316));
		ZoombiniInteractive::onGoButtonActivated();
	}
}

int16 ZoombiniInteractiveBridge::collectZmbAttrPacked(Common::Array<uint32> &outTraits) const {
	// IDA: collectZmbAttrBytes_4552FE
	// Collects attribute bytes from all loaded Zoombini snoids into packed DWORDs.
	// Layout: byte0=foot, byte1=nose, byte2=eye, byte3=head (matching original memory layout)
	const ZmbStateFile &f = _vm->_state->_f;

	outTraits.clear();
	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount; i++) {
		const ZmbStateActiveEntry &entry = f._zmbPackActive._entries[i];
		if (!entry._bIsOccupied)
			continue;

		// Pack traits into a DWORD: foot in byte0, nose in byte1, eye in byte2, head in byte3
		uint32 packed = entry._traits._foot
					  | (static_cast<uint32>(entry._traits._nose) << 8)
					  | (static_cast<uint32>(entry._traits._eye) << 16)
					  | (static_cast<uint32>(entry._traits._head) << 24);
		outTraits.push_back(packed);
	}

	return outTraits.size();
}

void ZoombiniInteractiveBridge::buildAttrTollTable() {
	// IDA: bridge_buildAttrTollTable_4160EF
	// Builds a pool of all valid toll combinations for the current difficulty level,
	// counts how many Zoombinis match each combo, and selects the one closest to half.

	// Allocate combo pool and match count arrays
	Common::Array<uint32> comboPool;
	Common::Array<uint16> matchCounts;

	// Collect packed Zoombini trait DWORDs
	Common::Array<uint32> zmbTraits;
	collectZmbAttrPacked(zmbTraits);

	uint32 poolSize = 0;

	switch (_routeLevel) {
	case 0: {
		// Level 0: 20 single-attribute combos (5 values × 4 types)
		// foot:1-5, nose:256-1280, eye:0x10000-0x50000, head:0x1000000-0x5000000
		poolSize = 20;
		comboPool.resize(poolSize); 
		uint32 val = 1;
		uint32 step = 1;
		for (uint32 k = 0; k < 20; k++) {
			comboPool[k] = val;
			switch (val) {
			case 5:
				step = 256;
				val = 256;
				break;
			case 1280:
				step = 0x10000;
				val = 0x10000;
				break;
			case 327680:
				step = 0x1000000;
				val = 0x1000000;
				break;
			default:
				if (val != 83886080)
					val += step;
				break;
			}
		}
		break;
	}

	case 1: {
		// Level 1: 40 dual-nibble combos (4 types × 10 combos each)
		// Uses kLevel1ComboTable shifted by 0/8/16/24 bits per type group
		poolSize = 40;
		comboPool.resize(poolSize);
		uint32 idx = 0;
		int shift = 0;
		for (uint32 j = 0; j < 4; j++) {
			for (uint32 k = 0; k < 10; k++) {
				comboPool[idx++] = kLevel1ComboTable[k] << shift;
			}
			shift += 8;
		}
		break;
	}

	case 2: {
		// Level 2: 150 arithmetic combos (6 groups × 5 × 5)
		// Uses base+step tables
		poolSize = 150;
		comboPool.resize(poolSize);
		uint32 idx = 0;
		for (uint32 j = 0; j < 6; j++) {
			for (uint32 n = 1; n <= 5; n++) {
				uint32 acc = kLevel2BaseTable[j] + n * kLevel2StepTable[j];
				for (uint32 ii = 1; ii <= 5; ii++) {
					comboPool[idx++] = acc;
					acc += kLevel2BaseTable[j];
				}
			}
		}
		break;
	}

	case 3: {
		// Level 3: 500 four-attribute permutation combos (4 groups × 125 each)
		poolSize = 500;
		comboPool.resize(poolSize);
		uint32 groupOffset = 0;

		for (uint32 j = 0; j < 4; j++) {
			uint32 v12, incHigh, rawMask, midMask, swappedMask, highMask;
			int shiftLow, shiftMid;

			switch (j) {
			case 0:
				v12 = 65793;       // 0x10101
				incHigh = 1;
				rawMask = 986880;  // 0xF0F00
				midMask = 256;     // 0x100
				swappedMask = 983055; // 0xF000F
				highMask = 0x10000;
				shiftLow = 0;
				shiftMid = 8;
				break;
			case 1:
				v12 = 16777473;    // 0x1000101
				incHigh = 1;
				rawMask = 251662080; // 0xF00F000
				midMask = 256;
				swappedMask = 251658255; // 0xF00000F
				highMask = 0x1000000;
				shiftLow = 0;
				shiftMid = 8;
				break;
			case 2:
				v12 = 0x1010001;
				incHigh = 1;
				rawMask = 0xF0F0000;
				midMask = 0x10000;
				swappedMask = 0xF00000F;
				highMask = 0x1000000;
				shiftLow = 0;
				shiftMid = 16;
				break;
			default: // case 3
				v12 = 0x1010100;
				incHigh = 0x100;
				rawMask = 0xF0F0000;
				midMask = 0x10000;
				swappedMask = 0xF000F00;
				highMask = 0x1000000;
				shiftLow = 8;
				shiftMid = 16;
				break;
			}

			for (uint32 jj = 0; jj < 125; jj++) {
				comboPool[jj + groupOffset] = v12;
				v12 += incHigh;
				if (((v12 >> shiftLow) & 0xF) == 6) {
					v12 = midMask + incHigh + (rawMask & v12);
					if (((v12 >> shiftMid) & 0xF) == 6)
						v12 = highMask + midMask + (swappedMask & v12);
				}
			}
			groupOffset += 125;
		}
		break;
	}

	default:
		poolSize = 20;
		comboPool.resize(poolSize);
		break;
	}

	// Count how many Zoombinis match each combo
	matchCounts.resize(poolSize);
	for (uint32 i = 0; i < poolSize; i++)
		matchCounts[i] = 0;

	if (_routeLevel == 1) {
		// Level 1: match if any nibble of the Zoombini matches the corresponding combo nibble
		// Uses shifted nibble comparison (also checks the second nibble in each byte)
		for (uint32 j = 0; j < zmbTraits.size(); j++) {
			// Swap bytes to match original memory layout (the original does byte endian swap)
			uint32 zmb = zmbTraits[j];
			uint32 swapped = ((zmb & 0xFF) << 24) | (((zmb >> 8) & 0xFF) << 16)
						   | (((zmb >> 16) & 0xFF) << 8) | ((zmb >> 24) & 0xFF);

			for (uint32 m = 0; m < poolSize; m++) {
				uint32 combo = comboPool[m];
				if ((swapped & 0xF) == (combo & 0xF) ||
					(swapped & 0xF00) == (combo & 0xF00) ||
					(swapped & 0xF0000) == (combo & 0xF0000) ||
					(swapped & 0xF000000) == (combo & 0xF000000) ||
					(swapped & 0xF) == ((combo & 0xF0) >> 4) ||
					(swapped & 0xF00) == ((combo & 0xF000) >> 4) ||
					(swapped & 0xF0000) == ((combo & 0xF00000) >> 4) ||
					(swapped & 0xF000000) == ((combo & 0xF0000000u) >> 4)) {
					matchCounts[m]++;
				}
			}
		}
	} else {
		// Levels 0, 2, 3: match if any byte-level nibble matches
		for (uint32 j = 0; j < zmbTraits.size(); j++) {
			uint32 zmb = zmbTraits[j];
			uint32 swapped = ((zmb & 0xFF) << 24) | (((zmb >> 8) & 0xFF) << 16)
						   | (((zmb >> 16) & 0xFF) << 8) | ((zmb >> 24) & 0xFF);

			for (uint32 m = 0; m < poolSize; m++) {
				uint32 combo = comboPool[m];
				if ((swapped & 0xF) == (combo & 0xF) ||
					(swapped & 0xF00) == (combo & 0xF00) ||
					(swapped & 0xF0000) == (combo & 0xF0000) ||
					(swapped & 0xF000000) == (combo & 0xF000000)) {
					matchCounts[m]++;
				}
			}
		}
	}

	// Early out: the original code hangs if no zoombinis are present (latent bug).
	// The spiral search below only checks matchCounts in range 1..15, and with 0
	// zoombinis all counts are 0, causing an infinite loop.
	if (zmbTraits.empty()) {
		_puzzleReady = false;
		return;
	}

	// Spiral search from total/2 to find a match count with at least 1 combo.
	// The original step formula (step = -(step+1)) alternates 1, -2, 1, -2...
	// producing a slow downward drift: total/2, +1, -1, 0, -2, -1, -3, ...
	// This is NOT a proper expanding spiral and can fail if the only matchCounts
	// are 0 or >= 16 (e.g. all zoombinis identical). A safety limit prevents hangs.
	int32 total = zmbTraits.size();
	int32 target = total / 2;
	int32 step = 1;
	int found = 0;
	uint32 chosenCount = target;

	int safetyLimit = 64;
	while (!found && safetyLimit-- > 0) {
		if (target > 0 && target < 16) {
			for (uint32 i = 0; i < poolSize; i++) {
				if (matchCounts[i] == static_cast<uint32>(target))
					found++;
			}
			chosenCount = target;
		}
		target += step;
		step = -(step + 1);
	}
	if (!found) {
		// Fallback: pick any combo with a non-zero match count
		for (uint32 i = 0; i < poolSize && !found; i++) {
			if (matchCounts[i] > 0) {
				chosenCount = matchCounts[i];
				found = 1;
			}
		}
	}
	if (!found) {
		// Still nothing — no valid toll can be formed
		_puzzleReady = false;
		return;
	}

	// Random pick among combos with the chosen match count
	int pick = _vm->_rnd->getRandomNumber(1, found);
	uint32 targetCombo = 0;
	for (uint32 i = 0; i < poolSize; i++) {
		if (matchCounts[i] == chosenCount) {
			pick--;
			if (pick == 0) {
				targetCombo = comboPool[i];
				break;
			}
		}
	}

	// Decode the selected combo into reqAttrTypes/reqAttrValues
	_puzzleReady = true;
	_trollSlot = _vm->_rnd->getRandomNumber(1, 2);

	if (_routeLevel == 0) {
		// Level 0: single attribute
		_reqAttrCount = 1;
		if (targetCombo & 0xFF) {
			_reqAttrTypes[0] = 4; // legs
			_reqAttrValues[0] = targetCombo & 0xF;
		} else if ((targetCombo >> 8) & 0xFF) {
			_reqAttrTypes[0] = 3; // nose
			_reqAttrValues[0] = (targetCombo >> 8) & 0xF;
		} else if ((targetCombo >> 16) & 0xFF) {
			_reqAttrTypes[0] = 2; // eyes
			_reqAttrValues[0] = (targetCombo >> 16) & 0xF;
		} else if ((targetCombo >> 24) & 0xFF) {
			_reqAttrTypes[0] = 1; // hair
			_reqAttrValues[0] = (targetCombo >> 24) & 0xF;
		}
	} else if (_routeLevel == 1) {
		// Level 1: two attributes
		_reqAttrCount = 2;
		if (targetCombo & 0xFF) {
			_reqAttrTypes[0] = 4;
			_reqAttrValues[0] = targetCombo & 0xF;
			_reqSecondAttrType = 4;
			_reqSecondAttrValue = (targetCombo & 0xF0) >> 4;
		} else if ((targetCombo >> 8) & 0xFF) {
			_reqAttrTypes[0] = 3;
			_reqAttrValues[0] = (targetCombo >> 8) & 0xF;
			_reqSecondAttrType = 3;
			_reqSecondAttrValue = (targetCombo >> 12) & 0xF;
		} else if ((targetCombo >> 16) & 0xFF) {
			_reqAttrTypes[0] = 2;
			_reqAttrValues[0] = (targetCombo >> 16) & 0xF;
			_reqSecondAttrType = 2;
			_reqSecondAttrValue = (targetCombo >> 20) & 0xF;
		} else if ((targetCombo >> 24) & 0xFF) {
			_reqAttrTypes[0] = 1;
			_reqAttrValues[0] = (targetCombo >> 24) & 0xF;
			_reqSecondAttrType = 1;
			_reqSecondAttrValue = (targetCombo >> 28) & 0xF;
		}
	} else {
		// Levels 2 and 3: extract all non-zero nibbles
		_reqAttrCount = 0;
		uint32 idx = 0;
		if (targetCombo & 0xFF) {
			_reqAttrTypes[idx] = 4;
			_reqAttrValues[idx] = targetCombo & 0xF;
			idx++;
			_reqAttrCount++;
		}
		if ((targetCombo >> 8) & 0xFF) {
			if (idx < static_cast<uint32>(_routeLevel)) {
				_reqAttrTypes[idx] = 3;
				_reqAttrValues[idx] = (targetCombo >> 8) & 0xF;
				idx++;
			}
			_reqAttrCount++;
		}
		if ((targetCombo >> 16) & 0xFF) {
			if (idx < static_cast<uint32>(_routeLevel)) {
				_reqAttrTypes[idx] = 2;
				_reqAttrValues[idx] = (targetCombo >> 16) & 0xF;
				idx++;
			}
			_reqAttrCount++;
		}
		if ((targetCombo >> 24) & 0xFF) {
			if (idx < static_cast<uint32>(_routeLevel)) {
				_reqAttrTypes[idx] = 1;
				_reqAttrValues[idx] = (targetCombo >> 24) & 0xF;
			}
			_reqAttrCount++;
		}
	}

	debugC(kZmbDebugPage, "Bridge: route level %d, reqAttrCount=%d", _routeLevel, _reqAttrCount);
	for (int i = 0; i < _reqAttrCount; i++) {
		debugC(kZmbDebugPage, "  reqAttr[%d]: type=%d, value=%d", i, _reqAttrTypes[i], _reqAttrValues[i]);
	}
}

bool ZoombiniInteractiveBridge::testAttrMatch(const ZmbTrait &trait, int16 targetSlot) const {
	// IDA: bridge_testAttrMatchRule_4168E9
	// targetSlot=1: returns true if ANY required attribute matches (match lane)
	// targetSlot=2: returns true if NONE matches (reject lane, inverted)

	if (targetSlot < 1 || targetSlot > 2)
		targetSlot = 1;

	bool anyMatch = false;
	for (uint8 i = 0; i < _reqAttrCount; i++) {
		uint8 traitValue = 0;
		switch (_reqAttrTypes[i]) {
		case 1: traitValue = trait._head; break;  // hair
		case 2: traitValue = trait._eye; break;   // eyes
		case 3: traitValue = trait._nose; break;  // nose
		case 4: traitValue = trait._foot; break;  // legs
		default: break;
		}

		if (traitValue == _reqAttrValues[i])
			anyMatch = true;
	}

	// targetSlot=2 inverts: match lane wants ANY match, reject lane wants NONE
	return (targetSlot == 2) ? !anyMatch : anyMatch;
}

void ZoombiniInteractiveBridge::bridgeButtons_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	// IDA: bridge_buttonDraw_415122 (but adapted for ScummVM's pre-render hook pattern)
	// Enables/disables the Go button based on whether any Zoombini has crossed
	setGoButtonsEnabled(_anyZmbCrossed != 0);
	goMapButtons_preRenderShape(feature, hsGroup, hotspots);
}

ZmbEventHandleResult ZoombiniInteractiveBridge::bridgeButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return goMapButtons_onLButtonDown(feature, absPos, relPos);
}

ZmbRenderResult ZoombiniInteractiveBridge::bridgeVisuals_render(ZmbFeature *feature) {
	// IDA: bridge_invalidateVisualRects_415204
	// Toggle the Go button visibility based on whether any Zoombini has crossed.
	setGoButtonsEnabled(_anyZmbCrossed != 0);
	return ZmbRenderResult::kRendered;
}

void ZoombiniInteractiveBridge::bridgeVisuals_postRender(ZmbFeature *feature) {
	// IDA: bridge_drawAllButtons_4151DC
	// In ScummVM, button rendering is handled by the interactive_base framework.
	// Nothing additional needed here.
}

// ---------------------------------------------------------------------------
// Helper: Reload SCRB animation data on an existing feature.
// Delegates to ZoombiniPage::loadScrbOntoFeature (IDA: scrb_loadOnRunner 0x460384).
// ---------------------------------------------------------------------------
void ZoombiniInteractiveBridge::reloadScrbAnimation(uint16 featureId, uint16 newScrbId) {
	auto it = _scrbFeatureMap.find(featureId);
	if (it == _scrbFeatureMap.end())
		return;
	loadScrbOntoFeature(it->second, newScrbId);
}

// ---------------------------------------------------------------------------
// Helper: Find an idle pack snoid (IDs 10000+).
// IDA: findIdleFeatureRunner_456A95
// ---------------------------------------------------------------------------
ZmbSnoid *ZoombiniInteractiveBridge::findIdlePackSnoid(uint16 preferredId) {
	// If a specific snoid is requested, try it first
	if (preferredId > 0) {
		ZmbSnoid *snoid = getSnoid(preferredId);
		if (snoid && snoid->getAnimState() == kSnoidAnimIdle)
			return snoid;
	}
	// Search all snoids for an idle pack snoid
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if (it->first < 10000)
			continue; // Skip template snoids
		ZmbSnoid *snoid = it->second;
		if (snoid->getAnimState() == kSnoidAnimIdle)
			return snoid;
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// Helper: Determine drop target lane from position.
// IDA: getDropTargetResult_453571 (uses slot positions from bridge segments)
// ---------------------------------------------------------------------------
int16 ZoombiniInteractiveBridge::getDropTargetLane(const Common::Point &pos) const {
	// Check proximity to each bridge segment entrance.
	// Segment 0 (lane 1/top) at kSegmentPositions[0] = (116, 104)
	// Segment 1 (lane 2/bottom) at kSegmentPositions[1] = (128, 203)
	for (int16 i = 0; i < 2; i++) {
		int16 dx = pos.x - kSegmentPositions[i].x;
		int16 dy = pos.y - kSegmentPositions[i].y;
		if (dx * dx + dy * dy < kDropZoneRadius * kDropZoneRadius)
			return i + 1; // 1 = lane 1, 2 = lane 2
	}
	return 0; // No valid drop zone
}

// ---------------------------------------------------------------------------
// Helper: Find a snoid whose drawn area contains the given point.
// ---------------------------------------------------------------------------
ZmbSnoid *ZoombiniInteractiveBridge::findSnoidAtPoint(const Common::Point &pos) {
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if (it->first < 10000)
			continue; // Skip template snoids
		ZmbSnoid *snoid = it->second;
		if (snoid->findDrawRecordAtPoint(pos))
			return snoid;
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// onEveryFrame: Main per-frame logic.
// IDA: puzzleBridge_onHover_4152C3
// ---------------------------------------------------------------------------
void ZoombiniInteractiveBridge::onEveryFrame() {
	if (_processingFrame || !_isActive)
		return;
	_processingFrame = true;

	// -----------------------------------------------------------------------
	// [0] Pending Go departure: skip normal frame logic while waiting.
	// Base class onAnimFrame() handles the actual departure transition.
	// IDA: wMouseClickedPuzzleIdx_4B0424 branch in puzzleBridge_onHover_4152C3
	// -----------------------------------------------------------------------
	if (_pendingGoDepart) {
		_processingFrame = false;
		return;
	}

	// -----------------------------------------------------------------------
	// [1] Troll entrance animation trigger.
	// IDA: word_4AAE6E check → start entrance anim on trollLane1
	// -----------------------------------------------------------------------
	if (_trollAnimPending) {
		_trollAnimPending = 0;

		// Hide the lane-2 troll feature (it's offscreen during entrance)
		auto itLane2 = _scrbFeatureMap.find(_scrbTrollLane2Idx);
		if (itLane2 != _scrbFeatureMap.end()) {
			itLane2->second->deactivateRender();
			itLane2->second->deactivateAnimate();
		}

		// Load SCRB 1235 on the gate troll
		reloadScrbAnimation(_scrbTrollGateIdx, 1235);

		// Load SCRB 1221 on lane-1 troll (entrance animation with callbacks)
		reloadScrbAnimation(_scrbTrollLane1Idx, 1221);
	}

	// -----------------------------------------------------------------------
	// [2] Process trail queue: start the next crossing animation.
	// IDA: word_4AAE88 && !word_4AAE72 branch
	// -----------------------------------------------------------------------
	if (_trailLength > 0 && !_isRejectPlaying) {
		// Determine the snoid to cross
		uint16 trailRunnerId = _trailRunnerIdx[0];

		// Guard: skip if too many crossed, or reject in transit, or not enough time
		bool skip = false;
		if (_successCount >= 6)
			skip = true;
		if (_trailMatchResult[0] && _bridgeTransitCount > 4)
			skip = true;
		if (!skip && (getCurrentFrameCounter() - _lastFrameSnapshot) < 0x2D)
			skip = true;
		if (skip)
			trailRunnerId = 0;

		ZmbSnoid *snoid = findIdlePackSnoid(trailRunnerId);
		if (snoid) {
			// Snapshot frame counter
			_lastFrameSnapshot = getCurrentFrameCounter();

			// Shift trail queue forward
			if (_trailLength >= 1 && _trailLength <= 2) {
				_currentDropLane = _trailDropZone[0];
				_currentMatchResult = _trailMatchResult[0];
				_trailDropZone[0] = _trailDropZone[1];
				_trailRunnerIdx[0] = _trailRunnerIdx[1];
				_trailMatchResult[0] = _trailMatchResult[1];
				_trailLength--;
			}

			// Determine SCRS resource based on lane and match result.
			// IDA: lane1 match→2010, lane1 reject→2015, lane2 match→2000, lane2 reject→2005
			uint16 scrsBase;
			if (_currentDropLane == 1) {
				scrsBase = _currentMatchResult ? 2010 : 2015;
			} else {
				scrsBase = _currentMatchResult ? 2000 : 2005;
			}

			_bridgeTransitCount++;
			_isRejectPlaying = _currentMatchResult;
			if (!_currentMatchResult) {
				// Set random speed for reject path
				snoid->setAnimSpeed(_vm->_rnd->getRandomNumber(4, 5), 0);
			}

			_activeRejectScrb = -1;
			_activeLaneScrb = -1;

			// Start SCRS playback on the pack snoid.
			// IDA: snoidScript_initAndPlay_455C0D(0, 0, shapeImageIdx + scrsBase - 1, core)
			// With shapeImageIdx=1, the SCRS resource ID is scrsBase.
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, scrsBase));
			if (scrsStream) {
				snoid->startScrsPlayback(scrsStream, false /* hideOnComplete */,
										 _currentMatchResult == 0 /* rejectState */);
			}
		}
	}

	// -----------------------------------------------------------------------
	// [3] Process pending lane event (troll gate animation after crossing step).
	// IDA: word_4AAE7A branch
	// -----------------------------------------------------------------------
	if (_pendingLaneEvent) {
		auto itLane = _scrbFeatureMap.find(_pendingLaneEvent);
		if (itLane != _scrbFeatureMap.end() && _currentMatchResult) {
			// Load the appropriate troll animation SCRB on the lane feature
			uint16 trollLaneScrb;
			if (_pendingLaneEvent == _scrbTrollLane2Idx)
				trollLaneScrb = 1222;
			else
				trollLaneScrb = 1214;
			_pendingLaneEvent = 0;
			reloadScrbAnimation(itLane->first, trollLaneScrb);

			// Trigger troll gate match animation.
			// IDA: troll gate SCRB = successCount + 1223 (lane1) or 1208 (lane2)
			if (_currentMatchResult) {
				uint16 gateScrbId;
				if (_currentDropLane == 1)
					gateScrbId = _successCount + 1223;
				else
					gateScrbId = _successCount + 1208;
				reloadScrbAnimation(_scrbTrollMainIdx, gateScrbId);
			}

			// Update bridge segment animation.
			// IDA: troll gate visual SCRB on _scrbTrollGateIdx
			uint16 segScrbId;
			if (!_currentMatchResult) {
				segScrbId = (_currentDropLane == 1) ? _successCount + 1243 : _successCount + 1237;
			} else {
				segScrbId = (_currentDropLane == 1) ? _successCount + 1229 : _successCount + 1215;
			}
			reloadScrbAnimation(_scrbTrollGateIdx, segScrbId);
		}
		_pendingLaneEvent = 0;
	}

	// -----------------------------------------------------------------------
	// [4] Fidget scheduling.
	// IDA: word_4AAEB4 < word_4AAEB2 and timer check
	// -----------------------------------------------------------------------
	if (_fidgetPlayedCount < _fidgetScheduleCount &&
		getCurrentFrameCounter() - _fidgetTimer > _fidgetInterval) {

		_fidgetTimer = getCurrentFrameCounter();
		bool triggered = false;
		int16 attempts = 0;

		do {
			attempts++;
			// Pick a random pack snoid.
			uint16 poolIdx = _vm->_rnd->getRandomNumber(0, _totalZmbCount > 0 ? _totalZmbCount - 1 : 0);
			uint16 snoidId = 10000 + poolIdx;

			ZmbSnoid *snoid = getSnoid(snoidId);
			if (snoid && snoid->getAnimState() == kSnoidAnimIdle &&
				snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
				// Play fidget SCRS: 2019 + shapeImageIdx - 1 ≈ 2019
				Common::SeekableReadStream *scrsStream =
					_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, 2019));
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

// ---------------------------------------------------------------------------
// onFeatureAnimEvent: Dispatches animation event codes to the appropriate handler.
// ---------------------------------------------------------------------------
void ZoombiniInteractiveBridge::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
		// Crossing snoid → lane step callback
		processLaneStepEvent(feature, eventCode);
	} else {
		// SCRB feature → entrance callback
		processEntranceEvent(eventCode, feature);
	}
}

// ---------------------------------------------------------------------------
// processLaneStepEvent: Lane step callback from crossing snoid SCRS playback.
// IDA: bridge_zmbLaneStepCallback_415D30
// ---------------------------------------------------------------------------
void ZoombiniInteractiveBridge::processLaneStepEvent(ZmbFeature *snoidFeature, int16 stepCode) {
	ZmbSnoid *snoid = static_cast<ZmbSnoid *>(snoidFeature);

	switch (stepCode) {
	case 1:
	case 4:
		// Set pending lane event to troll lane 1
		_pendingLaneEvent = _scrbTrollLane1Idx;
		break;

	case 2:
	case 5:
		// Set pending lane event to troll lane 2
		_pendingLaneEvent = _scrbTrollLane2Idx;
		break;

	case 3:
	case 6: {
		// Zoombini arrives at destination lane.
		_bridgeTransitCount--;

		// Set depart animation
		Common::Point destPos;
		if (stepCode == 6) {
			// Arrived at lane 1 (top)
			if (_lane1Count < 16) {
				destPos = kLane1Positions[_lane1Count];
				_lane1ZmbIds[_lane1Count] = snoid->getId();
				_lane1Count++;
			}
		} else {
			// Arrived at lane 2 (bottom)
			if (_lane2Count < 16) {
				destPos = kLane2Positions[_lane2Count];
				_lane2ZmbIds[_lane2Count] = snoid->getId();
				_lane2Count++;
			}
		}

		snoid->finishScrsPlayback();
		snoid->setAnimState(kSnoidAnimDepart, &destPos);

		// Track crossed count for Go button and fidget scheduling
		int16 totalCrossed = _lane1Count + _lane2Count;
		if (!_anyZmbCrossed)
			_anyZmbCrossed = totalCrossed;

		// Fidget schedule thresholds: 10, 12, 14, all
		if (totalCrossed == 10)
			_fidgetScheduleCount++;
		else if (totalCrossed == 12)
			_fidgetScheduleCount++;
		else if (totalCrossed == 14)
			_fidgetScheduleCount += 2;
		if (totalCrossed == _totalZmbCount)
			_fidgetScheduleCount += 2;

		// Play voice-over when all are crossed
		if (totalCrossed == _totalZmbCount && _bridgeTransitCount == 0) {
			uint16 sndId = _vm->_rnd->getRandomNumber(20055, 20063);
			_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, sndId),
									  Audio::Mixer::kSFXSoundType);
		}
		break;
	}

	case 10: {
		// Play snoid attribute display script.
		// IDA: SCRS base depends on _trollAttrState (which attr type the troll shows)
		if (_trollAttrState <= 0)
			break;

		uint16 scrsBase;
		switch (_trollAttrState) {
		case 2: scrsBase = 1012; break; // eyes
		case 3: scrsBase = 1008; break; // nose
		case 4: scrsBase = 1000; break; // feet
		case 5: scrsBase = 1004; break; // hair? (head)
		default: scrsBase = 1016; break; // default
		}

		// Offset by lane number
		if (_currentDropLane == 1)
			scrsBase += 2;

		// Random variant (0 or 1)
		scrsBase += _vm->_rnd->getRandomNumber(0, 1);

		Common::SeekableReadStream *scrsStream =
			_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, scrsBase));
		if (scrsStream) {
			snoid->startScrsPlayback(scrsStream, false, true);
			_activeRejectScrb = scrsBase;
			_activeLaneScrb = _trollAttrState;
		}
		_trollAttrState = 0;
		break;
	}

	case 20:
		// Transit complete: Zoombini has crossed the bridge.
		_bridgeTransitCount--;
		if (_successCount < 6)
			_successCount++;
		_newArrivalReady = 1;
		break;

	case -1: {
		// End of SCRS playback: reposition rejected Zoombini.
		_newArrivalReady = 0;
		if (_isRejectPlaying)
			_isRejectPlaying = 0;

		// Find a non-colliding position to place the rejected Zoombini.
		// Use the lane positions as reference for repositioning.
		const Common::Point *posTable = (_currentDropLane == 1) ? kSnoidPositions : kSnoidPositions + 4;
		Common::Point targetPos = *posTable;

		// Try snoid positions that don't collide
		for (int i = 0; i < 16; i++) {
			if (!isPointOccupiedByOtherSnoid(snoid, kSnoidPositions[i], 500)) {
				targetPos = kSnoidPositions[i];
				break;
			}
		}

		snoid->finishScrsPlayback();
		snoid->setAnimState(kSnoidAnimIdle, &targetPos);
		snoid->setupIdleHotspots();
		break;
	}

	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// processEntranceEvent: Troll entrance event callback.
// IDA: bridge_onEntranceCallback_415C34
// ---------------------------------------------------------------------------
void ZoombiniInteractiveBridge::processEntranceEvent(int16 eventId, ZmbFeature *eventSource) {
	if (eventId >= 1 && eventId <= 6) {
		// Record the troll attribute display state (which attribute the troll shows)
		_trollAttrState = eventId;
	} else if (eventId == 100 || eventId == 101) {
		// Change water overlay animation.
		// 100: load SCRB 1236 (water splash), 101: load SCRB 1103 (normal water)
		uint16 waterScrbId = (eventId == 100) ? 1236 : 1103;
		reloadScrbAnimation(_scrbWaterIdx, waterScrbId);
	} else if (eventId == -1) {
		// End of entrance animation. Maybe play a voice-over.
		int16 totalCrossed = _lane1Count + _lane2Count;
		if (totalCrossed < _totalZmbCount) {
			if (totalCrossed > 0 || _vm->_state->getDifficultyIdFromPageFlag(
					_vm->_state->_f._pageFlagBridge) <= ZMB_DIFFICULTY_LEVEL2_02) {
				uint16 sndId = _vm->_rnd->getRandomNumber(20045, 20048);
				_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, sndId),
										  Audio::Mixer::kSFXSoundType);
			}
		}
	} else if (eventId == 0) {
		// Activate troll entrance animation trigger for next frame.
		_trollAnimPending = 1;
	}
}

// ---------------------------------------------------------------------------
// Drag-and-drop: Zoombini interaction.
// IDA: bridge_funcOnClick_4157EB case 4
// ---------------------------------------------------------------------------
ZmbEventHandleResult ZoombiniInteractiveBridge::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// In sticky mouse mode, a second click ends the drag
	if (isDragging() && _vm->_state->getEnableStickyMouse()) {
		endDrag(absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	// Let the base class handle button clicks first.
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Zoombini drag start
	if (_successCount >= 6 || isDragging())
		return ZmbEventHandleResult::kPassthrough;

	ZmbSnoid *snoid = findSnoidAtPoint(absPos);
	if (!snoid)
		return ZmbEventHandleResult::kPassthrough;

	// Don't drag snoids that are playing scripts
	SnoidAnimState state = snoid->getAnimState();
	if (state == kSnoidAnimScriptReject)
		return ZmbEventHandleResult::kPassthrough;
	if (state == kSnoidAnimScriptNormal) {
		if (!_newArrivalReady)
			return ZmbEventHandleResult::kPassthrough;
		if (_isRejectPlaying)
			_isRejectPlaying = 0;
		_newArrivalReady = 0;
	}

	// Begin drag — IDA: beginDragFeatureRunner_45360F
	startSnoidDrag(snoid, absPos);
	_isDragging = 1;

	// If this snoid is already in the trail, remove it
	if (_trailLength == 1 && _trailRunnerIdx[0] == snoid->getId()) {
		_trailLength = 0;
	} else if (_trailLength == 2) {
		if (_trailRunnerIdx[1] == snoid->getId()) {
			_trailLength = 1;
		} else if (_trailRunnerIdx[0] == snoid->getId()) {
			_trailDropZone[0] = _trailDropZone[1];
			_trailRunnerIdx[0] = _trailRunnerIdx[1];
			_trailMatchResult[0] = _trailMatchResult[1];
			_trailLength = 1;
		}
	}

	return ZmbEventHandleResult::kConsumed;
}

void ZoombiniInteractiveBridge::endDrag(const Common::Point &dropPos) {
	ZmbSnoid *snoid = finishSnoidDrag();
	_isDragging = 0;

	// Check drop target
	Common::Point snoidPos = snoid->getPointLoc();
	int16 dropLane = getDropTargetLane(snoidPos);

	if (dropLane > 0 && static_cast<uint16>(_trailLength) < 2) {
		// Valid drop: add to trail
		bool isMatch = testAttrMatch(snoid->_trait, dropLane);
		_trailDropZone[_trailLength] = dropLane;
		_trailRunnerIdx[_trailLength] = snoid->getId();
		_trailMatchResult[_trailLength] = isMatch ? 1 : 0;
		_trailLength++;

		// Send snoid to arrival animation
		snoid->setAnimState(kSnoidAnimArrive);
	} else {
		// No valid drop: validate against terrain barrier bitmap.
		// IDA: terrain_validateAndPlaceSnoid (0x453D28) — checks walkability
		// at drop position, adjusts for collision, or returns to original.
		if (!validateTerrainDrop(snoid)) {
			// Terrain invalid — return snoid to original position
			snoid->setPointLoc(_dragOrigPos);
		}
		snoid->setAnimState(kSnoidAnimIdle);
		snoid->setupIdleHotspots();
	}
}

ZmbEventHandleResult ZoombiniInteractiveBridge::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	if (!isDragging()) {
		return ZoombiniInteractive::onLButtonUp(absPos, relPos);
	}

	// In sticky mouse mode, button-up does NOT end drag (click again to drop)
	if (_vm->_state->getEnableStickyMouse())
		return ZmbEventHandleResult::kConsumed;

	endDrag(absPos);

	return ZmbEventHandleResult::kConsumed;
}

} // End of namespace Mohawk
