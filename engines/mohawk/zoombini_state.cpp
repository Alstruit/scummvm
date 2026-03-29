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

#include "common/debug.h"
#include "common/serializer.h"
#include "common/system.h"
#include "graphics/thumbnail.h"

#include "mohawk/sound.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_page.h"
#include "mohawk/zoombini_pages/interactive_base.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"
#include "mohawk/zoombini_text.h"

namespace Mohawk {

ZoombiniGameState::ZoombiniGameState(MohawkEngine_Zoombini *vm, Common::SaveFileManager *saveFileMan) : _vm(vm), _saveFileMan(saveFileMan) {
	// Initialize help string map
	_helpStrlMap[ZoombiniPageType::kPicker] = HelpSTRL(1300, 0, 3, 1, 1, 1);
	_helpStrlMap[ZoombiniPageType::kBasecamp1] = HelpSTRL(1400, 4, 2, 1, 1, 1);
	_helpStrlMap[ZoombiniPageType::kBasecamp2] = HelpSTRL(1500, 11, 1, 1, 1, 1);
	_helpStrlMap[ZoombiniPageType::kTown] = HelpSTRL(1600, 15, 3, 1, 1, 1);
	_helpStrlMap[ZoombiniPageType::kBridge] = HelpSTRL(1700, 1, 1, 1, 1, 1);
	_helpStrlMap[ZoombiniPageType::kCaves] = HelpSTRL(2600, 12, 2, 2, 2, 2);
	_helpStrlMap[ZoombiniPageType::kPizza] = HelpSTRL(1900, 3, 1, 2, 3, 3);
	_helpStrlMap[ZoombiniPageType::kFerry] = HelpSTRL(2000, 5, 1, 1, 1, 1);
	_helpStrlMap[ZoombiniPageType::kLilly] = HelpSTRL(2100, 6, 2, 2, 4, 4);
	_helpStrlMap[ZoombiniPageType::kSlides] = HelpSTRL(2200, 7, 1, 1, 1, 2);
	_helpStrlMap[ZoombiniPageType::kFleens] = HelpSTRL(2300, 8, 2, 3, 3, 3);
	_helpStrlMap[ZoombiniPageType::kHotel] = HelpSTRL(2400, 9, 1, 2, 3, 3);
	_helpStrlMap[ZoombiniPageType::kNet] = HelpSTRL(2500, 10, 1, 1, 2, 2);
	_helpStrlMap[ZoombiniPageType::kTunnels] = HelpSTRL(1800, 2, 2, 2, 2, 2);
	_helpStrlMap[ZoombiniPageType::kSmoke] = HelpSTRL(2700, 13, 2, 2, 3, 3);
	_helpStrlMap[ZoombiniPageType::kMaze] = HelpSTRL(2800, 14, 3, 3, 3, 3);
	_helpStrlMap[static_cast<ZoombiniPageType>(19)] = HelpSTRL(2900, 0, 1, 1, 1, 1);
}

ZoombiniGameState::~ZoombiniGameState() {
}

bool ZoombiniGameState::loadGame(int slot) {
	if (!loadState(slot)) {
		return false;
	}

	// Setup _zoombiniNameGeneratedTable based on the generated/stored zoombinis information
	buildNameGeneratedTable();

	return true;
}

bool ZoombiniGameState::saveGame(int slot) {
	if (!saveState(slot)) {
		return false;
	}
	return true;
}

bool ZoombiniGameState::deleteGame(int slot) {
	Common::String filename = buildSaveFilename(slot);

	debugC(kZmbDebugSaveLoad, "Deleting save file \'%s\'", filename.c_str());

	return g_system->getSavefileManager()->removeSavefile(filename);
}

bool ZoombiniGameState::loadState(int slot) {
	Common::String filename = buildSaveFilename(slot);
	Common::InSaveFile *loadFile = _saveFileMan->openForLoading(filename);
	if (!loadFile) {
		return false;
	}

	debugC(kZmbDebugSaveLoad, "Loading game from '%s'", filename.c_str());

	// Check save file size
	int32 size = loadFile->size();
	if (size != 44559) {
		warning("Incompatible saved game version");
		delete loadFile;
		return false;
	}

	Common::Serializer s(loadFile, nullptr);
	syncGameState(s);
	delete loadFile;

	_currentSaveSlot = slot;
	return true;
}

void ZoombiniGameState::syncGameState(Common::Serializer &s) {
	_f.sync(s);
	_f._isDirty = false;
}

Common::String ZoombiniGameState::buildSaveFilename(int slot) {
	return Common::String::format("ZOOM%04d.TXT", slot);
}

Common::String ZoombiniGameState::getRosterFilename() {
	return Common::String("ZOOMBINI.WHO");
}

void ZoombiniGameState::buildNameGeneratedTable() {
	memset(_zoombiniNameGeneratedTable, 0, sizeof(_zoombiniNameGeneratedTable));

	// v1.1 English ZOOMBINI.MHK does not contain STRL 30000-30006 (the name pool).
	// The original engine never rebuilt this table from save data anyway — it is
	// session-only.  Skip the reverse-lookup rebuild and leave the table zeroed.
	if (!_vm->hasResource(ID_STRL, ZmbResource(ZmbArchiveKind::kSystem, ZoombiniPage::kResStrl30000_ZoombiniNames)))
		return;

	// Ensure the full name pool (0..624) is loaded and the reverse index is ready.
	_vm->_text->cacheAllZoombiniNames();

	// Helper: decode a raw name buffer and mark its slot in the generated table.
	auto markName = [&](const byte *nameBytes, uint32 nameLen) {
		Common::U32String name = _vm->_text->toU32String(nameBytes, nameLen);
		if (name.empty())
			return;
		int32 nameId = _vm->_text->findZoombiniNameId(name);
		if (nameId >= 0)
			_zoombiniNameGeneratedTable[nameId] = 1;
	};

	// Scan active packs (Isle, BC1, BC2, Active)
	ZmbStateActivePack *packs[] = {&_f._zmbPackIsle, &_f._zmbPackBC1, &_f._zmbPackBC2, &_f._zmbPackActive};
	for (uint32 p = 0; p < ARRAYSIZE(packs); p++) {
		for (int i = 0; i < ARRAYSIZE(packs[p]->_entries); i++) {
			const ZmbStateActiveEntry &e = packs[p]->_entries[i];
			if (e._bIsOccupied)
				markName(e._name, ARRAYSIZE(e._name));
		}
	}

	// Scan stored chunks (BC1, BC2, Town).
	// Entries at arbitrary positions — check for any non-zero name byte.
	ZmbStateStoredChunk *chunks[] = {&_f._storedChunkBC1, &_f._storedChunkBC2, &_f._storedChunkTown};
	for (uint32 c = 0; c < ARRAYSIZE(chunks); c++) {
		for (int i = 0; i < ARRAYSIZE(chunks[c]->_entries); i++) {
			const ZmbStateStoredEntry &e = chunks[c]->_entries[i];
			bool hasName = false;
			for (uint32 nb = 0; nb < ARRAYSIZE(e._name); nb++) {
				if (e._name[nb] != 0) {
					hasName = true;
					break;
				}
			}
			if (hasName)
				markName(e._name, ARRAYSIZE(e._name));
		}
	}
}

bool ZoombiniGameState::saveState(int slot) {
	// TODO
	debug("Saving game state not implemented yet");
	return false;
}

void ZoombiniGameState::loadRoster() {
	Common::String filename = getRosterFilename();
	Common::InSaveFile *rosterFile = _saveFileMan->openForLoading(filename);
	if (!rosterFile) {
		return;
	}

	debugC(kZmbDebugSaveLoad, "Loading roster from '%s'", filename.c_str());

	// Check save file size
	int32 size = rosterFile->size();
	if (size != 1606) {
		warning("Incompatible roster game version");
		delete rosterFile;
		return;
	}

	Common::Serializer r(rosterFile, nullptr);
	_r.sync(r);

	return;
}

bool ZoombiniGameState::saveRoster() {
	return false;
}

int16 ZmbTrait::snoidId() const {
	assert(0 <= _head && _head <= 5);
	assert(0 <= _eye && _eye <= 5);
	assert(0 <= _nose && _nose <= 5);
	assert(0 <= _foot && _foot <= 5);

	if (_head == 0 || _eye == 0 || _nose == 0 || _foot == 0)
		return SNOID_INCOMPLETE; // No trait, invalid id

	return (_head - 1) * 125 + (_eye - 1) * 25 + (_nose - 1) * 5 + (_foot - 1);
}

void ZmbTrait::sync(Common::Serializer &s) {
	s.syncAsByte(_head);
	s.syncAsByte(_eye);
	s.syncAsByte(_nose);
	s.syncAsByte(_foot);

	assert(0 <= _head && _head <= 5);
	assert(0 <= _eye && _eye <= 5);
	assert(0 <= _nose && _nose <= 5);
	assert(0 <= _foot && _foot <= 5);
}

void ZmbStateStoredEntry::sync(Common::Serializer &s) {
	_traits.sync(s);
	s.syncAsUint16LE(_rect.bottom);
	s.syncAsUint16LE(_rect.right);
	s.syncAsUint16LE(_rect.top);
	s.syncAsUint16LE(_rect.left);
	s.syncBytes(_name, ARRAYSIZE(_name));
}

Common::U32String ZmbStateStoredEntry::getU32Name(MohawkEngine_Zoombini *vm) {
	return vm->_text->toU32String(_name, ARRAYSIZE(_name));
}

void Mohawk::ZmbStateStoredChunk::sync(Common::Serializer &s) {
	s.syncAsUint16LE(_leftmostColumnIdx);
	s.syncAsUint16LE(_storedCount);
	for (int i = 0; i < ARRAYSIZE(_entries); i++)
		_entries[i].sync(s);
}

void ZmbStateActiveEntry::sync(Common::Serializer &s) {
	_traits.sync(s);
	s.syncAsUint16LE(_posX);
	s.syncAsUint16LE(_posY);
	s.syncAsByte(_bIsOccupied);
	s.syncBytes(_name, ARRAYSIZE(_name));
}

Common::U32String ZmbStateActiveEntry::getU32Name(MohawkEngine_Zoombini *vm) {
	return vm->_text->toU32String(_name, ARRAYSIZE(_name));
}

void ZmbStateActivePack::sync(Common::Serializer &s) {
	s.syncAsSint16LE(_wPackZmbCount);
	s.syncAsSint16LE(_bSkipOccupiedAnim);
	s.syncAsSint16LE(_bSkipUnoccupiedAnim);
	for (int i = 0; i < ARRAYSIZE(_entries); i++)
		_entries[i].sync(s);
	s.syncBytes(_unk0136, ARRAYSIZE(_unk0136));
}

void ZmbRosterFile::sync(Common::Serializer &r) {
	r.syncAsUint16LE(_magic006B);
	r.syncAsUint16LE(_saveCount1);
	r.syncAsUint16LE(_saveCount2);
	for (int32 i = 0; i < ARRAYSIZE(_entries); i++) {
		_entries[i].sync(r);
	}
}

void ZmbRosterEntry::sync(Common::Serializer &r) {
	r.syncBytes(_saveName, ARRAYSIZE(_saveName));
	r.syncBytes(_fileName, ARRAYSIZE(_fileName));
}

Common::U32String ZmbRosterEntry::getSaveName(MohawkEngine_Zoombini *vm) const {
	return vm->_text->toU32String(_saveName, ARRAYSIZE(_saveName) - 1);
}

Common::U32String ZmbRosterEntry::getFileName(MohawkEngine_Zoombini *vm) const {
	return vm->_text->toU32String(_fileName, ARRAYSIZE(_fileName) - 1);
}

bool ZmbRosterEntry::checkSaveNameSize(MohawkEngine_Zoombini *vm, const Common::U32String &uSaveName) {
	Common::String ansiSaveName = vm->_text->fromU32String(uSaveName);
	return ansiSaveName.size() < ARRAYSIZE(_saveName);
}

ZoombiniPageType ZmbStateFile::getCurrentPageType() const {
	switch (_currentPage) {
	case ZMB_DI_MAP_01:
		return ZoombiniPageType::kRodMap;
	case ZMB_DI_ISLE_03:
		return ZoombiniPageType::kPicker;
	case ZMB_DI_BC1_04:
		return ZoombiniPageType::kBasecamp1;
	case ZMB_DI_BC2_05:
		return ZoombiniPageType::kBasecamp2;
	case ZMB_DI_TOWN_06:
		return ZoombiniPageType::kTown;
	case ZMB_DI_BRIDGE_07:
		return ZoombiniPageType::kBridge;
	case ZMB_DI_TUNNELS_08:
		return ZoombiniPageType::kTunnels;
	case ZMB_DI_PIZZA_09:
		return ZoombiniPageType::kPizza;
	case ZMB_DI_FERRY_10:
		return ZoombiniPageType::kFerry;
	case ZMB_DI_LILLY_11:
		return ZoombiniPageType::kLilly;
	case ZMB_DI_SLIDES_12:
		return ZoombiniPageType::kSlides;
	case ZMB_DI_FLEENS_13:
		return ZoombiniPageType::kFleens;
	case ZMB_DI_HOTEL_14:
		return ZoombiniPageType::kHotel;
	case ZMB_DI_NET_15:
		return ZoombiniPageType::kNet;
	case ZMB_DI_CAVES_16:
		return ZoombiniPageType::kCaves;
	case ZMB_DI_SMOKE_17:
		return ZoombiniPageType::kSmoke;
	case ZMB_DI_MAZE_18:
		return ZoombiniPageType::kMaze;
	default:
		error("Invalid currentPage value: %d", _currentPage);
		return ZoombiniPageType::kNone;
	}
}

void ZmbStateFile::setCurrentPageType(ZoombiniPageType pageType) {
	ZMB_DI_PAGE lastPage = _currentPage;

	switch (pageType) {
	case ZoombiniPageType::kRodMap:
		_currentPage = ZMB_DI_MAP_01;
		break;
	case ZoombiniPageType::kPicker:
		_currentPage = ZMB_DI_ISLE_03;
		break;
	case ZoombiniPageType::kBasecamp1:
		_currentPage = ZMB_DI_BC1_04;
		break;
	case ZoombiniPageType::kBasecamp2:
		_currentPage = ZMB_DI_BC2_05;
		break;
	case ZoombiniPageType::kTown:
		_currentPage = ZMB_DI_TOWN_06;
		break;
	case ZoombiniPageType::kBridge:
		_currentPage = ZMB_DI_BRIDGE_07;
		break;
	case ZoombiniPageType::kTunnels:
		_currentPage = ZMB_DI_TUNNELS_08;
		break;
	case ZoombiniPageType::kPizza:
		_currentPage = ZMB_DI_PIZZA_09;
		break;
	case ZoombiniPageType::kFerry:
		_currentPage = ZMB_DI_FERRY_10;
		break;
	case ZoombiniPageType::kLilly:
		_currentPage = ZMB_DI_LILLY_11;
		break;
	case ZoombiniPageType::kSlides:
		_currentPage = ZMB_DI_SLIDES_12;
		break;
	case ZoombiniPageType::kFleens:
		_currentPage = ZMB_DI_FLEENS_13;
		break;
	case ZoombiniPageType::kHotel:
		_currentPage = ZMB_DI_HOTEL_14;
		break;
	case ZoombiniPageType::kNet:
		_currentPage = ZMB_DI_NET_15;
		break;
	case ZoombiniPageType::kCaves:
		_currentPage = ZMB_DI_CAVES_16;
		break;
	case ZoombiniPageType::kSmoke:
		_currentPage = ZMB_DI_SMOKE_17;
		break;
	case ZoombiniPageType::kMaze:
		_currentPage = ZMB_DI_MAZE_18;
		break;
	default:
		error("Invalid pageType value: %d", static_cast<int32>(pageType));
		break;
	}

	_isDirty |= (lastPage != _currentPage);
}

void ZmbStateFile::sync(Common::Serializer &s) {
	// 0x0000: Magic
	s.syncAsUint16LE(_magic6B00);
	s.syncAsUint16LE(_autoStickyDelay);

	// 0x0004: Flags
	s.syncAsByte(_flagSfxEnable);
	s.syncAsByte(_flagBgmEnable);
	s.syncAsByte(_flagStickyMouseEnable);
	s.syncAsByte(_flagCursorVisible);
	s.syncAsByte(_flagDebug);
	s.syncAsByte(_flagAutoStickyMouse);
	s.syncAsByte(_flagTransitionsDisable);
	s.syncAsByte(_unk000B);
	s.syncAsByte(_unk000C);
	s.syncAsByte(_unk000D);
	s.syncAsByte(_unk000E);
	s.syncAsByte(_unk000F);
	s.syncAsByte(_unk0010);
	s.syncAsByte(_unk0011);
	s.syncAsByte(_unk0012);
	s.syncAsByte(_unk0013);
	for (int32 i = 0; i < ARRAYSIZE(_bcOneMushroomColors); i++)
		s.syncAsUint16LE(_bcOneMushroomColors[i]);
	s.syncAsUint16LE(_townScrollCol);
	s.syncAsUint16LE(_lessActionFlag);
	s.syncAsUint16LE(_wFleensHighScore);
	s.syncAsUint16LE(_wMudballHighScore);
	s.syncAsUint16LE(_wPickerCaveBlinkState);

	// 0x0028: Page Flags
	s.syncAsUint16LE(_pageFlagIsle);
	s.syncAsUint16LE(_pageFlagBridge);
	s.syncAsUint16LE(_pageFlagTunnels);
	s.syncAsUint16LE(_pageFlagPizza);
	s.syncAsUint16LE(_pageFlagBasecamp1);
	s.syncAsUint16LE(_pageFlagFerry);
	s.syncAsUint16LE(_pageFlagLilly);
	s.syncAsUint16LE(_pageFlagSlides);
	s.syncAsUint16LE(_pageFlagFleens);
	s.syncAsUint16LE(_pageFlagHotel);
	s.syncAsUint16LE(_pageFlagNet);
	s.syncAsUint16LE(_pageFlagBasecamp2);
	s.syncAsUint16LE(_pageFlagCaves);
	s.syncAsUint16LE(_pageFlagSmoke);
	s.syncAsUint16LE(_pageFlagMaze);
	s.syncAsUint16LE(_pageFlagTown);

	// 0x0048: Generated & Stored Zoombini Count
	s.syncAsSint16LE(_zmbGeneratedCount);
	s.syncAsSint16LE(_zmbStoredBC1Count);
	s.syncAsSint16LE(_zmbStoredBC2Count);
	s.syncAsSint16LE(_zmbStoredTownCount);

	// 0x0050: Level Flags
	s.syncAsByte(_levelFlagRouteBigBadHungry);
	s.syncAsByte(_levelFlagRouteMontDespair);
	s.syncAsByte(_levelFlagLoWhosBayouHiDeepDarkForest);
	s.syncBytes(_levelFlagPageArr, ARRAYSIZE(_levelFlagPageArr));

	// 0x0062: Memorial Stone Records
	for (int32 i = 0; i < ARRAYSIZE(_memorialYear); i++)
		s.syncAsUint16LE(_memorialYear[i]);
	for (int32 i = 0; i < ARRAYSIZE(_memorialMonth); i++)
		s.syncAsByte(_memorialMonth[i]);
	for (int32 i = 0; i < ARRAYSIZE(_memorialDay); i++)
		s.syncAsByte(_memorialDay[i]);
	for (int32 i = 0; i < ARRAYSIZE(_memorialRoute); i++)
		s.syncAsByte(_memorialRoute[i]);
	for (int32 i = 0; i < ARRAYSIZE(_memorialLevel); i++)
		s.syncAsByte(_memorialLevel[i]);

	// 0x00C2: Route Levels
	for (int32 i = 0; i < ARRAYSIZE(_routeLevels); i++) {
		s.syncAsUint16LE(_routeLevels[i]);
	}
	s.syncAsUint16LE(_currentRoute);
	s.syncAsSint16LE(_currentPage);

	// 0x00CE: Stored Zoombinis on Basecamp 1
	_storedChunkBC1.sync(s);

	// 0x3688: Stored Zoombinis on Basecamp 2
	_storedChunkBC2.sync(s);

	// 0x6C42: Stored Zoombinis on Town
	_storedChunkTown.sync(s);

	// 0xA1FC: Active Zoombini Packs
	_zmbPackIsle.sync(s);
	s.syncAsUint16LE(_wZmbPackIsleVal);
	_zmbPackBC1.sync(s);
	s.syncAsUint16LE(_wZmbPackBC1Val);
	_zmbPackBC2.sync(s);
	s.syncAsUint16LE(_wZmbPackBC2Val);
	_zmbPackActive.sync(s);
	s.syncAsUint16LE(_wZmbPackActiveVal);

	// 0xAB94: Zoombini Twin Status
	s.syncBytes(_twinGenStatus, ARRAYSIZE(_twinGenStatus));

	// 0xAE05: Reserved WORDs (zeroed when loading v1 format / 44549-byte saves)
	s.syncAsSint16LE(_wReservedAE05);
	s.syncAsSint16LE(_wReservedAE07);
	s.syncAsSint16LE(_wReservedAE09);
	s.syncAsSint16LE(_wReservedAE0B);
	s.syncAsSint16LE(_townDevelopLevel);

	// 0xAE0F: EOF
}

ZMB_DIFFICULTY_ID ZoombiniGameState::getDifficultyIdFromPageFlag(uint16 &pageFlag) {
	if (0 < _practiceLevel) {
		return ZMB_DIFFICULTY_LEVEL3_05;
	}

	if ((pageFlag & ZMB_PAGE_MASK_0FFF) < 0x0FFF) {
		pageFlag += 1;
	}

	int routeLevel = readActivePageRouteLevel();
	if (routeLevel == 0) {
		return ZMB_DIFFICULTY_LEVEL1_01;
	}
	if (routeLevel != 1) {
		if ((pageFlag & ZMB_PAGE_FLAG_1000) != 0) {
			if ((pageFlag & ZMB_PAGE_FLAG_2000) == 0) {
				pageFlag |= ZMB_PAGE_FLAG_2000;
				return ZMB_DIFFICULTY_LEVEL4_12;
			}
		} else {
			pageFlag |= ZMB_PAGE_FLAG_1000;
			return ZMB_DIFFICULTY_LEVEL2_02;
		}
	}
	return ZMB_DIFFICULTY_NOTVISITED_00;
}

uint16 &ZoombiniGameState::getPageFlagFromPageType(ZoombiniPageType pageType) {
	switch (pageType) {
	case ZoombiniPageType::kPicker:
		return _f._pageFlagIsle;
	case ZoombiniPageType::kBridge:
		return _f._pageFlagBridge;
	case ZoombiniPageType::kCaves:
		return _f._pageFlagCaves;
	case ZoombiniPageType::kPizza:
		return _f._pageFlagPizza;
	case ZoombiniPageType::kBasecamp1:
		return _f._pageFlagBasecamp1;
	case ZoombiniPageType::kFerry:
		return _f._pageFlagFerry;
	case ZoombiniPageType::kLilly:
		return _f._pageFlagLilly;
	case ZoombiniPageType::kSlides:
		return _f._pageFlagSlides;
	case ZoombiniPageType::kFleens:
		return _f._pageFlagFleens;
	case ZoombiniPageType::kHotel:
		return _f._pageFlagHotel;
	case ZoombiniPageType::kNet:
		return _f._pageFlagNet;
	case ZoombiniPageType::kBasecamp2:
		return _f._pageFlagBasecamp2;
	case ZoombiniPageType::kTunnels:
		return _f._pageFlagTunnels;
	case ZoombiniPageType::kSmoke:
		return _f._pageFlagSmoke;
	case ZoombiniPageType::kMaze:
		return _f._pageFlagMaze;
	case ZoombiniPageType::kTown:
		return _f._pageFlagTown;
	default:
		error("Invalid pageType: %u", static_cast<uint32>(pageType));
		return _f._pageFlagIsle; // Avoid compiler warning
	}
}

ZMB_DIFFICULTY_ID ZoombiniGameState::getDifficultyIdFromPageType(ZoombiniPageType pageType) {
	uint16 &pageFlag = getPageFlagFromPageType(pageType);
	return getDifficultyIdFromPageFlag(pageFlag);
}

int16 ZoombiniGameState::readActivePageRouteLevel() {
	if (1 <= _practiceLevel && _practiceLevel <= 4) {
		return _practiceLevel - 1;
	}

	uint16 pageInRoute = getPageIdxInRoute(); // 0 ~ 3
	if (pageInRoute != 0)                     // Not in a container page
		return _f._routeLevels[pageInRoute - 1];

	switch (_f._currentPage) {
	case ZMB_DI_BC1_04: // BIG BAD AND HUNGRY
		return _f._routeLevels[0];
	case ZMB_DI_BC2_05: // WHO'S BAYOU or DEEP DARK FOREST
		return MAX(_f._routeLevels[1], _f._routeLevels[2]);
	case ZMB_DI_TOWN_06: // MOUNTAIN OF DESPAIR
		return _f._routeLevels[3];
	default:
		break;
	}
	return 0;
}

uint16 ZoombiniGameState::readRouteLevel(uint16 routeId) {
	switch (routeId) {
	case ZMB_ROUTE_BIG_BAD_HUNGRY:
		return _f._routeLevels[0];
	case ZMB_ROUTE_WHOS_BAYOU:
		return _f._routeLevels[1];
	case ZMB_ROUTE_DEEP_DARK_FOREST:
		return _f._routeLevels[2];
	case ZMB_ROUTE_MONT_DESPAIR:
		return _f._routeLevels[3];
	default:
		error("STATE: invalid route level for routeId(%hd)", routeId);
		break;
	}
	return 0;
}

uint16 ZoombiniGameState::getPageIdxInRoute() {
	switch (_f._currentPage) {
	case ZMB_DI_BRIDGE_07:
	case ZMB_DI_FERRY_10:
	case ZMB_DI_FLEENS_13:
	case ZMB_DI_CAVES_16:
		return 1;
	case ZMB_DI_TUNNELS_08:
	case ZMB_DI_LILLY_11:
	case ZMB_DI_HOTEL_14:
	case ZMB_DI_SMOKE_17:
		return 2;
	case ZMB_DI_PIZZA_09:
	case ZMB_DI_SLIDES_12:
	case ZMB_DI_NET_15:
	case ZMB_DI_MAZE_18:
		return 3;
	default:
		return 0;
	}
}

bool ZoombiniGameState::isNextPageContainer() {
	return _f._currentPage == ZMB_DI_PIZZA_09 || _f._currentPage == ZMB_DI_SLIDES_12 || _f._currentPage == ZMB_DI_NET_15 || _f._currentPage == ZMB_DI_MAZE_18;
}

ZmbRosterEntry *ZoombiniGameState::getActiveSaveRosterEntry() {
	if (0 <= _currentSaveSlot && _currentSaveSlot < static_cast<int32>(_r._saveCount1))
		return &_r._entries[_currentSaveSlot];

	// Unsaved new game
	if (_currentSaveSlot == kUnsavedNewGame)
		return nullptr;

	// Invalid slot
	error("Invalid current save slot: %d", _currentSaveSlot);
	return nullptr;
}

Common::U32String ZoombiniGameState::getActiveSaveName() {
	if (_currentSaveSlot == kUnsavedNewGame)
		return _vm->_text->getLocalizedString(ZoombiniText::kNewGame);

	return _vm->_state->getActiveSaveRosterEntry()->getSaveName(_vm);
}

int ZoombiniGameState::searchSaveSlotByName(const Common::U32String &saveName) {
	for (uint16 i = 0; i < _r._saveCount1; i++) {
		const ZmbRosterEntry &entry = _r._entries[i];

		// Zoombini save names are case sensitive
		if (entry.getSaveName(_vm).equals(saveName))
			return i;
	}

	return -1;
}

int ZoombiniGameState::getAvailableSaveSlot() {
	if (_r._saveCount1 < ARRAYSIZE(_r._entries))
		return _r._saveCount1;

	// No available slot
	return -1;
}

void ZoombiniGameState::readPageHelpStrings(ZoombiniPageType pageType, Common::Array<Common::U32String> &helpStrs) {
	uint16 level = 0;
	switch (pageType) {
	case ZoombiniPageType::kPicker:
	case ZoombiniPageType::kTown:
	case ZoombiniPageType::kBasecamp1:
	case ZoombiniPageType::kBasecamp2:
		level = 0;
		break;
	default:
		level = readActivePageRouteLevel();
		break;
	}

	// base: 1300 ~ 2800
	Common::HashMap<ZoombiniPageType, HelpSTRL>::iterator it = _helpStrlMap.find(pageType);
	if (it == _helpStrlMap.end()) {
		error("Cannot find help strings for pageType(%u)", static_cast<uint16>(pageType));
		return;
	}

	const HelpSTRL &helpStrl = it->_value;
	uint16 strlResNo = helpStrl._helpResBase + 20 * level;
	_vm->_text->getStrl(helpStrs, ZmbResource(ZmbArchiveKind::kSystem, strlResNo));
}

void ZoombiniGameState::startNewGame() {
	// Ask whether to save the current save, regardless of isDirty flag
	ZoombiniDialogResult result = _vm->openMsgBoxDialog(ZoombiniMsgBoxType::kAskSaveDirtyGame);
	if (result == ZoombiniDialogResult::kYes) {
		_vm->openSaveDialog();
	}

	// Ask whether to create a new game
	result = _vm->openMsgBoxDialog(ZoombiniMsgBoxType::kAskCreateNewGame);
	if (result == ZoombiniDialogResult::kNo) {
		return;
	}

	// Initialize a new game state
	_f = ZmbStateFile();
	_f._isDirty = true;
	_currentSaveSlot = kUnsavedNewGame;

	_practiceLevel = 0;
	ZoombiniPage *activePage = _vm->getActivePage();
	ZoombiniPageType nextPageType = activePage->getPageType();
	if (nextPageType != ZoombiniPageType::kPicker)
		nextPageType = ZoombiniPageType::kRodMap;
	_vm->setNextPage(nextPageType);
	activePage->close();
}

void ZoombiniGameState::setEnableSound(bool val) {
	_f._flagSfxEnable = val ? 1 : 0;
	_vm->_mixer->muteSoundType(Audio::Mixer::kSFXSoundType, !val);

	ZoombiniPage *page = _vm->getActivePage();
	if (page && page->getPageCategory() == ZoombiniPageCategory::kInteractive) {
		ZoombiniInteractive *interactive = dynamic_cast<ZoombiniInteractive *>(page);
		if (interactive)
			interactive->showNotiBoxLong(val ? ZoombiniText::kNotiBoxSoundOn : ZoombiniText::kNotiBoxSoundOff);
	}
}

void ZoombiniGameState::setEnableMusic(bool val) {
	_f._flagBgmEnable = val ? 1 : 0;
	_vm->_mixer->muteSoundType(Audio::Mixer::kMusicSoundType, !val);
	_vm->_midi->setVolume(val ? 255 : 0);

	ZoombiniPage *page = _vm->getActivePage();
	if (page && page->getPageCategory() == ZoombiniPageCategory::kInteractive) {
		ZoombiniInteractive *interactive = dynamic_cast<ZoombiniInteractive *>(page);
		if (interactive)
			interactive->showNotiBoxLong(val ? ZoombiniText::kNotiBoxMusicOn : ZoombiniText::kNotiBoxMusicOff);
	}
}

void ZoombiniGameState::setEnableStickyMouse(bool val) {
	_f._flagStickyMouseEnable = val ? 1 : 0;

	ZoombiniPage *page = _vm->getActivePage();
	if (page && page->getPageCategory() == ZoombiniPageCategory::kInteractive) {
		ZoombiniInteractive *interactive = dynamic_cast<ZoombiniInteractive *>(page);
		if (interactive)
			interactive->showNotiBoxLong(val ? ZoombiniText::kNotiBoxStickeyMouse : ZoombiniText::kNotiBoxNonStickeyMouse);
	}
}

void ZoombiniGameState::setEnableTransitions(bool val) {
	_f._flagTransitionsDisable = val ? 0 : 1;

	ZoombiniPage *page = _vm->getActivePage();
	if (page && page->getPageCategory() == ZoombiniPageCategory::kInteractive) {
		ZoombiniInteractive *interactive = dynamic_cast<ZoombiniInteractive *>(page);
		if (interactive)
			interactive->showNotiBoxLong(val ? ZoombiniText::kNotiBoxTransitionsOn : ZoombiniText::kNotiBoxTransitionsOff);
	}
}

void ZoombiniGameState::setLessActionEnabled(bool val) {
	_f._lessActionFlag = val ? 1 : 0;

	ZoombiniPage *page = _vm->getActivePage();
	if (page && page->getPageCategory() == ZoombiniPageCategory::kInteractive) {
		ZoombiniInteractive *interactive = dynamic_cast<ZoombiniInteractive *>(page);
		if (interactive)
			interactive->showNotiBoxLong(val ? ZoombiniText::kNotiBoxLessAction : ZoombiniText::kNotiBoxMoreAction);
	}
}

void ZoombiniGameState::setCursorVisible(bool val) {
	_flagCursorVisible = val;

	ZoombiniPage *page = _vm->getActivePage();
	if (page && page->getPageCategory() == ZoombiniPageCategory::kInteractive) {
		ZoombiniInteractive *interactive = dynamic_cast<ZoombiniInteractive *>(page);
		if (interactive)
			interactive->showNotiBoxLong(val ? ZoombiniText::kNotiBoxShowCursor : ZoombiniText::kNotiBoxHideCursor);
	}
}

void ZoombiniGameState::generateRandomPack() {
	// Generate 16 snoids with random traits (1-5 each).
	// IDA: puzzleRodMap_maybeOnClickPuzzleIcon_42A9D6 — practice mode Zoombini generation.
	ZmbStateActivePack &pack = _f._zmbPackActive;
	pack._wPackZmbCount = 16;
	pack._bSkipOccupiedAnim = 0;

	for (int16 i = 0; i < pack._wPackZmbCount; i++) {
		ZmbStateActiveEntry &entry = pack._entries[i];
		entry._traits._head = static_cast<uint8>(_vm->_rnd->getRandomNumber(1, 5));
		entry._traits._eye  = static_cast<uint8>(_vm->_rnd->getRandomNumber(1, 5));
		entry._traits._nose = static_cast<uint8>(_vm->_rnd->getRandomNumber(1, 5));
		entry._traits._foot = static_cast<uint8>(_vm->_rnd->getRandomNumber(1, 5));
		entry._bIsOccupied = 1;
		entry._name[0] = 0;
	}
}

} // End of namespace Mohawk
