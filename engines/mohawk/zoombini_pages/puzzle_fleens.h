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

#ifndef MOHAWK_ZOOMBINI_PAGES_PUZZLE_FLEENS_H
#define MOHAWK_ZOOMBINI_PAGES_PUZZLE_FLEENS_H

#include "mohawk/zoombini_pages/puzzle_base.h"

namespace Mohawk {

/**
 * Fleens puzzle page (ZoombiniPageType::kFleens).
 * Route 3, Puzzle 1
 *
 * Fleens is a Zoombini-like creature that its traits are linked with the Zoombini's.
 * Dropping a Zoombini to the predefined seat triggers Fleens jump animation.
 * Player must lure the randomly selected three fleens which seating on the beehive.
 * If the player correctly identifies the three key Zoombini, angry bee animation are played and fleens flee.
 *
 * IDA entry: fleens_initAndSetupPuzzle (0x41C1E0)
 */
class ZoombiniPuzzleFleens : public ZoombiniPuzzle {
public:
	ZoombiniPuzzleFleens(MohawkEngine_Zoombini *vm);
	~ZoombiniPuzzleFleens() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onEveryFrame() override;
	void onGoButtonActivated() override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;
	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onMouseMove(const Common::Point &absPos, const Common::Point &relPos) override;

private:
	void loadZoombinisFromPack();

	/**
	 * Build trait transformation data for Zoombinis.
	 * IDA: ferry_buildZmbRunners_41D9F4
	 * Selects mismatch zoombinis and generates trait transformation offsets.
	 */
	void buildZmbTraitSetup();

	/**
	 * Map an event type and snoid foot trait to a SCRS resource ID.
	 * IDA: fleens_mapEventToScrsId (0x41E860)
	 */
	uint16 mapEventToScrsId(int16 eventType, const ZmbSnoid *snoid) const;

	/**
	 * Process the departure queue: each queued snoid plays its exit SCRS.
	 * IDA: fleens_processRaftDeparture (0x41EAF3)
	 */
	void processRaftDeparture();

	/**
	 * Start the initial raft arrival animation.
	 * IDA: fleens_initAndSetupPuzzle tail (0x41C4CA-0x41C52C).
	 */
	void startInitialRaftAnim();

	/**
	 * Start a boarding animation for the pending snoid.
	 * IDA: fleens_onHoverPerFrame boarding block (0x41C9xx).
	 */
	void startBoardingAnimation();

	/**
	 * Handle the raft exit completion callback.
	 * IDA: fleens_onRaftExitComplete (0x41ED04)
	 */
	void onRaftExitComplete();

	/**
	 * Check if a snoid is a "mismatch" (will be captured by Fleens).
	 * Uses the mod-5 trait transformation from buildZmbTraitSetup.
	 */
	bool isMismatchSnoid(uint16 snoidIdx) const;

	/**
	 * Find the next available seat on the raft.
	 * Returns seat index (0-15), or -1 if no seat available.
	 */
	int16 findAvailableRaftSeat() const;

	/** End a drag and process the drop target. */
	void endDrag(const Common::Point &mousePos);

	// Attribute slot render callbacks
	bool attrSlots_preRender(ZmbFeature *feature);
	ZmbRenderResult attrSlots_render(ZmbFeature *feature);

	// --- Constants ---
	static const Common::Point kSnoidPositions[16];
	static const Common::Point kRaftPosition;

	/** Puzzle difficulty level (1-4, 1-based). IDA: fleens_routeLevel */
	ZmbPuzzleDifficultyLevel _difficultyLevel = kPuzzleDiffLevel1;

	// === Puzzle state (IDA: fleens_clearAllPuzzleState @ 0x41C0B4) ===

	/** Whether the puzzle is actively running. IDA: fleens_bPuzzleActive (0x4AB200) */
	bool _bPuzzleActive = false;

	/** Whether the raft is ready to depart. IDA: fleens_bRaftReady (0x4AB202) */
	bool _bRaftReady = false;

	/** Whether player interaction is allowed. IDA: fleens_bInteractionAllowed (0x4AB204) */
	bool _bInteractionAllowed = false;

	/** Whether a raft boarding animation is currently playing. IDA: fleens_bRaftAnimPlaying (0x4AB192) */
	bool _bRaftAnimPlaying = false;

	/** Whether a zoombini is currently boarding the raft. IDA: fleens_bBoardingInProgress (0x4AB1F2) */
	bool _bBoardingInProgress = false;

	/** Whether auxiliary runner has been linked. IDA: word_4AB1B6 */
	bool _bAuxLinked = false;

	/** Whether overlay link is pending. IDA: word_4AB1CC */
	bool _bOverlayLinkPending = false;

	/** Number of Zoombinis that don't match. IDA: fleens_mismatchCount (0x4AB1A6) */
	int16 _mismatchCount = 0;

	/** Indices of mismatched Zoombinis (1-based). IDA: word_4AB1BC-C0 */
	int16 _mismatchIdx[3] = {0, 0, 0};

	/** Trait transformation offsets per attribute type (mod-5). */
	uint8 _traitOffsets[4] = {0, 0, 0, 0};

	/** Trait slot order indices (for higher difficulty). */
	uint8 _traitSlotOrder[4] = {0, 0, 0, 0};

	// --- Raft seat tracking ---

	/** Seat occupied flags (16 seats). IDA: byte_4AB24A[16] */
	bool _seatOccupied[16] = {};

	/** Snoid IDs occupying each seat. IDA: word_4AB22A[16] */
	uint16 _seatSnoidId[16] = {};

	// --- Departure queue ---

	/** Departure queue of snoid IDs. IDA: word_4AB1D2[7] */
	uint16 _departQueue[7] = {};

	/** Fleen runner queue. IDA: word_4AB1E0[7] */
	uint16 _departFleenQueue[7] = {};

	/** Departure queue count (0-7). IDA: fleens_departureQueueCount (0x4AB1CE) */
	int16 _departQueueCount = 0;

	// --- Boarding state ---

	/** Pending raft boarding snoid ID. IDA: word_4AB1F8 */
	uint16 _pendingBoardSnoidId = 0;

	/** Foot trait of snoid being boarded. IDA: word_4AB1F4 */
	uint16 _boardingSnoidFoot = 0;

	/** Active raft animation snoid. IDA: word_4AB1BA */
	uint16 _activeRaftAnimSnoidId = 0;

	/** Active raft snoid runner. IDA: word_4AB1B8 */
	uint16 _activeRaftSnoidRunner = 0;

	/** Overlay runner index. IDA: word_4AB1AC */
	uint16 _overlayRunnerIdx = 0;

	/** Auxiliary runner index. IDA: word_4AB1A8 */
	uint16 _auxRunnerIdx = 0;

	/** Exit runner index. IDA: word_4AB1AE */
	uint16 _exitRunnerIdx = 0;

	/** Pending exit runner. IDA: word_4AB1F0 */
	uint16 _pendingExitRunner = 0;

	/** Saved exit runner. IDA: word_4AB1EE */
	uint16 _savedExitRunner = 0;

	/** Capture phase runner. IDA: word_4AB1CA */
	uint16 _capturePhaseRunner = 0;

	// --- Script completion flags ---

	/** Raft departure pending. IDA: word_4AB1C8 */
	bool _bRaftDepartPending = false;

	/** Script handler D completed. IDA: word_4AB1C4 */
	bool _bScriptDComplete = false;

	/** Script handler E completed. IDA: word_4AB1C2 */
	bool _bScriptEComplete = false;

	/** Script handler A completed. IDA: word_4AB1C6 */
	bool _bScriptAComplete = false;

	/** Pending transition target page. IDA: puzzle_pendingTransitionTarget */
	int16 _pendingTransitionTarget = 0;

	/** Total zoombini count on page. IDA: fleens_totalZmbCount */
	int16 _totalZmbCount = 0;

	/** Deferred SCRS loading countdown. IDA: fleens_deferredScrsCountdown */
	int16 _deferredScrsCountdown = 0;

	// --- Dirty flags for attr slot rendering ---
	bool _raftButtonDirty = false;
	bool _attrSlot1Dirty = false;
	bool _attrSlot2Dirty = false;

	// --- Puzzle-specific feature runners ---
	ZmbFeature *_animFeature = nullptr;
	ZmbFeature *_raftFeature = nullptr;
	ZmbFeature *_overlayFeatures[7] = {};

	// --- Animation event state ---
	int16 _pendingBodyArrangement = 0;

	// --- Idle animation state ---
	int16 _idleAnimCount = 0;
	int16 _idleAnimTarget = 0;
	uint32 _idleAnimLastFrame = 0;
	uint32 _idleAnimInterval = 0;
	uint32 _idleAnimPoolState = 0;

	/** Idle animation delay counter. IDA: word_4A4764 */
	int16 _idleAnimDelayCounter = 64;

	/** Number of loaded Zoombinis from active pack. */
	int16 _loadedZmbCount = 0;

	/** Saved drag origin. */
	Common::Point _savedDragOrigin;
};

} // End of namespace Mohawk

#endif
