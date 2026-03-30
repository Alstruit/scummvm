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

#include "mohawk/mohawk.h"
#include "mohawk/sound.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_pages/town.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

ZoombiniInteractiveTown::ZoombiniInteractiveTown(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kTown) {
}

ZoombiniInteractiveTown::~ZoombiniInteractiveTown() {
}

void ZoombiniInteractiveTown::open() {
	openArchive(ZMB_MHK_TOWN);
}

void ZoombiniInteractiveTown::setBackgroundMusic() {
	if (_allZoombinisInTown) {
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, kResSound3003_BGM), Audio::Mixer::kMusicSoundType);
	} else {
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, kResSound3000_BGM), Audio::Mixer::kMusicSoundType);
	}
}

void ZoombiniInteractiveTown::setBackgroundBitmap() {
	_vm->_gfx->setPalette(kResBackground1200);
	_vm->_gfx->drawBackground(kResBackground1200);
}

void ZoombiniInteractiveTown::loadFeatures() {
	ZmbStateFile &f = _vm->_state->_f;

	// Move active pack Zoombinis into town storage
	_activePackCount = _vm->_state->_loadedZmbFeatures.size();
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

	// Load REGS
	loadREGS(ZmbArchiveKind::kPage, kResRegs2000);

	// [*] SCRB 1000: Main overlay
	_overlayFeatures[0] = loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb1000_Overlay, 0,
					ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
					ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY);

	{ // [*] SCRB 1002: Overlay with REGS + pre-render shape callback
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractiveTown::overlay_preRenderShape));
		_overlayFeatures[1] = loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb1002_Overlay, 0,
						ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY |
						ZmbFeature::FLAG_08000000_REGION_TRACK,
						hooks);
	}

	{ // [*] SCRB 1003: Overlay with REGS + pre-render shape callback
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractiveTown::overlay_preRenderShape));
		_overlayFeatures[2] = loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb1003_Overlay, 0,
						ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00020000_SKIP_RENDER | ZmbFeature::FLAG_04000000_OVERLAY |
						ZmbFeature::FLAG_08000000_REGION_TRACK,
						hooks);
	}

	{ // [*] SCRB 1001: Overlay with REGS + pre-render shape callback
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractiveTown::overlay_preRenderShape));
		_overlayFeatures[3] = loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb1001_Overlay, 0,
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
	loadSubFeature(_overlayFeatures[0], ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb6000_Zodiac);

	{ // [*] SCRB 8000 ~ 8043: Town building sub-features (44 of them, chained from SCRB 1002)
		ZmbFeature *parent = _overlayFeatures[1];
		for (uint16 i = 0; i < 44; i++) {
			parent = loadSubFeature(parent, ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb8000_SubFeatureBase + i);
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
				ZmbSnoid *snoid = loadSnoidFromScrb(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100),
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

	// Walking Zoombinis from stored chunk (up to 20)
	{
		int16 storedCount = static_cast<int16>(f._zmbStoredTownCount);
		int16 maxWalking = storedCount;
		if (maxWalking > 20)
			maxWalking = 20;
		if (maxWalking < 0)
			maxWalking = 0;
		_walkingZmbCount = maxWalking;

		bool entryUsed[625] = { };
		for (uint16 i = 0; i < _walkingZmbCount; i++) {
			// Find a random occupied entry in stored chunk
			int16 storedIdx = -1;
			uint16 attempts = 0;
			while (storedIdx < 0 && attempts < 256) {
				int16 idx = _vm->_rnd->getRandomNumber(0, 624);
				if (!entryUsed[idx] && f._storedChunkTown._entries[idx]._traits._head != ZmbTrait::TRAIT_NONE) {
					storedIdx = idx;
					entryUsed[idx] = true;
				}
				attempts++;
			}
			_walkingZmbStoredIdx[i] = storedIdx;
		}
	}

	{ // [*] Virtual Feature: Town Zoombini render (renders walking Zoombinis)
		ZmbFeature::EventHooks hooks;
		hooks.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniInteractiveTown::townZoombini_render));
		hooks.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveTown::townZoombini_postRender));
		loadVirtualFeature(kVirtualFeatureTownZoombini, 0,
						   ZmbFeature::FLAG_00001000_TOPMOST,
						   hooks);
	}

	// IDA 0x4581d9: SCRB 6000 memorial statue feature
	// Originally created with TYPE_SNOID|LOOP_ANIM then bitmask overwritten to
	// TYPE_TOWN_ENTITY|LOOP_ANIM, with onPreRenderShapeFunc = town_preRenderMemorialStatue
	// TODO: Add preRenderMemorialStatue callback for town development level filtering
	_memorialStatueFeature = loadScrbFeature(
		ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb6000_Zodiac, 6,
		ZmbFeature::FLAG_00000002_TYPE_TOWN_ENTITY | ZmbFeature::FLAG_00008000_LOOP_ANIM);

	// Determine sound to play based on difficulty
	ZMB_DIFFICULTY_ID difficultyId = ZMB_DIFFICULTY_NOTVISITED_00;
	if (f._townScrollCol != 0) {
		f._townScrollCol = 0;
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
		uint16 mazePF = _vm->_state->_f._pageFlagMaze;
		int16 soundId = ((mazePF - 1) & 0x0FFF) % 3 + 3000;
		if (soundId < 3000)
			soundId = 3000;
		if (soundId >= 3003)
			soundId = 3002;
		_entrySoundRes = ZmbResource(ZmbArchiveKind::kSystem, soundId);
		_playEntrySoundImmediately = true;
	}

	// If all Zoombinis are in town, play victory BGM (3003 is in ZOOMBINI.MHK per range registration)
	if (_allZoombinisInTown) {
		_entrySoundRes = ZmbResource(ZmbArchiveKind::kSystem, kResSound3003_BGM);
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
	if (_entrySoundRes.hasId() && _playEntrySoundImmediately && !_developAnimTimer) {
		_vm->_sound->playZmbSound(_entrySoundRes, Audio::Mixer::kSFXSoundType);
	}
}

void ZoombiniInteractiveTown::transferActivePackToTownStorage() {
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

ZmbRenderResult ZoombiniInteractiveTown::townZoombini_render(ZmbFeature *feature) {
	// Virtual feature render: captures the background rect for later compositing.
	// Walking Zoombinis are rendered through their own snoid feature runners.
	return ZmbRenderResult::kRendered;
}

void ZoombiniInteractiveTown::townZoombini_postRender(ZmbFeature *feature) {
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

void ZoombiniInteractiveTown::overlay_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
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

// Town inhabitant position data (16 x,y coordinate pairs).
// Source: unk_4A72D0 in the original binary (puzzleTown_457C7E).
const Common::Point ZoombiniInteractiveTown::kInhabitantPositions[16] = {
	Common::Point(467, 265), Common::Point(349, 225), Common::Point(777, 291), Common::Point(828, 284),
	Common::Point( 44, 330), Common::Point(283, 152), Common::Point(195, 211), Common::Point(607, 201),
	Common::Point(1182, 287), Common::Point(1299, 228), Common::Point(1422, 269), Common::Point(1807, 316),
	Common::Point(1048, 309), Common::Point( 709, 228), Common::Point(1740, 284), Common::Point(1532, 172),
};

// Town inhabitant SCRB IDs (16 IDs for inhabitant animations, cycling 4000-4007 twice).
// Source: unk_4A7310 in the original binary (puzzleTown_457C7E).
// These are SCRB resources, not SCRS; loaded via loadSnoidFromScrb().
const uint16 ZoombiniInteractiveTown::kInhabitantScrbIds[16] = {
	4000, 4001, 4002, 4003, 4004, 4005, 4006, 4007,
	4000, 4001, 4002, 4003, 4004, 4005, 4006, 4007,
};

} // End of namespace Mohawk
