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

#include "mohawk/zoombini_pages/puzzle_pizza.h"
#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions at 0x4A3834 (16 POINTS)
const Common::Point ZoombiniPuzzlePizza::kSnoidPositions[16] = {
	Common::Point(288, 389),
	Common::Point(240, 386),
	Common::Point(257, 434),
	Common::Point(202, 396),
	Common::Point(224, 437),
	Common::Point(186, 443),
	Common::Point(158, 400),
	Common::Point(151, 455),
	Common::Point(126, 391),
	Common::Point(118, 446),
	Common::Point(89, 403),
	Common::Point(86, 456),
	Common::Point(48, 396),
	Common::Point(51, 440),
	Common::Point(20, 416),
	Common::Point(18, 457),
};

// IDA: stru_4A381C+8 — DRAW_ON_REG position for answer display
const Common::Point ZoombiniPuzzlePizza::kAnswerDisplayPosition = Common::Point(270, 334);

// IDA: base SCRB IDs for topping features per difficulty level (diff 0-3)
const uint16 ZoombiniPuzzlePizza::kToppingScrbBase[4] = {7005, 7015, 7027, 7041};

// IDA: click rect for answer/submit area (derived from onClick case 4 / case 13)
const Common::Rect ZoombiniPuzzlePizza::kAnswerClickRect = Common::Rect(290, 260, 600, 440);

ZoombiniPuzzlePizza::ZoombiniPuzzlePizza(MohawkEngine_Zoombini *vm) : ZoombiniPuzzle(vm, ZoombiniPageType::kPizza) {
}

ZoombiniPuzzlePizza::~ZoombiniPuzzlePizza() {
}

void ZoombiniPuzzlePizza::open() {
	openArchive(ZMB_MHK_PIZZA);
}

void ZoombiniPuzzlePizza::setBackgroundMusic() {
	// IDA: pizza_init (0x43b394) has no music playback call on page load.
	// sound_activeHandle is stored at end of funcInit for F1 replay only.
}

void ZoombiniPuzzlePizza::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

// ---------------------------------------------------------------------------
// setDifficultyParams: Apply per-level constants (IDA §6 table)
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::setDifficultyParams() {
	// IDA: pizza_init sets these per-level parameters
	// Level 0: slots=5, target=2, threshold=500, min=1, extra=0, deliveries=6
	// Level 1: slots=7, target=3, threshold=800, min=2, extra=0, deliveries=7
	// Level 2: slots=7, target=3, threshold=1000, min=2, extra=1, deliveries=7
	// Level 3: slots=8, target=4, threshold=1000, min=3, extra=2, deliveries=7
	static const int16 kSlots[4] = {5, 7, 7, 8};
	static const int16 kTarget[4] = {2, 3, 3, 4};
	static const int16 kThreshold[4] = {500, 800, 1000, 1000};
	static const int16 kMinPerOrd[4] = {1, 2, 2, 3};
	static const int16 kExtraTier[4] = {0, 0, 1, 2};
	static const int16 kDelivery[4] = {6, 7, 7, 7};

	_totalToppingSlots = kSlots[_difficultyLevel];
	_targetToppingCount = kTarget[_difficultyLevel];
	_toppingPlaceThreshold = kThreshold[_difficultyLevel];
	_minToppingsPerOrder = kMinPerOrd[_difficultyLevel];
	_extraToppingTiers = kExtraTier[_difficultyLevel];
	_remainingDeliveries = kDelivery[_difficultyLevel];
	_initialDeliveryCount = kDelivery[_difficultyLevel];

	// Order line activation (IDA: §6)
	_orderState[0] = 1;                               // Arno always active
	_orderState[1] = (_difficultyLevel >= 1) ? 1 : 0; // Willa at level 1+
	_orderState[2] = (_difficultyLevel >= 2) ? 1 : 0; // Shyler at level 2+
}

void ZoombiniPuzzlePizza::loadFeatures() {
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
	{
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniPuzzlePizza::answerDisplay_preRenderShape));
		_drawOnRegFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 7000), 7063, 7,
			kAnswerDisplayPosition,
			ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
				ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER,
			hooks);
	}

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
		ZmbFeature *found = _subFeatures.find(12000);
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

	// IDA 0x43bdd5: Register answer display at init — makes button visible from start.
	// This is called BEFORE the intro sequence, matching the original engine.
	registerAnswerDisplay();

	// IDA 0x43c12c: Set delivery in progress before starting intro
	_isDeliveryInProgress = 1;

	// Start the intro sequence
	// IDA: pizza_advanceIntroSequence (0x440C04)
	_introSequenceStep = 1;
	_puzzleActive = true;
	advanceIntroSequence();

	// IDA 0x43c143: scrb_drawOnRegFlagArr[0] = 1 — enable the generate button
	_drawOnRegEnabled = true;
}

// ---------------------------------------------------------------------------
// onGoButtonActivated
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::onGoButtonActivated() {
	// IDA: pizza_onClick case 2
	_vm->_sound->stopAllSoundQueues();

	_departXferSrcSiPage = ZMB_SI_PIZZA_04;
	startDepartWalkAnimation(Common::Point(690, 250));
	ZoombiniInteractive::onGoButtonActivated();
}

// ---------------------------------------------------------------------------
// loadZoombinisFromPack
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::loadZoombinisFromPack() {
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
void ZoombiniPuzzlePizza::generateToppingSet() {
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
void ZoombiniPuzzlePizza::distributeToppings() {
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
void ZoombiniPuzzlePizza::onEveryFrame() {
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
		debugC(1, kZmbDebugAnimation, "Pizza: all deliveries done -> enabling Go button");
		_allDeliveriesDone = true;
		setGoButtonsEnabled(true);
	}

	// Celebration scheduling (hoorah fidget)
	// IDA: 0x43CE80..0x43CF8F — non-repeat random snoid SCRS playback
	if (_celebrationActive && _celebrationsPlayed < _celebrationTarget) {
		uint32 now = getCurrentFrameCounter();
		if (now > _lastCelebrationFrame + 30) { // IDA: 0x1E = 30 frame delta
			// Count remaining (non-departed) zoombinis
			int16 remaining = 0;
			for (auto it = _snoidMap.begin(); it != _snoidMap.end(); it++) {
				ZmbSnoid *s = *it;
				if (s->_packIsOccupied)
					remaining++;
			}

			if (remaining < 4) {
				// IDA: too few zoombinis — clear celebration, spawn answer zmb
				_isDeliveryInProgress = 0;
				_celebrationActive = false;
				spawnAnswerZmb();
			} else {
				// IDA: update last celebration frame
				_lastCelebrationFrame = now;

				// Collect eligible snoids for non-repeat random selection
				Common::Array<ZmbSnoid *> eligible;
				for (auto it = _snoidMap.begin(); it != _snoidMap.end(); it++) {
					ZmbSnoid *s = *it;
					if (s == _answerSnoid)
						continue;
					if (!s->_packIsOccupied)
						continue;
					if (s->getAnimState() != kSnoidAnimIdle)
						continue;
					if (s->_trait._foot == 0)
						continue;
					eligible.push_back(s);
				}

				if (!eligible.empty()) {
					// Non-repeat random: use bitmask to avoid repeats
					int16 poolSize = eligible.size();
					if (_celebrationRandomUsed >= (uint16)((1 << poolSize) - 1))
						_celebrationRandomUsed = 0; // All used, reset

					int16 idx;
					int16 attempts = 0;
					do {
						idx = _vm->_rnd->getRandomNumber(0, poolSize - 1);
						attempts++;
					} while ((_celebrationRandomUsed & (1 << idx)) && attempts < 32);

					_celebrationRandomUsed |= (1 << idx);
					ZmbSnoid *snoid = eligible[idx];

					// IDA: play SCRS (13035 + foot_trait - 1)
					uint16 scrsId = 13035 + snoid->_trait._foot - 1;
					Common::SeekableReadStream *scrsStream =
						_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
										 ZmbResource(ZmbArchiveKind::kPage, scrsId));
					if (scrsStream) {
						snoid->startScrsPlayback(scrsStream, false, true);
						_celebrationsPlayed++;
					}
				}
			}
		}
	}

	// IDA: 0x43CF68 — if played >= max, reset all celebration state
	if (_celebrationsPlayed >= _celebrationTarget && _celebrationTarget > 0) {
		_celebrationRandomUsed = 0;
		_lastCelebrationFrame = 0;
		_celebrationActive = false;
		_celebrationsPlayed = 0;
	}

	_processingFrame = false;
}

// ---------------------------------------------------------------------------
// onFeatureAnimEvent: Comprehensive dispatch based on feature identity + phase
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// --- Question runner events (SCRB 7066 — exit callback chain) ---
	if (feature == _questionRunnerFeature) {
		if (_questionRunnerPhase == kPhaseExitCallback) {
			handleZmbExitEvent(feature, eventCode);
		}
		return;
	}

	// --- Order base feature events (Arno) ---
	if (feature == _orderBaseFeature) {
		if (_orderBasePhase == kPhaseDeliveryEval && eventCode != kZmbAnimEventM1_End) {
			// IDA: delivery callback events (61, 0, etc.) during eval SCRB
			handleZmbDeliveryEvent(feature, eventCode);
			return;
		}
		if (eventCode == kZmbAnimEventM1_End) {
			switch (_orderBasePhase) {
			case kPhaseIntro:
				_orderBasePhase = kPhaseNone;
				advanceIntroSequence();
				break;
			case kPhasePostIntroAmbient:
				// IDA: wUnk002C[35] handler with town_bAmbientAnimActive —
				// ambient finishes → clear delivery lock, auto-pick snoid
				// (which loads answer display SCRB 7067/7068 via spawnAnswerZmb).
				_orderBasePhase = kPhaseNone;
				_isDeliveryInProgress = 0;
				autoPickAnswerSnoid();
				break;
			case kPhaseServeReaction:
				_orderBasePhase = kPhaseNone;
				handleOrderLineComplete(0);
				break;
			case kPhaseDeliveryEval:
				_orderBasePhase = kPhaseNone;
				handleZmbDeliveryEvent(feature, kZmbAnimEventM1_End);
				break;
			case kPhaseDeliveryResult:
				// IDA: slot 38 done → registerToppingRunner, then advance
				_orderBasePhase = kPhaseNone;
				registerToppingRunner();
				advanceToNextDeliverySlot();
				break;
			default:
				break;
			}
		}
		return;
	}

	// --- Order 1 feature events (Willa) ---
	if (feature == _order1Feature) {
		if (_order1Phase == kPhaseDeliveryEval && eventCode != kZmbAnimEventM1_End) {
			handleZmbDeliveryEvent(feature, eventCode);
			return;
		}
		if (eventCode == kZmbAnimEventM1_End) {
			switch (_order1Phase) {
			case kPhaseIntro:
				_order1Phase = kPhaseNone;
				advanceIntroSequence();
				break;
			case kPhasePostIntroAmbient:
				_order1Phase = kPhaseNone;
				_isDeliveryInProgress = 0;
				autoPickAnswerSnoid();
				break;
			case kPhaseServeReaction:
				_order1Phase = kPhaseNone;
				handleOrderLineComplete(1);
				break;
			case kPhaseDeliveryEval:
				_order1Phase = kPhaseNone;
				handleZmbDeliveryEvent(feature, kZmbAnimEventM1_End);
				break;
			case kPhaseDeliveryResult:
				// IDA: slot 38 done → registerToppingRunner, then advance
				_order1Phase = kPhaseNone;
				registerToppingRunner();
				advanceToNextDeliverySlot();
				break;
			default:
				break;
			}
		}
		return;
	}

	// --- Order 2 feature events (Shyler) ---
	if (feature == _order2Feature) {
		if (_order2Phase == kPhaseDeliveryEval && eventCode != kZmbAnimEventM1_End) {
			handleZmbDeliveryEvent(feature, eventCode);
			return;
		}
		if (eventCode == kZmbAnimEventM1_End) {
			switch (_order2Phase) {
			case kPhaseIntro:
				_order2Phase = kPhaseNone;
				advanceIntroSequence();
				break;
			case kPhasePostIntroAmbient:
				_order2Phase = kPhaseNone;
				_isDeliveryInProgress = 0;
				autoPickAnswerSnoid();
				break;
			case kPhaseServeReaction:
				_order2Phase = kPhaseNone;
				handleOrderLineComplete(2);
				break;
			case kPhaseDeliveryEval:
				_order2Phase = kPhaseNone;
				handleZmbDeliveryEvent(feature, kZmbAnimEventM1_End);
				break;
			case kPhaseDeliveryResult:
				// IDA: slot 38 done → registerToppingRunner, then advance
				_order2Phase = kPhaseNone;
				registerToppingRunner();
				advanceToNextDeliverySlot();
				break;
			default:
				break;
			}
		}
		return;
	}

	// --- Topping overlay events ---
	if (feature == _toppingOverlayFeature) {
		if (eventCode == kZmbAnimEventM1_End) {
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
		if (eventCode == kZmbAnimEventM1_End) {
			SnoidAnimState state = snoid->getAnimState();
			if (state == kSnoidAnimScriptNormal || state == kSnoidAnimScriptReject) {
				snoid->setAnimState(kSnoidAnimIdle);
				snoid->setupIdleHotspots();
			}
		}
		return;
	}

	// --- Draw-on-reg (answer display) ---
	// IDA: wUnk002C[40] handler in frame update. When the answer display
	// SCRB (7067/7068) finishes, clear _isDeliveryInProgress and pick
	// the next snoid for delivery.
	if (feature == _drawOnRegFeature) {
		if (eventCode == kZmbAnimEventM1_End && _drawOnRegPhase == kPhaseSpawnAnswer) {
			// IDA: wUnk002C+50h handler — answer display SCRB (7067/7068)
			// finished. Clear phase and delivery lock. The snoid was already
			// picked/walked by autoPickAnswerSnoid() before spawnAnswerZmb().
			_drawOnRegPhase = kPhaseNone;
			_isDeliveryInProgress = 0;
			debugC(kZmbDebugPage, "Pizza: Answer display SCRB done — delivery unlocked");
		}
		return;
	}
}

// ---------------------------------------------------------------------------
// onLButtonDown: Click handler
// IDA: pizza_onClick (0x43CFA1)
// ---------------------------------------------------------------------------
ZmbEventHandleResult ZoombiniPuzzlePizza::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// Sticky mouse: second click drops dragged snoid
	if (isDragging() && _vm->_state->getEnableStickyMouse()) {
		endDrag(absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	// Let base class handle Go/Map/Help buttons
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Guard conditions — only check puzzleActive; per-case guards follow
	// IDA: original has no global _introComplete guard; each case has its own
	if (!_puzzleActive)
		return ZmbEventHandleResult::kPassthrough;
	if (isDragging())
		return ZmbEventHandleResult::kPassthrough;

	// --- Check ingredient toggle clicks (on topping features) ---
	// IDA: pizza_onClick cases 5-12 (ingredient toggles)
	// Original guard: only !wUnk002C[28] (no pending exit callback)
	if (_questionRunnerPhase == kPhaseNone) {
		for (uint16 i = 0; i < _toppingCount; i++) {
			if (_toppingFeatures[i]) {
				ZmbDrawRecord *drawRecord = _toppingFeatures[i]->findDrawRecordAtPoint(absPos);
				if (drawRecord) {
					handleIngredientToggle(i);
					return ZmbEventHandleResult::kConsumed;
				}
			}
		}
	}

	// --- Check answer/submit area click ---
	// IDA: pizza_onClick case 4 / case 13
	// Original guard: !isDeliveryInProgress && !ambientAnimActive && !allOrdersReady
	//   && !allDeliveriesDone && !wUnk002C[35-37,41] && drawOnRegFlagArr[0]
	if (kAnswerClickRect.contains(absPos)) {
		if (_isDeliveryInProgress == 0 &&
			!_allOrdersReady && !_allDeliveriesDone &&
			_drawOnRegEnabled) {
			// IDA: if no answer zmb assigned, auto-pick one
			if (!_answerSnoid) {
				autoPickAnswerSnoid();
			}
			if (_answerSnoid) {
				handleSubmit();
			}
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
ZmbEventHandleResult ZoombiniPuzzlePizza::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
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
void ZoombiniPuzzlePizza::endDrag(const Common::Point &dropPos) {
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

			// Initialize the celebration animation system for this interaction
			_celebrationActive = true;
			_celebrationsPlayed = 0;
			_celebrationTarget = 3;
			_lastCelebrationFrame = getCurrentFrameCounter();

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
void ZoombiniPuzzlePizza::handleIngredientToggle(int16 ingredientIdx) {
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
int16 ZoombiniPuzzlePizza::classifyOrderType(int16 orderLine) const {
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
// Classify the current meal against the given order line and play
// the appropriate reaction animation.  Sets _pendingDeliverySlot for
// the non-exact-match path and _orderState for exact matches.
// @param orderLine Which order line (0=Arno, 1=Willa, 2=Shyler)
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::serveNextTopping(int16 orderLine) {
	// IDA: skip already-accepted orders
	if (_orderState[orderLine] >= 3)
		return;

	int16 resultType = classifyOrderType(orderLine);
	ZmbFeature *orderFeature = nullptr;
	uint16 scrbId = 0;
	FeaturePhase *phase = nullptr;
	bool setPendingDelivery = false;

	switch (orderLine) {
	case 0: // Arno (order base)
		orderFeature = _orderBaseFeature;
		phase = &_orderBasePhase;
		switch (resultType) {
		case 0: // One extra
			scrbId = 8006 + _anim0_oneCorrectCtr;
			_anim0_oneCorrectCtr = (_anim0_oneCorrectCtr + 1) % 2;
			_retryCounter = 0;
			_currentOrderType = 4;
			setPendingDelivery = true;
			break;
		case 1: // Partial subset
			scrbId = 8000 + _anim0_allWrongCtr;
			if (_anim0_allWrongCtr < 5)
				_anim0_allWrongCtr++;
			_deliverySlotType = 0;
			if (!_currentToppingType) {
				_currentToppingType = 1;
				_currentOrderType = 5;
			}
			_retryCounter = 0;
			setPendingDelivery = true;
			break;
		case 2: // Exact match
			scrbId = 8017 + _vm->_rnd->getRandomNumber(0, 2);
			_orderState[orderLine] = 2;
			_retryCounter++;
			break;
		case 3: // Dead code path
			scrbId = 8015 + _vm->_rnd->getRandomNumber(0, 1);
			_retryCounter = 0;
			_currentOrderType = 4;
			setPendingDelivery = true;
			break;
		case 4: // Multiple extras
			scrbId = 8008 + _anim0_multiNonWrongCtr;
			_anim0_multiNonWrongCtr = (_anim0_multiNonWrongCtr + 1) % 6;
			_retryCounter = 0;
			_currentOrderType = 4;
			setPendingDelivery = true;
			break;
		default:
			scrbId = 8000;
			setPendingDelivery = true;
			break;
		}
		break;

	case 1: // Willa (order 1)
		orderFeature = _order1Feature;
		phase = &_order1Phase;
		switch (resultType) {
		case 0:
			scrbId = 9000 + _anim1_oneCorrectCtr;
			_anim1_oneCorrectCtr = (_anim1_oneCorrectCtr + 1) % 5;
			_retryCounter = 0;
			if (!_currentToppingType)
				_currentOrderType = 4;
			setPendingDelivery = true;
			break;
		case 1:
			scrbId = 9021 + _anim1_allWrongCtr;
			if (_anim1_allWrongCtr < 4)
				_anim1_allWrongCtr++;
			_deliverySlotType = 1;
			if (!_currentToppingType) {
				_currentToppingType = 2;
				_currentOrderType = 6;
			}
			_retryCounter = 0;
			setPendingDelivery = true;
			break;
		case 2:
			scrbId = 9010 + _vm->_rnd->getRandomNumber(0, 6);
			_orderState[orderLine] = 2;
			_retryCounter++;
			break;
		case 3:
			scrbId = 9017 + _vm->_rnd->getRandomNumber(0, 1);
			_retryCounter = 0;
			_currentOrderType = 4;
			setPendingDelivery = true;
			break;
		case 4:
			scrbId = 9005 + _anim1_multiNonWrongCtr;
			_anim1_multiNonWrongCtr = (_anim1_multiNonWrongCtr + 1) % 5;
			_retryCounter = 0;
			if (!_currentToppingType)
				_currentOrderType = 4;
			setPendingDelivery = true;
			break;
		default:
			scrbId = 9000;
			setPendingDelivery = true;
			break;
		}
		break;

	case 2: // Shyler (order 2)
		orderFeature = _order2Feature;
		phase = &_order2Phase;
		switch (resultType) {
		case 0:
			scrbId = 10014 + _anim2_oneCorrectCtr;
			_anim2_oneCorrectCtr = (_anim2_oneCorrectCtr + 1) % 6;
			_retryCounter = 0;
			if (!_currentToppingType)
				_currentOrderType = 4;
			setPendingDelivery = true;
			break;
		case 1:
			scrbId = 10009 + _anim2_allWrongCtr;
			if (_anim2_allWrongCtr < 4)
				_anim2_allWrongCtr++;
			_deliverySlotType = 2;
			if (!_currentToppingType) {
				_currentToppingType = 3;
				_currentOrderType = 7;
			}
			_retryCounter = 0;
			setPendingDelivery = true;
			break;
		case 2:
			scrbId = 10023 + _vm->_rnd->getRandomNumber(0, 3);
			_orderState[orderLine] = 2;
			_retryCounter++;
			break;
		case 3:
			scrbId = 10027 + _vm->_rnd->getRandomNumber(0, 2);
			_retryCounter = 0;
			_currentOrderType = 4;
			setPendingDelivery = true;
			break;
		case 4:
			scrbId = 10020 + _anim2_multiNonWrongCtr;
			_anim2_multiNonWrongCtr = (_anim2_multiNonWrongCtr + 1) % 3;
			_retryCounter = 0;
			if (!_currentToppingType)
				_currentOrderType = 4;
			setPendingDelivery = true;
			break;
		default:
			scrbId = 10009;
			setPendingDelivery = true;
			break;
		}
		break;

	default:
		return;
	}

	if (orderFeature && scrbId) {
		loadScrbOntoFeature(orderFeature, scrbId);
		if (phase)
			*phase = kPhaseServeReaction;
		_currentServingLine = orderLine;

		// IDA: set pending delivery slot for non-exact results
		// Slot value = orderLine + 1 (1-based)
		if (setPendingDelivery)
			_pendingDeliverySlot = orderLine + 1;

		debugC(kZmbDebugPage, "Pizza: Serving order %d, result=%d (SCRB %d, pendingDelivery=%d)",
			   orderLine, resultType, scrbId, setPendingDelivery ? _pendingDeliverySlot : 0);
	}

	// IDA: at the end, check if all orders are ready (all active >= 2)
	bool allReady = true;
	for (int16 i = 0; i <= _extraToppingTiers; i++) {
		if (_orderState[i] < 2) {
			allReady = false;
			break;
		}
	}
	if (allReady) {
		_allOrdersReady = true;
		_celebrationTarget = 15; // IDA: pickerRunner.wUnk002C[13] - 1
	}
}

// ---------------------------------------------------------------------------
// evaluateDelivery: IDA 0x4403A4
// Called after all active order lines have been served for one delivery.
// Decrements remaining deliveries, determines correct/wrong status,
// and either takes the skip path or loads the delivery eval SCRB.
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::evaluateDelivery() {
	// IDA: pizza_pendingOrderCount = 0
	_pendingOrderCount = 0;

	// IDA: wasDeliveryCorrect = --remainingDeliveries >= 0
	_remainingDeliveries--;
	_wasDeliveryCorrect = (_remainingDeliveries >= 0) ? 1 : 0;

	// IDA: if (!remainingDeliveries) ++deliveryStreak
	if (_remainingDeliveries == 0)
		_deliveryStreak++;

	debugC(kZmbDebugPage, "Pizza: Evaluate delivery — remaining=%d, correct=%d, streak=%d",
		   _remainingDeliveries, _wasDeliveryCorrect, _deliveryStreak);

	// IDA: if (!deliveryStreak && wasDeliveryCorrect) → skip path
	if (!_deliveryStreak && _wasDeliveryCorrect) {
		// Skip the delivery eval animation — directly load delivery result
		animateAnswerZmb();
		_skipDeliveryFlag++;
		// IDA: slot 33 = 1000 → immediate trigger for loadDeliveryResultScrb
		loadDeliveryResultScrb();
		return;
	}

	// IDA: Else → load evaluation SCRB on the first active order feature
	// Uses the delivery callback for snoid SCRS playback
	ZmbFeature *evalFeature = nullptr;
	uint16 evalScrbId = 0;
	FeaturePhase *phase = nullptr;

	if (_orderState[0] == 1) {
		evalFeature = _orderBaseFeature;
		evalScrbId = 8022 + _wasDeliveryCorrect;
		phase = &_orderBasePhase;

		if (_deliveryStreak) {
			// IDA: pendingReplayFlag++ and register on reaction slot
			// When the reaction completes, handleOrderLineComplete sees
			// pendingReplayFlag and triggers loadDeliveryResultScrb
			_pendingReplayFlag++;
			*phase = kPhaseServeReaction;
		} else {
			*phase = kPhaseDeliveryEval;
		}
	} else if (_orderState[1] == 1) {
		evalFeature = _order1Feature;
		evalScrbId = 9028 + _wasDeliveryCorrect;
		phase = &_order1Phase;
		*phase = kPhaseDeliveryEval;
	} else if (_orderState[2] == 1) {
		evalFeature = _order2Feature;
		evalScrbId = 10032 + _wasDeliveryCorrect;
		phase = &_order2Phase;
		*phase = kPhaseDeliveryEval;
	}

	if (evalFeature && evalScrbId) {
		loadScrbOntoFeature(evalFeature, evalScrbId);
		_deliveryCallbackActive = true;
	}

	// IDA: Clear streak and skip flag for non-skip path
	_skipDeliveryFlag = 0;
	_deliveryStreak = 0;

	debugC(kZmbDebugPage, "Pizza: Delivery eval SCRB %d loaded on order feature", evalScrbId);
}

// ---------------------------------------------------------------------------
// loadDeliveryResultScrb: IDA 0x43FEA0 (pizza_onToppingDelivered)
// Called after evaluation to load delivery result SCRBs (8020/9026/10030).
// These show the pizza being delivered to the troll.
// In the skip path, this is called directly; otherwise it's called when
// the eval SCRB completes.
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::loadDeliveryResultScrb() {
	// IDA: Record bitmask in history (from pizza_onToppingDelivered_43FEA0)
	// Must happen here, NOT in onToppingDelivered (overlay completion),
	// otherwise the first submission would self-match in checkToppingMaskMatch.
	_toppingMaskHistoryIdx++;
	if (_toppingMaskHistoryIdx < 28) {
		_toppingMaskHistory[_toppingMaskHistoryIdx] = packToppingBitmask();
	}

	// IDA: if currentToppingType: override pendingDeliverySlot
	if (_currentToppingType)
		_pendingDeliverySlot = _currentToppingType;

	// IDA: --pendingDeliverySlot (convert from 1-based to 0-based)
	_pendingDeliverySlot--;

	debugC(kZmbDebugPage, "Pizza: loadDeliveryResultScrb (deliverySlot=%d)", _pendingDeliverySlot);

	ZmbFeature *orderFeature = nullptr;
	uint16 scrbId = 0;

	if (_pendingDeliverySlot <= 0) {
		// IDA: Order 0 — SCRB 8020 on orderBase (slot 38)
		orderFeature = _orderBaseFeature;
		scrbId = 8020;
		_orderBasePhase = kPhaseDeliveryResult;
	} else if (_pendingDeliverySlot == 1) {
		// IDA: Order 1 — check pendingReplayFlag
		if (_pendingReplayFlag) {
			// Skip this delivery slot, trigger evaluate again
			_pendingDeliverySlot++;
			evaluateDelivery();
			return;
		}
		orderFeature = _order1Feature;
		scrbId = 9026;
		_order1Phase = kPhaseDeliveryResult;
	} else if (_pendingDeliverySlot == 2) {
		// IDA: Order 2 — SCRB 10030 on order2 (slot 38)
		orderFeature = _order2Feature;
		scrbId = 10030;
		_order2Phase = kPhaseDeliveryResult;
	}

	if (orderFeature && scrbId) {
		loadScrbOntoFeature(orderFeature, scrbId);
	}

	// IDA: Update deliverySlotType
	_deliverySlotType = (_pendingDeliverySlot > 0) ? _pendingDeliverySlot : 0;
	_pendingDeliverySlot = 0;
}

// ---------------------------------------------------------------------------
// advanceToNextDeliverySlot: IDA 0x4409DA
// Move to the next zoombini for delivery.
// Resets delivery state, handles the answer snoid, and checks completion.
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::advanceToNextDeliverySlot() {
	// IDA: Guard conditions
	if (_allDeliveriesDone || _allOrdersReady)
		return;

	_isDeliveryInProgress = 0;
	_retryCounter = 0;
	_toppingMaskHistoryIdx = -1;
	memset(_toppingMaskHistory, 0, sizeof(_toppingMaskHistory));
	_currentServingLine = -1;
	_deliveryCallbackActive = false;
	_needsSlotAdvance = false;
	_pendingReplayFlag = 0;
	_skipDeliveryFlag = 0;
	_hasMaskMatch = 0;
	_pendingOrderCount = 0;
	_currentToppingType = 0;
	_currentOrderType = 0;
	_pendingDeliverySlot = 0;

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
	_celebrationActive = false;

	// Reset phase tracking
	_orderBasePhase = kPhaseNone;
	_order1Phase = kPhaseNone;
	_order2Phase = kPhaseNone;
	_overlayPhase = kPhaseNone;
	_questionRunnerPhase = kPhaseNone;

	// Refresh the answer display (empty preview)
	registerAnswerDisplay();

	// Check if all deliveries are done
	if (_allOrdersReady || _remainingDeliveries <= 0) {
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
void ZoombiniPuzzlePizza::advanceIntroSequence() {
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
		_celebrationActive = true;
		_celebrationTarget = 2;
		_lastCelebrationFrame = getCurrentFrameCounter();
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
void ZoombiniPuzzlePizza::triggerOrderFeatureAmbientAnim() {
	// IDA: After ambient SCRB finishes, wUnk002C[40] handler fires to clear
	// isDeliveryInProgress and call advanceToNextDeliverySlot (auto-pick snoid).
	// Set kPhasePostIntroAmbient so onFeatureAnimEvent can trigger that flow.
	if (_difficultyLevel == 0) {
		loadScrbOntoFeature(_orderBaseFeature, 8014);
		_orderBasePhase = kPhasePostIntroAmbient;
	} else if (_difficultyLevel == 1) {
		int16 variant = _vm->_rnd->getRandomNumber(1); // 0 or 1
		loadScrbOntoFeature(_order1Feature, 9019 + variant);
		_order1Phase = kPhasePostIntroAmbient;
	} else {
		int16 variant = _vm->_rnd->getRandomNumber(7); // 0-7
		loadScrbOntoFeature(_order2Feature, 10001 + variant);
		_order2Phase = kPhasePostIntroAmbient;
	}
}

// ---------------------------------------------------------------------------
// packToppingBitmask: IDA 0x43F794
// Pack current ingredient flags into a single byte
// ---------------------------------------------------------------------------
uint8 ZoombiniPuzzlePizza::packToppingBitmask() const {
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
bool ZoombiniPuzzlePizza::checkToppingMaskMatch() const {
	uint8 currentMask = packToppingBitmask();
	for (int16 i = 0; i <= _toppingMaskHistoryIdx; i++) {
		if (_toppingMaskHistory[i] == currentMask)
			return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
// handleSubmit: IDA: pizza_handleIngredientToggle case 4 + onClick case 4/13
// Called when player clicks submit in the answer area.
// Starts the delivery cycle: answer display → exit callback → overlay →
// classify & serve → evaluate → delivery callback → advance
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::handleSubmit() {
	// IDA: pizza_onClick case 4/13
	// ++pizza_isDeliveryInProgress
	_isDeliveryInProgress++;

	// Snapshot the current meal
	// IDA: memcpy(word_4B0DAC, &word_4B0D9C, 0x10u)
	for (int16 i = 0; i < 8; i++) {
		_mealSnapshot[i] = _ingredientFlags[i];
		_currentMeal[i] = _ingredientFlags[i];
	}

	// IDA: if (!allDeliveriesDone) handleIngredientToggle(4)
	if (_allDeliveriesDone)
		return;

	// IDA: handleIngredientToggle case 4
	// Load the answer display SCRB (7057 at level 0, 7058 at level 1+)
	uint16 answerScrbId = (_difficultyLevel == 0) ? 7057 : 7058;
	loadScrbOntoFeature(_drawOnRegFeature, answerScrbId);

	// Load SCRB 7066 on the question runner to start the exit callback chain
	loadScrbOntoFeature(_questionRunnerFeature, 7066);
	_questionRunnerPhase = kPhaseExitCallback;

	debugC(kZmbDebugPage, "Pizza: Submit — starting delivery cycle");
}

// ---------------------------------------------------------------------------
// handleZmbExitEvent: IDA 0x43F3E6
// Handles animation events from SCRB 7066 on the question runner.
// Event 32: Initial overlay setup
// Event 60: Snoid trait reveal
// Event -1: Delivery overlay and classify
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::handleZmbExitEvent(ZmbFeature *feature, int16 eventCode) {
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
		// IDA: Load delivery overlay and play snoid SCRS
		// Determine active order line for this delivery
		_questionRunnerPhase = kPhaseNone;

		int16 traitIdx = 0;
		if (_answerSnoid) {
			traitIdx = (_answerSnoid->_trait._head) % 5;
		}

		// IDA: SCRS and overlay SCRB depend on which order is active
		uint16 scrsId = 0;
		uint16 overlayScrbId = 0;
		if (_orderState[0] == 1) {
			scrsId = 13005 + traitIdx;
			overlayScrbId = 12001 + traitIdx;
			_deliverySlotType = 0;
		} else if (_orderState[1] == 1) {
			scrsId = 13010 + traitIdx;
			overlayScrbId = 12006 + traitIdx;
			_deliverySlotType = 1;
		} else {
			scrsId = 13015 + traitIdx;
			overlayScrbId = 12011 + traitIdx;
			_deliverySlotType = 2;
		}

		// Play exit SCRS on the answer snoid
		if (_answerSnoid) {
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
								 ZmbResource(ZmbArchiveKind::kPage, scrsId));
			if (scrsStream) {
				_answerSnoid->startScrsPlayback(scrsStream, false, false);
			}
		}

		// Load delivery overlay SCRB
		if (_toppingOverlayFeature) {
			loadScrbOntoFeature(_toppingOverlayFeature, overlayScrbId);
			_overlayPhase = kPhaseToppingDelivery;
		}

		// IDA: calls registerAnswerDisplay() to refresh the preview
		registerAnswerDisplay();

		debugC(kZmbDebugPage, "Pizza: Exit callback event -1 — SCRS %d, overlay SCRB %d",
			   scrsId, overlayScrbId);
		break;
	}

	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// handleZmbDeliveryEvent: IDA 0x44005D (pizza_zmbDeliveryCallback)
// Handles events during the delivery evaluation animation.
// Event 61: play SCRS on snoid + SFX 8040
// Event -1: if wrong → clear snoid render, increment punishment;
//           if correct → walk animation, advance flag
// Event 0: toggle frame visibility, handle pending anim shape
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::handleZmbDeliveryEvent(ZmbFeature *feature, int16 eventCode) {
	if (eventCode == 61) {
		// IDA: event 61 — play snoid SCRS based on delivery correctness
		if (_skipDeliveryFlag) {
			_skipDeliveryFlag = 0;
			return;
		}

		// Determine SCRS ID and initial position based on active order
		uint16 scrsId = 0;
		if (_orderState[0] == 1) {
			scrsId = 14000 + _wasDeliveryCorrect;
		} else if (_orderState[1] == 1) {
			scrsId = 14002 + _wasDeliveryCorrect;
		} else {
			scrsId = 14004 + _wasDeliveryCorrect;
		}

		// Play SCRS on answer snoid
		if (_answerSnoid) {
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
								 ZmbResource(ZmbArchiveKind::kPage, scrsId));
			if (scrsStream) {
				bool isReject = (_wasDeliveryCorrect == 0);
				_answerSnoid->startScrsPlayback(scrsStream, false, isReject);
			}
		}

		// Play delivery SFX
		_vm->_sound->playZmbSound(
			ZmbResource(ZmbArchiveKind::kPage, 8040),
			Audio::Mixer::kSFXSoundType);

		_deliveryCallbackActive = true;

		debugC(kZmbDebugPage, "Pizza: Delivery callback event 61 — SCRS %d", scrsId);
	} else if (eventCode == kZmbAnimEventM1_End) {
		// IDA: event -1 — delivery evaluation SCRB complete
		// Original: delivery callback -1 handles snoid correct/wrong flags,
		// then slot 33 completion triggers loadDeliveryResultScrb.
		// In our callback-based system, both happen here. The result SCRB
		// uses kPhaseDeliveryResult (not kPhaseDeliveryEval) to avoid
		// re-entering this handler when the result SCRB completes.
		_deliveryCallbackActive = false;

		if (!_wasDeliveryCorrect) {
			_punishmentCount++;
		}

		// Load delivery result SCRB (8020/9026/10030) — tracked as kPhaseDeliveryResult
		loadDeliveryResultScrb();

		debugC(kZmbDebugPage, "Pizza: Delivery callback event -1 — punishment=%d",
			   _punishmentCount);
	} else if (eventCode == 0) {
		// IDA: event 0 — toggle frame visibility
		// Handled implicitly by the animation system
	}
}

// ---------------------------------------------------------------------------
// handleOrderLineComplete: IDA slot 35/36/37 handlers in onFrameUpdate
// Called when an order line's reaction animation finishes (event -1).
// Handles state 2→3 accept transitions, serve chaining, and evaluate trigger.
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::handleOrderLineComplete(int16 orderLine) {
	debugC(kZmbDebugPage, "Pizza: Order line %d reaction complete (state=%d)",
		   orderLine, _orderState[orderLine]);

	// IDA: if pendingReplayFlag → clear it, trigger immediate evaluate
	if (_pendingReplayFlag) {
		_pendingReplayFlag = 0;
		// IDA: slot 33 = 1000 → immediate onToppingDelivered (loads delivery result SCRB)
		loadDeliveryResultScrb();
		return;
	}

	// IDA: if orderState == 3 (already accepted) → spawnAnswerZmb
	if (_orderState[orderLine] == 3) {
		spawnAnswerZmb();
		return;
	}

	// IDA: if orderState == 2 (just matched this round) → accept transition
	if (_orderState[orderLine] == 2) {
		// Free the overlay runner
		_overlayPhase = kPhaseNone;

		// Load accept SCRB: 8021 / 9027 / 10031
		ZmbFeature *orderFeature = nullptr;
		uint16 acceptScrbId = 0;
		FeaturePhase *phase = nullptr;
		switch (orderLine) {
		case 0:
			orderFeature = _orderBaseFeature;
			acceptScrbId = 8021;
			phase = &_orderBasePhase;
			break;
		case 1:
			orderFeature = _order1Feature;
			acceptScrbId = 9027;
			phase = &_order1Phase;
			break;
		case 2:
			orderFeature = _order2Feature;
			acceptScrbId = 10031;
			phase = &_order2Phase;
			break;
		default:
			return;
		}

		loadScrbOntoFeature(orderFeature, acceptScrbId);
		_orderState[orderLine] = 3;

		// IDA: register topping overlay for the accepted order
		registerToppingRunner();

		// IDA: set phase to AcceptTransition so next completion
		// triggers the "state==3" branch above → spawnAnswerZmb
		if (phase)
			*phase = kPhaseServeReaction;

		_questionsAnswered++;
		animateAnswerZmb();
		_pendingOrderCount++;
		_pendingDeliverySlot = 0;

		debugC(kZmbDebugPage, "Pizza: Order %d accepted (SCRB %d, questions=%d)",
			   orderLine, acceptScrbId, _questionsAnswered);
		return;
	}

	// IDA: Non-match completion — check what to do next
	if (_allOrdersReady) {
		setupQuestionRunners();
		return;
	}

	if (_hasMaskMatch) {
		_retryCounter = 0;
		return;
	}

	// Chain to next active, non-accepted order line
	bool foundNext = false;
	if (orderLine == 0) {
		if (_orderState[1] == 1 && !_pendingOrderCount) {
			serveNextTopping(1);
			foundNext = true;
		} else if (_orderState[2] == 1 && !_pendingOrderCount) {
			serveNextTopping(2);
			foundNext = true;
		}
	} else if (orderLine == 1) {
		if (_orderState[2] == 1 && !_pendingOrderCount) {
			serveNextTopping(2);
			foundNext = true;
		}
	}

	if (!foundNext) {
		// All lines have been served — trigger evaluateDelivery
		// IDA: slot 34 was set by serveNextTopping, frame update checks it
		_pendingOrderCount = 0;
		if (_pendingDeliverySlot > 0) {
			evaluateDelivery();
		}
	}

	_retryCounter = 0;
}

// ---------------------------------------------------------------------------
// onToppingDelivered: IDA 0x43C4A8 (slot 30 handler in onFrameUpdate)
// Called when the delivery overlay animation completes.
// Records topping bitmask, checks for duplicate combinations, then either
// calls placeTopping (for repeated combos) or serveNextTopping (new combo).
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::onToppingDelivered() {
	debugC(kZmbDebugPage, "Pizza: Delivery overlay complete — classifying");

	// IDA: pizza_currentToppingType = 0
	_currentToppingType = 0;

	// IDA: Check mask match BEFORE recording (recording happens in loadDeliveryResultScrb)
	_hasMaskMatch = checkToppingMaskMatch() ? 1 : 0;

	if (_hasMaskMatch) {
		// IDA: Route via _currentOrderType set by previous delivery's serveNextTopping
		if (_currentOrderType == 5) {
			placeTopping(1, 0); // order0 partial subset
		} else if (_currentOrderType == 6) {
			placeTopping(1, 1); // order1 partial subset
		} else if (_currentOrderType == 7) {
			placeTopping(1, 2); // order2 partial subset
		} else {
			placeTopping(0, 0); // dead path / auto-select
		}
	} else {
		// IDA: New combination — serve to first active order line
		if (_orderState[0] == 1) {
			serveNextTopping(0);
		} else if (_orderState[1] == 1 && !_pendingOrderCount) {
			serveNextTopping(1);
		} else if (_orderState[2] == 1 && !_pendingOrderCount) {
			serveNextTopping(2);
		}
	}
}

// ---------------------------------------------------------------------------
// registerAnswerDisplay: IDA 0x43D615
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::registerAnswerDisplay() {
	uint16 scrbId = 7001 + _difficultyLevel;
	loadScrbOntoFeature(_drawOnRegFeature, scrbId);
	debugC(kZmbDebugPage, "Pizza: Answer display registered (SCRB %d)", scrbId);
}

// ---------------------------------------------------------------------------
// answerDisplay_preRenderShape: IDA pizza_filterHotspotsBySlotFlags_43D681
//
// Pre-render shape callback for the answer display feature.
// Filters out hotspot entries whose shapes correspond to unselected toppings.
// Shape IDs 57-61 map to ingredient flags 0-4; 67-69 map to flags 5-7.
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::answerDisplay_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	for (int i = (int)hotspots.size() - 1; i >= 0; --i) {
		int16 shapeIdx = hotspots[i]._shapeIdx;
		int flagIdx = -1;
		if (shapeIdx >= 57 && shapeIdx <= 61)
			flagIdx = shapeIdx - 57; // maps 57-61 → flags 0-4
		else if (shapeIdx >= 67 && shapeIdx <= 69)
			flagIdx = shapeIdx - 62; // maps 67-69 → flags 5-7

		if (flagIdx >= 0 && !_ingredientFlags[flagIdx])
			hotspots.remove_at(i);
	}
}

// ---------------------------------------------------------------------------
// spawnAnswerZmb: IDA 0x440D32
// If not busy, spawn the next answer-zone Zoombini with SCRB 7067/7068.
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::spawnAnswerZmb() {
	// IDA: Guard conditions — skip if question runners active, pending answer
	// display still playing (wUnk002C[40]), or all orders already matched
	if (_questionRunnerPhase != kPhaseNone || _drawOnRegPhase == kPhaseSpawnAnswer ||
		_allOrdersReady || _allDeliveriesDone)
		return;

	// IDA: if delivery index < max count → spawn; else → mark done
	if (!_allDeliveriesDone) {
		uint16 scrbId = (_difficultyLevel == 0) ? 7067 : 7068;
		loadScrbOntoFeature(_drawOnRegFeature, scrbId);
		// IDA: wUnk002C[40] = scrb_registerHotspotGroup(…) — track
		// answer display SCRB completion to unlock delivery button
		_drawOnRegPhase = kPhaseSpawnAnswer;
		debugC(kZmbDebugPage, "Pizza: Spawn answer zmb (SCRB %d)", scrbId);
	} else {
		_allDeliveriesDone = true;
	}
}

// ---------------------------------------------------------------------------
// autoPickAnswerSnoid: IDA 0x4409DA (advanceToNextDeliverySlot portion)
// Picks the next available zoombini from the pack and walks it to the answer
// area.  Called after the post-intro ambient troll animation completes,
// mirroring the original's wUnk002C[40] handler which clears
// isDeliveryInProgress and calls advanceToNextDeliverySlot.
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::autoPickAnswerSnoid() {
	if (_allDeliveriesDone || _allOrdersReady)
		return;

	// IDA: ++MEMORY[0x4B0CA6]. Find the next occupied snoid in pack order.
	_deliveryIndex++;
	int16 idx = 0;
	ZmbSnoid *picked = nullptr;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); it++) {
		ZmbSnoid *s = *it;
		if (!s->_packIsOccupied)
			continue;
		if (s == _answerSnoid)
			continue;
		if (idx >= _deliveryIndex) {
			picked = s;
			break;
		}
		idx++;
	}

	if (!picked) {
		// No more zoombinis to serve
		_allDeliveriesDone = true;
		setGoButtonsEnabled(true);
		debugC(kZmbDebugPage, "Pizza: No more zoombinis — all deliveries done");
		return;
	}

	_answerSnoid = picked;
	_answerZmbPackIdx = picked->getId() - 10000;

	// IDA 0x440AB6: animateZoombini(0, 7u, core) sets DEPARTING state,
	// which initialises route + dynamic velocity, then walks via state 112.
	picked->initWalkToTarget(kAnswerDisplayPosition);

	// Load answer display SCRB (7067/7068) for the newly picked snoid
	spawnAnswerZmb();

	debugC(kZmbDebugPage, "Pizza: Auto-picked snoid %d for answer area", _answerZmbPackIdx);
}

// ---------------------------------------------------------------------------
// animateAnswerZmb: IDA 0x4402EC
// Find the answer snoid, set speed=6, start walk right animation (anim 1),
// link to the first active (not yet accepted) order feature.
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::animateAnswerZmb() {
	if (!_answerSnoid)
		return;

	// IDA 0x4402EC: v0[10] = 6 (dFrameInterval=6, normal tick rate);
	// animateZoombini(0, 1u, core) enters state 1 (kSnoidAnimTurnRight),
	// which does a brief turn-around then transitions to idle — NO position
	// movement. The snoid is already at the answer seat.
	_answerSnoid->setAnimState(kSnoidAnimTurnRight);

	// IDA: if questionsAnswered > 0, clear it (skip linking)
	if (_questionsAnswered) {
		_questionsAnswered = 0;
	} else if (_orderState[0] == 1) {
		// IDA: runner_linkRelativeToParent(orderBase, 1, answerRunner)
		// In ScummVM: position the answer snoid near the order0 feature
	} else if (_orderState[1] == 1) {
		// IDA: runner_linkRelativeToParent(order1Runner, 1, answerRunner)
	} else if (_orderState[2] == 1) {
		// IDA: runner_linkRelativeToParent(order2Runner, 1, answerRunner)
	}

	// IDA: scrb_registerHotspotGroup → slot 40
	// Set up the answer display for the current answer snoid
	registerAnswerDisplay();

	debugC(kZmbDebugPage, "Pizza: animateAnswerZmb — walk right, speed=6");
}

// ---------------------------------------------------------------------------
// setupQuestionRunners: IDA 0x43F5CF
// Set up question SCRBs based on which order lines are active.
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::setupQuestionRunners() {
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
// For repeat topping combinations (mask match), decide which order feature
// gets the topping placement animation.
// @param mode  1 = allWrong/partial (use hintSlot directly),
//              0 or 4 = auto-select among active orders
// @param hintSlot Target order slot (0-2) when mode==1
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::placeTopping(int16 mode, int16 hintSlot) {
	// IDA: Complex slot selection logic based on active orders
	_retryCounter = 0;

	int16 targetSlot = hintSlot;
	if (!mode || mode == 4) {
		// IDA: Auto-select based on which orders are active
		if (_orderState[0] == 1) {
			if (_orderState[1] != 1 && _orderState[2] != 1) {
				targetSlot = 0;
			} else if (_orderState[1] == 1 && _orderState[2] != 1) {
				targetSlot = _vm->_rnd->getRandomNumber(0, 1);
			} else if (_orderState[1] != 1 && _orderState[2] == 1) {
				targetSlot = 2 * _vm->_rnd->getRandomNumber(0, 1);
			} else {
				// All three active — IDA picks 0 or 1
				targetSlot = _vm->_rnd->getRandomNumber(0, 1);
			}
		} else if (_orderState[1] == 1) {
			if (_orderState[2] == 1)
				targetSlot = _vm->_rnd->getRandomNumber(0, 1) + 1;
			else
				targetSlot = 1;
		} else {
			targetSlot = 2;
		}
	}

	ZmbFeature *orderFeature = nullptr;
	uint16 scrbId = 0;
	FeaturePhase *phase = nullptr;

	switch (targetSlot) {
	case 0:
		orderFeature = _orderBaseFeature;
		phase = &_orderBasePhase;
		_pendingDeliverySlot = 1;
		if (mode == 1) {
			scrbId = 8000 + _anim0_allWrongCtr;
			_currentToppingType = 1;
			_currentOrderType = 5;
		} else {
			scrbId = 8015 + _vm->_rnd->getRandomNumber(0, 1);
			_currentOrderType = 4;
		}
		break;
	case 1:
		orderFeature = _order1Feature;
		phase = &_order1Phase;
		_pendingDeliverySlot = 2;
		if (mode == 1) {
			scrbId = 9021 + _anim1_allWrongCtr;
			_currentToppingType = 2;
			_currentOrderType = 6;
		} else {
			scrbId = 9017 + _vm->_rnd->getRandomNumber(0, 1);
			_currentOrderType = 4;
		}
		break;
	case 2:
		orderFeature = _order2Feature;
		phase = &_order2Phase;
		_pendingDeliverySlot = 3;
		if (mode == 1) {
			scrbId = 10009 + _anim2_allWrongCtr;
			_currentToppingType = 3;
			_currentOrderType = 7;
		} else {
			scrbId = 10027 + _vm->_rnd->getRandomNumber(0, 1);
			_currentOrderType = 4;
		}
		break;
	default:
		return;
	}

	if (orderFeature && scrbId) {
		loadScrbOntoFeature(orderFeature, scrbId);
		if (phase)
			*phase = kPhaseServeReaction;
	}

	debugC(kZmbDebugPage, "Pizza: Place topping on slot %d (SCRB=%d, mode=%d)",
		   targetSlot, scrbId, mode);
}

// ---------------------------------------------------------------------------
// playSFXForOrder: IDA 0x441104
// Called from pizza_init only — plays ambient topping SFX during puzzle load.
// Original uses synchronous waits between sounds; ScummVM fires them
// non-blocking since they're ambient SFX.
//
// IDA case mapping (sfxVariant 0-4):
//   case 0: SND 15005, wait 60 frames, SND 15006
//   case 1: SND 15000, fallback SND 15001
//   case 2: SND 15002
//   case 3: SND 15003, fallback SND 15004
//   case 4: SND 15003 + 15004, wait 20, SND 15005, wait 60, SND 15006
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::playSFXForOrder(int16 sfxVariant) {
	if (sfxVariant > 4)
		return;

	switch (sfxVariant) {
	case 0:
		_vm->_sound->playZmbSound(
			ZmbResource(ZmbArchiveKind::kPage, 15005),
			Audio::Mixer::kSFXSoundType);
		_vm->_sound->playZmbSound(
			ZmbResource(ZmbArchiveKind::kPage, 15006),
			Audio::Mixer::kSFXSoundType);
		break;
	case 1:
		_vm->_sound->playZmbSound(
			ZmbResource(ZmbArchiveKind::kPage, 15000),
			Audio::Mixer::kSFXSoundType);
		break;
	case 2:
		_vm->_sound->playZmbSound(
			ZmbResource(ZmbArchiveKind::kPage, 15002),
			Audio::Mixer::kSFXSoundType);
		break;
	case 3:
		_vm->_sound->playZmbSound(
			ZmbResource(ZmbArchiveKind::kPage, 15003),
			Audio::Mixer::kSFXSoundType);
		break;
	case 4:
		_vm->_sound->playZmbSound(
			ZmbResource(ZmbArchiveKind::kPage, 15003),
			Audio::Mixer::kSFXSoundType);
		_vm->_sound->playZmbSound(
			ZmbResource(ZmbArchiveKind::kPage, 15005),
			Audio::Mixer::kSFXSoundType);
		break;
	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// linkToppingRunners: IDA 0x441286
// Original engine re-orders the feature runner linked list for Z-ordering:
//   overlayBase → toppingOverlay (if exists)
//   toppingOverlay → orderBase (if order0 accepted)
//   overlayBase → answerRunner → active orders (chained)
// ScummVM uses registration-order for LOOP_ANIM features in the render list,
// so explicit Z-order linking is not needed. See caves.cpp for precedent.
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::linkToppingRunners() {
	// NOTE: Original engine called runner_linkRelativeToParent for Z-order.
	// ScummVM uses registration order for LOOP_ANIM overlay features.
}

// ---------------------------------------------------------------------------
// registerToppingRunner: IDA 0x440558
// Creates a new SCRB overlay feature showing the accepted topping state on
// a pizza troll.  Called after an order line's serve reaction completes.
//
// Based on _currentOrderType:
//   Type 4: generic overlay, SCRB 12025+counter (wraps at 16, skip 13)
//   Type 5: order 0, SCRB 12016+counter (wraps at 3)
//   Type 6: order 1, SCRB 12019+counter (wraps at 3)
//   Type 7: order 2, SCRB 12022+counter (wraps at 3)
//
// Sets preRenderShape filter to show only active ingredients.
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::registerToppingRunner() {
	if (_toppingRunnerSlotIdx >= 27)
		return; // Safety: don't overflow the 28-slot array

	_toppingRunnerSlotIdx++;

	// IDA: record mask and order type in slot
	ToppingRunnerSlot &slot = _toppingRunnerSlots[_toppingRunnerSlotIdx];
	slot.mask = (_toppingMaskHistoryIdx >= 0) ? _toppingMaskHistory[_toppingMaskHistoryIdx] : 0;
	slot.orderType = _currentOrderType;

	uint16 scrbId = 0;

	switch (_currentOrderType) {
	case 4: {
		// IDA: Generic topping — SCRB 12025+counter, wraps at 16, skip 13
		_toppingRunnerCtrMain++;
		if (_toppingRunnerCtrMain >= 16) {
			_toppingRunnerCtrMain = 0;
			_toppingRunnersWrapped = true;
		}
		if (_toppingRunnerCtrMain == 13)
			_toppingRunnerCtrMain = 14;
		scrbId = 12025 + _toppingRunnerCtrMain;

		if (_toppingRunnersWrapped) {
			// IDA: Reuse existing runner — find the slot with matching SCRB, reload it
			for (int16 i = 0; i < _toppingRunnerSlotIdx; i++) {
				if (_toppingRunnerSlots[i].scrbId == scrbId && _toppingRunnerSlots[i].feature) {
					loadScrbOntoFeature(_toppingRunnerSlots[i].feature, scrbId);
					slot.feature = _toppingRunnerSlots[i].feature;
					slot.scrbId = scrbId;
					linkToppingRunners();
					return;
				}
			}
		}
		break;
	}
	case 5: {
		// IDA: Order 0 topping — SCRB 12016+counter (wraps at 3)
		_toppingRunnerCtr0++;
		if (_toppingRunnerCtr0 > 2)
			_toppingRunnerCtr0 = 0;
		scrbId = 12016 + _toppingRunnerCtr0;
		break;
	}
	case 6: {
		// IDA: Order 1 topping — SCRB 12019+counter (wraps at 3)
		_toppingRunnerCtr1++;
		if (_toppingRunnerCtr1 > 2)
			_toppingRunnerCtr1 = 0;
		scrbId = 12019 + _toppingRunnerCtr1;
		break;
	}
	case 7: {
		// IDA: Order 2 topping — SCRB 12022+counter (wraps at 3)
		_toppingRunnerCtr2++;
		if (_toppingRunnerCtr2 > 2)
			_toppingRunnerCtr2 = 0;
		scrbId = 12022 + _toppingRunnerCtr2;
		break;
	}
	default:
		return;
	}

	// Create a new overlay feature with a unique ID
	uint16 featureId = _nextDynamicFeatureId++;
	ZmbFeature::EventHooks hooks;
	hooks.setPreRenderShapeFunc(
		reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(
			&ZoombiniPuzzlePizza::toppingRunner_preRenderShape));

	ZmbFeature *newFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 12000), featureId, 6,
		ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE |
			ZmbFeature::FLAG_04000000_OVERLAY,
		hooks);

	if (newFeature) {
		// Load the actual SCRB data for this topping type
		loadScrbOntoFeature(newFeature, scrbId);

		slot.feature = newFeature;
		slot.scrbId = scrbId;

		// Store in per-order slot arrays
		switch (_currentOrderType) {
		case 5:
			_toppingRunnerOrder0Slots[_toppingRunnerCtr0] = newFeature;
			break;
		case 6:
			_toppingRunnerOrder1Slots[_toppingRunnerCtr1] = newFeature;
			break;
		case 7:
			_toppingRunnerOrder2Slots[_toppingRunnerCtr2] = newFeature;
			break;
		default:
			break;
		}
	}

	linkToppingRunners();

	debugC(kZmbDebugPage, "Pizza: registerToppingRunner — type=%d, SCRB=%d, slotIdx=%d",
		   _currentOrderType, scrbId, _toppingRunnerSlotIdx);
}

// ---------------------------------------------------------------------------
// toppingRunner_preRenderShape: IDA 0x43DCDD (pizza_filterHotspotsByActiveIngredients)
// Pre-render callback for topping runner overlay features.
// Filters hotspot shapes by ingredient flags. Shape groups of 4:
//   5-8  → _ingredientFlags[4]    21-24 → _ingredientFlags[0]
//   9-12 → _ingredientFlags[3]    25-28 → always (diff>=1)
//   13-16→ _ingredientFlags[2]    29-32 → _ingredientFlags[7] && diff==3
//   17-20→ _ingredientFlags[1]    33-36 → _ingredientFlags[6] && diff>=1
//                                 37-40 → _ingredientFlags[5] && diff>=1
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::toppingRunner_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	for (int i = (int)hotspots.size() - 1; i >= 0; --i) {
		int16 shapeIdx = hotspots[i]._shapeIdx;
		bool keep = true;

		if (shapeIdx >= 5 && shapeIdx <= 8) {
			keep = _ingredientFlags[4] != 0;
		} else if (shapeIdx >= 9 && shapeIdx <= 12) {
			keep = _ingredientFlags[3] != 0;
		} else if (shapeIdx >= 13 && shapeIdx <= 16) {
			keep = _ingredientFlags[2] != 0;
		} else if (shapeIdx >= 17 && shapeIdx <= 20) {
			keep = _ingredientFlags[1] != 0;
		} else if (shapeIdx >= 21 && shapeIdx <= 24) {
			keep = _ingredientFlags[0] != 0;
		} else if (shapeIdx >= 25 && shapeIdx <= 28) {
			keep = _difficultyLevel >= 1;
		} else if (shapeIdx >= 29 && shapeIdx <= 32) {
			keep = (_ingredientFlags[7] != 0) && (_difficultyLevel == 3);
		} else if (shapeIdx >= 33 && shapeIdx <= 36) {
			keep = (_ingredientFlags[6] != 0) && (_difficultyLevel >= 1);
		} else if (shapeIdx >= 37 && shapeIdx <= 40) {
			keep = (_ingredientFlags[5] != 0) && (_difficultyLevel >= 1);
		}

		if (!keep)
			hotspots.remove_at(i);
	}
}

// ---------------------------------------------------------------------------
// reloadScrbAnimation: Helper to swap SCRB on a feature
// ---------------------------------------------------------------------------
void ZoombiniPuzzlePizza::reloadScrbAnimation(ZmbFeature *feature, uint16 scrbId) {
	if (feature) {
		loadScrbOntoFeature(feature, scrbId);
	}
}

// ---------------------------------------------------------------------------
// getTraitIndexForOrder: Get trait-based index for SCRS/overlay selection
// Returns a value 0-4 based on the answer snoid's traits.
// ---------------------------------------------------------------------------
int16 ZoombiniPuzzlePizza::getTraitIndexForOrder(int16 orderSlot) const {
	if (!_answerSnoid)
		return 0;

	// IDA: Uses zoombini trait fields to determine topping type
	// The trait index is modulo 5 of a trait field
	return (_answerSnoid->_trait._head) % 5;
}

} // End of namespace Mohawk
