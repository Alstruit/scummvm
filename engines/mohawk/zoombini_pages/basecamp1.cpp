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

#include "mohawk/console.h"
#include "mohawk/cursors.h"
#include "mohawk/mohawk.h"
#include "mohawk/sound.h"
#include "mohawk/video.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_pages/basecamp1.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"
#include "mohawk/zoombini_text.h"

namespace Mohawk {

ZoombiniInteractiveBasecampOne::ZoombiniInteractiveBasecampOne(MohawkEngine_Zoombini *vm) : ZoombiniInteractive(vm, ZoombiniPageType::kBasecamp1) {
	_scrollButtonStateMap[kScrollButtons_LeftFour] = ContinuousButtonState(0, 4, kShape2100_ScrollLeftFourNormal_07, kShape2100_ScrollLeftFourPressed_08);
	_scrollButtonStateMap[kScrollButtons_LeftOne] = ContinuousButtonState(1, 5, kShape2100_ScrollLeftOneNormal_09, kShape2100_ScrollLeftOnePressed_10);
	_scrollButtonStateMap[kScrollButtons_RightOne] = ContinuousButtonState(2, 6, kShape2100_ScrollRightOneNormal_11, kShape2100_ScrollRightOnePressed_12);
	_scrollButtonStateMap[kScrollButtons_RightFour] = ContinuousButtonState(3, 7, kShape2100_ScrollRightFourNormal_13, kShape2100_ScrollRightFourPressed_14);

	_scrollButtonRectMap[kScrollButtons_LeftFour] = _scrollLeftFourButtonRect;
	_scrollButtonRectMap[kScrollButtons_LeftOne] = _scrollLeftOneButtonRect;
	_scrollButtonRectMap[kScrollButtons_RightOne] = _scrollRightOneButtonRect;
	_scrollButtonRectMap[kScrollButtons_RightFour] = _scrollRightFourButtonRect;
}

ZoombiniInteractiveBasecampOne::~ZoombiniInteractiveBasecampOne() {
}

void ZoombiniInteractiveBasecampOne::open() {
	openArchive(ZMB_MHK_BASECAMP);

	loadREGS(ZmbArchiveKind::kPage, kResRegs9000);
}

void ZoombiniInteractiveBasecampOne::setBackgroundMusic() {
}

void ZoombiniInteractiveBasecampOne::setBackgroundBitmap() {
	_vm->_gfx->setPalette(kResBackground1000);
	_vm->_gfx->drawBackground(kResBackground1000);
}

void ZoombiniInteractiveBasecampOne::loadFeatures() {
	_vm->_gfx->preloadImage(kResBitmapShape1100);
	_vm->_gfx->preloadImage(kResBitmapShape1200_Pedestal);
	_vm->_gfx->preloadImage(kResBitmapShape2000_Storage);
	_vm->_gfx->preloadImage(kResBitmapShape2100_Buttons);

	{ // [*] Virtual Feature: Storage (refers to tBMP 2000)
		ZmbFeature::EventHooks hooksStorage;
		hooksStorage.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniInteractiveBasecampOne::storage_render));
		hooksStorage.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveBasecampOne::storage_postRender));
		ZmbFeature *vfeature = loadVirtualFeature(kVirtualFeature2000_Storage, 6,
												  ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM,
												  hooksStorage);
		vfeature->setClickRect(_storageRect);
	}

	// [*] Virtual Feature (tBMP 2100) - Go, Map, Scroll Buttons
	setGoButton(_goRouteUpButtonRect, kShape2100_GoRouteUpButtonDisabled_15, kShape2100_GoRouteUpButtonNormal_01, kShape2100_GoRouteUpButtonPressed_02);
	setSecondGoButton(_goRouteDownButtonRect, kShape2100_GoRouteDownButtonDisabled_16, kShape2100_GoRouteDownButtonNormal_03, kShape2100_GoRouteDownButtonPressed_04);
	setMapButton(_mapButtonRect, kShape2100_MapNormal_05, kShape2100_MapPressed_06);
	ZoombiniInteractive::loadGoMapButtonsFeature(kResBitmapShape2100_Buttons);

	// [*] Virtual Feature (tBMP c:0001) - Help Button
	setHelpButton(_helpButtonRect);
	ZoombiniInteractive::loadHelpButtonFeature();

	{ // [*] Virtual Feature: Storage Scroll Buttons (refers to tBMP 2100)
		ZmbFeature::EventHooks hooksScroll;
		hooksScroll.setPreRenderShapeFunc(reinterpret_cast<ZmbFeature::OnPreRenderShapeFunc>(&ZoombiniInteractiveBasecampOne::scroll_preRenderShape));
		hooksScroll.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveBasecampOne::scroll_postRender));
		hooksScroll.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractiveBasecampOne::scroll_lButtonDown));
		hooksScroll.setLButtonUpFunc(reinterpret_cast<ZmbFeature::OnLButtonUpFunc>(&ZoombiniInteractiveBasecampOne::scroll_lButtonUp));
		hooksScroll.setMouseMoveFunc(reinterpret_cast<ZmbFeature::OnMouseMoveFunc>(&ZoombiniInteractiveBasecampOne::scroll_mouseMove));

		Common::Array<ZmbHotspot> scrollHotspots;
		scrollHotspots.push_back(ZmbHotspot(0, kShape2100_ScrollLeftFourNormal_07, 0, _scrollLeftFourButtonRect));
		scrollHotspots.push_back(ZmbHotspot(1, kShape2100_ScrollLeftOneNormal_09, 0, _scrollLeftOneButtonRect));
		scrollHotspots.push_back(ZmbHotspot(2, kShape2100_ScrollRightOneNormal_11, 0, _scrollRightOneButtonRect));
		scrollHotspots.push_back(ZmbHotspot(3, kShape2100_ScrollRightFourNormal_13, 0, _scrollRightFourButtonRect));
		scrollHotspots.push_back(ZmbHotspot(4, kShape2100_ScrollLeftFourPressed_08, 0, _scrollLeftFourButtonRect));
		scrollHotspots.push_back(ZmbHotspot(5, kShape2100_ScrollLeftOnePressed_10, 0, _scrollLeftOneButtonRect));
		scrollHotspots.push_back(ZmbHotspot(6, kShape2100_ScrollRightOnePressed_12, 0, _scrollRightOneButtonRect));
		scrollHotspots.push_back(ZmbHotspot(7, kShape2100_ScrollRightFourPressed_14, 0, _scrollRightFourButtonRect));

		loadVirtualFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape2100_Buttons), kVirtualFeatureBasecamp1_ScrollButtons, scrollHotspots, 0,
						   ZmbFeature::FLAG_00001000_TOPMOST,
						   hooksScroll);
	}

	// [*] SCRB 1200 ~ 1215: Pedestals
	for (uint32 i = kResScrb1200_Pedestal; i <= kResScrb1215_Pedestal; i++) {
		loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1200_Pedestal), i, 7, _pedestalPoints[i - kResScrb1200_Pedestal],
						ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM | ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER);
	}

	{ // [*] SCRB 1106, 1108, 1109, 1110, 1107: Easter Eggs
		ZmbFeature::EventHooks hooksStoneMan;
		hooksStoneMan.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveBasecampOne::easterEggStoneMan_postRender));
		hooksStoneMan.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractiveBasecampOne::easterEggStoneMan_onLButtonDown));
		loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb1106_EasterEggStoneMan, 6,
						ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER,
						hooksStoneMan);

		ZmbFeature::EventHooks hooksFish;
		hooksFish.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveBasecampOne::easterEggFish_postRender));
		hooksFish.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractiveBasecampOne::easterEggFish_onLButtonDown));
		loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb1108_EasterEggFish, 6,
						ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER,
						hooksFish);

		ZmbFeature::EventHooks hooksBear;
		hooksBear.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveBasecampOne::easterEggBear_postRender));
		hooksBear.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractiveBasecampOne::easterEggBear_onLButtonDown));
		loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb1109_EasterEggBear, 6,
						ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE,
						hooksBear);

		ZmbFeature::EventHooks hooksStoneFace;
		hooksStoneFace.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveBasecampOne::easterEggStoneFace_postRender));
		hooksStoneFace.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractiveBasecampOne::easterEggStoneFace_onLButtonDown));
		// ZSORT_RIGHT+LEFT block snoids (entityList) from being sorted in front of StoneFace (normalList).
		loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb1110_EasterEggStoneFace, 6,
						ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_10000000_ZSORT_RIGHT | ZmbFeature::FLAG_40000000_ZSORT_LEFT,
						hooksStoneFace);

		ZmbFeature::EventHooks hooksHollowBugs;
		hooksHollowBugs.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveBasecampOne::easterEggHollowBugs_postRender));
		hooksHollowBugs.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractiveBasecampOne::easterEggHollowBugs_onLButtonDown));
		loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb1107_EasterEggHollowBugs, 6,
						ZmbFeature::FLAG_00001000_TOPMOST | ZmbFeature::FLAG_00100000_PLAY_ONCE | ZmbFeature::FLAG_01000000_DEFER_RENDER,
						hooksHollowBugs);
	}

	// [*] SCRB 1111 ~ 1115: Easter Egg Mushrooms
	// Original: bitmask=0x20000 (FLAG_00020000_SKIP_RENDER). preRenderStandard zeroes wBoolDoRender
	// each frame, but postRenderStandard draws if !DEFER_RENDER (regardless of wBoolDoRender).
	// ScummVM replicates this: preRenderFeature early-returns when !isRenderActivated, blitShapes
	// skips only if !isRenderActivated && DEFER_RENDER. Mushrooms (no DEFER_RENDER) always draw.
	// activateRender() is called in init and on each click so preRenderFeature runs once to update
	// _lastFrameIdx from bcOneMushroomColors, then FLAG_00020000 deactivates it.
	for (uint32 i = 0; i <= kResScrb1115_EasterEggMushroom5 - kResScrb1111_EasterEggMushroom1; i++) {
		ZmbFeature::EventHooks hooks;
		hooks.setSelectRenderFrameFunc(reinterpret_cast<ZmbFeature::OnSelectRenderFrameFunc>(&ZoombiniInteractiveBasecampOne::easterEggMushroom_selectRenderFrame));
		hooks.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractiveBasecampOne::easterEggMushroom_onLButtonDown));
		ZmbFeature *feature = loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), i + kResScrb1111_EasterEggMushroom1, 0,
											  ZmbFeature::FLAG_00020000_SKIP_RENDER,
											  hooks);
		feature->activateRender(); // Trigger first preRenderFeature run to load initial color from bcOneMushroomColors
	}

	{ // [*] SCRB 1104: Bonfire (randomly animates; clicking triggers Pod animation)
		ZmbFeature::EventHooks hooks;
		hooks.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractiveBasecampOne::easterEggBonfire_onLButtonDown));
		ZmbFeature *featureBonfire = loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb1104_Bonfire, 6,
													 ZmbFeature::FLAG_00040000_CHAIN_SCRIPT | ZmbFeature::FLAG_02000000_RANDOM_FRAME,
													 hooks);

		// [*] SCRB 1105: Easter Egg Pod (sub-feature owned by Bonfire)
		loadSubFeature(featureBonfire, ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), kResScrb1105_EasterEggPod);
	}

	// [*] SCRB 1100 ~ 1103: Bottom shapes
	for (uint32 i = kResScrb1100_BottomShape1; i <= kResScrb1103_BottomShape4; i++) {
		loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape1100), i, 6,
						ZmbFeature::FLAG_00000000_TYPE_SHAPES);
	}

	// [*] Zoombini Pack Management
	ZmbStateFile &f = _vm->_state->_f;

	// Storage grid init: restore leftmost column from save and recalculate column layout.
	// IDA: wBC1StorageLeftmostColumnIdx_4AAB90 = pStoredChunkBC1._leftmostColumnIdx; calcStorageColumns();
	_storageLeftmostColumnIdx = static_cast<int16>(f._storedChunkBC1._leftmostColumnIdx);
	_storageMaxCellIdx = findLastOccupiedIdx();
	calcStorageColumns();

	// Step 1: Load occupied zoombinis from ORIGINAL active pack (arriving from previous page)
	// IDA: if (zmbPackActive.wPackZmbCount) handleZoombiniAnimation_maybe_4528A6(0);
	int16 arrivingCount = 0;
	if (f._zmbPackActive._wPackZmbCount > 0) {
		arrivingCount = loadZoombinisFromPack(f._zmbPackActive, false);
	}

	// Step 2: Add arriving count to BC1 stored count
	// IDA: wStoredBC1ZmbCount += getLoadedZmbRunnerCount_452402();
	f._zmbStoredBC1Count += arrivingCount;

	// Step 3: Swap BC1 pack -> Active pack, clear BC1 pack
	// IDA: qmemcpy(zmbPackActive, zmbPackBC1, ...); zmbPackBC1.wPackZmbCount = 0; ...
	memcpy(&f._zmbPackActive, &f._zmbPackBC1, sizeof(f._zmbPackActive));
	f._wZmbPackActiveVal = f._wZmbPackBC1Val;
	f._zmbPackBC1._wPackZmbCount = 0;
	f._zmbPackBC1._bSkipOccupiedAnim = 1;
	f._zmbPackBC1._bSkipUnoccupiedAnim = 1;

	// Step 4: Storage grid management (insert arriving zoombinis into BC1 storage)
	// IDA: if (!zmbPackActive.bSkipOccupiedAnim && countOccupiedInActivePack() > 0)
	if (arrivingCount > 0 && !f._zmbPackActive._bSkipOccupiedAnim) {
		int16 occupiedInActive = 0;
		for (int16 i = 0; i < static_cast<int16>(f._zmbPackActive._wPackZmbCount); i++) {
			if (f._zmbPackActive._entries[i]._bIsOccupied != 0)
				occupiedInActive++;
		}
		if (occupiedInActive > 0) {
			// Insert arriving zoombinis into the BC1 storage grid.
			// IDA: puzzleBasecamp1_doSomething_412C5A().
			int16 prevMaxCellIdx = _storageMaxCellIdx;
			int16 storeResult = storeArrivingZoombinis();

			// Update stored count and recalculate column layout.
			f._storedChunkBC1._storedCount += static_cast<uint16>(occupiedInActive);
			_storageMaxCellIdx = findLastOccupiedIdx();
			calcStorageColumns();

			// If normal (non-overflow) insertion, scroll the view to show the newly arrived zoombinis.
			// IDA: sets wBC1StorageLeftmostColumnIdx to the column of the first new entry.
			if (storeResult == 1) {
				_storageLeftmostColumnIdx = static_cast<int16>((prevMaxCellIdx / 5) % _storageColumnCount);
				calcStorageColumns();
			}
		}
	}

	// Step 5: Load ALL zoombinis from NEW active pack (BC1 sitting zoombinis)
	// IDA: handleZoombiniAnimation_maybe_4528A6(1);
	loadZoombinisFromPack(f._zmbPackActive, true);

	// Compute total loaded zoombini count (for _canGoEnabled)
	// IDA: getLoadedZmbRunnerCount_452402() — counts all loaded zoombini features
	int16 totalLoadedCount = static_cast<int16>(_snoidMap.size());

	// Compute _notFirstArrival and _canGoEnabled
	// IDA: bc1_bFinalArrival = (generatedCount >= 625) && (bc0Count + storedBC1Count < 16)
	// IDA: bc1_bCanProceed = (totalLoaded > 0) && (bc0Count + storedBC1Count <= totalLoaded)
	_notFirstArrival = (f._zmbGeneratedCount >= 625) &&
		(static_cast<int16>(f._zmbPackIsle._wPackZmbCount) + f._zmbStoredBC1Count < 16);
	if (_notFirstArrival) {
		_canGoEnabled = (totalLoadedCount > 0) &&
			(static_cast<int16>(f._zmbPackIsle._wPackZmbCount) + f._zmbStoredBC1Count <= totalLoadedCount);
	} else {
		_canGoEnabled = (16 <= totalLoadedCount);
	}

	// Sync Go button enabled state
	setGoButtonsEnabled(_canGoEnabled);

	// Arrival voice sound (Only played when the player arrives from completing a puzzle)
	uint16 arriveSoundId = 0;
	int16 diffId = -1; // -1 = sentinel: not coming from a puzzle
	if (_vm->_state->_lastPageBeforeContainer != 0) {
		ZMB_DIFFICULTY_ID rawDiff = _vm->_state->getDifficultyIdFromPageFlag(f._pageFlagBasecamp1);
		_vm->_state->_lastPageBeforeContainer = 0;
		// Demote level2 to level1 if Ferry/Fleens never visited and few Zoombinis stored
		if (rawDiff == ZMB_DIFFICULTY_LEVEL2_02 &&
			!f._pageFlagFerry && !f._pageFlagFleens && f._zmbStoredBC1Count <= 16) {
			rawDiff = ZMB_DIFFICULTY_LEVEL1_01;
			f._pageFlagBasecamp1 &= 0xCFFFu;
		}
		diffId = static_cast<int16>(rawDiff);
	}

	// Eexpands random choice range once BC1 has been entered multiple times (bits 12-13 of flag)
	const int16 randFlag = ((f._pageFlagBasecamp1 >> 8) & 0x30) ? 6 : 4;
	if (_notFirstArrival) {
		if (diffId != -1) {
			const int16 r = static_cast<int16>(_vm->_rnd->getRandomNumber(0, 2));
			switch (r) {
			case 0:
				arriveSoundId = kResSound20051_ArriveBC1Voice;
				break;
			case 1:
				arriveSoundId = kResSound20053_ArriveBC1Voice;
				break;
			case 2:
				arriveSoundId = kResSound20054_ArriveBC1Voice;
				break;
			}
		}
	} else {
		switch (diffId) {
		case 0: { // ZMB_DIFFICULTY_NOTVISITED_00: random from first randFlag sounds
			const int16 r = static_cast<int16>(_vm->_rnd->getRandomNumber(1, static_cast<uint16>(randFlag)));
			switch (r) {
			case 1:
				arriveSoundId = kResSound20049_ArriveBC1VoiceFirst;
				break;
			case 2:
				arriveSoundId = kResSound20051_ArriveBC1Voice;
				break;
			case 3:
				arriveSoundId = kResSound20053_ArriveBC1Voice;
				break;
			case 4:
				arriveSoundId = kResSound20054_ArriveBC1Voice;
				break;
			case 5:
				arriveSoundId = kResSound20050_ArriveBC1Voice;
				break;
			case 6:
				arriveSoundId = kResSound20052_ArriveBC1Voice;
				break;
			default:
				break;
			}
			break;
		}
		case 1: // ZMB_DIFFICULTY_LEVEL1_01
		case 5: // ZMB_DIFFICULTY_LEVEL3_05 (practice)
			arriveSoundId = kResSound20049_ArriveBC1VoiceFirst;
			break;
		case 2: // ZMB_DIFFICULTY_LEVEL2_02
			arriveSoundId = kResSound20050_ArriveBC1Voice;
			break;
		case 12: // ZMB_DIFFICULTY_LEVEL4_12
			arriveSoundId = kResSound20052_ArriveBC1Voice;
			break;
		default:
			break;
		}
	}

	if (arriveSoundId)
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kSystem, arriveSoundId), Audio::Mixer::kSFXSoundType);
}

void ZoombiniInteractiveBasecampOne::onGoButtonActivated() {
	// IDA: puzzleBasecamp1_buttonClick_4117F9 case 1 (Go route up)
	// SFX 996, walk snoids to (680, 316), stagger 45 frames, fade out when SFX finishes.
	playDepartSfx();
	startDepartWalkAnimation(Common::Point(680, 316));
	_departRouteDirection = 1;
	_pendingGoDepart = true;
}

void ZoombiniInteractiveBasecampOne::onSecondGoButtonActivated() {
	// IDA: puzzleBasecamp1_buttonClick_4117F9 case 2 (Go route down)
	// SFX 996, walk snoids to (680, 400), stagger 45 frames, fade out when SFX finishes.
	playDepartSfx();
	startDepartWalkAnimation(Common::Point(680, 400));
	_departRouteDirection = 2;
	_pendingGoDepart = true;
}

void ZoombiniInteractiveBasecampOne::onEveryFrame() {
	if (!_pendingGoDepart)
		return;

	if (isDepartSfxDone()) {
		// All walkers done — save pack state and transition via xfer.
		// IDA: bc1_saveActivePackAndReadBC2 is called when pendingTransitionTarget fires.
		saveSnoidsToPack();
		saveBc1PackState(true);

		_pendingGoDepart = false;
		if (_departRouteDirection == 1)
			_vm->_xferSrcSiPage = ZMB_SI_BC1_NORTH_05;
		else
			_vm->_xferSrcSiPage = ZMB_SI_BC1_SOUTH_06;
		_vm->setNextPage(ZoombiniPageType::kXfer);
		close();
	}
}

ZmbRenderResult ZoombiniInteractiveBasecampOne::storage_render(ZmbFeature *feature) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	// Draw only the honeycomb background here (behind snoids).
	// The lattice overlay and border are drawn in storage_postRender (on top of snoids).
	// Mirrors IDA: onPostRenderVirtualSCRB_storage_tBMP2000_41265F draw order —
	//   honeycomb first, then snoids, then lattice, then border.
	uint16 matrixShapeId = _storageMatrixInAnimation ? kShapeStorage01_Honeycomb : kShapeStorage03_Honeycomb;
	ZmbResource storageBitmap = ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape2000_Storage);
	_vm->_gfx->drawShape(screenKind, storageBitmap, matrixShapeId, Common::Point(53, 6));
	return ZmbRenderResult::kRendered;
}

void ZoombiniInteractiveBasecampOne::storage_postRender(ZmbFeature *feature) {
	// Mirrors IDA: onPostRenderVirtualSCRB_storage_tBMP2000_41265F.
	// Draw stored zoombinis, then the lattice grid overlay, then the border.
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;
	ZmbStateStoredChunk &chunk = _vm->_state->_f._storedChunkBC1;

	int16 col0 = _storageLeftmostColumnIdx % _storageColumnCount;
	int16 visibleSlots = _storageMatrixInAnimation ? 30 : 25;
	int16 slotStart = 5 * col0;
	int16 col = 0, row = 0;

	for (int16 i = 0; i < visibleSlots; i++, slotStart++) {
		int16 wrappedSlot = slotStart % _storageCapacity;
		ZmbStateStoredEntry &entry = chunk._entries[wrappedSlot];

		if (entry._traits.isComplete()) {
			int16 px = static_cast<int16>(_storageMatrixInAnimation ? _storageMatrixX1[col] : _storageMatrixX2[col]);
			int16 py = static_cast<int16>(_storageMatrixInAnimation ? _storageMatrixY1[col][row] : _storageMatrixY2[col][row]);
			entry._rect = renderStoredSnoid(screenKind, entry._traits, Common::Point(px, py));
		}

		if (++row >= 5) {
			row = 0;
			++col;
		}
	}

	// Lattice overlay (drawn on top of snoids) and border (drawn last)
	ZmbResource storageBitmap = ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape2000_Storage);
	uint16 latticeShapeId = _storageMatrixInAnimation ? kShapeStorage02_Lattice : kShapeStorage04_Lattice;
	uint16 latticePosY = _storageMatrixInAnimation ? 9 : 12;
	_vm->_gfx->drawShape(screenKind, storageBitmap, latticeShapeId, Common::Point(53, latticePosY));
	_vm->_gfx->drawShape(screenKind, storageBitmap, kShapeStorage05_Border, Common::Point(31, 0));
}

void ZoombiniInteractiveBasecampOne::scroll_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	continuousButton_selectShapes(feature, hotspots, _scrollButtonStateMap);
}

void ZoombiniInteractiveBasecampOne::scroll_postRender(ZmbFeature *feature) {
	for (auto it = _scrollButtonStateMap.begin(); it != _scrollButtonStateMap.end(); it++) {
		uint32 buttonIdx = it->first;
		ContinuousButtonState &bs = it->second;

		if (!bs._enabled)
			continue;

		if (!bs._pressed)
			continue;

		switch (buttonIdx) {
		case kScrollButtons_LeftFour:
		case kScrollButtons_LeftOne: {
			// Mirrors IDA: action 1 (LeftFour = 5 steps/frame) and action 2 (LeftOne = 1 step/frame).
			// LeftFour is suppressed if not mid-animation and fewer than 5 columns remain to the left.
			int16 steps = (buttonIdx == kScrollButtons_LeftFour) ? 5 : 1;
			if (buttonIdx == kScrollButtons_LeftFour &&
			    !_storageMatrixInAnimation && _storageLeftmostColumnIdx < 5)
				steps = 0;
			for (; steps > 0; steps--) {
				if (_storageMatrixInAnimation || _storageLeftmostColumnIdx <= 0) {
					_storageMatrixInAnimation = false;
				} else {
					_storageLeftmostColumnIdx--;
					_storageMatrixInAnimation = true;
				}
			}
			_vm->_state->_f._storedChunkBC1._leftmostColumnIdx =
			    static_cast<uint16>(_storageLeftmostColumnIdx);
			break;
		}
		case kScrollButtons_RightOne:
		case kScrollButtons_RightFour: {
			// Mirrors IDA: action 3 (RightOne = 1 step/frame) and action 4 (RightFour = 5 steps/frame).
			// RightFour is suppressed if not mid-animation and fewer than 5 columns remain to the right.
			int16 maxCol = _storageColumnCount - 5;
			if (maxCol > 120)
				maxCol = 120;
			int16 steps = (buttonIdx == kScrollButtons_RightFour) ? 5 : 1;
			if (buttonIdx == kScrollButtons_RightFour &&
			    !_storageMatrixInAnimation && _storageLeftmostColumnIdx + 5 > maxCol)
				steps = 0;
			for (; steps > 0; steps--) {
				if (_storageMatrixInAnimation) {
					_storageMatrixInAnimation = false;
					if (++_storageLeftmostColumnIdx >= maxCol) {
						_storageLeftmostColumnIdx = maxCol;
						steps = 1; // clamp: stop looping after this step
					}
				} else if (_storageLeftmostColumnIdx + 1 <= 120) {
					_storageMatrixInAnimation = (_storageLeftmostColumnIdx < maxCol);
				}
			}
			_vm->_state->_f._storedChunkBC1._leftmostColumnIdx =
			    static_cast<uint16>(_storageLeftmostColumnIdx);
			break;
		}
		default:
			error("ZoombiniInteractiveBasecampOne::scroll_postRender: Invalid buttonIdx %u", buttonIdx);
			break;
		}
	}
}

ZmbEventHandleResult ZoombiniInteractiveBasecampOne::scroll_lButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	ZmbDrawRecord *drawRecord = feature->findDrawRecordAtPoint(absPos);
	if (!drawRecord)
		return ZmbEventHandleResult::kPassthrough;

	for (auto it = _scrollButtonStateMap.begin(); it != _scrollButtonStateMap.end(); it++) {
		ContinuousButtonState &bs = it->second;

		if (!bs._enabled)
			continue;

		// Find the pressed button
		if (drawRecord->_hs._hsId != bs._hsNormalIdx && drawRecord->_hs._hsId != bs._hsPressedIdx)
			continue;

		bs.press();
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, static_cast<uint16>(kResSound2000_StorageScrolling)), Audio::Mixer::kSFXSoundType, true);
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniInteractiveBasecampOne::scroll_lButtonUp(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	ZmbDrawRecord *drawRecord = feature->findDrawRecordAtPoint(absPos);
	if (!drawRecord)
		return ZmbEventHandleResult::kPassthrough;

	for (auto it = _scrollButtonStateMap.begin(); it != _scrollButtonStateMap.end(); it++) {
		ContinuousButtonState &bs = it->second;

		if (!bs._enabled)
			continue;

		// Find the pressed button
		if (drawRecord->_hs._hsId != bs._hsNormalIdx && drawRecord->_hs._hsId != bs._hsPressedIdx)
			continue;

		bs.release();
		_vm->_sound->stopZmbSound(ZmbResource(ZmbArchiveKind::kPage, kResSound2000_StorageScrolling));
		_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, kResSound2001_StorageScrollEnd), Audio::Mixer::kSFXSoundType, false);
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniInteractiveBasecampOne::scroll_mouseMove(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	uint16 cursorShapeIdx = ZmbHotspot::kShapeNone;

	ZmbDrawRecord *drawRecord = feature->findDrawRecordAtPoint(absPos);
	if (drawRecord) {
		for (auto it = _scrollButtonStateMap.begin(); it != _scrollButtonStateMap.end(); it++) {
			uint32 buttonIdx = it->first;
			ContinuousButtonState &bs = it->second;

			if (!bs._enabled)
				continue;

			// If the button was not hovered, release that button.
			// It is to handle the case of the clicking cursor moved outside wihtout releasing.
			if (drawRecord->_hs._hsId != bs._hsNormalIdx && drawRecord->_hs._hsId != bs._hsPressedIdx) {
				bs.release();
				continue;
			}

			// The button is being hovered. Set corresponding bitmap as a cursor.
			switch (buttonIdx) {
			case kScrollButtons_LeftFour:
				cursorShapeIdx = kShape9000_ArrowLeftMax_01;
				break;
			case kScrollButtons_LeftOne:
				cursorShapeIdx = kShape9000_ArrowLeft_02;
				break;
			case kScrollButtons_RightOne:
				cursorShapeIdx = kShape9000_ArrowRight_03;
				break;
			case kScrollButtons_RightFour:
				cursorShapeIdx = kShape9000_ArrowRightMax_04;
				break;
			default:
				error("ZoombiniInteractiveBasecampOne::scroll_mouseMove: Invalid buttonIdx %u", buttonIdx);
				break;
			}
		}
	}

	if (cursorShapeIdx != _storageButtonCursorShapeIdx) {
		if (cursorShapeIdx == ZmbHotspot::kShapeNone) {
			_vm->_cursor->setDefaultCursor();
		} else {
			ZmbRegs *regs = _regsMap[kResRegs9000];
			ZoombiniCursorManager *zmbCursor = dynamic_cast<ZoombiniCursorManager *>(_vm->_cursor);
			zmbCursor->setShapeCursor(ZmbArchiveKind::kPage, kResBitmapShape9000_Cursors, cursorShapeIdx, regs->getShapeDelta(cursorShapeIdx));
		}
		_storageButtonCursorShapeIdx = cursorShapeIdx;
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbRenderResult ZoombiniInteractiveBasecampOne::virt03_render(ZmbFeature *feature) {
	return ZmbRenderResult::kRendered;
}

void ZoombiniInteractiveBasecampOne::virt03_postRender(ZmbFeature *feature) {
}

ZmbEventHandleResult ZoombiniInteractiveBasecampOne::genericEasterEgg_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos, const Common::Rect &clickRect) {
	if (!clickRect.contains(absPos))
		return ZmbEventHandleResult::kPassthrough;

	// Ignore clicks while animation is already playing
	if (feature->isAnimateActivated())
		return ZmbEventHandleResult::kConsumed;

	feature->activateRender();
	feature->activateAnimate(_currentFrameCounter);
	return ZmbEventHandleResult::kConsumed;
}

void ZoombiniInteractiveBasecampOne::easterEggStoneMan_postRender(ZmbFeature *feature) {
}

ZmbEventHandleResult ZoombiniInteractiveBasecampOne::easterEggStoneMan_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericEasterEgg_onLButtonDown(feature, absPos, relPos, _easterEggStoneManRect);
}

void ZoombiniInteractiveBasecampOne::easterEggFish_postRender(ZmbFeature *feature) {
}

ZmbEventHandleResult ZoombiniInteractiveBasecampOne::easterEggFish_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericEasterEgg_onLButtonDown(feature, absPos, relPos, _easterEggFishRect);
}

void ZoombiniInteractiveBasecampOne::easterEggBear_postRender(ZmbFeature *feature) {
}

ZmbEventHandleResult ZoombiniInteractiveBasecampOne::easterEggBear_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericEasterEgg_onLButtonDown(feature, absPos, relPos, _easterEggBearRect);
}

void ZoombiniInteractiveBasecampOne::easterEggStoneFace_postRender(ZmbFeature *feature) {
}

ZmbEventHandleResult ZoombiniInteractiveBasecampOne::easterEggStoneFace_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericEasterEgg_onLButtonDown(feature, absPos, relPos, _easterEggStoneFaceRect);
}

void ZoombiniInteractiveBasecampOne::easterEggHollowBugs_postRender(ZmbFeature *feature) {
}

ZmbEventHandleResult ZoombiniInteractiveBasecampOne::easterEggHollowBugs_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	return genericEasterEgg_onLButtonDown(feature, absPos, relPos, _easterEggHollowBugsRect);
}

int16 ZoombiniInteractiveBasecampOne::findLastOccupiedIdx() const {
	const ZmbStateStoredChunk &chunk = _vm->_state->_f._storedChunkBC1;
	for (int16 i = 624; i >= 0; i--) {
		if (chunk._entries[i]._traits.isComplete())
			return i + 1;
	}
	return 0;
}

void ZoombiniInteractiveBasecampOne::calcStorageColumns() {
	// Mirrors IDA: puzzleBasecamp1_calcStorageLeftmostColumn_412868.
	// Recomputes _storageCapacity and _storageColumnCount from _storageMaxCellIdx,
	// then clamps _storageLeftmostColumnIdx so the view doesn't go past the last column.
	ZmbStateStoredChunk &chunk = _vm->_state->_f._storedChunkBC1;

	_storageCapacity = 5 * ((_storageMaxCellIdx + 10) / 5);
	if (_storageCapacity > 625)
		_storageCapacity = 625;
	if (_storageCapacity < 50)
		_storageCapacity = 50;
	_storageColumnCount = _storageCapacity / 5;

	int16 maxCol = _storageColumnCount - 5;
	if (_storageLeftmostColumnIdx > maxCol)
		_storageLeftmostColumnIdx = maxCol;

	chunk._leftmostColumnIdx = static_cast<uint16>(_storageLeftmostColumnIdx);
}

int16 ZoombiniInteractiveBasecampOne::storeArrivingZoombinis() {
	// Mirrors IDA: puzzleBasecamp1_doSomething_412C5A.
	// Inserts all occupied entries from the active pack into the BC1 storage chunk.
	// Normal path (no overflow): places them consecutively after the last occupied slot.
	// Overflow path (grid nearly full): fills the first available empty slots in-order.
	// Returns 1 on normal path, 0 on overflow.
	ZmbStateStoredChunk &chunk = _vm->_state->_f._storedChunkBC1;
	const ZmbStateActivePack &active = _vm->_state->_f._zmbPackActive;

	// Count occupied active entries (by bIsOccupied flag, not by traits)
	// IDA: countOccupiedInActivePack() at 0x452875
	int16 activeCount = 0;
	for (int16 i = 0; i < static_cast<int16>(active._wPackZmbCount); i++) {
		if (active._entries[i]._bIsOccupied != 0)
			activeCount++;
	}

	// Find insertion point: one past the last occupied stored slot
	const int16 v0 = findLastOccupiedIdx();

	if (activeCount + v0 > 624) {
		// Overflow path: fill the first empty slots in the storage array
		int16 activeIdx = 0;
		int16 stored = 0;
		for (int16 j = 0; stored < activeCount && j < 625; j++) {
			if (!chunk._entries[j]._traits.isComplete()) {
				// Advance to the next occupied active entry
				while (activeIdx < static_cast<int16>(active._wPackZmbCount) &&
				       active._entries[activeIdx]._bIsOccupied == 0)
					activeIdx++;
				if (activeIdx >= static_cast<int16>(active._wPackZmbCount))
					break;
				chunk._entries[j]._traits = active._entries[activeIdx]._traits;
				chunk._entries[j]._rect   = Common::Rect();
				memcpy(chunk._entries[j]._name, active._entries[activeIdx]._name,
				       sizeof(chunk._entries[j]._name));
				activeIdx++;
				stored++;
			}
		}
		return 0;
	} else {
		// Normal path: copy consecutively starting at v0
		int16 activeIdx = 0;
		for (int16 k = 0; k < activeCount; k++) {
			while (activeIdx < static_cast<int16>(active._wPackZmbCount) &&
			       active._entries[activeIdx]._bIsOccupied == 0)
				activeIdx++;
			if (activeIdx >= static_cast<int16>(active._wPackZmbCount))
				break;
			chunk._entries[k + v0]._traits = active._entries[activeIdx]._traits;
			chunk._entries[k + v0]._rect   = Common::Rect();
			memcpy(chunk._entries[k + v0]._name, active._entries[activeIdx]._name,
			       sizeof(chunk._entries[k + v0]._name));
			activeIdx++;
		}
		return 1;
	}
}

/**
 * Find the storage entry index at the given click area.
 *
 * Iterates through the 25 visible storage slots (5 columns x 5 rows) starting
 * from the current leftmost visible column. For occupied slots, checks if the
 * cursor point falls within the zoombini's stored rect (immediate return).
 * For empty slots, constructs a 60x60 rect centered on the grid position and
 * finds the best overlapping slot (intersection area > 625).
 *
 * @param searchOccupied If true, search for occupied slots; if false, search for empty slots.
 * @param clickRect The click area rectangle. For occupied search, only the top-left point is used.
 * @param leftmostColumnIdx The current leftmost visible column index (0~120).
 * @return The storage entry index (0~624), or -1 if no matching slot found.
 */
int16 ZoombiniInteractiveBasecampOne::findStorageSlotIndex(bool searchOccupied, const Common::Rect &clickRect, uint16 leftmostColumnIdx) {
	static constexpr uint16 kRowsPerColumn = 5;
	static constexpr uint16 kTotalColumns = ZmbTrait::SNOID_MAX / kRowsPerColumn; // 125
	static constexpr uint16 kVisibleSlots = 25;                                   // 5 visible columns * 5 rows
	static constexpr int16 kMinOverlapArea = 625;                                 // 25 * 25 minimum intersection
	static constexpr int16 kSlotHalfSize = 30;
	static constexpr int16 kSlotSize = 60;

	const ZmbStateStoredChunk &chunk = _vm->_state->_f._storedChunkBC1;
	int16 bestSlotIdx = -1;
	int16 bestArea = 0;

	uint16 entryStart = kRowsPerColumn * (leftmostColumnIdx % kTotalColumns);
	uint16 col = 0;
	uint16 row = 0;

	for (uint16 i = 0; i < kVisibleSlots; i++) {
		uint16 entryIdx = (entryStart + i) % ZmbTrait::SNOID_MAX;
		bool isOccupied = chunk._entries[entryIdx]._traits._head != ZmbTrait::TRAIT_NONE;

		if (searchOccupied == isOccupied) {
			if (isOccupied) {
				// Occupied slot: check if cursor point falls within the zoombini's stored rect
				Common::Point clickPoint(clickRect.left, clickRect.top);
				if (chunk._entries[entryIdx]._rect.contains(clickPoint)) {
					return entryIdx;
				}
			} else {
				// Empty slot: construct a 60x60 rect centered on the grid position
				int16 cx = _storageMatrixX2[col];
				int16 cy = _storageMatrixY2[col][row];
				Common::Rect slotRect(cx - kSlotHalfSize, cy - kSlotHalfSize,
									  cx - kSlotHalfSize + kSlotSize, cy - kSlotHalfSize + kSlotSize);
				Common::Rect intersection = clickRect.findIntersectingRect(slotRect);
				if (!intersection.isEmpty()) {
					int16 area = intersection.width() * intersection.height();
					if (area > kMinOverlapArea && area > bestArea) {
						bestArea = area;
						bestSlotIdx = entryIdx;
					}
				}
			}
		}

		row++;
		if (row >= kRowsPerColumn) {
			row = 0;
			col++;
		}
	}

	return bestSlotIdx;
}

ZmbEventHandleResult ZoombiniInteractiveBasecampOne::easterEggBonfire_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	if (!feature->findDrawRecordAtPoint(absPos))
		return ZmbEventHandleResult::kPassthrough;

	feature->runSubFeature(this);
	return ZmbEventHandleResult::kConsumed;
}

// IDA: bc1_onHotspotClick 0x411EE8 — click_findRunnerAtPoint(1, 0x20000, cursorPos) finds mushroom
// runner by clickRect containment. Original postRenderStandard draws non-DEFER_RENDER features
// regardless of wBoolDoRender. activateRender() triggers one preRenderFeature run (updating
// _lastFrameIdx from bcOneMushroomColors), then FLAG_00020000 deactivates; blitShapes still draws.
uint32 ZoombiniInteractiveBasecampOne::easterEggMushroom_selectRenderFrame(ZmbFeature *feature) {
	uint16 stateIdx = feature->getId() - kResScrb1111_EasterEggMushroom1;
	assert(stateIdx <= kResScrb1115_EasterEggMushroom5 - kResScrb1111_EasterEggMushroom1);
	return _vm->_state->_f._bcOneMushroomColors[stateIdx];
}

ZmbEventHandleResult ZoombiniInteractiveBasecampOne::easterEggMushroom_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	// Original: click_findRunnerAtPoint uses clickRect, not drawRecord — always hittable.
	if (!feature->isPointInClickRect(absPos))
		return ZmbEventHandleResult::kPassthrough;

	uint16 i = feature->getId() - kResScrb1111_EasterEggMushroom1;
	assert(i <= kResScrb1115_EasterEggMushroom5 - kResScrb1111_EasterEggMushroom1);

	// IDA: bcOneMushroomColors[i] = (wGroupFrameIdx0098 + 1) % (wScriptFrameCount + 1)
	uint16 colorCount = static_cast<uint16>(feature->getMaxFrameIdx() + 1);
	_vm->_state->_f._bcOneMushroomColors[i] = (_vm->_state->_f._bcOneMushroomColors[i] + 1) % colorCount;

	// Activate so preRenderFeature runs once, updating _lastFrameIdx to new color.
	// FLAG_00020000 deactivates it after; blitShapes draws anyway (no DEFER_RENDER).
	feature->activateRender();

	uint16 soundId = i + kResSound1118_EasterEggMushroom1;
	_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, soundId), Audio::Mixer::kSFXSoundType, false);

	return ZmbEventHandleResult::kConsumed;
}

void ZoombiniInteractiveBasecampOne::saveSnoidsToPack() {
	// Mirrors IDA: save_updateZmbPacksOnPuzzleComplete(0, 1) for the BC1 case.
	// Saves all loaded snoids back into f._zmbPackActive with a two-pass iteration:
	//   Pass 1 (v36=1): save occupied snoids (on pedestals) first.
	//   Pass 2 (v36=0): save non-occupied snoids (at arbitrary positions).
	// IDA: pGameState->zmbPackActive.wPackZmbCount = zmb_countFeatureRunners();
	ZmbStateFile &f = _vm->_state->_f;

	int16 snoidCount = 0;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if (it->second->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			snoidCount++;
	}

	f._zmbPackActive._bSkipOccupiedAnim = 0;
	f._zmbPackActive._bSkipUnoccupiedAnim = 0;
	f._zmbPackActive._wPackZmbCount = snoidCount;

	int16 packIdx = 0;
	for (int pass = 0; pass < 2 && packIdx < 16; pass++) {
		bool wantOccupied = (pass == 0);
		for (auto it = _snoidMap.begin(); it != _snoidMap.end() && packIdx < 16; ++it) {
			ZmbSnoid *snoid = it->second;
			if (!snoid->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
				continue;
			if (snoid->_packIsOccupied != wantOccupied)
				continue;

			ZmbStateActiveEntry &entry = f._zmbPackActive._entries[packIdx];
			entry._traits = snoid->_trait;
			entry._posX = static_cast<uint16>(snoid->getPointLoc().x);
			entry._posY = static_cast<uint16>(snoid->getPointLoc().y);
			entry._bIsOccupied = wantOccupied ? 1 : 0;
			// Convert U32String name back to raw bytes
			Common::String nameBytes = _vm->_text->fromU32String(snoid->_name);
			memset(entry._name, 0, sizeof(entry._name));
			uint32 nameLen = MIN<uint32>(nameBytes.size(), sizeof(entry._name));
			memcpy(entry._name, nameBytes.c_str(), nameLen);

			packIdx++;
		}
	}
}

void ZoombiniInteractiveBasecampOne::saveBc1PackState(bool isDeparture) {
	// Mirrors IDA: bc1_saveActivePackAndReadBC2 (0x4115CF).
	// Must be called AFTER saveSnoidsToPack() has populated f._zmbPackActive.
	ZmbStateFile &f = _vm->_state->_f;

	if (!isDeparture) {
		// Map button: all snoids stay at BC1.
		// IDA: (wChangeColorPaletteState_4A4462 || puzzle_nextPuzzleId == 1) path
		f._zmbPackActive._bSkipOccupiedAnim = 0;
		f._zmbPackActive._bSkipUnoccupiedAnim = 0;
		memcpy(&f._zmbPackBC1, &f._zmbPackActive, sizeof(f._zmbPackBC1));
		f._wZmbPackBC1Val = f._wZmbPackActiveVal;
		f._zmbPackActive._wPackZmbCount = 0;
	} else {
		// Go button departure: occupied snoids leave, non-occupied stay at BC1.
		// IDA: else path in bc1_saveActivePackAndReadBC2
		f._zmbPackActive._bSkipOccupiedAnim = 1;
		f._zmbPackActive._bSkipUnoccupiedAnim = 0;
		memcpy(&f._zmbPackBC1, &f._zmbPackActive, sizeof(f._zmbPackBC1));
		f._wZmbPackBC1Val = f._wZmbPackActiveVal;
		f._zmbPackActive._bSkipOccupiedAnim = 0;
		f._zmbPackActive._bSkipUnoccupiedAnim = 1;

		// Reduce stored BC1 count by the number of occupied snoids that are departing.
		int16 occupiedCount = 0;
		for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount; i++) {
			if (f._zmbPackActive._entries[i]._bIsOccupied)
				occupiedCount++;
		}
		f._zmbStoredBC1Count -= occupiedCount;
	}

	// Recalculate storage columns and save leftmost column to stored chunk.
	// IDA: puzzleBasecamp1_calcStorageLeftmostColumn_412868(0xFFFF) — reset/clamp.
	calcStorageColumns();
	f._storedChunkBC1._leftmostColumnIdx = static_cast<uint16>(_storageLeftmostColumnIdx);
}

void ZoombiniInteractiveBasecampOne::onMapButtonActivated() {
	// IDA: bc1_onButtonClick case 3 (map button)
	// Save snoids back to pack, then swap active → BC1, then transition.
	saveSnoidsToPack();
	saveBc1PackState(false);
	_vm->setNextPage(ZoombiniPageType::kRodMap);
	close();
}

// ---------------------------------------------------------------------------
// Drag-and-drop implementation
// IDA: beginDragFeatureRunner_45360F + bc1_onHotspotClick (0x411A90)
// ---------------------------------------------------------------------------

int16 ZoombiniInteractiveBasecampOne::findNearestEmptyPedestal(const Common::Point &pos) const {
	int16 bestIdx = -1;
	int32 bestDistSq = kPedestalSnapRadiusSq;

	for (int16 i = 0; i < 16; i++) {
		// Check if this pedestal is already occupied by another snoid
		bool isOccupied = false;
		for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
			const ZmbSnoid *other = it->second;
			if (other == _draggedSnoid)
				continue;
			if (!other->_packIsOccupied)
				continue;
			const Common::Point &opos = other->getPointLoc();
			int32 dx = opos.x - _pedestalPoints[i].x;
			int32 dy = opos.y - _pedestalPoints[i].y;
			if (dx * dx + dy * dy < 100) { // within 10px of pedestal center
				isOccupied = true;
				break;
			}
		}
		if (isOccupied)
			continue;

		int32 dx = pos.x - _pedestalPoints[i].x;
		int32 dy = pos.y - _pedestalPoints[i].y;
		int32 distSq = dx * dx + dy * dy;
		if (distSq < bestDistSq) {
			bestDistSq = distSq;
			bestIdx = i;
		}
	}
	return bestIdx;
}

void ZoombiniInteractiveBasecampOne::endDrag(const Common::Point &dropPos) {
	ZmbSnoid *snoid = finishSnoidDrag();

	ZmbStateFile &f = _vm->_state->_f;

	// Check drop target: pedestal, storage, or free space
	int16 pedestalIdx = findNearestEmptyPedestal(dropPos);

	if (pedestalIdx >= 0) {
		// --- Drop on pedestal ---
		// IDA: wFeatureRunnerIdxArr_4B7E38[dropSlotIdx] = origRunnerIdx
		snoid->_packIsOccupied = true;
		snoid->setAnimTargetPos(_pedestalPoints[pedestalIdx]);
		snoid->setAnimState(kSnoidAnimArrive);
	} else if (_storageRect.contains(dropPos)) {
		// --- Drop on storage grid ---
		// IDA: bc1_findStorageSlotByClick(searchOccupied=false) → copy traits+name → free runner
		// Use a rect centered on the drop position for slot intersection
		Common::Rect dropRect(dropPos.x - 15, dropPos.y - 15, dropPos.x + 15, dropPos.y + 15);
		int16 emptySlot = findStorageSlotIndex(false, dropRect, static_cast<uint16>(_storageLeftmostColumnIdx));
		if (emptySlot >= 0) {
			ZmbStateStoredChunk &chunk = f._storedChunkBC1;
			chunk._entries[emptySlot]._traits = snoid->_trait;
			Common::String nameBytes = _vm->_text->fromU32String(snoid->_name);
			memset(chunk._entries[emptySlot]._name, 0, sizeof(chunk._entries[emptySlot]._name));
			uint32 nameLen = MIN<uint32>(nameBytes.size(), sizeof(chunk._entries[emptySlot]._name));
			memcpy(chunk._entries[emptySlot]._name, nameBytes.c_str(), nameLen);
			chunk._entries[emptySlot]._rect = Common::Rect();

			// Free the snoid runner
			unloadSnoid(snoid->getId());

			// Recalculate storage columns
			_storageMaxCellIdx = findLastOccupiedIdx();
			calcStorageColumns();

			// Update stored count
			f._zmbStoredBC1Count++;

			// Dragged from storage originally: no net change
			// Dragged from field: +1 stored, Go button may change
		} else {
			// No empty slot found: return to original position.
			// IDA: BC1 has no terrain bitmap, so terrain_validateAndPlaceSnoid
			// returns 0 → snoid always returns to original position.
			snoid->_packIsOccupied = false;
			snoid->setPointLoc(_dragOrigPos);
			snoid->setAnimState(kSnoidAnimIdle);
			snoid->setupIdleHotspots();
		}
	} else {
		// --- Drop in free space ---
		// IDA: terrain_validateAndPlaceSnoid (0x453D28) — BC1 has no terrain
		// bitmap loaded, so returns 0 → snoid returns to original position.
		// Pages WITH terrain would validate and stay if walkable.
		if (!validateTerrainDrop(snoid)) {
			snoid->setPointLoc(_dragOrigPos);
		}
		snoid->_packIsOccupied = false;
		snoid->setAnimState(kSnoidAnimIdle);
		snoid->setupIdleHotspots();
	}

	// If snoid was picked from storage and dropped successfully (not back to storage),
	// decrement stored count was already done in the pickup phase.
	// If it was placed back into storage, count was incremented above.

	// Update Go button enabled state
	// IDA: bc1_bFinalArrival / bc1_bCanProceed
	int16 totalLoadedCount = 0;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if (it->second->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			totalLoadedCount++;
	}

	if (_notFirstArrival) {
		int16 islePlusStored = static_cast<int16>(f._zmbPackIsle._wPackZmbCount) + f._zmbStoredBC1Count;
		_canGoEnabled = (0 < totalLoadedCount && islePlusStored <= totalLoadedCount);
	} else {
		_canGoEnabled = (16 <= totalLoadedCount);
	}
	setGoButtonsEnabled(_canGoEnabled);

	_dragFromStorage = false;
	_dragStorageOriginSlot = -1;
}

ZmbEventHandleResult ZoombiniInteractiveBasecampOne::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// In sticky mouse mode, a second click ends the drag
	if (isDragging() && _vm->_state->getEnableStickyMouse()) {
		endDrag(absPos);
		return ZmbEventHandleResult::kConsumed;
	}

	// Let the base class handle button clicks first (Go, Map, Help, etc.)
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Don't start a new drag while already dragging
	if (isDragging())
		return ZmbEventHandleResult::kPassthrough;

	// Don't start drag while departure is pending
	if (_pendingGoDepart)
		return ZmbEventHandleResult::kPassthrough;

	// --- ClickIdx 1: Find snoid at cursor or pick from storage ---
	// IDA: bc1_onHotspotClick case wClickIdx==1
	ZmbSnoid *snoid = findSnoidAtPoint(absPos);

	if (!snoid) {
		// Try to pick from storage grid
		// IDA: bc1_findStorageSlotByClick(searchOccupied=true, clickRect, scrollColumn)
		if (_storageRect.contains(absPos)) {
			Common::Rect clickRect(absPos.x, absPos.y, absPos.x, absPos.y);
			int16 storageIdx = findStorageSlotIndex(true, clickRect, static_cast<uint16>(_storageLeftmostColumnIdx));
			if (storageIdx >= 0) {
				ZmbStateFile &f = _vm->_state->_f;
				ZmbStateStoredChunk &chunk = f._storedChunkBC1;

				// Decrement stored count
				if (f._zmbStoredBC1Count > 0)
					f._zmbStoredBC1Count--;

				// Create a new snoid from the storage entry
				uint16 snoidId = kSnoidPackBase + _nextPackSnoidId++;
				ZmbSnoid *newSnoid = loadSnoidFromPack(snoidId, absPos,
													   ZmbFeature::FLAG_00000001_TYPE_SNOID);
				if (newSnoid) {
					newSnoid->_trait = chunk._entries[storageIdx]._traits;
					newSnoid->_name = chunk._entries[storageIdx].getU32Name(_vm);
					newSnoid->_packIsOccupied = false;
					newSnoid->setupIdleHotspots();
				}

				// Clear the storage slot
				chunk._entries[storageIdx]._traits = ZmbTrait();

				// Recalculate storage
				_storageMaxCellIdx = findLastOccupiedIdx();
				calcStorageColumns();

				snoid = newSnoid;
				_dragFromStorage = true;
				_dragStorageOriginSlot = storageIdx;
			}
		}
	}

	if (!snoid)
		return ZmbEventHandleResult::kPassthrough;

	// Don't drag snoids that are playing scripts or walking
	SnoidAnimState state = snoid->getAnimState();
	if (state == kSnoidAnimScriptReject || state == kSnoidAnimScriptNormal ||
		state == kSnoidAnimWalkRight || state == kSnoidAnimWalkLeft ||
		state == kSnoidAnimDepart || state == kSnoidAnimPath ||
		state == kSnoidAnimArrivalMotion)
		return ZmbEventHandleResult::kPassthrough;

	// --- Begin drag ---
	// IDA: beginDragFeatureRunner_45360F entry (detach, set anim, hide cursor, show notibox)
	startSnoidDrag(snoid, absPos);
	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniInteractiveBasecampOne::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	if (!isDragging()) {
		return ZoombiniInteractive::onLButtonUp(absPos, relPos);
	}

	// In sticky mouse mode, button-up does NOT end drag (click again to drop)
	if (_vm->_state->getEnableStickyMouse())
		return ZmbEventHandleResult::kConsumed;

	// Non-sticky mode: button release ends drag
	endDrag(absPos);
	return ZmbEventHandleResult::kConsumed;
}

int16 ZoombiniInteractiveBasecampOne::loadZoombinisFromPack(ZmbStateActivePack &pack, bool loadNonOccupied) {
	int16 count = 0;
	uint16 occupiedPosIdx = 0;

	for (int16 i = 0; i < pack._wPackZmbCount; i++) {
		ZmbStateActiveEntry &entry = pack._entries[i];

		// Skip entries without complete traits
		if (!entry._traits.isComplete())
			continue;

		// IDA: (bIsOccupied && !wUnkA930_zero) || (!bIsOccupied && a2 && !wUnkA932_zero)
		bool isOccupied = (entry._bIsOccupied != 0);
		if (isOccupied && pack._bSkipOccupiedAnim)
			continue;
		if (!isOccupied && (!loadNonOccupied || pack._bSkipUnoccupiedAnim))
			continue;

		// Determine position
		// IDA: occupied → sub_4535B5(++v9, &posLoc) uses pedestal positions
		//      non-occupied → position from entry data (wUnk04/wUnk06)
		Common::Point pos;
		if (isOccupied) {
			if (occupiedPosIdx >= 16)
				continue;
			pos = _pedestalPoints[occupiedPosIdx];
			occupiedPosIdx++;
		} else {
			pos = Common::Point(entry._posX,
							entry._posY);
		}

		// Create ZmbSnoid from pack entry
		uint16 snoidId = kSnoidPackBase + _nextPackSnoidId++;
		ZmbSnoid *snoid = loadSnoidFromPack(snoidId, pos,
											ZmbFeature::FLAG_00000001_TYPE_SNOID);
		if (snoid) {
			snoid->_trait = entry._traits;
			snoid->_name = entry.getU32Name(_vm);
			snoid->_packIsOccupied = isOccupied;
			snoid->setupIdleHotspots();
		}

		count++;
	}

	pack._wPackZmbCount = 0;

	return count;
}

} // End of namespace Mohawk
