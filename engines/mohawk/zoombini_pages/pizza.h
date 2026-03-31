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

#ifndef MOHAWK_ZOOMBINI_PAGES_PIZZA_H
#define MOHAWK_ZOOMBINI_PAGES_PIZZA_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Pizza puzzle page (ZoombiniPageType::kPizza).
 *
 * Route 1, End puzzle: Zoombinis order pizzas with specific toppings.
 * The player must figure out which toppings each Zoombini wants by
 * trial and error. Correct toppings make the Zoombini happy.
 *
 * IDA entry: puzzlePizza_43B394
 */
class ZoombiniInteractivePizza : public ZoombiniInteractive {
public:
	ZoombiniInteractivePizza(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractivePizza() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;

private:
	void loadZoombinisFromPack();
	void generateToppingSet();
	void distributeToppings();

	static const Common::Point kSnoidPositions[16];

	/** Position of the answer display area for DRAW_ON_REG. IDA: stru_4A381C+8 */
	static const Common::Point kAnswerDisplayPosition;

	/** Base SCRB IDs for topping features per difficulty level. */
	static const uint16 kToppingScrbBase[4];

	/** Difficulty level (0-3). IDA: pizza_difficultyLevel */
	int16 _difficultyLevel = 0;

	/** Number of topping slots (5 + difficulty). IDA: pizza_totalToppingSlots */
	int16 _totalToppingSlots = 5;

	/** Target number of toppings to place on pizza. IDA: pizza_targetToppingCount */
	int16 _targetToppingCount = 0;

	/** Threshold for random topping placement (out of 1000). IDA: pizza_toppingPlaceThreshold */
	int16 _toppingPlaceThreshold = 500;

	/**
	 * Topping slot active flags (1 = has topping, 0 = empty).
	 * IDA: pizza_toppingSet
	 */
	uint8 _toppingSet[16] = {};

	/**
	 * Correct topping flags — these make the zoombini happy.
	 * IDA: pizza_correctToppings
	 */
	uint8 _correctToppings[16] = {};

	/**
	 * Wrong topping A flags — these are incorrect (used at all difficulty levels).
	 * IDA: pizza_wrongToppingsA
	 */
	uint8 _wrongToppingsA[16] = {};

	/**
	 * Wrong topping B flags — secondary incorrect set (levels 2-3 only).
	 * IDA: pizza_wrongToppingsB
	 */
	uint8 _wrongToppingsB[16] = {};

	/** SCRB 7063 — answer display drawn on region. IDA: scrb_drawOnRegRunnerIdxArr[0] */
	ZmbFeature *_drawOnRegFeature = nullptr;
	/** SCRB 7000 — main tree/interaction animation. IDA: 0x4B0CC0 */
	ZmbFeature *_treeAnimFeature = nullptr;
	/** Topping display features (up to 8). IDA: pizza_scrbTopping0-7 */
	ZmbFeature *_toppingFeatures[8] = {};
	/** Number of active topping features. */
	uint16 _toppingCount = 0;
	/** SCRB 9034 — order 1 display (diff>=1). IDA: pizza_scrbOrder1Runner */
	ZmbFeature *_order1Feature = nullptr;
	/** SCRB 8032 — order base display (always). IDA: 0x4B0CDE */
	ZmbFeature *_orderBaseFeature = nullptr;
	/** SCRB 10038 — order 2 display (diff>=2). IDA: pizza_scrbOrder2Runner */
	ZmbFeature *_order2Feature = nullptr;
	/** SCRB 8033 — overlay base runner. IDA: pizza_overlayBaseRunner */
	ZmbFeature *_overlayFeature = nullptr;

	enum {
		kResSound996_DepartSFX = 996
	};
};

} // End of namespace Mohawk

#endif
