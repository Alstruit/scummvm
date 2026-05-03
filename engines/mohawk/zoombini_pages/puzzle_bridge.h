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

#ifndef MOHAWK_ZOOMBINI_PAGES_PUZZLE_BRIDGE_H
#define MOHAWK_ZOOMBINI_PAGES_PUZZLE_BRIDGE_H

#include "mohawk/zoombini_pages/puzzle_base.h"

namespace Mohawk {

/**
 * Bridge puzzle page (ZoombiniPageType::kBridge).
 * Route 1, Puzzle 1
 *
 * The bridge has two lanes. A troll stands at one end and demands Zoombinis
 * with certain attribute(s) to cross. The player must drag each Zoombini
 * to the correct lane.
 *
 * IDA entry: puzzleBridge_414D6E
 */
class ZoombiniPuzzleBridge : public ZoombiniPuzzle {
public:
	ZoombiniPuzzleBridge(MohawkEngine_Zoombini *vm);
	~ZoombiniPuzzleBridge() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;
	void onEveryFrame() override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;

	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) override;
	void endDrag(const Common::Point &dropPos);

protected:
	void onGoButtonActivated() override;
	void debugPrepareForDeparture() override;
	Common::String debugGetAnswer() const override;

	/**
	 * Build the attribute toll table and select the winning combination.
	 * IDA: bridge_buildAttrTollTable_4160EF
	 *
	 * Fills _reqAttrTypes/_reqAttrValues with the required attribute(s)
	 * for crossing. Sets _puzzleReady = true when done.
	 */
	void buildAttrTollTable();

	/**
	 * Test whether a Zoombini's traits match the bridge toll rule.
	 * IDA: bridge_testAttrMatchRule_4168E9
	 *
	 * @param trait      The Zoombini's traits.
	 * @param targetSlot 1 = match lane (returns true if ANY attribute matches),
	 *                   2 = reject lane (returns true if NONE match).
	 * @return true if the Zoombini belongs on the given lane.
	 */
	bool testAttrMatch(const ZmbTrait &trait, int16 targetSlot) const;

	/**
	 * Collect attribute nibble-packed DWORDs from all active pack Zoombinis.
	 * IDA: collectZmbAttrBytes_4552FE
	 * @param outTraits  Output array of packed DWORDs (foot|nose|eye|head nibbles).
	 * @return Number of entries written.
	 */
	int16 collectZmbAttrPacked(Common::Array<uint32> &outTraits) const;

	/**
	 * Load Zoombinis from the active pack into snoid slots at predefined positions.
	 * IDA: setPosToZmbFeatureRunners_45F8DC / loadZoombiniAnimations_4528A6
	 */
	void loadZoombinisFromPack();

	// Button rendering callback
	// IDA: bridge_buttonDraw_415122
	void bridgeButtons_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	ZmbEventHandleResult bridgeButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);

	// Bridge visual rendering callback
	// IDA: bridge_invalidateVisualRects_415204 / bridge_drawAllButtons_4151DC
	ZmbRenderResult bridgeVisuals_render(ZmbFeature *feature);
	void bridgeVisuals_postRender(ZmbFeature *feature);

	/**
	 * Reload SCRB animation data on an existing feature.
	 * IDA: loadSCRB_460384(1, newScrbId, featureRunner)
	 */
	void reloadScrbAnimation(uint16 featureId, uint16 newScrbId);

	/**
	 * Process lane step events from crossing snoid SCRS playback.
	 * IDA: bridge_zmbLaneStepCallback_415D30
	 */
	void processLaneStepEvent(ZmbFeature *snoidFeature, int16 stepCode);

	/**
	 * Process troll entrance events from SCRB animation playback.
	 * IDA: bridge_onEntranceCallback_415C34
	 */
	void processEntranceEvent(int16 eventId, ZmbFeature *eventSource);

	/**
	 * Find an idle pack snoid, optionally preferring a specific ID.
	 * IDA: findIdleFeatureRunner_456A95
	 */
	ZmbSnoid *findIdlePackSnoid(uint16 preferredId = 0);

	/**
	 * Determine which lane a drop position maps to.
	 * @return 1 = lane 1 (top), 2 = lane 2 (bottom), 0 = no valid drop.
	 */
	int16 getDropTargetLane(const Common::Point &pos) const;

	/**
	 * Find a snoid whose draw record contains the given point.
	 * Skips template snoids with ID < 10000.
	 * @return The snoid, or nullptr if no snoid at that point.
	 */
	ZmbSnoid *findSnoidAtPoint(const Common::Point &pos) override;

	/**
	 * Return the bridge-specific drag constraint rect (left bank area).
	 */
	const Common::Rect &getDragConstraintRect() const override;

	enum PageResourceId : uint16 {
		// Background
		kResBackground1000 = 1000,

		// Shape bitmaps (tBMP for SHPL)
		kResBitmapShape1100 = 1100,
		kResBitmapShape1200 = 1200,
		kResBitmapShape1300 = 1300,
		kResBitmapTerrain1600 = 1600,

		// SCRB features - overlays
		kResScrb1100_Main = 1100,
		kResScrb1101_Overlay = 1101,
		kResScrb1102_Overlay = 1102,
		kResScrb1103_Overlay = 1103,  // special: water overlay
		kResScrb1104_Overlay = 1104,
		kResScrb1105_Overlay = 1105,  // troll gate

		// SCRB features - SHPL (shapes loaded separately)
		kResScrb1106_Water = 1106,    // 0x452

		// SCRB features - bridge segments
		kResScrb1300_Segment0 = 1300,
		kResScrb1301_Segment1 = 1301,

		// SCRB features - troll/gate animations
		kResScrb1200_TrollLane1 = 1200,  // 0x4B0
		kResScrb1201_TrollLane2 = 1201,  // 0x4B1
		kResScrb1202_TrollGate = 1202,   // 0x4B2

		// SCRS snoid scripts - reject pool
		kResScrs1000_RejectBase = 1000,
		kBridgeRejectScrsCount = 20,

		// SCRS snoid scripts - normal pool
		kResScrs2000_NormalBase = 2000,
		kBridgeNormalScrsCount = 5,

		// Sound resources
		kResSound997_MoveSFX = 997,
		kResSound996_ButtonSFX = 996,
		kResSound999_ClickSFX = 999,
		kResSoundBGM29999 = 29999,
		kResSoundBGM20000 = 20000,
	};

	// Snoid position table for 16 Zoombinis on the left bank.
	// IDA: unk_4A07B0 (16 POINTS as x,y int16 pairs)
	static const Common::Point kSnoidPositions[16];

	// Bridge segment feature positions (2 entries).
	// IDA: dword_4A07F0 / dword_4A07F4
	static const Common::Point kSegmentPositions[2];

	// Lane 1 (top) arrival positions for Zoombinis (16 entries).
	// IDA: unk_4A0718
	static const Common::Point kLane1Positions[16];

	// Lane 2 (bottom) arrival positions for Zoombinis (16 entries).
	// IDA: unk_4A0758
	static const Common::Point kLane2Positions[16];

	// Constraint rect for Zoombini drag (left bank area).
	// IDA: unk_4A07A8
	static const Common::Rect kDragConstraint;

	// Drop zone radius for bridge segment hit-test. IDA: wClickZoneRadius_4B6D3E = 55
	static const int16 kDropZoneRadius = 55;

	// --- Puzzle State ---

	/** Route difficulty level (1-4). IDA: word_4AAE18 */
	ZmbPuzzleDifficultyLevel _difficultyLevel = kPuzzleDiffLevel1;

	/** True once the toll table has been built. IDA: bridge_puzzleReady (0x4AAE8C) */
	bool _puzzleReady = false;

	/** Number of required attributes (1 for level 0, 2 for level 1, etc.). IDA: byte_4AAE90 */
	uint8 _reqAttrCount = 0;

	/** Required attribute types (1=hair,2=eyes,3=nose,4=legs). IDA: bridge_reqAttrTypes (0x4AAE91) */
	uint8 _reqAttrTypes[5] = {};

	/** Required attribute values (1-5). IDA: bridge_reqAttrValues (0x4AAE96) */
	uint8 _reqAttrValues[5] = {};

	/** For level 1: second attribute type. IDA: byte_4AAE92 */
	uint8 _reqSecondAttrType = 0;

	/** For level 1: second attribute value. IDA: byte_4AAE97 */
	uint8 _reqSecondAttrValue = 0;

	/** Random lane swap flag (0 or 1). IDA: bridge_bRandomLaneSwap (word_4AAE8E) */
	int16 _bRandomLaneSwap = 0;

	/** Whether any Zoombini has successfully crossed (enables Go button). IDA: word_4AAE12 */
	int16 _anyZmbCrossed = 0;

	/** Page is initialized and running. IDA: word_4AAE10 */
	int16 _isActive = 0;

	/** Number of successfully crossed Zoombinis (0-6 max). IDA: word_4AAE62 */
	int16 _successCount = 0;

	/** Number of Zoombinis currently on the bridge (in transit). IDA: word_4AAE76 */
	int16 _bridgeTransitCount = 0;

	/** Whether a reject script is playing. IDA: word_4AAE72 */
	int16 _isRejectPlaying = 0;

	/** Whether the current crossing result was a match. IDA: word_4AAE70 */
	int16 _currentMatchResult = 0;

	/** Current drop target lane (1 or 2). IDA: word_4AAE74 */
	int16 _currentDropLane = 0;

	/** Drag trail length (0-2). IDA: word_4AAE88 */
	int16 _trailLength = 0;

	/** Drag trail drop zone IDs. IDA: word_4AAE7C[2] */
	int16 _trailDropZone[2] = {};

	/** Drag trail runner IDs. IDA: word_4AAE80[2] */
	int16 _trailRunnerIdx[2] = {};

	/** Drag trail match results. IDA: word_4AAE84[2] */
	int16 _trailMatchResult[2] = {};

	/** Lane 1 (top) Zoombini runner IDs. IDA: word_4AAE1A[16] */
	int16 _lane1ZmbIds[16] = {};

	/** Lane 2 (bottom) Zoombini runner IDs. IDA: word_4AAE3A[16] */
	int16 _lane2ZmbIds[16] = {};

	/** Lane 1 fill count. IDA: word_4AAE14 */
	int16 _lane1Count = 0;

	/** Lane 2 fill count. IDA: word_4AAE16 */
	int16 _lane2Count = 0;

	/** Whether a Zoombini is currently being dragged. IDA: word_4AAE60 */
	int16 _isDragging = 0;

	/** Active lane indicator (-1=none). IDA: word_4AAEB0 */
	int16 _activeLaneScrb = -1;

	/** Active reject lane indicator (-1=none). IDA: word_4AAEAE */
	int16 _activeRejectScrb = -1;

	/** Troll attribute display state. IDA: word_4AAE8A */
	int16 _trollAttrState = 0;

	/** Snoid hotspot group index for bridge crossing. IDA: word_4AAE78 */
	int16 _crossingHotspotIdx = 0;

	/** Snoid script event from bridge animation. IDA: word_4AAE7A */
	int16 _pendingLaneEvent = 0;

	/** New arrival flag / retry allowed. IDA: word_4AAEAC (bridge_bRetryAllowed) */
	int16 _bRetryAllowed = 0;

	/** Whether a new troll anim needs rendering. IDA: word_4AAE6E */
	int16 _trollAnimPending = 0;

	/** Total loaded Zoombini count. IDA: word_4AAEB6 */
	int16 _totalZmbCount = 0;

	/** Celebration animation schedule count. IDA: word_4AAEB2 (bridge_celebrationCounter) */
	int16 _celebrationTarget = 0;

	/** Celebration animation played count. IDA: word_4AAEB4 (bridge_celebrationPlayed) */
	int16 _celebrationsPlayed = 0;

	/** Celebration timer (frame counter). IDA: dword_4AAEB8 (bridge_celebrationLastTime) */
	uint32 _celebrationTimer = 0;

	/** Celebration interval (120 or 60 frames). IDA: dword_4AAEBC (bridge_celebrationInterval) */
	uint32 _celebrationInterval = 120;

	/** Celebration pool cursor. IDA: dword_4AAEC0 (bridge_celebrationPoolState) */
	uint32 _celebrationPoolCursor = 0;

	/** Previous exclude count (level 0 retry avoidance). IDA: bridge_prevExcludeCount (0x41665D) */
	uint32 _prevExcludeCount = 0;

	/** Previous exclude pattern (level 0 retry avoidance). IDA: bridge_prevExcludePattern (0x416668) */
	uint32 _prevExcludePattern = 0;

	/** Re-entrance guard. IDA: word_4A07FC */
	bool _processingFrame = false;

	/** Last frame counter snapshot. IDA: snapshotFrameCounter_414C6C */
	uint32 _lastFrameSnapshot = 0;

	// --- Feature handles ---

	/** SCRB feature indices for troll animations. IDA: word_4AAE68 (0x4B0), word_4AAE64 (0x4B1), word_4AAE66 (0x4B2), word_4AAE6A (0x451) */
	uint16 _scrbTrollLane1Idx = 0;  // 0x4B0 = SCRB 1200
	uint16 _scrbTrollLane2Idx = 0;  // 0x4B1 = SCRB 1201
	uint16 _scrbTrollGateIdx = 0;   // 0x4B2 = SCRB 1202
	uint16 _scrbTrollMainIdx = 0;   // 0x451 = SCRB 1105 (troll gate overlay)

	/** Water overlay feature index. IDA: word_4AAE6C */
	uint16 _scrbWaterIdx = 0;

	/** Bridge segment feature indices (2). IDA: wLastScrbListIdxArr_4B7B4A */
	uint16 _scrbSegmentIdx[2] = {};

	// --- Button regions ---
	// IDA: word_4A0630/word_4A0632 tables (button rects, button idx 1-3)
	// Button 1 = Map, Button 2 = Go, Button 3 = Help
	// Decoded from IDA data at 0x4A0630:
	// Map button (idx 1): rect (0x0258, 0x0193, 0x027F, 0x01B8)
	// Go button  (idx 2): rect (0x0258, 0x01B9, 0x027F, 0x01DE)
	// Help button(idx 3): same as system help button
	const Common::Rect kMapButtonRect = Common::Rect(600, 403, 639, 440);
	const Common::Rect kGoButtonRect = Common::Rect(600, 441, 639, 478);
	const Common::Rect kHelpButtonRect = Common::Rect(600, 365, 639, 402);
};

} // End of namespace Mohawk

#endif
