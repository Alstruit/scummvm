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
const Common::Point ZoombiniPuzzleFerry::kSnoidPositions[20] = {
	Common::Point(370, 160), Common::Point(395, 196), Common::Point(332, 156), Common::Point(348, 196),
	Common::Point(294, 168), Common::Point(316, 196), Common::Point(253, 166), Common::Point(276, 196),
	Common::Point(214, 157), Common::Point(237, 196), Common::Point(175, 160), Common::Point(196, 190),
	Common::Point(135, 152), Common::Point(150, 191), Common::Point( 94, 145), Common::Point(110, 186),
	Common::Point( 57, 146), Common::Point( 71, 182), Common::Point( 25, 145), Common::Point( 27, 183),
};

// IDA: qword_4A0EBC — dock area rect
const Common::Rect ZoombiniPuzzleFerry::kDockRect(0, 130, 469, 240);

// IDA: word_4A0CFC — boat approach SCRB pool (4 entries)
const uint16 ZoombiniPuzzleFerry::kBoatScrbPool[4] = { 1800, 1801, 1802, 1803 };

// IDA: word_4A0D08 — captain idle fidget SCRB pool (5 entries)
const uint16 ZoombiniPuzzleFerry::kFidgetScrbPool[5] = { 1823, 1824, 1825, 1826, 1827 };

// IDA: word_4A0D18 — correct placement reaction SCRB pool (2 entries)
const uint16 ZoombiniPuzzleFerry::kGoodReactionPool[2] = { 1817, 1818 };

// IDA: word_4A0D20 — wrong placement reaction SCRB pool (11 entries)
const uint16 ZoombiniPuzzleFerry::kBadReactionPool[11] = {
	1804, 1805, 1806, 1807, 1808, 1809, 1810, 1811, 1812, 1813, 1814
};

// IDA: word_4A0D4C — moved-from-dock reaction SCRB pool (3 entries)
const uint16 ZoombiniPuzzleFerry::kMoveReactionPool[3] = { 1820, 1821, 1822 };

// Reject-flight lookup tables, keyed by destination (0..9):
//   0       = dock exit
//   1..6    = rowboat seats
//   7..9    = raft seats
//
// (IDA `word_4A0D58` (10 int16) lists boat SCRBs 1700-1703 for case 1's
//  `caves_swapRunnerScrb` (0x41B463). That swap is a no-op for Ferry because
//  the global it indexes by holds a seat number rather than a runner-table
//  index; the boat keeps playing the controller SCRB throughout the flight.
//  Table omitted from the port.)

// IDA: word_4A0D6C (10 int16) — primary snoid SCRS loaded by case 1's
// `tunnels_playZmbScript(1, callback, ...)` (0x41B566). Pool 0 (NORMAL state 9)
// + chRand_64_0=1 (hide-on-complete). This is the actual flight visual: the
// snoid plays this SCRS (with embedded position deltas) and then disappears.
static const uint16 kRejectFlightSnoidScrsA[10] = {
	1902, 1900, 1900, 1904, 1904, 1906, 1906, 1900, 1904, 1906
};

// IDA: word_4A0D80 (10 int16) — secondary snoid SCRS loaded by case 2's
// `tunnels_playZmbScript(0, 0, ...)` (0x41B5CF) at SCRB 1605's later frame.
// Pool 0 + chRand_64_0=0 (return-to-idle on complete) + pInitPos=&dword_4AB11A
// so the SCRS lands the snoid at the destination point. Value 1907 in the
// dest 7-9 entries triggers the raft branch instead (runner linking +
// _departAnimPending) per IDA 0x41B581.
static const uint16 kRejectFlightSnoidScrsB[10] = {
	1903, 1901, 1905, 1901, 1905, 1901, 1905, 1907, 1907, 1907
};

ZoombiniPuzzleFerry::ZoombiniPuzzleFerry(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kFerry) {
}

ZoombiniPuzzleFerry::~ZoombiniPuzzleFerry() {
}

void ZoombiniPuzzleFerry::open() {
	openArchive(ZMB_MHK_FERRY);
}

void ZoombiniPuzzleFerry::setBackgroundMusic() {
	// IDA: ferry_funcInit (0x41a394) has NO music playback call on page load.
	// sound_activeHandle (20073/20074) is stored at the END of funcInit for F1 replay only.
	// scrb_enqueueSoundResource(0, SND_00997_MOVE_SHORT_SFX) plays a UI click via SCRB when
	// walk animations start — it is NOT a narrator voice and is handled by the SCRB system.
	// Therefore no sound plays here; the narrator voice must not auto-play on page load.
}

void ZoombiniPuzzleFerry::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(1300)
	_vm->_gfx->setPalette(1300);
	_vm->_gfx->drawBackground(1300);
}

void ZoombiniPuzzleFerry::loadFeatures() {
	// IDA: ferry_funcInit (0x41a394)
	_difficultyLevel = static_cast<ZmbPuzzleDifficultyLevel>(_vm->_state->readActivePageRouteLevel() + 1); // 1-based (1-4)
	_visitCounter++;

	// IDA: ferry_selectSCRB (0x41bc4e) — calculate SCRB ID based on difficulty and zoombini count
	{
		int16 zmbCount = _vm->_state->_f._zmbPackActive._wPackZmbCount;
		if (zmbCount < 16)
			zmbCount = 16;
		else if (zmbCount > 20)
			zmbCount = 20;

		uint16 scrbBase = 1510 + ((_difficultyLevel - 1) * 5);
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

	// Compute adjacency matrix from seat bounding boxes.
	// IDA: ferry_drawAdjacencyLines(0) — called after ferry_selectSCRB + layout,
	// AND after gfx_renderFrame (0x41a724). The render populates each seat's
	// clickRect via runner_preRenderStandard; only then is the bounding box
	// available. In ScummVM we haven't rendered yet, so defer the build to the
	// first drop test (see testAdjacentMatch).
	_adjacencyBuilt = false;

	// IDA: v2 = getDifficultyIdFromPuzzleFlag(FERRY_FLAG) - 2
	//   v2 == 0 (diff == LEVEL2) → 20074 (hard voice)
	//   else if routeLevel > 0   → random(20073, 20074)
	//   else                     → 20073
	{
		ZMB_DIFFICULTY_ID diffId = _vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagFerry);
		uint16 helpSoundId;
		if (diffId == ZMB_DIFFICULTY_LEVEL2_02) {
			helpSoundId = 20074;
		} else if (_difficultyLevel >= kPuzzleDiffLevel2) {
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
	_rejectingSnoid = nullptr;
	_rejectFlightActive = false;
	_rejectFlightLandingScrsActive = false;
}

void ZoombiniPuzzleFerry::onGoButtonActivated() {
	// IDA: ferry_onClickHandler case 2 -> word_4AB17C=1 -> puzzle_pendingTransitionTarget = 11
	// Route 2: Ferry -> Lilly (via Xfer)
	_departXferSrcSiPage = ZMB_SI_FERRY_07;
	if (_rejectWalkPending)
		_interactionLocked = true;
	_goButtonPressed = true;
}

Common::String ZoombiniPuzzleFerry::debugGetAnswer() const {
	Common::String s = Common::String::format("Ferry (level %d): seatingSCRB=%d, seats=%d\n",
		_difficultyLevel, _seatingSCRB, _drawOnRegCount);
	s += "  Rule: adjacent seats must share at least one trait\n";

	// Show current seating with occupant traits
	s += "  Seat occupancy:\n";
	for (int16 seat = 0; seat < _drawOnRegCount; seat++) {
		uint16 occupantId = getDrawOnRegOccupant(seat);
		s += Common::String::format("    seat %2d:", seat);
		if (occupantId == 0) {
			s += " (empty)";
		} else {
			ZmbSnoid *snoid = getSnoid(occupantId);
			if (snoid) {
				const ZmbTrait &t = snoid->_trait;
				s += Common::String::format(" head=%d(%s) eye=%d(%s) nose=%d(%s) foot=%d(%s)",
					t._head, ZmbTrait::debugTraitValueName(ZmbTrait::kTraitHair, t._head),
					t._eye, ZmbTrait::debugTraitValueName(ZmbTrait::kTraitEyes, t._eye),
					t._nose, ZmbTrait::debugTraitValueName(ZmbTrait::kTraitNose, t._nose),
					t._foot, ZmbTrait::debugTraitValueName(ZmbTrait::kTraitFeet, t._foot));
			}
		}
		// Show neighbors
		bool hasNeighbor = false;
		for (int n = 0; n < 8; n++) {
			if (_adjacencyMatrix[seat][n] != 0)
				hasNeighbor = true;
		}
		if (hasNeighbor) {
			s += " adj:[";
			bool first = true;
			for (int n = 0; n < 8; n++) {
				if (_adjacencyMatrix[seat][n] != 0) {
					if (!first)
						s += ",";
					s += Common::String::format("%d", _adjacencyMatrix[seat][n] - 1);
					first = false;
				}
			}
			s += "]";
		}
		s += "\n";
	}
	return s;
}

void ZoombiniPuzzleFerry::loadZoombinisFromPack() {
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
void ZoombiniPuzzleFerry::loadSeatLayout() {
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
void ZoombiniPuzzleFerry::buildAdjacencyMatrix() {
	memset(_adjacencyMatrix, 0, sizeof(_adjacencyMatrix));

	// Gather seat bounding rects from seat runners.
	// IDA reads each runner's clickRect (core188 +206/+210), populated by
	// runner_preRenderStandard during the gfx_renderFrame() call right before
	// ferry_drawAdjacencyLines. ScummVM's lazy-build path runs after many
	// natural frames, but DRAW_ON_REG features only set their clickRect when
	// they actually draw something — and several seat sub-shape SCRBs may not
	// produce a sortRect on frame 0 (e.g. shapeIdx=ZmbHotspot::kShapeNone or
	// no hotspot at frame 0). Fall back to a snap-position-centered box when
	// the click rect is empty so adjacency still classifies sensibly.
	//
	// Fallback box: 40x32 centered on the seat's snap position (which is
	// entry.pos + (22,-7), see loadSeatLayout). This roughly matches the IDA
	// seat shape footprint observed across the 5 difficulty seat tBMPs and
	// makes the IDA `halfHeight = (h/2)-2` formula yield ~14, comparable to
	// what a rendered seat shape produces.
	Common::Rect seatRects[20];
	for (int16 i = 0; i < _drawOnRegCount; i++) {
		ZmbFeature *seatRunner = _scrbFeatures.find(_drawOnRegRunnerIds[i]);
		Common::Rect rect;
		if (seatRunner)
			rect = seatRunner->getClickRect();
		if (rect.isEmpty() || rect.width() <= 0 || rect.height() <= 0) {
			const Common::Point &snap = _drawOnRegSnapPositions[i];
			rect = Common::Rect(snap.x - 20, snap.y - 16, snap.x + 20, snap.y + 16);
		}
		seatRects[i] = rect;
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

			// Test 3: Vertical-only expansion (difficulty >= LEVEL3).
			// IDA 0x41BE25-0x41BE49: v25=top-halfH, v27=bottom+halfH, v26=right,
			// v24=left — left/right unchanged, only vertical grown. Previous
			// port used `rectK.intersects(rectM)` which is the raw rect test,
			// and gated on == LEVEL4, both incorrect.
			if (!adjacent && _difficultyLevel >= kPuzzleDiffLevel3) {
				Common::Rect expandedK(rectK.left, rectK.top - halfHeight,
				                       rectK.right, rectK.bottom + halfHeight);
				adjacent = expandedK.intersects(rectM);
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
int16 ZoombiniPuzzleFerry::getDropTargetSeat(const Common::Point &pos) const {
	return hitTestDrawOnRegSlot(pos, _clickZoneRadius, true);
}

// ---------------------------------------------------------------------------
// testAdjacentMatch: Check if a dropped snoid shares any trait with
// any occupied adjacent seat.
// IDA: ferry_onClickHandler case 4, the inner loop.
// seatIdx is 0-based. Returns true if valid placement; also sets _matchBitmask.
// ---------------------------------------------------------------------------
bool ZoombiniPuzzleFerry::testAdjacentMatch(int16 seatIdx, ZmbSnoid *droppedSnoid) {
	// Lazy-build adjacency on first use (see loadFeatures for the rationale).
	if (!_adjacencyBuilt) {
		buildAdjacencyMatrix();
		_adjacencyBuilt = true;
	}

	// IDA ferry_funcOnClick @ 0x41B041:
	//   v4 = 1;  // optimistic: valid until proven otherwise
	//   for (i = 0; v4 && i < 8; ++i) {
	//     if (adjacency[i] && (runner = findByIndex(adj[i]))) {
	//       v4 = 0;  // reset: this neighbor must contribute a match
	//       for (j = 0; j < 4; ++j)
	//         if (droppedTrait[j] == neighborTrait[j]) {
	//           word_4AB18C |= 1<<j;
	//           v4 = 1;  // any match for THIS neighbor → keep going
	//         }
	//     }
	//   }
	//   if (v4) accept; else reject;
	//
	// Net rule: placement is valid iff EVERY occupied neighbor shares at least
	// one trait with the dropped snoid. Previously the C++ port returned `true`
	// on the first matching neighbor, which under-restricted the puzzle.
	_matchBitmask = 0;
	bool valid = true;

	for (int16 slot = 0; slot < 8 && valid; slot++) {
		byte neighborIdx = _adjacencyMatrix[seatIdx][slot];
		if (neighborIdx == 0)
			continue;

		uint16 occupantId = getDrawOnRegOccupant(neighborIdx - 1);
		if (occupantId == 0)
			continue;
		ZmbSnoid *neighborSnoid = getSnoid(occupantId);
		if (!neighborSnoid)
			continue;

		// IDA: v4 = 0 reset before inner trait scan — this occupied neighbor
		// must contribute at least one trait match or the placement fails.
		valid = false;

		const byte *droppedTraits = reinterpret_cast<const byte *>(&droppedSnoid->_trait);
		const byte *neighborTraits = reinterpret_cast<const byte *>(&neighborSnoid->_trait);
		for (int16 j = 0; j < 4; j++) {
			if (droppedTraits[j] == neighborTraits[j]) {
				_matchBitmask |= (1u << j);
				valid = true;
			}
		}
	}

	// Implicit: when no occupied neighbors exist, `valid` stays `true` from
	// the initial assignment — matches IDA `v4 = 1` initialization.
	return valid;
}

// ---------------------------------------------------------------------------
// findIdlePackSnoid: Find idle Zoombini from pack (IDs >= 10000).
// IDA: zmb_findIdleFeatureRunner (0x456A95)
// ---------------------------------------------------------------------------
ZmbSnoid *ZoombiniPuzzleFerry::findIdlePackSnoid(uint16 preferredId) {
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
void ZoombiniPuzzleFerry::startRejectWalk(int16 destination) {
	_rejectDestination = destination;

	if (_rejectSnoidId == 0)
		return;

	// IDA leaves `dword_4AB124` (= `_rejectWalkPos2`) carrying its previous
	// value across reject branches and only reassigns it in the raft case at
	// 0x41BB49 (`*(_DWORD *)&dword_4AB124 = dword_4AB114`). For every Ferry
	// reject the desired SCRS-end-anchor is the snoid's saved origin (its
	// pedestal or rowboat pickup tile), so default to that here and let the
	// raft branch keep its identical explicit assignment for clarity.
	_rejectWalkPos2 = _savedDragOrigin;

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

	// IDA `puzzleFerry_1705_1706_41BA30` @ 0x41BBB4-0x41BC2C (re-read after
	// confirming `dwSomeFrameVal` = boat runner idx via `ferry_funcInit`
	// 0x41A5CE-0x41A5DA where `word_4AB17A = dwSomeFrameVal`):
	//   v2 = runner_findByIndex(dwSomeFrameVal);     // the BOAT runner (captain)
	//   scrb_loadOnRunner(1, word_4AB19E, v2);       // load SCRB 1604/1605/1606/1607
	//   scrb_playFrameSounds(1, dwSomeFrameVal);     // flush SND queue on boat
	//   dword_4AB120 = 0;                            // clear path-target ptr
	//   v2->onHotspotShapeOrFrameFunc = tunnels_zmbApproachGateCallback;
	//   scrb_registerHotspotGroup(0, 0, 0, word_4AB17A, word_4AB148, v2->wFeatureRunnerIdx);
	// Note `word_4AB17A` was overwritten at 0x41BA54 from `word_4AB178` to
	// hold the SNOID runner idx for the duration of the flight; that lookup
	// is what `tunnels_zmbApproachGateCallback` cases 4/5 use to find the
	// snoid to load SCRS 998-1007 onto.
	ZmbSnoid *rejectedSnoid = nullptr;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *s = *it;
		if (s && s->getId() == _rejectSnoidId) {
			rejectedSnoid = s;
			break;
		}
	}
	if (rejectedSnoid) {
		// Reset transient flight state from any prior reject before re-arming.
		_rejectFlightLandingScrsActive = false;
		// IDA `ferry_startRejectFlight_41BA30` @ 0x41BB9D loads the controller
		// SCRB (1604/1605/1606/1607) onto the boat runner and at 0x41BBB7
		// installs `ferry_rejectFlightSCRBCallback_41B50B` on it. Per the SCRB
		// binary dumps:
		//   - 1604, 1606, 1607 emit only event 2 -> case 1 (snoid SCRS load,
		//     hide-on-complete). Snoid sails away invisible.
		//   - 1605 emits events 2 and 3 -> case 1 then case 2 (loads landing
		//     SCRS with pInitPos for the dock-exit slide-in).
		// ScummVM dispatches boat events through the page-level
		// `onFeatureAnimEvent`, gated by `_rejectFlightActive`, into
		// `processBoatFlightEvent`. The rejected snoid is tracked separately
		// in `_rejectingSnoid` because IDA reuses `word_4AB17A` for both the
		// boat runner idx (init time) and the snoid runner idx (here).
		_rejectingSnoid = rejectedSnoid;
		_rejectFlightActive = true;
	}

	// IDA loads the flight-controller SCRB (1604/1605/1606/1607) onto the
	// boat runner AFTER the reject-reaction SCRB (1815 / 1804-1814) has
	// finished animating. Queue it here so the next `_pendingFrogmanScrb`
	// tick picks it up — this re-uses the existing SCRB-load pipeline and
	// keeps the captain's animation tied to the flight-controller SCRB as
	// the original does, which is also what drives the SCRB's SND frame
	// events for any embedded flight-specific voice.
	_pendingFrogmanScrb = _rejectWalkScrb;

	_departAnimPending = true;
}

// ---------------------------------------------------------------------------
// handleRejectWalkSetup: Called from onEveryFrame when reject walk is pending.
// IDA: ferry_funcOnHover, word_4AB12A branch
// ---------------------------------------------------------------------------
void ZoombiniPuzzleFerry::handleRejectWalkSetup() {
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

	// IDA `puzzleFerry_1705_1706_41BA30` tail @ 0x41BC43: once the reject walk
	// is set up on the snoid runner, `setInteractionLock_460C54(0)` fires —
	// the interaction lock is released immediately so the player can drop
	// another zoombini while the rejected one is still flying home. The lock
	// only needs to hold from the drop decision until the rejection reaction
	// SCRB has queued and the reject-walk runner is configured. Without this
	// the lock stays true forever (there is no other path that clears it on
	// the reject branch — the `_interactionLocked = false` at onFeatureAnim
	// Event only fires for departure overlays, not rejection flights), so the
	// first mis-drop permanently freezes all further zoombini interaction.
	_interactionLocked = false;
}

// ---------------------------------------------------------------------------
// onEveryFrame: Per-frame tick for ferry puzzle.
// IDA: ferry_funcOnHover (0x41a9f6)
// ---------------------------------------------------------------------------
void ZoombiniPuzzleFerry::onEveryFrame() {
	if (!_isActive)
		return;

	// -----------------------------------------------------------------------
	// [0] Pending Go departure
	// IDA: word_4AB17C && !word_4AB12A && !word_4AB118
	// -----------------------------------------------------------------------
	if (_goButtonPressed && !_rejectWalkPending && !_interactionLocked) {
		debugC(1, kZmbDebugAnimation, "Ferry: Go departure triggered");
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
	// IDA: word_4AB128 branch.
	//
	// IDA dispatches the queued SCRB unconditionally each frame, but in the
	// original engine voice SFX from a previous SCRB are flushed in lock-step
	// with frame advancement - the audio mixer there had implicit "one voice
	// channel per runner" exclusivity. ScummVM's mixer treats every voice as
	// an independent SFX, so loading three captain SCRBs across three ticks
	// (move pickup -> reaction -> next pickup) stacks three overlapping
	// voices and visually snaps the captain mid-pose.
	//
	// Gate the dispatch on `_frogmanHotspotGroup == 0` (= captain idle, same
	// flag IDA's reject-walk and fidget branches already poll). The queued
	// SCRB sits in `_pendingFrogmanScrb` until the current animation ends
	// and `onFeatureAnimEvent` clears the busy flag. New user actions just
	// overwrite the queue slot - matching the IDA single-slot behavior - so
	// the most recent reaction wins, but every reaction plays through its
	// full SCRB cycle without overlap.
	// -----------------------------------------------------------------------
	if (_pendingFrogmanScrb != 0 && _frogmanHotspotGroup == 0) {
		debug("[FERRY] load frogman SCRB %u (prev hsGrp=%u flight=%d)",
		      _pendingFrogmanScrb, _frogmanHotspotGroup,
		      static_cast<int>(_rejectFlightActive));
		uint16 scrb = _pendingFrogmanScrb;
		_pendingFrogmanScrb = 0;

		if (_boatAnimFeature) {
			// IDA `ferry_funcOnHover` @ 0x41ABC0:
			//   scrb_initRunnerWithScript(0, 0, v1, dwSomeFrameVal);  // load SCRB onto boat runner
			//   scrb_playFrameSounds(1, dwSomeFrameVal);              // force-flush embedded SND events
			//   word_4AB19A = scrb_registerHotspotGroup(...);         // mark "frogman busy"
			// The SCRB's voice SFX is a frame-event SND reference; IDA enqueues
			// it immediately on load. The boat runner was registered with
			// FLAG_00080000_DEFER_ANIM so that it stays silent until a reaction
			// is requested; `loadScrbOntoFeature` alone replaces the data but
			// leaves the DEFER_ANIM / idle-animate state, so the first frame's
			// sound queue never fires. Activate animate + render explicitly so
			// the natural frame tick drains the SCRB's sound queue.
			loadScrbOntoFeature(_boatAnimFeature, scrb);
			_boatAnimFeature->activateAnimate();
			_boatAnimFeature->activateRender();
			// Mark frogman as mid-reaction — cleared on anim-end in
			// onFeatureAnimEvent. Reject-walk setup and fidget gating both
			// poll this flag so they can wait for the voice/animation to
			// finish before proceeding.
			_frogmanHotspotGroup = scrb;
		}
	}
	// -----------------------------------------------------------------------
	// [2] Departure animation pending (reject walk overlay)
	// IDA: word_4AB12C branch
	// -----------------------------------------------------------------------
	else if (_departAnimPending) {
		debugC(1, kZmbDebugAnimation, "Ferry: departure animation pending -> activating overlay");
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
		// IDA `ferry_funcOnHover` @ 0x41AD2F: `if (!hotspot_ownerRunnerArr
		// [word_4AB19A])` — wait until the frogman's hotspot group has
		// cleared (= reaction animation finished). The end-of-cycle fires
		// `kZmbAnimEventM1_End` on `_boatAnimFeature` which zeroes
		// `_frogmanHotspotGroup` in `onFeatureAnimEvent`. Polling
		// `isAnimateActivated()` here was the wrong gate: after the Voice
		// SFX fix activates animate unconditionally on SCRB load, that flag
		// stayed true forever and `handleRejectWalkSetup` was never called
		// — which in turn never cleared `_interactionLocked`.
		if (_frogmanHotspotGroup == 0) {
			handleRejectWalkSetup();
		}
	}
	// -----------------------------------------------------------------------
	// [4] Idle fidget timer
	// IDA: getElapsedFrameTime > dword_4AB10C branch
	// IDA `ferry_funcOnHover` gates every reaction/fidget branch on
	// `!hotspot_ownerRunnerArr[word_4AB19A]` — i.e. the frogman is idle.
	// `_frogmanHotspotGroup` mirrors that flag (set on SCRB load, cleared on
	// kZmbAnimEventM1_End). Without this gate the fidget timer kept
	// scheduling new SCRBs over an already-playing captain reaction, which
	// is what produced the overlapping voice SFX.
	// -----------------------------------------------------------------------
	else if (_frogmanHotspotGroup == 0 && _currentFrameTime > _nextFidgetTime) {
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
ZmbEventHandleResult ZoombiniPuzzleFerry::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
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
	if (_interactionLocked || _goButtonPressed) {
		debug("[FERRY] click blocked: interactionLocked=%d goButtonPressed=%d "
		      "rejectWalkPending=%d rejectingSnoid=%p flightActive=%d landingActive=%d",
		      static_cast<int>(_interactionLocked),
		      static_cast<int>(_goButtonPressed),
		      static_cast<int>(_rejectWalkPending),
		      static_cast<void *>(_rejectingSnoid),
		      static_cast<int>(_rejectFlightActive),
		      static_cast<int>(_rejectFlightLandingScrsActive));
		return ZmbEventHandleResult::kPassthrough;
	}
	if (isDragging())
		return ZmbEventHandleResult::kPassthrough;

	// Find snoid at click position
	ZmbSnoid *snoid = findSnoidAtPoint(absPos);
	if (!snoid) {
		debug("[FERRY] click at (%d,%d): no snoid found", absPos.x, absPos.y);
		return ZmbEventHandleResult::kPassthrough;
	}

	// Guard: can't drag during reject walk or departure
	if (_departAnimDone || _goButtonPressed)
		return ZmbEventHandleResult::kPassthrough;

	// IDA: save the pcStr1[11] (seated state) and reset it
	// IDA: dword_4AB114 = snoid->core188.posLoc
	_savedDragOrigin = snoid->getPointLoc();

	// Begin drag
	startSnoidDrag(snoid, absPos);

	// IDA `ferry_funcOnClick_41AE20` (0x41AE20) does NOT queue a captain
	// reaction on pickup. The move-reaction pool (`ferry_moveReactionScrbTable`
	// = SCRB 1820/1821/1822) only fires from the non-seat-drop branch at
	// 0x41B300-0x41B323 (`if (!word_4AB128 && click_testZoneRadius(posLoc))`).
	// Earlier ScummVM revisions queued it on every dock pickup, which produced
	// an extra captain voice before legitimate reactions (welcome, good, bad)
	// and made the first-snoid welcome (SCRB 1816) sound like the wrong voice
	// because a pickup grunt always preceded it. The non-seat drop path in
	// `endDrag` already queues the move SCRB for IDA-equivalent terrain drops.

	return ZmbEventHandleResult::kConsumed;
}

// ---------------------------------------------------------------------------
// onLButtonUp: Release drag.
// ---------------------------------------------------------------------------
ZmbEventHandleResult ZoombiniPuzzleFerry::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
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
void ZoombiniPuzzleFerry::endDrag(const Common::Point &mousePos) {
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
				_successThreshold += _vm->_rnd->getRandomNumber(3, 5);

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

			// IDA ferry_funcOnClick @ 0x41B1E9 calls `clearPendingRunnerSlot_
			// 45354C` which zeroes `scrb_drawOnRegFlagArr[word_4B7318]` —
			// i.e. undoes the optimistic seat occupancy written by
			// `beginDragFeatureRunner` when the cursor was released over a
			// seat. Without this the rejected snoid's target seat is marked
			// occupied by that snoid even though it flies back to its
			// pedestal, permanently blocking that seat.
			clearDrawOnRegOccupant(_dropTargetSeat);

			// Mark snoid for rejection
			snoid->_packIsOccupied = false;
			_rejectSnoidId = snoid->getId();

			// IDA: word_4AB148 = word_4AB14C[word_4AB148]
			// Store rejected seat for animation target

			// Select rejection reaction SCRB
			// IDA: if (nextRand(5,3) == word_4AB192) → 1815 (harsh), else random from bad pool
			if (_vm->_rnd->getRandomNumber(3, 5) == _consecutiveFailures) {
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

		// IDA: if !word_4AB128 && click_testZoneRadius(posLoc), additionally
		// gated on frogman-idle (`!hotspot_ownerRunnerArr[word_4AB19A]`) so
		// the dock-drop voice does not overlap a still-playing reaction.
		if (!_pendingFrogmanScrb && _frogmanHotspotGroup == 0 &&
		    kDockRect.contains(snoidPos.x, snoidPos.y)) {
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
// IDA: dispatched via per-runner hotspot/frame callbacks. Ferry installs
// `tunnels_zmbApproachGateCallback` on the boat runner during reject flight
// (see `puzzleFerry_1705_1706_41BA30` 0x41BBB7); ScummVM's page-level
// dispatcher routes those events into `processBoatFlightEvent`.
// ---------------------------------------------------------------------------
void ZoombiniPuzzleFerry::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (feature == _boatAnimFeature) {
		debug("[FERRY] boat event=%d hsGrp=%u flight=%d",
		      eventCode, _frogmanHotspotGroup, static_cast<int>(_rejectFlightActive));
		if (_rejectFlightActive && eventCode != kZmbAnimEventM1_End) {
			// Boat is currently running the reject-flight controller SCRB
			// (1604/1605/1606/1607). Route mid-frame event codes 1..6 into
			// the SCRB->SCRS hand-off (IDA tunnels_zmbApproachGateCallback).
			processBoatFlightEvent(eventCode);
			return;
		}
		// Frogman/boat animation completed
		if (eventCode == kZmbAnimEventM1_End) {
			// IDA: End of SCRB chain — frogman returns to idle
			_frogmanHotspotGroup = 0;
			// Reject-flight controller SCRB ran to completion. If a snoid is
			// still flagged as in-flight at this point and no landing SCRS is
			// playing, this is the rowboat/raft "true sail-away" case (case 2
			// never fired or the raft branch hid the snoid permanently). Clear
			// the in-flight pointer and release locks - otherwise the puzzle
			// stays frozen because no later event will fire to clean up.
			if (_rejectFlightActive && _rejectingSnoid &&
			    !_rejectFlightLandingScrsActive) {
				int16 dest = _rejectDestination;
				if (dest < 0 || 10 <= dest)
					dest = 0;
				if (kRejectFlightSnoidScrsB[dest] != 1907) {
					debug("[FERRY] controller SCRB done with no landing -> restore rejected snoid");
					_rejectingSnoid->setPointLoc(_rejectWalkDest);
					_rejectingSnoid->setFacingLeft(false);
					_rejectingSnoid->setAnimState(kSnoidAnimIdle);
					_rejectingSnoid->setupIdleHotspots();
					_rejectingSnoid->activateRender();
					_rejectingSnoid->setNeedsRedraw(true);
					_rejectingSnoid->_packIsOccupied = true;
				} else {
					debug("[FERRY] controller SCRB done with no landing -> raft sail-away cleanup");
				}
				_rejectingSnoid = nullptr;
				_interactionLocked = false;
				_rejectWalkPending = false;
			}
			_rejectFlightActive = false;
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
		// IDA installs the same `ferry_rejectFlightSCRBCallback_41B50B` on
		// the rejected snoid runner via `tunnels_playZmbScript` (case 1's
		// scriptData=callback arg, 0x41B566). The snoid's SCRS terminator
		// events (e.g. SCRS 1900/1902/1904/1906 ending with raw byte 3 ->
		// adjusted code 2) therefore feed into the same dispatcher. ScummVM's
		// page-level dispatch needs to forward those events so case 2 fires
		// and loads the landing SCRS for visible touchdown.
		if (snoid == _rejectingSnoid && eventCode >= 0 && eventCode <= 6) {
			processBoatFlightEvent(eventCode);
			return;
		}
		if (eventCode == kZmbAnimEventM1_End) {
			if (snoid == _rejectingSnoid) {
				if (_rejectFlightLandingScrsActive) {
					// Case 2 (non-raft) or case 3 finished: snoid is at the
					// landing destination via SCRS pInitPos alignment. Re-show
					// it (case 1's hide-on-complete may have deactivated render
					// before the landing SCRS started), settle its position,
					// and re-mark the pack slot so the player can drag it again.
					debug("[FERRY] landing SCRS done -> land at (%d,%d)",
					      _rejectWalkDest.x, _rejectWalkDest.y);
					snoid->setPointLoc(_rejectWalkDest);
					snoid->setFacingLeft(false);
					snoid->setAnimState(kSnoidAnimIdle);
					snoid->setupIdleHotspots();
					snoid->activateRender();
					snoid->_packIsOccupied = true;
					_rejectingSnoid = nullptr;
					_rejectFlightLandingScrsActive = false;
					// Defensive: ensure no stale lock blocks the player from
					// picking up a new snoid after a successful return-to-pedestal.
					_interactionLocked = false;
					_rejectWalkPending = false;
				} else {
					// Case 1's flight SCRS A finished with hide-on-complete
					// active. Three possibilities:
					//   (a) Rowboat / raft: case 2 was never going to fire
					//       (their SCRBs only emit case 1). Captain controller
					//       SCRB will run to its -1 and we finish the flight
					//       there.
					//   (b) Dock (SCRB 1605): case 2 fires LATER from the boat
					//       to load the visible-landing SCRS B. SCRS A is
					//       always shorter than the gap between case 1 and
					//       case 2 (SCRS 1902 has 7 frames vs ~16 boat frames),
					//       so snoid -1 fires while the controller still has
					//       case 2 pending.
					// In both cases keep `_rejectingSnoid` valid - just leave
					// the snoid hidden and let the boat-side handler decide
					// the final outcome. Clearing the pointer here would let
					// case 2 (b) early-return with no snoid to land, leaving
					// `_interactionLocked` stuck and the puzzle frozen.
					debug("[FERRY] flight SCRS A done, snoid hidden (waiting for boat)");
				}
			} else {
				snoid->setAnimState(kSnoidAnimIdle);
				snoid->setupIdleHotspots();
			}
		}
	}
}

// ---------------------------------------------------------------------------
// processBoatFlightEvent: SCRB->SCRS hand-off dispatcher fired by the boat
// runner while the reject-flight controller SCRB (1604/1605/1606/1607) is
// loaded.
//
// Verified via SCRB binary dumps under
// `AGENTS/Z1-RESOURCE/Z1-11K-DUMP/FERRY/SCRB/{1604..1607}.bin`:
//   SCRB 1604, 1606, 1607: emit raw byte 2 only       -> case 1
//   SCRB 1605            : emits raw bytes 2 and 3    -> case 1, then case 2
// (no SCRB in the Ferry pool emits raw bytes 5/6/7, so cases 4/5/6 in the
// IDA `ferry_rejectFlightSCRBCallback_41B50B` switch are caves-only branches
// and are not invoked here).
//
// IDA switch (0x41B51C):
//   case 1 @ 0x41B53F : caves_swapRunnerScrb() then tunnels_playZmbScript(
//                       1=hideOnComplete, callback, kRejectFlightSnoidScrsA[dest],
//                       endFrame). Loads SCRB 1700-1703 onto the boat (the
//                       actual rowboat/raft animation) and SCRS 1900-1906 onto
//                       the snoid (the snoid's flight visual; pool 0 -> NORMAL
//                       state 9). The snoid hides on completion.
//   case 2 @ 0x41B57F : if kRejectFlightSnoidScrsB[dest] == 1907 -> raft path:
//                         link runners, set ferry_departAnimPending = 1.
//                       else -> ferry_zmbScript_pInitPos = &ferry_rejectFlightDest
//                         then tunnels_playZmbScript(0=return-to-idle, NULL,
//                         kRejectFlightSnoidScrsB[dest], endFrame), then clear
//                         interaction lock. The pInitPos override aligns the
//                         SCRS end-anchor to the dock destination (122,164).
//   case 3 @ 0x41B5EE : ferry_zmbScript_pInitPos = &ferry_rejectFlightDest then
//                       tunnels_playZmbScript(1=hide, NULL,
//                       kRejectFlightSnoidScrsB[dest], endFrame). Same as
//                       case 2's else-branch but hides on complete.
//   case 6 @ 0x41B630 : if (!ferry_pendingFrogmanScrb) refill from fidget pool.
// ---------------------------------------------------------------------------
void ZoombiniPuzzleFerry::processBoatFlightEvent(int16 callbackCode) {
	int16 dest = _rejectDestination;
	if (dest < 0 || dest >= 10) {
		// IDA `ferry_startRejectFlight_41BA30` clamps dest to 0 when >= 10
		// (0x41BA67) before storing into `ferry_rejectDestination`. Mirror that
		// safety here so the lookup tables stay in bounds.
		dest = 0;
	}
	debug("[FERRY] flight event code=%d dest=%d snoid=%p foot=%u scrsA=%u scrsB=%u",
	      callbackCode, dest, static_cast<void *>(_rejectingSnoid),
	      _rejectingSnoid ? _rejectingSnoid->_trait._foot : 0,
	      kRejectFlightSnoidScrsA[dest], kRejectFlightSnoidScrsB[dest]);

	switch (callbackCode) {
	case 1: {
		// IDA case 1 @ 0x41B53F: load the snoid's primary flight SCRS.
		//
		// IDA also calls `caves_swapRunnerScrb` (0x41B463) which would load
		// `kRejectFlightBoatSwapScrb[dest]` (1700-1703) onto the runner indexed
		// by `word_4AB148`. In Ferry that global holds the dropped-on SEAT
		// INDEX (1-based, set in ferry_onClickHandler @ 0x41B020), NOT a
		// runner-table index. `runner_findByIndex(seat_idx)` therefore returns
		// either NULL or some unrelated low-ID runner (landscape, boat-approach
		// etc.), so `caves_swapRunnerScrb` is effectively a no-op here. The
		// boat keeps playing the controller SCRB 1604/1605/1606/1607, which
		// embeds the captain's throw animation, and the snoid's flight is
		// driven solely by the SCRS loaded below.
		if (!_rejectingSnoid)
			break;
		uint16 scrsA = kRejectFlightSnoidScrsA[dest];
		debug("[FERRY] case 1: load snoid SCRS %u (hide-on-complete)", scrsA);
		Common::SeekableReadStream *scrsStream = _vm->getResource(
			MKTAG('S', 'C', 'R', 'S'),
			ZmbResource(ZmbArchiveKind::kPage, scrsA));
		if (scrsStream) {
			// IDA `tunnels_playZmbScript(1, callback, scrsId, endFrame)`:
			//   - chIsFacingLeft=1 -> chRand_64_0=1 (hide on complete)
			//   - pInitPos=NULL  -> first-frame anchor (snoid starts at SCRS
			//                       init pos defined inside the SCRS itself,
			//                       e.g. init_x=16/init_y=235 for SCRS 1900)
			//   - SCRS 1900-1906 belong to pool 0 -> NORMAL state 9
			_rejectingSnoid->startScrsPlayback(scrsStream, true /* hideOnComplete */,
			                                   false /* rejectState=NORMAL */);
		} else {
			warning("Ferry: primary flight SCRS %u not found", scrsA);
		}
		break;
	}
	case 2: {
		// IDA case 2 @ 0x41B57F: raft vs landing-SCRS branch.
		uint16 scrsB = kRejectFlightSnoidScrsB[dest];
		debug("[FERRY] case 2: load snoid SCRS %u (return-to-idle, pInitPos=dest)",
		      scrsB);
		if (scrsB == 1907) {
			// IDA raft branch @ 0x41B581: runner_linkRelativeToParent(
			//   word_4AB144, 0, word_4AB17A); word_4AB12C = 1.
			// Activates the raft-departure overlay (the secondary 1705/1706
			// runners allocated by `startRejectWalk` for raft destinations)
			// instead of loading another snoid SCRS. The snoid stays hidden
			// from case 1.
			_departAnimPending = true;
			_interactionLocked = false;
			break;
		}
		if (!_rejectingSnoid)
			break;
		Common::SeekableReadStream *scrsStream = _vm->getResource(
			MKTAG('S', 'C', 'R', 'S'),
			ZmbResource(ZmbArchiveKind::kPage, scrsB));
		if (scrsStream) {
			// IDA `tunnels_playZmbScript(0, 0, scrsId, endFrame)` with
			// `ferry_zmbScript_pInitPos = &ferry_rejectFlightDest`:
			//   - chIsFacingLeft=0 -> chRand_64_0=0 (return-to-idle on complete)
			//   - pInitPos=&ferry_rejectFlightDest -> SCRS end-anchor aligned to
			//     the destination point (dock = 122,164; rowboat = saved origin).
			//   - SCRS 1901-1907 are in pool 0 -> NORMAL state 9.
			_rejectingSnoid->startScrsPlayback(scrsStream, false /* hideOnComplete */,
			                                   false /* rejectState=NORMAL */,
			                                   &_rejectWalkDest);
			_rejectingSnoid->activateRender();  // case 1 may have hidden the snoid
			_rejectFlightLandingScrsActive = true;
		} else {
			warning("Ferry: secondary flight SCRS %u not found", scrsB);
		}
		// IDA 0x41B5DB: word_4AB118 = 0 (clear interaction lock).
		_interactionLocked = false;
		break;
	}
	case 3: {
		// IDA case 3 @ 0x41B5EE: same SCRS pool as case 2 but hide on complete.
		uint16 scrsB = kRejectFlightSnoidScrsB[dest];
		debug("[FERRY] case 3: load snoid SCRS %u (hide, pInitPos=dest)", scrsB);
		if (!_rejectingSnoid || scrsB == 1907)
			break;
		Common::SeekableReadStream *scrsStream = _vm->getResource(
			MKTAG('S', 'C', 'R', 'S'),
			ZmbResource(ZmbArchiveKind::kPage, scrsB));
		if (scrsStream) {
			_rejectingSnoid->startScrsPlayback(scrsStream, true /* hideOnComplete */,
			                                   false /* rejectState=NORMAL */,
			                                   &_rejectWalkDest);
			_rejectingSnoid->activateRender();  // ensure visible during landing arc
			// Case 3 hides on complete (matches its IDA chIsFacingLeft=1 arg)
			// so don't set the landing flag — the snoid finishes hidden, same
			// as case 1.
		} else {
			warning("Ferry: tertiary flight SCRS %u not found", scrsB);
		}
		break;
	}
	case 6: {
		// IDA case 6 @ 0x41B630: top up the captain fidget pool when empty.
		if (_pendingFrogmanScrb == 0) {
			uint16 idx = _vm->_rnd->getNonRepeatRandom(5, _fidgetRandomState);
			_pendingFrogmanScrb = kFidgetScrbPool[idx];
		}
		break;
	}
	default:
		// Cases 4 and 5 are caves-entrance-only (takeoff/landing of the gate
		// approach SCRS 998-1009 used by Caves Stone Cold) and are not emitted
		// by Ferry SCRBs 1604/1605/1606/1607.
		break;
	}
}

} // End of namespace Mohawk
