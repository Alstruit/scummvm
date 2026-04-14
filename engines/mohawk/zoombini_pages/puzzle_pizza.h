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

#ifndef MOHAWK_ZOOMBINI_PAGES_PUZZLE_PIZZA_H
#define MOHAWK_ZOOMBINI_PAGES_PUZZLE_PIZZA_H

#include "mohawk/zoombini_pages/puzzle_base.h"

namespace Mohawk {

/**
 * Pizza puzzle page (ZoombiniPageType::kPizza).
 * Route 1, Puzzle 3
 *
 * Zoombinis deliver pizzas to the pizza trolls with specific toppings.
 * The player must figure out which toppings each pizza troll wants by trial and error.
 * Deliver exact correct toppings to the pizza trolls to make Zoombinis passable.
 *
 * IDA entry: puzzlePizza_43B394
 */
class ZoombiniPuzzlePizza : public ZoombiniPuzzle {
public:
	ZoombiniPuzzlePizza(MohawkEngine_Zoombini *vm);
	~ZoombiniPuzzlePizza() override;

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
	// --- Phase tracking for feature animations ---
	enum FeaturePhase {
		kPhaseNone = 0,
		kPhaseIntro,
		kPhasePostIntroAmbient, // IDA: wUnk002C[40] — troll ambient anim after intro
		kPhaseServeReaction,
		kPhaseDeliveryEval,
		kPhaseDeliveryResult, // IDA: slot 38 — delivery result SCRBs (8020/9026/10030)
		kPhaseExitCallback,
		kPhaseToppingOverlay,
		kPhaseToppingDelivery,
		kPhaseQuestionSetup,
		kPhaseSpawnAnswer,
		kPhaseAcceptTransition,
	};

	// --- Initialization ---
	void loadZoombinisFromPack();
	void setDifficultyParams();
	void generateToppingSet();
	void distributeToppings();

	// --- Ingredient toggle & submit ---
	void handleIngredientToggle(int16 ingredientIdx);
	void handleSubmit();

	// --- Answer display ---
	void registerAnswerDisplay();
	void answerDisplay_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void autoPickAnswerSnoid();

	// --- Order classification & delivery ---
	int16 classifyOrderType(int16 orderLine) const;
	void serveNextTopping(int16 orderLine);
	void placeTopping(int16 mode, int16 hintSlot);
	void evaluateDelivery();
	void loadDeliveryResultScrb();
	void advanceToNextDeliverySlot();
	void advanceIntroSequence();
	void triggerOrderFeatureAmbientAnim();
	void spawnAnswerZmb();
	void animateAnswerZmb();
	void setupQuestionRunners();
	void onToppingDelivered();
	void playSFXForOrder(int16 sfxVariant);

	// --- Topping runner management ---
	void registerToppingRunner();
	void linkToppingRunners();
	void toppingRunner_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);

	// --- Callback event handlers ---
	void handleZmbExitEvent(ZmbFeature *feature, int16 eventCode);
	void handleZmbDeliveryEvent(ZmbFeature *feature, int16 eventCode);
	void handleOrderLineComplete(int16 orderLine);

	// --- Topping bitmask history ---
	uint8 packToppingBitmask() const;
	bool checkToppingMaskMatch() const;

	// --- Drag-and-drop ---
	void endDrag(const Common::Point &dropPos);

	// --- Helpers ---
	void reloadScrbAnimation(ZmbFeature *feature, uint16 scrbId);
	int16 getTraitIndexForOrder(int16 orderSlot) const;

	// -----------------------------------------------------------------------
	// Static data
	// -----------------------------------------------------------------------
	static const Common::Point kSnoidPositions[16];
	static const Common::Point kAnswerDisplayPosition;
	static const uint16 kToppingScrbBase[4];
	static const Common::Rect kAnswerClickRect;

	// -----------------------------------------------------------------------
	// Difficulty parameters
	// -----------------------------------------------------------------------
	ZmbPuzzleDifficultyLevel _difficultyLevel = kPuzzleDiffLevel1;
	int16 _totalToppingSlots = 5;
	int16 _targetToppingCount = 2;
	int16 _toppingPlaceThreshold = 500;
	int16 _minToppingsPerOrder = 1;
	int16 _extraToppingTiers = 0;
	int16 _remainingDeliveries = 6;
	int16 _initialDeliveryCount = 6;

	// -----------------------------------------------------------------------
	// Topping state arrays
	// -----------------------------------------------------------------------
	uint8 _toppingSet[16] = {};
	uint8 _correctToppings[16] = {};
	uint8 _wrongToppingsA[16] = {};
	uint8 _wrongToppingsB[16] = {};
	int16 _currentMeal[8] = {};
	int16 _mealSnapshot[8] = {};
	int16 _ingredientFlags[8] = {};

	// -----------------------------------------------------------------------
	// Order line state (0=inactive, 1=active, 2=matched, 3=accepted)
	// -----------------------------------------------------------------------
	int16 _orderState[3] = {};

	// -----------------------------------------------------------------------
	// Delivery tracking
	// -----------------------------------------------------------------------
	int16 _deliveryIndex = -1;
	int16 _wasDeliveryCorrect = 0;
	int16 _deliveryStreak = 0;
	bool _allDeliveriesDone = false;
	bool _allOrdersReady = false;
	int16 _isDeliveryInProgress = 0;
	int16 _retryCounter = 0;
	int16 _currentServingLine = -1;
	int16 _deliverySlotType = 0;
	int16 _questionsAnswered = 0;
	int16 _hasMaskMatch = 0;
	int16 _pendingOrderCount = 0;
	int16 _currentToppingType = 0;
	int16 _currentOrderType = 0;
	int16 _skipDeliveryFlag = 0;
	int16 _pendingReplayFlag = 0;
	int16 _pendingAnimShape = 0;
	int16 _pendingDeliverySlot = 0;
	int16 _punishmentCount = 0;
	bool _needsSlotAdvance = false;
	bool _deliveryCallbackActive = false;

	// -----------------------------------------------------------------------
	// Topping bitmask history
	// -----------------------------------------------------------------------
	uint8 _toppingMaskHistory[28] = {};
	int16 _toppingMaskHistoryIdx = -1;

	// -----------------------------------------------------------------------
	// Intro sequence
	// -----------------------------------------------------------------------
	int16 _introSequenceStep = 1;
	bool _introComplete = false;

	// -----------------------------------------------------------------------
	// Animation cycling counters (per order line)
	// -----------------------------------------------------------------------
	int16 _anim0_allWrongCtr = 0;
	int16 _anim0_oneCorrectCtr = 0;
	int16 _anim0_multiNonWrongCtr = 0;
	int16 _anim1_allWrongCtr = 0;
	int16 _anim1_oneCorrectCtr = 0;
	int16 _anim1_multiNonWrongCtr = 0;
	int16 _anim2_allWrongCtr = 0;
	int16 _anim2_oneCorrectCtr = 0;
	int16 _anim2_multiNonWrongCtr = 0;

	// -----------------------------------------------------------------------
	// Phase tracking per feature
	// -----------------------------------------------------------------------
	FeaturePhase _orderBasePhase = kPhaseNone;
	FeaturePhase _order1Phase = kPhaseNone;
	FeaturePhase _order2Phase = kPhaseNone;
	FeaturePhase _overlayPhase = kPhaseNone;
	FeaturePhase _questionRunnerPhase = kPhaseNone;
	FeaturePhase _treePhase = kPhaseNone;
	FeaturePhase _drawOnRegPhase = kPhaseNone;

	// -----------------------------------------------------------------------
	// Answer snoid state
	// -----------------------------------------------------------------------
	ZmbSnoid *_answerSnoid = nullptr;
	int16 _answerZmbPackIdx = -1;

	// -----------------------------------------------------------------------
	// Puzzle state
	// -----------------------------------------------------------------------
	bool _puzzleActive = false;
	bool _processingFrame = false;
	bool _drawOnRegEnabled = false; // IDA: scrb_drawOnRegFlagArr[0] — gates submit clicks

	// -----------------------------------------------------------------------
	// Celebration animation (hoorah fidget)
	// IDA: pizza_idleAnimActive, pizza_idleAnimCount, pizza_maxIdleAnims
	// Per-puzzle SCRS scheduled on idle snoids after correct answers or
	// puzzle milestones. Distinct from the global idle fidget system which
	// is driven by user inactivity (kSnoidAnimFidget / onSnoidAnimTick).
	// -----------------------------------------------------------------------
	int16 _celebrationTarget = 0;
	int16 _celebrationsPlayed = 0;
	bool _celebrationActive = false;
	uint32 _lastCelebrationFrame = 0;
	uint16 _celebrationRandomUsed = 0; // bitmask for non-repeat random pool

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
	ZmbFeature *_questionRunnerFeature = nullptr;
	ZmbFeature *_toppingOverlayFeature = nullptr;

	// -----------------------------------------------------------------------
	// Topping runner tracking (IDA: word_4B0E06 array + counters)
	// -----------------------------------------------------------------------
	struct ToppingRunnerSlot {
		ZmbFeature *feature = nullptr;
		uint8 mask = 0;
		int16 orderType = 0;
		uint16 scrbId = 0;
	};
	ToppingRunnerSlot _toppingRunnerSlots[28] = {};
	int16 _toppingRunnerSlotIdx = -1;
	int16 _toppingRunnerCtrMain = -1;
	int16 _toppingRunnerCtr0 = -1;
	int16 _toppingRunnerCtr1 = -1;
	int16 _toppingRunnerCtr2 = -1;
	bool _toppingRunnersWrapped = false;
	ZmbFeature *_toppingRunnerOrder0Slots[3] = {};
	ZmbFeature *_toppingRunnerOrder1Slots[3] = {};
	ZmbFeature *_toppingRunnerOrder2Slots[3] = {};
	uint16 _nextDynamicFeatureId = 30000;

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
