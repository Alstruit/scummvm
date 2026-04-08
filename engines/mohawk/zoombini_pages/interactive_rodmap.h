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

#ifndef MOHAWK_ZOOMBINI_PAGES_INTERACTIVE_RODMAP_H
#define MOHAWK_ZOOMBINI_PAGES_INTERACTIVE_RODMAP_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

class ZoombiniInteractiveRodMap : public ZoombiniInteractive {
public:
	ZoombiniInteractiveRodMap(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveRodMap() override;

	void open() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

	ZmbEventHandleResult onMouseMove(const Common::Point &absPos, const Common::Point &relPos) override;
	ZmbEventHandleResult onKeyDown(const Common::KeyState &kbd, bool kbdRepeat) override;

protected:
	void setPracticeMode(bool setPracticeMode);
	void togglePracticeMode();

	/**
	 * Fill the active pack with 16 random-trait Zoombinis for practice mode.
	 * IDA: puzzleRodMap_maybeOnClickPuzzleIcon_42A9D6 — the section guarded by
	 * wPracticeLevel_0to4_4B6D2A != 0.
	 *
	 * Generates wPackZmbCount entries (always 16) with random trait values 1-5
	 * for each of the four attributes (head, eye, nose, foot), sets bIsOccupied=1,
	 * and clears the name field. Called before navigating to any puzzle page in
	 * practice mode so that the destination puzzle always receives a full pack.
	 */
	void generatePracticePack();

	/**
	 * 1000: Patch page shapeid on hovered
	 */
	void patchPageShape1000_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	/**
	 * 1000: Run page
	 */
	ZmbEventHandleResult runPage1000_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	/**
	 * 1001: Patch route shapeid to match the route's level
	 */
	void patchRouteShape1001_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	/**
	 * 1005: Page text box appears only after page icon has been hovered
	 */
	bool drawAfterPageIconHover1005_preRender(ZmbFeature *feature);
	/**
	 * 1005: Gate shape blitting on hover state.
	 * IDA: The original's postRender callback (0x42BEF4) combined shape blitting
	 * and text drawing, both gated by bHasClickRect. This custom renderFunc
	 * replicates that gate for the ScummVM split architecture.
	 */
	ZmbRenderResult renderAfterPageIconHover1005(ZmbFeature *feature);
	/**
	 * 1005: Select text of the page
	 */
	void textPageName1005_postRender(ZmbFeature *feature);
	/**
	 * 1006: Select shape of menu button when clicked
	 */
	void optionButton1006_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	/**
	 * 1006: PostAnimation of option button
	 */
	void optionButton1006_postRender(ZmbFeature *feature);
	/**
	 * 1006: Clicked on menu button
	 */
	ZmbEventHandleResult optionButton1006_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	/**
	 * 1004: (Practice mode only) Change shape of current selected level
	 */
	void patchSelectedLevelShape1004_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	/**
	 * 1004: Draw text of the level legend
	 */
	void textLegend1004_postRender(ZmbFeature *feature);
	/**
	 * 1004: (Practice mode only) Clicked on level
	 */
	ZmbEventHandleResult legendLevel1004_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	/**
	 * 1002: Draw text of the journey stat
	 */
	void textJourneyStat1002_postRender(ZmbFeature *feature);
	/**
	 * 1003: Combobox which selects Practice vs Journey
	 */
	void drawComboBox1003_preRenderShape(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	/**
	 * 1003: Clicked on Practice/Journey Combobox
	 */
	ZmbEventHandleResult selectMode1003_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	/**
	 * Virtual Feature: Draw route names on the map
	 */
	void textRouteNames_postRender(ZmbFeature *feature);

	// Constant
	enum PageResourceId : uint16 {
		kResBackground300 = 300,
		kResBitmapShape1000 = 1000,
		kResScrbPageIcon1000 = 1000,
		kResScrbRoute1001 = 1001,
		kResScrbStatusRect1002 = 1002,
		kResScrbModeCombobox1003 = 1003,
		kResScrbLevelLegend1004 = 1004,
		kResScrbPageNameHover1005 = 1005,
		kResScrbMenuButton1006 = 1006,
	};

	enum ShapeId : uint16 {
		kResShapeBridge01 = 1,
		kResShapeTunnels02 = 2,
		kResShapePizza03 = 3,
		kResShapeBcOne04 = 4,
		kResShapeFerry05 = 5,
		kResShapeLilly06 = 6,
		kResShapeSlides07 = 7,
		kResShapeFleens08 = 8,
		kResShapeHotel09 = 9,
		kResShapeNet10 = 10,
		kResShapeBcTwo11 = 11,
		kResShapeCaves12 = 12,
		kResShapeSmoke13 = 13,
		kResShapeMaze14 = 14,
		kResShapeTown15 = 15,
		kResShapePicker16 = 16,

		kResShapeRouteBigBadHungryP0_17 = 17,
		kResShapeRouteBigBadHungryP1_18 = 18,
		kResShapeRouteBigBadHungryP2_19 = 19,
		kResShapeRouteBigBadHungryP3_20 = 20,
		kResShapeRouteWhosBayouP0_21 = 21,
		kResShapeRouteWhosBayouP1_22 = 22,
		kResShapeRouteWhosBayouP2_23 = 23,
		kResShapeRouteWhosBayouP3_24 = 24,
		kResShapeRouteDeepDarkForestP0_25 = 25,
		kResShapeRouteDeepDarkForestP1_26 = 26,	
		kResShapeRouteDeepDarkForestP2_27 = 27,
		kResShapeRouteDeepDarkForestP3_28 = 28,
		kResShapeRouteMontDespairP0_29 = 29,
		kResShapeRouteMontDespairP1_30 = 30,
		kResShapeRouteMontDespairP2_31 = 31,
		kResShapeRouteMontDespairP3_32 = 32,

		kShapeOptionButtonPressed = 92,
		kShapeOptionButtonNormal = 93,

		kShapeComboBoxRedCircle111 = 111,
		kShapeComboBoxOutline112 = 112,
	};

	ZoombiniPageType _lastHoveredPageType = ZoombiniPageType::kNone;
	
	// MapRect & MapSave data
	const Common::Rect _routeNameRects[4] = {
		{ Common::Rect(15, 247, 140, 275) },
		{ Common::Rect(214, 47, 384, 62) },
		{ Common::Rect(327, 414, 439, 444) },
		{ Common::Rect(507, 193, 587, 223) },
	};
	const ZoombiniText::Key _routeNameTextKey[4] = {
		ZoombiniText::kRoute1,
		ZoombiniText::kRoute2,
		ZoombiniText::kRoute3,
		ZoombiniText::kRoute4,
	};
	
	// Common::Rect _modeSelectClickRect = Common::Rect(23, 23, 197, 127);
	// Common::Rect _levelSelectClickRect = Common::Rect(459, 373, 617, 457);

	const uint16 _levelLegendPalettes[4] = {
		0x00EC, 0x00EA, 0x00E8, 0x00EE,
	};
	// Clickable Pos & Rect
	const Common::Rect _levelLegendClickRects[4] = {
		{ Common::Rect(0x01D0, 0x018B, 0x0264, 0x0199) },
		{ Common::Rect(0x01D0, 0x0199, 0x0264, 0x01A8) },
		{ Common::Rect(0x01D0, 0x01A8, 0x0264, 0x01B6) },
		{ Common::Rect(0x01D0, 0x01B6, 0x0264, 0x01C6) },
	};
	const Common::Point _pageClickPoints[16] = {
		{ Common::Point(0x003C, 0x0190) },
		{ Common::Point(0x00AA, 0x0157) },
		{ Common::Point(0x00A0, 0x0105) },
		{ Common::Point(0x00DE, 0x0139) },
		{ Common::Point(0x00EC, 0x010D) },
		{ Common::Point(0x00EA, 0x0092) },
		{ Common::Point(0x011E, 0x0052) },
		{ Common::Point(0x016A, 0x00B9) },
		{ Common::Point(0x0141, 0x0159) },
		{ Common::Point(0x01AB, 0x0160) },
		{ Common::Point(0x01E4, 0x0145) },
		{ Common::Point(0x01BA, 0x00D3) },
		{ Common::Point(0x01EA, 0x00B6) },
		{ Common::Point(0x01E7, 0x005F) },
		{ Common::Point(0x021C, 0x007D) },
		{ Common::Point(0x0242, 0x0041) },
	};
	Common::Rect _pageClickRects[16]; // Populated from _pageClickPos;
	const ZoombiniPageType _pageClickTypes[16] = {
		ZoombiniPageType::kPicker,
		ZoombiniPageType::kBridge,
		ZoombiniPageType::kTunnels,
		ZoombiniPageType::kPizza,
		ZoombiniPageType::kBasecamp1,
		ZoombiniPageType::kFerry,
		ZoombiniPageType::kLilly,
		ZoombiniPageType::kSlides,
		ZoombiniPageType::kFleens,
		ZoombiniPageType::kHotel,
		ZoombiniPageType::kNet,
		ZoombiniPageType::kBasecamp2,
		ZoombiniPageType::kCaves,
		ZoombiniPageType::kSmoke,
		ZoombiniPageType::kMaze,
		ZoombiniPageType::kTown,
	};
	const uint16 _pageClickShapes[16] = {
		kResShapePicker16,
		kResShapeBridge01,
		kResShapeTunnels02,
		kResShapePizza03,
		kResShapeBcOne04,
		kResShapeFerry05,
		kResShapeLilly06,
		kResShapeSlides07,
		kResShapeFleens08,
		kResShapeHotel09,
		kResShapeNet10,
		kResShapeBcTwo11,
		kResShapeCaves12,
		kResShapeSmoke13,
		kResShapeMaze14,
		kResShapeTown15,
	};

	const Common::Rect _modeSelectClickRects[2] = {
		{ Common::Rect(23, 131, 197, 152) },
		{ Common::Rect(23, 153, 197, 175) },
	};

	enum RodmapButtonIdx {
		kRodmapButton_OptionDialog = 0,
	};
	
	// Option Button SCRB does not have two hotspots, so normal hsId and pressed hsId are equal.
	ZmbResource soundResId = ZmbResource(ZmbArchiveKind::kSystem, kResSound0999_ButtonSFX);
	ButtonState _optionButtonState = ButtonState(soundResId, 0, 0, kShapeOptionButtonNormal, kShapeOptionButtonPressed);

	/**
	 * Map of route keys (17 ~ 32) to its level. 
	 * Value of 0 means the route is not yet visited.
	 * Value of 1 ~ 4 means the route level is 1 to 4.
	 */
	Common::StableMap<uint16, int16> _pageRouteLevelMap;
	void buildPageRouteLevelMap();
};

} // End of namespace Mohawk

#endif
