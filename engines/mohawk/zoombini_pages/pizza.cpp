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
	_initialDeliveryCount  = kDelivery[_difficultyLevel];

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
	// Z-order (back→front): Shyler → Willa → Arno.
	// LOOP_ANIM features are unsorted; registration order = draw order.
	if (_difficultyLevel >= 2) {
		_order2Feature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 10000), 10038, 6,
			ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
			ZmbFeature::FLAG_00100000_PLAY_ONCE);
	}

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

	// IDA: pizza_overlayBaseRunner — SCRB 8033, flags=0x4108000
	_overlayFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), 8033, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
		ZmbFeature::FLAG_04000000_OVERLAY);

	// Question runner feature — used for SCRB 7066 (delivery exit callback chain)
	_questionRunnerFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 7000), 7066, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_00020000_SKIP_RENDER);

	// Save a pointer to overlay sub-feature for delivery overlays
	{
		ZmbFeature *found = _subFeatureMap.find(12000);
		if (found) {
			_toppingOverlayFeature = found;
		}
	}

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
// The original uses a cascading polling state machine. In ScummVM, most
// state transitions are driven by onFeatureAnimEvent. This per-frame
// function handles idle animations, go-departure, and completion checks.
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::onEveryFrame() {
	if (_processingFrame || !_puzzleActive)
		return;
	_processingFrame = true;

	// Pending Go departure — skip normal frame logic
	if (_pendingGoDepart) {
		_processingFrame = false;
		return;
	}

	// Check if all orders are ready (all active lines matched or accepted)
	if (!_allOrdersReady && _introComplete) {
		bool allReady = true;
		for (int16 i = 0; i <= _extraToppingTiers; i++) {
			if (_orderState[i] < 2) {
				allReady = false;
				break;
			}
		}
		if (allReady)
			_allOrdersReady = true;
	}

	// Check if all deliveries are done
	if (_allOrdersReady && !_allDeliveriesDone && _remainingDeliveries <= 0) {
		_allDeliveriesDone = true;
		setGoButtonsEnabled(true);
	}

	// Idle fidget scheduling
	if (_idleAnimActive && _idleAnimsPlayed < _maxIdleAnims) {
		uint32 now = getCurrentFrameCounter();
		if (now > _lastIdleFrame + 180) { // ~3 seconds between fidgets
			_lastIdleFrame = now;

			// Pick a random snoid from the map and play a fidget
			for (auto it = _snoidMap.begin(); it != _snoidMap.end(); it++) {
				ZmbSnoid *s = *it;
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
// onFeatureAnimEvent: Comprehensive dispatch based on feature identity + phase
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// --- Question runner events (SCRB 7066 — exit callback chain) ---
	if (feature == _questionRunnerFeature) {
		if (_questionRunnerPhase == kPhaseExitCallback) {
			handleZmbExitEvent(feature, eventCode);
		}
		return;
	}

	// --- Order base feature events (Arno) ---
	if (feature == _orderBaseFeature) {
		if (eventCode == -1) {
			switch (_orderBasePhase) {
			case kPhaseIntro:
				_orderBasePhase = kPhaseNone;
				advanceIntroSequence();
				break;
			case kPhaseServeReaction:
				_orderBasePhase = kPhaseNone;
				handleOrderLineComplete(0);
				break;
			case kPhaseDeliveryEval:
				_orderBasePhase = kPhaseNone;
				// After delivery eval animation, play snoid SCRS
				handleZmbDeliveryEvent(feature, -1);
				break;
			default:
				break;
			}
		}
		return;
	}

	// --- Order 1 feature events (Willa) ---
	if (feature == _order1Feature) {
		if (eventCode == -1) {
			switch (_order1Phase) {
			case kPhaseIntro:
				_order1Phase = kPhaseNone;
				advanceIntroSequence();
				break;
			case kPhaseServeReaction:
				_order1Phase = kPhaseNone;
				handleOrderLineComplete(1);
				break;
			default:
				break;
			}
		}
		return;
	}

	// --- Order 2 feature events (Shyler) ---
	if (feature == _order2Feature) {
		if (eventCode == -1) {
			switch (_order2Phase) {
			case kPhaseIntro:
				_order2Phase = kPhaseNone;
				advanceIntroSequence();
				break;
			case kPhaseServeReaction:
				_order2Phase = kPhaseNone;
				handleOrderLineComplete(2);
				break;
			default:
				break;
			}
		}
		return;
	}

	// --- Topping overlay events ---
	if (feature == _toppingOverlayFeature) {
		if (eventCode == -1) {
			if (_overlayPhase == kPhaseToppingDelivery) {
				_overlayPhase = kPhaseNone;
				onToppingDelivered();
			} else if (_overlayPhase == kPhaseToppingOverlay) {
				_overlayPhase = kPhaseNone;
				// Initial overlay done — this is handled via exit callback
			}
		}
		return;
	}

	// --- Tree animation events ---
	if (feature == _treeAnimFeature) {
		// Tree anim completion not used in pizza puzzle
		return;
	}

	// --- Topping feature events ---
	for (uint16 i = 0; i < _toppingCount; i++) {
		if (feature == _toppingFeatures[i]) {
			return;
		}
	}

	// --- Snoid events ---
	if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
		ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);
		if (eventCode == -1) {
			SnoidAnimState state = snoid->getAnimState();
			if (state == kSnoidAnimScriptNormal || state == kSnoidAnimScriptReject) {
				snoid->setAnimState(kSnoidAnimIdle);
				snoid->setupIdleHotspots();
			}
		}
		return;
	}

	// --- Draw-on-reg (answer display) ---
	if (feature == _drawOnRegFeature) {
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
	if (!_puzzleActive || !_introComplete || _isDeliveryInProgress > 0)
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
		if (_answerSnoid && _isDeliveryInProgress == 0 &&
		    !_allOrdersReady && !_allDeliveriesDone) {
			handleSubmit();
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
				if (*it == snoid) {
					_answerZmbPackIdx = (*it)->getId() - 10000;
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

	// IDA: pizza_handleIngredientToggle — after toggling (v1=1),
	// calls pizza_registerAnswerDisplay() to refresh the big preview button.
	registerAnswerDisplay();

	debugC(kZmbDebugPage, "Pizza: Ingredient %d toggled %s (SCRB %d)",
	       ingredientIdx, _ingredientFlags[ingredientIdx] ? "ON" : "OFF", targetScrb);
}

// ---------------------------------------------------------------------------
// classifyOrderType: IDA 0x43E5C9
// Classify current meal against an order line.
// v1 = count of selected toppings NOT in order (extras/non-matching)
// v2 = count of selected toppings IN order (matching)
// v3 = count of toppings in the order
// Returns: 0=one-extra, 1=partial-subset, 2=exact-match, 4=multi-extra
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
		return 1;
	}

	int16 nonMatching = 0; // v1: selected but NOT in order
	int16 matching = 0;    // v2: selected AND in order
	int16 orderCount = 0;  // v3: total toppings in order

	for (int16 i = 0; i < _totalToppingSlots; i++) {
		if (orderArray[i])
			orderCount++;
		if (_currentMeal[i]) {
			if (orderArray[i])
				matching++;
			else
				nonMatching++;
		}
	}

	// IDA: if exactly one non-matching extra → return 0
	if (nonMatching == 1)
		return 0;

	// IDA: if multiple non-matching extras → return 4
	if (nonMatching > 1)
		return 4;

	// Here nonMatching == 0: all selected toppings are in the order
	// IDA: if all order toppings are selected (exact match) → return 2
	if (orderCount == matching)
		return 2;

	// IDA: some order toppings not selected → return 1 (partial subset)
	return 1;
}

// ---------------------------------------------------------------------------
// serveNextTopping: IDA 0x43E75F
// Play the serve reaction animation for a specific order line.
// @param resultType Classification result (0=one-extra, 1=partial, 2=exact, 3=dead, 4=multi-extra)
// @param orderLine Which order line (0=Arno, 1=Willa, 2=Shyler)
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::serveNextTopping(int16 resultType, int16 orderLine) {
	ZmbFeature *orderFeature = nullptr;
	uint16 scrbId = 0;

	switch (orderLine) {
	case 0: // Arno (order base)
		orderFeature = _orderBaseFeature;
		switch (resultType) {
		case 0: // One extra
			scrbId = 8006 + _anim0_oneCorrectCtr;
			_anim0_oneCorrectCtr = (_anim0_oneCorrectCtr + 1) % 2;
			break;
		case 1: // Partial subset
			scrbId = 8000 + _anim0_allWrongCtr;
			if (_anim0_allWrongCtr < 5)
				_anim0_allWrongCtr++;
			break;
		case 2: // Exact match
			scrbId = 8017 + _vm->_rnd->getRandomNumber(0, 2);
			break;
		case 3: // Dead code path
			scrbId = 8015 + _vm->_rnd->getRandomNumber(0, 1);
			break;
		case 4: // Multiple extras
			scrbId = 8008 + _anim0_multiNonWrongCtr;
			_anim0_multiNonWrongCtr = (_anim0_multiNonWrongCtr + 1) % 6;
			break;
		default:
			scrbId = 8000;
			break;
		}
		_orderBasePhase = kPhaseServeReaction;
		break;

	case 1: // Willa (order 1)
		orderFeature = _order1Feature;
		switch (resultType) {
		case 0:
			scrbId = 9000 + _anim1_oneCorrectCtr;
			_anim1_oneCorrectCtr = (_anim1_oneCorrectCtr + 1) % 5;
			break;
		case 1:
			scrbId = 9021 + _anim1_allWrongCtr;
			if (_anim1_allWrongCtr < 4)
				_anim1_allWrongCtr++;
			break;
		case 2:
			scrbId = 9010 + _vm->_rnd->getRandomNumber(0, 6);
			break;
		case 3:
			scrbId = 9017 + _vm->_rnd->getRandomNumber(0, 1);
			break;
		case 4:
			scrbId = 9005 + _anim1_multiNonWrongCtr;
			_anim1_multiNonWrongCtr = (_anim1_multiNonWrongCtr + 1) % 5;
			break;
		default:
			scrbId = 9000;
			break;
		}
		_order1Phase = kPhaseServeReaction;
		break;

	case 2: // Shyler (order 2)
		orderFeature = _order2Feature;
		switch (resultType) {
		case 0:
			scrbId = 10014 + _anim2_oneCorrectCtr;
			_anim2_oneCorrectCtr = (_anim2_oneCorrectCtr + 1) % 6;
			break;
		case 1:
			scrbId = 10009 + _anim2_allWrongCtr;
			if (_anim2_allWrongCtr < 4)
				_anim2_allWrongCtr++;
			break;
		case 2:
			scrbId = 10023 + _vm->_rnd->getRandomNumber(0, 3);
			break;
		case 3:
			scrbId = 10027 + _vm->_rnd->getRandomNumber(0, 2);
			break;
		case 4:
			scrbId = 10020 + _anim2_multiNonWrongCtr;
			_anim2_multiNonWrongCtr = (_anim2_multiNonWrongCtr + 1) % 3;
			break;
		default:
			scrbId = 10009;
			break;
		}
		_order2Phase = kPhaseServeReaction;
		break;

	default:
		return;
	}

	if (orderFeature) {
		loadScrbOntoFeature(orderFeature, scrbId);
		_currentServingLine = orderLine;
		debugC(kZmbDebugPage, "Pizza: Serving order %d, result=%d (SCRB %d)",
		       orderLine, resultType, scrbId);
	}

	// Update order state based on result
	if (resultType == 2) {
		_orderState[orderLine] = 2; // Matched (pending accept)
	}
}

// ---------------------------------------------------------------------------
// evaluateDelivery: IDA 0x4403A4
// Called after all active order lines have been served for one delivery.
// Loads the delivery evaluation SCRB on the base order feature.
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::evaluateDelivery() {
	// Determine if any order line was an exact match
	_wasDeliveryCorrect = 0;
	for (int16 i = 0; i <= _extraToppingTiers; i++) {
		if (_orderState[i] == 2) {
			// Mark as accepted
			_orderState[i] = 3;
			_wasDeliveryCorrect = 1;
		}
	}

	if (_wasDeliveryCorrect) {
		_deliveryStreak++;
	} else {
		_deliveryStreak = 0;
	}

	// Load delivery evaluation SCRB on the base order runner
	// IDA: SCRB = 8022 + wasDeliveryCorrect (order 0)
	uint16 evalScrbId = 8022 + _wasDeliveryCorrect;
	loadScrbOntoFeature(_orderBaseFeature, evalScrbId);
	_orderBasePhase = kPhaseDeliveryEval;

	_remainingDeliveries--;

	debugC(kZmbDebugPage, "Pizza: Delivery %s (remaining=%d, streak=%d, SCRB=%d)",
	       _wasDeliveryCorrect ? "CORRECT" : "WRONG",
	       _remainingDeliveries, _deliveryStreak, evalScrbId);
}

// ---------------------------------------------------------------------------
// advanceToNextDeliverySlot: IDA 0x4409DA
// Move to the next zoombini for delivery
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::advanceToNextDeliverySlot() {
	_isDeliveryInProgress = 0;
	_retryCounter = 0;
	_toppingMaskHistoryIdx = -1;
	memset(_toppingMaskHistory, 0, sizeof(_toppingMaskHistory));
	_currentServingLine = -1;

	// Reset ingredient flags
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

	// Handle the answer snoid
	if (_answerSnoid) {
		if (_wasDeliveryCorrect) {
			// Correct: snoid departs happily
			_answerSnoid->setAnimState(kSnoidAnimDepart);
			_answerSnoid->_packIsOccupied = false;
		} else {
			// Wrong: return snoid to pedestal
			_answerSnoid->setPointLoc(kSnoidPositions[_answerZmbPackIdx]);
			_answerSnoid->setAnimState(kSnoidAnimIdle);
			_answerSnoid->setupIdleHotspots();
		}
		_answerSnoid = nullptr;
		_answerZmbPackIdx = -1;
	}

	_deliveryIndex++;
	_idleAnimActive = false;

	// Reset phase tracking
	_orderBasePhase = kPhaseNone;
	_order1Phase = kPhaseNone;
	_order2Phase = kPhaseNone;
	_overlayPhase = kPhaseNone;
	_questionRunnerPhase = kPhaseNone;

	// Check if all deliveries are done
	if (_remainingDeliveries <= 0) {
		_allDeliveriesDone = true;
		setGoButtonsEnabled(true);
		debugC(kZmbDebugPage, "Pizza: All deliveries complete!");
	}
}

// ---------------------------------------------------------------------------
// advanceIntroSequence: IDA 0x440C04
// Steps through the intro animation sequence, matching original step numbering.
// Each step that loads a SCRB only increments the counter (1→2, 2→3, 3→4).
// The termination step (→0) fires on the NEXT callback, AFTER the SCRB finishes.
//
// Original flow:
//   Step 1: Load SCRB 8032 (Arno), step=2  (always)
//   Step 2: diff==0 → step=0 | diff>=1 → Load SCRB 9034 (Willa), step=3
//   Step 3: diff==1 → step=0 | diff>=2 → Load SCRB 10038 (Shyler), step=4
//   Step 4: step=0
//
// IDA 0x43CB75: After returning, frame update checks if step==0 and calls
//   town_triggerAmbientCharAnim (loads ambient SCRBs on the last troll).
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::advanceIntroSequence() {
	switch (_introSequenceStep) {
	case 1:
		// Step 1: Load intro SCRB 8032 on the base order runner (Arno)
		loadScrbOntoFeature(_orderBaseFeature, 8032);
		_orderBasePhase = kPhaseIntro;
		_introSequenceStep = 2;
		break;
	case 2:
		if (_difficultyLevel >= 1) {
			// Step 2 (diff>=1): Load intro SCRB 9034 on order 1 runner (Willa)
			if (_order1Feature) {
				loadScrbOntoFeature(_order1Feature, 9034);
				_order1Phase = kPhaseIntro;
			}
			_introSequenceStep = 3;
		} else {
			// Step 2 (diff==0): Arno's SCRB finished, intro done
			_introSequenceStep = 0;
		}
		break;
	case 3:
		if (_difficultyLevel >= 2) {
			// Step 3 (diff>=2): Load intro SCRB 10038 on order 2 runner (Shyler)
			if (_order2Feature) {
				loadScrbOntoFeature(_order2Feature, 10038);
				_order2Phase = kPhaseIntro;
			}
			_introSequenceStep = 4;
		} else {
			// Step 3 (diff==1): Willa's SCRB finished, intro done
			_introSequenceStep = 0;
		}
		break;
	case 4:
		// Step 4 (diff>=2): Shyler's SCRB finished, intro done
		_introSequenceStep = 0;
		break;
	default:
		break;
	}

	// IDA 0x43CB82: When step reaches 0, intro is complete.
	// IDA 0x43CB88: town_triggerAmbientCharAnim — loads an ambient SCRB
	// on the last troll feature (reactivates render for idle animation).
	if (_introSequenceStep == 0 && !_introComplete) {
		_introComplete = true;
		_idleAnimActive = true;
		_maxIdleAnims = 2;
		_lastIdleFrame = getCurrentFrameCounter();
		triggerOrderFeatureAmbientAnim();
		debugC(kZmbDebugPage, "Pizza: Intro sequence complete");
	}
}

// ---------------------------------------------------------------------------
// triggerOrderFeatureAmbientAnim: IDA 0x440B14 (town_triggerAmbientCharAnim)
// Loads an ambient idle SCRB on the last active troll feature after the intro
// sequence completes.  This reactivates render (scrb_loadOnRunner sets
// wBoolDoRender=1) so the troll plays a short idle animation.
//   diff 0: SCRB 8014 on orderBase (Arno)
//   diff 1: random SCRB 9019-9020 on order1 (Willa)
//   diff>=2: random SCRB 10001-10008 on order2 (Shyler)
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::triggerOrderFeatureAmbientAnim() {
	if (_difficultyLevel == 0) {
		loadScrbOntoFeature(_orderBaseFeature, 8014);
	} else if (_difficultyLevel == 1) {
		int16 variant = _vm->_rnd->getRandomNumber(1); // 0 or 1
		loadScrbOntoFeature(_order1Feature, 9019 + variant);
	} else {
		int16 variant = _vm->_rnd->getRandomNumber(7); // 0-7
		loadScrbOntoFeature(_order2Feature, 10001 + variant);
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
	for (int16 i = 0; i <= _toppingMaskHistoryIdx; i++) {
		if (_toppingMaskHistory[i] == currentMask)
			return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
// handleSubmit: IDA: pizza_handleIngredientToggle case 4
// Called when player clicks submit in the answer area.
// Starts the delivery cycle: answer display → exit callback → overlay →
// classify & serve → evaluate → delivery callback → advance
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::handleSubmit() {
	// Check for duplicate submission
	if (checkToppingMaskMatch()) {
		debugC(kZmbDebugPage, "Pizza: Duplicate topping combination submitted");
		return;
	}

	// Record this combination in history
	_toppingMaskHistoryIdx++;
	if (_toppingMaskHistoryIdx < 28) {
		_toppingMaskHistory[_toppingMaskHistoryIdx] = packToppingBitmask();
	}

	// Snapshot the current meal
	for (int16 i = 0; i < 8; i++) {
		_mealSnapshot[i] = _ingredientFlags[i];
		_currentMeal[i] = _ingredientFlags[i];
	}

	// Start delivery sequence
	_isDeliveryInProgress++;

	// Load the answer display SCRB (7057 at level 0, 7058 at level 1+)
	uint16 answerScrbId = (_difficultyLevel == 0) ? 7057 : 7058;
	loadScrbOntoFeature(_drawOnRegFeature, answerScrbId);

	// Load SCRB 7066 on the question runner to start the exit callback chain
	if (_questionRunnerFeature) {
		loadScrbOntoFeature(_questionRunnerFeature, 7066);
		_questionRunnerPhase = kPhaseExitCallback;
	} else {
		// Fallback: if no question runner, directly classify and serve
		// This shouldn't happen with proper init, but provides safety
		for (int16 i = 0; i <= _extraToppingTiers; i++) {
			if (_orderState[i] == 1) {
				int16 result = classifyOrderType(i);
				serveNextTopping(result, i);
				break;
			}
		}
	}

	debugC(kZmbDebugPage, "Pizza: Submit — starting delivery cycle");
}

// ---------------------------------------------------------------------------
// handleZmbExitEvent: IDA 0x43F3E6
// Handles animation events from SCRB 7066 on the question runner.
// Event 32: Initial overlay setup
// Event 60: Snoid trait reveal
// Event -1: Delivery overlay and classify
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::handleZmbExitEvent(ZmbFeature *feature, int16 eventCode) {
	switch (eventCode) {
	case 32: {
		// Load initial topping overlay SCRB 12000
		if (_toppingOverlayFeature) {
			loadScrbOntoFeature(_toppingOverlayFeature, 12000);
			_overlayPhase = kPhaseToppingOverlay;
		}
		debugC(kZmbDebugPage, "Pizza: Exit callback event 32 — overlay setup");
		break;
	}

	case 60: {
		// Play snoid SCRS for trait reveal
		if (_answerSnoid) {
			int16 traitIdx = getTraitIndexForOrder(0);
			uint16 scrsId = 13000 + traitIdx;
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
				                 ZmbResource(ZmbArchiveKind::kPage, scrsId));
			if (scrsStream) {
				_answerSnoid->startScrsPlayback(scrsStream, false, false);
			}
		}
		debugC(kZmbDebugPage, "Pizza: Exit callback event 60 — snoid SCRS");
		break;
	}

	case -1: {
		// Load delivery overlay based on current delivery slot
		// For order 0: SCRB 12001 + traitIdx
		// For order 1: SCRB 12006 + traitIdx
		// For order 2: SCRB 12011 + traitIdx
		_questionRunnerPhase = kPhaseNone;

		int16 traitIdx = getTraitIndexForOrder(_deliverySlotType);
		uint16 overlayScrbId = 0;
		switch (_deliverySlotType) {
		case 0:
			overlayScrbId = 12001 + traitIdx;
			break;
		case 1:
			overlayScrbId = 12006 + traitIdx;
			break;
		case 2:
			overlayScrbId = 12011 + traitIdx;
			break;
		default:
			overlayScrbId = 12001;
			break;
		}

		if (_toppingOverlayFeature) {
			loadScrbOntoFeature(_toppingOverlayFeature, overlayScrbId);
			_overlayPhase = kPhaseToppingDelivery;
		}

		debugC(kZmbDebugPage, "Pizza: Exit callback event -1 — delivery overlay SCRB %d",
		       overlayScrbId);
		break;
	}

	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// handleZmbDeliveryEvent: IDA 0x44005D
// Handles events after delivery evaluation completes.
// Plays appropriate SCRS on the answer snoid and advances the slot.
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::handleZmbDeliveryEvent(ZmbFeature *feature, int16 eventCode) {
	if (eventCode == -1) {
		// Delivery evaluation animation finished
		if (_answerSnoid) {
			// Play delivery SCRS on the snoid
			// IDA: SCRS 14000 + wasDeliveryCorrect (order 0)
			//      SCRS 14002 + wasDeliveryCorrect (order 1)
			//      SCRS 14004 + wasDeliveryCorrect (order 2)
			uint16 scrsBase = 14000 + (_deliverySlotType * 2) + _wasDeliveryCorrect;
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
				                 ZmbResource(ZmbArchiveKind::kPage, scrsBase));
			if (scrsStream) {
				bool isReject = (_wasDeliveryCorrect == 0);
				_answerSnoid->startScrsPlayback(scrsStream, false, isReject);
			}

			// Play delivery SFX
			_vm->_sound->playZmbSound(
				ZmbResource(ZmbArchiveKind::kPage, 8040),
				Audio::Mixer::kSFXSoundType);
		}

		// Advance to next delivery slot after snoid finishes
		// The snoid SCRS completion will trigger advanceToNextDeliverySlot
		// via the snoid event handler
		advanceToNextDeliverySlot();
	}
}

// ---------------------------------------------------------------------------
// handleOrderLineComplete: Called when an order line's reaction animation finishes.
// Determines whether to serve the next order line or evaluate the delivery.
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::handleOrderLineComplete(int16 orderLine) {
	debugC(kZmbDebugPage, "Pizza: Order line %d reaction complete (state=%d)",
	       orderLine, _orderState[orderLine]);

	// Find the next active order line to serve
	int16 nextLine = -1;
	for (int16 i = orderLine + 1; i <= _extraToppingTiers; i++) {
		if (_orderState[i] == 1) {
			nextLine = i;
			break;
		}
	}

	if (nextLine >= 0) {
		// Serve the next active order line
		int16 result = classifyOrderType(nextLine);
		serveNextTopping(result, nextLine);
	} else {
		// All active lines have been served — evaluate
		evaluateDelivery();
	}
}

// ---------------------------------------------------------------------------
// onToppingDelivered: IDA 0x43FEA0
// Called when the topping delivery overlay animation completes.
// Classifies the current meal against each active order line and starts
// serving the first one.
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::onToppingDelivered() {
	debugC(kZmbDebugPage, "Pizza: Topping delivery overlay complete — classifying");

	// Find the first active order line and serve it
	for (int16 i = 0; i <= _extraToppingTiers; i++) {
		if (_orderState[i] == 1) {
			int16 result = classifyOrderType(i);
			serveNextTopping(result, i);
			return;
		}
	}

	// No active order lines — all are matched/accepted, evaluate
	evaluateDelivery();
}

// ---------------------------------------------------------------------------
// registerAnswerDisplay: IDA 0x43D615
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::registerAnswerDisplay() {
	uint16 scrbId = 7001 + _difficultyLevel;
	loadScrbOntoFeature(_drawOnRegFeature, scrbId);
	debugC(kZmbDebugPage, "Pizza: Answer display registered (SCRB %d)", scrbId);
}

// ---------------------------------------------------------------------------
// spawnAnswerZmb: IDA 0x440D32
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::spawnAnswerZmb() {
	uint16 scrbId = (_difficultyLevel == 0) ? 7067 : 7068;
	loadScrbOntoFeature(_drawOnRegFeature, scrbId);
	debugC(kZmbDebugPage, "Pizza: Spawn answer zmb (SCRB %d)", scrbId);
}

// ---------------------------------------------------------------------------
// animateAnswerZmb: IDA 0x4402EC
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::animateAnswerZmb() {
	// Set up the answer display for the current answer snoid
	registerAnswerDisplay();
}

// ---------------------------------------------------------------------------
// setupQuestionRunners: IDA 0x43F5CF
// Set up question SCRBs based on which order lines are active.
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::setupQuestionRunners() {
	bool has0 = (_orderState[0] >= 2);
	bool has1 = (_difficultyLevel >= 1) && (_orderState[1] >= 2);
	bool has2 = (_difficultyLevel >= 2) && (_orderState[2] >= 2);

	// Select question SCRBs based on which order lines are ready
	if (has0 && has1 && has2) {
		// All three active
		loadScrbOntoFeature(_orderBaseFeature, 8030 + _vm->_rnd->getRandomNumber(0, 1));
		loadScrbOntoFeature(_order1Feature, 9032 + _vm->_rnd->getRandomNumber(0, 1));
		loadScrbOntoFeature(_order2Feature, 10036 + _vm->_rnd->getRandomNumber(0, 1));
	} else if (has0 && has1) {
		loadScrbOntoFeature(_orderBaseFeature, 8026 + _vm->_rnd->getRandomNumber(0, 1));
		loadScrbOntoFeature(_order1Feature, 9030 + _vm->_rnd->getRandomNumber(0, 1));
	} else if (has0 && has2) {
		loadScrbOntoFeature(_orderBaseFeature, 8028 + _vm->_rnd->getRandomNumber(0, 1));
		loadScrbOntoFeature(_order2Feature, 10035);
	} else if (has0) {
		loadScrbOntoFeature(_orderBaseFeature, 8024 + _vm->_rnd->getRandomNumber(0, 1));
	}

	debugC(kZmbDebugPage, "Pizza: Question runners setup");
}

// ---------------------------------------------------------------------------
// placeTopping: IDA 0x440DD1
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::placeTopping(int16 orderSlot, int16 isAllWrong) {
	// Load random topping SCRB based on order slot
	uint16 scrbId = 0;
	switch (orderSlot) {
	case 0:
		scrbId = 8034 + _vm->_rnd->getRandomNumber(0, 1);
		break;
	case 1:
		scrbId = 9019 + _vm->_rnd->getRandomNumber(0, 1);
		break;
	case 2:
		scrbId = 10006 + _vm->_rnd->getRandomNumber(0, 2);
		break;
	default:
		return;
	}

	loadScrbOntoFeature(_orderBaseFeature, scrbId);
	debugC(kZmbDebugPage, "Pizza: Place topping (order=%d, SCRB=%d)", orderSlot, scrbId);
}

// ---------------------------------------------------------------------------
// playSFXForOrder: IDA 0x441104
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::playSFXForOrder(int16 sfxVariant) {
	uint16 sndId = 15000 + sfxVariant;
	_vm->_sound->playZmbSound(
		ZmbResource(ZmbArchiveKind::kPage, sndId),
		Audio::Mixer::kSFXSoundType);
}

// ---------------------------------------------------------------------------
// reloadScrbAnimation: Helper to swap SCRB on a feature
// ---------------------------------------------------------------------------
void ZoombiniInteractivePizza::reloadScrbAnimation(ZmbFeature *feature, uint16 scrbId) {
	if (feature) {
		loadScrbOntoFeature(feature, scrbId);
	}
}

// ---------------------------------------------------------------------------
// getTraitIndexForOrder: Get trait-based index for SCRS/overlay selection
// Returns a value 0-4 based on the answer snoid's traits.
// ---------------------------------------------------------------------------
int16 ZoombiniInteractivePizza::getTraitIndexForOrder(int16 orderSlot) const {
	if (!_answerSnoid)
		return 0;

	// IDA: Uses zoombini trait fields to determine topping type
	// The trait index is modulo 5 of a trait field
	return (_answerSnoid->_trait._head) % 5;
}

} // End of namespace Mohawk
