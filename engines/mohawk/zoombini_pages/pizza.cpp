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
#include "mohawk/zoombini_pages/pizza.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions at 0x4A3834 (16 POINTS)
const Common::Point ZoombiniInteractivePizza::kSnoidPositions[16] = {
	Common::Point(288, 389), Common::Point(240, 386), Common::Point(257, 434), Common::Point(202, 396),
	Common::Point(224, 437), Common::Point(186, 443), Common::Point(158, 400), Common::Point(151, 455),
	Common::Point(126, 391), Common::Point(118, 446), Common::Point( 89, 403), Common::Point( 86, 456),
	Common::Point( 48, 396), Common::Point( 51, 440), Common::Point( 20, 416), Common::Point( 18, 457),
};

// IDA: stru_4A381C+8 — DRAW_ON_REG position for answer display
const Common::Point ZoombiniInteractivePizza::kAnswerDisplayPosition = Common::Point(270, 334);

// IDA: base SCRB IDs for topping features per difficulty level (diff 0-3)
const uint16 ZoombiniInteractivePizza::kToppingScrbBase[4] = { 7005, 7015, 7027, 7041 };

// IDA: click rect for answer/submit area (derived from onClick case 4 / case 13)
const Common::Rect ZoombiniInteractivePizza::kAnswerClickRect = Common::Rect(290, 260, 600, 440);

ZoombiniInteractivePizza::ZoombiniInteractivePizza(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kPizza) {
}

ZoombiniInteractivePizza::~ZoombiniInteractivePizza() {
}

void ZoombiniInteractivePizza::open() {
	openArchive(ZMB_MHK_PIZZA);
}

void ZoombiniInteractivePizza::setBackgroundMusic() {
	// IDA: pizza_init (0x43b394) has no music playback call on page load.
	// sound_activeHandle is stored at end of funcInit for F1 replay only.
}

void ZoombiniInteractivePizza::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

// ---------------------------------------------------------------------------
// setDifficultyParams: Apply per-level constants (IDA §6 table)
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::setDifficultyParams() {
	// IDA: pizza_init sets these per-level parameters
	// Level 0: slots=5, target=2, threshold=500, min=1, extra=0, deliveries=6
	// Level 1: slots=7, target=3, threshold=800, min=2, extra=0, deliveries=7
	// Level 2: slots=7, target=3, threshold=1000, min=2, extra=1, deliveries=7
	// Level 3: slots=8, target=4, threshold=1000, min=3, extra=2, deliveries=7
	static const int16 kSlots[4]      = { 5,   7,    7,    8 };
	static const int16 kTarget[4]     = { 2,   3,    3,    4 };
	static const int16 kThreshold[4]  = { 500, 800,  1000, 1000 };
	static const int16 kMinPerOrd[4]  = { 1,   2,    2,    3 };
	static const int16 kExtraTier[4]  = { 0,   0,    1,    2 };
	static const int16 kDelivery[4]   = { 6,   7,    7,    7 };

	_totalToppingSlots     = kSlots[_difficultyLevel];
	_targetToppingCount    = kTarget[_difficultyLevel];
	_toppingPlaceThreshold = kThreshold[_difficultyLevel];
	_minToppingsPerOrder   = kMinPerOrd[_difficultyLevel];
	_extraToppingTiers     = kExtraTier[_difficultyLevel];
	_remainingDeliveries   = kDelivery[_difficultyLevel];

	// Order line activation (IDA: §6)
	_orderState[0] = 1;                              // Arno always active
	_orderState[1] = (_difficultyLevel >= 1) ? 1 : 0; // Willa at level 1+
	_orderState[2] = (_difficultyLevel >= 2) ? 1 : 0; // Shyler at level 2+
}

void ZoombiniInteractivePizza::loadFeatures() {
	// IDA: puzzlePizza_43B394
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// Apply per-level constants
	setDifficultyParams();

	// Generate and distribute toppings
	// IDA: pizza_generateToppingSet (0x43F349) and pizza_toppingDistribution (0x43E0E0)
	generateToppingSet();
	distributeToppings();

	// Load NODE and PATH for walk network
	// IDA: node_loadNodeAndPath(0x3E8u)
	loadNODE(ZmbArchiveKind::kPage, 1000);

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(0x64u)
	loadTerrainBitmap(100);

	// Preload shape images
	// IDA: shape_loadSubShapesFromArchive(&stru_4A381C, 0x1770u) — shapes at tBMP 6000
	_vm->_gfx->preloadImage(6000);
	_vm->_gfx->preloadImage(7000);
	_vm->_gfx->preloadImage(8000);
	_vm->_gfx->preloadImage(9000);
	_vm->_gfx->preloadImage(10000);
	_vm->_gfx->preloadImage(12000);

	// Load main features: 69 SCRBs at 7000
	// IDA: scrb_loadMainFeatureSet(69, 7000)
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 36, 8000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 36; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 8000), 8000 + i);
		}
	}

	// IDA: scrb_loadSubFeatureSet(0, 45, 12000)
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 45; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 12000), 12000 + i);
		}
	}

	// Conditional feature groups for difficulty levels 1+
	if (_difficultyLevel >= 1) {
		// IDA: scrb_loadSubFeatureSet(0, 35, 9000)
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 35; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 9000), 9000 + i);
		}
	}

	if (_difficultyLevel >= 2) {
		// IDA: scrb_loadSubFeatureSet(0, 39, 10000)
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 39; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 10000), 10000 + i);
		}
	}

	// Load reject pool: 6 reject scripts at SCRS 14000
	// IDA: scrs_loadRejectPool(0, 6, 14000)
	for (uint16 i = 0; i < 6; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  14000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 40 normal scripts at SCRS 13000
	// IDA: scrs_loadNormalPool(0, 40, 13000)
	for (uint16 i = 0; i < 40; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  13000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// IDA: answer display DRAW_ON_REG — SCRB 7063, interval=7
	_drawOnRegFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7063, 7,
		kAnswerDisplayPosition,
		ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER);

	// IDA: main tree/interaction animation — SCRB 7000, interval=6
	_treeAnimFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7000, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE);

	// IDA: topping display features (difficulty-dependent count and base SCRB)
	{
		_toppingCount = _totalToppingSlots;
		uint16 scrbBase = kToppingScrbBase[_difficultyLevel];
		for (uint16 i = 0; i < _toppingCount; i++) {
			_toppingFeatures[i] = loadScrbFeature(
				ZmbResource(ZmbArchiveKind::kPage, 7000), scrbBase + i * 2, 6,
				ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
				ZmbFeature::FLAG_00100000_PLAY_ONCE);
		}
	}

	// IDA: order display runners (conditional on difficulty)
	if (_difficultyLevel >= 1) {
		_order1Feature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 9000), 9034, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_00100000_PLAY_ONCE);
	}

	// IDA: MEMORY[0x4B0CDE] = SCRB 8032 (always)
	_orderBaseFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8032, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE);

	if (_difficultyLevel >= 2) {
		_order2Feature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 10038, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_00100000_PLAY_ONCE);
	}

	// IDA: pizza_overlayBaseRunner — SCRB 8033, flags=0x4108000
	_overlayFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8033, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// Load Zoombinis from active pack at 16 pedestal positions
	loadZoombinisFromPack();

	// Layout and stagger walk-in (200ms walk delay)
	layoutStaticAndWalkIn();
	assignStaggeredWalkDelays();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(6000);
	loadHelpButtonFeature();

	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagPizza);
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, (_difficultyLevel > 0) ? 20072 : 20071);

	// Start the intro sequence
	// IDA: pizza_advanceIntroSequence (0x440C04)
	_introSequenceStep = 1;
	_puzzleActive = true;
	advanceIntroSequence();
}

// ---------------------------------------------------------------------------
// onGoButtonActivated
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::onGoButtonActivated() {
	// IDA: pizza_onClick case 2
	_vm->_sound->stopAllSoundQueues();

	_departXferSrcSiPage = ZMB_SI_PIZZA_04;
	startDepartWalkAnimation(Common::Point(690, 250));
	ZoombiniInteractive::onGoButtonActivated();
}

// ---------------------------------------------------------------------------
// loadZoombinisFromPack
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::loadZoombinisFromPack() {
	ZmbStateFile &f = _vm->_state->_f;
	uint16 posIdx = 0;

	for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount && posIdx < 16; i++) {
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
// generateToppingSet: IDA 0x43F349
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::generateToppingSet() {
	memset(_toppingSet, 0, sizeof(_toppingSet));

	// At level 1, forbid topping slot 4
	int16 forbiddenSlot = (_difficultyLevel == 1) ? 4 : -1;

	int16 remaining = _targetToppingCount;

	do {
		for (int16 i = 0; i < _totalToppingSlots && remaining > 0; i++) {
			if (_vm->_rnd->getRandomNumber(0, 999) < _toppingPlaceThreshold) {
				if (_toppingSet[i] == 0 && i != forbiddenSlot) {
					_toppingSet[i] = 1;
					remaining--;
				}
			}
		}
	} while (remaining > 0);

	// Safety: ensure at least one topping placed
	bool anyPlaced = false;
	for (int16 i = 0; i < _totalToppingSlots; i++) {
		if (_toppingSet[i]) {
			anyPlaced = true;
			break;
		}
	}
	if (!anyPlaced) {
		int16 slot = _vm->_rnd->getRandomNumber(0, 3);
		_toppingSet[slot] = 1;
	}

	debugC(kZmbDebugPage, "Pizza: Generated topping set for %d slots (target %d)",
	       _totalToppingSlots, _targetToppingCount);
}

// ---------------------------------------------------------------------------
// distributeToppings: IDA 0x43E0E0
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::distributeToppings() {
	memset(_correctToppings, 0, sizeof(_correctToppings));
	memset(_wrongToppingsA, 0, sizeof(_wrongToppingsA));
	memset(_wrongToppingsB, 0, sizeof(_wrongToppingsB));

	if (_difficultyLevel == 0) {
		// Level 0: All toppings are correct
		for (int16 i = 0; i < _totalToppingSlots; i++) {
			_correctToppings[i] = _toppingSet[i];
		}
		debugC(kZmbDebugPage, "Pizza Level 0: All toppings correct");
		return;
	}

	int16 correctCount = 0;
	int16 wrongACount = 0;
	int16 wrongBCount = 0;

	if (_difficultyLevel == 1) {
		// Level 1: Binary distribution — 50/50 correct or wrong
		for (int16 i = 0; i < _totalToppingSlots; i++) {
			if (_toppingSet[i]) {
				if (_vm->_rnd->getRandomNumber(0, 1) == 0) {
					_correctToppings[i] = 1;
					correctCount++;
				} else {
					_wrongToppingsA[i] = 1;
					wrongACount++;
				}
			}
		}

		// Ensure at least one is assigned
		if (correctCount == 0 && wrongACount == 0) {
			int16 slot = _vm->_rnd->getRandomNumber(0, _totalToppingSlots - 1);
			if (_vm->_rnd->getRandomNumber(0, 999) >= 500) {
				_correctToppings[slot] = 1;
			} else {
				_wrongToppingsA[slot] = 1;
			}
		}
		debugC(kZmbDebugPage, "Pizza Level 1: correct=%d, wrongA=%d", correctCount, wrongACount);
		return;
	}

	// Levels 2-3: Three-way distribution
	for (int16 i = 0; i < _totalToppingSlots; i++) {
		if (_toppingSet[i]) {
			int16 category = _vm->_rnd->getRandomNumber(0, 2);
			switch (category) {
			case 0:
				_correctToppings[i] = 1;
				correctCount++;
				break;
			case 1:
				_wrongToppingsA[i] = 1;
				wrongACount++;
				break;
			default:
				_wrongToppingsB[i] = 1;
				wrongBCount++;
				break;
			}
		}
	}

	// Rebalancing: ensure each category has at least one topping
	while (correctCount == 0 || wrongACount == 0 || wrongBCount == 0) {
		if (correctCount == 0) {
			int16 slot;
			if (wrongACount <= wrongBCount && wrongBCount > 0) {
				do {
					slot = _vm->_rnd->getRandomNumber(0, _totalToppingSlots - 1);
				} while (!_wrongToppingsB[slot]);
				_wrongToppingsB[slot] = 0;
				wrongBCount--;
			} else if (wrongACount > 0) {
				do {
					slot = _vm->_rnd->getRandomNumber(0, _totalToppingSlots - 1);
				} while (!_wrongToppingsA[slot]);
				_wrongToppingsA[slot] = 0;
				wrongACount--;
			} else {
				break;
			}
			_correctToppings[slot] = 1;
			correctCount = 1;
		}

		if (wrongACount == 0) {
			int16 slot;
			if (correctCount <= wrongBCount && wrongBCount > 0) {
				do {
					slot = _vm->_rnd->getRandomNumber(0, _totalToppingSlots - 1);
				} while (!_wrongToppingsB[slot]);
				_wrongToppingsB[slot] = 0;
				wrongBCount--;
			} else if (correctCount > 0) {
				do {
					slot = _vm->_rnd->getRandomNumber(0, _totalToppingSlots - 1);
				} while (!_correctToppings[slot]);
				_correctToppings[slot] = 0;
				correctCount--;
			} else {
				break;
			}
			_wrongToppingsA[slot] = 1;
			wrongACount = 1;
		}

		if (wrongBCount == 0) {
			int16 slot;
			if (wrongACount >= correctCount && wrongACount > 0) {
				do {
					slot = _vm->_rnd->getRandomNumber(0, _totalToppingSlots - 1);
				} while (!_wrongToppingsA[slot]);
				_wrongToppingsA[slot] = 0;
				wrongACount--;
			} else if (correctCount > 0) {
				do {
					slot = _vm->_rnd->getRandomNumber(0, _totalToppingSlots - 1);
				} while (!_correctToppings[slot]);
				_correctToppings[slot] = 0;
				correctCount--;
			} else {
				break;
			}
			_wrongToppingsB[slot] = 1;
			wrongBCount = 1;
		}
	}

	debugC(kZmbDebugPage, "Pizza Level %d: correct=%d, wrongA=%d, wrongB=%d",
	       _difficultyLevel, correctCount, wrongACount, wrongBCount);
}

// ---------------------------------------------------------------------------
// onEveryFrame: IDA: pizza_onFrameUpdate (0x43C31B)
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::onEveryFrame() {
	if (_processingFrame || !_puzzleActive)
		return;
	_processingFrame = true;

	// [0] Pending Go departure — skip normal frame logic
	if (_pendingGoDepart) {
		_processingFrame = false;
		return;
	}

	// [1] Advance intro sequence if still in progress
	if (!_introComplete && _introSequenceStep > 0) {
		// Intro is driven by onFeatureAnimEvent callbacks (-1 end-of-cycle)
		// Nothing to do here per-frame; the sequence advances via anim events
	}

	// [2] Process pending delivery
	if (_isDeliveryInProgress && _pendingOrderCount > 0) {
		serveNextTopping();
	}

	// [3] Check if all orders are ready (all lines reached state >= 2)
	if (!_allOrdersReady) {
		bool allReady = true;
		for (int16 i = 0; i <= _extraToppingTiers; i++) {
			if (_orderState[i] == 1) {
				allReady = false;
				break;
			}
		}
		if (allReady)
			_allOrdersReady = true;
	}

	// [4] Check if all deliveries are done
	if (_allOrdersReady && !_allDeliveriesDone && _remainingDeliveries <= 0) {
		_allDeliveriesDone = true;
		setGoButtonsEnabled(true);
	}

	// [5] Idle fidget scheduling
	// IDA: if idleAnimActive and played < max, periodically play fidget SCRS
	if (_idleAnimActive && _idleAnimsPlayed < _maxIdleAnims) {
		uint32 now = getCurrentFrameCounter();
		if (now > _lastIdleFrame + 180) { // ~3 seconds between fidgets
			_lastIdleFrame = now;

			// Pick a random snoid from the map and play a fidget
			for (auto it = _snoidMap.begin(); it != _snoidMap.end(); it++) {
				ZmbSnoid *s = static_cast<ZmbSnoid *>(it->second);
				if (s->getAnimState() == kSnoidAnimIdle && s->_packIsOccupied) {
					s->setAnimState(kSnoidAnimFidget);
					_idleAnimsPlayed++;
					break;
				}
			}
		}
	}

	_processingFrame = false;
}

// ---------------------------------------------------------------------------
// onFeatureAnimEvent: IDA: dispatches by feature identity
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// Tree animation events
	if (feature == _treeAnimFeature) {
		if (eventCode == -1) {
			// Tree intro anim finished — advance intro
			if (!_introComplete) {
				advanceIntroSequence();
			}
		}
		return;
	}

	// Order base feature events (Arno)
	if (feature == _orderBaseFeature) {
		if (eventCode == -1) {
			if (!_introComplete) {
				advanceIntroSequence();
			} else if (_isDeliveryInProgress) {
				evaluateDelivery();
			}
		}
		return;
	}

	// Order 1 feature events (Willa)
	if (feature == _order1Feature) {
		if (eventCode == -1) {
			if (!_introComplete) {
				advanceIntroSequence();
			} else if (_isDeliveryInProgress) {
				evaluateDelivery();
			}
		}
		return;
	}

	// Order 2 feature events (Shyler)
	if (feature == _order2Feature) {
		if (eventCode == -1) {
			if (!_introComplete) {
				advanceIntroSequence();
			} else if (_isDeliveryInProgress) {
				evaluateDelivery();
			}
		}
		return;
	}

	// Topping feature events
	for (uint16 i = 0; i < _toppingCount; i++) {
		if (feature == _toppingFeatures[i]) {
			if (eventCode == -1) {
				// Topping animation done
			}
			return;
		}
	}

	// Snoid events
	if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
		ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);
		if (eventCode == -1) {
			// SCRS playback finished
			SnoidAnimState state = snoid->getAnimState();
			if (state == kSnoidAnimScriptNormal || state == kSnoidAnimScriptReject) {
				snoid->setAnimState(kSnoidAnimIdle);
				snoid->setupIdleHotspots();

				// If this was a delivery snoid, advance to next
				if (_isDeliveryInProgress) {
					advanceToNextDeliverySlot();
				}
			}
		}
		return;
	}

	// Draw-on-reg (answer display)
	if (feature == _drawOnRegFeature) {
		if (eventCode == -1) {
			// Answer display animation done
		}
		return;
	}
}

// ---------------------------------------------------------------------------
// onLButtonDown: Click handler
// IDA: pizza_onClick (0x43CFA1)
// ---------------------------------------------------------------------------
ZmbEventHandleResult ZoombiniInteractivePizza::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// Sticky mouse: second click drops dragged snoid
	if (isDragging() && _vm->_state->getEnableStickyMouse()) {
		endDrag(absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	// Let base class handle Go/Map/Help buttons
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Guard conditions
	if (!_puzzleActive || !_introComplete || _isDeliveryInProgress)
		return ZmbEventHandleResult::kPassthrough;
	if (isDragging())
		return ZmbEventHandleResult::kPassthrough;

	// --- Check ingredient toggle clicks (on topping features) ---
	// IDA: pizza_onClick cases 5-12 (ingredient toggles)
	for (uint16 i = 0; i < _toppingCount; i++) {
		if (_toppingFeatures[i]) {
			ZmbDrawRecord *drawRecord = _toppingFeatures[i]->findDrawRecordAtPoint(absPos);
			if (drawRecord) {
				handleIngredientToggle(i);
				return ZmbEventHandleResult::kConsumed;
			}
		}
	}

	// --- Check answer/submit area click ---
	// IDA: pizza_onClick case 4 / case 13
	if (kAnswerClickRect.contains(absPos)) {
		// Submit current pizza — only if not already in delivery
		if (_answerSnoid && !_isDeliveryInProgress) {
			// Check for duplicate submission
			if (checkToppingMaskMatch()) {
				// Already tried this exact combination — play a hint SFX
				debugC(kZmbDebugPage, "Pizza: Duplicate topping combination submitted");
				return ZmbEventHandleResult::kConsumed;
			}

			// Record this combination in history
			_toppingMaskHistory[_toppingMaskHistoryIdx] = packToppingBitmask();
			_toppingMaskHistoryIdx++;

			// Start delivery sequence
			_isDeliveryInProgress = true;
			_pendingOrderCount = 1 + _extraToppingTiers;

			// Load the answer display SCRB (7057 or 7058)
			loadScrbOntoFeature(_drawOnRegFeature, 7057);

			// Copy current meal and classify
			for (int16 j = 0; j < 8; j++) {
				_currentMeal[j] = _ingredientFlags[j];
			}

			serveNextTopping();
			return ZmbEventHandleResult::kConsumed;
		}
	}

	// --- Check snoid drag ---
	// IDA: pizza_onClick case 14 (drag Zoombini to answer area)
	ZmbSnoid *snoid = findSnoidAtPoint(absPos);
	if (snoid) {
		SnoidAnimState state = snoid->getAnimState();
		if (state == kSnoidAnimScriptReject || state == kSnoidAnimScriptNormal)
			return ZmbEventHandleResult::kPassthrough;

		startSnoidDrag(snoid, absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

// ---------------------------------------------------------------------------
// onLButtonUp: Release drag
// ---------------------------------------------------------------------------
ZmbEventHandleResult ZoombiniInteractivePizza::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	if (!isDragging())
		return ZoombiniInteractive::onLButtonUp(absPos, relPos);

	// Sticky mouse: don't drop on button-up
	if (_vm->_state->getEnableStickyMouse())
		return ZmbEventHandleResult::kConsumed;

	endDrag(absPos);
	return ZmbEventHandleResult::kConsumed;
}

// ---------------------------------------------------------------------------
// endDrag: Process snoid drop
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::endDrag(const Common::Point &dropPos) {
	ZmbSnoid *snoid = finishSnoidDrag();
	if (!snoid)
		return;

	Common::Point snoidPos = snoid->getPointLoc();

	// Check if dropped on the answer/delivery area
	if (kAnswerClickRect.contains(snoidPos)) {
		// Place this zoombini at the answer display position
		if (!_answerSnoid) {
			_answerSnoid = snoid;

			// Find the pack index for this snoid
			for (auto it = _snoidMap.begin(); it != _snoidMap.end(); it++) {
				if (it->second == snoid) {
					_answerZmbPackIdx = it->first - 10000;
					break;
				}
			}

			snoid->setPointLoc(kAnswerDisplayPosition);
			snoid->setAnimState(kSnoidAnimArrive);

			// Initialize the idle animation system for this interaction
			_idleAnimActive = true;
			_idleAnimsPlayed = 0;
			_maxIdleAnims = 3;
			_lastIdleFrame = getCurrentFrameCounter();

			debugC(kZmbDebugPage, "Pizza: Zoombini placed at answer area (packIdx=%d)",
			       _answerZmbPackIdx);
		} else {
			// Already have a zoombini at answer — return to original position
			snoid->setPointLoc(_dragOrigPos);
			snoid->setAnimState(kSnoidAnimIdle);
			snoid->setupIdleHotspots();
		}
	} else {
		// Dropped elsewhere — validate terrain and return to idle
		if (!validateTerrainDrop(snoid)) {
			snoid->setPointLoc(_dragOrigPos);
		}
		snoid->setAnimState(kSnoidAnimIdle);
		snoid->setupIdleHotspots();
	}
}

// ---------------------------------------------------------------------------
// handleIngredientToggle: Toggle a topping on/off
// IDA: pizza_handleIngredientToggle (0x43D79E), cases 5-12
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::handleIngredientToggle(int16 ingredientIdx) {
	if (ingredientIdx < 0 || ingredientIdx >= _totalToppingSlots)
		return;

	// Level-based restrictions on higher ingredients
	// IDA: case 9 (ingredient 4) blocked at level 1 (forbidden slot)
	if (ingredientIdx == 4 && _difficultyLevel == 1)
		return;
	if (ingredientIdx >= 5 && ingredientIdx <= 6 && _difficultyLevel < 1)
		return;
	if (ingredientIdx == 7 && _difficultyLevel < 3)
		return;

	// XOR toggle the flag
	_ingredientFlags[ingredientIdx] ^= 1;

	// Mirror into the meal array
	_currentMeal[ingredientIdx] = _ingredientFlags[ingredientIdx];

	// Swap topping SCRB to on/off visual
	// IDA: Each topping has 2 SCRBs: base+0 = off, base+1 = on
	uint16 scrbBase = kToppingScrbBase[_difficultyLevel];
	uint16 targetScrb = scrbBase + ingredientIdx * 2 + (_ingredientFlags[ingredientIdx] ? 1 : 0);

	if (_toppingFeatures[ingredientIdx]) {
		loadScrbOntoFeature(_toppingFeatures[ingredientIdx], targetScrb);
	}

	debugC(kZmbDebugPage, "Pizza: Ingredient %d toggled %s (SCRB %d)",
	       ingredientIdx, _ingredientFlags[ingredientIdx] ? "ON" : "OFF", targetScrb);
}

// ---------------------------------------------------------------------------
// classifyOrderType: IDA 0x43E5C9
// Classify current meal against an order line.
// @param orderLine 0=correct(Arno), 1=wrongA(Willa), 2=wrongB(Shyler)
// @return 0=one correct, 1=all wrong, 2=exact match, 4=multiple non-wrong
// ---------------------------------------------------------------------------
int16 ZoombiniInteractivePizza::classifyOrderType(int16 orderLine) const {
	const uint8 *orderArray;
	switch (orderLine) {
	case 0:
		orderArray = _correctToppings;
		break;
	case 1:
		orderArray = _wrongToppingsA;
		break;
	case 2:
		orderArray = _wrongToppingsB;
		break;
	default:
		return 1; // all wrong
	}

	int16 matchCount = 0;
	int16 orderCount = 0;
	int16 mealCount = 0;

	for (int16 i = 0; i < _totalToppingSlots; i++) {
		if (orderArray[i])
			orderCount++;
		if (_currentMeal[i])
			mealCount++;
		if (orderArray[i] && _currentMeal[i])
			matchCount++;
	}

	// No matches at all — all wrong
	if (matchCount == 0)
		return 1;

	// Exact match — all order toppings are selected and nothing extra
	if (matchCount == orderCount && matchCount == mealCount)
		return 2;

	// Exactly one correct
	if (matchCount == 1)
		return 0;

	// Multiple non-wrong
	return 4;
}

// ---------------------------------------------------------------------------
// serveNextTopping: IDA 0x43E75F
// Process the current order line delivery and animate result
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::serveNextTopping() {
	if (_pendingOrderCount <= 0) {
		_isDeliveryInProgress = false;
		return;
	}

	// Determine which order line to serve
	int16 orderLine = _extraToppingTiers - _pendingOrderCount + 1;
	if (orderLine < 0)
		orderLine = 0;

	int16 result = classifyOrderType(orderLine);

	// Load the appropriate result SCRB on the order feature
	// IDA: Result SCRBs are at offset from order base:
	//   Order 0: 8000-based (8022 + result)
	//   Order 1: 9000-based (9028 + result)
	//   Order 2: 10000-based (10032 + result)
	ZmbFeature *orderFeature = nullptr;
	uint16 resultScrbBase = 0;

	switch (orderLine) {
	case 0:
		orderFeature = _orderBaseFeature;
		resultScrbBase = 8022;
		break;
	case 1:
		orderFeature = _order1Feature;
		resultScrbBase = 9028;
		break;
	case 2:
		orderFeature = _order2Feature;
		resultScrbBase = 10032;
		break;
	default:
		break;
	}

	if (orderFeature) {
		uint16 resultScrb = resultScrbBase + result;
		loadScrbOntoFeature(orderFeature, resultScrb);
		debugC(kZmbDebugPage, "Pizza: Serving order %d, result=%d (SCRB %d)",
		       orderLine, result, resultScrb);
	}

	// Update order state based on result
	if (result == 2) {
		// Exact match — mark as done
		_orderState[orderLine] = 3;
		_wasDeliveryCorrect = 1;
		_deliveryStreak++;
	} else {
		// Not exact — keep active
		_wasDeliveryCorrect = 0;
		_deliveryStreak = 0;
	}

	_pendingOrderCount--;

	// If all order lines have been served, evaluate the full delivery
	if (_pendingOrderCount <= 0) {
		evaluateDelivery();
	}
}

// ---------------------------------------------------------------------------
// evaluateDelivery: IDA 0x4403A4
// Called after all order lines for one zoombini have been served
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::evaluateDelivery() {
	_remainingDeliveries--;

	if (_wasDeliveryCorrect) {
		// Correct delivery — animate answer zoombini positively
		if (_answerSnoid) {
			// Play a normal (happy) SCRS on the answer snoid
			uint16 scrsBase = 13000 + (_answerZmbPackIdx % 40);
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
				                 ZmbResource(ZmbArchiveKind::kPage, scrsBase));
			if (scrsStream) {
				_answerSnoid->startScrsPlayback(scrsStream, false, false);
			}
		}
		debugC(kZmbDebugPage, "Pizza: Delivery CORRECT (remaining=%d, streak=%d)",
		       _remainingDeliveries, _deliveryStreak);
	} else {
		// Wrong delivery — play reject animation
		_retryCounter++;
		if (_answerSnoid) {
			uint16 scrsBase = 14000 + (_retryCounter % 6);
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
				                 ZmbResource(ZmbArchiveKind::kPage, scrsBase));
			if (scrsStream) {
				_answerSnoid->startScrsPlayback(scrsStream, false, true);
			}
		}
		debugC(kZmbDebugPage, "Pizza: Delivery WRONG (remaining=%d, retry=%d)",
		       _remainingDeliveries, _retryCounter);
	}

	// Reset ingredient flags for next attempt
	for (int16 i = 0; i < 8; i++) {
		_ingredientFlags[i] = 0;
		_currentMeal[i] = 0;
	}

	// Reset topping visuals to "off" state
	uint16 scrbBase = kToppingScrbBase[_difficultyLevel];
	for (uint16 i = 0; i < _toppingCount; i++) {
		if (_toppingFeatures[i]) {
			loadScrbOntoFeature(_toppingFeatures[i], scrbBase + i * 2);
		}
	}
}

// ---------------------------------------------------------------------------
// advanceToNextDeliverySlot: IDA 0x4409DA
// Move to the next zoombini for delivery
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::advanceToNextDeliverySlot() {
	_isDeliveryInProgress = false;
	_retryCounter = 0;
	_toppingMaskHistoryIdx = 0;
	memset(_toppingMaskHistory, 0, sizeof(_toppingMaskHistory));

	// Return the current answer snoid to its original position
	if (_answerSnoid) {
		_answerSnoid->setPointLoc(kSnoidPositions[_answerZmbPackIdx]);
		_answerSnoid->setAnimState(kSnoidAnimIdle);
		_answerSnoid->setupIdleHotspots();
		_answerSnoid->_packIsOccupied = _wasDeliveryCorrect ? false : true;
		_answerSnoid = nullptr;
		_answerZmbPackIdx = -1;
	}

	_deliveryIndex++;
	_idleAnimActive = false;

	// Check if all deliveries are done
	if (_remainingDeliveries <= 0) {
		_allDeliveriesDone = true;
		setGoButtonsEnabled(true);
		debugC(kZmbDebugPage, "Pizza: All deliveries complete!");
	}
}

// ---------------------------------------------------------------------------
// advanceIntroSequence: IDA 0x440C04
// Steps through the intro animation sequence
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::advanceIntroSequence() {
	switch (_introSequenceStep) {
	case 1:
		// Step 1: Load SCRB 8032 on the base runner
		loadScrbOntoFeature(_orderBaseFeature, 8032);
		_introSequenceStep = 2;
		break;
	case 2:
		// Step 2: Load SCRB 9034 on order 1 runner (level >= 1)
		if (_difficultyLevel >= 1 && _order1Feature) {
			loadScrbOntoFeature(_order1Feature, 9034);
			_introSequenceStep = 3;
		} else {
			_introSequenceStep = 4;
			advanceIntroSequence(); // Skip to step 4
		}
		break;
	case 3:
		// Step 3: Load SCRB 10038 on order 2 runner (level >= 2)
		if (_difficultyLevel >= 2 && _order2Feature) {
			loadScrbOntoFeature(_order2Feature, 10038);
			_introSequenceStep = 4;
		} else {
			_introSequenceStep = 4;
			advanceIntroSequence(); // Skip to final
		}
		break;
	case 4:
		// Step 4: Intro complete
		_introSequenceStep = 0;
		_introComplete = true;
		_idleAnimActive = true;
		_maxIdleAnims = 2;
		_lastIdleFrame = getCurrentFrameCounter();

		// Do not play intro SFX here - Arno's voice SFX takes this role
		debugC(kZmbDebugPage, "Pizza: Intro sequence complete");
		break;
	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// packToppingBitmask: IDA 0x43F794
// Pack current ingredient flags into a single byte
// ---------------------------------------------------------------------------
uint8 ZoombiniInteractivePizza::packToppingBitmask() const {
	uint8 mask = 0;
	for (int16 i = 0; i < 8; i++) {
		if (_currentMeal[i])
			mask |= (1 << i);
	}
	return mask;
}

// ---------------------------------------------------------------------------
// checkToppingMaskMatch: IDA 0x43F848
// Returns true if current bitmask was already tried
// ---------------------------------------------------------------------------
bool ZoombiniInteractivePizza::checkToppingMaskMatch() const {
	uint8 currentMask = packToppingBitmask();
	for (int16 i = 0; i < _toppingMaskHistoryIdx; i++) {
		if (_toppingMaskHistory[i] == currentMask)
			return true;
	}
	return false;
}

} // End of namespace Mohawk
