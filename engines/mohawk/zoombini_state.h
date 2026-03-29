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

#ifndef ZOOMBINI_STATE_H
#define ZOOMBINI_STATE_H

#include "common/rect.h"
#include "common/savefile.h"
#include "common/file.h"
#include "common/str.h"
#include "engines/savestate.h"

namespace Common {

class Serializer;

}

namespace Mohawk {

class MohawkEngine_Zoombini;
class ZmbFeature;

enum ZMB_DIFFICULTY_ID : uint16 {
	ZMB_DIFFICULTY_NOTVISITED_00 = 0,
	ZMB_DIFFICULTY_LEVEL1_01 = 1,
	ZMB_DIFFICULTY_LEVEL2_02 = 2,
	ZMB_DIFFICULTY_LEVEL3_05 = 5,
	ZMB_DIFFICULTY_LEVEL4_12 = 12,
};

enum ZMB_PAGE_FLAG : uint16 {
	ZMB_PAGE_MASK_0FFF = 0xFFF,
	ZMB_PAGE_FLAG_1000 = 0x1000,
	ZMB_PAGE_FLAG_2000 = 0x2000,
};

enum ZMB_SI_PAGE : int16 {
	ZMB_SI_MINUS1 = -1,
	ZMB_SI_TOWN_00 = 0,
	ZMB_SI_PICKER_01 = 1,
	ZMB_SI_BRIDGE_02 = 2,
	ZMB_SI_TUNNELS_03 = 3,
	ZMB_SI_PIZZA_04 = 4,
	ZMB_SI_BC1_NORTH_05 = 5,
	ZMB_SI_BC1_SOUTH_06 = 6,
	ZMB_SI_FERRY_07 = 7,
	ZMB_SI_LILLY_08 = 8,
	ZMB_SI_SLIDES_09 = 9,
	ZMB_SI_FLEENS_10 = 10,
	ZMB_SI_HOTEL_11 = 11,
	ZMB_SI_NET_12 = 12,
	ZMB_SI_BASECAMP2_13 = 13,
	ZMB_SI_CAVES_14 = 14,
	ZMB_SI_SMOKE_15 = 15,
	ZMB_SI_MAZE_16 = 16,
};

enum ZMB_DI_PAGE : int16 {
	ZMB_DI_UNK_M1 = -1,
	ZMB_DI_UNK_00 = 0,
	ZMB_DI_MAP_01 = 1,
	ZMB_DI_UNK_02 = 2,
	ZMB_DI_ISLE_03 = 3,
	ZMB_DI_BC1_04 = 4,
	ZMB_DI_BC2_05 = 5,
	ZMB_DI_TOWN_06 = 6,
	ZMB_DI_BRIDGE_07 = 7,
	ZMB_DI_TUNNELS_08 = 8,
	ZMB_DI_PIZZA_09 = 9,
	ZMB_DI_FERRY_10 = 10,
	ZMB_DI_LILLY_11 = 11,
	ZMB_DI_SLIDES_12 = 12,
	ZMB_DI_FLEENS_13 = 13,
	ZMB_DI_HOTEL_14 = 14,
	ZMB_DI_NET_15 = 15,
	ZMB_DI_CAVES_16 = 16,
	ZMB_DI_SMOKE_17 = 17,
	ZMB_DI_MAZE_18 = 18,
	ZMB_DI_UNK_19 = 19,
	ZMB_DI_UNK_20 = 20,
	ZMB_DI_UNK_21 = 21,
};

enum ZMB_SUB_ROUTE_ID : uint16 {
	ZMB_ROUTE_BIG_BAD_HUNGRY_0 = 0,
	ZMB_ROUTE_BIG_BAD_HUNGRY_1 = 1,
	ZMB_ROUTE_BIG_BAD_HUNGRY_2 = 2,
	ZMB_ROUTE_BIG_BAD_HUNGRY_3 = 3,
	ZMB_ROUTE_WHOS_BAYOU_0 = 4,
	ZMB_ROUTE_WHOS_BAYOU_1 = 5,
	ZMB_ROUTE_WHOS_BAYOU_2 = 6,
	ZMB_ROUTE_WHOS_BAYOU_3 = 7,
	ZMB_ROUTE_DEEP_DARK_FOREST_0 = 8,
	ZMB_ROUTE_DEEP_DARK_FOREST_1 = 9,
	ZMB_ROUTE_DEEP_DARK_FOREST_2 = 10,
	ZMB_ROUTE_DEEP_DARK_FOREST_3 = 11,
	ZMB_ROUTE_MONT_DESPAIR_0 = 12,
	ZMB_ROUTE_MONT_DESPAIR_1 = 13,
	ZMB_ROUTE_MONT_DESPAIR_2 = 14,
	ZMB_ROUTE_MONT_DESPAIR_3 = 15,
};

enum ZMB_ROUTE_ID : uint16 {
	ZMB_ROUTE_BIG_BAD_HUNGRY = 0,
	ZMB_ROUTE_WHOS_BAYOU = 1,
	ZMB_ROUTE_DEEP_DARK_FOREST = 2,
	ZMB_ROUTE_MONT_DESPAIR = 3,
};

struct ZmbTrait {
	static constexpr int16 SNOID_INCOMPLETE = -1;
	static constexpr int16 SNOID_MAX = 625; // 5^4 combinations
	static constexpr byte TRAIT_NONE = 0;

	/***
	 * 0 means no trait, and 1 ~ 5 are the valid trait values.
	 */
	byte _head = TRAIT_NONE;
	/***
	 * 0 means no trait, and 1 ~ 5 are the valid trait values.
	 */
	byte _eye = TRAIT_NONE;
	/***
	 * 0 means no trait, and 1 ~ 5 are the valid trait values.
	 */
	byte _nose = TRAIT_NONE;
	/***
	 * 0 means no trait, and 1 ~ 5 are the valid trait values.
	 */
	byte _foot = TRAIT_NONE;

	ZmbTrait() = default;
	ZmbTrait(byte head, byte eye, byte nose, byte foot) : _head(head), _eye(eye), _nose(nose), _foot(foot) { }

	int16 snoidId() const;
	bool isComplete() const { return snoidId() != SNOID_INCOMPLETE; }
	void sync(Common::Serializer &s);
};

// For Stored Zoombinis (the ones which are on Rest or Ville)
struct ZmbStateStoredEntry {
	ZmbTrait _traits;
	Common::Rect _rect;
	byte _name[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	void sync(Common::Serializer &s);
	Common::U32String getU32Name(MohawkEngine_Zoombini* vm);
};

struct ZmbStateStoredChunk {
	/**
	 * Range: 0 ~ 120, each column has 5 entries, so max 600 entries in total.
	 */
	uint16 _leftmostColumnIdx = 0;
	uint16 _storedCount = 0;
	ZmbStateStoredEntry _entries[625];

	void sync(Common::Serializer &s);
};

// For Active Packs (the ones you are carrying)
struct ZmbStateActiveEntry {
	ZmbTrait _traits;
	/**
	 * 0x04: X coordinate of unoccupied slot position (WORD)
	 * When the slot is unoccupied, this is the X screen position used for animation.
	 */
	uint16 _posX = 0;
	/**
	 * 0x06: Y coordinate of unoccupied slot position (WORD)
	 * When the slot is unoccupied, this is the Y screen position used for animation.
	 */
	uint16 _posY = 0;
	uint8 _bIsOccupied = 0;
	byte _name[10] = { 0, };

	void sync(Common::Serializer &s);
	Common::U32String getU32Name(MohawkEngine_Zoombini* vm);
};

struct ZmbStateActivePack {
	int16 _wPackZmbCount = 0;
	/**
	 * +0x02: Skip occupied Zoombini animations flag
	 * 0 = show arrival animations for occupied slots, 1 = suppress them.
	 * Set to 1 when a pack is snapshotted into a basecamp (arrivals irrelevant).
	 */
	int16 _bSkipOccupiedAnim = 0;
	/**
	 * +0x04: Skip unoccupied slot animations flag
	 * 0 = show departure animations for empty slots, 1 = suppress them.
	 * Set to 1 on the live active pack (departures from empty slots irrelevant).
	 */
	int16 _bSkipUnoccupiedAnim = 0;
	ZmbStateActiveEntry _entries[16];
	uint8 _unk0136[302] = { 0, };

	void sync(Common::Serializer &s);
	void copyTo(ZmbStateActivePack &dest) {
		memcpy(&dest, this, sizeof(ZmbStateActivePack));
	}
};

struct ZmbStateFile { // Size: 44559 (0xAE0F)
	// 
	/**
	 * 0x0000: Magic, always 00 6B in bytes
	 */
	uint16 _magic6B00 = ZMB_ENDIAN_MAGIC;
	/**
	 * 0x0002: Auto-sticky mouse delay threshold (big-endian in file), default 0x1E (30)
	 */
	uint16 _autoStickyDelay = 0x001E;
	
	// 0x0004: Flags
	/**
	 * 0x0004: SFX (Sound Effects) Enable Flag
	 * Runtime global: chBoolSFXTurnOnOff, toggled by Ctrl+D and options menu button 5
	 */
	uint8 _flagSfxEnable = 1;
	/**
	 * 0x0005: BGM (Background Music) Enable Flag
	 * Runtime global: chBoolBGMTurnOnOff, toggled by Ctrl+B and options menu button 6
	 */
	uint8 _flagBgmEnable = 1;
	/**
	 * 0x0006: Sticky Mouse Enable Flag
	 * Runtime global: byte_4B8254, toggled by Ctrl+J and options menu button 7
	 */
	uint8 _flagStickyMouseEnable = 1;
	/**
	 * 0x0007: Cursor Visible Flag
	 * Runtime global: chCursorVisible_4B8255, toggled by Ctrl+H
	 */
	uint8 _flagCursorVisible = 1;
	/**
	 * 0x0008: Debug Mode Flag
	 * Runtime global: chDebugFlag_4B8257
	 */
	uint8 _flagDebug = 0;
	/**
	 * 0x0009: Auto-Sticky Mouse Flag
	 * Runtime global: chAutoStickeyFlag_4B8256, toggled by Ctrl+U
	 */
	uint8 _flagAutoStickyMouse = 0;
	/**
	 * 0x000A: Transition Disable Flag
	 */
	uint8 _flagTransitionsDisable = 0;
	uint8 _unk000B = 0;
	uint8 _unk000C = 0;
	uint8 _unk000D = 0;
	uint8 _unk000E = 0;
	uint8 _unk000F = 0;
	uint8 _unk0010 = 0;
	uint8 _unk0011 = 0;
	uint8 _unk0012 = 0;
	uint8 _unk0013 = 0;
	/**
	 * 0x0014 ~ 0x001C: Basecamp1 mushroom color state (0 ~ 4)
	 */
	uint16 _bcOneMushroomColors[5] = { 0, 0, 0, 0, 0 }; 
	/**
	 * 0x001E: Town zoombini grid scroll column (0~5)
	 * Restored on town load: scrolls the grid left by (scrollCol × 320) pixels.
	 * Updated by left/right click on the scroll arrows, wraps 0↔5.
	 */
	uint16 _townScrollCol = 0;
	/**
	 * 0x0020: Less/More Action Mode Flag
	 * - less action: 1, some features will not be drawn.
	 * - more action: 0
	 */
	uint16 _lessActionFlag = 0;
	/**
	 * 0x0022: (Unused) Fleens puzzle best score (0~99)
	 * Tracks the highest number of zoombinis successfully placed in the
	 * Fleens minigame. Updated each frame from unk_4AEF5A[0] preRender callback.
	 */
	uint16 _wFleensHighScore = 0;
	/**
	 * 0x0024: (Unused) Mudball Wall puzzle best score (0~999)
	 * Tracks the highest score achieved across all Mudball Wall sessions.
	 * Updated each frame from word_4AF242 in the puzzle preRender callback.
	 */
	uint16 _wMudballHighScore = 0;
	/**
	 * 0x0026: (Unused?) Picker cave mark blink state (0~3)
	 * Animation cycle state for the cave-mark arrows on the picker screen.
	 * Cycles 0→1→2→3→0 each tick. Each value controls which of the two
	 * runner SCRBs (wFeatureRunnerIdx24 / wFeatureRunnerIdx26) are visible.
	 * Initialized to 1 in less-action mode (both arrows hidden).
	 */
	uint16 _wPickerCaveBlinkState = 1;

	// 0x0028: Page Flags (Guess: Increase on page visit?)
	uint16 _pageFlagIsle = 0; 
  	uint16 _pageFlagBridge = 0;
  	uint16 _pageFlagTunnels = 0;
	uint16 _pageFlagPizza = 0;
	uint16 _pageFlagBasecamp1 = 0;
	uint16 _pageFlagFerry = 0;
	uint16 _pageFlagLilly = 0;
	uint16 _pageFlagSlides = 0;
	uint16 _pageFlagFleens = 0;
	uint16 _pageFlagHotel = 0;
	uint16 _pageFlagNet = 0;
	uint16 _pageFlagBasecamp2 = 0;
	uint16 _pageFlagCaves = 0;
	uint16 _pageFlagSmoke = 0;
	uint16 _pageFlagMaze = 0;
	uint16 _pageFlagTown = 0;

	// 0x0048: Generated & Stored Zoombini Count
	int16 _zmbGeneratedCount = 0; // 0x0048
	int16 _zmbStoredBC1Count = 0; // 0x004A
	int16 _zmbStoredBC2Count = 0; // 0x004C
	int16 _zmbStoredTownCount = 0; // 0x004E

	// 0x0050: Level Flags
	uint8 _levelFlagRouteBigBadHungry = 0;
	uint8 _levelFlagRouteMontDespair = 0;
	uint8 _levelFlagLoWhosBayouHiDeepDarkForest = 0;
	uint8 _levelFlagPageArr[15] = { 0, };

	// 0x0062: Memorial Stone Records
	uint16 _memorialYear[16] = { 0, };
	uint8 _memorialMonth[16] = { 0, };
	uint8 _memorialDay[16] = { 0, };
	uint8 _memorialRoute[16] = { 0, };
	uint8 _memorialLevel[16] = { 0, };

	// 0x00C2: Route Levels (Little Endian)
	/**
	 * 0 ~ 3 (Level 1 ~ 4)
	 */
	uint16 _routeLevels[4] = { 0, 0, 0, 0};
	/**
	 * 0x00CA: Current route
	 * @remarks 0: Not in route (game launch), 1~4: Big Bad Hungry, Who's Bayou, Deep Dark Forest, Mountain of Despair
	 */
	uint16 _currentRoute = 0;
	/**
	 * 0x00CC: Current interactive page
	 * @remarks 3 ~ 18 (3: ISLE, 4: BC1, 5: BC2, 6: TOWN, 18: MAZE)
	 */
	ZMB_DI_PAGE _currentPage = ZMB_DI_ISLE_03;
	ZoombiniPageType getCurrentPageType() const;
	void setCurrentPageType(ZoombiniPageType pageType);
	
	// 0x00CE: Stored Zoombinis on Basecamp 1
	// 16 its entries (finished game), their head/eye/nose/foot are zeroed
	// Maybe it directs to its active pack?
	ZmbStateStoredChunk _storedChunkBC1;

	// 0x3688: Stored Zoombinis on Basecamp 2
	// On some of its 16 entries (finished game), their head/eye/nose/foot are zeroed
	// Maybe it directs to its active pack?
	ZmbStateStoredChunk _storedChunkBC2;

	// 0x6C42: Stored Zoombinis on Town
	ZmbStateStoredChunk _storedChunkTown;

	// 0xA1FC: Active Zoombini Packs
	ZmbStateActivePack _zmbPackIsle = { };
	uint16 _wZmbPackIsleVal = 0;
	ZmbStateActivePack _zmbPackBC1 = { }; // 0xA462
	uint16 _wZmbPackBC1Val = 0;
	ZmbStateActivePack _zmbPackBC2 = { }; // 0xA6C8
	uint16 _wZmbPackBC2Val = 0;
	ZmbStateActivePack _zmbPackActive = { }; // 0xA92E
	uint16 _wZmbPackActiveVal = 0;

	// 0xAB94: Zoombini Twin Status
	uint8 _twinGenStatus[625] = { 0, };
	
	/**
	 * 0xAE05~0xAE0B: Reserved WORDs added in v2 save format (44559 bytes).
	 * When loading a v1 save (44549 bytes), these are zeroed out.
	 * No other code reads or writes these fields; likely reserved padding.
	 */
	int16 _wReservedAE05 = 0;
	int16 _wReservedAE07 = 0;
	int16 _wReservedAE09 = 0;
	int16 _wReservedAE0B = 0;
	/**
	 * 0xAE0D: Town Develop Level (0~6)
	 */
	int16 _townDevelopLevel = 0;

	// 0xAE0F: EOF

	void sync(Common::Serializer &s);

	bool _isDirty = false;
};

struct ZmbRosterEntry {
	/**
	 * 22bytes + null terminator
	 */
	byte _saveName[23] = { 0, };
	/** 
	 * 8bytes + null terminator 
	*/
	byte _fileName[9] = { 0, };

	void sync(Common::Serializer &r);
	Common::U32String getSaveName(MohawkEngine_Zoombini* vm) const;
	Common::U32String getFileName(MohawkEngine_Zoombini* vm) const;
	/**
	 * Check if the given save name can fit in the save name field.
	 */
	static bool checkSaveNameSize(MohawkEngine_Zoombini* vm, const Common::U32String &uSaveName);
};

struct ZmbRosterFile {
	uint16 _magic006B = ZMB_ENDIAN_MAGIC;
	uint16 _saveCount1 = 0;
	/**
	 * Maybe last saved/modified index?
	 */
	uint16 _saveCount2 = 0;
	/**
	 * Up to 50 entries.
	 */
	ZmbRosterEntry _entries[50] = { };

	void sync(Common::Serializer &r);
};

class ZoombiniGameState {
public:
	ZoombiniGameState(MohawkEngine_Zoombini *vm, Common::SaveFileManager *saveFileMan);
	~ZoombiniGameState();

	void loadRoster();
	bool saveRoster();
	bool loadGame(int slot);
	bool saveGame(int slot);
	bool deleteGame(int slot);

	/**
	 * Scans all living Zoombini entries (active packs and stored chunks) and marks
	 * their names in _zoombiniNameGeneratedTable so that new names will not duplicate
	 * names already assigned to existing Zoombinis.
	 */
	void buildNameGeneratedTable();

	bool isStateDirty() const { return _f._isDirty; }
	bool isFirstLaunch() { 
		bool ret = _isFirstLaunch;
		_isFirstLaunch = false;
		return ret;
	}

	ZMB_DIFFICULTY_ID getDifficultyIdFromPageFlag(uint16& pageFlag);
	uint16 &getPageFlagFromPageType(ZoombiniPageType pageType);
	ZMB_DIFFICULTY_ID getDifficultyIdFromPageType(ZoombiniPageType pageType);
	int16 readActivePageRouteLevel();
	uint16 readRouteLevel(uint16 routeId);
	uint16 getPageIdxInRoute();
	bool isNextPageContainer();

	/**
	 * Runtime (non-persisted): set to the puzzle page DI when a puzzle that leads
	 * to a container page (BC1, BC2, Town) completes successfully.
	 * Read and cleared by BC1/BC2/Town during loadFeatures().
	 */
	uint16 _lastPageBeforeContainer = 0;
	bool inPracticeMode() { return _practiceLevel != 0; }

	/**
	 * Generate 16 random snoids in the active pack.
	 * Used for practice mode and debug jump commands.
	 */
	void generateRandomPack();

	ZmbRosterEntry *getActiveSaveRosterEntry();
	Common::U32String getActiveSaveName();
	int getActiveSaveSlot() { return _currentSaveSlot; }
	int searchSaveSlotByName(const Common::U32String &saveName);
	int getAvailableSaveSlot();
	void readPageHelpStrings(ZoombiniPageType pageType, Common::Array<Common::U32String> &helpStrs);

	// Options
	void startNewGame();
	bool getEnableSound() { return _f._flagSfxEnable != 0; }
	bool getEnableMusic() { return _f._flagBgmEnable != 0; }
	bool getEnableStickyMouse() { return _f._flagStickyMouseEnable != 0; }
	bool getEnableTransitions() { return _f._flagTransitionsDisable == 0; }
	bool isLessActionEnabled() { return _f._lessActionFlag != 0; }
	bool isCursorVisible() { return _flagCursorVisible; }
	void setEnableSound(bool val);
	void setEnableMusic(bool val);
	void setEnableStickyMouse(bool val);
	void setEnableTransitions(bool val);
	void setLessActionEnabled(bool val);
	void setCursorVisible(bool val);
	bool toggleSound() { setEnableSound(!getEnableSound()); return getEnableSound(); }
	bool toggleMusic() { setEnableMusic(!getEnableMusic()); return getEnableMusic(); }
	bool toggleStickyMouse() { setEnableStickyMouse(!getEnableStickyMouse()); return getEnableStickyMouse(); }
	bool toggleTransitions() { setEnableTransitions(!getEnableTransitions()); return getEnableTransitions(); }
	bool toggleLessMoreAction() { setLessActionEnabled(!isLessActionEnabled()); return isLessActionEnabled(); }
	bool toggleCursorVisibility() { setCursorVisible(!isCursorVisible()); return isCursorVisible(); }
	
	ZmbStateFile _f;
	ZmbRosterFile _r;
	Common::Array<ZmbFeature*> _loadedZmbFeatures;

	uint16 _practiceLevel = 0;
	byte _zoombiniNameGeneratedTable[625] = { 0,};

	enum PredefinedSaveSlot {
		kNormal = 0,
		kUnsavedNewGame = -1,
	};
	int32 _currentSaveSlot = kUnsavedNewGame;

private:
	MohawkEngine_Zoombini *_vm;
	Common::SaveFileManager *_saveFileMan;

	bool _isFirstLaunch = true;
	
	/**
	 * Volatile runtime flag, not stored in save file.
	 */
	bool _flagCursorVisible = true;

	struct HelpSTRL{
		uint16 _helpResBase = 0;
		uint16 _pageNameIdx = 0;
		uint16 _unk3 = 0;
		uint16 _unk4 = 0;
		uint16 _unk5 = 0;
		uint16 _unk6 = 0;

		HelpSTRL() { }
		HelpSTRL(uint16 helpResNo, uint16 unk2, uint16 unk3, uint16 unk4, uint16 unk5, uint16 unk6) :
			_helpResBase(helpResNo), _pageNameIdx(unk2), _unk3(unk3), _unk4(unk4), _unk5(unk5), _unk6(unk6) { }
	};

	Common::HashMap<ZoombiniPageType, HelpSTRL> _helpStrlMap;

	void syncGameState(Common::Serializer &s);
	static Common::String buildSaveFilename(int slot);
	static Common::String getRosterFilename();
	bool loadState(int slot);
	bool saveState(int slot);
};

} // End of namespace Mohawk

#endif
