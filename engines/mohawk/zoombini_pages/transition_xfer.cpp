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

// Route path flood-fill seed coordinates table (IDA: word_4A7EA0).
// 16 entries: 4 views × 4 route bands, indexed by (view-1)*4 + (route-1).
// IDA: v2 = word_4B9804 + 4 * xfer_wViewType - 5
static const Common::Point kRoutePathSeeds[16] = {
	// XFER_1 (BigBadHungry): indices 0-3
	{3, 105},
	{130, 146},
	{1, 2},
	{6, 60},
	// XFER_2 (WhosBayou): indices 4-7
	{42, 194},
	{1, 106},
	{1, 1},
	{1, 4},
	// XFER_3 (DeepDarkForest): indices 8-11
	{1, 1},
	{1, 1},
	{1, 53},
	{102, 162},
	// XFER_4 (MountainDespair): indices 12-15
	{1, 12},
	{57, 154},
	{1, 1},
	{2, 109},
};

ZoombiniTransitionXfer::ZoombiniTransitionXfer(MohawkEngine_Zoombini *vm) : ZoombiniTransition(vm, ZoombiniPageType::kXfer) {
	_useFadeEffect = true;
}

ZoombiniTransitionXfer::~ZoombiniTransitionXfer() {
	delete[] _routePathPixels;
	_routePathPixels = nullptr;
}

void ZoombiniTransitionXfer::open() {
	openArchive(ZMB_MHK_XFER);
}

void ZoombiniTransitionXfer::setBackgroundMusic() {
}

void ZoombiniTransitionXfer::computeXferRoute() {
	// IDA: puzzleXfer_465FEE — determine xfer view (0-5) from source SI page.
	// Source SI page set by each page before calling setNextPage(kXfer).
	ZMB_SI_PAGE src = _vm->_xferSrcSiPage;

	switch (src) {
	case ZMB_SI_PICKER_01:
		// Picker → Bridge (first crossing on route 1)
		_xferView = XFER_ROUTE_FROM_ISLE;
		_nextPageType = ZoombiniPageType::kBridge;
		_xferBgId = kResBackgroundFromIsle;
		_xferShapesId = kResShapesFromIsle;
		_xferScrbCount = 9;
		break;
	case ZMB_SI_BRIDGE_02:
		// Bridge → Tunnels (route 1, second crossing)
		_xferView = XFER_ROUTE_BIG_BAD_HUNGRY;
		_nextPageType = ZoombiniPageType::kTunnels;
		_xferBgId = kResBackgroundBigBadHungry;
		_xferShapesId = kResShapesBigBadHungry;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_TUNNELS_03:
		// Tunnels → Pizza (route 1, third crossing)
		_xferView = XFER_ROUTE_BIG_BAD_HUNGRY;
		_nextPageType = ZoombiniPageType::kPizza;
		_xferBgId = kResBackgroundBigBadHungry;
		_xferShapesId = kResShapesBigBadHungry;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_PIZZA_04:
		// Pizza → Basecamp 1 (end of route 1)
		_xferView = XFER_ROUTE_BIG_BAD_HUNGRY;
		_nextPageType = ZoombiniPageType::kBasecamp1;
		_xferBgId = kResBackgroundBigBadHungry;
		_xferShapesId = kResShapesBigBadHungry;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_BC1_NORTH_05:
		// Basecamp 1 north exit → Ferry (route 2, first crossing)
		_xferView = XFER_ROUTE_WHOS_BAYOU;
		_nextPageType = ZoombiniPageType::kFerry;
		_xferBgId = kResBackgroundWhosBayou;
		_xferShapesId = kResShapesWhosBayou;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_BC1_SOUTH_06:
		// Basecamp 1 south exit → Lilly (route 2, alt first crossing)
		_xferView = XFER_ROUTE_WHOS_BAYOU;
		_nextPageType = ZoombiniPageType::kLilly;
		_xferBgId = kResBackgroundWhosBayou;
		_xferShapesId = kResShapesWhosBayou;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_FERRY_07:
		// Ferry → Slides
		_xferView = XFER_ROUTE_WHOS_BAYOU;
		_nextPageType = ZoombiniPageType::kSlides;
		_xferBgId = kResBackgroundWhosBayou;
		_xferShapesId = kResShapesWhosBayou;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_LILLY_08:
		// Lilly → Basecamp 2 (via route 2)
		_xferView = XFER_ROUTE_WHOS_BAYOU;
		_nextPageType = ZoombiniPageType::kBasecamp2;
		_xferBgId = kResBackgroundWhosBayou;
		_xferShapesId = kResShapesWhosBayou;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_SLIDES_09:
		// Slides → Fleens (route 3)
		_xferView = XFER_ROUTE_DEEP_DARK_FOREST;
		_nextPageType = ZoombiniPageType::kFleens;
		_xferBgId = kResBackgroundDeepDarkForest;
		_xferShapesId = kResShapesDeepDarkForest;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_FLEENS_10:
		// Fleens → Hotel (route 3)
		_xferView = XFER_ROUTE_DEEP_DARK_FOREST;
		_nextPageType = ZoombiniPageType::kHotel;
		_xferBgId = kResBackgroundDeepDarkForest;
		_xferShapesId = kResShapesDeepDarkForest;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_HOTEL_11:
		// Hotel → Net (route 3)
		_xferView = XFER_ROUTE_DEEP_DARK_FOREST;
		_nextPageType = ZoombiniPageType::kNet;
		_xferBgId = kResBackgroundDeepDarkForest;
		_xferShapesId = kResShapesDeepDarkForest;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_NET_12:
		// Net → Basecamp 2 (via route 3)
		_xferView = XFER_ROUTE_DEEP_DARK_FOREST;
		_nextPageType = ZoombiniPageType::kBasecamp2;
		_xferBgId = kResBackgroundDeepDarkForest;
		_xferShapesId = kResShapesDeepDarkForest;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_BASECAMP2_13:
		// Basecamp 2 → Caves (route 4)
		_xferView = XFER_ROUTE_MOUNTAIN_OF_DESPAIR;
		_nextPageType = ZoombiniPageType::kCaves;
		_xferBgId = kResBackgroundMountainOfDespair;
		_xferShapesId = kResShapesMountainOfDespair;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_CAVES_14:
		// Caves → Smoke (route 4)
		_xferView = XFER_ROUTE_MOUNTAIN_OF_DESPAIR;
		_nextPageType = ZoombiniPageType::kSmoke;
		_xferBgId = kResBackgroundMountainOfDespair;
		_xferShapesId = kResShapesMountainOfDespair;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_SMOKE_15:
		// Smoke → Maze (route 4)
		_xferView = XFER_ROUTE_MOUNTAIN_OF_DESPAIR;
		_nextPageType = ZoombiniPageType::kMaze;
		_xferBgId = kResBackgroundMountainOfDespair;
		_xferShapesId = kResShapesMountainOfDespair;
		_xferScrbCount = 3;
		break;
	case ZMB_SI_MAZE_16:
		// Maze → Town (final destination!)
		_xferView = XFER_ROUTE_TO_TOWN;
		_nextPageType = ZoombiniPageType::kTown;
		_xferBgId = kResBackgroundToTown;
		_xferShapesId = kResShapesToTown;
		_xferScrbCount = 9;
		break;
	default:
		// Fallback: show FromIsle, go to Bridge
		warning("ZoombiniTransitionXfer: unknown source SI page %d, defaulting to FromIsle view", (int)src);
		_xferView = XFER_ROUTE_FROM_ISLE;
		_nextPageType = ZoombiniPageType::kBridge;
		_xferBgId = kResBackgroundFromIsle;
		_xferShapesId = kResShapesFromIsle;
		_xferScrbCount = 9;
		break;
	}
}

// IDA: puzzleXfer_465FEE sound selection — difficulty/routeLevel-based voice SND from XFER.MHK.
// wRouteLevel = readRouteLevel_4569EC() + 1, wDifficulty = getDifficultyIdFromPuzzleFlag(&wPuzzleFlag).
// Returns SND resource ID (20000-20103) or 0 if none.
uint16 ZoombiniTransitionXfer::selectXferSound() const {
	// IDA: wDifficulty derived from destination puzzle flag (wPuzzleFlagIdx = dest page)
	uint16 difficulty = _vm->_state->getDifficultyIdFromPageType(_nextPageType);
	// IDA: wRouteLevel = readRouteLevel_4569EC() + 1
	int16 routeLevel = _vm->_state->readActivePageRouteLevel() + 1;

	switch (_xferView) {
	//
	// XFER_0 — FROM ISLE (Bridge): SND 20094-20099
	//
	case XFER_ROUTE_FROM_ISLE:
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
		break;

	//
	// XFER_1 — BIG BAD HUNGRY: switches on destination puzzle
	//
	case XFER_ROUTE_BIG_BAD_HUNGRY:
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
		break;

	//
	// XFER_2 — WHO'S BAYOU: switches on destination puzzle
	//
	case XFER_ROUTE_WHOS_BAYOU:
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
		break;

	//
	// XFER_3 — DEEP DARK FOREST: switches on destination puzzle
	//
	case XFER_ROUTE_DEEP_DARK_FOREST:
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
		break;

	//
	// XFER_4 — MOUNTAIN OF DESPAIR: switches on destination puzzle
	//
	case XFER_ROUTE_MOUNTAIN_OF_DESPAIR:
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
		break;

	//
	// XFER_5 — TO TOWN: SND 20100-20103
	//
	case XFER_ROUTE_TO_TOWN:
		if (difficulty == ZMB_DIFFICULTY_LEVEL1_01 || difficulty == ZMB_DIFFICULTY_LEVEL3_05) {
			return 20100;
		}
		switch (_vm->_rnd->getRandomNumber(1, 4)) {
		case 1:
			return 20100;
		case 2:
			return 20101;
		case 3:
			return 20102;
		default:
			return 20103; // no-voice
		}
		break;

	default:
		break;
	}

	return 0;
}

void ZoombiniTransitionXfer::setBackgroundBitmap() {
	computeXferRoute();
	_vm->_gfx->setPalette(_xferBgId);
	_vm->_gfx->drawBackground(_xferBgId);
}

void ZoombiniTransitionXfer::loadFeatures() {
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

	const bool isMidRoute = (_xferView >= XFER_ROUTE_BIG_BAD_HUNGRY &&
							 _xferView <= XFER_ROUTE_MOUNTAIN_OF_DESPAIR);
	const bool isToTown = (_xferView == XFER_ROUTE_TO_TOWN);
	const bool isFromIsle = (_xferView == XFER_ROUTE_FROM_ISLE);

	// Initialize callback state.
	_completionCounter = 0;
	_bodyArrangementOverride = 0;
	_linkTargetScrbId = 0;
	_finalEnvScrbId = 0;
	_envOneShotScrbId = 0;
	_envOneShotAvailable = false;
	_xfer5EventScrbId = 0;
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
		loadScrbFeature(xferShapes, _xferShapesId + 5, 0, ZmbFeature::FLAG_00000000_TYPE_SHAPES); // 6105
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
		loadScrbFeature(xferShapes, _xferShapesId, 6, kMainScrbFlags);

		// Sub-feature SCRB at bgId+200 (e.g. 1200, 2200, …) — foreground overlay detail.
		// IDA: loadSubFeatureSCRB_45FE2C(0, 1, bgId+200) with overlay flags.
		// IDA: xfer_loadArrowOverlay_467FF1 sets up post-render callback for path arrow flood-fill
		// and a pre-render shape callback (rodmap_selectRouteBand_4683D4) to select the active band.
		const uint16 subId = _xferBgId + 200;
		ZmbFeature::EventHooks routePathHooks;
		routePathHooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(
			&ZoombiniTransitionXfer::routePath_selectBand));
		routePathHooks.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(
			&ZoombiniTransitionXfer::routePath_onPostRender));
		_routePathFeature = loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, subId), subId, 4,
											ZmbFeature::FLAG_04000000_OVERLAY, routePathHooks);

		// Compute the route band position (1-4) and color level (1-4).
		computeRoutePathLevel();
		computeRoutePathColorLevel();
	}

	// -----------------------------------------------------------------------
	// Snoids — loaded from active pack (set by preceding page's save/cleanup).
	// IDA: handleZoombiniAnimation_maybe_4528A6 / zmbMoveAnimation_45479D.
	// The original engine always reads the active pack for xfer transitions.
	// -----------------------------------------------------------------------
	ZmbStateActivePack &pack = _vm->_state->_f._zmbPackActive;

	// Snoid feature flags: SNOID | OVERLAY (one-shot walk, not a looping feature).
	const uint32 snoidFlags = ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_04000000_OVERLAY;

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
			const Common::Point targetPos(670, startPos.y);
			snoid->setAnimTargetPos(targetPos);
			snoid->setAnimState(kSnoidAnimArrivalMotion, nullptr);

			if (walkerIdx > 0) {
				// Defer rendering until the snoid's scheduled frame (stagger effect).
				snoid->deactivateRender();
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
		loadScrbFeature(xferShapes, _xferShapesId + 6, 6, kEnvScrbFlagsNoLoop); // 6106
		loadScrbFeature(xferShapes, _xferShapesId + 7, 6, kEnvScrbFlagsNoLoop); // 6107
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
		_scrsResIdBase = 5199;
	} else if (isToTown) {
		// IDA: dword_4B97BC starts at 0 → first trigger fires immediately.
		_scrsNextTriggerFrame = 0;
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
	//      palette 10 (fg=white), 0x2D=45 (outline/shadow=black).
	// Text rects from IDA data at 0x4A7E4E / 0x4A7E52 (leftTop/rightBottom pairs indexed by view).
	if (_xferView >= XFER_ROUTE_BIG_BAD_HUNGRY && _xferView <= XFER_ROUTE_MOUNTAIN_OF_DESPAIR) {
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
		tc._outlineEffect = true;
		tc._textPalette = ZoombiniGraphics::kColor0A_White;    // palette #10 (fg)
		tc._outlinePalette = ZoombiniGraphics::kColor2D_Black; // palette #45 (shadow)
		tc._hAlign = Graphics::kTextAlignCenter;
		tc._vAlign = Graphics::kTextAlignCenter;
		_vm->_gfx->drawText(ZoombiniGraphics::kShapeScreen, textKey, textRect, tc);
	}
}

void ZoombiniTransitionXfer::onEveryFrame() {
	if (isClosed())
		return;

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
		if (_xferView == XFER_ROUTE_FROM_ISLE && _xferSnoidCount > 0) {
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
						// IDA 0x4676B3: SCRS resource = foot trait + 5199.
						// Each foot type (1-5) gets a different walk animation path.
						uint16 scrsId = _scrsResIdBase + snoid->_trait._foot;
						Common::SeekableReadStream *scrsStream =
							_vm->getResource(ID_SCRS, ZmbResource(ZmbArchiveKind::kPage, scrsId));
						if (scrsStream) {
							// IDA 0x4676A2: chIsFacingLeft = 0 before SCRS playback.
							snoid->setFacingLeft(false);
							snoid->startScrsPlayback(scrsStream, true /* hideOnComplete */, true /* rejectState */);
						}
					}
					_scrsTriggerIdx++;
				}
			}
		}

		// -------------------------------------------------------------------
		// XFER_5: periodic SCRS trigger — same structure as XFER_0.
		// IDA: wXferView == 5 branch.
		// Timer: 40 * rand(3,6) = 120-240 frames between triggers.
		// SCRS resource per-snoid: foot trait + 6199 (IDA 0x4677E2).
		// -------------------------------------------------------------------
		if (_xferView == XFER_ROUTE_TO_TOWN && _xferSnoidCount > 0) {
			_scrsNextTriggerFrame = getCurrentFrameCounter() + 40 * _vm->_rnd->getRandomNumber(3, 6);

			if (_scrsTriggerIdx < _xferSnoidCount) {
				uint16 snoidId = static_cast<uint16>(kSnoidPackBase) + _scrsTriggerIdx;
				ZmbSnoid *snoid = getSnoid(snoidId);
				if (snoid && snoid->getAnimState() == kSnoidAnimIdle) {
					// IDA 0x4677E2: SCRS resource = foot trait + 6199.
					uint16 scrsId = _scrsResIdBase + snoid->_trait._foot;
					Common::SeekableReadStream *scrsStream =
						_vm->getResource(ID_SCRS, ZmbResource(ZmbArchiveKind::kPage, scrsId));
					if (scrsStream) {
						// IDA 0x4677D8: chIsFacingLeft = 0 before SCRS playback.
						snoid->setFacingLeft(false);
						snoid->startScrsPlayback(scrsStream, true /* hideOnComplete */, true /* rejectState */);
					}
				}
				_scrsTriggerIdx++;
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Helper: activate a deferred env SCRB feature by ID.
// IDA: loadSCRB_460384(1, 0, runner) / scrb_initRunnerWithScript(0, 0, 0, runner).
// For features with DEFER_ANIM | DEFER_RENDER, this starts their animation.
// ---------------------------------------------------------------------------
void ZoombiniTransitionXfer::activateEnvScrb(uint16 scrbId) {
	auto it = _scrbFeatureMap.find(scrbId);
	if (it == _scrbFeatureMap.end())
		return;
	ZmbFeature *feature = it->second;
	feature->initValues();
	feature->activateAnimate();
	feature->activateRender();
}

// ---------------------------------------------------------------------------
// IDA: xfer_scrbAnimCallback_467DD4 — handles SCRS event codes during playback.
// Called from the script engine when a SCRS frame terminator carries an event code.
// @param feature  The snoid feature that fired the event.
// @param eventCode  Adjusted event code (raw byte - 1). -1 = end-of-animation.
// ---------------------------------------------------------------------------
void ZoombiniTransitionXfer::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	// Only XFER_0 and XFER_5 use SCRS-driven animation with callbacks.
	if (_xferView != XFER_ROUTE_FROM_ISLE && _xferView != XFER_ROUTE_TO_TOWN)
		return;

	// The feature must be a snoid for body arrangement and visibility operations.
	ZmbSnoid *snoid = dynamic_cast<ZmbSnoid *>(feature);

	if (eventCode > 26) {
		// ---------------------------------------------------------------
		// Event 50: XFER_5 only — activate env SCRB (IDA: word_4B9802).
		// IDA: ++town_displayedZmbCount; initFeatureRunnerWithScrb(0, 0, 0, word_4B9802).
		// ---------------------------------------------------------------
		if (eventCode == 50) {
			if (_xfer5EventScrbId != 0)
				activateEnvScrb(_xfer5EventScrbId);
		}
		// ---------------------------------------------------------------
		// Events 240-243: Set pending body arrangement override.
		// IDA: word_4B97E0 = scrbIdx - 239 (applied on next event 0).
		// ---------------------------------------------------------------
		else if (eventCode >= 240 && eventCode <= 243) {
			_bodyArrangementOverride = eventCode - 239; // 1-4
		}
		// ---------------------------------------------------------------
		// Events 250-253: Direct body arrangement change.
		// IDA: zmbRunner_setAnimShape(scrbIdx - 250, callbackData+48).
		// ---------------------------------------------------------------
		else if (eventCode >= 250 && eventCode <= 253) {
			if (snoid)
				snoid->setBodyArrangement(eventCode - 250);
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
	} else if (eventCode == -1) {
		// End-of-animation (PLAY_ONCE completion). No special handling needed.
	} else if (eventCode == 0) {
		// ---------------------------------------------------------------
		// Event 0: Toggle visibility, apply pending arrangement, inc cycle.
		// IDA 0x467E92: *(callbackData+290) = *(callbackData+290) == 0.
		// Offset 290 = CFeatureRunner307 offset 0x122 = FeatureCore259 offset 0xF2
		// = chIsFacingLeft. This toggles facing direction, NOT render visibility.
		// IDA: if word_4B97E0: setAnimShape(word_4B97E0 - 1), clear override.
		// IDA: ++*(callbackData+288) — increment cycle counter.
		// IDA: if XFER_0 && cycleCount == 2: linkFeatureRunner(word_4B97E2, 1, idx).
		// ---------------------------------------------------------------
		if (snoid) {
			// Toggle facing direction (left ↔ right).
			snoid->setFacingLeft(!snoid->isFacingLeft());

			// Apply pending body arrangement override (set by events 240-243).
			if (_bodyArrangementOverride != 0) {
				snoid->setBodyArrangement(_bodyArrangementOverride - 1);
				_bodyArrangementOverride = 0;
			}

			// Increment per-snoid SCRS facing-toggle counter.
			snoid->_scrsAnimCycleCount++;

			// XFER_0: after 2 facing toggles, link snoid after the env overlay.
			// IDA: linkFeatureRunner(word_4B97E2, 1, runnerIdx) — link snoid after env overlay.
			// In ScummVM, removing OVERLAY moves the snoid from overlayList into entityList
			// (merged after normalList where env overlay 5100 resides), achieving the "in front" effect.
			if (_xferView == XFER_ROUTE_FROM_ISLE && snoid->_scrsAnimCycleCount == 2)
				snoid->removeFlag(ZmbFeature::FLAG_04000000_OVERLAY);
		}
	} else if (eventCode >= 10 && eventCode <= 11) {
		// ---------------------------------------------------------------
		// Events 10-11: One-shot env SCRB activation (XFER_0 only).
		// IDA: if word_4B97D4[scrbIdx]: clear flag, find runner word_4B97C8[scrbIdx],
		//      set bitmask = 0x188000, loadSCRB(1, 0, runner).
		// Event 10 → SCRB 5102, Event 11 → SCRB 5103.
		// ---------------------------------------------------------------
		if (_xferView == XFER_ROUTE_FROM_ISLE) {
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

// ---------------------------------------------------------------------------
// Path Arrow Route Level Calculation
// IDA: Derives from source puzzle, maps to band index within the current view.
// The route level (1-4) tells which route band to highlight.
// ---------------------------------------------------------------------------
void ZoombiniTransitionXfer::computeRoutePathLevel() {
	// Route level is 1-based (1-4 for most views, though not all routes use 4 bands).
	// Based on source puzzle, we determine which crossing this is within the route.
	ZMB_SI_PAGE src = _vm->_xferSrcSiPage;

	switch (_xferView) {
	case XFER_ROUTE_BIG_BAD_HUNGRY: // XFER_1
		// Route 1: Bridge → Tunnels → Pizza → BC1
		switch (src) {
		case ZMB_SI_BRIDGE_02:
			_routePathLevel = 1;
			break; // First crossing
		case ZMB_SI_TUNNELS_03:
			_routePathLevel = 2;
			break; // Second crossing
		case ZMB_SI_PIZZA_04:
			_routePathLevel = 3;
			break; // Third crossing
		default:
			_routePathLevel = 1;
			break;
		}
		break;

	case XFER_ROUTE_WHOS_BAYOU: // XFER_2
		// Route 2: BC1 → Ferry → Lilly → BC2
		switch (src) {
		case ZMB_SI_BC1_NORTH_05:
			_routePathLevel = 1;
			break; // First path
		case ZMB_SI_BC1_SOUTH_06:
			_routePathLevel = 1;
			break; // Alt first path
		case ZMB_SI_FERRY_07:
			_routePathLevel = 2;
			break; // Second crossing
		case ZMB_SI_LILLY_08:
			_routePathLevel = 3;
			break; // Third crossing
		default:
			_routePathLevel = 1;
			break;
		}
		break;

	case XFER_ROUTE_DEEP_DARK_FOREST: // XFER_3
		// Route 3: BC2 via Slides → Fleens → Hotel → Net
		switch (src) {
		case ZMB_SI_SLIDES_09:
			_routePathLevel = 1;
			break; // First crossing
		case ZMB_SI_FLEENS_10:
			_routePathLevel = 2;
			break; // Second crossing
		case ZMB_SI_HOTEL_11:
			_routePathLevel = 3;
			break; // Third crossing
		default:
			_routePathLevel = 1;
			break;
		}
		break;

	case XFER_ROUTE_MOUNTAIN_OF_DESPAIR: // XFER_4
		// Route 4: BC2 via Net → Tunnels → Smoke → Maze
		switch (src) {
		case ZMB_SI_NET_12:
			_routePathLevel = 1;
			break; // First crossing
		case ZMB_SI_BASECAMP2_13:
			_routePathLevel = 2;
			break; // Alt path
		case ZMB_SI_CAVES_14:
			_routePathLevel = 3;
			break; // Third crossing
		case ZMB_SI_SMOKE_15:
			_routePathLevel = 4;
			break; // Fourth crossing
		default:
			_routePathLevel = 1;
			break;
		}
		break;

	default:
		_routePathLevel = 1;
		break;
	}
}

// ---------------------------------------------------------------------------
// Route Path Color Level Calculation
// IDA: readPuzzleLevelFromState_46788A — derives word_4B97FA (color level)
// from route difficulty. On first traversal the route level is 0 so the
// color level defaults to 1 (string "10/.", palette 0x2E-0x31).
// On subsequent traversals the route level increments, advancing the color
// tier: 2 → "3210", 3 → "5432", 4 → "7654".
// ---------------------------------------------------------------------------
void ZoombiniTransitionXfer::computeRoutePathColorLevel() {
	uint16 routeIdx;
	switch (_xferView) {
	case XFER_ROUTE_BIG_BAD_HUNGRY:       routeIdx = 0; break;
	case XFER_ROUTE_WHOS_BAYOU:           routeIdx = 1; break;
	case XFER_ROUTE_DEEP_DARK_FOREST:     routeIdx = 2; break;
	case XFER_ROUTE_MOUNTAIN_OF_DESPAIR:  routeIdx = 3; break;
	default: routeIdx = 0; break;
	}
	_routePathColorLevel = _vm->_state->_f._routeLevels[routeIdx] + 1;
	if (_routePathColorLevel > 4)
		_routePathColorLevel = 4;
}

// ---------------------------------------------------------------------------
// Route Band Shape Selection (Pre-Render Shape Callback)
// IDA: rodmap_selectRouteBand_4683D4 — selects which band shape to render.
// The SCRB_1200 frame has 4 hotspot entries (one per route band), each
// referencing a different shape at a different position. This callback keeps
// only the hotspot for the current band, removing the rest so only one
// band shape is drawn by the standard renderer.
// ---------------------------------------------------------------------------
void ZoombiniTransitionXfer::routePath_selectBand(ZmbFeature * /*feature*/,
												  ZmbHotspotGroup * /*hsGroup*/,
												  Common::Array<ZmbHotspot> &hotspots) {
	uint16 targetIdx = _routePathLevel - 1; // 0-based
	if (targetIdx >= hotspots.size())
		return;

	ZmbHotspot target = hotspots[targetIdx];
	hotspots.clear();
	hotspots.push_back(target);
}

// ---------------------------------------------------------------------------
// Route Path Post-Render Callback
// IDA: xfer_onPostRenderRoutePath (0x468457)
// Called after the path overlay feature is rendered. Performs flood-fill animation.
// ---------------------------------------------------------------------------
void ZoombiniTransitionXfer::routePath_onPostRender(ZmbFeature *feature) {
	// Note: This callback is only invoked when the feature was actually rendered.

	// IDA: xfer_onPostRenderPathArrow (0x468457) gates all flood-fill logic
	// behind chGetDrawnRect, which is only set by runner_preRenderStandard
	// (0x4619A1) when dNextRenderFrame <= scrb_dwFrameRenderTime. We replicate
	// this with _routePathNextFrame: skip flood-fill processing until enough
	// frame ticks have elapsed per the feature's frameInterval.
	uint32 currentFrame = getCurrentFrameCounter();
	bool doFloodFill = (currentFrame >= _routePathNextFrame);
	if (doFloodFill) {
		_routePathNextFrame = currentFrame + feature->getFrameInterval();
	}

	// Resolve the current band's hotspot: _routePathLevel (1-4) selects which
	// of the 4 SCRB hotspot entries to use (each references a different shape
	// at a different screen position). The pre-render shape callback
	// (routePath_selectBand) already filtered the hotspot array for the
	// standard renderer; here we read the original group for shape/position.
	int32 frameIdx = feature->getLastFrameIdx();
	ZmbHotspotGroup *hsGroup = feature->getHotspotGroup(frameIdx);
	if (!hsGroup || hsGroup->getHotspotCount() == 0)
		return;

	uint16 bandIdx = _routePathLevel - 1; // 0-based
	if (bandIdx >= hsGroup->getHotspotCount())
		bandIdx = 0;
	const ZmbHotspot &hsBand = hsGroup->getHotspot(bandIdx);
	int16 shapeId = hsBand._shapeIdx;
	int screenX = hsBand._x;
	int screenY = hsBand._y;

	if (doFloodFill) {
		// First frame (counter == 0): Initialize the flood-fill grid.
		if (_routePathCounter == 0) {
			// Get the shape surface for the current route band.
			// IDA: v3 = RMap + offset[hsArr[0].shapeid] — reads the shape
			// selected by rodmap_selectRouteBand_4683D4.
			uint16 subId = _xferBgId + 200; // 1200, 2200, 3200, or 4200
			MohawkSurface *shapeSurf = _vm->_gfx->findShape(ZmbResource(ZmbArchiveKind::kPage, subId), shapeId);
			if (!shapeSurf || !shapeSurf->getSurface())
				return;

			Graphics::Surface *surf = shapeSurf->getSurface();
			_routePathWidth = surf->w;
			_routePathHeight = surf->h;

			// Allocate working pixel buffer (copy of shape pixels).
			delete[] _routePathPixels;
			_routePathPixels = new byte[_routePathWidth * _routePathHeight];
			memcpy(_routePathPixels, surf->getPixels(), _routePathWidth * _routePathHeight);

			// Calculate seed index: (view-1)*4 + (band-1)
			// IDA: v2 = word_4B9804 + 4 * xfer_wViewType - 5
			int seedIdx = (_xferView - 1) * 4 + (_routePathLevel - 1);
			if (seedIdx < 0 || seedIdx >= 16)
				seedIdx = 0;

			Common::Point seedPos = kRoutePathSeeds[seedIdx];

			// Select color replacement values based on COLOR level (word_4B97FA),
			// NOT band position (word_4B9804). The color level is derived from
			// route difficulty, not which crossing in the route.
			// IDA: switch(word_4B97FA) selects strings "10/.", "3210", "5432", "7654".
			byte mark1, mark2, replace1, replace2;
			switch (_routePathColorLevel) {
			default:                                                           // Level 1
				mark1 = ZoombiniGraphics::kRoutePathColor2E_LevelOneBack1;     // 0x2E (46) #D6A55A
				mark2 = ZoombiniGraphics::kRoutePathColor2F_LevelOneBack2;     // 0x2F (47) #D6AD9C
				replace1 = ZoombiniGraphics::kRoutePathColor30_LevelOneColor1; // 0x30 (48) #005F41
				replace2 = ZoombiniGraphics::kRoutePathColor31_LevelOneColor2; // 0x31 (49) #579984
				break;
			case 2:                                                            // Level 2
				mark1 = ZoombiniGraphics::kRoutePathColor30_LevelOneColor1;    // 0x30 (48) #005F41
				mark2 = ZoombiniGraphics::kRoutePathColor31_LevelOneColor2;    // 0x31 (49) #579984
				replace1 = ZoombiniGraphics::kRoutePathColor32_LevelTwoColor1; // 0x32 (50) #F4A200
				replace2 = ZoombiniGraphics::kRoutePathColor33_LevelTwoColor2; // 0x33 (51) #FFC863
				break;
			case 3:                                                              // Level 3
				mark1 = ZoombiniGraphics::kRoutePathColor32_LevelTwoColor1;      // 0x32 (50) #F4A200
				mark2 = ZoombiniGraphics::kRoutePathColor33_LevelTwoColor2;      // 0x33 (51) #FFC863
				replace1 = ZoombiniGraphics::kRoutePathColor34_LevelThreeColor1; // 0x34 (52) #FF5711
				replace2 = ZoombiniGraphics::kRoutePathColor35_LevelThreeColor2; // 0x35 (53) #FF9569
				break;
			case 4:                                                             // Level 4
				mark1 = ZoombiniGraphics::kRoutePathColor34_LevelThreeColor1;   // 0x34 (52) #FF5711
				mark2 = ZoombiniGraphics::kRoutePathColor35_LevelThreeColor2;   // 0x35 (53) #FF9569
				replace1 = ZoombiniGraphics::kRoutePathColor36_LevelFourColor1; // 0x36 (54) #BF0218
				replace2 = ZoombiniGraphics::kRoutePathColor37_LevelFourColor2; // 0x37 (55) #E25161
				break;
			}

			// Initialize the flood-fill grid.
			routePath_initGrid(seedPos.x, seedPos.y, mark1, mark2, replace1, replace2);
		} else {
			// Subsequent frames: Expand the flood-fill.
			routePath_expandFloodFill(_routePathCounter);
		}

		// Increment animation counter (wraps at 1000).
		_routePathCounter += 7;
		if (_routePathCounter > 1000)
			_routePathCounter = 1000;
	} // doFloodFill

	// Blit the modified pixel buffer to the shape screen.
	if (!_routePathPixels)
		return;

	Graphics::Surface *screen = _vm->_gfx->getShapeScreen();
	if (!screen)
		return;

	// Blit at the current band's hotspot position (each band shape has its
	// own position on screen, selected by routePath_selectBand).
	for (uint16 y = 0; y < _routePathHeight; y++) {
		int destY = screenY + y;
		if (destY < 0 || destY >= screen->h)
			continue;

		for (uint16 x = 0; x < _routePathWidth; x++) {
			int destX = screenX + x;
			if (destX < 0 || destX >= screen->w)
				continue;

			byte pixel = _routePathPixels[y * _routePathWidth + x];
			// Only blit non-transparent pixels (palette index 0 = transparent).
			if (pixel != ZoombiniGraphics::kTransparentKey)
				*((byte *)screen->getBasePtr(destX, destY)) = pixel;
		}
	}
}

// ---------------------------------------------------------------------------
// Initialize flood-fill grid.
// IDA: xfer_initRoutePathGrid (0x468078)
// Scans pixels, replaces mark colors, seeds BFS queue.
// ---------------------------------------------------------------------------
void ZoombiniTransitionXfer::routePath_initGrid(int16 seedX, int16 seedY,
												byte mark1, byte mark2,
												byte replace1, byte replace2) {
	// Clear BFS queue.
	for (int i = 0; i < kRoutePathQueueSize; i++) {
		_routePathQueueActive[i] = false;
		_routePathQueueX[i] = 0;
		_routePathQueueY[i] = 0;
	}

	// Store color values for expansion.
	_routePathMark1 = mark1;
	_routePathMark2 = mark2;
	_routePathReplace1 = replace1;
	_routePathReplace2 = replace2;

	// Scan pixels and count replaceable ones.
	// IDA replaces color 1 → mark1, color 2 → mark2.
	// We look for specific palette indices that represent the "unfilled" path.
	_routePathTotalPixels = 0;

	// In the original, colors 1 and 2 are replaced. In our case, we'll look for
	// palette indices that should be flood-filled (e.g., 0x01 and 0x02).
	for (uint32 i = 0; i < (uint32)(_routePathWidth * _routePathHeight); i++) {
		byte pixel = _routePathPixels[i];
		if (pixel == 0x01) {
			_routePathPixels[i] = mark1;
			_routePathTotalPixels++;
		} else if (pixel == 0x02) {
			_routePathPixels[i] = mark2;
			_routePathTotalPixels++;
		}
	}

	_routePathRemainingPixels = _routePathTotalPixels;

	// Seed the BFS queue with the starting position.
	if (seedX >= _routePathWidth)
		seedX = _routePathWidth - 1;
	if (seedY >= _routePathHeight)
		seedY = _routePathHeight - 1;
	if (seedX < 0)
		seedX = 0;
	if (seedY < 0)
		seedY = 0;

	_routePathQueueActive[0] = true;
	_routePathQueueX[0] = seedX;
	_routePathQueueY[0] = seedY;
}

// ---------------------------------------------------------------------------
// Expand flood-fill by one iteration.
// IDA: xfer_expandRoutePathFloodFill (0x4681A8)
// Processes queued cells and adds neighbors to the queue.
// ---------------------------------------------------------------------------
void ZoombiniTransitionXfer::routePath_expandFloodFill(uint32 counter) {
	if (!_routePathPixels || _routePathTotalPixels == 0)
		return;

	// Calculate target remaining pixels based on animation progress.
	// IDA: v5 = dword_4B980C - counter * dword_4B980C / 1000
	uint32 targetRemaining = _routePathTotalPixels - (counter * _routePathTotalPixels) / 1000;

	// Expand until we reach the target or no more progress.
	while (_routePathRemainingPixels > targetRemaining) {
		uint32 prevRemaining = _routePathRemainingPixels;

		// Process each active cell in the queue.
		for (int i = 0; i < kRoutePathQueueSize; i++) {
			if (!_routePathQueueActive[i])
				continue;

			_routePathQueueActive[i] = false;
			int16 x = _routePathQueueX[i];
			int16 y = _routePathQueueY[i];

			// Get pixel address.
			byte *pixelAddr = _routePathPixels + y * _routePathWidth + x;

			// Try to expand to all 8 neighbors.
			// Row below (y+1)
			if (y + 1 < _routePathHeight) {
				byte *below = pixelAddr + _routePathWidth;
				routePath_reserveSlot(y + 1, x, below);
				if (x > 0)
					routePath_reserveSlot(y + 1, x - 1, below - 1);
				if (x + 1 < _routePathWidth)
					routePath_reserveSlot(y + 1, x + 1, below + 1);
			}

			// Row above (y-1)
			if (y > 0) {
				byte *above = pixelAddr - _routePathWidth;
				routePath_reserveSlot(y - 1, x, above);
				if (x > 0)
					routePath_reserveSlot(y - 1, x - 1, above - 1);
				if (x + 1 < _routePathWidth)
					routePath_reserveSlot(y - 1, x + 1, above + 1);
			}

			// Same row
			if (x > 0)
				routePath_reserveSlot(y, x - 1, pixelAddr - 1);
			if (x + 1 < _routePathWidth)
				routePath_reserveSlot(y, x + 1, pixelAddr + 1);
		}

		// If no progress was made, we're done.
		if (prevRemaining == _routePathRemainingPixels) {
			_routePathRemainingPixels = 0;
			break;
		}
	}
}

// ---------------------------------------------------------------------------
// Reserve a slot in the BFS queue for a pixel.
// IDA: xfer_reserveGridSlot (0x468312)
// Checks if pixel matches mark color, adds to queue, replaces with final color.
// ---------------------------------------------------------------------------
void ZoombiniTransitionXfer::routePath_reserveSlot(int16 y, int16 x, byte *pixel) {
	if (*pixel == _routePathMark1) {
		// Find empty slot in queue.
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
		// Find empty slot in queue.
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
	_vm->_xferSrcSiPage = ZMB_SI_MINUS1; // Reset for next xfer
	_vm->setNextPage(_nextPageType);
	ZoombiniTransition::close();
}

} // End of namespace Mohawk
