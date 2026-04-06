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

#ifndef MOHAWK_ZOOMBINI_PAGES_BASECAMP2_H
#define MOHAWK_ZOOMBINI_PAGES_BASECAMP2_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

class ZoombiniInteractiveBasecampTwo : public ZoombiniInteractive {
public:
	ZoombiniInteractiveBasecampTwo(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveBasecampTwo() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;
	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onMouseMove(const Common::Point &absPos, const Common::Point &relPos) override;

protected:
	void endDrag(const Common::Point &dropPos);
	void executeDeparture() override;
	// [*] Virtual feature callbacks — Storage
	bool storage_preRender(ZmbFeature *feature);
	void storage_postRender(ZmbFeature *feature);

	// [*] Virtual feature callbacks — Scroll buttons area
	void buttons_postRender(ZmbFeature *feature);

	// [*] Virtual feature callbacks — Go/Map/Save buttons
	bool goButton_preRender(ZmbFeature *feature);
	void goButton_postRender(ZmbFeature *feature);
	ZmbEventHandleResult goButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);

	/**
	 * Renders one or more BC2 buttons using SHPL 9000 shapes.
	 *
	 * Corresponds to sub_413E91 in the original binary.
	 *
	 * @param blit          If true, blits the button region to screen after drawing.
	 * @param group         0 = all, 1 = go/map/save group (v4=0..2), 2 = scroll arrows (v4=4..7).
	 * @param pressed       Treat the specific button as pressed (+1 shape index).
	 * @param singleButton  When > 0, draw only that one button (1-indexed; 0 = draw by group).
	 */
	void renderButtons(bool blit, int group, bool pressed, int singleButton);

	// [*] Storage management helpers
	int16 findLastOccupiedSlot();
	void recalcStorageCapacity(int16 newSlotIdx);
	void expandStorageCapacity();
	int16 placeZoombinisIntoStorage(int16 occupiedCount);
	void compactStorage();
	void resetStorageSortRect();

	// [*] Pack management (mirrors BC1 pattern)
	int16 loadZoombinisFromPack(ZmbStateActivePack &pack, bool loadNonOccupied);
	void saveSnoidsToPack();
	void saveBc2PackState(bool isDeparture);

	// [*] Storage slot searching
	int16 findStorageSlotIndex(bool searchOccupied, const Common::Point &cursorPos, uint16 leftmostColumnIdx);

	// [*] Scroll sound helper (sub_414981)
	void updateScrollSound(int scrollDir, bool forceReset);

	/**
	 * Updates pedestal highlight during snoid drag.
	 * Activates blink animation on nearest empty pedestal within hover radius;
	 * deactivates the previous highlight if hovering a different pedestal or none.
	 * IDA: beginDragFeatureRunner_45360F (~0x453A23–0x453B4B)
	 * @param snoidPos Current snoid position during drag.
	 */
	void updatePedestalHover(const Common::Point &snoidPos);

	/**
	 * Deactivates any active pedestal highlight when drag ends.
	 */
	void deactivatePedestalHover();

	/**
	 * Checks button animation hotspots and triggers decorative animations.
	 * IDA: bc2_onHotspotHover (0x41392D) button hotspot loop.
	 * @param cursorPos Current cursor position to check against hotspots.
	 * @return True if a button animation was triggered, false otherwise.
	 */
	bool updateButtonAnimations(const Common::Point &cursorPos);

	/**
	 * Plays random arrival voice line based on difficulty.
	 * IDA: bc2_initAndSetupPuzzle end section (~0x4133E0–0x4134CD).
	 */
	void playArrivalVoice();

	// -----------------------------------------------------------------------
	// Resource IDs
	// -----------------------------------------------------------------------
	enum PageResourceId : uint32 {
		kResBackground5000 = 5000,

		kResBitmapShape6000_Main = 6000,
		kResBitmapShape7000_Pedestal = 7000,
		kResBitmapShape8000_Storage = 8000,
		kResBitmapShape9000_Buttons = 9000,
		kResBitmapTerrain100 = 100,

		kResNode1000 = 1000,
		kResRegs10000 = 10000,

		// SCRBs within tBMP group 6000 (= 0x1770..0x177B)
		kResScrb6000_TransportLoop = 6000,    // 0x1770: transport anim (loop)
		kResScrb6001_TransportFire = 6001,    // 0x1771: transport activation
		kResScrb6002_ButtonRoundTripA = 6002, // 0x1772
		kResScrb6003_ButtonRoundTripB = 6003, // 0x1773
		kResScrb6004_Button3 = 6004,          // 0x1774
		kResScrb6005_Button0 = 6005,          // 0x1775
		kResScrb6006_Button6 = 6006,          // 0x1776
		kResScrb6007_Button7 = 6007,          // 0x1777
		kResScrb6008_Button8 = 6008,          // 0x1778
		kResScrb6009_Button5 = 6009,          // 0x1779
		kResScrb6010_Button2 = 6010,          // 0x177A
		kResScrb6011_Button1 = 6011,          // 0x177B

		// SCRBs within tBMP group 7000 (pedestals 7000..7015)
		kResScrb7000_Pedestal = 7000,

		kResSound2000_StorageScrolling = 2000,
		kResSound2001_StorageScrollEnd = 2001,
		kResSound996_Go = 996,
		kResSound999_Button = 999,
		kResSound20082_BC2Voice1 = 20082,
		kResSound20084_BC2Voice2 = 20084,
		kResSound20085_BC2Voice3 = 20085,
	};

	// -----------------------------------------------------------------------
	// Shape indices within SHPL 8000 (storage) and SHPL 9000 (buttons)
	// -----------------------------------------------------------------------
	enum ShapeId : uint16 {
		// SHPL 8000 — storage panel
		kShape8000_StorageAnim_Honeycomb = 1,
		kShape8000_StorageAnim_Lattice = 2,
		kShape8000_StorageStill_Honeycomb = 3,
		kShape8000_StorageStill_Lattice = 4,
		kShape8000_StorageBorder = 5,

		// SHPL 9000 — button panel (all shapes used by renderButtons)
		kShape9000_GoEnabled_01 = 1,
		kShape9000_GoPressed_02 = 2,
		// index 3: unused in loop
		kShape9000_MapNormal_05 = 5,
		kShape9000_MapPressed_06 = 6,
		// index 7..14: scroll arrows (2×normal, 2×pressed per direction)
		kShape9000_ScrollLMaxNormal_07 = 7,
		kShape9000_ScrollLMaxPressed_08 = 8,
		kShape9000_ScrollLOneNormal_09 = 9,
		kShape9000_ScrollLOnePressed_10 = 10,
		kShape9000_ScrollROneNormal_11 = 11,
		kShape9000_ScrollROnePressed_12 = 12,
		kShape9000_ScrollRMaxNormal_13 = 13,
		kShape9000_ScrollRMaxPressed_14 = 14,
		kShape9000_GoDisabled_15 = 15,
		// Help/Save button (slot 3): IDA uses shape 24 via the SCRB shape-table path
		// (gfx_blitBitmapShape, result >= 24 branch).  There are no tBMP 9023/9024 —
		// 9000-9015 are the only tBMP files.  The help button is rendered by the SCRB
		// runner automatically; renderButtons() skips slot 3 entirely.
		// These constants are kept for documentation only.
		kShape9000_HelpNormal_24 = 24,
		kShape9000_HelpPressed_25 = 25,
	};

	// -----------------------------------------------------------------------
	// Virtual feature IDs (arbitrary unique IDs used by loadVirtualFeature)
	// -----------------------------------------------------------------------
	enum VirtualFeatureId : uint16 {
		kVirtualFeature_Storage = 40000,
		kVirtualFeature_Buttons = 40001,
		kVirtualFeature_GoButton = 40002,
		kSnoidPackBase = 60000,
	};

	// -----------------------------------------------------------------------
	// Scroll button direction indices (used with _scrollDirection)
	// -----------------------------------------------------------------------
	enum ScrollDir {
		kScrollDir_None = 0,
		kScrollDir_LeftMax = 1,
		kScrollDir_LeftOne = 2,
		kScrollDir_RightOne = 3,
		kScrollDir_RightMax = 4,
	};

	// -----------------------------------------------------------------------
	// Static position data (decoded from IDA binary)
	// -----------------------------------------------------------------------

	/** Pedestal positions for the 16 active Zoombinis (SCRB 7000..7015). */
	const Common::Point _pedestalPoints[16] = {
		Common::Point(0x01EA, 0x0174), //  0
		Common::Point(0x01CA, 0x0167), //  1
		Common::Point(0x01C2, 0x0180), //  2
		Common::Point(0x019C, 0x0178), //  3
		Common::Point(0x0189, 0x018E), //  4
		Common::Point(0x016D, 0x0182), //  5
		Common::Point(0x015C, 0x0195), //  6
		Common::Point(0x0141, 0x0185), //  7
		Common::Point(0x0130, 0x019A), //  8
		Common::Point(0x0116, 0x018D), //  9
		Common::Point(0x0108, 0x01A1), // 10
		Common::Point(0x00EA, 0x0190), // 11
		Common::Point(0x00DA, 0x01A4), // 12
		Common::Point(0x00C5, 0x018E), // 13
		Common::Point(0x00B1, 0x01A2), // 14
		Common::Point(0x0098, 0x0193), // 15
	};

	/**
	 * Storage grid X positions for animation-mode display (6 columns visible).
	 * Accessed as word_4A05BE[2*col] in original code.
	 */
	const uint16 _storageMatrixX_anim[6] = {
		0x008F,
		0x00B7,
		0x00DF,
		0x0107,
		0x012F,
		0x0157,
	};

	/**
	 * Storage grid X positions for non-animation (still) display (5 columns visible).
	 * Accessed as word_4A05C0[2*col] in original code.
	 */
	const uint16 _storageMatrixX_nonanim[5] = {
		0x00A3,
		0x00CB,
		0x00F3,
		0x011B,
		0x0143,
	};

	/**
	 * Storage grid Y positions for animation-mode display (6 cols × 5 rows).
	 * Accessed as unk_4A05D4 + 20*col + 2*row in original code.
	 */
	const uint16 _storageMatrixY_anim[6][5] = {
		{0x0035, 0x0064, 0x0093, 0x00C2, 0x00F4}, // col 0
		{0x0037, 0x0065, 0x0094, 0x00C4, 0x00F3}, // col 1
		{0x0038, 0x0067, 0x0095, 0x00C4, 0x00F3}, // col 2
		{0x0036, 0x0064, 0x0093, 0x00C3, 0x00F2}, // col 3
		{0x002F, 0x0061, 0x0090, 0x00BF, 0x00ED}, // col 4
		{0x002B, 0x005A, 0x008A, 0x00B8, 0x00E7}, // col 5
	};

	/**
	 * Storage grid Y positions for non-animation (still) display (5 cols × 5 rows).
	 * Accessed as word_4A05DE + 20*col + 2*row in original code.
	 */
	const uint16 _storageMatrixY_nonanim[5][5] = {
		{0x0037, 0x0066, 0x0095, 0x00C4, 0x00F3}, // col 0
		{0x0037, 0x0066, 0x0096, 0x00C4, 0x00F3}, // col 1
		{0x0036, 0x0066, 0x0095, 0x00C4, 0x00F3}, // col 2
		{0x0034, 0x0063, 0x0092, 0x00C1, 0x00F0}, // col 3
		{0x002E, 0x005D, 0x008D, 0x00BB, 0x00EB}, // col 4
	};

	/**
	 * Clickable hotspot rectangles for the 10 animation button slots and the storage area.
	 * Accessed as word_4A0524[4*i] in sub_41392D.
	 * Index 0 = storage drag region. Indices 1..9 = animation button click areas.
	 */
	const Common::Rect _buttonHotspotRects[10] = {
		Common::Rect(52, 290, 136, 332),  // [0] storage drag area
		Common::Rect(469, 169, 521, 241), // [1] button 1 anim hotspot
		Common::Rect(499, 289, 566, 308), // [2] button 2 anim hotspot
		Common::Rect(455, 301, 504, 319), // [3] button 3 anim hotspot
		Common::Rect(568, 304, 604, 320), // [4] button 4 anim hotspot
		Common::Rect(570, 36, 624, 66),   // [5] button 5 anim hotspot
		Common::Rect(229, 304, 273, 317), // [6] button 6 anim hotspot
		Common::Rect(242, 324, 292, 336), // [7] button 7 anim hotspot
		Common::Rect(253, 348, 305, 361), // [8] button 8 anim hotspot
		Common::Rect(520, 259, 545, 300), // [9] button 9 anim hotspot (transport trigger)
	};

	/**
	 * Click region of the full storage panel virtual feature.
	 * BC2 columns are wider-spaced than BC1:
	 *   Non-anim (5 cols): X = 0xA3, 0xCB, 0xF3, 0x11B, 0x143
	 *   Anim     (6 cols): X = 0x8F, 0xB7, 0xDF, 0x107, 0x12F, 0x157
	 * Rightmost anim column centre at 0x157=343, plus ~30px snoid radius → right ≥ 373.
	 * Use 0x017F=383 to give a safe margin.  Left/top/bottom unchanged from the
	 * BC1 original (border at x=101, belt top at y=19, belt bottom at y=252).
	 */
	const Common::Rect _storageRect = Common::Rect(0x0037, 0x0013, 0x017F, 0x00FC);

	/** Go button click rect (derived from render position 0x0257,0x0140 + button size). */
	const Common::Rect _goButtonRect = Common::Rect(0x0257, 0x0140, 0x027E, 0x0165);

	/** Map button click rect (derived from render position 0x0257,0x0166 + button size). */
	const Common::Rect _mapButtonRect = Common::Rect(0x0257, 0x0166, 0x027E, 0x018B);

	/** Help/Save button click rect (derived from renderButtons slot 3). */
	const Common::Rect _helpButtonRect = Common::Rect(0x0257, 0x018C, 0x027E, 0x01B1);

	/**
	 * Scroll button click rects (decoded from dword_4A0384, 36-byte stride).
	 * [0] = Scroll Left Max (slot 4), [1] = Scroll Left One (slot 5),
	 * [2] = Scroll Right One (slot 6), [3] = Scroll Right Max (slot 7).
	 */
	const Common::Rect _scrollButtonRects[4] = {
		Common::Rect(0x0072, 0x0079, 0x0083, 0x00D0), // [0] Left Max
		Common::Rect(0x0083, 0x0075, 0x0090, 0x00CD), // [1] Left One
		Common::Rect(0x0151, 0x006B, 0x015E, 0x00C3), // [2] Right One
		Common::Rect(0x015E, 0x006E, 0x0169, 0x00C6), // [3] Right Max
	};

	// -----------------------------------------------------------------------
	// Runtime state (mirroring IDA globals)
	// -----------------------------------------------------------------------

	/** Runner index of the virtual storage feature (word_4AACD8). */
	uint16 _storageRunnerIdx = 0;

	/** Runner index of the transport animation SCRB (word_4AAD04). */
	uint16 _transportAnimRunnerIdx = 0;

	/**
	 * Runner indices for the 10 animation button SCRBs (word_4AACF2..word_4AAD04).
	 * [0..8] = button SCRBs 6005,6011,6010,6002,6004,6009,6006,6007,6008
	 * [9]    = transport anim (same as _transportAnimRunnerIdx).
	 */
	uint16 _buttonAnimRunnerIdxs[10] = {0};

	/** Scroll direction for the storage bar (word_4A0364 LOWORD); 0 = none. */
	int16 _scrollDirection = 0;

	/** True while the storage is mid-scroll animation (word_4A0364 HIWORD). */
	bool _scrollAnimating = false;

	/** Current column index of the left edge of the visible storage window (word_4AACC8). */
	int16 _storageLeftmostColumnIdx = 0;

	/** Total number of grid columns = _storageCapacity / 5 (word_4AACCA). */
	int16 _storageColumnCount = 0;

	/** Current storage grid capacity in slots, multiple of 5 in range [50..625] (word_4AACCC). */
	int16 _storageCapacity = 0;

	/** Index of the last occupied storage slot (word_4AACD0). */
	int16 _storageLastOccupiedIdx = 0;

	/** Number of Zoombinis currently in storage (word_4AACCE). */
	int16 _storedCount = 0;

	/** Whether we are in the scrolling-animation display mode (HIWORD of dword_4A0364). */
	bool _storageInAnimation = false;

	/** Frame counter threshold for the next storage scroll update (a2+36 in sub_4142BF). */
	uint32 _storageNextUpdateFrame = 0;

	/** Per-frame update interval for the storage scroll animation (a2+40 in sub_4142BF). */
	static constexpr uint32 kStorageScrollInterval = 2;

	/** Scroll sound state: 0 = silent, 1 = playing. */
	int16 _scrollSoundState = 0;

	/** Go-button enabled state (word_4AACE4). True when player may proceed. */
	bool _canGoEnabled = false;

	/** Go-button visible state, synced from _canGoEnabled each preRender (word_4AACE6). */
	bool _canGoVisible = false;

	/** True when the transport button animation has been armed (word_4AACF0). */
	bool _transportButtonArmed = false;

	/** Round-trip toggle for button 3 in sub_41392D (word_4AACEE). Starts at 1. */
	bool _roundTripToggle = true;

	/** True once player has re-entered BC2 (word_4AAD06). */
	bool _notFirstArrival = false;

	/** Monotonically increasing snoid ID counter for loadSnoidFromPack. */
	uint16 _nextPackSnoidId = 0;

	/** True if a Zoombini drag is currently in progress (word_4AACEC). */
	bool _dragInProgress = false;

	/** Secondary drag / session flag (word_4AACEA). */
	bool _dragActive = false;

	/** True when the currently dragged snoid was picked from storage (not from field). */
	bool _dragFromStorage = false;

	/** Storage slot index the dragged snoid was picked from (-1 if not from storage). */
	int16 _dragStorageOriginSlot = -1;

	/**
	 * Index of the pedestal currently highlighted during drag (-1 if none).
	 * IDA: wFeatureRunnerIdx in beginDragFeatureRunner_45360F
	 */
	int16 _hoveredPedestalIdx = -1;

	/**
	 * Hover radius for pedestal highlight during drag (same as zmb_clickZoneRadius = 15).
	 * IDA: beginDragFeatureRunner_45360F uses zmb_clickZoneRadius for drop zone detection.
	 */
	static const int16 kPedestalHoverRadius = 15;

	/** Index of the currently held scroll button (word_4AACD2; 0 = none). */
	int16 _currentScrollButton = 0;

	/** Session scroll-sound state helper (word_4AACDA). */
	int16 _sessionScrollCounter = 0;
};

} // End of namespace Mohawk

#endif
