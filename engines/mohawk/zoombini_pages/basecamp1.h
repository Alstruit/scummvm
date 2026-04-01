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

#ifndef MOHAWK_ZOOMBINI_PAGES_BASECAMP1_H
#define MOHAWK_ZOOMBINI_PAGES_BASECAMP1_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

class ZoombiniInteractiveBasecampOne : public ZoombiniInteractive {
public:
	ZoombiniInteractiveBasecampOne(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveBasecampOne() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onMouseMove(const Common::Point &absPos, const Common::Point &relPos) override;

	void onMapButtonActivated() override;

protected:
	/**
	 * Save all loaded snoids from _snoidMap back into f._zmbPackActive.
	 * Two-pass: occupied snoids first, then non-occupied.
	 * Mirrors IDA: save_updateZmbPacksOnPuzzleComplete(0, 1) for the BC1 case.
	 */
	void saveSnoidsToPack();

	/**
	 * Save active pack into BC1 pack before leaving the page.
	 * @param isDeparture  false = all snoids stay in BC1 (map button);
	 *                     true = occupied snoids depart, non-occupied stay (go button).
	 * Mirrors IDA: bc1_saveActivePackAndReadBC2 (0x4115CF).
	 */
	void saveBc1PackState(bool isDeparture);
	ZmbRenderResult storage_render(ZmbFeature *feature);
	void storage_postRender(ZmbFeature *feature);

	void scroll_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void scroll_postRender(ZmbFeature *feature);
	ZmbEventHandleResult scroll_lButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	ZmbEventHandleResult scroll_lButtonUp(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	ZmbEventHandleResult scroll_mouseMove(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);

	ZmbRenderResult virt03_render(ZmbFeature *feature);
	void virt03_postRender(ZmbFeature *feature);

	ZmbEventHandleResult genericEasterEgg_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos, const Common::Rect &clickRect);

	void easterEggStoneMan_postRender(ZmbFeature *feature);
	ZmbEventHandleResult easterEggStoneMan_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	void easterEggFish_postRender(ZmbFeature *feature);
	ZmbEventHandleResult easterEggFish_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	void easterEggBear_postRender(ZmbFeature *feature);
	ZmbEventHandleResult easterEggBear_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	void easterEggStoneFace_postRender(ZmbFeature *feature);
	ZmbEventHandleResult easterEggStoneFace_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	void easterEggHollowBugs_postRender(ZmbFeature *feature);
	ZmbEventHandleResult easterEggHollowBugs_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);

	ZmbEventHandleResult easterEggBonfire_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);

	int16 findStorageSlotIndex(bool searchOccupied, const Common::Rect &clickRect, uint16 leftmostColumnIdx);

	/**
	 * Find the index one past the last occupied slot (scanning 624..0 for traits != 0).
	 * Returns 0 if no occupied slot found.
	 * Mirrors IDA: occupancy scan loop in puzzleBasecamp1_doSomething_412C5A.
	 */
	int16 findLastOccupiedIdx() const;

	/**
	 * Recalculate _storageColumnCount, _storageCapacity, and clamp _storageLeftmostColumnIdx.
	 * Mirrors IDA: puzzleBasecamp1_calcStorageLeftmostColumn_412868.
	 */
	void calcStorageColumns();

	/**
	 * Insert arriving zoombinis from the active pack into the BC1 storage chunk.
	 * Returns 1 if all were placed consecutively, 0 if overflow occurred.
	 * Mirrors IDA: puzzleBasecamp1_doSomething_412C5A.
	 */
	int16 storeArrivingZoombinis();

	uint32 easterEggMushroom_selectRenderFrame(ZmbFeature *feature);
	ZmbEventHandleResult easterEggMushroom_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);

	void onGoButtonActivated() override;
	void onSecondGoButtonActivated() override;
	void executeDeparture() override;

	/**
	 * Find the nearest unoccupied pedestal slot to the given position.
	 * @param pos The position to check proximity against.
	 * @return Pedestal index (0–15), or -1 if none within snap radius.
	 */
	int16 findNearestEmptyPedestal(const Common::Point &pos) const;

	/**
	 * Updates pedestal highlight during snoid drag.
	 * Activates blink animation on nearest empty pedestal within hover radius;
	 * deactivates the previous highlight if hovering a different pedestal or none.
	 * IDA: beginDragFeatureRunner_45360F (~0x453A23–0x453B4B)
	 * @param snoidPos Current snoid position during drag.
	 */
	void updatePedestalHover(const Common::Point &snoidPos);

	/**
	 * End the current drag operation. Determines drop target and places the snoid:
	 * - Pedestal slot: snap to pedestal, set _packIsOccupied = true
	 * - Storage area: copy traits/name back to storage grid, unload snoid
	 * - Free space: place at non-colliding position, set _packIsOccupied = false
	 * Updates Go button enabled state.
	 */
	void endDrag(const Common::Point &dropPos);

	/**
	 * Load Zoombini snoids from the active pack data.
	 *
	 * Equivalent to handleZoombiniAnimation_maybe_4528A6(a2) in the original engine.
	 * Iterates _entries[0..wPackZmbCount-1] and for each qualifying entry:
	 *   - Occupied (bIsOccupied && !wUnkA930_zero): loaded, position from _pedestalPoints
	 *   - Non-occupied (!bIsOccupied && loadNonOccupied && !wUnkA932_zero): loaded, position from entry data
	 * After loading, pack._wPackZmbCount is set to 0.
	 *
	 * @param pack  The active pack to load from.
	 * @param loadNonOccupied  If true, non-occupied entries are also loaded (IDA: a2=1).
	 * @return  Number of loaded zoombini snoids.
	 */
	int16 loadZoombinisFromPack(ZmbStateActivePack &pack, bool loadNonOccupied);

	enum PageResourceId : uint32 {
		kResBackground1000 = 1000,
		kResBitmapShape1100 = 1100,
		kResBitmapShape1200_Pedestal = 1200,
		kResBitmapShape9000_Cursors = 9000,
		kResBitmapShape2000_Storage = 2000,
		kResBitmapShape2100_Buttons = 2100,

		kResNode1000 = 1000,
		kResRegs9000 = 9000,

		kResScrb1100_BottomShape1 = 1100,
		kResScrb1101_BottomShape2 = 1101,
		kResScrb1102_BottomShape3 = 1102,
		kResScrb1103_BottomShape4 = 1103,
		kResScrb1104_Bonfire = 1104,
		kResScrb1105_EasterEggPod = 1105,
		kResScrb1106_EasterEggStoneMan = 1106,
		kResScrb1107_EasterEggHollowBugs = 1107,
		kResScrb1108_EasterEggFish = 1108,
		kResScrb1109_EasterEggBear = 1109,
		kResScrb1110_EasterEggStoneFace = 1110,
		kResScrb1111_EasterEggMushroom1 = 1111,
		kResScrb1112_EasterEggMushroom2 = 1112,
		kResScrb1113_EasterEggMushroom3 = 1113,
		kResScrb1114_EasterEggMushroom4 = 1114,
		kResScrb1115_EasterEggMushroom5 = 1115,

		kResScrb1200_Pedestal = 1200,
		kResScrb1201_Pedestal = 1201,
		kResScrb1202_Pedestal = 1202,
		kResScrb1203_Pedestal = 1203,
		kResScrb1204_Pedestal = 1204,
		kResScrb1205_Pedestal = 1205,
		kResScrb1206_Pedestal = 1206,
		kResScrb1207_Pedestal = 1207,
		kResScrb1208_Pedestal = 1208,
		kResScrb1209_Pedestal = 1209,
		kResScrb1210_Pedestal = 1210,
		kResScrb1211_Pedestal = 1211,
		kResScrb1212_Pedestal = 1212,
		kResScrb1213_Pedestal = 1213,
		kResScrb1214_Pedestal = 1214,
		kResScrb1215_Pedestal = 1215,

		kResSound1118_EasterEggMushroom1 = 1118,
		kResSound1119_EasterEggMushroom2 = 1119,
		kResSound1120_EasterEggMushroom3 = 1120,
		kResSound1121_EasterEggMushroom4 = 1121,
		kResSound1122_EasterEggMushroom5 = 1122,

		kResSound2000_StorageScrolling = 2000,
		kResSound2001_StorageScrollEnd = 2001,

		kResSound20049_ArriveBC1VoiceFirst = 20049,
		kResSound20050_ArriveBC1Voice = 20050,
		kResSound20051_ArriveBC1Voice = 20051,
		kResSound20052_ArriveBC1Voice = 20052,
		kResSound20053_ArriveBC1Voice = 20053,
		kResSound20054_ArriveBC1Voice = 20054,

		kResSound996_DepartSFX = 996,
	};

	enum ShapeId : uint16 {
		kShapeStorage01_Honeycomb = 1,
		kShapeStorage02_Lattice = 2,
		kShapeStorage03_Honeycomb = 3,
		kShapeStorage04_Lattice = 4,
		kShapeStorage05_Border = 5,

		kShape2100_GoRouteUpButtonNormal_01 = 1,
		kShape2100_GoRouteUpButtonPressed_02 = 2,
		kShape2100_GoRouteDownButtonNormal_03 = 3,
		kShape2100_GoRouteDownButtonPressed_04 = 4,
		kShape2100_MapNormal_05 = 5,
		kShape2100_MapPressed_06 = 6,
		kShape2100_ScrollLeftFourNormal_07 = 7,
		kShape2100_ScrollLeftFourPressed_08 = 8,
		kShape2100_ScrollLeftOneNormal_09 = 9,
		kShape2100_ScrollLeftOnePressed_10 = 10,
		kShape2100_ScrollRightOneNormal_11 = 11,
		kShape2100_ScrollRightOnePressed_12 = 12,
		kShape2100_ScrollRightFourNormal_13 = 13,
		kShape2100_ScrollRightFourPressed_14 = 14,
		kShape2100_GoRouteUpButtonDisabled_15 = 15,
		kShape2100_GoRouteDownButtonDisabled_16 = 16,

		kShape9000_ArrowLeftMax_01 = 1,
		kShape9000_ArrowLeft_02 = 2,
		kShape9000_ArrowRight_03 = 3,
		kShape9000_ArrowRightMax_04 = 4,
	};

	enum VirtualFeatureId : uint16 {
		kVirtualFeature2000_Storage = 40000,
		kVirtualFeatureBasecamp1_ScrollButtons = 40001,
		kVirtualFeatureBasecamp1_03 = 40002,
		kSnoidPackBase = 60000,
	};

	enum ScrollButtonIdx {
		kScrollButtons_LeftFour = 0,
		kScrollButtons_LeftOne,
		kScrollButtons_RightOne,
		kScrollButtons_RightFour,
	};

	const Common::Point _pedestalPoints[16] = {
		Common::Point(0x194, 0x14E),
		Common::Point(0x185, 0x162),
		Common::Point(0x16A, 0x153),
		Common::Point(0x15E, 0x16B),
		Common::Point(0x143, 0x15C),
		Common::Point(0x130, 0x16F),
		Common::Point(0x118, 0x15E),
		Common::Point(0x106, 0x170),
		Common::Point(0x0EE, 0x15A),
		Common::Point(0x0DD, 0x16E),
		Common::Point(0x0C1, 0x160),
		Common::Point(0x0B0, 0x173),
		Common::Point(0x098, 0x15F),
		Common::Point(0x085, 0x171),
		Common::Point(0x06E, 0x155),
		Common::Point(0x05E, 0x169),
	};

	const Common::Rect _goRouteUpButtonRect = Common::Rect(0x0257, 0x0136, 0x027E, 0x015B);
	const Common::Rect _goRouteDownButtonRect = Common::Rect(0x0257, 0x0182, 0x027E, 0x01A7);
	const Common::Rect _mapButtonRect = Common::Rect(0x0257, 0x015C, 0x027E, 0x0181);
	const Common::Rect _helpButtonRect = Common::Rect(0x0257, 0x01A8, 0x027E, 0x01CD);
	const Common::Rect _scrollLeftFourButtonRect = Common::Rect(0x0013, 0x004E, 0x0023, 0x00CD);
	const Common::Rect _scrollLeftOneButtonRect = Common::Rect(0x0023, 0x004E, 0x0034, 0x00CD);
	const Common::Rect _scrollRightOneButtonRect = Common::Rect(0x0101, 0x004E, 0x0111, 0x00CD);
	const Common::Rect _scrollRightFourButtonRect = Common::Rect(0x0111, 0x004E, 0x0122, 0x00CD);
	const Common::Rect _easterEggStoneManRect = Common::Rect(0x020F, 0x005A, 0x0232, 0x0071);
	const Common::Rect _easterEggFishRect = Common::Rect(0x0226, 0x0077, 0x024C, 0x0080);
	const Common::Rect _easterEggBearRect = Common::Rect(0x024D, 0x0070, 0x027F, 0x00A2);
	const Common::Rect _easterEggStoneFaceRect = Common::Rect(0x0144, 0x011F, 0x0187, 0x0143);
	const Common::Rect _easterEggHollowBugsRect = Common::Rect(0x0019, 0x01B0, 0x007A, 0x01DD);

	Common::StableMap<uint32, ContinuousButtonState> _scrollButtonStateMap;
	Common::HashMap<uint32, Common::Rect> _scrollButtonRectMap;
	Common::HashMap<uint32, Common::Rect> _easterEggRectMap;

	const Common::Rect _storageRect = Common::Rect(0x0037, 0x0013, 0x00FF, 0x00FC);

	const Common::Array<uint16> _storageMatrixX1 = {
		0x37,
		0x5F,
		0x87,
		0xAF,
		0xD7,
		0xFF,
	};
	const Common::Array<uint16> _storageMatrixX2 = {
		0x4B,
		0x73,
		0x9B,
		0xC3,
		0xEB,
		0xFF,
	};
	const Common::Array<Common::Array<uint16>> _storageMatrixY1 = {
		{0x25, 0x54, 0x83, 0xB2, 0xE1},
		{0x27, 0x56, 0x85, 0xB4, 0xE3},
		{0x28, 0x57, 0x86, 0xB5, 0xE4},
		{0x26, 0x55, 0x84, 0xB3, 0xE2},
		{0x22, 0x51, 0x80, 0xAF, 0xDE},
		{0x1B, 0x4A, 0x79, 0xA8, 0xD7},
	};
	const Common::Array<Common::Array<uint16>> _storageMatrixY2 = {
		{0x27, 0x56, 0x85, 0xB4, 0xE3},
		{0x28, 0x57, 0x86, 0xB5, 0xE4},
		{0x27, 0x56, 0x85, 0xB4, 0xE3},
		{0x24, 0x53, 0x82, 0xB1, 0xE0},
		{0x1F, 0x4E, 0x7D, 0xAC, 0xDB},
		{0x1B, 0x4A, 0x79, 0xA8, 0xD7},
	};

	bool _storageMatrixInAnimation = false;
	uint16 _storageButtonCursorShapeIdx = ZmbHotspot::kShapeNone;
	uint16 _nextPackSnoidId = 0;

	/**
	 * Index of the storage leftmost column currently being rendered.
	 * Range: 0 ~ 120. Mirrors IDA: wBC1StorageLeftmostColumnIdx_4AAB90.
	 */
	int16 _storageLeftmostColumnIdx = 0;

	/**
	 * Total column count = _storageCapacity / 5. Minimum 10.
	 * Mirrors IDA: wZmbIdx_4AAB92.
	 */
	int16 _storageColumnCount = 10;

	/**
	 * Total slot capacity = _storageColumnCount * 5. Range: 50 ~ 625.
	 * Mirrors IDA: wZmbIdx_4AAB94.
	 */
	int16 _storageCapacity = 50;

	/**
	 * One past the index of the highest occupied slot (= findLastOccupiedIdx()).
	 * Mirrors IDA: wBC1StorageLeftmostCellIdx_4AAB98.
	 */
	int16 _storageMaxCellIdx = 0;

	/** True if all 625 Zoombinis generated and fewer than 16 cleared BC1+Isle. */
	bool _notFirstArrival = false;
	/** True when the Go buttons should be active. */
	bool _canGoEnabled = false;

	/** 1 = route up (north), 2 = route down (south). Set when departure starts. */
	int16 _departRouteDirection = 0;

	// --- Drag state (IDA: beginDragFeatureRunner_45360F, bc1_onHotspotClick) ---

	/** True if the dragged snoid was picked from the storage grid (not from a pedestal/field). */
	bool _dragFromStorage = false;

	/** Original storage grid index the snoid was picked from (-1 if not from storage). */
	int16 _dragStorageOriginSlot = -1;

	/**
	 * Index of the pedestal currently highlighted during drag (-1 if none).
	 * IDA: wFeatureRunnerIdx in beginDragFeatureRunner_45360F
	 */
	int16 _hoveredPedestalIdx = -1;

	/**
	 * Snap radius (squared) for pedestal drop target detection.
	 * IDA: wClickZoneRadius_4B6D3E is 15 for BC1 (default from puzzleDispatch_sharedCleanup).
	 * We use squared distance for efficiency: 20^2 = 400.
	 */
	static const int32 kPedestalSnapRadiusSq = 400;

	/**
	 * Hover radius for pedestal highlight during drag (same as zmb_clickZoneRadius = 15).
	 * IDA: beginDragFeatureRunner_45360F uses zmb_clickZoneRadius for drop zone detection.
	 */
	static const int16 kPedestalHoverRadius = 15;
};

} // End of namespace Mohawk

#endif
