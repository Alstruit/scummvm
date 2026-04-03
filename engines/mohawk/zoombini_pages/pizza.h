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
	void onEveryFrame() override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;

	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) override;

protected:
	void onGoButtonActivated() override;

private:
	// --- Initialization ---
	void loadZoombinisFromPack();
	void setDifficultyParams();
	void generateToppingSet();
	void distributeToppings();

	// --- Ingredient toggle ---
	void handleIngredientToggle(int16 ingredientIdx);

	// --- Order classification & delivery ---
	/**
	 * Classify current meal against an order line.
	 * IDA: pizza_classifyOrderType (0x43E5C9)
	 * @param orderLine 0=correct, 1=wrongA, 2=wrongB
	 * @return 0=one correct, 1=all wrong, 2=exact match, 4=multiple non-wrong
	 */
	int16 classifyOrderType(int16 orderLine) const;

	/** Serve toppings to the current order line. IDA: pizza_serveNextTopping (0x43E75F) */
	void serveNextTopping();

	/** Evaluate the final delivery result. IDA: pizza_evaluateDelivery (0x4403A4) */
	void evaluateDelivery();

	/** Advance to next zoombini for delivery. IDA: pizza_advanceToNextDeliverySlot (0x4409DA) */
	void advanceToNextDeliverySlot();

	/** Advance the intro sequence step. IDA: pizza_advanceIntroSequence (0x440C04) */
	void advanceIntroSequence();

	// --- Topping bitmask history ---
	/** Pack current ingredient flags into a bitmask. IDA: pizza_packToppingBitmask (0x43F794) */
	uint8 packToppingBitmask() const;

	/** Check if current bitmask already tried. IDA: pizza_checkToppingMaskMatch (0x43F848) */
	bool checkToppingMaskMatch() const;

	// --- Drag-and-drop ---
	void endDrag(const Common::Point &dropPos);

	// -----------------------------------------------------------------------
	// Static data (IDA)
	// -----------------------------------------------------------------------
	static const Common::Point kSnoidPositions[16];
	static const Common::Point kAnswerDisplayPosition;
	static const uint16 kToppingScrbBase[4];

	/** Click rect for the answer/submit area. IDA: 0x43CFA1 case 4 */
	static const Common::Rect kAnswerClickRect;

	// -----------------------------------------------------------------------
	// Difficulty parameters (IDA: set in pizza_init)
	// -----------------------------------------------------------------------
	int16 _difficultyLevel = 0;
	int16 _totalToppingSlots = 5;
	int16 _targetToppingCount = 2;
	int16 _toppingPlaceThreshold = 500;
	int16 _minToppingsPerOrder = 1;
	int16 _extraToppingTiers = 0;
	int16 _remainingDeliveries = 6;

	// -----------------------------------------------------------------------
	// Topping state arrays (IDA: 8 words each)
	// -----------------------------------------------------------------------
	uint8 _toppingSet[16] = {};
	uint8 _correctToppings[16] = {};
	uint8 _wrongToppingsA[16] = {};
	uint8 _wrongToppingsB[16] = {};

	/** Current player-selected toppings ("meal"). IDA: word_4B0DAC[0..7] */
	int16 _currentMeal[8] = {};

	/** Per-ingredient toggle flags. IDA: pizza_ingredientFlag0-7 */
	int16 _ingredientFlags[8] = {};

	// -----------------------------------------------------------------------
	// Order line state (1=active, 2=pending, 3=done). IDA: pizza_order0State-2State
	// -----------------------------------------------------------------------
	int16 _orderState[3] = {};

	// -----------------------------------------------------------------------
	// Delivery tracking
	// -----------------------------------------------------------------------
	/** Current delivery index (which zmb is being served). IDA: pizza_deliveryIndex */
	int16 _deliveryIndex = 0;
	/** Was the last delivery correct? IDA: pizza_wasDeliveryCorrect */
	int16 _wasDeliveryCorrect = 0;
	/** Consecutive correct deliveries. IDA: pizza_deliveryStreak */
	int16 _deliveryStreak = 0;
	/** All deliveries done flag. IDA: pizza_allDeliveriesDone */
	bool _allDeliveriesDone = false;
	/** All orders ready flag (all lines reached state >= 2). IDA: pizza_allOrdersReady */
	bool _allOrdersReady = false;
	/** Delivery animation in progress. IDA: pizza_isDeliveryInProgress */
	bool _isDeliveryInProgress = false;
	/** Retry counter for same order. IDA: pizza_retryCounter */
	int16 _retryCounter = 0;

	// -----------------------------------------------------------------------
	// Topping bitmask history (duplicate detection). IDA: pizza_toppingMaskHistory
	// -----------------------------------------------------------------------
	uint8 _toppingMaskHistory[256] = {};
	int16 _toppingMaskHistoryIdx = 0;

	// -----------------------------------------------------------------------
	// Intro sequence. IDA: pizza_introSequenceStep
	// -----------------------------------------------------------------------
	int16 _introSequenceStep = 0;
	bool _introComplete = false;

	// -----------------------------------------------------------------------
	// Answer display state
	// -----------------------------------------------------------------------
	/** Snoid currently at the answer area. IDA: pizza_answerZmb */
	ZmbSnoid *_answerSnoid = nullptr;
	/** Index in pack of current answer zmb. IDA: word_4B0D04 */
	int16 _answerZmbPackIdx = -1;

	// -----------------------------------------------------------------------
	// Puzzle active / reentrancy
	// -----------------------------------------------------------------------
	bool _puzzleActive = false;
	bool _processingFrame = false;
	int16 _pendingOrderCount = 0;
	int16 _currentToppingType = 0;

	// -----------------------------------------------------------------------
	// Idle animation. IDA: pizza_idleAnim*
	// -----------------------------------------------------------------------
	int16 _maxIdleAnims = 0;
	int16 _idleAnimsPlayed = 0;
	bool _idleAnimActive = false;
	uint32 _lastIdleFrame = 0;

	// -----------------------------------------------------------------------
	// Feature runners
	// -----------------------------------------------------------------------
	ZmbFeature *_drawOnRegFeature = nullptr;
	ZmbFeature *_treeAnimFeature = nullptr;
	ZmbFeature *_toppingFeatures[8] = {};
	uint16 _toppingCount = 0;
	ZmbFeature *_order1Feature = nullptr;
	ZmbFeature *_orderBaseFeature = nullptr;
	ZmbFeature *_order2Feature = nullptr;
	ZmbFeature *_overlayFeature = nullptr;

	// -----------------------------------------------------------------------
	// Resource IDs
	// -----------------------------------------------------------------------
	enum {
		kResSound996_DepartSFX = 996,
		kResSound997_Intro = 997,
	};
};

} // End of namespace Mohawk

#endif
