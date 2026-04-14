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

#ifndef MOHAWK_ZOOMBINI_PAGES_TRANSITION_XFER_H
#define MOHAWK_ZOOMBINI_PAGES_TRANSITION_XFER_H

#include "mohawk/zoombini_pages/transition_base.h"
#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_state.h"

#include "common/array.h"
#include "graphics/surface.h"

namespace Mohawk {

class ZoombiniTransitionXfer : public ZoombiniTransition {
public:
	ZoombiniTransitionXfer(MohawkEngine_Zoombini *vm);
	~ZoombiniTransitionXfer() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;
	void onEveryFrame() override;
	void close() override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;

protected:
	// Constants
	enum kPageResourceId : uint16 {
		kResBackground1000_BigBadHungry = 1000,
		kResShapes1100_BigBadHungry = 1100,
		kResBackground2000_WhosBayou = 2000,
		kResShapes2100_WhosBayou = 2100,
		kResBackground3000_DeepDarkForest = 3000,
		kResShapes3100_DeepDarkForest = 3100,
		kResBackground4000_MountainOfDespair = 4000,
		kResShapes4100_MountainOfDespair = 4100,
		kResBackground5000_FromIsle = 5000,
		kResShapes5100_FromIsle = 5100,
		kResBackground6000_ToTown = 6000,
		kResShapes6100_ToTown = 6100,
	};

	enum kXferRouteId : uint16 {
		XFER_ROUTE0_FROM_ISLE = 0,
		XFER_ROUTE1_BIG_BAD_HUNGRY = 1,
		XFER_ROUTE2_WHOS_BAYOU = 2,
		XFER_ROUTE3_DEEP_DARK_FOREST = 3,
		XFER_ROUTE4_MOUNTAIN_OF_DESPAIR = 4,
		XFER_ROUTE5_TO_TOWN = 5,
	};

	enum kSnoidBase : uint16 {
		kSnoidPackBase = 60000,
	};

	// Route determination
	void computeXferRoute();

	// Sound selection
	uint16 selectXferSound() const;

	// XFer state, set by computeXferRoute()
	uint16 _xferView = XFER_ROUTE0_FROM_ISLE;
	ZoombiniPageType _nextPageType = ZoombiniPageType::kBridge;
	uint16 _xferBackgroundResId = kResBackground5000_FromIsle;
	uint16 _xferShapesId = kResShapes5100_FromIsle;
	uint16 _xferScrbCount = 9;   ///< Number of main environment SCRBs to load

	uint16 _nextPackSnoidId = 0;

	// Completion tracking for auto-close.
	// IDA: ALL views use getElapsedFrameTime_460872() > 0x12C (300 frames) + sound-finish check.
	uint32 _closureFrame = 0;      ///< Absolute frame counter for timer-based auto-close (all views: +300 frames)
	uint16 _xferSoundId = 0;       ///< SND resource ID being played (for close-wait-for-sound check)
	bool _useSmallSnoids = false;   ///< Use small-scale snoid shapes (XFER_0 only; drives resource 3200 + small tables)

	// SCRS periodic trigger state (XFER_0 and XFER_5 only)
	// IDA: dword_4B97BC (next trigger frame), word_4B97F4 (trigger index), word_4B97EE (phase1 flag)
	uint32 _scrsNextTriggerFrame = 0;  ///< Absolute frame counter for next SCRS trigger event
	uint16 _scrsTriggerIdx = 0;        ///< Index of next snoid to trigger (0..snoidCount-1)
	bool _scrsTriggerPhase1 = false;   ///< True once the first snoid trigger has fired (XFER_0: enables env SCRB branch)
	uint16 _xferSnoidCount = 0;        ///< Total snoids loaded for this XFER (for trigger indexing)
	uint16 _scrsResIdBase = 5199;      ///< SCRS resource base for foot-trait offset (XFER_0: 5199, XFER_5: 6199)

	// SCRB animation callback state (XFER_0 and XFER_5 only)
	// IDA: xfer_scrbAnimCallback_467DD4 — handles SCRS event codes during playback.

	/**
	 * Snoid SCRS completion counter (IDA: word_4B97E4).
	 * Incremented on event code 26 (animation complete). When >4 (5 snoids done),
	 * the final env SCRB is activated to trigger page transition.
	 * Set to -1 to disable further counting after final activation.
	 */
	int16 _completionCounter = 0;

	/**
	 * Pending body arrangement override (IDA: word_4B97E0).
	 * Set by event codes 240-243 (value = eventCode - 239, so 1-4).
	 * Applied on the next event code 0 (visibility toggle) as arrangement (value - 1).
	 * 0 = no pending override.
	 */
	uint16 _bodyArrangementOverride = 0;

	/**
	 * SCRB IDs of the 4 env animation runners (XFER_0 only).
	 * IDA: word_4B97D4[0..3] — loaded SCRBs 5102-5105 (indices relative to kResShapesFromIsle).
	 * Activated randomly (40% chance) in onEveryFrame when the SCRS trigger timer fires.
	 * Set to 0 when consumed (event codes 10-11 clear their entry).
	 */
	uint16 _envScrbIds[4] = {0, 0, 0, 0};

	/**
	 * SCRB ID of the one-shot env animation runner (XFER_0 only).
	 * IDA: word_4B97D2 — loaded SCRB 5108.
	 * Activated once (rand==4 in the 40% branch); _envOneShotAvailable gates re-use.
	 */
	uint16 _envOneShotScrbId = 0;
	bool _envOneShotAvailable = false;

	/**
	 * Feature linking SCRB ID (IDA: word_4B97E2).
	 * XFER_0: SCRB 5100 (foreground overlay, linked to snoids on event 26/cycle 2).
	 * XFER_5: SCRB 6104 (mid-background, linked to snoids on event 26).
	 */
	uint16 _linkTargetScrbId = 0;

	/**
	 * Final env SCRB ID (IDA: word_4B97E6, XFER_5 only: 6108).
	 * Activated when _completionCounter > 4.
	 * XFER_0 uses _linkTargetScrbId (5100) instead for the final link.
	 */
	uint16 _finalEnvScrbId = 0;

	/**
	 * One-shot trigger flags for events 10-11 (XFER_0 only).
	 * IDA: word_4B97E8[0..1] — initialized to true, cleared after activation.
	 * [0] = SCRB 5102 (event 10), [1] = SCRB 5103 (event 11).
	 */
	bool _envEventTriggerFlags[2] = {false, false};

	/**
	 * SCRB ID for event 50 activation (XFER_5 only: 6105).
	 * IDA: word_4B9802 — runner for SCRB 6105 (town count display).
	 */
	uint16 _xfer5EventScrbId = 0;

	// -----------------------------------------------------------------------
	// Route Path Flood-Fill State (XFER_1-4 only)
	// IDA: xfer_onPostRenderRoutePath (0x468457) — post-render flood-fill animation.
	// -----------------------------------------------------------------------

	/**
	 * Route Path animation counter (IDA: dword_4B9808).
	 * Increments by 7 per frame, wraps at 1000. Controls flood-fill expansion rate.
	 */
	uint32 _routePathCounter = 0;

	/**
	 * Frame-interval gate for route path flood-fill (IDA: dNextRenderFrame).
	 * In the original engine, runner_preRenderStandard (0x4619A1) only sets
	 * chGetDrawnRect=1 when dNextRenderFrame <= scrb_dwFrameRenderTime,
	 * then advances dNextRenderFrame += dFrameInterval. The post-render
	 * callback only runs flood-fill when chGetDrawnRect is set.
	 * We replicate this by checking _currentFrameCounter >= _routePathNextFrame,
	 * then advancing _routePathNextFrame += feature->getFrameInterval().
	 */
	uint32 _routePathNextFrame = 0;

	/**
	 * Route band position (IDA: word_4B9804).
	 * 1-4 based on which crossing within the current route.
	 * Used for shape selection and seed index.
	 */
	uint16 _routePathBand = 1;

	/**
	 * Route color level (IDA: word_4B97FA).
	 * 1-4 based on the puzzle difficulty / route progression level.
	 * Determines the flood-fill color string ("10/.", "3210", "5432", "7654").
	 * Separate from _routePathLevel because band position and color level differ:
	 * e.g. first traversal always uses level 1 colors for all bands.
	 */
	uint16 _routePathColorLevel = 1;

	/**
	 * Pointer to the route path overlay feature for callback.
	 */
	ZmbFeature *_routePathFeature = nullptr;

	/**
	 * Working pixel buffer for flood-fill (points into shape surface pixels).
	 * Set on first render call; dimensions match the path overlay shape.
	 */
	byte *_routePathPixels = nullptr;
	uint16 _routePathWidth = 0;
	uint16 _routePathHeight = 0;
	uint32 _routePathPitch = 0;

	/**
	 * BFS queue for flood-fill expansion.
	 * IDA: byte_4B9880[24] (active flags), word_4B9820/4B9822 (x/y coords).
	 */
	static constexpr int kRoutePathQueueSize = 24;
	bool _routePathQueueActive[kRoutePathQueueSize] = {};
	int16 _routePathQueueX[kRoutePathQueueSize] = {};
	int16 _routePathQueueY[kRoutePathQueueSize] = {};

	/**
	 * Flood-fill progress tracking.
	 * IDA: dword_4B980C (total replaceable), dword_4B9810 (remaining to fill).
	 */
	uint32 _routePathTotalPixels = 0;
	uint32 _routePathRemainingPixels = 0;

	/**
	 * Color values for flood-fill (IDA: byte_4B989C-4B989F).
	 * mark1/mark2: intermediate colors set during init.
	 * replace1/replace2: final colors set during expansion.
	 */
	byte _routePathMark1 = 0;
	byte _routePathMark2 = 0;
	byte _routePathReplace1 = 0;
	byte _routePathReplace2 = 0;

	/**
	 * Screen position where the route path overlay was rendered.
	 */
	Common::Rect _routePathScreenRect;

	// -----------------------------------------------------------------------
	// Route View Slot State (XFER_1-4 only)
	// IDA: xfer_updateRouteViewSlots (0x467B2C) — remaps main SCRB shapes
	// based on puzzle completion to show completed bands in foreground colors.
	// -----------------------------------------------------------------------

	/**
	 * Per-puzzle completion level array (IDA: xfer_puzzleCompletionArr, 0x4B97C0).
	 * Built from game state flags by buildPuzzleCompletionArray().
	 * Index = ZMB_SI_PAGE enum value (0-16).
	 * Values: -1 = current band (being animated), 0 = not completed, 1-4 = highest level.
	 */
	int8 _puzzleCompletionArr[17] = {};

	/**
	 * Route progress level for shape variant selection (IDA: xfer_wRouteProgressLevel, 0x4B97B6).
	 * Used by routeView_updateSlots to select shape variant for the current & cascading bands.
	 * -1 = uninitialized, 0-4 = level.
	 */
	int16 _routeProgressLevel = -1;

	/**
	 * Global route slot index for the current destination (IDA: word_4B97F8).
	 * Maps to a position in kRouteViewSlotTable.
	 */
	int16 _routeSlotIndex = 0;

	// Route path methods
	void computeRoutePathBand();
	void computeRoutePathColorLevel();
	void buildPuzzleCompletionArray();
	static uint16 readPuzzleLevelFlag(const ZmbStateFile &state, ZMB_SI_PAGE siPage);
	void routeView_updateSlots(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void routePath_selectBand(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	ZmbRenderResult routePath_onPostRender(ZmbFeature *feature);
	void routePath_initGrid(int16 seedX, int16 seedY, byte mark1, byte mark2, byte replace1, byte replace2);
	void routePath_expandFloodFill(uint32 counter);
	void routePath_reserveSlot(int16 y, int16 x, byte *pixel);

	// Helper: activate a deferred env SCRB feature by ID.
	void activateEnvScrb(uint16 scrbId);
};

} // End of namespace Mohawk

#endif
