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

#ifndef MOHAWK_ZOOMBINI_PAGES_PUZZLE_FERRY_H
#define MOHAWK_ZOOMBINI_PAGES_PUZZLE_FERRY_H

#include "mohawk/zoombini_pages/puzzle_base.h"

namespace Mohawk {

/**
 * Ferry puzzle page (ZoombiniPageType::kFerry).
 * Route 2, Puzzle 1
 *
 * Zoombinis must board boats by matching attributes.
 * Adjacent seated Zoombinis must share at least one trait.
 * The captain (frogman) enforces seating rules with reaction animations.
 *
 * IDA entry: ferry_funcInit (0x41a394)
 */
class ZoombiniPuzzleFerry : public ZoombiniPuzzle {
public:
	ZoombiniPuzzleFerry(MohawkEngine_Zoombini *vm);
	~ZoombiniPuzzleFerry() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

	void onEveryFrame() override;
	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;

protected:
	void onGoButtonActivated() override;

private:
	void loadZoombinisFromPack();
	void loadSeatLayout();
	void buildAdjacencyMatrix();
	void endDrag(const Common::Point &mousePos);
	int16 getDropTargetSeat(const Common::Point &pos) const;
	bool testAdjacentMatch(int16 seatIdx, ZmbSnoid *droppedSnoid);
	ZmbSnoid *findIdlePackSnoid(uint16 preferredId = 0);
	void startRejectWalk(int16 destination);
	void handleRejectWalkSetup();

	/**
	 * SCRB->SCRS hand-off dispatcher fired by `_boatAnimFeature`'s SCRB frame events
	 * while the reject-flight controller SCRB (1604/1605/1606/1607) is loaded.
	 * Mirrors the IDA `tunnels_zmbApproachGateCallback_41B50B` switch: case 4 starts
	 * the takeoff SCRS on the rejected snoid, case 5 starts the landing SCRS aligned
	 * to the destination, case 6 queues the next captain fidget if idle. Cases 1/2/3
	 * (caves runner SCRB swap + zmb script play) are not yet ported because they only
	 * affect the alternate caves entrance flow that Ferry does not invoke.
	 */
	void processBoatFlightEvent(int16 callbackCode);

	static const Common::Point kSnoidPositions[20];
	static const Common::Rect kDockRect;

	// Frogman SCRB animation pools
	// IDA: word_4A0CFC (boat approach), word_4A0D08 (fidget), word_4A0D18 (good),
	//      word_4A0D20 (bad), word_4A0D4C (move)
	static const uint16 kBoatScrbPool[4];
	static const uint16 kFidgetScrbPool[5];
	static const uint16 kGoodReactionPool[2];
	static const uint16 kBadReactionPool[11];
	static const uint16 kMoveReactionPool[3];

	/** Puzzle difficulty level (1-4, 1-based). IDA: word_4AB112 */
	ZmbPuzzleDifficultyLevel _difficultyLevel = kPuzzleDiffLevel1;

	/** Selected SCRB ID for seating layout. IDA: ferry_selectSCRB result */
	uint16 _seatingSCRB = 0;

	/** Frame processing guard. IDA: word_4AB134 */
	bool _isActive = false;

	/** Seated count (enables Go button when > 0). IDA: word_4AB136 */
	int16 _seatedCount = 0;

	/** Interaction locked during reject animation. IDA: word_4AB118 */
	bool _interactionLocked = false;

	/** Pending frogman SCRB to play. IDA: word_4AB128 */
	uint16 _pendingFrogmanScrb = 0;

	/** Reject walk pending. IDA: word_4AB12A */
	bool _rejectWalkPending = false;

	/** Departure overlay animation pending. IDA: word_4AB12C */
	bool _departAnimPending = false;

	/** Departure hotspot group completed flag. IDA: word_4AB12E */
	bool _departAnimDone = false;

	/** Current drop target seat (0-based index, -1=none). IDA: word_4AB148 */
	int16 _dropTargetSeat = -1;

	/** Reject walk destination index. IDA: word_4AB176 */
	int16 _rejectDestination = 0;

	/** Runner ID of rejected snoid. IDA: word_4AB178 */
	uint16 _rejectSnoidId = 0;

	/**
	 * Pointer to the snoid currently being thrown by the captain. IDA reuses
	 * `word_4AB17A` (boat runner idx) for this purpose: `puzzleFerry_1705_1706_41BA30`
	 * overwrites it with `word_4AB178` (the rejected snoid runner) just before loading
	 * the reject-flight controller SCRB onto the boat. We keep the boat runner
	 * pointer pristine in `_boatAnimFeature` and store the snoid here separately.
	 */
	ZmbSnoid *_rejectingSnoid = nullptr;

	/**
	 * True while a reject-flight controller SCRB (1604/1605/1606/1607) is loaded on
	 * the boat runner. Gates the SCRB->SCRS hand-off dispatch in `onFeatureAnimEvent`
	 * so cases 4/5/6 only fire while the flight is in progress and not for ordinary
	 * captain reaction/fidget SCRBs (which use the same boat runner). Cleared by
	 * `processBoatFlightEvent` case 5 (matching IDA `word_4AB17A = 0; word_4AB118 = 0`
	 * at 0x41B716/0x41B71F) or by the boat -1 end-of-animation event.
	 */
	bool _rejectFlightActive = false;

	/**
	 * True after the flight controller fires case 2 (else-branch) or case 3,
	 * meaning the snoid is now playing the secondary "land at destination"
	 * SCRS (1901-1905, indexed by destination through `kRejectFlightSnoidScrsB`)
	 * with `pInitPos=&_rejectWalkDest`. When the snoid's -1 fires we re-show
	 * the snoid at the landing point and re-mark its pack slot. Without this
	 * flag the rowboat/raft paths (case 1 only, hide-on-complete) would also
	 * be re-shown and the rejected snoid would never sail away as IDA intends.
	 */
	bool _rejectFlightLandingScrsActive = false;

	/** Go departure triggered. IDA: word_4AB17C */
	bool _goButtonPressed = false;

	/** Adjacency matrix: 20 seats × 8 neighbors (1-based IDs). IDA: dword_4AB180 */
	byte _adjacencyMatrix[20][8] = {};

	/**
	 * Lazy-build flag for `_adjacencyMatrix`. IDA calls `gfx_renderFrame`
	 * before `ferry_drawAdjacencyLines` so seat click rects are populated by
	 * runner_preRenderStandard. In ScummVM seat rects are only set after the
	 * first render pass, so we defer the build to the first drop test.
	 */
	bool _adjacencyBuilt = false;

	/** Visit counter. IDA: word_4AB184 */
	int16 _visitCounter = 0;

	/** Trait match bitmask for last drop. IDA: word_4AB18C */
	uint16 _matchBitmask = 0;

	/** Snoid to show attribute match on. IDA: word_4AB18E */
	uint16 _attrDisplaySnoid = 0;

	/** Consecutive correct placements. IDA: word_4AB190 */
	int16 _consecutiveSuccesses = 0;

	/** Consecutive wrong placements. IDA: word_4AB192 */
	int16 _consecutiveFailures = 0;

	/** Success count to trigger good reaction. IDA: word_4AB194 */
	int16 _successThreshold = 1;

	/** Total Zoombini count for completion check. IDA: word_4AB196 */
	int16 _totalZmbCount = 0;

	/** Whether first good reaction has been used. IDA: word_4AB198 */
	bool _hasReactedOnce = false;

	/** Frogman animation hotspot group. IDA: word_4AB19A */
	uint16 _frogmanHotspotGroup = 0;

	/** Reject walk SCRB to load. IDA: word_4AB19E */
	uint16 _rejectWalkScrb = 0;

	/** Saved reject walk destination position. IDA: dword_4AB11A */
	Common::Point _rejectWalkDest;

	/** Secondary reject walk position. IDA: dword_4AB124 */
	Common::Point _rejectWalkPos2;

	/** Saved drag origin position. IDA: dword_4AB114 */
	Common::Point _savedDragOrigin;

	/** Timer for next idle fidget. IDA: dword_4AB10C */
	uint32 _nextFidgetTime = 0;

	/** Non-repeat random pool state for fidget. IDA: dword_4A0D14 */
	uint32 _fidgetRandomState = 0;

	/** Non-repeat random pool state for boat SCRB. IDA: asc_4A0CFE+6 */
	uint32 _boatRandomState = 0;

	/** Non-repeat random pool state for good reaction. IDA: dword_4A0D1C */
	uint32 _goodReactionRandomState = 0;

	/** Non-repeat random pool state for bad reaction. IDA: dword_4A0D38 */
	uint32 _badReactionRandomState = 0;

	/** Non-repeat random pool state for move reaction. IDA: dword_4A0D54 */
	uint32 _moveReactionRandomState = 0;

	/** Non-repeat random pool state for reject walk. IDA: dword_4AB188 */
	uint32 _rejectWalkRandomState = 0;

	/** Frogman ambient sound started. IDA: word_4AB138 */
	bool _ambientStarted = false;

	// Puzzle-specific feature runners
	/** Landscape overlay runner (SCRB 1601). IDA: word_4AB13A */
	ZmbFeature *_landscapeFeature = nullptr;
	/** Boat animation runner (SCRB varies, default 1803). IDA: word_4AB17A */
	ZmbFeature *_boatAnimFeature = nullptr;
	/** Boat approach runner A (SCRB 1602). IDA: word_4AB13E — only if more-action enabled */
	ZmbFeature *_boatApproachA = nullptr;
	/** Boat approach runner B (SCRB 1603). IDA: word_4AB140 — only if more-action enabled */
	ZmbFeature *_boatApproachB = nullptr;
	/** Departure overlay runner (SCRB 1704). IDA: word_4AB142 */
	ZmbFeature *_departOverlayFeature = nullptr;
	/** Secondary departure runner (SCRB 1705). IDA: word_4AB144 */
	ZmbFeature *_departRunnerA = nullptr;
	/** Raft departure runner (SCRB 1706). IDA: word_4AB146 */
	ZmbFeature *_departRunnerB = nullptr;
	/** 3 overlay SCRBs (1450-1452). IDA: word_4AB14C[] */
	ZmbFeature *_overlayFeatures[3] = {};
};

} // End of namespace Mohawk

#endif
