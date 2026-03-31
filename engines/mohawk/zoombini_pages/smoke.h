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

#ifndef MOHAWK_ZOOMBINI_PAGES_SMOKE_H
#define MOHAWK_ZOOMBINI_PAGES_SMOKE_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Smoke puzzle page (ZoombiniPageType::kSmoke).
 *
 * Route 3, Puzzle 3: Zoombinis must be sorted by comparing attributes
 * on a smoke-stack cliffside. The puzzle uses a custom stack-based
 * positioning system instead of the standard walk-in layout.
 * NON-STANDARD: Does not use layoutStaticAndWalkIn().
 *
 * IDA entry: smoke_init (0x44983c)
 */
class ZoombiniInteractiveSmoke : public ZoombiniInteractive {
public:
	ZoombiniInteractiveSmoke(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveSmoke() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;

private:
	void loadZoombinisFromPack();

	static const Common::Point kSnoidPositions[20];

	/** DRAW_ON_REG position for answer display. IDA: stru_4A400C */
	static const Common::Point kDrawOnRegPosition;

	/** Route difficulty level + 1 (1-4). IDA: smoke_difficultyLevel */
	int16 _difficultyLevel = 0;

	/**
	 * Orientation flag for runner attribute assignment.
	 * Levels 1-2: 0; Levels 3-4: 2.
	 * IDA: runner+241 in smoke_assignRunnerAttrsForLevel
	 */
	uint8 _orientation = 0;

	/**
	 * Seen attribute history arrays for cross-level persistence.
	 * Cleared at level 1, used for 65% reuse at levels 3-4.
	 * IDA: smoke_seenAttrA, smoke_seenAttrB
	 */
	uint8 _seenAttrA[4] = {};
	uint8 _seenAttrB[4] = {};

	/** SCRB overlay animation (11013 or 11011). IDA: smoke_scrbOverlayAnim */
	ZmbFeature *_overlayAnimFeature = nullptr;
	/** SCRB 11076 — extra feature for diff 1/2 only. IDA: smoke_scrbLevel12Extra */
	ZmbFeature *_level12ExtraFeature = nullptr;
	/** SCRB 11006 — cliff left animation. IDA: smoke_scrbCliffLeft */
	ZmbFeature *_cliffLeftFeature = nullptr;
	/** SCRB 11007 — cliff right animation. IDA: smoke_scrbCliffRight */
	ZmbFeature *_cliffRightFeature = nullptr;
	/** Main animation (diff 1/2: 11024, diff 3/4: 11028). IDA: smoke_scrbMainAnim */
	ZmbFeature *_mainAnimFeature = nullptr;
	/** Smoke stack A animation. IDA: smoke_scrbSmokeStackA */
	ZmbFeature *_smokeStackAFeature = nullptr;
	/** Smoke stack B animation (diff 3/4 only). IDA: smoke_scrbSmokeStackB */
	ZmbFeature *_smokeStackBFeature = nullptr;
	/** Second animation. IDA: smoke_scrbSecondAnim */
	ZmbFeature *_secondAnimFeature = nullptr;
	/** SCRB 11018 — compare A animation. IDA: smoke_scrbCompareA */
	ZmbFeature *_compareAFeature = nullptr;
	/** SCRB 11019 — compare B animation. IDA: smoke_scrbCompareB */
	ZmbFeature *_compareBFeature = nullptr;
	/** SCRB 11009 — background overlay. IDA: smoke_scrbBgOverlay */
	ZmbFeature *_bgOverlayFeature = nullptr;
	/** SCRB 11036 — rejection animation. IDA: smoke_scrbRejection */
	ZmbFeature *_rejectionFeature = nullptr;
	/** SCRB 11008 — background. IDA: smoke_scrbBackground */
	ZmbFeature *_backgroundFeature = nullptr;
	/** SCRB 11002 — answer zone animation. IDA: smoke_scrbAnswerZone */
	ZmbFeature *_answerZoneFeature = nullptr;
	/** SCRB 11077 — holding area. IDA: smoke_scrbHoldingArea */
	ZmbFeature *_holdingAreaFeature = nullptr;
	/** SCRB 11001 — DRAW_ON_REG (diff < 3 only). IDA: scrb_drawOnRegRunnerIdxArr[0] */
	ZmbFeature *_drawOnRegFeature = nullptr;
};

} // End of namespace Mohawk

#endif
