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
#include "mohawk/zoombini_pages/puzzle_ferry.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions at 0x4A0E5C (20 POINTS)
const Common::Point ZoombiniInteractiveFerry::kSnoidPositions[20] = {
	Common::Point(370, 160), Common::Point(395, 196), Common::Point(332, 156), Common::Point(348, 196),
	Common::Point(294, 168), Common::Point(316, 196), Common::Point(253, 166), Common::Point(276, 196),
	Common::Point(214, 157), Common::Point(237, 196), Common::Point(175, 160), Common::Point(196, 190),
	Common::Point(135, 152), Common::Point(150, 191), Common::Point( 94, 145), Common::Point(110, 186),
	Common::Point( 57, 146), Common::Point( 71, 182), Common::Point( 25, 145), Common::Point( 27, 183),
};

// IDA: qword_4A0EBC — dock area rect
const Common::Rect ZoombiniInteractiveFerry::kDockRect(0, 130, 469, 240);

// IDA: word_4A0CFC — boat approach SCRB pool (4 entries)
const uint16 ZoombiniInteractiveFerry::kBoatScrbPool[4] = { 1800, 1801, 1802, 1803 };

// IDA: word_4A0D08 — captain idle fidget SCRB pool (5 entries)
const uint16 ZoombiniInteractiveFerry::kFidgetScrbPool[5] = { 1823, 1824, 1825, 1826, 1827 };

// IDA: word_4A0D18 — correct placement reaction SCRB pool (2 entries)
const uint16 ZoombiniInteractiveFerry::kGoodReactionPool[2] = { 1817, 1818 };

// IDA: word_4A0D20 — wrong placement reaction SCRB pool (11 entries)
const uint16 ZoombiniInteractiveFerry::kBadReactionPool[11] = {
	1804, 1805, 1806, 1807, 1808, 1809, 1810, 1811, 1812, 1813, 1814
};

// IDA: word_4A0D4C — moved-from-dock reaction SCRB pool (3 entries)
const uint16 ZoombiniInteractiveFerry::kMoveReactionPool[3] = { 1820, 1821, 1822 };

ZoombiniInteractiveFerry::ZoombiniInteractiveFerry(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kFerry) {
}

ZoombiniInteractiveFerry::~ZoombiniInteractiveFerry() {
}

void ZoombiniInteractiveFerry::open() {
	openArchive(ZMB_MHK_FERRY);
}

void ZoombiniInteractiveFerry::setBackgroundMusic() {
	// IDA: ferry_funcInit (0x41a394) has NO music playback call on page load.
	// sound_activeHandle (20073/20074) is stored at the END of funcInit for F1 replay only.
	// scrb_enqueueSoundResource(0, SND_00997_MOVE_SHORT_SFX) plays a UI click via SCRB when
	// walk animations start — it is NOT a narrator voice and is handled by the SCRB system.
	// Therefore no sound plays here; the narrator voice must not auto-play on page load.
}

void ZoombiniInteractiveFerry::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(1300)
	_vm->_gfx->setPalette(1300);
	_vm->_gfx->drawBackground(1300);
}

void ZoombiniInteractiveFerry::loadFeatures() {
	// IDA: ferry_funcInit (0x41a394)
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();
	_visitCounter++;

	// IDA: ferry_selectSCRB (0x41bc4e) — calculate SCRB ID based on difficulty and zoombini count
	{
		int16 zmbCount = _vm->_state->_f._zmbPackActive._wPackZmbCount;
		if (zmbCount < 16)
			zmbCount = 16;
		else if (zmbCount > 20)
			zmbCount = 20;

		uint16 scrbBase = 1510 + (_difficultyLevel * 5);
		_seatingSCRB = scrbBase + (zmbCount - 16);
		debugC(kZmbDebugPage, "Ferry: difficultyLevel=%d, zmbCount=%d, seatingSCRB=%d",
		       _difficultyLevel, zmbCount, _seatingSCRB);
	}

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(100u)
	loadTerrainBitmap(100);

	// Preload shape images
	// IDA: shape_loadSubShapesFromArchive(&stru_4A0E58, 1400u)
	_vm->_gfx->preloadImage(1400);
	_vm->_gfx->preloadImage(1450);
	_vm->_gfx->preloadImage(1500);
	_vm->_gfx->preloadImage(1600);
	_vm->_gfx->preloadImage(1700);
	_vm->_gfx->preloadImage(1800);

	// Load main features: 10 SCRBs at 1500
	// IDA: scrb_preloadMainFeatureSet(10, 1500)
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_preloadSubFeatureSet(0, 10, 0x640) — 10 subs at 1600
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 10; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 1600), 1600 + i);
		}
	}

	// IDA: scrb_preloadSubFeatureSet(0, 7, 0x6A4) — 7 subs at 1700
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 7; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 1700), 1700 + i);
		}
	}

	// IDA: scrb_preloadSubFeatureSet(5, 33, 0x708) — 33 subs at 1800
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 33; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 1800), 1800 + i);
		}
	}

	// IDA: scrb_preloadSubFeatureSet(0, 3, 0x5AA) — 3 subs at 1450
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 3; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 1450), 1450 + i);
		}
	}

	// Load reject pool: 8 reject scripts at SCRS 1900
	// IDA: scrs_loadRejectPool(0, 8, 1900)
	for (uint16 i = 0; i < 8; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 1400),
				  1900 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 10 normal scripts at SCRS 1000
	// IDA: scrs_loadNormalPool(1, 10, 1000)
	for (uint16 i = 0; i < 10; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 1400),
				  1000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// --- Puzzle-specific feature runners ---

	// IDA: word_4AB13A = runner_registerAndAllocate(..., 6, 0x641, standard, standard, 0xC000)
	// Landscape overlay animation (SCRB 1601)
	_landscapeFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 1600), 1601, 6,
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// IDA: word_4AB17A — boat animation runner. First visit uses 1803; subsequent visits random from pool.
	{
		uint16 boatScrb;
		if (_visitCounter == 1) {
			boatScrb = 1803;
		} else {
			uint16 idx = _vm->_rnd->getNonRepeatRandom(4, _boatRandomState);
			boatScrb = kBoatScrbPool[idx];
		}
		_boatAnimFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 1800), boatScrb, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_00100000_PLAY_ONCE);
	}

	// IDA: conditional on !g_pGameState->wMoreActionFlag0020
	// Boat approach runners — only loaded when "more action" mode is active (lessAction=false)
	if (!_vm->_state->isLessActionEnabled()) {
		// IDA: word_4AB13E = runner_registerAndAllocate(..., 6, 1602, standard, standard, 0x8000)
		_boatApproachA = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 1600), 1602, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM);

		// IDA: word_4AB140 = runner_registerAndAllocate(..., 6, 1603, standard, standard, 0x8000)
		_boatApproachB = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 1600), 1603, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM);
	}

	// IDA: word_4AB142 = runner_registerAndAllocate(..., 6, 0x6A8, standard, standard, 0x1188000)
	// Departure overlay runner (SCRB 1704)
	_departOverlayFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 1700), 1704, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER);

	// IDA: runner_registerAndAllocate(..., 6, 0x640, standard, standard, 0) — anonymous (SCRB 1600)
	loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 1600), 1600, 6,
		ZmbFeature::FLAG_00000000_TYPE_SHAPES);

	// IDA: 3× word_4AB14C[i] = runner_registerAndAllocate(..., 0, 1450+i, standard, standard, 0x4000000)
	// Overlay SCRBs (1450-1452)
	for (int16 i = 0; i < 3; i++) {
		_overlayFeatures[i] = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 1450), 1450 + i, 0,
			ZmbFeature::FLAG_04000000_OVERLAY);
	}

	// Load Zoombinis from active pack at 20 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, stru_4A0E58, 20)
	loadZoombinisFromPack();

	// Load seat layout (creates seat + decoration runners on the raft)
	// IDA: ferry_selectSCRB() — called after zmb_loadAnimationsFromActivePack, before layout
	loadSeatLayout();

	// Layout and stagger walk-in (30ms walk delay)
	// IDA: zmb_layoutStaticAndWalkInGroups(0)
	layoutStaticAndWalkIn();
	assignStaggeredWalkDelays();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(1400);
	loadHelpButtonFeature();

	// Compute adjacency matrix from seat bounding boxes
	// IDA: ferry_drawAdjacencyLines(0) — called after ferry_selectSCRB + layout
	buildAdjacencyMatrix();

	// IDA: v2 = getDifficultyIdFromPuzzleFlag(FERRY_FLAG) - 2
	//   v2 == 0 (diff == LEVEL2) → 20074 (hard voice)
	//   else if routeLevel > 0   → random(20073, 20074)
	//   else                     → 20073
	{
		ZMB_DIFFICULTY_ID diffId = _vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagFerry);
		uint16 helpSoundId;
		if (diffId == ZMB_DIFFICULTY_LEVEL2_02) {
			helpSoundId = 20074;
		} else if (_difficultyLevel > 0) {
			helpSoundId = _vm->_rnd->getRandomNumber(20073, 20074);
		} else {
			helpSoundId = 20073;
		}
		_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, helpSoundId);
	}

	// Initialize idle fidget timer
	// IDA: dword_4AB10C = nextRand_410705(10800, 5400)
	_nextFidgetTime = _currentFrameTime + _vm->_rnd->getRandomNumber(5400, 10800);

	// IDA: word_4AB196 = zmb_countFeatureRunners()
	_totalZmbCount = 0;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if ((*it)->getId() >= 10000)
			_totalZmbCount++;
	}

	_isActive = true;
	_seatedCount = 0;
	_interactionLocked = false;
	_pendingFrogmanScrb = 0;
	_rejectWalkPending = false;
	_departAnimPending = false;
	_departAnimDone = false;
	_goButtonPressed = false;
	_consecutiveSuccesses = 0;
	_consecutiveFailures = 0;
	_successThreshold = 1;
	_hasReactedOnce = false;
	_ambientStarted = false;
	_matchBitmask = 0;
	_attrDisplaySnoid = 0;
	_rejectSnoidId = 0;
}

void ZoombiniInteractiveFerry::onGoButtonActivated() {
	// IDA: ferry_onClickHandler case 2 -> word_4AB17C=1 -> puzzle_pendingTransitionTarget = 11
	// Route 2: Ferry -> Lilly (via Xfer)
	_departXferSrcSiPage = ZMB_SI_FERRY_06;
	if (_rejectWalkPending)
		_interactionLocked = true;
	_goButtonPressed = true;
}

void ZoombiniInteractiveFerry::loadZoombinisFromPack() {
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

// ---------------------------------------------------------------------------
// loadSeatLayout: Parse the seating layout SCRB and create seat + decoration
// runners on the raft.
// IDA: scrb_loadHotspotLayout_ferry (0x41b779) called from ferry_selectSCRB (0x41bc4e).
//
// The seating SCRB (1510-1529) contains 2 frame groups:
//   Frame 0 — seat entries (shape IDs 1-3), creates DRAW_ON_REG runners
//   Frame 1 — decoration entries (shape IDs 4-10), creates overlay runners
//
// Within each frame group, shape=0 entries act as "sub-frame" delimiters.
// The original engine interleaves the two frame groups at sub-frame boundaries:
//   sub-frame N from frame 0, then sub-frame N from frame 1, etc.
// This determines z-order via priority chaining from _overlayFeatures[0].
//
// For seats (shape 1-3):
//   SCRB ID = shapeId + 1499 (→ 1500, 1501, 1502)
//   Flags: DRAW_ON_REG | CHAIN_SCRIPT | DEFER_ANIM | POS_DELTA | OVERLAY | ZSORT_*
//   Snap position stored at layout pos + (22, -7)
//
// For decorations (shape 4-10, only when !lessAction):
//   SCRB ID = shapeId + 1499 (→ 1503-1509)
//   Flags: DEFER_ANIM | PLAY_ONCE | POS_DELTA | OVERLAY | ZSORT_*
// ---------------------------------------------------------------------------
void ZoombiniInteractiveFerry::loadSeatLayout() {
	// Parse the seating SCRB to extract layout entries from both frame groups
	Common::SeekableReadStream *stream = _vm->getResource(ID_SCRB,
		ZmbResource(ZmbArchiveKind::kPage, _seatingSCRB));

	uint16 frameCount = stream->readUint16BE();
	if (frameCount < 2) {
		warning("Ferry: seating SCRB %d has only %d frames (expected >= 2)", _seatingSCRB, frameCount);
		delete stream;
		return;
	}

	// Parse both frame groups into sub-frame batches.
	// A sub-frame batch = sequence of entries between shape=0 delimiters.
	struct LayoutEntry {
		int16 shapeId;
		Common::Point pos;
	};
	typedef Common::Array<LayoutEntry> SubFrame;
	Common::Array<SubFrame> seatSubFrames;  // Frame 0 (seats)
	Common::Array<SubFrame> decoSubFrames;  // Frame 1 (decorations)

	for (uint16 frame = 0; frame < 2; frame++) {
		Common::Array<SubFrame> &targetSubFrames = (frame == 0) ? seatSubFrames : decoSubFrames;
		SubFrame currentBatch;

		while (!stream->eos()) {
			int16 shapeId = stream->readSint16BE();
			if (shapeId < 0) {
				// Frame terminator — push remaining batch and move to next frame
				if (!currentBatch.empty())
					targetSubFrames.push_back(currentBatch);
				break;
			}

			int16 x = stream->readSint16BE();
			int16 y = stream->readSint16BE();

			if (shapeId > 0) {
				LayoutEntry entry;
				entry.shapeId = shapeId;
				entry.pos = Common::Point(x, y);
				currentBatch.push_back(entry);
			} else {
				// shape=0: sub-frame delimiter
				targetSubFrames.push_back(currentBatch);
				currentBatch.clear();
			}
		}
	}

	delete stream;

	// Interleave frame groups and create runners, matching the original's z-order.
	// IDA: scrb_loadHotspotLayout_ferry alternates between frame groups at
	// sub-frame boundaries. Priority chains from _overlayFeatures[0].
	// Seat runners with FLAG_DRAW_ON_REG are auto-registered by registerFeature(),
	// populating the base class _drawOnRegRunnerIds/SnapPositions/Occupancy arrays.

	const uint32 seatFlags =
		ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00040000_CHAIN_SCRIPT |
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00800000_POS_DELTA |
		ZmbFeature::FLAG_04000000_OVERLAY |
		ZmbFeature::FLAG_10000000_ZSORT_RIGHT | ZmbFeature::FLAG_20000000_ZSORT_BOTTOM |
		ZmbFeature::FLAG_40000000_ZSORT_LEFT;

	const uint32 decoFlags =
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_00800000_POS_DELTA | ZmbFeature::FLAG_04000000_OVERLAY |
		ZmbFeature::FLAG_10000000_ZSORT_RIGHT | ZmbFeature::FLAG_20000000_ZSORT_BOTTOM |
		ZmbFeature::FLAG_40000000_ZSORT_LEFT;

	bool lessAction = _vm->_state->isLessActionEnabled();
	uint32 maxSubFrameCount = MAX(seatSubFrames.size(), decoSubFrames.size());
	int16 seatCountBefore = _drawOnRegCount;

	for (uint32 sf = 0; sf < maxSubFrameCount; sf++) {
		// Process seat entries from frame 0 sub-frame
		if (sf < seatSubFrames.size()) {
			for (uint32 e = 0; e < seatSubFrames[sf].size(); e++) {
				const LayoutEntry &entry = seatSubFrames[sf][e];
				if (entry.shapeId >= 1 && entry.shapeId <= 3 && (_drawOnRegCount - seatCountBefore) < 20) {
					uint16 scrbId = entry.shapeId + 1499;

					loadScrbFeature(
						ZmbResource(ZmbArchiveKind::kPage, 1500), scrbId, 6,
						entry.pos, seatFlags);

					// registerFeature() auto-registered the slot with entry.pos as snap.
					// Override snap position to +22,-7 offset (IDA: posArr_4B7C44[idx])
					setDrawOnRegSnapPosition(_drawOnRegCount - 1,
						Common::Point(entry.pos.x + 22, entry.pos.y - 7));
				}
			}
		}

		// Process decoration entries from frame 1 sub-frame
		if (!lessAction && sf < decoSubFrames.size()) {
			for (uint32 e = 0; e < decoSubFrames[sf].size(); e++) {
				const LayoutEntry &entry = decoSubFrames[sf][e];
				if (entry.shapeId >= 4 && entry.shapeId <= 10) {
					uint16 scrbId = entry.shapeId + 1499;

					loadScrbFeature(
						ZmbResource(ZmbArchiveKind::kPage, 1500), scrbId, 6,
						entry.pos, decoFlags);
				}
			}
		}
	}

	debugC(kZmbDebugPage, "Ferry: loaded seat layout SCRB %d — %d seats, %u seat sub-frames, %u deco sub-frames",
		   _seatingSCRB, _drawOnRegCount, seatSubFrames.size(), decoSubFrames.size());
}

// ---------------------------------------------------------------------------
// buildAdjacencyMatrix: Compute adjacency between seat positions.
// IDA: ferry_drawAdjacencyLines (0x41bcc7) with arg0=0 (no draw).
//
// For every pair of seats, test if their expanded bounding boxes overlap.
// Two overlap test orientations are always tried (vertical + horizontal expand).
// Difficulty >= 3 adds a third test (raw overlap with no expansion).
// Matching pairs store 1-based neighbor IDs in the adjacency matrix (max 8 per seat).
// ---------------------------------------------------------------------------
void ZoombiniInteractiveFerry::buildAdjacencyMatrix() {
	memset(_adjacencyMatrix, 0, sizeof(_adjacencyMatrix));

	// Gather seat bounding rects from seat runners
	// IDA: each runner's core188 at offset +206/+210 stores clickRect leftTop/rightBottom
	Common::Rect seatRects[20];
	for (int16 i = 0; i < _drawOnRegCount; i++) {
		ZmbFeature *seatRunner = _scrbFeatures.find(_drawOnRegRunnerIds[i]);
		if (seatRunner) {
			seatRects[i] = seatRunner->getClickRect();
		}
	}

	for (int16 k = 0; k < _drawOnRegCount; k++) {
		int16 slotCount = 0;
		const Common::Rect &rectK = seatRects[k];
		int16 halfHeight = (rectK.bottom - rectK.top) / 2 - 2;

		for (int16 m = 0; m < _drawOnRegCount; m++) {
			if (m == k)
				continue;

			const Common::Rect &rectM = seatRects[m];
			bool adjacent = false;

			// Test 1: Vertical expansion — expand top/bottom by halfHeight
			{
				Common::Rect expandedK(rectK.left + halfHeight, rectK.top - halfHeight,
				                       rectK.right - halfHeight, rectK.bottom + halfHeight);
				adjacent = expandedK.intersects(rectM);
			}

			// Test 2: Horizontal expansion — expand left/right by halfHeight
			if (!adjacent) {
				Common::Rect expandedK(rectK.left - halfHeight, rectK.top + halfHeight,
				                       rectK.right + halfHeight, rectK.bottom - halfHeight);
				adjacent = expandedK.intersects(rectM);
			}

			// Test 3: Raw overlap (difficulty >= 3 only)
			if (!adjacent && _difficultyLevel >= 3) {
				adjacent = rectK.intersects(rectM);
			}

			if (adjacent && slotCount < 8) {
				_adjacencyMatrix[k][slotCount] = static_cast<byte>(m + 1); // 1-based
				slotCount++;
			}
		}
	}

	debugC(kZmbDebugPage, "Ferry: built adjacency matrix for %d seats", _drawOnRegCount);
}

// ---------------------------------------------------------------------------
// getDropTargetSeat: Test if a point is near any empty seat snap position.
// IDA: click_testZoneRadius_455DFB — builds ±clickZoneRadius rect around pos,
// tests each posArr_4B7C44 slot. Returns 0-based seat index, or -1 if no match.
// ---------------------------------------------------------------------------
int16 ZoombiniInteractiveFerry::getDropTargetSeat(const Common::Point &pos) const {
	return hitTestDrawOnRegSlot(pos, _clickZoneRadius, true);
}

// ---------------------------------------------------------------------------
// testAdjacentMatch: Check if a dropped snoid shares any trait with
// any occupied adjacent seat.
// IDA: ferry_onClickHandler case 4, the inner loop.
// seatIdx is 0-based. Returns true if valid placement; also sets _matchBitmask.
// ---------------------------------------------------------------------------
bool ZoombiniInteractiveFerry::testAdjacentMatch(int16 seatIdx, ZmbSnoid *droppedSnoid) {
	_matchBitmask = 0;
	bool anyNeighborFound = false;

	for (int16 slot = 0; slot < 8; slot++) {
		byte neighborIdx = _adjacencyMatrix[seatIdx][slot];
		if (neighborIdx == 0)
			continue;

		// IDA: runner_findByIndex(word_4B7E36[adjacency_entry])
		// neighborIdx is 1-based → use neighborIdx-1 as 0-based draw-on-reg slot.
		uint16 occupantId = getDrawOnRegOccupant(neighborIdx - 1);
		if (occupantId == 0)
			continue;

		ZmbSnoid *neighborSnoid = getSnoid(occupantId);
		if (!neighborSnoid)
			continue;

		anyNeighborFound = true;

		// IDA: Compare 4 trait bytes (foot, nose, eye, head — indices 0-3)
		// The trait struct layout is: _head(0), _eye(1), _nose(2), _foot(3)
		const byte *droppedTraits = reinterpret_cast<const byte *>(&droppedSnoid->_trait);
		const byte *neighborTraits = reinterpret_cast<const byte *>(&neighborSnoid->_trait);
		bool matchFound = false;
		for (int16 j = 0; j < 4; j++) {
			if (droppedTraits[j] == neighborTraits[j]) {
				// Original: j=0 → |=1, j=1 → |=2, j=2 → |=4, j=3 → |=8
				_matchBitmask |= (1u << j);
				matchFound = true;
			}
		}
		if (matchFound)
			return true;
	}

	return !anyNeighborFound; // If no neighbors exist (first seat), always valid
}

// ---------------------------------------------------------------------------
// findIdlePackSnoid: Find idle Zoombini from pack (IDs >= 10000).
// IDA: zmb_findIdleFeatureRunner (0x456A95)
// ---------------------------------------------------------------------------
ZmbSnoid *ZoombiniInteractiveFerry::findIdlePackSnoid(uint16 preferredId) {
	if (preferredId > 0) {
		ZmbSnoid *snoid = getSnoid(preferredId);
		if (snoid && snoid->getAnimState() == kSnoidAnimIdle)
			return snoid;
	}
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if ((*it)->getId() < 10000)
			continue;
		ZmbSnoid *snoid = *it;
		if (snoid->getAnimState() == kSnoidAnimIdle)
			return snoid;
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// startRejectWalk: Set up the reject walk animation.
// IDA: puzzleFerry_1705_1706_41BA30
// ---------------------------------------------------------------------------
void ZoombiniInteractiveFerry::startRejectWalk(int16 destination) {
	_rejectDestination = destination;

	if (_rejectSnoidId == 0)
		return;

	// IDA: Select reject walk SCRB based on destination
	if (destination >= 10 || destination == 0) {
		// Dock exit
		_rejectWalkScrb = 1605;
		_rejectWalkDest = Common::Point(122, 164); // IDA: 0xA4007A
	} else if (destination >= 1 && destination <= 6) {
		// Rowboat ride — 50/50 chance of 1604 or 1606
		if (_vm->_rnd->getRandomNumber(1, 100) > 50)
			_rejectWalkScrb = 1606;
		else
			_rejectWalkScrb = 1604;
		_rejectWalkDest = _savedDragOrigin;
	} else if (destination >= 7 && destination <= 9) {
		// Raft ride — uses SCRB 1607 with extra departure runners
		_rejectWalkScrb = 1607;

		// Free existing departure runners and create new ones for raft
		// IDA: word_4AB144, word_4AB146
		if (_departRunnerA) {
			_departRunnerA->deactivateAnimate();
			_departRunnerA = nullptr;
		}
		if (_departRunnerB) {
			_departRunnerB->deactivateAnimate();
			_departRunnerB = nullptr;
		}

		_departRunnerA = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 1700), 1705, 6,
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_01000000_DEFER_RENDER);

		// IDA: coordPair.x = dword_4AB114 - 14; coordPair.y = HIWORD(dword_4AB114) - 14
		Common::Point raftPos(_savedDragOrigin.x - 14, _savedDragOrigin.y - 14);
		_departRunnerB = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 1700), 1706, 6,
			ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_00800000_POS_DELTA | ZmbFeature::FLAG_01000000_DEFER_RENDER);

		_rejectWalkDest = Common::Point(236, 474); // IDA: 0x1DA00EC
		_rejectWalkPos2 = _savedDragOrigin;
	}

	// Load reject walk SCRB onto boat runner and set approach callback
	// IDA: scrb_loadOnRunner(1, word_4AB19E, v2)
	if (_boatAnimFeature) {
		loadScrbOntoFeature(_boatAnimFeature, _rejectWalkScrb);
	}

	_departAnimPending = true;
}

// ---------------------------------------------------------------------------
// handleRejectWalkSetup: Called from onEveryFrame when reject walk is pending.
// IDA: ferry_funcOnHover, word_4AB12A branch
// ---------------------------------------------------------------------------
void ZoombiniInteractiveFerry::handleRejectWalkSetup() {
	_rejectWalkPending = false;

	// IDA: picker_findOpenSlotForZmb — find an open slot to send the rejected zmb to
	// In ScummVM, pick a non-repeat random destination from 0-9 range
	int16 dest;
	bool retry;
	do {
		retry = false;
		dest = _vm->_rnd->getNonRepeatRandom(10, _rejectWalkRandomState);

		// IDA: destinations 7-9 require checking if certain back-row slots are available
		// (zmb_sortedRunnerIds[19], [17], [15] etc. for rows 11-19 odd indices)
		if (dest >= 7 && dest <= 9) {
			// Check if back row positions are available
			bool backRowAvailable = false;
			for (int16 i = 19; i >= 11; i -= 2) {
				if (i < static_cast<int16>(_drawOnRegCount * 2)) {
					// Check if this position slot is unoccupied
					backRowAvailable = true;
					break;
				}
			}
			if (!backRowAvailable)
				retry = true;
		}

		// IDA: destination 0 check — dock positions must be available
		if (dest == 0) {
			// Dock area must have space
			// In IDA: unk_4B6DE6 || unk_4B6DEA check — dock occupancy
		}
	} while (retry);

	_savedDragOrigin = kSnoidPositions[MIN<int16>(dest, 19)];
	startRejectWalk(dest);
}

// ---------------------------------------------------------------------------
// onEveryFrame: Per-frame tick for ferry puzzle.
// IDA: ferry_funcOnHover (0x41a9f6)
// ---------------------------------------------------------------------------
void ZoombiniInteractiveFerry::onEveryFrame() {
	if (!_isActive)
		return;

	// -----------------------------------------------------------------------
	// [0] Pending Go departure
	// IDA: word_4AB17C && !word_4AB12A && !word_4AB118
	// -----------------------------------------------------------------------
	if (_goButtonPressed && !_rejectWalkPending && !_interactionLocked) {
		_goButtonPressed = false;

		// Free landscape and approach runners
		// IDA: runner_freeByIndex(word_4AB13A), runner_freeByIndex(word_4AB13E/word_4AB140)
		if (_landscapeFeature) {
			_landscapeFeature->deactivateAnimate();
			_landscapeFeature->deactivateRender();
			_landscapeFeature = nullptr;
		}
		if (_boatApproachA) {
			_boatApproachA->deactivateAnimate();
			_boatApproachA->deactivateRender();
		}
		if (_boatApproachB) {
			_boatApproachB->deactivateAnimate();
			_boatApproachB->deactivateRender();
		}

		// Play departure SCRB (random 1608-1609)
		// IDA: scrb_initRunnerWithScript(0, caves_shiftRunnerPositions_41BBEA, rand(1608,1609), ...)
		uint16 departScrb = _vm->_rnd->getRandomNumber(1608, 1609);
		if (_boatAnimFeature) {
			loadScrbOntoFeature(_boatAnimFeature, departScrb);
		}

		_departAnimDone = false;

		// Set transition target: Ferry → Slides (page 11)
		// IDA: puzzle_pendingTransitionTarget = 11
		executeDeparture();
		return;
	}

	// -----------------------------------------------------------------------
	// [1] Pending frogman SCRB animation
	// IDA: word_4AB128 branch
	// -----------------------------------------------------------------------
	if (_pendingFrogmanScrb != 0) {
		uint16 scrb = _pendingFrogmanScrb;
		_pendingFrogmanScrb = 0;

		if (_boatAnimFeature) {
			loadScrbOntoFeature(_boatAnimFeature, scrb);
		}
	}
	// -----------------------------------------------------------------------
	// [2] Departure animation pending (reject walk overlay)
	// IDA: word_4AB12C branch
	// -----------------------------------------------------------------------
	else if (_departAnimPending) {
		_departAnimPending = false;

		// Activate departure overlay runners
		// IDA: scrb_initRunnerWithScript(0, tunnels_zmbApproachGateCallback, 0, word_4AB142)
		if (_departOverlayFeature) {
			_departOverlayFeature->activateAnimate();
			_departOverlayFeature->activateRender();
		}
		if (_departRunnerA) {
			_departRunnerA->activateAnimate();
			_departRunnerA->activateRender();
		}
	}
	// -----------------------------------------------------------------------
	// [3] Reject walk pending — set up reject animation
	// IDA: word_4AB12A branch — waits for frogman animation to complete
	// -----------------------------------------------------------------------
	else if (_rejectWalkPending) {
		// IDA: Wait for frogman hotspot group to clear before starting reject walk
		// Check if frogman animation is no longer playing
		if (_boatAnimFeature && !_boatAnimFeature->isAnimateActivated()) {
			handleRejectWalkSetup();
		}
	}
	// -----------------------------------------------------------------------
	// [4] Idle fidget timer
	// IDA: getElapsedFrameTime > dword_4AB10C branch
	// -----------------------------------------------------------------------
	else if (_currentFrameTime > _nextFidgetTime) {
		// Select random fidget SCRB
		// IDA: word_4A0D08[e2GetPoolValue_nonRepeatRandom(0, 5, &dword_4A0D14)]
		uint16 idx = _vm->_rnd->getNonRepeatRandom(5, _fidgetRandomState);
		_pendingFrogmanScrb = kFidgetScrbPool[idx];

		// Reset fidget timer: 5400-10800 ms
		_nextFidgetTime = _currentFrameTime + _vm->_rnd->getRandomNumber(5400, 10800);
	}

	// -----------------------------------------------------------------------
	// [5] Attribute display scheduling
	// IDA: word_4AB18E branch — schedule attribute match display on a snoid
	// -----------------------------------------------------------------------
	if (_attrDisplaySnoid != 0) {
		ZmbSnoid *snoid = findIdlePackSnoid(_attrDisplaySnoid);
		if (snoid) {
			// IDA: snoid[1].core188.u.s.pcStr1[9] = word_4AB18C
			// Store match bitmask for attribute display
			_matchBitmask = 0;
			_attrDisplaySnoid = 0;
		}
	}

	// -----------------------------------------------------------------------
	// [6] Update Go button enabled state
	// -----------------------------------------------------------------------
	setGoButtonsEnabled(_seatedCount > 0);

	// -----------------------------------------------------------------------
	// [7] Ambient sound scheduling
	// IDA: word_4AB138 check — start ambient after drag lock releases
	// -----------------------------------------------------------------------
	if (!_ambientStarted && !isDragging()) {
		_ambientStarted = true;
	}

	// Ambient sound is driven by the base interactive frame loop
}

// ---------------------------------------------------------------------------
// onLButtonDown: Click handler.
// IDA: ferry_onClickHandler (0x41ae20)
// ---------------------------------------------------------------------------
ZmbEventHandleResult ZoombiniInteractiveFerry::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// Handle sticky mouse drop on second click
	if (isDragging() && _vm->_state->getEnableStickyMouse()) {
		endDrag(absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	// Let interactive base handle Go/Map/Help buttons
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Guard: don't allow dragging during interaction lock or departure
	if (_interactionLocked || _goButtonPressed)
		return ZmbEventHandleResult::kPassthrough;
	if (isDragging())
		return ZmbEventHandleResult::kPassthrough;

	// Find snoid at click position
	ZmbSnoid *snoid = findSnoidAtPoint(absPos);
	if (!snoid)
		return ZmbEventHandleResult::kPassthrough;

	// Guard: can't drag during reject walk or departure
	if (_departAnimDone || _goButtonPressed)
		return ZmbEventHandleResult::kPassthrough;

	// IDA: save the pcStr1[11] (seated state) and reset it
	// IDA: dword_4AB114 = snoid->core188.posLoc
	_savedDragOrigin = snoid->getPointLoc();

	// Begin drag
	startSnoidDrag(snoid, absPos);

	// Play move SFX: pick from kMoveReactionPool if in dock area
	// IDA: click_testZoneRadius(posLoc) check
	if (!_pendingFrogmanScrb && kDockRect.contains(_savedDragOrigin.x, _savedDragOrigin.y)) {
		uint16 idx = _vm->_rnd->getNonRepeatRandom(3, _moveReactionRandomState);
		_pendingFrogmanScrb = kMoveReactionPool[idx];
	}

	return ZmbEventHandleResult::kConsumed;
}

// ---------------------------------------------------------------------------
// onLButtonUp: Release drag.
// ---------------------------------------------------------------------------
ZmbEventHandleResult ZoombiniInteractiveFerry::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	if (!isDragging())
		return ZoombiniInteractive::onLButtonUp(absPos, relPos);

	// In sticky mouse mode, don't end on button-up
	if (_vm->_state->getEnableStickyMouse())
		return ZmbEventHandleResult::kConsumed;

	endDrag(absPos);
	return ZmbEventHandleResult::kConsumed;
}

// ---------------------------------------------------------------------------
// endDrag: Process drag completion.
// IDA: ferry_onClickHandler case 4, after beginDragFeatureRunner.
// Uses draw-on-reg occupancy for seat tracking and snap positions.
// ---------------------------------------------------------------------------
void ZoombiniInteractiveFerry::endDrag(const Common::Point &mousePos) {
	ZmbSnoid *snoid = finishSnoidDrag();
	if (!snoid)
		return;

	Common::Point snoidPos = snoid->getPointLoc();
	_dropTargetSeat = getDropTargetSeat(snoidPos);

	if (_dropTargetSeat >= 0) {
		// Dropped on an empty seat — test adjacency matching
		bool valid = testAdjacentMatch(_dropTargetSeat, snoid);

		if (valid) {
			// ---------------------------------------------------------------
			// [CORRECT PLACEMENT]
			// IDA: scrb_drawOnRegFlagArr[dropSlotIdx] = origRunnerIdx
			// IDA: word_4AB192 = 0; ++word_4AB190
			// ---------------------------------------------------------------
			setDrawOnRegOccupant(_dropTargetSeat, snoid->getId());
			_consecutiveFailures = 0;
			_consecutiveSuccesses++;

			// IDA: Check if success threshold met for good reaction
			if (_seatedCount + 1 == _totalZmbCount || _consecutiveSuccesses == _successThreshold) {
				_successThreshold += _vm->_rnd->getRandomNumberSigned(3, 5);

				if (_hasReactedOnce) {
					uint16 idx = _vm->_rnd->getNonRepeatRandom(2, _goodReactionRandomState);
					_pendingFrogmanScrb = kGoodReactionPool[idx];
				} else {
					_hasReactedOnce = true;
					_pendingFrogmanScrb = 1816; // IDA: first good reaction = 1816
				}
			}

			// Mark snoid as seated — set arrive target to snap position
			// IDA: posArr_4B7C44[dropSlotIdx] → animateZoombini(0, 4, pZmb)
			snoid->_packIsOccupied = true;
			snoid->setAnimTargetPos(_drawOnRegSnapPositions[_dropTargetSeat]);
			snoid->setAnimState(kSnoidAnimArrive);

			// IDA: if matching bitmask && practice level, show attribute match
			if (_matchBitmask && _vm->_state->readActivePageRouteLevel() > 0) {
				_attrDisplaySnoid = snoid->getId();
			}

			_seatedCount++;
		} else {
			// ---------------------------------------------------------------
			// [WRONG PLACEMENT]
			// IDA: ++word_4AB192; word_4AB190=0; word_4AB194=1
			// ---------------------------------------------------------------
			_consecutiveFailures++;
			_consecutiveSuccesses = 0;
			_successThreshold = 1;
			_interactionLocked = true;

			// Mark snoid for rejection
			snoid->_packIsOccupied = false;
			_rejectSnoidId = snoid->getId();

			// IDA: word_4AB148 = word_4AB14C[word_4AB148]
			// Store rejected seat for animation target

			// Select rejection reaction SCRB
			// IDA: if (nextRand(5,3) == word_4AB192) → 1815 (harsh), else random from bad pool
			if (_vm->_rnd->getRandomNumberSigned(3, 5) == _consecutiveFailures) {
				_pendingFrogmanScrb = 1815;
				_consecutiveFailures = 5; // prevent further harsh rejects
			} else {
				uint16 idx = _vm->_rnd->getNonRepeatRandom(11, _badReactionRandomState);
				_pendingFrogmanScrb = kBadReactionPool[idx];
			}

			_rejectWalkPending = true;
		}
	} else {
		// Dropped outside any seat — check terrain or dock validity
		// IDA: terrain_validateAndPlaceSnoid — or return to origPos
		if (!validateTerrainDrop(snoid)) {
			// IDA: return to origPos via arrive anim
			snoid->setAnimTargetPos(_savedDragOrigin);
			snoid->setAnimState(kSnoidAnimArrive);
		} else {
			snoid->setAnimState(kSnoidAnimIdle);
			snoid->setupIdleHotspots();
		}

		// IDA: if !word_4AB128 && click_testZoneRadius(posLoc)
		if (!_pendingFrogmanScrb && kDockRect.contains(snoidPos.x, snoidPos.y)) {
			uint16 idx = _vm->_rnd->getNonRepeatRandom(3, _moveReactionRandomState);
			_pendingFrogmanScrb = kMoveReactionPool[idx];
		}
	}

	// IDA: word_4AB136 — count occupied draw-on-reg slots
	_seatedCount = 0;
	for (int16 i = 0; i < _drawOnRegCount; i++) {
		if (_drawOnRegOccupancy[i] != 0)
			_seatedCount++;
	}
}

// ---------------------------------------------------------------------------
// onFeatureAnimEvent: Animation event callback.
// IDA: dispatched via hotspot group callbacks
// ---------------------------------------------------------------------------
void ZoombiniInteractiveFerry::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (feature == _boatAnimFeature) {
		// Frogman/boat animation completed
		if (eventCode == kZmbAnimEventM1_End) {
			// IDA: End of SCRB chain — frogman returns to idle
			_frogmanHotspotGroup = 0;
		}
	} else if (feature == _departOverlayFeature || feature == _departRunnerA || feature == _departRunnerB) {
		// Departure overlay completed
		if (eventCode == kZmbAnimEventM1_End) {
			_departAnimDone = true;
			_interactionLocked = false;
		}
	} else if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
		// Snoid animation event
		ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);
		if (eventCode == kZmbAnimEventM1_End) {
			// SCRS playback completed — return snoid to idle
			snoid->setAnimState(kSnoidAnimIdle);
			snoid->setupIdleHotspots();
		}
	}
}

} // End of namespace Mohawk
