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

#include "common/rect.h"

#include "mohawk/mohawk.h"
#include "mohawk/resource.h"
#include "mohawk/sound.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_pages/transition_xfer.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"
#include "mohawk/zoombini_text.h"

namespace Mohawk {

ZoombiniTransitionXfer::ZoombiniTransitionXfer(MohawkEngine_Zoombini *vm) : ZoombiniTransition(vm, ZoombiniPageType::kXfer) {
	_useFadeEffect = true;
}

ZoombiniTransitionXfer::~ZoombiniTransitionXfer() {
}

void ZoombiniTransitionXfer::open() {
	openArchive(ZMB_MHK_XFER);
}

void ZoombiniTransitionXfer::setBackgroundMusic() {
}

void ZoombiniTransitionXfer::computeXferRoute() {
	// IDA: puzzleXfer_465FEE — determine xfer view (0-5) from source SI page.
	// Source SI page set by each page before calling setNextPage(kXfer).
	ZMB_SI_PAGE src = _vm->_xferSrcPage;

	switch (src) {
	case ZMB_SI_PICKER_01: // Picker -> Bridge (Route from Picker)
		_xferView = XFER_ROUTE0_FROM_ISLE;
		_nextPageType = ZoombiniPageType::kBridge;
		_xferBackgroundResId = kResBackground5000_FromIsle;
		_xferShapesId = kResShapes5100_FromIsle;
		_xferScrbCount = 9;
		break;
	case ZMB_SI_BRIDGE_02: // Bridge -> Tunnels (Route 1 Path 2)
		_xferView = XFER_ROUTE1_BIG_BAD_HUNGRY;
		_nextPageType = ZoombiniPageType::kTunnels;
		_xferBackgroundResId = kResBackground1000_BigBadHungry;
		_xferShapesId = kResShapes1100_BigBadHungry;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_TUNNELS_03: // Tunnels -> Pizza (Route 1 Path 3)
		_xferView = XFER_ROUTE1_BIG_BAD_HUNGRY;
		_nextPageType = ZoombiniPageType::kPizza;
		_xferBackgroundResId = kResBackground1000_BigBadHungry;
		_xferShapesId = kResShapes1100_BigBadHungry;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_PIZZA_04: // Pizza -> Basecamp 1 (Route 1 Path 4)
		_xferView = XFER_ROUTE1_BIG_BAD_HUNGRY;
		_nextPageType = ZoombiniPageType::kBasecamp1;
		_xferBackgroundResId = kResBackground1000_BigBadHungry;
		_xferShapesId = kResShapes1100_BigBadHungry;
		_xferScrbCount = 3;
		// IDA: puzzleDispatch_switchToPending sets wLastContainerPuzzleId for container-bound routes
		_vm->_state->_lastPageBeforeContainer = static_cast<uint16>(src);
		break;
	case ZMB_SI_BC1_NORTH_05: // Basecamp 1 north exit -> Ferry (Route 2 Path 1)
		_xferView = XFER_ROUTE2_WHOS_BAYOU;
		_nextPageType = ZoombiniPageType::kFerry;
		_xferBackgroundResId = kResBackground2000_WhosBayou;
		_xferShapesId = kResShapes2100_WhosBayou;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_FERRY_07: // Ferry -> Lilly (Route 2 Path 2)
		_xferView = XFER_ROUTE2_WHOS_BAYOU;
		_nextPageType = ZoombiniPageType::kLilly;
		_xferBackgroundResId = kResBackground2000_WhosBayou;
		_xferShapesId = kResShapes2100_WhosBayou;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_LILLY_08: // Lilly -> Slides (Route 2 Path 3)
		_xferView = XFER_ROUTE2_WHOS_BAYOU;
		_nextPageType = ZoombiniPageType::kSlides;
		_xferBackgroundResId = kResBackground2000_WhosBayou;
		_xferShapesId = kResShapes2100_WhosBayou;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_SLIDES_09: // Slides -> Basecamp 2 (Route 2 Path 4)
		_xferView = XFER_ROUTE2_WHOS_BAYOU;
		_nextPageType = ZoombiniPageType::kBasecamp2;
		_xferBackgroundResId = kResBackground2000_WhosBayou;
		_xferShapesId = kResShapes2100_WhosBayou;
		_xferScrbCount = 3;
		_vm->_state->_lastPageBeforeContainer = static_cast<uint16>(src);
		break;
	case ZMB_SI_BC1_SOUTH_06: // Basecamp 1 south exit -> Fleens (Route 3 Path 1)
		_xferView = XFER_ROUTE3_DEEP_DARK_FOREST;
		_nextPageType = ZoombiniPageType::kFleens;
		_xferBackgroundResId = kResBackground3000_DeepDarkForest;
		_xferShapesId = kResShapes3100_DeepDarkForest;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_FLEENS_10: // Fleens -> Hotel (Route 3 Path 2)
		_xferView = XFER_ROUTE3_DEEP_DARK_FOREST;
		_nextPageType = ZoombiniPageType::kHotel;
		_xferBackgroundResId = kResBackground3000_DeepDarkForest;
		_xferShapesId = kResShapes3100_DeepDarkForest;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_HOTEL_11: // Hotel -> Net (Route 3 Path 3)
		_xferView = XFER_ROUTE3_DEEP_DARK_FOREST;
		_nextPageType = ZoombiniPageType::kNet;
		_xferBackgroundResId = kResBackground3000_DeepDarkForest;
		_xferShapesId = kResShapes3100_DeepDarkForest;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_NET_12: // Net -> Basecamp 2 (Route 3 Path 4)
		_xferView = XFER_ROUTE3_DEEP_DARK_FOREST;
		_nextPageType = ZoombiniPageType::kBasecamp2;
		_xferBackgroundResId = kResBackground3000_DeepDarkForest;
		_xferShapesId = kResShapes3100_DeepDarkForest;
		_xferScrbCount = 3;
		_vm->_state->_lastPageBeforeContainer = static_cast<uint16>(src);
		break;
	case ZMB_SI_BASECAMP2_13: // Basecamp 2 -> Caves (Route 4 Path 1)
		_xferView = XFER_ROUTE4_MOUNTAIN_OF_DESPAIR;
		_nextPageType = ZoombiniPageType::kCaves;
		_xferBackgroundResId = kResBackground4000_MountainOfDespair;
		_xferShapesId = kResShapes4100_MountainOfDespair;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_CAVES_14: // Caves -> Smoke (Route 4 Path 2)
		_xferView = XFER_ROUTE4_MOUNTAIN_OF_DESPAIR;
		_nextPageType = ZoombiniPageType::kSmoke;
		_xferBackgroundResId = kResBackground4000_MountainOfDespair;
		_xferShapesId = kResShapes4100_MountainOfDespair;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_SMOKE_15: // Smoke -> Maze (Route 4 Path 3)
		_xferView = XFER_ROUTE4_MOUNTAIN_OF_DESPAIR;
		_nextPageType = ZoombiniPageType::kMaze;
		_xferBackgroundResId = kResBackground4000_MountainOfDespair;
		_xferShapesId = kResShapes4100_MountainOfDespair;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_MAZE_16: // Maze -> Town (Route to Town)
		_xferView = XFER_ROUTE5_TO_TOWN;
		_nextPageType = ZoombiniPageType::kTown;
		_xferBackgroundResId = kResBackground6000_ToTown;
		_xferShapesId = kResShapes6100_ToTown;
		_xferScrbCount = 9;
		_vm->_state->_lastPageBeforeContainer = static_cast<uint16>(src);
		break;
	default:
		error("ZoombiniTransitionXfer: unknown source SI page %d", static_cast<int>(src));
		break;
	}
}

// IDA: puzzleXfer_465FEE sound selection — difficulty/routeLevel-based voice SND from XFER.MHK.
// wRouteLevel = readRouteLevel_4569EC() + 1, wDifficulty = getDifficultyIdFromPuzzleFlag(&wPuzzleFlag).
// Returns SND resource ID (20000-20103) or 0 if none.
uint16 ZoombiniTransitionXfer::selectXferSound() const {
	// IDA: wDifficulty derived from destination puzzle flag (wPuzzleFlagIdx = dest page).
	// Original uses a LOCAL COPY of the page flag so getDifficultyIdFromPuzzleFlag
	// does not increment the real state counter.
	uint16 pageFlagCopy = _vm->_state->getPageFlagFromPageType(_nextPageType);
	uint16 difficulty = static_cast<uint16>(_vm->_state->getDifficultyIdFromPageFlag(pageFlagCopy));
	// IDA: wRouteLevel = readRouteLevel_4569EC() + 1
	int16 routeLevel = _vm->_state->readActivePageRouteLevel() + 1;

	if (_xferView == XFER_ROUTE0_FROM_ISLE) { // XFER_0 — FROM ISLE (Bridge): SND 20094-20099
		switch (difficulty) {
		case ZMB_DIFFICULTY_NOTVISITED_00:
			if (routeLevel >= 2 && routeLevel <= 3) {
				// Higher route level: 6 choices including hard voice
				switch (_vm->_rnd->getRandomNumber(1, 6)) {
				case 1:
					return 20094;
				case 2:
					return 20095;
				case 3:
					return 20096;
				case 4:
					return 20097;
				case 5:
					return 20098; // hard voice
				default:
					return 20099; // no-voice
				}
			} else {
				// Low route level: 5 choices, no hard voice
				switch (_vm->_rnd->getRandomNumber(1, 5)) {
				case 1:
					return 20094;
				case 2:
					return 20095;
				case 3:
					return 20096;
				case 4:
					return 20097;
				default:
					return 20099; // no-voice
				}
			}
			break;
		case ZMB_DIFFICULTY_LEVEL1_01:
			return 20094;
		case ZMB_DIFFICULTY_LEVEL2_02:
		case ZMB_DIFFICULTY_LEVEL4_12:
			return 20098; // hard voice
		case ZMB_DIFFICULTY_LEVEL3_05:
			return (routeLevel >= 2 && routeLevel <= 3) ? 20098 : 20094;
		default:
			break;
		}
	} else if (_xferView == XFER_ROUTE1_BIG_BAD_HUNGRY) { // XFER_1 — BIG BAD HUNGRY: switches on destination puzzle
		switch (_nextPageType) {
		case ZoombiniPageType::kBasecamp1:
			// BC1 destination: random no-voice pick from caves/pizza no-voice
			return (_vm->_rnd->getRandomNumber(0, 1) == 0) ? 20009 : 20012;

		case ZoombiniPageType::kTunnels:
			// difficulty 1, 2, or 4 → fixed voice; 0 → random; else → voice
			if (difficulty == ZMB_DIFFICULTY_LEVEL1_01 ||
				difficulty == ZMB_DIFFICULTY_LEVEL2_02 ||
				difficulty == ZMB_DIFFICULTY_LEVEL4_12) {
				return 20008;
			} else if (difficulty == ZMB_DIFFICULTY_NOTVISITED_00) {
				switch (_vm->_rnd->getRandomNumber(1, 3)) {
				case 1:
					return 20007;
				case 2:
					return 20008;
				default:
					return 20009; // no-voice
				}
			} else {
				return 20007;
			}
			break;

		case ZoombiniPageType::kPizza:
			switch (difficulty) {
			case ZMB_DIFFICULTY_LEVEL1_01:
				return 20010;
			case ZMB_DIFFICULTY_LEVEL2_02:
				return 20011;
			case ZMB_DIFFICULTY_LEVEL3_05:
				return (routeLevel < 2) ? 20010 : 20011;
			case ZMB_DIFFICULTY_LEVEL4_12:
				return 20011;
			case ZMB_DIFFICULTY_NOTVISITED_00:
			default:
				if (routeLevel < 2) {
					return (_vm->_rnd->getRandomNumber(0, 1) == 0) ? 20010 : 20012;
				} else {
					return (_vm->_rnd->getRandomNumber(0, 1) == 0) ? 20011 : 20012;
				}
			}
			break;

		default:
			break;
		}
	} else if (_xferView == XFER_ROUTE2_WHOS_BAYOU) { // XFER_2 — WHO'S BAYOU: switches on destination puzzle
		switch (_nextPageType) {
		case ZoombiniPageType::kBasecamp2:
			// BC2 via bayou: random no-voice from ferry/lilly/slides
			switch (_vm->_rnd->getRandomNumber(1, 3)) {
			case 1:
				return 20016; // ferry no-voice
			case 2:
				return 20020; // lilly no-voice
			default:
				return 20024; // slides no-voice
			}
			break;

		case ZoombiniPageType::kFerry:
			switch (difficulty) {
			case ZMB_DIFFICULTY_NOTVISITED_00:
				if (routeLevel >= 2) {
					// Higher level: 4 choices including hard
					switch (_vm->_rnd->getRandomNumber(1, 4)) {
					case 1:
						return 20013;
					case 2:
						return 20014;
					case 3:
						return 20015; // hard
					default:
						return 20016; // no-voice
					}
				} else {
					switch (_vm->_rnd->getRandomNumber(1, 3)) {
					case 1:
						return 20013;
					case 2:
						return 20014;
					default:
						return 20016; // no-voice
					}
				}
				break;
			case ZMB_DIFFICULTY_LEVEL1_01:
			case ZMB_DIFFICULTY_LEVEL3_05:
				return 20014;
			case ZMB_DIFFICULTY_LEVEL2_02:
			case ZMB_DIFFICULTY_LEVEL4_12:
				return 20015; // hard
			default:
				break;
			}
			break;

		case ZoombiniPageType::kLilly:
			switch (difficulty) {
			case ZMB_DIFFICULTY_NOTVISITED_00:
				if (routeLevel >= 2) {
					switch (_vm->_rnd->getRandomNumber(1, 4)) {
					case 1:
						return 20017;
					case 2:
						return 20018;
					case 3:
						return 20019; // hard
					default:
						return 20020; // no-voice
					}
				} else {
					switch (_vm->_rnd->getRandomNumber(1, 3)) {
					case 1:
						return 20017;
					case 2:
						return 20018;
					default:
						return 20020; // no-voice
					}
				}
				break;
			case ZMB_DIFFICULTY_LEVEL1_01:
				return 20018;
			case ZMB_DIFFICULTY_LEVEL2_02:
			case ZMB_DIFFICULTY_LEVEL4_12:
				return 20019; // hard
			case ZMB_DIFFICULTY_LEVEL3_05:
				return (routeLevel < 2) ? 20018 : 20019;
			default:
				break;
			}
			break;

		case ZoombiniPageType::kSlides:
			if (difficulty == ZMB_DIFFICULTY_NOTVISITED_00) {
				switch (_vm->_rnd->getRandomNumber(1, 3)) {
				case 1:
					return 20021;
				case 2:
					return 20022;
				default:
					return 20024; // no-voice
				}
			} else {
				// difficulty 1, 2, 4, 5 → voice
				return 20022;
			}
			break;

		default:
			break;
		}
	} else if (_xferView == XFER_ROUTE3_DEEP_DARK_FOREST) { // XFER_3 — DEEP DARK FOREST: switches on destination puzzle
		switch (_nextPageType) {
		case ZoombiniPageType::kBasecamp2:
			// BC2 via forest: random no-voice from fleens/hotel/net
			switch (_vm->_rnd->getRandomNumber(1, 3)) {
			case 1:
				return 20028; // fleens no-voice
			case 2:
				return 20031; // hotel no-voice
			default:
				return 20034; // net no-voice
			}
			break;

		case ZoombiniPageType::kFleens:
			switch (difficulty) {
			case ZMB_DIFFICULTY_NOTVISITED_00:
				if (routeLevel == 1 || routeLevel == 3) {
					// Low difficulty levels: 3 choices, no hard
					switch (_vm->_rnd->getRandomNumber(1, 3)) {
					case 1:
						return 20025;
					case 2:
						return 20026;
					default:
						return 20028; // no-voice
					}
				} else {
					// Higher: 4 choices including hard
					switch (_vm->_rnd->getRandomNumber(1, 4)) {
					case 1:
						return 20025;
					case 2:
						return 20026;
					case 3:
						return 20027; // hard
					default:
						return 20028; // no-voice
					}
				}
				break;
			case ZMB_DIFFICULTY_LEVEL1_01:
			case ZMB_DIFFICULTY_LEVEL3_05:
				return 20026;
			case ZMB_DIFFICULTY_LEVEL2_02:
			case ZMB_DIFFICULTY_LEVEL4_12:
				return 20026;
			default:
				break;
			}
			break;

		case ZoombiniPageType::kHotel:
			if (difficulty == ZMB_DIFFICULTY_NOTVISITED_00) {
				switch (_vm->_rnd->getRandomNumber(1, 3)) {
				case 1:
					return 20029;
				case 2:
					return 20030;
				default:
					return 20031; // no-voice
				}
			} else {
				return 20030;
			}
			break;

		case ZoombiniPageType::kNet:
			if (difficulty == ZMB_DIFFICULTY_NOTVISITED_00) {
				switch (_vm->_rnd->getRandomNumber(1, 3)) {
				case 1:
					return 20032;
				case 2:
					return 20033;
				default:
					return 20034; // no-voice
				}
			} else {
				return 20033;
			}
			break;

		default:
			break;
		}
	} else if (_xferView == XFER_ROUTE4_MOUNTAIN_OF_DESPAIR) { // XFER_4 — MOUNTAIN OF DESPAIR: switches on destination puzzle
		switch (_nextPageType) {
		case ZoombiniPageType::kCaves:
			if (difficulty == ZMB_DIFFICULTY_NOTVISITED_00) {
				switch (_vm->_rnd->getRandomNumber(1, 3)) {
				case 1:
					return 20035;
				case 2:
					return 20036;
				default:
					return 20037; // no-voice
				}
			} else {
				return 20036;
			}
			break;

		case ZoombiniPageType::kSmoke:
			switch (difficulty) {
			case ZMB_DIFFICULTY_NOTVISITED_00:
				if (routeLevel >= 2) {
					switch (_vm->_rnd->getRandomNumber(1, 4)) {
					case 1:
						return 20000;
					case 2:
						return 20001;
					case 3:
						return 20002; // hard
					default:
						return 20003; // no-voice
					}
				} else {
					switch (_vm->_rnd->getRandomNumber(1, 3)) {
					case 1:
						return 20000;
					case 2:
						return 20001;
					default:
						return 20003; // no-voice
					}
				}
				break;
			case ZMB_DIFFICULTY_LEVEL1_01:
				return 20002;
			case ZMB_DIFFICULTY_LEVEL2_02:
			case ZMB_DIFFICULTY_LEVEL3_05:
			case ZMB_DIFFICULTY_LEVEL4_12:
				return (routeLevel < 2) ? 20001 : 20002;
			default:
				break;
			}
			break;

		case ZoombiniPageType::kMaze:
			if (difficulty == ZMB_DIFFICULTY_NOTVISITED_00) {
				switch (_vm->_rnd->getRandomNumber(1, 3)) {
				case 1:
					return 20004;
				case 2:
					return 20005;
				default:
					return 20006; // no-voice
				}
			} else {
				return 20005;
			}
			break;

		default:
			break;
		}
	} else if (_xferView == XFER_ROUTE5_TO_TOWN) { // XFER_5 — TO TOWN
		// IDA xfer_initAndRunTransition @ 0x466E80-0x466EAA: each random branch
		// stores a candidate (20100/20101/20102/20103) but immediately falls
		// through to 0x466EAA which unconditionally rewrites the slot to 20100
		// (likely a missing-break in the original C). The result is XFER_5
		// always plays SND 20100 regardless of difficulty/RNG. We mirror that
		// faithfully — verified via disasm.
		return 20100;
	}

	return 0;
}

void ZoombiniTransitionXfer::setBackgroundBitmap() {
	computeXferRoute();
	_vm->_gfx->setPalette(_xferBackgroundResId);
	_vm->_gfx->drawBackground(_xferBackgroundResId);
}

void ZoombiniTransitionXfer::loadFeatures() {
	// IDA: xfer_initGlobalState (0x465EE4) sets word_4A4764 = 0 to disable
	// fidget/idle animations during the transition.
	_vm->_fidgetThreshold = 0;

	// IDA: puzzleXfer_465FEE — load environment SCRBs, zoombinis, sub-feature, sound, text.
	const ZmbResource xferShapes(ZmbArchiveKind::kPage, _xferShapesId);

	// Load environment SCRBs.
	// XFER_0/XFER_5: 9 SCRBs (5100-5108 / 6100-6108); each loops with event-trigger flags.
	// IDA env flags: 0x01188000 (LOOP_ANIM | DEFER_ANIM | PLAY_ONCE | DEFER_RENDER)
	// XFER_1-4: 3 SCRBs from xferShapes:
	//   [0] main overlay: 0x0C10C000 (NO_DIRTY_MERGE | LOOP_ANIM | PLAY_ONCE | OVERLAY | REGION_TRACK)
	//   [1],[2] static shapes: flags = 0
	const uint32 kEnvScrbFlags = ZmbFeature::FLAG_00008000_LOOP_ANIM |
								 ZmbFeature::FLAG_00080000_DEFER_ANIM |
								 ZmbFeature::FLAG_00100000_PLAY_ONCE |
								 ZmbFeature::FLAG_01000000_DEFER_RENDER;

	const bool isMidRoute = (_xferView >= XFER_ROUTE1_BIG_BAD_HUNGRY &&
							 _xferView <= XFER_ROUTE4_MOUNTAIN_OF_DESPAIR);
	const bool isToTown = (_xferView == XFER_ROUTE5_TO_TOWN);
	const bool isFromIsle = (_xferView == XFER_ROUTE0_FROM_ISLE);

	// Initialize callback state.
	_completionCounter = 0;
	_bodyArrangementOverride = 0;
	_linkTargetScrbId = 0;
	_finalEnvScrbId = 0;
	_envOneShotScrbId = 0;
	_envOneShotAvailable = false;
	_xfer5EventScrbId = 0;
	_xfer5DisplayedTownCount = static_cast<int16>(_vm->_state->_f._zmbStoredTownCount);
	_xfer5ForegroundFeatures[0] = nullptr;
	_xfer5ForegroundFeatures[1] = nullptr;
	for (int i = 0; i < 4; i++)
		_envScrbIds[i] = 0;
	for (int i = 0; i < 2; i++)
		_envEventTriggerFlags[i] = false;

	// -----------------------------------------------------------------------
	// Phase 1: pre-snoid environment SCRBs
	// -----------------------------------------------------------------------
	if (isFromIsle) {
		// XFER_0: IDA (0x466F39) loads animated SCRBs 5102-5108 BEFORE snoids (they render behind),
		// then static overlays 5100-5101 AFTER snoids (they render in front).
		// IDA: 5102-5103 stored in word_4B7000[], 5104-5107 in word_4B6FF4[], 5108 in word_4B97D2.
		for (uint16 i = 2; i < _xferScrbCount; i++)
			loadScrbFeature(xferShapes, _xferShapesId + i, 6, kEnvScrbFlags);

		// Track env SCRB IDs for the 40% trigger branch in onEveryFrame.
		// IDA: word_4B97D4[0..3] are used for random env activation.
		// These map to SCRBs 5102-5105 (the first 4 animated env SCRBs).
		for (int i = 0; i < 4; i++)
			_envScrbIds[i] = _xferShapesId + 2 + i; // 5102-5105

		// IDA: word_4B97D2 = one-shot env SCRB 5108.
		_envOneShotScrbId = _xferShapesId + 8; // 5108
		_envOneShotAvailable = true;

		// IDA: word_4B97E8[0..1] = 1 — one-shot flags for events 10-11 (SCRBs 5102-5103).
		_envEventTriggerFlags[0] = true; // event 10 → SCRB 5102
		_envEventTriggerFlags[1] = true; // event 11 → SCRB 5103

		// IDA: sub_4572C5(0) swaps body tables to small variants and loads SHPL 3200.
		_useSmallSnoids = true;
	} else if (isToTown) {
		// XFER_5 (IDA LABEL_295): 6108 (animated far bg), 6105 (static), 6104 (static)
		// appear BEHIND the snoids; 6100-6103, 6106-6107 appear in front (loaded below).
		loadScrbFeature(xferShapes, _xferShapesId + 8, 6, kEnvScrbFlags);                         // 6108 animated
		ZmbFeature::EventHooks townCountHooks;
		townCountHooks.setPostRenderFunc(
			reinterpret_cast<ZmbFeature::OnPostRenderFunc>(
				&ZoombiniTransitionXfer::xfer5TownCount_onPostRender));
		loadScrbFeature(xferShapes, _xferShapesId + 5, 0, ZmbFeature::FLAG_00000000_TYPE_SHAPES, townCountHooks); // 6105
		loadScrbFeature(xferShapes, _xferShapesId + 4, 0, ZmbFeature::FLAG_00000000_TYPE_SHAPES); // 6104

		// IDA: word_4B97E6 = runner for 6108 (activated when completionCounter > 4).
		_finalEnvScrbId = _xferShapesId + 8; // 6108

		// IDA: word_4B97E2 = runner for 6104 (linked to snoids on callback event 26).
		_linkTargetScrbId = _xferShapesId + 4; // 6104

		// IDA: word_4B9802 = runner for 6105 (activated by event 50).
		_xfer5EventScrbId = _xferShapesId + 5; // 6105
	} else {
		// XFER_1-4: main overlay SCRB with patch hook + sub-feature go before snoids.
		const uint32 kMainScrbFlags = ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE |
									  ZmbFeature::FLAG_00008000_LOOP_ANIM |
									  ZmbFeature::FLAG_00100000_PLAY_ONCE |
									  ZmbFeature::FLAG_04000000_OVERLAY |
									  ZmbFeature::FLAG_08000000_REGION_TRACK;

		// Sub-feature SCRB at bgId+200 (e.g. 1200, 2200, …) — route path overlay.
		// IDA: loadSubFeatureSCRB_45FE2C(0, 1, bgId+200) with overlay flags.
		// Callbacks: selectBand (preRenderShape) + flood-fill render (renderFunc).
		computeRoutePathBand();
		computeRoutePathColorLevel();
		buildPuzzleCompletionArray();
		_routePathCounter = 0;
		_routePathNextFrame = getCurrentFrameCounter();
		_routePathPixels = nullptr;

		// Main SCRB (IDA: wResShplTbmpResIdPlus100) with route view shape
		// remapping callback (IDA: onPreRenderShapeFunc = xfer_updateRouteViewSlots).
		ZmbFeature::EventHooks mainScrbHooks;
		mainScrbHooks.setPreRenderShapeFunc(
			reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(
				&ZoombiniTransitionXfer::routeView_updateSlots));
		loadScrbFeature(xferShapes, _xferShapesId, 6, kMainScrbFlags, mainScrbHooks);

		const uint16 subId = _xferBackgroundResId + 200;
		ZmbFeature::EventHooks routePathHooks;
		routePathHooks.setPreRenderShapeFunc(
			reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(
				&ZoombiniTransitionXfer::routePath_selectBand));
		routePathHooks.setRenderFunc(
			reinterpret_cast<ZmbFeature::OnRenderFunc>(
				&ZoombiniTransitionXfer::routePath_onPostRender));
		_routePathFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, subId), subId, 4,
			ZmbFeature::FLAG_04000000_OVERLAY, routePathHooks);
	}

	// -----------------------------------------------------------------------
	// Snoids — loaded from active pack (set by preceding page's save/cleanup).
	// IDA: handleZoombiniAnimation_maybe_4528A6 / zmbMoveAnimation_45479D.
	// The original engine always reads the active pack for xfer transitions.
	// -----------------------------------------------------------------------
	ZmbStateActivePack &pack = _vm->_state->_f._zmbPackActive;

	// Snoid feature flags: SNOID | OVERLAY (one-shot walk, not a looping feature).
	const uint32 snoidFlags = ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_04000000_OVERLAY;

	// IDA: zmbMoveAnimation_45479D resets ui_bDragLockActive = 0 at the start.
	_vm->_walkersInProgress = 0;

	uint16 walkerIdx = 0;
	for (int16 i = 0; i < 16; i++) {
		ZmbStateActiveEntry &entry = pack._entries[i];
		if (!entry._bIsOccupied)
			continue;

		// Determine start position per route.
		// XFER_0: all snoids at (200, 235), walk horizontally to right edge.
		// XFER_1-4: off-screen left at (-22, 445), walk to (670, 445).
		// XFER_5: off-screen left at (-22, 282+rand), walk to (670, y).
		Common::Point startPos;
		if (isFromIsle) {
			startPos = Common::Point(200, 235);
		} else if (isMidRoute) {
			startPos = Common::Point(-22, 445);
		} else { // isToTown
			// IDA: pPosArr[v79].y = 6 * rand(3,0) + 282  (282, 288, 294 or 300)
			const int16 randY = static_cast<int16>(6 * _vm->_rnd->getRandomNumber(3) + 282);
			startPos = Common::Point(-22, randY);
		}

		ZmbSnoid *snoid = loadSnoidFromPack(static_cast<uint16>(kSnoidPackBase) + _nextPackSnoidId++,
											startPos, snoidFlags);
		if (!snoid)
			continue;

		snoid->_trait = entry._traits;
		snoid->_name = entry.getU32Name(_vm);

		if (_useSmallSnoids) {
			// IDA: sub_4572C5(0) swaps body tables to small variants and loads SHPL 0xC80=3200.
			// _useSmallShapeRegs activates the small table path in updateWalkHotspots() and
			// the small REGS table path in the render loop.
			snoid->setResource(ZmbResource(ZmbArchiveKind::kSystem, 3200));
			snoid->setupSmallIdleHotspots();
		} else {
			snoid->setupIdleHotspots();
		}

		_xferSnoidCount++;

		if (isMidRoute) {
			// XFER_1-4: IDA zmbMoveAnimation_45479D(90, 445, 670) — stagger walk off right edge.
			// IDA: does NOT set wBoolDoRender=0 — only sets dNextRenderFrame for timing.
			// Snoids remain visible at current positions until stagger delay fires.
			const Common::Point targetPos(670, startPos.y);
			snoid->setAnimTargetPos(targetPos);
			snoid->setAnimState(kSnoidAnimArrivalMotion, nullptr);

			if (walkerIdx > 0) {
				snoid->setDelayUntilFrame(getCurrentFrameCounter() + walkerIdx * 90);
			}
			walkerIdx++;
		} else {
			// XFER_0 and XFER_5: snoids start idle.
			// XFER_0: SCRS 5200 triggers; XFER_5: SCRS 6200 triggers.
			// IDA: registerVirtualScrbZoombiniAnimation_452A64 with wBool=0.
			walkerIdx++;
		}
	}

	// -----------------------------------------------------------------------
	// Phase 2: post-snoid features (rendered in front of snoids)
	// -----------------------------------------------------------------------
	if (isFromIsle) {
		// XFER_0: static SCRBs 5100-5101 loaded AFTER snoids — foreground overlays.
		// IDA: these are the last loadSCRB calls at 0x466FC8/466FDE with flags=0.
		// IDA: word_4B97E2 = runner for 5100 (linked to snoids on callback event 26 / cycle 2).
		loadScrbFeature(xferShapes, _xferShapesId + 0, 0, ZmbFeature::FLAG_00000000_TYPE_SHAPES);
		loadScrbFeature(xferShapes, _xferShapesId + 1, 0, ZmbFeature::FLAG_00000000_TYPE_SHAPES);

		_linkTargetScrbId = _xferShapesId + 0; // 5100
	} else if (isToTown) {
		// 6100-6103 static foreground, 6106-6107 animated foreground — above snoid walkers.
		for (uint16 i = 0; i <= 3; i++)
			loadScrbFeature(xferShapes, _xferShapesId + i, 0, ZmbFeature::FLAG_00000000_TYPE_SHAPES);

		// IDA: kEnvScrbFlagsNoLoop for 6106/6107 (no LOOP flag — DEFER_ANIM | PLAY_ONCE | DEFER_RENDER).
		const uint32 kEnvScrbFlagsNoLoop = ZmbFeature::FLAG_00080000_DEFER_ANIM |
										   ZmbFeature::FLAG_00100000_PLAY_ONCE |
										   ZmbFeature::FLAG_01000000_DEFER_RENDER;
		ZmbFeature *fg6106 = loadScrbFeature(xferShapes, _xferShapesId + 6, 6, kEnvScrbFlagsNoLoop);
		ZmbFeature *fg6107 = loadScrbFeature(xferShapes, _xferShapesId + 7, 6, kEnvScrbFlagsNoLoop);
		_xfer5ForegroundFeatures[0] = fg6106;
		_xfer5ForegroundFeatures[1] = fg6107;

		// IDA xfer_initAndRunTransition @ 0x46741B-0x46743A: 6107/6106 are
		// activated via scrb_initRunnerWithScript right after init, before the
		// hover loop starts. Without this they sit inert (DEFER_ANIM) and the
		// XFER_5 foreground animation never plays.
		if (fg6107) {
			fg6107->initValues();
			fg6107->activateAnimate();
			fg6107->activateRender();
		}
		if (fg6106) {
			fg6106->initValues();
			fg6106->activateAnimate();
			fg6106->activateRender();
		}
		addExternalDirtyRect(Common::Rect(0, 0, 640, 480));
	} else if (isMidRoute) {
		// shapes[1] and shapes[2] are static overlapping edges above the walker overlay.
		loadScrbFeature(xferShapes, _xferShapesId + 1, 0, ZmbFeature::FLAG_00000000_TYPE_SHAPES);
		loadScrbFeature(xferShapes, _xferShapesId + 2, 0, ZmbFeature::FLAG_00000000_TYPE_SHAPES);
	}

	// IDA: ALL views use getElapsedFrameTime_460872() > 0x12C (300 frames, ~5s at 60fps)
	// as the auto-close timer. No walker-completion logic in the original.
	_closureFrame = getCurrentFrameCounter() + 300;

	// SCRS trigger timer initialization (XFER_0 and XFER_5 only).
	// IDA: dword_4B97BC is not explicitly initialized in puzzleXfer_465FEE for XFER_5;
	// it starts at 0 (global), so the first trigger fires immediately on first check.
	if (isFromIsle) {
		// IDA: dword_4B97BC = currentFrame + 30 * rand(3,6) — delay first trigger.
		_scrsNextTriggerFrame = getCurrentFrameCounter() + 30 * _vm->_rnd->getRandomNumber(3, 6);
		// IDA 0x4676C3: scrsResId = traits.cFoot + 5199 (foot 1-5 -> SCRS 5200-5204).
		// Store the base so runtime can add cFoot.
		_scrsResIdBase = 5199;
	} else if (isToTown) {
		// IDA: dword_4B97BC starts at 0 → first trigger fires immediately.
		_scrsNextTriggerFrame = 0;
		// IDA 0x4677F2: scrsResId = traits.cFoot + 6199 (foot 1-5 -> SCRS 6200-6204).
		_scrsResIdBase = 6199;
	}

	// Play voice sound for this xfer route.
	// IDA: wCurrentSound_4B97F0 is enqueued after the render loop.
	// Sound IDs 20000-20104 are in ZOOMBINI.MHK (kSystem), not XFER.MHK.
	_xferSoundId = selectXferSound();
	if (_xferSoundId != 0 && _vm->hasResource(ID_SND, ZmbResource(ZmbArchiveKind::kSystem, _xferSoundId)))
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, _xferSoundId), Audio::Mixer::kSFXSoundType);

	// Draw route name text for mid-route views (XFER_1-4).
	// IDA: drawOutlinedText_410D48 with pStrTableRouteNames_Pre1Idx_4A4F0C[view],
	//      palette 10 (fg=white), 0x2D=45 (outline/shadow=black). The original
	//      blits the rendered text directly onto the back screen port via the
	//      memcpy that follows the call (xfer_initAndRunTransition @ 0x46735A-
	//      0x46737E), so the text is baked into the persistent background and
	//      survives subsequent shape-screen redraws.
	//
	// Text rects from IDA data at 0x4A7E4E / 0x4A7E52 (leftTop/rightBottom pairs indexed by view).
	//
	// Previously we drew into kShapeScreen, but the shape screen is rebuilt
	// every frame from registered features — so the route name flashed for one
	// frame at most and disappeared. Drawing into kBackScreen matches the IDA
	// behavior of baking the text into the permanent background bitmap.
	if (_xferView >= XFER_ROUTE1_BIG_BAD_HUNGRY && _xferView <= XFER_ROUTE4_MOUNTAIN_OF_DESPAIR) {
		// Exact rects from IDA binary analysis:
		//   View 1 (BigBadHungry):    left=43,  top=54,  right=226, bottom=107
		//   View 2 (WhosBayou):       left=371, top=33,  right=613, bottom=65
		//   View 3 (DeepDarkForest):  left=127, top=29,  right=299, bottom=81
		//   View 4 (MountainDespair): left=135, top=29,  right=323, bottom=82
		static const Common::Rect kRouteTextRects[4] = {
			Common::Rect(43, 54, 226, 107), // View 1
			Common::Rect(371, 33, 613, 65), // View 2
			Common::Rect(127, 29, 299, 81), // View 3
			Common::Rect(135, 29, 323, 82), // View 4
		};
		const uint32 textKey = static_cast<uint32>(ZoombiniText::Key::kRoute1) + _xferView - 1;
		const Common::Rect &textRect = kRouteTextRects[_xferView - 1];

		ZoombiniGraphics::TextConf tc;
		// IDA xfer_initAndRunTransition @ 0x467301: loads `g_pGulimCheInst_Title`
		// before drawOutlinedText — i.e. the larger 18pt title font, not the
		// regular text font. Without this override the route name renders in
		// the default 12pt body font and looks visibly small vs the original.
		tc._fontUsage = ZoombiniFontUsage::kFontTitle;
		tc._outlineEffect = true;
		tc._textPalette = ZoombiniGraphics::kColor0A_White;    // palette #10 (fg)
		tc._outlinePalette = ZoombiniGraphics::kColor2D_Black; // palette #45 (shadow)
		tc._hAlign = Graphics::kTextAlignCenter;
		tc._vAlign = Graphics::kTextAlignCenter;
		_vm->_gfx->drawText(ZoombiniGraphics::kBackScreen, textKey, textRect, tc);
	}
}

void ZoombiniTransitionXfer::onEveryFrame() {
	if (isClosed())
		return;

	if (_xferView == XFER_ROUTE5_TO_TOWN) {
		for (int featureIdx = 0; featureIdx < 2; featureIdx++) {
			ZmbFeature *feature = _xfer5ForegroundFeatures[featureIdx];
			if (feature && !feature->hasAnimEndCallbackFired())
				addExternalDirtyRect(Common::Rect(0, 0, 640, 480));
		}
	}

	// -----------------------------------------------------------------------
	// ALL views: timer-based auto-close (300 frames) + wait for sound to finish.
	// IDA: puzzleXfer_onHover_4674EA — getElapsedFrameTime_460872() > 0x12C
	//      AND wCurrentSound_4B97F0 finished playing.
	// -----------------------------------------------------------------------
	if (_closureFrame > 0 && getCurrentFrameCounter() >= _closureFrame) {
		if (_xferSoundId == 0 || !_vm->_sound->isPlaying(_xferSoundId)) {
			close();
			return;
		}
	}

	// -----------------------------------------------------------------------
	// Completion counter check + SCRS periodic triggers.
	// IDA: puzzleXfer_onHover_4674EA — all inside `currentFrame > dword_4B97BC`.
	// Completion check fires first; view-specific branches follow.
	// -----------------------------------------------------------------------
	if (getCurrentFrameCounter() >= _scrsNextTriggerFrame) {
		// IDA: if (word_4B97E4 > 4) — 5+ snoids completed, activate final env SCRB.
		if (_completionCounter > 4) {
			_completionCounter = -1; // Disable further counting
			if (_finalEnvScrbId != 0)
				activateEnvScrb(_finalEnvScrbId);
			// IDA: sets callback to xfer_commitDestPuzzleId_467F4D (event 30 → page transition).
			// In ScummVM, the timer-based auto-close handles transition.
		}

		// -------------------------------------------------------------------
		// XFER_0: periodic SCRS trigger — start one snoid's animation per interval.
		// IDA: wXferView == 0 branch.
		// Timer: 30 * rand(3,6) = 90-180 frames between triggers.
		// 60% chance: trigger next idle snoid to play SCRS 5200.
		// 40% chance (after first trigger): trigger random env SCRB animation.
		// -------------------------------------------------------------------
		if (_xferView == XFER_ROUTE0_FROM_ISLE && _xferSnoidCount > 0) {
			_scrsNextTriggerFrame = getCurrentFrameCounter() + 30 * _vm->_rnd->getRandomNumber(3, 6);

			int16 chance = _vm->_rnd->getRandomNumber(1, 100);
			if (chance <= 40 && _scrsTriggerPhase1) {
				// 40% chance (only after first snoid trigger): env SCRB activation.
				// IDA: nextRand(4, 0) → 0-3 = random env SCRB, 4 = one-shot.
				int16 envIdx = _vm->_rnd->getRandomNumber(0, 4);
				if (envIdx < 4) {
					// Activate one of the 4 env SCRBs (5102-5105).
					if (_envScrbIds[envIdx] != 0)
						activateEnvScrb(_envScrbIds[envIdx]);
				} else {
					// envIdx == 4: one-shot env SCRB 5108 (only once).
					if (_envOneShotAvailable && _envOneShotScrbId != 0) {
						_envOneShotAvailable = false;
						activateEnvScrb(_envOneShotScrbId);
					}
				}
			} else {
				// 60% chance (or 100% if first trigger): trigger next idle snoid SCRS.
				_scrsTriggerPhase1 = true;

				if (_scrsTriggerIdx < _xferSnoidCount) {
					uint16 snoidId = static_cast<uint16>(kSnoidPackBase) + _scrsTriggerIdx;
					ZmbSnoid *snoid = getSnoid(snoidId);
					if (snoid && snoid->getAnimState() == kSnoidAnimIdle) {
						// IDA 0x4676A2-0x4676AB: chIsFacingLeft is cleared
						// before SCRS playback so the snoid faces right (the
						// walk-off direction). Without this, leftover facing
						// from a prior cycle can flip the sprite mid-transition.
						snoid->setFacingLeft(false);
						// IDA 0x4676C3: scrsResId = _scrsResIdBase (5199) + traits.cFoot.
						// Foot 1-5 -> SCRS 5200-5204 (different walk anim per foot type).
						uint16 scrsResId = _scrsResIdBase + snoid->_trait._foot;
						Common::SeekableReadStream *scrsStream =
							_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, scrsResId));
						if (scrsStream)
							snoid->startScrsPlayback(scrsStream, true /* hideOnComplete */, true /* rejectState */);
					}
					_scrsTriggerIdx++;
				}
			}
		}

		// -------------------------------------------------------------------
		// XFER_5: periodic SCRS trigger — same structure as XFER_0.
		// IDA: wXferView == 5 branch.
		// Timer: 40 * rand(3,6) = 120-240 frames between triggers.
		// 100% snoid triggers (no env SCRB split), using SCRS 6200.
		// -------------------------------------------------------------------
		if (_xferView == XFER_ROUTE5_TO_TOWN && _xferSnoidCount > 0) {
			_scrsNextTriggerFrame = getCurrentFrameCounter() + 40 * _vm->_rnd->getRandomNumber(3, 6);

			if (_scrsTriggerIdx < _xferSnoidCount) {
				uint16 snoidId = static_cast<uint16>(kSnoidPackBase) + _scrsTriggerIdx;
				ZmbSnoid *snoid = getSnoid(snoidId);
				if (snoid && snoid->getAnimState() == kSnoidAnimIdle) {
					// IDA 0x4677D8: chIsFacingLeft cleared before SCRS playback.
					snoid->setFacingLeft(false);
					// IDA 0x4677F2: scrsResId = _scrsResIdBase (6199) + traits.cFoot.
					// Foot 1-5 -> SCRS 6200-6204 (different walk anim per foot type).
					uint16 scrsResId = _scrsResIdBase + snoid->_trait._foot;
					Common::SeekableReadStream *scrsStream =
						_vm->getResource(MKTAG('S', 'C', 'R', 'S'), ZmbResource(ZmbArchiveKind::kPage, scrsResId));
					if (scrsStream)
						snoid->startScrsPlayback(scrsStream, true /* hideOnComplete */, false /* rejectState */);
				}
				_scrsTriggerIdx++;
			}
		}
	}
}

// ---------------------------------------------------------------------------
// IDA: xfer_handleDestinationClick @ 0x467814 — click during the transition
// commits the pending destination and exits immediately. The original uses a
// two-stage handshake (first click sets puzzle_pendingTransitionTarget, second
// click commits) to coordinate with the SCRB completion path; we collapse it
// to a single skip because ScummVM's auto-close timer is the only race.
// ---------------------------------------------------------------------------
ZmbEventHandleResult ZoombiniTransitionXfer::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	if (!isClosed()) {
		// IDA xfer_handleDestinationClick: a second click after the pending
		// target is set commits transition immediately, bypassing the sound
		// wait. Collapse to one click here.
		_closureFrame = 1;
		_xferSoundId = 0;
		return ZmbEventHandleResult::kConsumed;
	}
	return ZmbEventHandleResult::kPassthrough;
}

// ---------------------------------------------------------------------------
// Helper: activate a deferred env SCRB feature by ID.
// IDA: loadSCRB_460384(1, 0, runner) / scrb_initRunnerWithScript(0, 0, 0, runner).
// For features with DEFER_ANIM | DEFER_RENDER, this starts their animation.
// ---------------------------------------------------------------------------
void ZoombiniTransitionXfer::activateEnvScrb(uint16 scrbId) {
	ZmbFeature *feature = _scrbFeatures.find(scrbId);
	if (!feature)
		return;
	feature->initValues();
	feature->activateAnimate();
	feature->activateRender();
	const Common::Rect &dirtyRect = feature->getZSortRect();
	addExternalDirtyRect(dirtyRect.isEmpty() ? Common::Rect(0, 0, 640, 480) : dirtyRect);
}

void ZoombiniTransitionXfer::xfer5TownCount_onPostRender(ZmbFeature *feature) {
	Common::Rect textRect = feature->getClickRect();
	if (textRect.isEmpty())
		return;

	textRect.left += 16;
	textRect.top += 8;

	Common::U32String text = _vm->_text->getLocalizedString(ZoombiniText::kXferVillePopulation);
	text += Common::U32String::format(" %d", _xfer5DisplayedTownCount);

	ZoombiniGraphics::TextConf tc;
	tc._outlineEffect = true;
	tc._textPalette = 0xD1;
	tc._outlinePalette = 0x70;
	tc._hAlign = Graphics::kTextAlignLeft;
	tc._vAlign = Graphics::kTextAlignStart;

	_vm->_gfx->drawText(ZoombiniGraphics::kShapeScreen, text, textRect, tc);
}

// ---------------------------------------------------------------------------
// IDA: xfer_scrbAnimCallback_467DD4 — handles SCRS event codes during playback.
// Called from the script engine when a SCRS frame terminator carries an event code.
// @param feature  The snoid feature that fired the event.
// @param eventCode  Adjusted event code (raw byte - 1). -1 = end-of-animation.
// ---------------------------------------------------------------------------
void ZoombiniTransitionXfer::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// Only XFER_0 and XFER_5 use SCRS-driven animation with callbacks.
	if (_xferView != XFER_ROUTE0_FROM_ISLE && _xferView != XFER_ROUTE5_TO_TOWN)
		return;

	// IDA xfer_initAndRunTransition @ 0x46761E: when the completion counter
	// activates the final env SCRB (XFER_5: 6108 arrival animation), it also
	// installs xfer_commitDestAndTriggerTransition as the runner's frame-event
	// callback. Frame event 30 of 6108 then triggers the page transition. We
	// model that here so the to-town animation finishes before close fires,
	// rather than racing the 300-frame auto-close.
	if (eventCode == 30 && _finalEnvScrbId != 0 && feature == _scrbFeatures.find(_finalEnvScrbId)) {
		_closureFrame = 1;
		_xferSoundId = 0;
		return;
	}

	// The feature must be a snoid for body arrangement and visibility operations.
	ZmbSnoid *snoid = dynamic_cast<ZmbSnoid *>(feature);

	if (eventCode > 26) {
		// ---------------------------------------------------------------
		// Event 50: XFER_5 only — activate env SCRB (IDA: word_4B9802).
		// IDA: ++town_displayedZmbCount; initFeatureRunnerWithScrb(0, 0, 0, word_4B9802).
		// ---------------------------------------------------------------
		if (eventCode == 50) {
			++_xfer5DisplayedTownCount;
			if (_xfer5EventScrbId != 0)
				activateEnvScrb(_xfer5EventScrbId);
		}
		// ---------------------------------------------------------------
		// Events 240-243: Set pending body arrangement override.
		// IDA: word_4B97E0 = scrbIdx - 239 (applied on next event 0).
		// ---------------------------------------------------------------
		else if (eventCode >= kZmbAnimEvent240_BodyArrangePendFirst && eventCode <= kZmbAnimEvent243_BodyArrangePendLast) {
			_bodyArrangementOverride = eventCode - (kZmbAnimEvent240_BodyArrangePendFirst - 1); // 1-4
		}
		// ---------------------------------------------------------------
		// Events 250-253: Direct body arrangement change.
		// IDA: zmbRunner_setAnimShape(scrbIdx - 250, callbackData+48).
		// ---------------------------------------------------------------
		else if (eventCode >= kZmbAnimEvent250_BodyArrangeDirectFirst && eventCode <= kZmbAnimEvent253_BodyArrangeDirectLast) {
			if (snoid)
				snoid->setBodyArrangement(eventCode - kZmbAnimEvent250_BodyArrangeDirectFirst);
		}
	} else if (eventCode == 26) {
		// ---------------------------------------------------------------
		// Event 26: Animation complete — reset body arrangement, link, count.
		// IDA: zmbRunner_setAnimShape(0, pZmb) — reset to front arrangement.
		// IDA: linkFeatureRunner(word_4B97E2, 0, runnerIdx) — link before env overlay.
		// IDA: if (word_4B97E4 >= 0) ++word_4B97E4.
		// ---------------------------------------------------------------
		if (snoid) {
			snoid->setBodyArrangement(0);

			// IDA: linkFeatureRunner(word_4B97E2, 0, runnerIdx) — link snoid before env overlay.
			// In ScummVM, adding OVERLAY moves the snoid into overlayList (rendered before
			// normalList where the env overlay 5100/6104 resides), achieving the "behind" effect.
			snoid->addFlag(ZmbFeature::FLAG_04000000_OVERLAY);
		}

		if (_completionCounter >= 0)
			++_completionCounter;
	} else if (eventCode == kZmbAnimEventM1_End) {
		// End-of-animation (PLAY_ONCE completion). No special handling needed.
	} else if (eventCode == 0) {
		// ---------------------------------------------------------------
		// Event 0: Toggle visibility, apply pending arrangement, inc cycle.
		// IDA: *(callbackData+290) = *(callbackData+290) == 0 — toggle render.
		// IDA: if word_4B97E0: setAnimShape(word_4B97E0 - 1), clear override.
		// IDA: ++*(callbackData+288) — increment cycle counter.
		// IDA: if XFER_0 && cycleCount == 2: linkFeatureRunner(word_4B97E2, 1, idx).
		// ---------------------------------------------------------------
		if (snoid) {
			// Toggle render visibility.
			if (snoid->isRenderActivated())
				snoid->deactivateRender();
			else
				snoid->activateRender();

			// Apply pending body arrangement override (set by events 240-243).
			if (_bodyArrangementOverride != 0) {
				snoid->setBodyArrangement(_bodyArrangementOverride - 1);
				_bodyArrangementOverride = 0;
			}

			// Increment per-snoid SCRS cycle counter.
			snoid->_scrsAnimCycleCount++;

			// XFER_0: after 2 visibility cycles, link snoid after the env overlay.
			// IDA: linkFeatureRunner(word_4B97E2, 1, runnerIdx) — link snoid after env overlay.
			// In ScummVM, removing OVERLAY moves the snoid from overlayList into entityList
			// (merged after normalList where env overlay 5100 resides), achieving the "in front" effect.
			if (_xferView == XFER_ROUTE0_FROM_ISLE && snoid->_scrsAnimCycleCount == 2)
				snoid->removeFlag(ZmbFeature::FLAG_04000000_OVERLAY);
		}
	} else if (eventCode >= 10 && eventCode <= 11) {
		// ---------------------------------------------------------------
		// Events 10-11: One-shot env SCRB activation (XFER_0 only).
		// IDA: if word_4B97D4[scrbIdx]: clear flag, find runner word_4B97C8[scrbIdx],
		//      set bitmask = 0x188000, loadSCRB(1, 0, runner).
		// Event 10 → SCRB 5102, Event 11 → SCRB 5103.
		// ---------------------------------------------------------------
		if (_xferView == XFER_ROUTE0_FROM_ISLE) {
			uint16 flagIdx = eventCode - 10;
			if (flagIdx < 2 && _envEventTriggerFlags[flagIdx]) {
				_envEventTriggerFlags[flagIdx] = false;
				// Activate the corresponding env SCRB (5102 for event 10, 5103 for event 11).
				uint16 envScrbId = _xferShapesId + 2 + flagIdx; // 5102 or 5103
				activateEnvScrb(envScrbId);
			}
		}
	}
}

// ============================================================
// Route Path Flood-Fill Implementation (XFER_1-4 only)
// IDA: xfer_onPostRenderRoutePath (0x468457), xfer_initRoutePathGrid
// (0x468078), xfer_expandRoutePathFloodFill (0x4681A8),
// reserveGridSlot (0x468312), rodmap_selectRouteBand (0x4683D4).
// ============================================================

// Seed coordinates from IDA: word_4A7EA0 — 16 (x,y) word pairs.
// Indexed as kRoutePathSeeds[seedIdx * 2] for X, [seedIdx * 2 + 1] for Y.
// seedIdx = routePathLevel + 4 * xferView - 5.
static const int16 kRoutePathSeeds[] = {
	// XFER_1 bands 1-4
	3,
	105,
	130,
	146,
	1,
	2,
	6,
	60,
	// XFER_2 bands 1-4
	42,
	194,
	1,
	106,
	1,
	1,
	1,
	4,
	// XFER_3 bands 1-4
	1,
	1,
	1,
	1,
	1,
	53,
	102,
	162,
	// XFER_4 bands 1-4
	1,
	12,
	57,
	154,
	1,
	1,
	2,
	109,
};

// IDA: word_4A7E76 / word_4A7E7A — overlapping route view slot tables.
// word_4A7E76 starts at index 0 (22 entries), word_4A7E7A = word_4A7E76 + 2.
// Pre-slot lookup: kRouteViewSlotTable[result + tableOffset]
// Band slot lookup: kRouteViewSlotTable[result + tableOffset - firstBandSlot + 2]
static const int16 kRouteViewSlotTable[22] = {
	0, 0,                 // word_4A7E76[0..1] (XFER_1 pre-slot padding)
	1, 2, 3, 4,           // word_4A7E76[2..5]  / word_4A7E7A[0..3]
	4, 5, 6, 7,           // word_4A7E76[6..9]  / word_4A7E7A[4..7]
	11, 4, 8, 9, 10, 16,  // word_4A7E76[10..15] / word_4A7E7A[8..13]
	11, 12, 13, 14, 15, 3 // word_4A7E76[16..21] / word_4A7E7A[14..19]
};

// IDA: readPuzzleLevelFromState (0x46788A) + xfer_init post-processing.
// Builds the per-puzzle completion array from game state, then applies the
// route slot fix-up and sets _routeProgressLevel for shape variant selection.
void ZoombiniTransitionXfer::buildPuzzleCompletionArray() {
	const ZmbStateFile &state = _vm->_state->_f;

	// First loop (IDA: 0x467894-0x4679A8): read per-SI-page completion level.
	// SI numbering matches IDA, so `_puzzleCompletionArr[siPage]` lines up with
	// the IDA `pPuzzleLevelArr` slots consumed by `_routeSlotIndex` lookups,
	// `kRouteViewSlotTable`, and `routeView_updateSlots`.
	for (int i = 0; i <= 16; i++) {
		if (_vm->_state->inPracticeMode()) {
			_puzzleCompletionArr[i] = static_cast<int8>(CLIP<uint16>(_vm->_state->_practiceLevel, 0, 4));
		} else {
			_puzzleCompletionArr[i] = static_cast<int8>(readPuzzleLevelFlag(state, static_cast<ZMB_SI_PAGE>(i)));
		}
	}

	// Determine global route slot index (IDA: v4 in readPuzzleLevelFromState).
	switch (_nextPageType) {
	case ZoombiniPageType::kBridge:
		_routeSlotIndex = 1;
		break;
	case ZoombiniPageType::kTunnels:
		_routeSlotIndex = 2;
		break;
	case ZoombiniPageType::kPizza:
		_routeSlotIndex = 3;
		break;
	case ZoombiniPageType::kBasecamp1:
		_routeSlotIndex = 4;
		break;
	case ZoombiniPageType::kFerry:
		_routeSlotIndex = 5;
		break;
	case ZoombiniPageType::kLilly:
		_routeSlotIndex = 6;
		break;
	case ZoombiniPageType::kSlides:
		_routeSlotIndex = 7;
		break;
	case ZoombiniPageType::kBasecamp2:
		_routeSlotIndex = (_vm->_xferSrcPage == ZMB_SI_SLIDES_09) ? 11 : 16;
		break;
	case ZoombiniPageType::kFleens:
		_routeSlotIndex = 8;
		break;
	case ZoombiniPageType::kHotel:
		_routeSlotIndex = 9;
		break;
	case ZoombiniPageType::kNet:
		_routeSlotIndex = 10;
		break;
	case ZoombiniPageType::kCaves:
		_routeSlotIndex = 12;
		break;
	case ZoombiniPageType::kSmoke:
		_routeSlotIndex = 13;
		break;
	case ZoombiniPageType::kMaze:
		_routeSlotIndex = 14;
		break;
	case ZoombiniPageType::kTown:
		_routeSlotIndex = 15;
		break;
	default:
		_routeSlotIndex = 0;
		break;
	}

	if (_routeSlotIndex == 0)
		return;

	// IDA: pPuzzleLevelArr[v4] = puzzleFlag - 1; then check < 1.
	// puzzleFlag = _routePathColorLevel (word_4B97FA), already computed.
	_puzzleCompletionArr[_routeSlotIndex] = static_cast<int8>(_routePathColorLevel - 1);
	if (_puzzleCompletionArr[_routeSlotIndex] < 1) {
		if (_routeProgressLevel < 0)
			_routeProgressLevel = static_cast<int16>(_routePathColorLevel - 1);
		_puzzleCompletionArr[_routeSlotIndex] = -1;
	}

	// IDA: xfer_init post-processing (0x46621D-0x46623C).
	// wShuffledPuzzleId maps source SI page → predecessor's completion slot.
	// If readPuzzleLevelFromState set it to -1, restore it to 1 so the
	// route view shows the departure as completed.
	// Values from IDA: wShuffledPuzzleId in xfer_initAndRunTransition caller
	// switch (adjusted for ScummVM's SI enum: Tunnels=03, Caves=14).
	int16 shuffledId = -1;
	switch (_vm->_xferSrcPage) {
	case ZMB_SI_PICKER_01:
		shuffledId = 0;
		break;
	case ZMB_SI_BRIDGE_02:
		shuffledId = 1;
		break;
	case ZMB_SI_TUNNELS_03:
		shuffledId = 2;
		break;
	case ZMB_SI_PIZZA_04:
		shuffledId = 3;
		break;
	case ZMB_SI_BC1_NORTH_05:
		shuffledId = 4;
		break;
	case ZMB_SI_FERRY_07:
		shuffledId = 5;
		break;
	case ZMB_SI_LILLY_08:
		shuffledId = 6;
		break;
	case ZMB_SI_SLIDES_09:
		shuffledId = 7;
		break;
	case ZMB_SI_BC1_SOUTH_06:
		shuffledId = 4;
		break;
	case ZMB_SI_FLEENS_10:
		shuffledId = 8;
		break;
	case ZMB_SI_HOTEL_11:
		shuffledId = 9;
		break;
	case ZMB_SI_NET_12:
		shuffledId = 10;
		break;
	case ZMB_SI_BASECAMP2_13:
		shuffledId = 11;
		break;
	case ZMB_SI_CAVES_14:
		shuffledId = 12;
		break;
	case ZMB_SI_SMOKE_15:
		shuffledId = 13;
		break;
	case ZMB_SI_MAZE_16:
		shuffledId = 14;
		break;
	default:
		break;
	}

	if (0 <= shuffledId && shuffledId < 17 && _puzzleCompletionArr[shuffledId] < 0)
		_puzzleCompletionArr[shuffledId] = 1;

	// IDA: xfer_wRouteProgressLevel = xfer_puzzleCompletionArr[xferParam];
	//      xfer_puzzleCompletionArr[xferParam] = -1;
	_routeProgressLevel = _puzzleCompletionArr[_routeSlotIndex];
	_puzzleCompletionArr[_routeSlotIndex] = -1;
}

// IDA: xfer_updateRouteViewSlots (0x467B2C) sets word_4B9804 (route band
// position 1-4) by matching the destination puzzle slot against the
// word_4A7E7A route slot table.  We compute the same result directly from
// the source SI page that started this xfer transition.
void ZoombiniTransitionXfer::computeRoutePathBand() {
	switch (_vm->_xferSrcPage) {
	// Route 1 — Big Bad Hungry
	case ZMB_SI_BRIDGE_02:
		_routePathBand = 2;
		break; // dest Tunnels
	case ZMB_SI_TUNNELS_03:
		_routePathBand = 3;
		break; // dest Pizza
	case ZMB_SI_PIZZA_04:
		_routePathBand = 4;
		break; // dest BC1
	// Route 2 — Who's Bayou
	case ZMB_SI_BC1_NORTH_05:
		_routePathBand = 1;
		break; // dest Ferry
	case ZMB_SI_FERRY_07:
		_routePathBand = 2;
		break; // dest Lilly
	case ZMB_SI_LILLY_08:
		_routePathBand = 3;
		break; // dest Slides
	case ZMB_SI_SLIDES_09:
		_routePathBand = 4;
		break; // dest BC2
	// Route 3 — Deep Dark Forest
	case ZMB_SI_BC1_SOUTH_06:
		_routePathBand = 1;
		break; // dest Fleens
	case ZMB_SI_FLEENS_10:
		_routePathBand = 2;
		break; // dest Hotel
	case ZMB_SI_HOTEL_11:
		_routePathBand = 3;
		break; // dest Net
	case ZMB_SI_NET_12:
		_routePathBand = 4;
		break; // dest BC2
	// Route 4 — Mountain of Despair
	case ZMB_SI_BASECAMP2_13:
		_routePathBand = 1;
		break; // dest Caves
	case ZMB_SI_CAVES_14:
		_routePathBand = 2;
		break; // dest Smoke
	case ZMB_SI_SMOKE_15:
		_routePathBand = 3;
		break; // dest Maze
	default:
		_routePathBand = 1;
		break;
	}
}

// IDA: readPuzzleLevelFromState (0x46788A) — Helper: read per-puzzle
// completion level from game state flags for a given SI page.
// Returns the highest completed difficulty level (0-4) encoded as bit flags
// in the state: bit 0 → level 1, bit 1 → level 2, bit 2 → level 3, bit 3 → level 4.
//
// IDA `pbPuzzleLevelFlagArr` is a 15-byte array where the leading 3 bytes
// are reserved (dummy + perfect-streak WORD flag), and puzzle slots live at
// indices [3..14] for DI pages [7..18]. ScummVM's `_levelFlagPageArr` now
// shares the same layout byte-for-byte so that saves interchange with IDA.
//
// IDA index formulas (siPage + N):
//   SI 1-3 (Picker/Bridge/Tunnels): pbPuzzleLevelFlagArr[siPage + 2]
//   SI 4 (Pizza): route flag bBigBadHungry
//   SI 5-10 (BC1N/BC1S/Ferry/Lilly/Slides/Fleens): pbPuzzleLevelFlagArr[siPage + 1]
//   SI 11 (Hotel): route flag bLoWhosBayouHiDeepDark low nibble
//   SI 12-14 (Net/BC2/Caves): pbPuzzleLevelFlagArr[siPage]
//   SI 15 (Smoke): route flag bMontDespair
//   SI 16 (Maze): route flag bLoWhosBayouHiDeepDark high nibble
uint16 ZoombiniTransitionXfer::readPuzzleLevelFlag(
	const ZmbStateFile &state, ZMB_SI_PAGE siPage) {
	uint8 flag = 0;
	switch (siPage) {
	case ZMB_SI_TOWN_00:
		return 1; // Always level 1

	// BBH puzzle pages: pbPuzzleLevelFlagArr[siPage + 2]
	case ZMB_SI_PICKER_01:
	case ZMB_SI_BRIDGE_02:
	case ZMB_SI_TUNNELS_03:
		flag = state._levelFlagPageArr[siPage + 2];
		break;

	// BBH route completion flag
	case ZMB_SI_PIZZA_04:
		flag = state._levelFlagRouteBigBadHungry;
		break;

	// WB/DDF puzzle pages: pbPuzzleLevelFlagArr[siPage + 1]
	case ZMB_SI_BC1_NORTH_05:
	case ZMB_SI_BC1_SOUTH_06:
	case ZMB_SI_FERRY_07:
	case ZMB_SI_LILLY_08:
	case ZMB_SI_SLIDES_09:
	case ZMB_SI_FLEENS_10:
		flag = state._levelFlagPageArr[siPage + 1];
		break;

	// WB route completion flag (low nibble)
	case ZMB_SI_HOTEL_11:
		flag = state._levelFlagLoWhosBayouHiDeepDarkForest & 0x0F;
		break;

	// DDF/MD puzzle pages: pbPuzzleLevelFlagArr[siPage]
	case ZMB_SI_NET_12:
	case ZMB_SI_BASECAMP2_13:
	case ZMB_SI_CAVES_14:
		flag = state._levelFlagPageArr[siPage];
		break;

	// MD route completion flag
	case ZMB_SI_SMOKE_15:
		flag = state._levelFlagRouteMontDespair;
		break;

	// DDF route completion flag (high nibble)
	case ZMB_SI_MAZE_16:
		flag = (state._levelFlagLoWhosBayouHiDeepDarkForest & 0xF0) >> 4;
		break;

	default:
		return 0;
	}

	// Convert 4-bit flag to highest completed level (0-4).
	uint16 level = 0;
	if (flag & 1)
		level = 1;
	if (flag & 2)
		level = 2;
	if (flag & 4)
		level = 3;
	if (flag & 8)
		level = 4;
	return level;
}

// IDA: readPuzzleLevelFromState (0x46788A) sets word_4B97FA (color level
// 1-4) from the route's puzzle difficulty progression.
//
// For the first puzzle of each route (Bridge, Ferry, Fleens, Caves):
//   color = puzzle_getRouteDifficultyLevel() + 1 (with active page
//           temporarily set to destination for correct route lookup).
//
// For subsequent puzzles: color = preceding puzzle's completion level
//   read from per-puzzle state flags via readPuzzleLevelFlag().
//
// The predecessor SI page for each destination is taken from the second
// switch in readPuzzleLevelFromState (IDA: pPuzzleLevelArr[] indices).
void ZoombiniTransitionXfer::computeRoutePathColorLevel() {
	// Practice mode: all puzzles use the practice difficulty level directly.
	if (_vm->_state->inPracticeMode()) {
		_routePathColorLevel = CLIP<uint16>(_vm->_state->_practiceLevel, 1, 4);
		return;
	}

	const ZmbStateFile &state = _vm->_state->_f;
	uint16 colorLevel = 0;

	switch (_nextPageType) {
	// ---------------------------------------------------------------
	// First puzzle of each route: use destination's route level + 1.
	// IDA: puzzle_getRouteDifficultyLevel() + 1 with wActivePuzzleId
	// temporarily set to xfer_wDestPuzzleId.
	// ---------------------------------------------------------------
	case ZoombiniPageType::kBridge:
	case ZoombiniPageType::kFerry:
	case ZoombiniPageType::kFleens:
	case ZoombiniPageType::kCaves: {
		// Temporarily swap currentPage to destination for correct route lookup.
		ZMB_DI_PAGE savedPage = state._currentPage;
		_vm->_state->_f._currentPage = static_cast<ZMB_DI_PAGE>(
			static_cast<uint16>(_nextPageType));
		colorLevel = _vm->_state->readActivePageRouteLevel() + 1;
		_vm->_state->_f._currentPage = savedPage;
		break;
	}

	// ---------------------------------------------------------------
	// BBH route: predecessor puzzle's completion level.
	// Tunnels ← Picker, Pizza ← Bridge, BC1 ← Tunnels
	// ---------------------------------------------------------------
	case ZoombiniPageType::kTunnels:
		colorLevel = readPuzzleLevelFlag(state, ZMB_SI_PICKER_01);
		break;
	case ZoombiniPageType::kPizza:
		colorLevel = readPuzzleLevelFlag(state, ZMB_SI_BRIDGE_02);
		break;
	case ZoombiniPageType::kBasecamp1:
		colorLevel = readPuzzleLevelFlag(state, ZMB_SI_TUNNELS_03);
		break;

	// ---------------------------------------------------------------
	// WB route: predecessor puzzle's completion level.
	// Lilly ← BC1N, Slides ← BC1S, BC2 ← Ferry (via Slides) or Fleens (via Net)
	// ---------------------------------------------------------------
	case ZoombiniPageType::kLilly:
		colorLevel = readPuzzleLevelFlag(state, ZMB_SI_BC1_NORTH_05);
		break;
	case ZoombiniPageType::kSlides:
		colorLevel = readPuzzleLevelFlag(state, ZMB_SI_BC1_SOUTH_06);
		break;

	// ---------------------------------------------------------------
	// DDF route: predecessor puzzle's completion level.
	// Hotel ← Lilly, Net ← Slides
	// ---------------------------------------------------------------
	case ZoombiniPageType::kHotel:
		colorLevel = readPuzzleLevelFlag(state, ZMB_SI_LILLY_08);
		break;
	case ZoombiniPageType::kNet:
		colorLevel = readPuzzleLevelFlag(state, ZMB_SI_SLIDES_09);
		break;

	// ---------------------------------------------------------------
	// BC2: Slides → BC2 reads Ferry's flag, Net → BC2 reads Fleens's flag.
	// IDA: wLastRouteVal_4B0428 == 12 distinguishes the two paths.
	// In xfer, v3=12 only for source=Slides(SI 8).
	// ---------------------------------------------------------------
	case ZoombiniPageType::kBasecamp2:
		if (_vm->_xferSrcPage == ZMB_SI_SLIDES_09)
			colorLevel = readPuzzleLevelFlag(state, ZMB_SI_FERRY_07);
		else
			colorLevel = readPuzzleLevelFlag(state, ZMB_SI_FLEENS_10);
		break;

	// ---------------------------------------------------------------
	// MD route: predecessor puzzle's completion level.
	// Smoke ← Net, Maze ← BC2
	// ---------------------------------------------------------------
	case ZoombiniPageType::kSmoke:
		colorLevel = readPuzzleLevelFlag(state, ZMB_SI_NET_12);
		break;
	case ZoombiniPageType::kMaze:
		colorLevel = readPuzzleLevelFlag(state, ZMB_SI_BASECAMP2_13);
		break;

	// ---------------------------------------------------------------
	// Town (XFER_5): Last predecessor = Caves.
	// ---------------------------------------------------------------
	case ZoombiniPageType::kTown:
		colorLevel = readPuzzleLevelFlag(state, ZMB_SI_CAVES_14);
		break;

	default:
		colorLevel = 1;
		break;
	}

	_routePathColorLevel = CLIP<uint16>(colorLevel, 1, 4);
}

// IDA: xfer_updateRouteViewSlots (0x467B2C).
// Pre-render shape callback on the MAIN SCRB: remaps hotspot shape indices
// based on puzzle completion so that completed bands show level-colored
// shape variants and uncompleted bands are hidden.
//
// Shape remapping formula: shapeIdx = slot + 4 * completionLevel
// E.g. band slot 5 at level 1 → shape 9; at level 2 → shape 13; etc.
// The tBMP contains pre-colored variants for each band × level combination.
void ZoombiniTransitionXfer::routeView_updateSlots(
	ZmbFeature *feature, ZmbHotspotGroup *hsGroup,
	Common::Array<ZmbHotspot> &hotspots) {

	// Determine per-view parameters (IDA: 0x467B34-0x467B8E).
	int16 tableOffset, firstBandSlot, lastSlot;
	switch (_xferView) {
	case XFER_ROUTE1_BIG_BAD_HUNGRY:
		tableOffset = 0;
		firstBandSlot = 5;
		lastSlot = 8;
		break;
	case XFER_ROUTE2_WHOS_BAYOU:
		tableOffset = 5;
		firstBandSlot = 6;
		lastSlot = 9;
		break;
	case XFER_ROUTE3_DEEP_DARK_FOREST:
		tableOffset = 10;
		firstBandSlot = 6;
		lastSlot = 9;
		break;
	case XFER_ROUTE4_MOUNTAIN_OF_DESPAIR:
		tableOffset = 15;
		firstBandSlot = 6;
		lastSlot = 9;
		break;
	default:
		return;
	}

	// Build shape remapping table v9[0..lastSlot] (IDA: 0x467B94-0x467D8B).
	// v9[shapeId] = 0 means remove hotspot, >0 means remap to that shape.
	int16 v9[10] = {};

	for (int16 result = 1; result <= lastSlot; result++) {
		v9[result] = 0;

		if (firstBandSlot > result) {
			// Pre-slot (decoration) hotspot (IDA: 0x467D17-0x467D8B).
			if (tableOffset == 0) {
				// XFER_1 (BBH): direct puzzleCompletionArr lookup.
				if (_puzzleCompletionArr[result])
					v9[result] = result;
			} else {
				int16 puzzleIdx = kRouteViewSlotTable[result + tableOffset];
				if (_puzzleCompletionArr[puzzleIdx]) {
					v9[result] = result;
				} else if (puzzleIdx == 11) {
					// Hotel slot → show if Maze (index 16) was completed.
					if (_puzzleCompletionArr[16])
						v9[result] = result;
				} else if (puzzleIdx == 16 && _puzzleCompletionArr[11]) {
					// Maze slot → show if Hotel (index 11) was completed.
					v9[result] = result;
				}
			}
		} else {
			// Band slot (IDA: 0x467BC9-0x467D0B).
			int16 bandIdx = kRouteViewSlotTable[result + tableOffset - firstBandSlot + 2];

			// Match against current route slot (IDA: word_4B97F8 consumption).
			// _routeSlotIndex matching is only for setting word_4B9804 (band
			// position), which computeRoutePathLevel() already handles.

			int8 compLevel = _puzzleCompletionArr[bandIdx];
			if (compLevel > 0) {
				// Completed band: remap to level-colored shape variant.
				v9[result] = result + 4 * compLevel;
			} else {
				// Not completed or current band (IDA: 0x467CAC-0x467D0B).
				if (_routeProgressLevel < 0)
					_routeProgressLevel = 0;
				if (firstBandSlot == result) {
					// First band in this view: always use progress level.
					v9[result] = result + 4 * _routeProgressLevel;
				} else if (firstBandSlot < result &&
						   compLevel == -1 &&
						   v9[result - 1] > lastSlot) {
					// Cascading band: predecessor was also remapped above
					// lastSlot threshold. (IDA: 0x467CF5 condition.)
					v9[result] = result + 4 * _routeProgressLevel;
				}
			}
		}
	}

	// Apply remapping to hotspots (IDA: 0x467D94-0x467DCC).
	Common::Array<ZmbHotspot> remapped;
	for (uint i = 0; i < hotspots.size(); i++) {
		int16 shapeId = hotspots[i]._shapeIdx;
		if (shapeId <= 0 || shapeId > lastSlot)
			continue;
		if (v9[shapeId]) {
			hotspots[i]._shapeIdx = v9[shapeId];
			remapped.push_back(hotspots[i]);
		}
		// else: remove (skip insertion = effectively hotspot_removeFirstEntry)
	}
	hotspots = remapped;
}

// IDA: rodmap_selectRouteBand (0x4683D4).
// Pre-render shape callback: keep only the target band's hotspot in the
// copy, removing all others so only one shape renders.
void ZoombiniTransitionXfer::routePath_selectBand(
	ZmbFeature *feature, ZmbHotspotGroup *hsGroup,
	Common::Array<ZmbHotspot> &hotspots) {
	if (hotspots.empty() || _routePathBand < 1 || _routePathBand > 4)
		return;

	uint16 targetIdx = _routePathBand - 1;
	if (targetIdx >= hotspots.size())
		return;

	// IDA copies target entry to slot 0of and sets slot 1's shape to 0
	// (terminator). In ScummVM, we just keep the target entry.
	ZmbHotspot target = hotspots[targetIdx];
	hotspots.clear();
	hotspots.push_back(target);
}

// IDA: xfer_onPostRenderRoutePath (0x468457).
// Custom render callback (_renderFunc) for the path overlay sub-feature.
// Runs flood-fill pixel modification on the shape surface, then calls
// the standard blitShapes to render the (now-modified) pixels.
ZmbRenderResult ZoombiniTransitionXfer::routePath_onPostRender(ZmbFeature *feature) {
	if (!feature->isRenderActivated() &&
		feature->hasFlag(ZmbFeature::FLAG_01000000_DEFER_RENDER))
		return ZmbRenderResult::kSkipped;

	// Frame-interval gate (IDA: chGetDrawnRect set when dNextRenderFrame <= renderTime).
	uint32 currentFrame = getCurrentFrameCounter();
	if (currentFrame >= _routePathNextFrame) {
		_routePathNextFrame = currentFrame + feature->getFrameInterval();

		if (_routePathCounter == 0) {
			// First active frame: initialize flood-fill grid from shape pixels.
			MohawkSurface *mohawkSurf = _vm->_gfx->findShape(
				feature->getResource(), _routePathBand);
			if (mohawkSurf) {
				Graphics::Surface *surf = mohawkSurf->getSurface();
				_routePathPixels = (byte *)surf->getPixels();
				_routePathWidth = surf->w;
				_routePathHeight = surf->h;
				_routePathPitch = surf->pitch;

				// Seed index: band + 4 * view - 5.
				uint16 seedIdx = _routePathBand + 4 * _xferView - 5;
				if (seedIdx >= 16)
					seedIdx = 0;
				int16 seedX = kRoutePathSeeds[seedIdx * 2];
				int16 seedY = kRoutePathSeeds[seedIdx * 2 + 1];

				// Select mark/replace palette indices from XferRoutePathLevelColor.
				// Level 1: "10/." → Back1/Back2 → LevelOneColor1/Color2
				// Level 2: "3210" → LevelOneColor1/Color2 → LevelTwoColor1/Color2
				// Level 3: "5432" → LevelTwoColor1/Color2 → LevelThreeColor1/Color2
				// Level 4: "7654" → LevelThreeColor1/Color2 → LevelFourColor1/Color2
				using Clr = ZoombiniGraphics::XferRoutePathLevelColor;
				byte mark1, mark2, replace1, replace2;
				switch (_routePathColorLevel) {
				case 1:
					mark1 = Clr::kRoutePathColor2E_LevelOneBack1;
					mark2 = Clr::kRoutePathColor2F_LevelOneBack2;
					replace1 = Clr::kRoutePathColor30_LevelOneColor1;
					replace2 = Clr::kRoutePathColor31_LevelOneColor2;
					break;
				case 2:
					mark1 = Clr::kRoutePathColor30_LevelOneColor1;
					mark2 = Clr::kRoutePathColor31_LevelOneColor2;
					replace1 = Clr::kRoutePathColor32_LevelTwoColor1;
					replace2 = Clr::kRoutePathColor33_LevelTwoColor2;
					break;
				case 3:
					mark1 = Clr::kRoutePathColor32_LevelTwoColor1;
					mark2 = Clr::kRoutePathColor33_LevelTwoColor2;
					replace1 = Clr::kRoutePathColor34_LevelThreeColor1;
					replace2 = Clr::kRoutePathColor35_LevelThreeColor2;
					break;
				case 4:
					mark1 = Clr::kRoutePathColor34_LevelThreeColor1;
					mark2 = Clr::kRoutePathColor35_LevelThreeColor2;
					replace1 = Clr::kRoutePathColor36_LevelFourColor1;
					replace2 = Clr::kRoutePathColor37_LevelFourColor2;
					break;
				default:
					error("Invalid route path color level: %u", _routePathColorLevel);
					break;
				}

				routePath_initGrid(seedX, seedY, mark1, mark2, replace1, replace2);
			}
		} else {
			routePath_expandFloodFill(_routePathCounter);
		}

		_routePathCounter += 7;
		if (_routePathCounter > 1000)
			_routePathCounter = 1000;
	}

	// Standard blit renders the (now modified) shape pixels.
	return blitShapes(feature);
}

// IDA: xfer_initRoutePathGrid (0x468078).
// Clear BFS queue, scan shape pixels replacing value 1→mark1 and 2→mark2,
// seed queue[0] with initial coordinates.
void ZoombiniTransitionXfer::routePath_initGrid(
	int16 seedX, int16 seedY,
	byte mark1, byte mark2, byte replace1, byte replace2) {
	// Clear BFS queue.
	for (int i = 0; i < kRoutePathQueueSize; i++) {
		_routePathQueueActive[i] = false;
		_routePathQueueX[i] = 0;
		_routePathQueueY[i] = 0;
	}

	_routePathTotalPixels = 0;
	_routePathMark1 = mark1;
	_routePathMark2 = mark2;
	_routePathReplace1 = replace1;
	_routePathReplace2 = replace2;

	// Scan all pixels: replace original marker values (1, 2) with mark colors.
	// IDA: replaces a7 (=1) with a5 (=mark1), a6 (=2) with a4 (=mark2).
	if (_routePathPixels) {
		byte *row = _routePathPixels;
		for (uint16 y = 0; y < _routePathHeight; y++) {
			for (uint16 x = 0; x < _routePathWidth; x++) {
				byte val = row[x];
				if (val == 1) {
					_routePathTotalPixels++;
					row[x] = mark1;
				}
				if (val == 2) {
					_routePathTotalPixels++;
					row[x] = mark2;
				}
			}
			row += _routePathPitch;
		}
	}

	_routePathRemainingPixels = _routePathTotalPixels;

	// Clamp seed coordinates to grid bounds.
	if (static_cast<uint16>(seedX) >= _routePathWidth)
		seedX = _routePathWidth - 1;
	if (static_cast<uint16>(seedY) >= _routePathHeight)
		seedY = _routePathHeight - 1;

	// Seed BFS queue slot 0.
	_routePathQueueActive[0] = true;
	_routePathQueueX[0] = seedX;
	_routePathQueueY[0] = seedY;
}

// IDA: xfer_expandRoutePathFloodFill (0x4681A8).
// BFS expansion: process active queue cells, expand to 8 neighbors.
// Repeat until remaining pixels <= target (progress threshold for this frame).
void ZoombiniTransitionXfer::routePath_expandFloodFill(uint32 counter) {
	if (!_routePathPixels || _routePathTotalPixels == 0)
		return;

	// Target remaining count: fewer pixels left as counter approaches 1000.
	uint32 target = _routePathTotalPixels -
					(counter * _routePathTotalPixels / 1000);

	while (_routePathRemainingPixels > target) {
		uint32 prevRemaining = _routePathRemainingPixels;

		for (int i = 0; i < kRoutePathQueueSize; i++) {
			if (!_routePathQueueActive[i])
				continue;

			_routePathQueueActive[i] = false;
			int16 x = _routePathQueueX[i];
			int16 y = _routePathQueueY[i];

			byte *pixel = _routePathPixels + x + _routePathPitch * y;

			// Expand south (y + 1)
			if (static_cast<uint16>(y + 1) < _routePathHeight) {
				byte *southRow = pixel + _routePathPitch;
				routePath_reserveSlot(y + 1, x, southRow);
				if (x > 0)
					routePath_reserveSlot(y + 1, x - 1, southRow - 1);
				if (static_cast<uint16>(x + 1) < _routePathWidth)
					routePath_reserveSlot(y + 1, x + 1, southRow + 1);
			}

			// Expand north (y - 1)
			// IDA: `dec eax; jz skip` → y > 0 (bounds guard).
			if (y > 0) {
				byte *northRow = pixel - _routePathPitch;
				routePath_reserveSlot(y - 1, x, northRow);
				if (x > 0)
					routePath_reserveSlot(y - 1, x - 1, northRow - 1);
				if (static_cast<uint16>(x + 1) < _routePathWidth)
					routePath_reserveSlot(y - 1, x + 1, northRow + 1);
			}

			// Expand left/right (same row)
			if (x > 0)
				routePath_reserveSlot(y, x - 1, pixel - 1);
			if (static_cast<uint16>(x + 1) < _routePathWidth)
				routePath_reserveSlot(y, x + 1, pixel + 1);
		}

		// If no progress, force remaining to 0 to break the loop.
		if (prevRemaining == _routePathRemainingPixels) {
			_routePathRemainingPixels = 0;
			break;
		}
	}
}

// IDA: reserveGridSlot (0x468312).
// If *pixel matches mark1 or mark2, find an empty BFS queue slot,
// store (x, y), replace the pixel with the corresponding final color.
void ZoombiniTransitionXfer::routePath_reserveSlot(int16 y, int16 x, byte *pixel) {
	if (*pixel == _routePathMark1) {
		for (int i = 0; i < kRoutePathQueueSize; i++) {
			if (!_routePathQueueActive[i]) {
				_routePathQueueX[i] = x;
				_routePathQueueY[i] = y;
				_routePathQueueActive[i] = true;
				if (_routePathRemainingPixels > 0)
					_routePathRemainingPixels--;
				*pixel = _routePathReplace1;
				return;
			}
		}
	} else if (*pixel == _routePathMark2) {
		for (int i = 0; i < kRoutePathQueueSize; i++) {
			if (!_routePathQueueActive[i]) {
				_routePathQueueX[i] = x;
				_routePathQueueY[i] = y;
				_routePathQueueActive[i] = true;
				if (_routePathRemainingPixels > 0)
					_routePathRemainingPixels--;
				*pixel = _routePathReplace2;
				return;
			}
		}
	}
}

void ZoombiniTransitionXfer::close() {
	_routePathPixels = nullptr;
	_routePathFeature = nullptr;
	_vm->_xferSrcPage = ZMB_SI_MINUS1; // Reset for next xfer

	// IDA: puzzleXfer_onExit (0x46746B) restores word_4A4764 = 64 to
	// re-enable fidget/idle animations after the transition.
	_vm->_fidgetThreshold = 64;

	_vm->setNextPage(_nextPageType);
	ZoombiniTransition::close();
}

} // End of namespace Mohawk
