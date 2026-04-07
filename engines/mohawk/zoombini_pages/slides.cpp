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
#include "mohawk/zoombini_pages/slides.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_resource.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

// IDA: pedestal positions at 0x4A3CF8 (16 POINTS)
const Common::Point ZoombiniInteractiveSlides::kSnoidPositions[16] = {
	Common::Point(482, 127), Common::Point(428, 128), Common::Point(375, 129), Common::Point(318, 127),
	Common::Point(272, 129), Common::Point(226, 128), Common::Point(184, 127), Common::Point(140, 129),
	Common::Point( 87, 128), Common::Point(110, 170), Common::Point(122, 246), Common::Point( 84, 212),
	Common::Point(140, 327), Common::Point( 77, 293), Common::Point( 40, 157), Common::Point( 44, 232),
};

ZoombiniInteractiveSlides::ZoombiniInteractiveSlides(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kSlides) {
}

ZoombiniInteractiveSlides::~ZoombiniInteractiveSlides() {
}

void ZoombiniInteractiveSlides::open() {
	openArchive(ZMB_MHK_SLIDES);
}

void ZoombiniInteractiveSlides::setBackgroundMusic() {
	// IDA: slides_puzzleInit (0x441f0c) has no music playback call on page load.
	// sound_activeHandle = 20078 is stored at end of funcInit for F1 replay only.
}

void ZoombiniInteractiveSlides::setBackgroundBitmap() {
	// IDA: gfx_drawBackgroundFromResId(5000)
	_vm->_gfx->setPalette(5000);
	_vm->_gfx->drawBackground(5000);
}

void ZoombiniInteractiveSlides::loadFeatures() {
	// IDA: puzzleSlides_441F0C
	_difficultyLevel = _vm->_state->readActivePageRouteLevel();

	// IDA: slides_initGridByDifficulty (0x4468F8) — initialize grid parameters
	// Default values: slotBaseState=504, cellSpacing=48
	_slotBaseState = 504;
	_cellSpacing = 48;

	// At highest difficulty, randomize grid parameters
	// IDA: if (slides_difficultyLevel == 3) { rand(0,1) → slotBaseState; if non-zero → cellSpacing=24 }
	if (_difficultyLevel == 3) {
		int16 randVal = _vm->_rnd->getRandomNumber(0, 1);
		_slotBaseState = 504 + randVal;
		if (randVal != 0)
			_cellSpacing = 24;
		debugC(kZmbDebugPage, "Slides Level 3: slotBaseState=%d, cellSpacing=%d",
		       _slotBaseState, _cellSpacing);
	}

	// At highest difficulty, load NODE/PATH for walking
	// IDA: if (slides_difficultyLevel == 3) node_loadNodeAndPath(0x3E8u)
	if (_difficultyLevel == 3) {
		loadNODE(ZmbArchiveKind::kPage, 1000);
	}

	// Load terrain barrier bitmap (tBMP 100)
	// IDA: rmap_loadTerrainArchive(0x64u)
	loadTerrainBitmap(100);

	// Preload shape images
	// IDA: shape_loadSubShapesFromArchive(&stru_4A3B20, 0x1770u) — shapes at tBMP 6000
	_vm->_gfx->preloadImage(6000);
	_vm->_gfx->preloadImage(8000);

	// Load feature groups
	// IDA: scrb_useFeatureGroup(0, 0, 7000)
	// IDA: scrb_useFeatureGroup(0, 1, 8000)

	// Load main features: 14 SCRBs at 7000
	// IDA: scrb_loadMainFeatureSet(14, 7000)
	ZmbFeature *mainFeature = createMainFeatureHead(
		ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	// IDA: scrb_loadSubFeatureSet(0, 3, 8000) — 3 subs at 8000
	{
		ZmbFeature *parent = mainFeature;
		for (uint16 i = 0; i < 3; i++) {
			parent = loadSubFeature(parent,
				ZmbResource(ZmbArchiveKind::kPage, 8000), 8000 + i);
		}
	}

	// Load reject pool: 4 reject scripts at SCRS 14000
	// IDA: scrs_loadRejectPool(0, 4, 14000)
	for (uint16 i = 0; i < 4; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  14000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load normal pool: 6 normal scripts at SCRS 13000
	// IDA: scrs_loadNormalPool(0, 6, 13000)
	for (uint16 i = 0; i < 6; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, 6000),
				  13000 + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// Load Zoombinis from active pack at 16 pedestal positions
	// IDA: zmb_assignPedestalPositions(1, &stru_4A3CF8, 16)
	loadZoombinisFromPack();

	// Layout and stagger walk-in
	// IDA: zmb_layoutStaticAndWalkInGroups(0)
	layoutStaticAndWalkIn();
	assignStaggeredWalkDelays();

	// Set up Go/Map/Help buttons
	setGoButton(Common::Rect(600, 441, 639, 478), 1, 2, 3);
	setMapButton(Common::Rect(600, 403, 639, 440), 5, 6);
	setHelpButton(Common::Rect(600, 365, 639, 402));
	loadGoMapButtonsFeature(6000);
	loadHelpButtonFeature();

	// Get difficulty
	_vm->_state->getDifficultyIdFromPageFlag(_vm->_state->_f._pageFlagSlides);

	// IDA: sound_activeHandle = 20078 — slides narrator voice (F1 key replay)
	_activeHelpSoundId = ZmbResource(ZmbArchiveKind::kSystem, 20078);

	// Celebration state init (IDA: slides_puzzleInit @ 0x441F0C)
	_celebrationActive = false;
	_celebrationIndex = 0;
	// IDA: slides_celebrationTarget = slides_numZoombinis
	_celebrationTarget = _loadedZmbCount;
	_celebrationPoolState = 0;
	_celebrationLastFrame = 0;
	_matchCount = 0;
}

void ZoombiniInteractiveSlides::onGoButtonActivated() {
	// IDA: slides_onClickHandler case 2 -> puzzle_pendingTransitionTarget = 5 (BC2)
	// Route 2: Slides -> Basecamp2 (via Xfer)
	_departXferSrcSiPage = ZMB_SI_SLIDES_08;
	ZoombiniInteractive::onGoButtonActivated();
}

// ---------------------------------------------------------------------------
// onEveryFrame: Per-frame celebration scheduling.
// IDA: slides_puzzleHoverUpdate @ 0x4427B7
// ---------------------------------------------------------------------------
void ZoombiniInteractiveSlides::onEveryFrame() {
	if (_loadedZmbCount <= 0)
		return;

	// Celebration scheduling.
	// Once _celebrationActive is set, it stays set (one celebration per match event).
	// Resets when _celebrationIndex reaches _celebrationTarget (= loaded zmb count).
	if (_celebrationActive || !_matchCount || _celebrationIndex >= _celebrationTarget) {
		if (_celebrationIndex >= _celebrationTarget) {
			_celebrationPoolState = 0;
			_celebrationLastFrame = 0;
			_matchCount = 0;
			_celebrationIndex = 0;
		}
	} else {
		_celebrationActive = true;
		if (getCurrentFrameCounter() - _celebrationLastFrame > 30) {
			_celebrationLastFrame = getCurrentFrameCounter();
			bool triggered = false;
			int16 attempts = 0;

			do {
				uint16 poolIdx = _vm->_rnd->getNonRepeatRandom(_loadedZmbCount, _celebrationPoolState);
				uint16 snoidId = 10000 + poolIdx;
				ZmbSnoid *snoid = getSnoid(snoidId);

				if (snoid && snoid->isRenderActivated() &&
					snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
					// IDA: snoidScript_initAndPlay(0, 0, byte_239 - 1 + 13001, core)
					uint16 scrsId = snoid->_trait._foot - 1 + 13001;
					Common::SeekableReadStream *scrsStream =
						_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
							ZmbResource(ZmbArchiveKind::kPage, scrsId));
					if (scrsStream) {
						snoid->startScrsPlayback(scrsStream, false, true);
						_celebrationIndex++;
						triggered = true;
					}
				} else if (++attempts > 20) {
					triggered = true;
				}
			} while (!triggered);
		}
	}
}

// ---------------------------------------------------------------------------
// onFeatureAnimEvent: Snoid travel animation callback.
// IDA: slides_snoidTravelCallback @ 0x4462BC
// ---------------------------------------------------------------------------
void ZoombiniInteractiveSlides::onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) {
	if (!feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
		return;

	ZmbSnoid *snoid = static_cast<ZmbSnoid *>(feature);

	if (eventCode == 0) {
		// Toggle render visibility + apply pending body arrangement.
		// IDA: *(runnerData+290) = *(runnerData+290)==0; if word_4B110E: apply & clear.
		if (snoid->isRenderActivated())
			snoid->deactivateRender();
		else
			snoid->activateRender();

		if (_pendingBodyArrangement != 0) {
			snoid->setBodyArrangement(_pendingBodyArrangement - 1);
			_pendingBodyArrangement = 0;
		}
	} else if (eventCode >= 90 && eventCode <= 93) {
		// Directional travel animations.
		// IDA: events 90-93 initiate SCRS 14000-14003 (left/right/up/down)
		// on the active travel snoid with re-set callback.
		if (_activeTravelSnoidId == 0)
			return;

		ZmbSnoid *travelSnoid = getSnoid(_activeTravelSnoidId);
		if (!travelSnoid)
			return;

		int16 scrsId = 14000 + (eventCode - 90);
		Common::SeekableReadStream *scrsStream =
			_vm->getResource(MKTAG('S', 'C', 'R', 'S'),
							 ZmbResource(ZmbArchiveKind::kPage, scrsId));
		if (scrsStream) {
			travelSnoid->startScrsPlayback(scrsStream, false, false);
			// IDA: events 90-92 set word_4B1112=1 (traveling), event 93 sets 0 (arrived)
			_travelState = (eventCode == 93) ? 0 : 1;
			debug(3, "Slides: Travel SCRS %d on snoid %d", scrsId, _activeTravelSnoidId);
		}
	} else if (eventCode >= kZmbAnimEvent240_BodyArrangePendFirst && eventCode <= kZmbAnimEvent243_BodyArrangePendLast) {
		// Pending body arrangement (applied on next event 0).
		// IDA: word_4B110E = travelIdx - 239 (range 1-4)
		_pendingBodyArrangement = eventCode - (kZmbAnimEvent240_BodyArrangePendFirst - 1);
	} else if (eventCode >= kZmbAnimEvent250_BodyArrangeDirectFirst && eventCode <= kZmbAnimEvent253_BodyArrangeDirectLast) {
		// Direct body arrangement change.
		// IDA: zmb_setBodyLayerShapes(travelIdx - 250, core)
		snoid->setBodyArrangement(eventCode - kZmbAnimEvent250_BodyArrangeDirectFirst);
	}
}

void ZoombiniInteractiveSlides::loadZoombinisFromPack() {
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

	_loadedZmbCount = posIdx;
}

} // End of namespace Mohawk
