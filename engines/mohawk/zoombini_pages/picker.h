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

#ifndef MOHAWK_ZOOMBINI_PAGES_PICKER_H
#define MOHAWK_ZOOMBINI_PAGES_PICKER_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

class ZoombiniInteractivePicker : public ZoombiniInteractive {
public:
	enum PickerMode {
		/**
		 * First new game: Do not open the LoadDialog.
		 * Has saved game: Open the LoadDialog.
		 */
		kPickerMode_LoadGame,
		/**
		 * Select Zoombinis.
		 */
		kPickerMode_SelectZoombinis,
	};

	ZoombiniInteractivePicker(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractivePicker() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	PickerMode _mode;

	void pickerMatrix_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void pickerMatrix_onPostRender(ZmbFeature *feature);
	void pickerMatrix_onButtonAction(ZmbFeature *feature, uint32 bsIdx, StickyButtonState &bs);
	ZmbEventHandleResult pickerMatrix_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);

	void pickerButtons_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	void pickerButtons_onPostRender(ZmbFeature *feature);
	void pickerButtons_onButtonAction(ZmbFeature *feature, uint32 bsIdx, ButtonState &bs);
	ZmbEventHandleResult pickerButtons_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	ZmbEventHandleResult pickerButtons_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);
	ZmbEventHandleResult pickerButtons_onKeyUp(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);

	void zoombiniPreview_onPreRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);

	ZmbRenderResult oneTimeLoadDialog_onRenderShape(ZmbFeature *feature);

	ZmbEventHandleResult onMouseMove(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) override;
	void endDrag(const Common::Point &dropPos);
	void updateCaveMarkHighlight();

	void randomizeTraitSelection();
	bool isZoombiniTraitGeneratable(ZmbTrait trait) const;
	void generateZoombiniName();
	void updatePendingGoTransition();
	void repackSeatPositions();

	void onGoButtonActivated() override;
	void onMapButtonActivated() override;

	/**
	 * Load Zoombini snoids from the active pack data.
	 *
	 * Equivalent to handleZoombiniAnimation_maybe_4528A6(0) in the original Picker.
	 * Only loads occupied entries (bIsOccupied && !wUnkA930_zero).
	 * Positions are assigned from _zoombiniSeatPoints (closest-match).
	 *
	 * @param pack  The active pack to load from.
	 * @return  Number of loaded zoombini snoids.
	 */
	int16 loadZoombinisFromPack(ZmbStateActivePack &pack);

	// Constant
	enum PageResourceId : uint16 {
		kResBackground4000 = 4000,
		
		kResBitmapShapes4100_BackObjects = 4100,
		kResBitmapShapes4200_Buttons = 4200,
		kResBitmapShapes4300_ZoombiniPreview = 4300,
		kResBitmapShapes4400_PickerMatrix = 4400,
		
		kResScrb4100_BackObjects = 4100,
		kResScrb4101_Star = 4101,
		kResScrb4102_Star = 4102,
		kResScrb4103_Star = 4103,
		kResScrb4104_Waves = 4104,
		kResScrb4105_Boat = 4105,
		kResScrb4106_RockShape = 4106,
		kResScrb4107_RockShape = 4107,
		kResScrb4108_RockShape = 4108,
		kResScrb4109_RockShape = 4109,
		kResScrb4110_CaveMark = 4110,

		kResMidi30001_Isle = 30001,
		kResSound30001_Isle = 30001,
		kResSound20042_PickerAfterVideoVoice = 20042,
		kResSound20043_PickerAfterVideoVoice = 20043,
		kResSound20044_PickerAfterVideoVoice = 20044,
		kResSound1000_PressMatrixButton = 1000,
		kResSound1004_ReleaseMatrixButton = 1004,
		kResSound1005_PressGenerateButton = 1005,
		kResSound1006_PressDiceButton = 1006,
		kResSound1007_RemoveZoombini = 1007,
		kResSound1008_AllZoombinisGenerated = 1008,
	};

	enum VirtualFeatureId {
		kVirtualFeatureGoMapButtons = 0,
		kVirtualFeatureHelpButton,
		kVirtualFeaturePickerMatrix,
		kVirtualFeaturePickerButtons,
		kVirtualFeatureZoombiniPreview,
		kVirtualFeatureLoadDialog = 65000,
		kSnoidPackBase = 60000,
	};

	enum Shape4200Id {
		kShape4200_01_GenerateButtonDisabled = 1,
		kShape4200_02_GenerateButtonNormal = 2,
		kShape4200_03_GenerateButtonPressed = 3,
		kShape4200_04_DiceButtonNormal = 4,
		kShape4200_05_DiceButtonPressed = 5,
		kShape4200_06_DiceArrowButtonNormal = 6,
		kShape4200_07_DiceArrowButtonPressed = 7,
		kShape4200_08_GoButtonDisabled = 8,
		kShape4200_09_GoButtonNormal = 9,
		kShape4200_10_GoButtonPressed = 10,
		kShape4200_11_MapButtonNormal = 11,
		kShape4200_12_MapButtonPressed = 12,
		kShape4200_13_NameBox = 13,
	};

	enum Shape4300Id {
		kShape4300_01_PreviewBody = 1,
		kShape4300_02_PreviewHair1,
		kShape4300_03_PreviewHair2,
		kShape4300_04_PreviewHair3,
		kShape4300_05_PreviewHair4,
		kShape4300_06_PreviewHair5,
		kShape4300_07_PreviewEye1,
		kShape4300_08_PreviewEye2,
		kShape4300_09_PreviewEye3,
		kShape4300_10_PreviewEye4,
		kShape4300_11_PreviewEye5,
		kShape4300_12_PreviewNose1,
		kShape4300_13_PreviewNose2,
		kShape4300_14_PreviewNose3,
		kShape4300_15_PreviewNose4,
		kShape4300_16_PreviewNose5,
		kShape4300_17_PreviewFoot1,
		kShape4300_18_PreviewFoot2,
		kShape4300_19_PreviewFoot3,
		kShape4300_20_PreviewFoot4,
		kShape4300_21_PreviewFoot5,
	};
		
	// MapRect & MapSave data
	const Common::Rect _pickerMatrixRects[20] = {
		Common::Rect(0x0003, 0x0130, 0x002A, 0x015A),
		Common::Rect(0x002A, 0x0130, 0x0051, 0x015A),
		Common::Rect(0x0051, 0x0130, 0x0078, 0x015A),
		Common::Rect(0x0078, 0x0130, 0x009F, 0x015A),
		Common::Rect(0x009F, 0x0130, 0x00C6, 0x015A),
		Common::Rect(0x0003, 0x015C, 0x002A, 0x0186),
		Common::Rect(0x002A, 0x015C, 0x0051, 0x0186),
		Common::Rect(0x0051, 0x015C, 0x0078, 0x0186),
		Common::Rect(0x0078, 0x015C, 0x009F, 0x0186),
		Common::Rect(0x009F, 0x015C, 0x00C6, 0x0186),
		Common::Rect(0x0003, 0x0188, 0x002A, 0x01B2),
		Common::Rect(0x002A, 0x0188, 0x0051, 0x01B2),
		Common::Rect(0x0051, 0x0188, 0x0078, 0x01B2),
		Common::Rect(0x0078, 0x0188, 0x009F, 0x01B2),
		Common::Rect(0x009F, 0x0188, 0x00C6, 0x01B2),
		Common::Rect(0x0003, 0x01B4, 0x002A, 0x01DE),
		Common::Rect(0x002A, 0x01B4, 0x0051, 0x01DE),
		Common::Rect(0x0051, 0x01B4, 0x0078, 0x01DE),
		Common::Rect(0x0078, 0x01B4, 0x009F, 0x01DE),
		Common::Rect(0x009F, 0x01B4, 0x00C6, 0x01DE),
	};
	Common::StableMap<uint32, StickyButtonState> _matrixButtonStateMap;
	Common::HashMap<uint32, Common::Rect> _matrixButtonRectMap;
	static constexpr uint32 kMatrixColumns = 5u;

	const Common::Point _previewTraitOffsets[21] = {
		Common::Point(0x16, 0x17), // Body
		Common::Point(0x19, 0x1E), // Hair1
		Common::Point(0x1C, 0x1D), // Hair2
		Common::Point(0x13, 0x1B), // Hair3
		Common::Point(0x0A, 0x1F), // Hair4
		Common::Point(0x1D, 0x1E), // Hair5
		Common::Point(0x0F, 0x0B), // Eye1
		Common::Point(0x07, 0x0B), // Eye2
		Common::Point(0x11, 0x0B), // Eye3
		Common::Point(0x18, 0x09), // Eye4
		Common::Point(0x18, 0x06), // Eye5
		Common::Point(0x07, static_cast<int8>(0xFE)), // Nose1
		Common::Point(0x07, static_cast<int8>(0xFE)), // Nose2
		Common::Point(0x07, static_cast<int8>(0xFE)), // Nose3
		Common::Point(0x07, static_cast<int8>(0xFE)), // Nose4
		Common::Point(0x07, static_cast<int8>(0xFE)), // Nose5
		Common::Point(0x17, static_cast<int8>(0xEA)), // Foot1
		Common::Point(0x18, static_cast<int8>(0xEA)), // Foot2
		Common::Point(0x0D, static_cast<int8>(0xEC)), // Foot3
		Common::Point(0x0F, static_cast<int8>(0xEB)), // Foot4
		Common::Point(0x17, static_cast<int8>(0xE9)), // Foot5
	};

	enum PickerButtonHotspotIndex {
		kHotspotGenerateButtonNormal = 0,
		kHotspotDiceButtonNormal = 1,
		kHotspotGenerateButtonPressed = 2,
		kHotspotDiceButtonPressed = 3,
		kHotspotNameBox = 4,
	};

	enum PickerButtonsIndex {
		kPickerButtons_Generate = 0,
		kPickerButtons_Dice,
	};

	const Common::Rect _generateButtonRect = Common::Rect(0x00CD, 0x0130, 0x0104, 0x0156);
	const Common::Rect _previewZoombiniRect = Common::Rect(0x00CD, 0x015B, 0x0111, 0x01A4);
	const Common::Rect _nameBoxRect = Common::Rect(0x00C9, 0x01A3, 0x011F, 0x01B5);
	const Common::Rect _diceButtonRect = Common::Rect(0x00CD, 0x01B8, 0x0104, 0x01DE);

	const Common::Rect _goButtonRect = Common::Rect(0x0258, 0x01B9, 0x027F, 0x01DE);
	const Common::Rect _mapButtonRect = Common::Rect(0x0258, 0x0193, 0x027F, 0x01B8);
	const Common::Rect _helpButtonRect = Common::Rect(0x0258, 0x016D, 0x027F, 0x0192);

	/**
	 * Embark priority order: seat indices checked when selecting which snoid to animate.
	 * IDA: dword_4A2D9A/4A2D9E = [11, 12, 6, 7]
	 * These seats are in the upper-right corner, near the ladder.
	 *
	 * When 16 snoids are on the boat (0 remaining on picker), the Go button
	 * animates one snoid climbing the ladder. The engine checks seats in this
	 * priority order: if seat 11 is empty, check 12; if 12 is empty, check 6; etc.
	 */
	static const int16 kEmbarkOrder[4];

	/**
	 * Embark destination point on the boat.
	 * IDA: pZmb->animDestPos = (544, 264)
	 */
	const Common::Point _embarkDestination = Common::Point(544, 264);

	/**
	 * Frame stagger between each embarking snoid.
	 * IDA: 60 frames between each snoid's start
	 */
	static constexpr uint32 kEmbarkStagger = 60;

	/**
	 * Drop zone rect for the cave entrance (SCRB 4110 CaveMark area).
	 * Dropping a dragged snoid inside this rect removes it from the board.
	 * IDA: DRAW_ON_REG overlap between snoid and CaveMark position.
	 */
	const Common::Rect _caveDropRect = Common::Rect(100, 50, 320, 280);

	/**
	 * Cave mark registration point from pCaveMarkFeatureCore_4A2D90.
	 * IDA data: 0xAC=172, 0xE2=226.
	 */
	const Common::Point _caveMarkRegPoint = Common::Point(172, 226);

	/** IDA: zmb_clickZoneRadius = 60 (set in picker_initRunner / 0x439679). */
	static constexpr int16 kPickerClickZoneRadius = 60;

	enum ZoombiniPreviewIndex {
		kHotspotPreviewBody = 0,
		kHotspotPreviewHair,
		kHotspotPreviewEye,
		kHotspotPreviewNose,
		kHotspotPreviewFoot,
	};

	const Common::Point _zoombiniSeatPoints[16] = {
		{ Common::Point(0x021E, 0x01BE) },
		{ Common::Point(0x01F9, 0x01BF) },
		{ Common::Point(0x01D2, 0x01C3) },
		{ Common::Point(0x01A9, 0x01C0) },
		{ Common::Point(0x017C, 0x01C2) },
		{ Common::Point(0x0156, 0x01C3) },
		{ Common::Point(0x020A, 0x0192) },
		{ Common::Point(0x01E8, 0x0198) },
		{ Common::Point(0x01BC, 0x01A0) },
		{ Common::Point(0x0193, 0x019D) },
		{ Common::Point(0x016C, 0x019D) },
		{ Common::Point(0x01F2, 0x0168) },
		{ Common::Point(0x01CF, 0x016F) },
		{ Common::Point(0x01AA, 0x0172) },
		{ Common::Point(0x0185, 0x0175) },
		{ Common::Point(0x0160, 0x0176) },
	};

	Common::StableMap<uint32, ButtonState> _pickerButtonStateMap;
	Common::HashMap<uint32, Common::Rect> _pickerButtonRectMap;

	ZmbFeature *_caveMarkFeature = nullptr;
	bool _caveMarkHighlighted = false;

	ZmbSnoid _previewSnoid;
	bool _randomizeAll = false;
	/** 
	 * True on the first visit to the picker this game session.
	 */
	bool _isFirstVisit = false;
	uint16 _nextPackSnoidId = 0;
	bool _pendingGoTransition = false;
	bool _pendingGoTransitionHasSoundHandle = false;
	Audio::SoundHandle _pendingGoTransitionSoundHandle;

	/**
	 * Snoids currently walking during embark animation.
	 * Cleared when all snoids finish their walk animation.
	 */
	Common::Array<ZmbSnoid *> _embarkingSnoids;

	/**
	 * Seat-to-snoid mapping. Index is seat position (0-15), value is pointer to snoid.
	 * Updated when snoids are generated or removed.
	 */
	ZmbSnoid *_seatToSnoid[16] = {nullptr};

	/**
	 * Find a snoid at or near the given seat position.
	 * Used for embark animation to locate snoids by their seat.
	 */
	ZmbSnoid *findSnoidAtSeat(int16 seatIdx);

	/**
	 * Check if all embarking snoids have finished their walk animation.
	 */
	bool areAllEmbarkersDone() const;

};

} // End of namespace Mohawk

#endif
