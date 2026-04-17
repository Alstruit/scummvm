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

#include "common/system.h"

#include "mohawk/mohawk.h"
#include "mohawk/sound.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_pages/shelter_town.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

ZoombiniShelterTown::ZoombiniShelterTown(MohawkEngine_Zoombini *vm) : ZoombiniShelter(vm, ZoombiniPageType::kTown) {
}

ZoombiniShelterTown::~ZoombiniShelterTown() {
}

void ZoombiniShelterTown::open() {
	openArchive(ZMB_MHK_TOWN);
}

void ZoombiniShelterTown::setBackgroundMusic() {
	// Town does not use the standard setBackgroundMusic() path.
	// Music is handled through the ambient sound cycling system in onEveryFrame().
	// IDA: town_initAndSetupPuzzle handles initial sound setup at the end of init.
}

void ZoombiniShelterTown::setBackgroundBitmap() {
	_vm->_gfx->setPalette(kResBackground1200);
	_vm->_gfx->drawBackground(kResBackground1200);
}

void ZoombiniShelterTown::loadFeatures() {
	ZmbStateFile &f = _vm->_state->_f;

	// IDA town_initAndSetupPuzzle @ 0x457CFF: town_nArrivingThisVisit =
	// countOccupiedInActivePack_452875() — counts ONLY occupied entries of
	// zmbPackActive, NOT all loaded features. Using total feature count
	// over-credits storage and triggers the +6 fireworks bonus on partial
	// packs (which IDA reserves for fully-occupied 16-zoombini arrivals).
	_activePackCount = 0;
	for (int16 i = 0; i < (int16)f._zmbPackActive._wPackZmbCount; i++) {
		if (f._zmbPackActive._entries[i]._bIsOccupied != 0)
			_activePackCount++;
	}
	f._zmbStoredTownCount += _activePackCount;
	if (625 <= static_cast<int16>(f._zmbStoredTownCount))
		_allZoombinisInTown = true;

	// Transfer active pack Zoombini trait/name data into stored chunk
	transferActivePackToTownStorage();

	f._zmbPackActive._wPackZmbCount = 0;

	// Find the first empty slot in town storage (searching from beginning)
	int16 firstEmptySlot = -1;
	for (int16 i = 0; firstEmptySlot < 0 && i < 625; ++i) {
		if (f._storedChunkTown._entries[i]._traits._head == ZmbTrait::TRAIT_NONE &&
			f._storedChunkTown._entries[i]._traits._eye == ZmbTrait::TRAIT_NONE &&
			f._storedChunkTown._entries[i]._traits._nose == ZmbTrait::TRAIT_NONE &&
			f._storedChunkTown._entries[i]._traits._foot == ZmbTrait::TRAIT_NONE) {
			firstEmptySlot = i;
		}
	}
	if (firstEmptySlot < 0)
		firstEmptySlot = 0;

	// Calculate town population density: (56 * storedCount / 625) + 1, clamped to [1, 56], then +24
	uint32 density = 56 * static_cast<int16>(f._zmbStoredTownCount) / 625u + 1;
	if (density > 56)
		density = 56;
	_townPopDensity = density + 24;

	// Preload images
	_vm->_gfx->preloadImage(kResBitmapShape1100);
	_vm->_gfx->preloadImage(1000);
	_vm->_gfx->preloadImage(4000);
	_vm->_gfx->preloadImage(6000);
	_vm->_gfx->preloadImage(8000);

	// Load REGS
	loadREGS(ZmbArchiveKind::kPage, kResRegs2000);

	// [*] SCRB 1000: Main overlay
	_overlayFeatures[0] = loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, 1000), kResScrb1000_Overlay, 0,
					ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
					ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	{ // [*] SCRB 1002: Overlay with REGS + pre-render shape callback
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniShelterTown::overlay_preRenderShape));
		_overlayFeatures[1] = loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, 1000), kResScrb1002_Overlay, 0,
						ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY |
						ZmbFeature::FLAG_08000000_REGION_TRACK,
						hooks);
	}

	{ // [*] SCRB 1003: Overlay with REGS + pre-render shape callback
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniShelterTown::overlay_preRenderShape));
		_overlayFeatures[2] = loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, 1000), kResScrb1003_Overlay, 0,
						ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY |
						ZmbFeature::FLAG_08000000_REGION_TRACK,
						hooks);
	}

	{ // [*] SCRB 1001: Overlay with REGS + pre-render shape callback
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniShelterTown::overlay_preRenderShape));
		_overlayFeatures[3] = loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, 1000), kResScrb1001_Overlay, 0,
						ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY |
						ZmbFeature::FLAG_08000000_REGION_TRACK,
						hooks);
	}

	// [*] SCRS 4999: Reject Zoombini snoid
	loadSnoid(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb4999_Reject,
			  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);

	// [*] SCRS 5000 ~ 5004: Normal Zoombini snoids (5 variants)
	for (uint16 i = 0; i < 5; i++) {
		loadSnoid(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb5000_Normal + i,
				  ZmbFeature::FLAG_00000001_TYPE_SNOID | ZmbFeature::FLAG_00020000_SKIP_RENDER);
	}

	// [*] SCRB 6000: Zodiac sub-feature (child of SCRB 1000)
	loadSubFeature(_overlayFeatures[0], ZmbResource(ZmbArchiveKind::kPage, 6000), kResScrb6000_Zodiac);

	{ // [*] SCRB 8000 ~ 8043: Town building sub-features (44 of them, chained from SCRB 1002)
		ZmbFeature *parent = _overlayFeatures[1];
		for (uint16 i = 0; i < 44; i++) {
			parent = loadSubFeature(parent, ZmbResource(ZmbArchiveKind::kPage, 8000), kResScrb8000_SubFeatureBase + i);
		}
	}

	// Town inhabitant Zoombini population (background decorative Zoombinis)
	// Number of inhabitants: (storedTownCount - 20) / 37, clamped to [0, 16]
	{
		int16 storedCount = static_cast<int16>(f._zmbStoredTownCount);
		int16 maxInhabitants = (storedCount - 20) / 37;
		if (maxInhabitants < 0)
			maxInhabitants = 0;
		if (maxInhabitants > 16)
			maxInhabitants = 16;
		_inhabitantCount = maxInhabitants;

		// Random-without-replacement pool: pick inhabitant positions from 16 slots
		bool positionUsed[16] = { };
		for (uint16 i = 0; i < _inhabitantCount; i++) {
			// Find a random unused position
			uint16 availCount = 0;
			for (int j = 0; j < 16; j++) {
				if (!positionUsed[j])
					availCount++;
			}
			if (availCount == 0)
				break;

			uint16 pick = _vm->_rnd->getRandomNumber(0, availCount - 1);
			uint16 posIdx = 0;
			for (int j = 0; j < 16; j++) {
				if (!positionUsed[j]) {
					if (pick == 0) {
						posIdx = j;
						break;
					}
					pick--;
				}
			}
			positionUsed[posIdx] = true;

			// Find a random occupied entry in stored chunk for this inhabitant
			int16 storedIdx = -1;
			if (storedCount > 0) {
				uint16 attempts = 0;
				while (storedIdx < 0 && attempts < 128) {
					int16 idx = _vm->_rnd->getRandomNumber(0, 624);
					if (f._storedChunkTown._entries[idx]._traits._head != ZmbTrait::TRAIT_NONE) {
						storedIdx = idx;
					}
					attempts++;
				}
			}
			_inhabitantStoredIdx[i] = storedIdx;

			// Load the inhabitant snoid at the designated position using its SCRB animation.
			// Inhabitants use SCRB 4000-4007 (not SCRS); use slot index i as the unique snoid ID.
			if (storedIdx >= 0) {
				ZmbSnoid *snoid = loadSnoidFromScrb(ZmbResource(ZmbArchiveKind::kPage, 4000),
													i, kInhabitantScrbIds[posIdx],
													kInhabitantPositions[posIdx],
													ZmbFeature::FLAG_00000001_TYPE_SNOID);
				if (snoid) {
					snoid->_trait = f._storedChunkTown._entries[storedIdx]._traits;
					snoid->_name = f._storedChunkTown._entries[storedIdx].getU32Name(_vm);
				}
			}
		}
	}

	// Memorial hotspot rects — 4×4 grid on the statue's pedestal (IDA layout
	// approximation). The statue is centered around (320, 200), with the
	// pedestal occupying ~(280, 160) to (400, 280).
	for (int16 i = 0; i < 16; i++) {
		int16 row = i / 4;
		int16 col = i % 4;
		int16 x0 = 280 + col * 30;
		int16 y0 = 160 + row * 30;
		_memorialHotspots[i] = Common::Rect(x0, y0, x0 + 30, y0 + 30);
	}

	// IDA town_initAndSetupPuzzle @ 0x458074:
	//   v7 = wTownScrollCol;
	//   if ((unsigned)v7 >= 6u) { v7 = 0; wTownScrollCol = 0; }
	//   for (; v7; --v7) town_shiftRunnersForScroll(1);
	//   town_advanceLayerFrameState(wTownScrollCol);
	//
	// Clamp the saved scroll column to [0, 5]; then shift inhabitant runner
	// X positions and advance the building/overlay frame state per scroll
	// column. Without this, mid-scroll save/load shows inhabitants at the
	// wrong X positions and the parallax layers start at frame 0 instead of
	// the saved scroll position.
	{
		uint16 scrollCol = f._townScrollCol;
		if (scrollCol >= 6) {
			scrollCol = 0;
			f._townScrollCol = 0;
		}
		for (uint16 s = scrollCol; s > 0; --s)
			shiftRunnersForScroll(1);
		advanceLayerFrameState(scrollCol);
	}

	// Walking Zoombinis from stored chunk (up to 20)
	// IDA 0x4580A0: Iterates backwards from (firstEmptySlot - 1) through stored chunk,
	// creates snoid feature runners with TYPE_TOWN_ENTITY for each occupied entry.
	// Original: zmb_registerSnoidFeatureRunner(0, &pFeatureCore), then bitmask &= ~1, |= 2.
	// Random position x=[-320, 1599], y=[410, 475].
	{
		int16 storedCount = static_cast<int16>(f._zmbStoredTownCount);
		_walkingZmbCount = 0;

		int16 walkIdx = firstEmptySlot - 1;
		if (walkIdx >= 0) {
			while (walkIdx >= 0 && _walkingZmbCount < 20 &&
				   _walkingZmbCount < static_cast<uint16>(storedCount)) {
				ZmbStateStoredEntry &entry = f._storedChunkTown._entries[walkIdx];

				// Use snoid IDs in the 20000+ range to avoid collision with inhabitants (0-15)
				uint16 snoidId = 20000 + _walkingZmbCount;

				Common::Point walkPos(
					_vm->_rnd->getRandomNumber(-320, 1599),
					_vm->_rnd->getRandomNumber(410, 475));

				ZmbSnoid *snoid = loadSnoidFromPack(snoidId, walkPos,
					ZmbFeature::FLAG_00000001_TYPE_SNOID);
				if (snoid) {
					snoid->_trait = entry._traits;
					snoid->_name = entry.getU32Name(_vm);
					// IDA: bitmask &= ~1; bitmask |= 2; → change TYPE_SNOID to TYPE_TOWN_ENTITY
					snoid->removeFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID);
					snoid->addFlag(ZmbFeature::FLAG_00000002_TYPE_TOWN_ENTITY);
					snoid->setupIdleHotspots();
					_walkingZmbSnoidIds[_walkingZmbCount] = snoidId;
					_walkingZmbCount++;
				}

				--walkIdx;
			}
		}
	}

	{ // [*] Virtual Feature: Town Zoombini render (renders walking Zoombinis)
		ZmbFeature::EventHooks hooks;
		hooks.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniShelterTown::townZoombini_render));
		hooks.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniShelterTown::townZoombini_postRender));
		loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, 0), 0, 0,
						ZmbFeature::FLAG_00001000_TOPMOST,
						hooks);
	}

	// IDA 0x4581d9: SCRB 6000 memorial statue feature
	// Originally created with TYPE_SNOID|LOOP_ANIM then bitmask overwritten to
	// TYPE_TOWN_ENTITY|LOOP_ANIM, with onPreRenderShapeFunc = town_preRenderMemorialStatue
	{
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniShelterTown::memorialStatue_preRenderShape));
		_memorialStatueFeature = loadScrbFeature(
			ZmbResource(ZmbArchiveKind::kPage, 6000), kResScrb6000_Zodiac, 6,
			ZmbFeature::FLAG_00000002_TYPE_TOWN_ENTITY | ZmbFeature::FLAG_00008000_LOOP_ANIM,
			hooks);
	}

	// Determine sound to play based on difficulty
	ZMB_DIFFICULTY_ID difficultyId = ZMB_DIFFICULTY_NOTVISITED_00;
	if (_vm->_state->_lastPageBeforeContainer != 0) {
		_vm->_state->_lastPageBeforeContainer = 0;
		difficultyId = _vm->_state->getDifficultyIdFromPageType(ZoombiniPageType::kTown);
		if (difficultyId == ZMB_DIFFICULTY_LEVEL2_02 && static_cast<int16>(f._zmbStoredTownCount) <= 16) {
			difficultyId = ZMB_DIFFICULTY_LEVEL1_01;
			f._pageFlagTown &= 0xCFFF;
		}
	}

	if (difficultyId == ZMB_DIFFICULTY_LEVEL1_01) {
		switch (f._pageFlagTown & ZMB_PAGE_MASK_0FFF) {
		case 1:
			_entrySoundRes = ZmbResource(ZmbArchiveKind::kSystem, kResSound20086_Voice);
			break;
		case 2:
			_entrySoundRes = ZmbResource(ZmbArchiveKind::kSystem, kResSound20087_Voice);
			break;
		case 3:
			_entrySoundRes = ZmbResource(ZmbArchiveKind::kSystem, kResSound20088_Voice);
			break;
		default: {
			int16 r = _vm->_rnd->getRandomNumber(1, 3);
			if (r == 1)
				_entrySoundRes = ZmbResource(ZmbArchiveKind::kSystem, kResSound20086_Voice);
			else if (r == 2)
				_entrySoundRes = ZmbResource(ZmbArchiveKind::kSystem, kResSound20087_Voice);
			else
				_entrySoundRes = ZmbResource(ZmbArchiveKind::kSystem, kResSound20088_Voice);
			break;
		}
		}
	} else if (difficultyId == ZMB_DIFFICULTY_LEVEL2_02 || difficultyId == ZMB_DIFFICULTY_LEVEL4_12) {
		int16 r = _vm->_rnd->getRandomNumber(1, 2);
		if (r == 1)
			_entrySoundRes = ZmbResource(ZmbArchiveKind::kSystem, kResSound20087_Voice);
		else
			_entrySoundRes = ZmbResource(ZmbArchiveKind::kSystem, kResSound20088_Voice);
	} else if (difficultyId == ZMB_DIFFICULTY_LEVEL3_05) {
		_entrySoundRes = ZmbResource(ZmbArchiveKind::kSystem, kResSound20086_Voice);
	} else {
		// Default: compute route-based sound ID from maze page flag.
		// IDA: rodmap_getScrbIdFromRoute (0x4588ED): ((pageFlagMaze - 1) & 0xFFF) % 3 + 3000, clamped to [3000, 3002].
		int16 soundId = computeRouteMusicId();
		_entrySoundRes = ZmbResource(ZmbArchiveKind::kPage, soundId);
		_playEntrySoundImmediately = true;
	}

	// If all Zoombinis are in town, play victory BGM (3003 is in TOWN.MHK)
	if (_allZoombinisInTown) {
		_entrySoundRes = ZmbResource(ZmbArchiveKind::kPage, kResSound3003_BGM);
		_playEntrySoundImmediately = true;
	}

	// Town develop level checks
	_developAnimTimer = 0;
	if (_allZoombinisInTown) {
		_developAnimTimer = 20;
	} else {
		if (f._zmbStoredTownCount < 17 && !f._townDevelopLevel) {
			f._townDevelopLevel = 1;
			_developAnimTimer = 10;
		}
		if (f._zmbStoredTownCount >= 100 && f._townDevelopLevel < 2) {
			f._townDevelopLevel = 2;
			_developAnimTimer = 20;
		}
		if (f._zmbStoredTownCount >= 200 && f._townDevelopLevel < 3) {
			f._townDevelopLevel = 3;
			_developAnimTimer = 20;
		}
		if (f._zmbStoredTownCount >= 300 && f._townDevelopLevel < 4) {
			f._townDevelopLevel = 4;
			_developAnimTimer = 20;
		}
		if (f._zmbStoredTownCount >= 400 && f._townDevelopLevel < 5) {
			f._townDevelopLevel = 5;
			_developAnimTimer = 20;
		}
		if (f._zmbStoredTownCount >= 500 && f._townDevelopLevel < 6) {
			f._townDevelopLevel = 6;
			_developAnimTimer = 25;
		}
		if (_activePackCount == 16)
			_developAnimTimer += 6;
	}

	// Play entry sound if conditions met
	// IDA: town_currentAmbientSoundId stores the raw sound ID for cycling.
	_ambientSoundId = _entrySoundRes._id;
	_ambientSoundFirstPlay = _playEntrySoundImmediately;
	_ambientSoundDone = false;
	_ambientSoundLastTime = 0;
	_ambientSoundDelay = 0;
	_ambientVoicePoolState = 0;
	_nPendingWalkerRemovals = 0;
	for (int i = 0; i < 3; i++)
		_celebWalkerFeatures[i] = nullptr;

	if (_entrySoundRes.hasId() && _playEntrySoundImmediately && !_developAnimTimer) {
		_vm->_sound->playZmbSound(_entrySoundRes, Audio::Mixer::kSFXSoundType);
		_ambientSoundDone = true;
	}

	// Idle animation state init (IDA: town_clearAllPuzzleState @ 0x457B3C)
	_idleAnimBudget = 0;
	_idleAnimLastFrame = 0;
	_idleAnimInterval = 120; // IDA: town_idleAnimInterval = s_updateMode ? 600 : 120
	_idleAnimPoolState = 0;
}

// ---------------------------------------------------------------------------
// onEveryFrame: Main per-frame update for Town.
// IDA: town_onHoverPerFrame (0x45891F)
// Order: cleanup finished walkers, spawn new walkers, ambient sound cycling,
// idle animation scheduling.
// ---------------------------------------------------------------------------
void ZoombiniShelterTown::onEveryFrame() {
	// --- 1. Cleanup finished celebration walkers ---
	// IDA 0x45895D: scan celebration walker slots, free completed ones.
	cleanupFinishedWalkers();

	// --- 2. Spawn celebration walkers ---
	// IDA 0x4589A8: town_spawnAmbientWalker
	spawnCelebrationWalker();

	// --- 3. Ambient sound cycling ---
	// IDA 0x458A05: timer-based alternation between music (3000-3002) and voice (20089-20093).
	if (_ambientSoundId && !_vm->_sound->isPlaying(static_cast<uint16>(_ambientSoundId))) {
		if (_ambientSoundDone) {
			// Sound just finished: start random delay timer
			_ambientSoundLastTime = getCurrentFrameCounter();
			_ambientSoundDelay = _vm->_rnd->getRandomNumber(150, 300);
			_ambientSoundDone = false;

			// Full town: keep spawning fireworks continuously
			if (_allZoombinisInTown)
				_developAnimTimer = 50;
		}

		if (getCurrentFrameCounter() - _ambientSoundLastTime > static_cast<uint32>(_ambientSoundDelay)) {
			_ambientSoundLastTime = getCurrentFrameCounter();

			int16 nextSoundId;
			if (_ambientSoundFirstPlay && !_allZoombinisInTown) {
				// First-play branch: pick random voice from pool [20089-20093]
				// IDA 0x458AB5: clear first-play flag, then pick from pool with retry
				_ambientSoundFirstPlay = false;
				bool retry;
				do {
					retry = false;
					uint16 poolIdx = _vm->_rnd->getNonRepeatRandom(5, _ambientVoicePoolState);
					nextSoundId = kAmbientVoicePool[poolIdx];
					// IDA: if voice 20093 selected and town count > 600, retry
					if (nextSoundId == 20093 &&
						static_cast<int16>(_vm->_state->_f._zmbStoredTownCount) > 600)
						retry = true;
					_ambientSoundId = nextSoundId;
				} while (retry);
			} else {
				// Music branch: cycle 3000->3001->3002->3000 or switch from voice to music
				if (_ambientSoundId >= 20000) {
					// Was a voice -> switch to route-based music
					_ambientSoundId = computeRouteMusicId();
				} else {
					// Advance to next music track (wrap 3003->3000)
					++_ambientSoundId;
					if (_ambientSoundId >= 3003)
						_ambientSoundId = 3000;
				}
				_ambientSoundFirstPlay = true;
				nextSoundId = _ambientSoundId;
			}

			// Play the selected sound
			ZmbArchiveKind kind = getAmbientSoundArchiveKind(nextSoundId);
			_vm->_sound->playZmbSound(ZmbResource(kind, static_cast<uint16>(nextSoundId)),
									  Audio::Mixer::kSFXSoundType);
			_ambientSoundDone = true;
		}
	}

	// --- 4. Idle animation scheduling ---
	// IDA 0x458B32: budget-based SCRS playback on walking Zoombinis.
	if (_walkingZmbCount <= 0)
		return;

	// Recalculate budget when exhausted.
	// IDA 0x458B40: budget thresholds based on storedTownCount (wStoredTownZmbCount)
	if (_idleAnimBudget <= 0) {
		uint16 storedCount = _vm->_state->_f._zmbStoredTownCount;
		if (storedCount == 0)
			return;
		else if (storedCount == 625)
			_idleAnimBudget = 8;
		else if (storedCount <= 156)
			_idleAnimBudget = 1;
		else if (storedCount <= 312)
			_idleAnimBudget = 4;
		else // storedCount <= 624
			_idleAnimBudget = 6;
		return;
	}

	if (getCurrentFrameCounter() - _idleAnimLastFrame <= _idleAnimInterval)
		return;
	_idleAnimLastFrame = getCurrentFrameCounter();

	// IDA 0x458B69: Try up to 16 times to find a valid idle walker on-screen
	bool triggered = false;
	int16 attempts = 0;
	do {
		++attempts;
		uint16 poolIdx = _vm->_rnd->getNonRepeatRandom(_walkingZmbCount, _idleAnimPoolState);
		ZmbSnoid *snoid = getSnoid(_walkingZmbSnoidIds[poolIdx]);
		if (snoid
			&& snoid->hasFlag(ZmbFeature::FLAG_00000002_TYPE_TOWN_ENTITY)
			&& snoid->getAnimState() == kSnoidAnimIdle
			&& snoid->getPointLoc().x > 20
			&& snoid->getPointLoc().x < 620) {
			// Play celebration SCRS: foot + 4999 (SCRS 5000-5004)
			uint16 scrsId = snoid->_trait._foot + 4999;
			Common::SeekableReadStream *scrsStream =
				_vm->getResource(ID_SCRS,
					ZmbResource(ZmbArchiveKind::kPage, scrsId));
			if (scrsStream) {
				snoid->startScrsPlayback(scrsStream, false, false);
				--_idleAnimBudget;
				triggered = true;
			}
		}
	} while (!triggered && attempts < 16);
}

void ZoombiniShelterTown::transferActivePackToTownStorage() {
	ZmbStateFile &f = _vm->_state->_f;

	for (uint16 i = 0; i < _activePackCount && i < 16; i++) {
		ZmbStateActiveEntry &activeEntry = f._zmbPackActive._entries[i];
		if (activeEntry._bIsOccupied == 0)
			continue;

		int16 id = activeEntry._traits.snoidId();
		if (id >= 0 && id < 625) {
			f._storedChunkTown._entries[id]._traits = activeEntry._traits;
			memcpy(f._storedChunkTown._entries[id]._name, activeEntry._name, 10);
		}
	}
}

ZmbRenderResult ZoombiniShelterTown::townZoombini_render(ZmbFeature *feature) {
	// Virtual feature render: captures the background rect for later compositing.
	// Walking Zoombinis are rendered through their own snoid feature runners.
	return ZmbRenderResult::kRendered;
}

void ZoombiniShelterTown::townZoombini_postRender(ZmbFeature *feature) {
	// Post-render: renders exit gate scroll buttons at overlay positions.
	// Original: town_onPostRenderButtons (0x45880F) calls picker_renderExitGateScrb
	// for buttons 1 (left gate, shape 5) and 2 (right gate, shape 24).
	//
	// IDA: picker_renderHotspot_45876F
	// - buttonIdx 1 → shape 5 (normal), 6 (pressed) — left gate exit
	// - buttonIdx 2 → shape 24 (normal), 25 (pressed) — right gate exit
	//
	// Position data from off_4A71B4 + 18*buttonIdx (RECT16 with x,y at offsets 0,2):
	// - Button 1: (600, 403)
	// - Button 2: (600, 441)

	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;
	ZmbResource shapeBitmap = ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100);

	// Render left exit gate button (shape 5, button index 1)
	_vm->_gfx->drawShape(screenKind, shapeBitmap, kShape1100_ExitGateLeftNormal_05,
						 Common::Point(600, 403));

	// Render right exit gate button (shape 24, button index 2)
	_vm->_gfx->drawShape(screenKind, shapeBitmap, kShape1100_ExitGateRightNormal_24,
						 Common::Point(600, 441));
}

void ZoombiniShelterTown::overlay_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	// Filters overlay building shapes by town population density threshold.
	// Original: town_preRenderFilterByPopulation (0x45945c)
	// Removes any hotspot whose shapeIdx exceeds the building display threshold.
	for (uint i = 0; i < hotspots.size(); ) {
		if (hotspots[i]._shapeIdx > static_cast<int16>(_townPopDensity)) {
			hotspots.remove_at(i);
		} else {
			i++;
		}
	}
}

// ---------------------------------------------------------------------------
// Celebration walker spawning.
// IDA: town_spawnAmbientWalker (0x4599F3)
// Creates SCRB 8000-8043 features with PLAY_ONCE animation.
// Up to 3 concurrent walkers; linked to SCRB 1001 overlay for Z-order.
// ---------------------------------------------------------------------------
void ZoombiniShelterTown::spawnCelebrationWalker() {
	if (_developAnimTimer <= 0)
		return;

	// Find an empty slot
	int16 slot = -1;
	for (int16 i = 0; i < 3; ++i) {
		if (_celebWalkerFeatures[i] == nullptr) {
			slot = i;
			break;
		}
	}
	if (slot < 0)
		return;

	// Select SCRB ID: 50% chance each range [8000,8021] or [8022,8043]
	int16 randVal = _vm->_rnd->getRandomNumber(0, 100);
	uint16 scrbId;
	if (randVal > 50)
		scrbId = _vm->_rnd->getRandomNumber(8000, 8021);
	else
		scrbId = _vm->_rnd->getRandomNumber(8022, 8043);

	// Determine Y position by SCRB ID group (building elevation ranges)
	// IDA 0x459A7D-0x459B4C: nested if-else tree
	int16 yPos;
	if (static_cast<int16>(scrbId) <= 8007)
		yPos = _vm->_rnd->getRandomNumber(170, 280);
	else if (static_cast<int16>(scrbId) <= 8009)
		yPos = _vm->_rnd->getRandomNumber(40, 280);
	else if (static_cast<int16>(scrbId) <= 8017)
		yPos = _vm->_rnd->getRandomNumber(110, 260);
	else if (static_cast<int16>(scrbId) <= 8021)
		yPos = _vm->_rnd->getRandomNumber(-10, 100);
	else if (static_cast<int16>(scrbId) <= 8029)
		yPos = _vm->_rnd->getRandomNumber(230, 310);
	else if (static_cast<int16>(scrbId) <= 8031)
		yPos = _vm->_rnd->getRandomNumber(140, 290);
	else if (static_cast<int16>(scrbId) <= 8039)
		yPos = _vm->_rnd->getRandomNumber(190, 280);
	else // 8040-8043
		yPos = _vm->_rnd->getRandomNumber(100, 200);

	// X position is random in [100, 540]
	int16 xPos = _vm->_rnd->getRandomNumber(100, 540);

	// Create the celebration SCRB feature.
	// IDA: runner_registerAndAllocate with TYPE_SNOID flag, then bitmask overwrite to
	// TYPE_TOWN_ENTITY | LOOP_ANIM | PLAY_ONCE | POS_DELTA (0x908002).
	// The original also sets pos2 = posLoc (delta = 0), so POS_DELTA has no visual effect.
	// We omit POS_DELTA to avoid incorrect delta from the SCRB's embedded hotspot position.
	// Frame interval = 6.
	ZmbFeature *walker = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, 8000), scrbId, 6,
		ZmbFeature::FLAG_00000002_TYPE_TOWN_ENTITY |
		ZmbFeature::FLAG_00008000_LOOP_ANIM |
		ZmbFeature::FLAG_00100000_PLAY_ONCE);

	if (walker) {
		// IDA: Sets posLoc to (random_x, random_y), then pos2 = posLoc (delta=0).
		// We set pointLoc for potential hit-testing; rendering uses embedded SCRB positions.
		walker->setPointLoc(Common::Point(xPos, yPos));
		// NOTE: Original engine runner_linkRelativeToParent(wFeatureRunnerIdx_1001, 0, walker)
		// for Z-order. ScummVM uses a different Z-ordering system; no-op here.
		_celebWalkerFeatures[slot] = walker;

		if (_developAnimTimer > 0)
			--_developAnimTimer;
	}
}

// ---------------------------------------------------------------------------
// Cleanup completed celebration walkers.
// IDA: town_onHoverPerFrame (0x45895D) cleanup loop.
// Scans celebration walker slots for completed animations, frees them.
// ---------------------------------------------------------------------------
void ZoombiniShelterTown::cleanupFinishedWalkers() {
	for (int16 i = 0; i < 3; ++i) {
		if (_celebWalkerFeatures[i] == nullptr)
			continue;
		if (_celebWalkerFeatures[i]->hasAnimEndCallbackFired()) {
			unloadScrbFeature(_celebWalkerFeatures[i]);
			_celebWalkerFeatures[i] = nullptr;
		}
	}
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
ZmbArchiveKind ZoombiniShelterTown::getAmbientSoundArchiveKind(int16 id) const {
	// Town music 3000-3003 is in TOWN.MHK (page archive).
	// Voice sounds 20000+ are in ZOOMBINI.MHK (system archive).
	if (id >= 1000 && id < 20000)
		return ZmbArchiveKind::kPage;
	return ZmbArchiveKind::kSystem;
}

int16 ZoombiniShelterTown::computeRouteMusicId() const {
	// IDA: rodmap_getScrbIdFromRoute (0x4588ED)
	// ((pageFlagMaze - 1) & 0xFFF) % 3 + 3000, clamped to [3000, 3002].
	uint16 mazePF = _vm->_state->_f._pageFlagMaze;
	int16 soundId = ((mazePF - 1) & 0x0FFF) % 3 + 3000;
	if (soundId < 3000)
		soundId = 3000;
	if (soundId >= 3003)
		soundId = 3002;
	return soundId;
}

// Ambient voice pool: 5 voice tracks for Town ambient cycling.
// IDA: word_4A727E[5]
const int16 ZoombiniShelterTown::kAmbientVoicePool[5] = {
	20089, 20090, 20091, 20092, 20093
};

// Town inhabitant position data (16 x,y coordinate pairs).
// Source: unk_4A72D0 in the original binary (puzzleTown_457C7E).
const Common::Point ZoombiniShelterTown::kInhabitantPositions[16] = {
	Common::Point(467, 265), Common::Point(349, 225), Common::Point(777, 291), Common::Point(828, 284),
	Common::Point( 44, 330), Common::Point(283, 152), Common::Point(195, 211), Common::Point(607, 201),
	Common::Point(1182, 287), Common::Point(1299, 228), Common::Point(1422, 269), Common::Point(1807, 316),
	Common::Point(1048, 309), Common::Point( 709, 228), Common::Point(1740, 284), Common::Point(1532, 172),
};

// Town inhabitant SCRB IDs (16 IDs for inhabitant animations, cycling 4000-4007 twice).
// Source: unk_4A7310 in the original binary (puzzleTown_457C7E).
// These are SCRB resources, not SCRS; loaded via loadSnoidFromScrb().
const uint16 ZoombiniShelterTown::kInhabitantScrbIds[16] = {
	4000, 4001, 4002, 4003, 4004, 4005, 4006, 4007,
	4000, 4001, 4002, 4003, 4004, 4005, 4006, 4007,
};

void ZoombiniShelterTown::memorialStatue_updateDials() {
	// IDA: town_updateDateCachePeriodic (0x457C19)
	// Debounce: only update every 1800 frames (~30 seconds at 60fps)
	if (_currentFrameCounter <= _statueUpdateTimer + 1800)
		return;

	_statueUpdateTimer = _currentFrameCounter;

	// IDA: getDate_410CFE returns hours in a1, minutes in a2
	// unk_4A72CE = hours / 5, unk_4A72CF = minutes % 12
	TimeDate td;
	g_system->getTimeAndDate(td);
	_statueHourDial = static_cast<uint8>(td.tm_hour) / 5;
	_statueMinuteDial = static_cast<uint8>(td.tm_min) % 12;
}

void ZoombiniShelterTown::memorialStatue_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	// IDA: town_preRenderMemorialStatue (0x458597)
	// Hides the memorial zodiac statue shapes, then selectively re-shows them
	// when the town is scrolled to column 1 or 2.

	// Zero out first hotspot shape (hide by default)
	if (hotspots.size() > 0)
		hotspots[0]._shapeIdx = ZmbHotspot::kShapeNone;

	// IDA: Only show statue when townScrollCol is 1 or 2
	uint16 scrollCol = _vm->_state->_f._townScrollCol;
	if (scrollCol != 1 && scrollCol != 2)
		return;

	// Update zodiac dials from system clock (debounced)
	memorialStatue_updateDials();

	// IDA 0x4586A6: Set hotspot shapes and positions
	// hotspot[0]: hour dial — shapeIdx = hourDial + 1
	// hotspot[1]: minute dial — shapeIdx = minuteDial + 13
	// Both positioned at y=218, x depends on scroll column
	int16 xPos = (scrollCol == 1) ? 626 : 307;

	if (hotspots.size() > 0) {
		hotspots[0]._shapeIdx = _statueHourDial + 1;
		hotspots[0]._x = xPos;
		hotspots[0]._y = 218;
	}
	if (hotspots.size() > 1) {
		hotspots[1]._shapeIdx = _statueMinuteDial + 13;
		hotspots[1]._x = xPos;
		hotspots[1]._y = 218;
	}
}

ZmbEventHandleResult ZoombiniShelterTown::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// IDA town_onClickHandler @ 0x457CE3 case 3:
	//   if (memorial card active) { hideMemorialCard(); return; }
	//   int slot = click_hitTestMemorialHotspots(pos);
	//   if (slot >= 0) { showMemorialCard(slot); return; }
	//   if (pos.x < 80)  { --wTownScrollCol; shiftRunnersForScroll(-1); }
	//   if (pos.x > 560) { ++wTownScrollCol; shiftRunnersForScroll(+1); }
	if (_memorialCardActive) {
		hideMemorialCard();
		return ZmbEventHandleResult::kConsumed;
	}

	// Memorial hotspot hit test (16 card slots on the statue)
	int16 slotHit = hitTestMemorialHotspots(absPos);
	if (slotHit >= 0) {
		showMemorialCard(slotHit);
		return ZmbEventHandleResult::kConsumed;
	}

	// Left/right edge scroll
	ZmbStateFile &f = _vm->_state->_f;
	if (absPos.x < 80) {
		if (f._townScrollCol > 0) {
			f._townScrollCol--;
			shiftRunnersForScroll(-1);
		}
		return ZmbEventHandleResult::kConsumed;
	}
	if (absPos.x > 560) {
		if (f._townScrollCol < 5) {
			f._townScrollCol++;
			shiftRunnersForScroll(1);
		}
		return ZmbEventHandleResult::kConsumed;
	}

	return ZoombiniShelter::onLButtonDown(absPos, relPos);
}

void ZoombiniShelterTown::showMemorialCard(int16 slotIdx) {
	// IDA town_renderMemorialCard @ 0x4595C0: registers a TOPMOST SCRB for the
	// card body with 5 text rows. The card reads gameState.memorialRoute[slot],
	// memorialYear[slot], memorialMonth[slot], memorialDay[slot],
	// memorialCardType[slot] (=level tier 1-4), memorialDiffLevel[slot].
	//
	// ScummVM doesn't have a pre-built memorial card SCRB; we gate interaction
	// via `_memorialCardActive` (all other onLButtonDown handlers return
	// immediately while active). A future iteration can draw the 5-row text
	// overlay via a direct-draw callback.
	if (slotIdx < 0 || slotIdx >= 16)
		return;
	_memorialCardActive = true;
	_memorialCardSlotIdx = slotIdx;

	// IDA: `scrb_enqueueSoundResource(0, SND_00996_MOVE_LONG_SFX)` plays on
	// open for audio feedback.
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, 996),
		Audio::Mixer::kSFXSoundType);

	debugC(1, kZmbDebugPage, "Town: memorial card slot=%d opened", slotIdx);
}

void ZoombiniShelterTown::hideMemorialCard() {
	if (!_memorialCardActive)
		return;
	_memorialCardActive = false;
	_memorialCardSlotIdx = -1;
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, 997),
		Audio::Mixer::kSFXSoundType);
	debugC(1, kZmbDebugPage, "Town: memorial card dismissed");
}

int16 ZoombiniShelterTown::hitTestMemorialHotspots(const Common::Point &pos) const {
	// IDA click_hitTestMemorialHotspots @ 0x458FF4: tests 16 hotspot rects
	// placed along the statue's pedestal. Default layout uses a 4×4 grid on
	// the statue's base at ~(280, 160)-(400, 280) with 30×30 cells.
	//
	// The slot must have `_memorialRoute[i]` non-zero (card was earned) to be
	// hit-test eligible. _memorialRoute stores 1-4 for leg A/B/C/D; 0 = empty.
	const ZmbStateFile &f = _vm->_state->_f;
	for (int16 i = 0; i < 16; i++) {
		if (f._memorialRoute[i] == 0)
			continue;
		if (_memorialHotspots[i].isEmpty())
			continue;
		if (_memorialHotspots[i].contains(pos))
			return i;
	}
	return -1;
}

void ZoombiniShelterTown::shiftRunnersForScroll(int16 steps) {
	// IDA town_shiftRunnersForScroll: every snoid (TYPE_SNOID or
	// TYPE_TOWN_ENTITY) gets its X shifted by -320 * steps. The town world is
	// 6 columns of 320px each; scrolling N columns moves the camera right
	// (== shifting all runners left).
	const int16 deltaX = -320 * steps;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		ZmbSnoid *s = *it;
		if (!s)
			continue;
		Common::Point p = s->getPointLoc();
		s->setPointLoc(Common::Point(p.x + deltaX, p.y));
	}
}

void ZoombiniShelterTown::advanceLayerFrameState(uint16 scrollCol) {
	// IDA town_advanceLayerFrameState: bumps the building/parallax overlay
	// runners' wGroupFrameIdx0098 by `scrollCol * frameStep` so that on
	// reload the parallax layers display the column the player saved on.
	// Each scroll column step corresponds to 1 animation frame on the
	// background overlays (1000/1002/1003/1001).
	for (int16 i = 0; i < 4; i++) {
		ZmbFeature *layer = _overlayFeatures[i];
		if (!layer)
			continue;
		// Advance _lastFrameIdx by scrollCol; clamp to frame max.
		// Without exposed setters we approximate via deactivate/reactivate
		// which the framework will pick up on the next render tick.
		(void)layer;
	}
	// scrollCol > 0 is a no-op until per-frame scroll-state is wired into
	// ZmbFeature; tracked here as future engine work. The shift above is
	// the user-visible portion of the original behavior.
	(void)scrollCol;
}

} // End of namespace Mohawk
