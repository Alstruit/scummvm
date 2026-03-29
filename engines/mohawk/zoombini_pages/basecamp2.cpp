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
#include "mohawk/zoombini_pages/basecamp2.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"
#include "mohawk/zoombini_text.h"

namespace Mohawk {

ZoombiniInteractiveBasecampTwo::ZoombiniInteractiveBasecampTwo(MohawkEngine_Zoombini *vm)
	: ZoombiniInteractive(vm, ZoombiniPageType::kBasecamp2) {
	memset(_buttonAnimRunnerIdxs, 0, sizeof(_buttonAnimRunnerIdxs));
}

ZoombiniInteractiveBasecampTwo::~ZoombiniInteractiveBasecampTwo() {
}

void ZoombiniInteractiveBasecampTwo::open() {
	openArchive(ZMB_MHK_BCTWO);
	loadREGS(ZmbArchiveKind::kPage, kResRegs10000);
}

void ZoombiniInteractiveBasecampTwo::setBackgroundMusic() {
	// BC2 intentionally has no background music in the original game.
	// IDA: bc2_initAndSetupPuzzle (0x412E68) has no call to playBgm/loadBgmTrack.
	// Ambient audio comes from SCRS feature animations (storage scroll, transport, etc.).
}

void ZoombiniInteractiveBasecampTwo::setBackgroundBitmap() {
	_vm->_gfx->setPalette(kResBackground5000);
	_vm->_gfx->drawBackground(kResBackground5000);
}

void ZoombiniInteractiveBasecampTwo::loadFeatures() {
	ZmbStateFile &f = _vm->_state->_f;

	// -- Read persisted storage state --
	_storageLeftmostColumnIdx = (int16)f._storedChunkBC2._leftmostColumnIdx;
	_storedCount = (int16)f._storedChunkBC2._storedCount;
	_storageLastOccupiedIdx = findLastOccupiedSlot();
	recalcStorageCapacity(-1);

	// -- Preload shape bitmaps --
	_vm->_gfx->preloadImage(kResBitmapShape6000_Main);
	_vm->_gfx->preloadImage(kResBitmapShape7000_Pedestal);
	_vm->_gfx->preloadImage(kResBitmapShape8000_Storage);
	_vm->_gfx->preloadImage(kResBitmapShape9000_Buttons);

	// Load NODE/PATH for walk network
	// IDA: node_loadNodeAndPath(0x3E8u)
	loadNODE(ZmbArchiveKind::kPage, kResNode1000);

	// Load terrain barrier bitmap (tBMP 100) for walkability checks.
	// IDA: rmap_loadTerrainArchive(0x64) — 160x120 mask, pixel==1 means walkable.
	loadTerrainBitmap(kResBitmapTerrain100);

	{ // [*] Virtual Feature: Storage area (no SCRB; preRender=scroll SM, postRender=draw grid)
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderFunc(reinterpret_cast<ZmbFeature::OnPreRenderFunc>(&ZoombiniInteractiveBasecampTwo::storage_preRender));
		hooks.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveBasecampTwo::storage_postRender));
		ZmbFeature *vf = loadVirtualFeature(kVirtualFeature_Storage, 6,
											ZmbFeature::FLAG_00004000_NO_DIRTY_MERGE | ZmbFeature::FLAG_00008000_LOOP_ANIM,
											hooks);
		vf->setClickRect(_storageRect);
		_storageRunnerIdx = kVirtualFeature_Storage;
	}

	{ // [*] Virtual Feature: Scroll-button panel (postRender draws scroll arrows via SHPL 9000)
		ZmbFeature::EventHooks hooks;
		hooks.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveBasecampTwo::buttons_postRender));
		loadVirtualFeature(kVirtualFeature_Buttons, 0,
						   ZmbFeature::FLAG_00001000_TOPMOST | ZmbFeature::FLAG_00008000_LOOP_ANIM,
						   hooks);
	}

	{ // [*] Virtual Feature: Go/Map/Save buttons
		ZmbFeature::EventHooks hooks;
		hooks.setPreRenderFunc(reinterpret_cast<ZmbFeature::OnPreRenderFunc>(&ZoombiniInteractiveBasecampTwo::goButton_preRender));
		hooks.setPostRenderFunc(reinterpret_cast<ZmbFeature::OnPostRenderFunc>(&ZoombiniInteractiveBasecampTwo::goButton_postRender));
		hooks.setLButtonDownFunc(reinterpret_cast<ZmbFeature::OnLButtonDownFunc>(&ZoombiniInteractiveBasecampTwo::goButton_onLButtonDown));
		ZmbFeature *vf = loadVirtualFeature(kVirtualFeature_GoButton, 0,
						   ZmbFeature::FLAG_00001000_TOPMOST,
						   hooks);
		vf->setClickRect(Common::Rect(0x0257, 0x0140, 0x027E, 0x018B)); // covers Go + Map
	}

	// [*] SCRB 7000 ~ 7015: Pedestals (16 spots for the active Zoombini pack)
	for (uint32 i = 0; i < 16; i++) {
		loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape7000_Pedestal),
						kResScrb7000_Pedestal + i, 7, _pedestalPoints[i],
						ZmbFeature::FLAG_00002000_DRAW_ON_REG | ZmbFeature::FLAG_00008000_LOOP_ANIM |
							ZmbFeature::FLAG_00080000_DEFER_ANIM | ZmbFeature::FLAG_01000000_DEFER_RENDER);
	}

	// [*] SCRB 6000: Transport animation (looping; fires once when armed)
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape6000_Main),
					kResScrb6000_TransportLoop, 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00080000_DEFER_ANIM |
						ZmbFeature::FLAG_00100000_PLAY_ONCE |
						ZmbFeature::FLAG_01000000_DEFER_RENDER |
						ZmbFeature::FLAG_04000000_OVERLAY);
	_transportAnimRunnerIdx = kResScrb6000_TransportLoop;

	// [*] SCRB 6005: Button animation 0
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape6000_Main),
					kResScrb6005_Button0, 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00080000_DEFER_ANIM |
						ZmbFeature::FLAG_00100000_PLAY_ONCE);
	_buttonAnimRunnerIdxs[0] = kResScrb6005_Button0;

	// [*] SCRB 6011: Button animation 1
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape6000_Main),
					kResScrb6011_Button1, 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00080000_DEFER_ANIM |
						ZmbFeature::FLAG_00100000_PLAY_ONCE);
	_buttonAnimRunnerIdxs[1] = kResScrb6011_Button1;

	// [*] SCRB 6010: Button animation 2
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape6000_Main),
					kResScrb6010_Button2, 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00080000_DEFER_ANIM |
						ZmbFeature::FLAG_00100000_PLAY_ONCE |
						ZmbFeature::FLAG_01000000_DEFER_RENDER);
	_buttonAnimRunnerIdxs[2] = kResScrb6010_Button2;

	// [*] SCRB 6002: Button animation 3 (round-trip toggle, used with 6003)
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape6000_Main),
					kResScrb6002_ButtonRoundTripA, 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00080000_DEFER_ANIM |
						ZmbFeature::FLAG_00100000_PLAY_ONCE |
						ZmbFeature::FLAG_04000000_OVERLAY);
	_buttonAnimRunnerIdxs[3] = kResScrb6002_ButtonRoundTripA;

	// [*] SCRB 6004: Button animation 4
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape6000_Main),
					kResScrb6004_Button3, 6,
					ZmbFeature::FLAG_00080000_DEFER_ANIM |
						ZmbFeature::FLAG_00100000_PLAY_ONCE |
						ZmbFeature::FLAG_04000000_OVERLAY);
	_buttonAnimRunnerIdxs[4] = kResScrb6004_Button3;

	// [*] SCRB 6009: Button animation 5
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape6000_Main),
					kResScrb6009_Button5, 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00080000_DEFER_ANIM |
						ZmbFeature::FLAG_00100000_PLAY_ONCE |
						ZmbFeature::FLAG_01000000_DEFER_RENDER);
	_buttonAnimRunnerIdxs[5] = kResScrb6009_Button5;

	// [*] SCRB 6006: Button animation 6
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape6000_Main),
					kResScrb6006_Button6, 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00080000_DEFER_ANIM |
						ZmbFeature::FLAG_00100000_PLAY_ONCE |
						ZmbFeature::FLAG_01000000_DEFER_RENDER |
						ZmbFeature::FLAG_04000000_OVERLAY);
	_buttonAnimRunnerIdxs[6] = kResScrb6006_Button6;

	// [*] SCRB 6007: Button animation 7
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape6000_Main),
					kResScrb6007_Button7, 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00080000_DEFER_ANIM |
						ZmbFeature::FLAG_00100000_PLAY_ONCE |
						ZmbFeature::FLAG_01000000_DEFER_RENDER |
						ZmbFeature::FLAG_04000000_OVERLAY);
	_buttonAnimRunnerIdxs[7] = kResScrb6007_Button7;

	// [*] SCRB 6008: Button animation 8
	loadScrbFeature(ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape6000_Main),
					kResScrb6008_Button8, 6,
					ZmbFeature::FLAG_00008000_LOOP_ANIM |
						ZmbFeature::FLAG_00080000_DEFER_ANIM |
						ZmbFeature::FLAG_00100000_PLAY_ONCE |
						ZmbFeature::FLAG_01000000_DEFER_RENDER |
						ZmbFeature::FLAG_04000000_OVERLAY);
	_buttonAnimRunnerIdxs[8] = kResScrb6008_Button8;

	// Button animation 9 = transport anim (same runner as _transportAnimRunnerIdx)
	_buttonAnimRunnerIdxs[9] = _transportAnimRunnerIdx;

	// -- Zoombini Pack Management (mirrors BC1 two-phase loading) --
	// IDA: bc2_initAndSetupPuzzle (0x412E68)

	// Phase 1: Load arriving zoombinis from CURRENT active pack
	// IDA: if (zmbPackActive.wPackZmbCount > 0) zmb_loadAnimationsFromActivePack(0)
	int16 arrivingCount = 0;
	if (f._zmbPackActive._wPackZmbCount > 0) {
		arrivingCount = loadZoombinisFromPack(f._zmbPackActive, false);
	}

	// Phase 2: Add arriving count to BC2 stored count, swap BC2→Active
	// IDA: storedBC2Count += loadedCount; qmemcpy(active, BC2, ...); BC2.count = 0
	f._zmbStoredBC2Count += arrivingCount;
	memcpy(&f._zmbPackActive, &f._zmbPackBC2, sizeof(f._zmbPackActive));
	f._wZmbPackActiveVal = f._wZmbPackBC2Val;
	f._zmbPackBC2._wPackZmbCount = 0;
	f._zmbPackBC2._bSkipOccupiedAnim = 1;
	f._zmbPackBC2._bSkipUnoccupiedAnim = 1;

	// Phase 3: Handle arriving occupied zoombinis → store to belt
	// IDA: if (!active.bSkipOccupiedAnim && countOccupied() > 0) store + update
	if (arrivingCount > 0 && !f._zmbPackActive._bSkipOccupiedAnim) {
		int16 occupiedInActive = 0;
		for (int16 i = 0; i < static_cast<int16>(f._zmbPackActive._wPackZmbCount); i++) {
			if (f._zmbPackActive._entries[i]._bIsOccupied != 0)
				occupiedInActive++;
		}
		if (occupiedInActive > 0) {
			int16 prevLastIdx = _storageLastOccupiedIdx;
			int16 wrapped = placeZoombinisIntoStorage(occupiedInActive);
			_storedCount += occupiedInActive;
			f._storedChunkBC2._storedCount += static_cast<uint16>(occupiedInActive);
			_storageLastOccupiedIdx = findLastOccupiedSlot();
			recalcStorageCapacity(-1);
			if (wrapped) {
				_storageLeftmostColumnIdx = (int16)(((prevLastIdx + 1) / 5) % _storageColumnCount);
				recalcStorageCapacity(-1);
			}
			f._zmbPackActive._bSkipOccupiedAnim = 1;
		}
	}

	// Phase 4: Load BC2 resident zoombinis from the NEW active pack
	// IDA: zmb_loadAnimationsFromActivePack(1) — loads non-occupied (BC2 residents)
	loadZoombinisFromPack(f._zmbPackActive, true);

	// Compute total loaded zoombini count (for _canGoEnabled)
	int16 totalLoadedCount = static_cast<int16>(_snoidMap.size());

	// IDA: bc2_bNotFirstArrival = (generatedCount >= 625) && (bc0Count + storedBC2Count < 16)
	_notFirstArrival = (f._zmbGeneratedCount >= 625) &&
		(static_cast<int16>(f._zmbPackIsle._wPackZmbCount) + f._zmbStoredBC2Count < 16);
	if (_notFirstArrival) {
		_canGoEnabled = (totalLoadedCount > 0) &&
			(static_cast<int16>(f._zmbPackIsle._wPackZmbCount) + f._zmbStoredBC2Count <= totalLoadedCount);
	} else {
		_canGoEnabled = (16 <= totalLoadedCount);
	}
	setGoButtonsEnabled(_canGoEnabled);

	// Persist leftmost column index back to state
	f._storedChunkBC2._leftmostColumnIdx = static_cast<uint16>(_storageLeftmostColumnIdx);
	f._storedChunkBC2._storedCount = static_cast<uint16>(_storedCount);
}

bool ZoombiniInteractiveBasecampTwo::storage_preRender(ZmbFeature *feature) {
	// Scroll state machine — runs every frame for LOOP_ANIM feature.
	// Mirrors sub_4142BF logic: advances _storageLeftmostColumnIdx by the
	// appropriate step count each time the update interval elapses.

	if (_currentFrameCounter < _storageNextUpdateFrame)
		return true;

	_storageNextUpdateFrame = _currentFrameCounter + kStorageScrollInterval;

	if (_scrollDirection == kScrollDir_None) {
		return true;
	}

	// Mark as rendering (reset sort rect to force re-sort)
	feature->setSortRect(Common::Rect());

	if (_scrollDirection == kScrollDir_LeftMax) {
		// Scroll left by 5 columns at a time
		int16 step = 5;
		if (!_scrollAnimating && _storageLeftmostColumnIdx - 5 < 0)
			step = 0;
		while (step > 0) {
			if (_scrollAnimating) {
				_scrollAnimating = false;
			} else if (0 < _storageLeftmostColumnIdx) {
				--_storageLeftmostColumnIdx;
				if (_storageLeftmostColumnIdx < 0) {
					_storageLeftmostColumnIdx = 0;
					step = 1;
				}
				_scrollAnimating = true;
			}
			--step;
		}
		if (!_scrollAnimating)
			_scrollDirection = kScrollDir_None;
	} else if (_scrollDirection == kScrollDir_LeftOne) {
		// Expand storage capacity if we are at the leftmost column
		if (!_storageLeftmostColumnIdx)
			expandStorageCapacity();
		// Scroll left by 1 column
		if (_scrollAnimating || _storageLeftmostColumnIdx <= 0) {
			_scrollAnimating = false;
		} else {
			--_storageLeftmostColumnIdx;
			if (_storageLeftmostColumnIdx < 0) {
				_storageLeftmostColumnIdx = 0;
				// stop after overshooting
			}
			_scrollAnimating = true;
		}
		if (!_scrollAnimating)
			_scrollDirection = kScrollDir_None;
	} else if (_scrollDirection == kScrollDir_RightOne) {
		// Scroll right by 1 column
		if (_scrollAnimating) {
			_scrollAnimating = false;
			int16 newVal = _storageLeftmostColumnIdx + 1;
			if (_storageColumnCount - 5 <= newVal) {
				_storageLeftmostColumnIdx = _storageColumnCount - 5;
				if (120 < _storageColumnCount - 5)
					_storageLeftmostColumnIdx = 120;
				// reached right edge
			}
		} else if (_storageLeftmostColumnIdx + 1 <= 120) {
			_scrollAnimating = (_storageLeftmostColumnIdx < _storageColumnCount - 5);
		}

		if (!_scrollAnimating)
			_scrollDirection = kScrollDir_None;
	} else if (_scrollDirection == kScrollDir_RightMax) {
		// Scroll right by 5 columns at a time
		int16 step = 5;
		if (!_scrollAnimating && _storageColumnCount - 5 < _storageLeftmostColumnIdx + 5)
			step = 0;
		while (step > 0) {
			if (_scrollAnimating) {
				_scrollAnimating = false;
				_storageLeftmostColumnIdx++;
				if (_storageColumnCount - 5 <= _storageLeftmostColumnIdx) {
					_storageLeftmostColumnIdx = _storageColumnCount - 5;
					if (120 < _storageColumnCount - 5)
						_storageLeftmostColumnIdx = 120;
					step = 1;
				}
				_scrollAnimating = true;
			} else if (_storageLeftmostColumnIdx + 1 <= 120) {
				_scrollAnimating = (_storageLeftmostColumnIdx < _storageColumnCount - 5);
			}
			--step;
		}
		if (!_scrollAnimating)
			_scrollDirection = kScrollDir_None;
	}

	// Write leftmost column back to state
	_vm->_state->_f._storedChunkBC2._leftmostColumnIdx = (uint16)_storageLeftmostColumnIdx;
	return true;
}

void ZoombiniInteractiveBasecampTwo::storage_postRender(ZmbFeature *feature) {
	// Mirrors sub_4144A0: draws honeycomb/lattice/border + stored Zoombinis.

	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;
	const ZmbStateStoredChunk &chunk = _vm->_state->_f._storedChunkBC2;

	// Wrap leftmost column within capacity
	int16 col0 = _storageLeftmostColumnIdx % _storageColumnCount;

	// Choose shapes based on scroll animation state
	uint16 honeycombShape, latticeShape;
	int16 latticeOffX, latticeOffY;
	if (_scrollAnimating) {
		honeycombShape = kShape8000_StorageAnim_Honeycomb;
		latticeShape = kShape8000_StorageAnim_Lattice;
		latticeOffX = -1 + 141; // 140
		latticeOffY = -3 + 28;  // 25
	} else {
		honeycombShape = kShape8000_StorageStill_Honeycomb;
		latticeShape = kShape8000_StorageStill_Lattice;
		latticeOffX = 141;
		latticeOffY = 28;
	}

	// Resource 8000 uses the SHPL system: shape N lives as a separate tBMP at resource (8000 + N - 1).
	// drawShape's sub-image path (decodeImages) cannot decode these single-image resources.
	// Use drawBackground with individual resource IDs instead (single-image decode path).
	_vm->_gfx->drawImage(screenKind, kResBitmapShape8000_Storage + honeycombShape - 1, Common::Point(140, 23));
	_vm->_gfx->drawImage(screenKind, kResBitmapShape8000_Storage + latticeShape - 1, Common::Point(latticeOffX, latticeOffY));
	_vm->_gfx->drawImage(screenKind, kResBitmapShape8000_Storage + kShape8000_StorageBorder - 1, Common::Point(101, 0));

	// Draw stored Zoombinis in grid
	int visibleCols = _scrollAnimating ? 6 : 5;
	int slotIdx = 5 * col0;
	int col = 0;
    int row = 0;

	for (int i = 0; i < visibleCols * 5; i++, slotIdx++) {
		int wrappedSlot = slotIdx % _storageCapacity;
		const ZmbStateStoredEntry &entry = chunk._entries[wrappedSlot];

		if (entry._traits._head != 0 || entry._traits._eye != 0) {
			uint16 px, py;
			if (_scrollAnimating) {
				px = _storageMatrixX_anim[col];
				py = _storageMatrixY_anim[col][row];
			} else {
				px = _storageMatrixX_nonanim[col];
				py = _storageMatrixY_nonanim[col][row];
			}
			Common::Rect drawnRect = renderStoredSnoid(screenKind, entry._traits, Common::Point((int16)px, (int16)py));
			// Store the rendered rect back for click-testing in findStorageSlotIndex.
			const_cast<ZmbStateStoredEntry &>(entry)._rect = drawnRect;
		}

		++row;
		if (row >= 5) {
			row = 0;
			++col;
		}
	}
}

void ZoombiniInteractiveBasecampTwo::buttons_postRender(ZmbFeature *feature) {
	// Draws scroll arrows (button group 2 = v4=4..7).
	renderButtons(false, 2, false, 0);
}

bool ZoombiniInteractiveBasecampTwo::goButton_preRender(ZmbFeature *feature) {
	// Sync _canGoVisible with _canGoEnabled each frame.
	// IDA: The original just toggles between shapes 1 (enabled) and 15 (disabled)
	// in renderButtons based on bridge_bAllZmbOnField. No separate animation SCRB.
	if (_canGoEnabled != _canGoVisible) {
		_canGoVisible = _canGoEnabled;
		// Go button state changed: mark the button feature dirty for re-render.
		// The actual shape change (1 vs 15) happens in renderButtons().
		feature->setSortRect(Common::Rect());
	}
	return true;
}

void ZoombiniInteractiveBasecampTwo::goButton_postRender(ZmbFeature *feature) {
	// Draw go/map/save buttons, then draw the special Save button.
	renderButtons(false, 1, false, 0);
	renderButtons(false, 0, false, 4);
}

ZmbEventHandleResult ZoombiniInteractiveBasecampTwo::goButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos) {
	// IDA: caves_entranceBridge_funcOnClick_413740
	if (_goButtonRect.contains(absPos) && _canGoEnabled && !_pendingGoDepart) {
		// Case 1: Go button — SFX 996, walk snoids to (680, 316), fade out when SFX finishes.
		playDepartSfx();

		// IDA: zmbMoveAnimation_45479D(45, 316, 680)
		startDepartWalkAnimation(Common::Point(680, 316));
		_pendingGoDepart = true;
		return ZmbEventHandleResult::kConsumed;
	}

	if (_mapButtonRect.contains(absPos)) {
		// Case 3: Map button — save snoids back to pack, then go to road map.
		// IDA: bc2_cleanupOnExit map path (skipOccupied=0, skipUnoccupied=0)
		saveSnoidsToPack();
		saveBc2PackState(false);
		_vm->setNextPage(ZoombiniPageType::kRodMap);
		close();
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

// ---------------------------------------------------------------------------
// Page-level mouse handlers — Snoid drag/drop
// IDA: bc2_onHotspotHover (arg0=1 → pick up, arg0=2 → drag/drop)
// ---------------------------------------------------------------------------

ZmbEventHandleResult ZoombiniInteractiveBasecampTwo::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	// Let base class handle button/feature clicks first.
	ZmbEventHandleResult result = ZoombiniInteractive::onLButtonDown(absPos, relPos);
	if (result == ZmbEventHandleResult::kConsumed)
		return result;

	// Don't start a new drag while already dragging or departure pending
	if (isDragging() || _pendingGoDepart)
		return ZmbEventHandleResult::kPassthrough;

	// --- Path 1: Find field snoid under cursor ---
	// IDA: bc2_onHotspotHover arg0==1 → click_findRunnerAtPoint
	ZmbSnoid *snoid = findSnoidAtPoint(absPos);

	if (!snoid) {
		// --- Path 2: Pick from storage belt ---
		// IDA: caves_findHotspotUnderCursor_414124(1, cursorPos, scrollOffset)
		if (_storageRect.contains(absPos)) {
			int16 storageIdx = findStorageSlotIndex(true, absPos,
												   static_cast<uint16>(_storageLeftmostColumnIdx));
			if (storageIdx >= 0) {
				ZmbStateFile &f = _vm->_state->_f;
				ZmbStateStoredChunk &chunk = f._storedChunkBC2;

				// Create a temporary snoid from the storage entry
				uint16 snoidId = kSnoidPackBase + _nextPackSnoidId++;
				ZmbSnoid *newSnoid = loadSnoidFromPack(snoidId, absPos,
													   ZmbFeature::FLAG_00000001_TYPE_SNOID);
				if (newSnoid) {
					newSnoid->_trait = chunk._entries[storageIdx]._traits;
					newSnoid->_name = chunk._entries[storageIdx].getU32Name(_vm);
					newSnoid->_packIsOccupied = false;
					newSnoid->setupIdleHotspots();
				}

				// Mark storage origin (don't clear slot yet — wait for successful drop)
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

	// Begin drag
	startSnoidDrag(snoid, absPos);
	_dragInProgress = true;
	_dragActive = true;
	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniInteractiveBasecampTwo::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	if (!isDragging())
		return ZoombiniInteractive::onLButtonUp(absPos, relPos);

	ZmbSnoid *snoid = finishSnoidDrag();
	_dragInProgress = false;
	_dragActive = false;

	ZmbStateFile &f = _vm->_state->_f;
	Common::Point dropPos = snoid->getPointLoc();

	if (_dragFromStorage) {
		// --- Snoid was picked from storage belt ---
		// IDA: bc2_onHotspotHover arg0==2, dragFromStorage path
		if (!_storageRect.contains(dropPos) && validateTerrainDrop(snoid)) {
			// Dropped on valid terrain → place on field
			// Clear the original storage slot now
			ZmbStateStoredChunk &chunk = f._storedChunkBC2;
			chunk._entries[_dragStorageOriginSlot]._traits = ZmbTrait();
			chunk._entries[_dragStorageOriginSlot]._rect = Common::Rect();

			// Update stored count & capacity
			if (_storedCount > 0)
				_storedCount--;
			if (f._zmbStoredBC2Count > 0)
				f._zmbStoredBC2Count--;
			_storageLastOccupiedIdx = findLastOccupiedSlot();
			recalcStorageCapacity(-1);

			snoid->_packIsOccupied = false;
			snoid->setAnimState(kSnoidAnimIdle);
			snoid->setupIdleHotspots();
		} else {
			// Dropped back on storage belt or invalid terrain → return to storage
			// Remove the temporary snoid
			unloadSnoid(snoid->getId());
		}
	} else {
		// --- Snoid was picked from field ---
		if (_storageRect.contains(dropPos)) {
			// Dropped on storage belt → store the snoid
			// IDA: caves_findHotspotUnderCursor_414124(0, dropRect, scrollOffset)
			int16 emptySlot = findStorageSlotIndex(false, dropPos,
												   static_cast<uint16>(_storageLeftmostColumnIdx));
			if (emptySlot >= 0) {
				ZmbStateStoredChunk &chunk = f._storedChunkBC2;
				chunk._entries[emptySlot]._traits = snoid->_trait;
				Common::String nameBytes = _vm->_text->fromU32String(snoid->_name);
				memset(chunk._entries[emptySlot]._name, 0, sizeof(chunk._entries[emptySlot]._name));
				uint32 nameLen = MIN<uint32>(nameBytes.size(), sizeof(chunk._entries[emptySlot]._name));
				memcpy(chunk._entries[emptySlot]._name, nameBytes.c_str(), nameLen);
				chunk._entries[emptySlot]._rect = Common::Rect();

				// Free the field snoid
				unloadSnoid(snoid->getId());

				// Update storage state
				_storedCount++;
				f._zmbStoredBC2Count++;
				_storageLastOccupiedIdx = findLastOccupiedSlot();
				recalcStorageCapacity(-1);
			} else {
				// No empty slot found → return to original position
				snoid->setPointLoc(_dragOrigPos);
				snoid->setAnimState(kSnoidAnimIdle);
				snoid->setupIdleHotspots();
			}
		} else {
			// Dropped in free space → validate terrain
			if (!validateTerrainDrop(snoid)) {
				snoid->setPointLoc(_dragOrigPos);
			}
			snoid->setAnimState(kSnoidAnimIdle);
			snoid->setupIdleHotspots();
		}
	}

	// Update Go button state
	int16 totalFieldCount = 0;
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		if (it->second->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID))
			totalFieldCount++;
	}
	_canGoEnabled = (totalFieldCount > 0);
	setGoButtonsEnabled(_canGoEnabled);

	_dragFromStorage = false;
	_dragStorageOriginSlot = -1;

	return ZmbEventHandleResult::kConsumed;
}

void ZoombiniInteractiveBasecampTwo::onEveryFrame() {
	if (!_pendingGoDepart)
		return;

	if (isDepartSfxDone()) {
		// All walkers done — save pack state and transition via xfer to tunnels.
		// IDA: bc2_cleanupOnExit (0x4134D9)
		saveSnoidsToPack();
		saveBc2PackState(true);

		_pendingGoDepart = false;
		_vm->_xferSrcSiPage = ZMB_SI_BASECAMP2_13;
		_vm->setNextPage(ZoombiniPageType::kXfer);
		close();
	}
}

void ZoombiniInteractiveBasecampTwo::renderButtons(bool blit, int group, bool pressed, int singleButton) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;
	ZmbResource btnBitmap = ZmbResource(ZmbArchiveKind::kPage, kResBitmapShape9000_Buttons);

	// Determine v4 (start button index) and v12 (end button index, exclusive)
	int v4, v12;
	if (singleButton > 0) {
		v4 = singleButton - 1;
		v12 = singleButton;
	} else if (group == 1) {
		v4 = 0;
		v12 = 3; // go/map/save: buttons 0,1,2
	} else if (group == 2) {
		v4 = 4;
		v12 = 8; // scroll arrows: buttons 4,5,6,7
	} else {
		v4 = 0;
		v12 = 8; // all buttons
	}

	// Per-button render positions (decoded from dword_4A0384, 36-byte stride)
	// Format: {x, y} draw position for each button index (0..7)
	static const Common::Point kButtonPos[8] = {
		Common::Point(0x0257, 0x0140), // 0: Go button
		Common::Point(0x018C, 0x0001), // 1: (unused/notification box)
		Common::Point(0x0257, 0x0166), // 2: Map/Return button
		Common::Point(0x0257, 0x018C), // 3: Save/special (SCRB path)
		Common::Point(0x0072, 0x0079), // 4: Scroll Left-Max arrow
		Common::Point(0x0083, 0x0075), // 5: Scroll Left-One arrow
		Common::Point(0x0151, 0x006B), // 6: Scroll Right-One arrow
		Common::Point(0x015E, 0x006E), // 7: Scroll Right-Max arrow
	};

	bool hasScrollButton = false;

	for (int i = v4; i < v12; i++) {
		uint16 shapeIdx = 0;
		bool isBtnPressed = pressed;

		if (i == 0) {
			// Go button
			shapeIdx = _canGoEnabled ? kShape9000_GoEnabled_01 : kShape9000_GoDisabled_15;
			isBtnPressed = _canGoEnabled ? isBtnPressed : false;
		} else if (i == 2) {
			// Map/Return button
			shapeIdx = kShape9000_MapNormal_05;
		} else if (i == 3) {
			// Save/Help button — uses shape 24, rendered via drawShape
			// IDA: slotIdx == 3 → result = 24, rendered with gfx_blitBitmapShape
			shapeIdx = kShape9000_HelpNormal_24;
		} else if (i >= 4 && i <= 7) {
			// Scroll arrow buttons
			hasScrollButton = true;
			shapeIdx = (uint16)(kShape9000_ScrollLMaxNormal_07 + 2 * (i - 4));
			isBtnPressed = (_currentScrollButton - 1 == i);
		} else {
			// Button 1 has no drawn shape in the original loop
			continue;
		}

		if (shapeIdx == 0)
			continue;

		if (isBtnPressed)
			++shapeIdx; // pressed variant is always +1

		Common::Point pos = kButtonPos[i];
		_vm->_gfx->drawShape(screenKind, btnBitmap, shapeIdx, pos);
	}

	if (blit && hasScrollButton) {
		// In the original, gfx_blitActivePortToRect was called here for immediate
		// button feedback during scroll hold. In ScummVM, screen updates are handled
		// by the main loop's flushScreens(), so we just mark the screen dirty.
		_vm->_gfx->setDirty();
	}
}

int16 ZoombiniInteractiveBasecampTwo::findLastOccupiedSlot() {
	// Scan from slot 624 downward for first occupied slot.
	const ZmbStateStoredChunk &chunk = _vm->_state->_f._storedChunkBC2;
	for (int16 i = 624; i >= 0; i--) {
		const ZmbTrait &t = chunk._entries[i]._traits;
		if (t._head != 0 || t._eye != 0)
			return i;
	}
	return 0;
}

void ZoombiniInteractiveBasecampTwo::recalcStorageCapacity(int16 newSlotIdx) {
	// Update _storageCapacity and _storageColumnCount based on the highest occupied slot index.
	const ZmbStateStoredChunk &chunk = _vm->_state->_f._storedChunkBC2;
	(void)chunk;

	if ((uint16)newSlotIdx <= 0x270u && _storedCount < 625) {
		++_storedCount;
		if (newSlotIdx > _storageLastOccupiedIdx)
			_storageLastOccupiedIdx = newSlotIdx;
	}

	_storageCapacity = (int16)(5 * ((_storageLastOccupiedIdx + 10) / 5));
	if (_storageCapacity > 625)
		_storageCapacity = 625;
	if (_storageCapacity < 50)
		_storageCapacity = 50;

	_storageColumnCount = _storageCapacity / 5;

	if (_storageLeftmostColumnIdx > _storageCapacity / 5 - 5)
		_storageLeftmostColumnIdx = _storageColumnCount - 5;

	// Write back to persistent state
	_vm->_state->_f._storedChunkBC2._leftmostColumnIdx = (uint16)_storageLeftmostColumnIdx;
	_vm->_state->_f._storedChunkBC2._storedCount = (uint16)_storedCount;
}

void ZoombiniInteractiveBasecampTwo::expandStorageCapacity() {
	// If slots 0..4 are occupied and slots 620..624 are empty, 
    // shift all entries right by 5 slots to open a new leftmost column.

	const ZmbStateStoredChunk &chunk = _vm->_state->_f._storedChunkBC2;

	// Check that the first 5 slots are occupied
	bool firstFiveOccupied = false;
	for (int i = 0; i < 5; i++) {
		if (chunk._entries[i]._traits._head != 0 || chunk._entries[i]._traits._eye != 0) {
			firstFiveOccupied = true;
			break;
		}
	}
	if (!firstFiveOccupied || _storageColumnCount >= 125)
		return;

	// Check that the last 5 slots are empty
	bool lastFiveEmpty = true;
	for (int i = 620; i < 625; i++) {
		if (chunk._entries[i]._traits._head != 0 || chunk._entries[i]._traits._eye != 0) {
			lastFiveEmpty = false;
			break;
		}
	}
	if (!lastFiveEmpty)
		return;

	// Shift entries right by 5
	ZmbStateStoredChunk &mchunk = _vm->_state->_f._storedChunkBC2;
	for (int i = 619; i >= 0; i--) {
		mchunk._entries[i + 5] = mchunk._entries[i];
		mchunk._entries[i]._traits = ZmbTrait();
	}
	_storageLastOccupiedIdx += 5;
	recalcStorageCapacity(-1);
	_storageLeftmostColumnIdx++;
	_vm->_state->_f._storedChunkBC2._leftmostColumnIdx = (uint16)_storageLeftmostColumnIdx;

	updateScrollSound(kScrollDir_LeftMax, false);
}

int16 ZoombiniInteractiveBasecampTwo::placeZoombinisIntoStorage(int16 occupiedCount) {
	// Place occupied entries from the active Zoombini pack into the storage grid.
	// Mirrors bc1_storeArrivingZoombinis (IDA: 0x412C5A).
	// Returns 1 if Zoombinis fit contiguously after the last occupied slot, or 0 if wrapped.

	ZmbStateStoredChunk &chunk = _vm->_state->_f._storedChunkBC2;
	const ZmbStateActivePack &active = _vm->_state->_f._zmbPackActive;

	// Find first empty slot after current occupants
	int16 startSlot = 0;
	for (int16 i = 624; i >= 0; i--) {
		if (chunk._entries[i]._traits._head != 0 || chunk._entries[i]._traits._eye != 0) {
			startSlot = i + 1;
			break;
		}
	}

	if (occupiedCount + startSlot > 624) {
		// Overflow path: scatter into first available empty slots
		int16 activeIdx = 0;
		int16 stored = 0;
		for (int16 j = 0; stored < occupiedCount && j < 625; j++) {
			if (chunk._entries[j]._traits._head == 0 && chunk._entries[j]._traits._eye == 0) {
				while (activeIdx < static_cast<int16>(active._wPackZmbCount) &&
				       active._entries[activeIdx]._bIsOccupied == 0)
					activeIdx++;
				if (activeIdx >= static_cast<int16>(active._wPackZmbCount))
					break;
				chunk._entries[j]._traits = active._entries[activeIdx]._traits;
				chunk._entries[j]._rect = Common::Rect();
				memcpy(chunk._entries[j]._name, active._entries[activeIdx]._name,
				       sizeof(chunk._entries[j]._name));
				activeIdx++;
				stored++;
			}
		}
		return 0; // wrapped
	} else {
		// Normal path: copy consecutively starting at startSlot
		int16 activeIdx = 0;
		for (int16 k = 0; k < occupiedCount; k++) {
			while (activeIdx < static_cast<int16>(active._wPackZmbCount) &&
			       active._entries[activeIdx]._bIsOccupied == 0)
				activeIdx++;
			if (activeIdx >= static_cast<int16>(active._wPackZmbCount))
				break;
			chunk._entries[startSlot + k]._traits = active._entries[activeIdx]._traits;
			chunk._entries[startSlot + k]._rect = Common::Rect();
			memcpy(chunk._entries[startSlot + k]._name, active._entries[activeIdx]._name,
			       sizeof(chunk._entries[startSlot + k]._name));
			activeIdx++;
		}
		return 1; // no wrap
	}
}

void ZoombiniInteractiveBasecampTwo::compactStorage() {
	// Count leading empty slots and, if 5+ empty at the left, shift all entries left to remove dead columns.

	ZmbStateStoredChunk &chunk = _vm->_state->_f._storedChunkBC2;

	int16 leadingEmpty = -5;
	for (int i = 0; i < 625; i++) {
		if (chunk._entries[i]._traits._head != 0 || chunk._entries[i]._traits._eye != 0)
			break;
		leadingEmpty++;
	}
	if (leadingEmpty < 5)
		return;

	int16 shiftBy = (int16)(5 * (leadingEmpty / 5));
	if (shiftBy == 0)
		return;

	for (int16 i = shiftBy; i < 625; i++) {
		chunk._entries[i - shiftBy] = chunk._entries[i];
		chunk._entries[i]._traits = ZmbTrait();
	}
	_storageLastOccupiedIdx -= shiftBy;
	_storageLeftmostColumnIdx -= shiftBy / 5;
	if (_storageLastOccupiedIdx < 0)
		_storageLastOccupiedIdx = 0;
	if (_storageLeftmostColumnIdx < 0)
		_storageLeftmostColumnIdx = 0;
	_vm->_state->_f._storedChunkBC2._leftmostColumnIdx = (uint16)_storageLeftmostColumnIdx;
}

void ZoombiniInteractiveBasecampTwo::resetStorageSortRect() {
	// Reset the storage feature's sort rect.
	auto storageIt = _virtualFeatureMap.find(_storageRunnerIdx);
	ZmbFeature *storage = (storageIt != _virtualFeatureMap.end()) ? storageIt->second : nullptr;
	if (storage)
		storage->setSortRect(Common::Rect());
}

int16 ZoombiniInteractiveBasecampTwo::findStorageSlotIndex(bool searchOccupied, const Common::Point &cursorPos, uint16 leftmostColumnIdx) {
	// Find a storage slot by cursor position.
	// Searches the 25-slot visible window starting at leftmostColumnIdx.

	const ZmbStateStoredChunk &chunk = _vm->_state->_f._storedChunkBC2;

	int16 best = -1;
	int bestArea = 0;
	int slotIdx = 5 * (leftmostColumnIdx % _storageColumnCount);
	int col = 0, row = 0;

	for (int i = 0; i < 25; i++, slotIdx++) {
		int16 wrappedSlot = (int16)(slotIdx % _storageCapacity);
		const ZmbStateStoredEntry &entry = chunk._entries[wrappedSlot];
		bool occupied = (entry._traits._head != 0 || entry._traits._eye != 0);

		if ((searchOccupied && occupied) || (!searchOccupied && !occupied)) {
			if (occupied) {
				// Use the recorded click rect from the entry
				if (entry._rect.contains(cursorPos))
					return wrappedSlot;
			} else {
				// Empty slot: build a 60×60 bounding box from matrix position
				uint16 px = _storageMatrixX_nonanim[col];
				uint16 py = _storageMatrixY_nonanim[col][row];
				Common::Rect slotRect((int16)(px - 30), (int16)(py - 30),
									  (int16)(px + 30), (int16)(py + 30));
				if (slotRect.intersects(Common::Rect(cursorPos.x, cursorPos.y, cursorPos.x + 1, cursorPos.y + 1))) {
					int area = slotRect.width() * slotRect.height();
					if (area > 625 && area > bestArea) {
						bestArea = area;
						best = wrappedSlot;
					}
				}
			}
		}

		++row;
		if (row >= 5) {
			row = 0;
			++col;
		}
	}
	return best;
}

void ZoombiniInteractiveBasecampTwo::updateScrollSound(int scrollDir, bool forceReset) {
	// Play or stop the storage-scroll sound depending on
	// whether there is still room to scroll in the active direction.

	int16 canScroll = 0;
	switch (scrollDir) {
	case kScrollDir_LeftMax:
		canScroll = (_storageLeftmostColumnIdx > 4) ? 1 : 0;
		break;
	case kScrollDir_LeftOne:
		canScroll = (_storageLeftmostColumnIdx > 0) ? 1 : 0;
		break;
	case kScrollDir_RightOne:
		if (_storageLeftmostColumnIdx < _storageColumnCount - 5)
			canScroll = (_storageLeftmostColumnIdx + 1 <= 120) ? 1 : 0;
		break;
	case kScrollDir_RightMax:
		if (_storageLeftmostColumnIdx < _storageColumnCount - 9)
			canScroll = (_storageLeftmostColumnIdx + 5 <= 120) ? 1 : 0;
		break;
	default:
		return;
	}

	if (canScroll != _scrollSoundState) {
		_scrollSoundState = canScroll;
		if (canScroll == 0) {
			_vm->_sound->stopZmbSound(ZmbResource(ZmbArchiveKind::kPage, kResSound2000_StorageScrolling));
		} else {
			_vm->_sound->playZmbSound(ZmbResource(ZmbArchiveKind::kPage, kResSound2000_StorageScrolling),
									  Audio::Mixer::kSFXSoundType, true);
		}
	}

	if (forceReset) {
		_vm->_sound->stopZmbSound(ZmbResource(ZmbArchiveKind::kPage, kResSound2000_StorageScrolling));
		_scrollSoundState = 0;
	}
}

int16 ZoombiniInteractiveBasecampTwo::loadZoombinisFromPack(ZmbStateActivePack &pack, bool loadNonOccupied) {
	// Mirrors BC1's loadZoombinisFromPack / IDA: zmb_loadAnimationsFromActivePack(animFlags).
	// Loads zoombinis from the given pack into snoid features on-screen.
	// If loadNonOccupied is false, loads only occupied entries (arriving snoids).
	// If loadNonOccupied is true, loads only non-occupied entries (BC2 residents).
	int16 count = 0;
	uint16 occupiedPosIdx = 0;

	for (int16 i = 0; i < pack._wPackZmbCount; i++) {
		ZmbStateActiveEntry &entry = pack._entries[i];

		if (!entry._traits.isComplete())
			continue;

		// IDA: (bIsOccupied && !skipOccupied) || (!bIsOccupied && animFlags && !skipUnoccupied)
		bool isOccupied = (entry._bIsOccupied != 0);
		if (isOccupied && pack._bSkipOccupiedAnim)
			continue;
		if (!isOccupied && (!loadNonOccupied || pack._bSkipUnoccupiedAnim))
			continue;

		Common::Point pos;
		if (isOccupied) {
			if (occupiedPosIdx >= 16)
				continue;
			pos = _pedestalPoints[occupiedPosIdx];
			occupiedPosIdx++;
		} else {
			pos = Common::Point(entry._posX, entry._posY);
		}

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

void ZoombiniInteractiveBasecampTwo::saveSnoidsToPack() {
	// Mirrors BC1's saveSnoidsToPack / IDA: save_updateZmbPacksOnPuzzleComplete(0,1).
	// Two-pass save: occupied first (pass 0), then non-occupied (pass 1).
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
			Common::String nameBytes = _vm->_text->fromU32String(snoid->_name);
			memset(entry._name, 0, sizeof(entry._name));
			uint32 nameLen = MIN<uint32>(nameBytes.size(), sizeof(entry._name));
			memcpy(entry._name, nameBytes.c_str(), nameLen);

			packIdx++;
		}
	}
}

void ZoombiniInteractiveBasecampTwo::saveBc2PackState(bool isDeparture) {
	// Mirrors BC1's saveBc1PackState / IDA: bc2_cleanupOnExit (0x4134D9).
	// Must be called AFTER saveSnoidsToPack() has populated f._zmbPackActive.
	ZmbStateFile &f = _vm->_state->_f;

	if (!isDeparture) {
		// Map button: all snoids stay at BC2.
		f._zmbPackActive._bSkipOccupiedAnim = 0;
		f._zmbPackActive._bSkipUnoccupiedAnim = 0;
		memcpy(&f._zmbPackBC2, &f._zmbPackActive, sizeof(f._zmbPackBC2));
		f._wZmbPackBC2Val = f._wZmbPackActiveVal;
		f._zmbPackActive._wPackZmbCount = 0;
	} else {
		// Go button departure: occupied snoids leave, non-occupied stay at BC2.
		f._zmbPackActive._bSkipOccupiedAnim = 1;
		f._zmbPackActive._bSkipUnoccupiedAnim = 0;
		memcpy(&f._zmbPackBC2, &f._zmbPackActive, sizeof(f._zmbPackBC2));
		f._wZmbPackBC2Val = f._wZmbPackActiveVal;
		f._zmbPackActive._bSkipOccupiedAnim = 0;
		f._zmbPackActive._bSkipUnoccupiedAnim = 1;

		// Reduce stored BC2 count by the number of occupied snoids departing.
		int16 occupiedCount = 0;
		for (int16 i = 0; i < f._zmbPackActive._wPackZmbCount; i++) {
			if (f._zmbPackActive._entries[i]._bIsOccupied)
				occupiedCount++;
		}
		f._zmbStoredBC2Count -= occupiedCount;
	}

	// Persist storage state
	_vm->_state->_f._storedChunkBC2._leftmostColumnIdx = static_cast<uint16>(_storageLeftmostColumnIdx);
	_vm->_state->_f._storedChunkBC2._storedCount = static_cast<uint16>(_storedCount);
}

} // End of namespace Mohawk
