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
		kResBackgroundBigBadHungry = 1000,
		kResShapesBigBadHungry = 1100,
		kResBackgroundWhosBayou = 2000,
		kResShapesWhosBayou = 2100,
		kResBackgroundDeepDarkForest = 3000,
		kResShapesDeepDarkForest = 3100,
		kResBackgroundMountainOfDespair = 4000,
		kResShapesMountainOfDespair = 4100,
		kResBackgroundFromIsle = 5000,
		kResShapesFromIsle = 5100,
		kResBackgroundToTown = 6000,
		kResShapesToTown = 6100,
	};

	enum kXferRouteId : uint16 {
		XFER_ROUTE_FROM_ISLE = 0,
		XFER_ROUTE_BIG_BAD_HUNGRY = 1,
		XFER_ROUTE_WHOS_BAYOU = 2,
		XFER_ROUTE_DEEP_DARK_FOREST = 3,
		XFER_ROUTE_MOUNTAIN_OF_DESPAIR = 4,
		XFER_ROUTE_TO_TOWN = 5,
	};

	enum kSnoidBase : uint16 {
		kSnoidPackBase = 60000,
	};

	// Route determination
	void computeXferRoute();

	// Sound selection
	uint16 selectXferSound() const;

	// XFer state, set by computeXferRoute()
	uint16 _xferView = XFER_ROUTE_FROM_ISLE;
	ZoombiniPageType _nextPageType = ZoombiniPageType::kBridge;
	uint16 _xferBgId = kResBackgroundFromIsle;
	uint16 _xferShapesId = kResShapesFromIsle;
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
	uint16 _scrsResIdBase = 5200;      ///< SCRS resource ID for snoid trigger (XFER_0: 5200, XFER_5: 6200)

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

	// Helper: activate a deferred env SCRB feature by ID.
	void activateEnvScrb(uint16 scrbId);
};

} // End of namespace Mohawk

#endif
