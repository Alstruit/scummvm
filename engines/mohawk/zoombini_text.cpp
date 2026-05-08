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

#include "graphics/fontman.h"
#include "graphics/fonts/ttf.h"
#include "gui/message.h"

#include "common/algorithm.h"
#include "common/compression/installshieldv3_archive.h"
#include "common/config-manager.h"
#include "common/file.h"
#include "common/textconsole.h"

#include "mohawk/resource.h"
#include "mohawk/ttfloader.h"
#include "mohawk/zoombini.h"
#include "mohawk/zoombini_page.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_state.h"
#include "mohawk/zoombini_text.h"

namespace Mohawk {

struct ExeTextEntry {
	ZoombiniText::Key key;
	uint32 offset;
	uint16 length;
};

struct ExeCreditPointerRange {
	uint16 firstIndex;
	uint16 lastIndex;
};

// v1.x credits are stored as a flat run of null-terminated strings. The first
// visible credit line lets us locate that run in the executable bytes.
static const char *const EXE_CREDIT_PARAGRAPHS_ANCHOR = "PRODUCT CONCEPT AND DESIGN";

struct ExeTextSource {
	// Exactly one source path is active: either a loose executable file, or a
	// member inside an InstallShield V3 .Z archive.
	const char *fileName = nullptr;
	const char *archiveName = nullptr;
	const char *archiveMemberName = nullptr;

	// Keyed UI strings are read from fixed byte ranges in the executable.
	Common::CodePage codePage = Common::kWindows1252;
	Common::Array<ExeTextEntry> entries;
	int32 entryOffsetDelta = 0;

	// Explicit ScummVM-authored overrides that replace selected executable text
	// slices after loading. These do not depend on the built-in string tables.
	Common::HashMap<uint32, Common::U32String> textPatches;
	CreditLinePatchMap creditLinePatches;

	// Credits are either a null-terminated string sequence found from that known
	// first line, or a pointer table that maps executable addresses to strings.
	const char *creditAnchor = nullptr;
	uint32 creditPointerTableOffset = 0;
	uint32 creditPointerBaseAddress = 0;
	uint32 creditPointerBlankAddress = 0;
	Common::Array<ExeCreditPointerRange> creditPointerRanges;

	static ExeTextSource fromFile(const char *sourceFileName) {
		ExeTextSource source;
		source.fileName = sourceFileName;
		return source;
	}

	static ExeTextSource fromArchiveMember(const char *sourceArchiveName, const char *sourceArchiveMemberName) {
		ExeTextSource source;
		source.archiveName = sourceArchiveName;
		source.archiveMemberName = sourceArchiveMemberName;
		return source;
	}

	ExeTextSource withTextTable(Common::CodePage sourceCodePage, const Common::Array<ExeTextEntry> &sourceEntries) {
		codePage = sourceCodePage;
		entries = sourceEntries;
		return *this;
	}

	ExeTextSource withEntryOffsetDelta(int32 sourceEntryOffsetDelta) {
		entryOffsetDelta = sourceEntryOffsetDelta;
		return *this;
	}

	ExeTextSource withTextPatches(const Common::HashMap<uint32, Common::U32String> &sourceTextPatches) {
		textPatches = sourceTextPatches;
		return *this;
	}

	ExeTextSource withCreditLinePatches(const CreditLinePatchMap &sourceCreditLinePatches) {
		creditLinePatches = sourceCreditLinePatches;
		return *this;
	}

	ExeTextSource withCreditAnchor(const char *sourceCreditAnchor) {
		creditAnchor = sourceCreditAnchor;
		return *this;
	}

	ExeTextSource withCreditPointerTable(uint32 sourceCreditPointerTableOffset, uint32 sourceCreditPointerBaseAddress,
										 const Common::Array<ExeCreditPointerRange> &sourceCreditPointerRanges) {
		creditPointerTableOffset = sourceCreditPointerTableOffset;
		creditPointerBaseAddress = sourceCreditPointerBaseAddress;
		creditPointerRanges = sourceCreditPointerRanges;
		return *this;
	}

	ExeTextSource withCreditPointerBlankAddress(uint32 sourceCreditPointerBlankAddress) {
		creditPointerBlankAddress = sourceCreditPointerBlankAddress;
		return *this;
	}
};

Common::HashMap<uint32, Common::U32String> ZoombiniText::buildEnglishExeTextPatches() {
	Common::HashMap<uint32, Common::U32String> patches;
	return patches;
}

CreditLinePatchMap ZoombiniText::buildEnglishExeCreditLinePatches() {
	CreditLinePatchMap patches;
	return patches;
}

CreditLinePatchMap ZoombiniText::buildKoreanExeCreditLinePatches() {
	CreditLinePatchMap patches;

	// Original: "이민선(프로그래머", omitted the closing parenthesis.
	patches[CreditLineAddress(36, 19)] = U"이민선(프로그래머)";

	// Original engine could not draw the 'Ø' character in "BrØderbund" which the byte encoded with Windows-1252 was kept.
	// ScummVM detects that and decodes only that string with Windows-1252.

	return patches;
}

static const int32 kEnglish11Win16TextOffsetDelta = 0x511B6;
static const int32 kKorean11Win16TextOffsetDelta = -0x134EC;

static const Common::Array<ExeTextEntry> kEnglish11ExeTextEntries = {
	{ZoombiniText::kTown, 0x8AD89, 13},
	{ZoombiniText::kPicker, 0x8ACA4, 13},
	{ZoombiniText::kBridge, 0x8ACB2, 15},
	{ZoombiniText::kTunnels, 0x8ACC2, 16},
	{ZoombiniText::kPizza, 0x8ACD3, 10},
	{ZoombiniText::kBasecamp1, 0x8ACDE, 12},
	{ZoombiniText::kFerry, 0x8ACEB, 25},
	{ZoombiniText::kLilly, 0x8AD05, 22},
	{ZoombiniText::kSlides, 0x8AD1C, 10},
	{ZoombiniText::kFleens, 0x8AD27, 7},
	{ZoombiniText::kHotel, 0x8AD2F, 14},
	{ZoombiniText::kNet, 0x8AD3E, 12},
	{ZoombiniText::kBasecamp2, 0x8AD4B, 10},
	{ZoombiniText::kCaves, 0x8AD56, 15},
	{ZoombiniText::kSmoke, 0x8AD66, 14},
	{ZoombiniText::kMaze, 0x8AD76, 18},
	{ZoombiniText::kNewGame, 0x8B9FC, 8},
	{ZoombiniText::kPracticeMode, 0x8ADCB, 13},
	{ZoombiniText::kContinueJourney, 0x8AEF3, 16},
	{ZoombiniText::kPracticeTitle, 0x8AEE5, 13},
	{ZoombiniText::kPracticeDesc1, 0x8ADD9, 18},
	{ZoombiniText::kPracticeDesc2, 0x8ADEC, 17},
	{ZoombiniText::kPracticeDesc3, 0x8ADFE, 16},
	{ZoombiniText::kPracticeDesc4, 0x8AE0F, 12},
	{ZoombiniText::kTerrainKey, 0x8AF04, 11},
	{ZoombiniText::kChooseLevel, 0x8AF10, 14},
	{ZoombiniText::kLevel1, 0x8AF1F, 11},
	{ZoombiniText::kLevel2, 0x8AF2B, 11},
	{ZoombiniText::kLevel3, 0x8AF37, 9},
	{ZoombiniText::kLevel4, 0x8AF41, 15},
	{ZoombiniText::kRoute1, 0x8AF51, 31},
	{ZoombiniText::kRoute2, 0x8AF71, 11},
	{ZoombiniText::kRoute3, 0x8AF7D, 17},
	{ZoombiniText::kRoute4, 0x8AF8F, 20},
	{ZoombiniText::kXferVillePopulation, 0x8AFA4, 24},
	{ZoombiniText::kMemorialJanuary, 0x8AFBE, 7},
	{ZoombiniText::kMemorialFebruary, 0x8AFC6, 8},
	{ZoombiniText::kMemorialMarch, 0x8AFCF, 5},
	{ZoombiniText::kMemorialApril, 0x8AFD5, 5},
	{ZoombiniText::kMemorialMay, 0x8AFDB, 3},
	{ZoombiniText::kMemorialJune, 0x8AFDF, 4},
	{ZoombiniText::kMemorialJuly, 0x8AFE4, 4},
	{ZoombiniText::kMemorialAugust, 0x8AFE9, 6},
	{ZoombiniText::kMemorialSeptember, 0x8AFF0, 9},
	{ZoombiniText::kMemorialOctober, 0x8AFFA, 7},
	{ZoombiniText::kMemorialNovember, 0x8B002, 8},
	{ZoombiniText::kMemorialDecember, 0x8B00B, 8},
	{ZoombiniText::kMemorialWhenLevel, 0x8B014, 18},
	{ZoombiniText::kMemorialHonorMonument, 0x8B027, 50},
	{ZoombiniText::kMemorialHonorWindmill, 0x8B05A, 53},
	{ZoombiniText::kMemorialHonorObservatory, 0x8B090, 44},
	{ZoombiniText::kMemorialHonorBowlingAlley, 0x8B0BD, 44},
	{ZoombiniText::kMemorialHonorGeneralStore, 0x8B0EA, 53},
	{ZoombiniText::kMemorialHonorSwimmingPool, 0x8B120, 45},
	{ZoombiniText::kMemorialHonorPlayground, 0x8B14E, 55},
	{ZoombiniText::kMemorialHonorBandShell, 0x8B186, 53},
	{ZoombiniText::kMemorialHonorSchool, 0x8B1BC, 43},
	{ZoombiniText::kMemorialHonorLibrary, 0x8B1E8, 51},
	{ZoombiniText::kMemorialHonorFire, 0x8B21C, 40},
	{ZoombiniText::kMemorialHonorOpera, 0x8B245, 52},
	{ZoombiniText::kMemorialHonorCityHall, 0x8B27A, 44},
	{ZoombiniText::kMemorialHonorClockTower, 0x8B2A7, 55},
	{ZoombiniText::kMemorialHonorMuseum, 0x8B2DF, 54},
	{ZoombiniText::kMemorialHonorCourt, 0x8B316, 54},
	{ZoombiniText::kMemorialRoute1Level1, 0x8B34D, 100},
	{ZoombiniText::kMemorialRoute1Level2, 0x8B3B2, 106},
	{ZoombiniText::kMemorialRoute1Level3, 0x8B41D, 102},
	{ZoombiniText::kMemorialRoute1Level4, 0x8B484, 113},
	{ZoombiniText::kMemorialRoute2Level1, 0x8B4F6, 66},
	{ZoombiniText::kMemorialRoute2Level2, 0x8B539, 81},
	{ZoombiniText::kMemorialRoute2Level3, 0x8B58B, 92},
	{ZoombiniText::kMemorialRoute2Level4, 0x8B4F6, 66},
	{ZoombiniText::kMemorialRoute3Level1, 0x8B62B, 106},
	{ZoombiniText::kMemorialRoute3Level2, 0x8B696, 93},
	{ZoombiniText::kMemorialRoute3Level3, 0x8B6F4, 94},
	{ZoombiniText::kMemorialRoute3Level4, 0x8B753, 98},
	{ZoombiniText::kMemorialRoute4Level1, 0x8B7B6, 103},
	{ZoombiniText::kMemorialRoute4Level2, 0x8B81E, 110},
	{ZoombiniText::kMemorialRoute4Level3, 0x8B88D, 91},
	{ZoombiniText::kMemorialRoute4Level4, 0x8B8E9, 134},
	{ZoombiniText::kDialogBodyGoMapWillLost, 0x8B970, 64},
	{ZoombiniText::kDialogButtonLoseThem, 0x8B9B1, 9},
	{ZoombiniText::kDialogButtonKeepThem, 0x8B9BB, 9},
	{ZoombiniText::kDialogButtonOkay, 0x8B9C5, 2},
	{ZoombiniText::kDialogButtonCancel, 0x8B9C8, 6},
	{ZoombiniText::kDialogButtonYes, 0x8BB76, 3},
	{ZoombiniText::kDialogButtonNo, 0x8BBDB, 2},
	{ZoombiniText::kDialogButtonQuit, 0x8BA34, 4},
	{ZoombiniText::kDialogButtonLoad, 0x8B9CF, 4},
	{ZoombiniText::kDialogButtonSave, 0x8B9D4, 4},
	{ZoombiniText::kOptionsTitle, 0x8B9D9, 7},
	{ZoombiniText::kOptionsLegendOn, 0x8B9E1, 4},
	{ZoombiniText::kOptionsLegendOff, 0x8B9E6, 5},
	{ZoombiniText::kOptionsNewGame, 0x8B9FC, 17},
	{ZoombiniText::kOptionsLoadGame, 0x8BA0E, 18},
	{ZoombiniText::kOptionsSaveGame, 0x8BA21, 18},
	{ZoombiniText::kOptionsQuit, 0x8BA34, 13},
	{ZoombiniText::kOptionsToggle, 0x8B9EC, 15},
	{ZoombiniText::kOptionsSound, 0x8BA42, 26},
	{ZoombiniText::kOptionsMusic, 0x8BA5D, 25},
	{ZoombiniText::kOptionsStickyMouse, 0x8BA77, 21},
	{ZoombiniText::kOptionsTransitions, 0x8BA8D, 20},
	{ZoombiniText::kOptionsCredits, 0x8BB56, 7},
	{ZoombiniText::kDialogBodyNoSavedGames, 0x8BAA2, 14},
	{ZoombiniText::kDialogBodyCreateAndSaveNewGame, 0x8BAB4, 56},
	{ZoombiniText::kDialogButtonNewGame, 0x8B9FC, 8},
	{ZoombiniText::kDialogButtonReplaceTitle, 0x8BAF6, 7},
	{ZoombiniText::kDialogTitleSave, 0x8BAFE, 11},
	{ZoombiniText::kDialogTitleSaveAs, 0x8BB0A, 13},
	{ZoombiniText::kDialogTitleLoad, 0x8BB18, 11},
	{ZoombiniText::kDialogBodyReplaceGame, 0x8BB24, 48},
	{ZoombiniText::kDialogBodySaveCurrentGame, 0x8BB5E, 23},
	{ZoombiniText::kDialogBodySaveDirtyGame, 0x8BCFD, 60},
	{ZoombiniText::kDialogBodyCannotSaveInPractice, 0x8BBDE, 42},
	{ZoombiniText::kDialogBodyCreateNewGame, 0x8BC09, 42},
	{ZoombiniText::kDialogBodyCannotSaveMoreGame, 0x8BC34, 70},
	{ZoombiniText::kDialogBodyCannotLoadInPractice, 0x8BC7B, 42},
	{ZoombiniText::kDialogBodyCannotCreateNewInPractice, 0x8BCA6, 48},
	{ZoombiniText::kDialogBodyNewGame, 0x8B9FC, 8},
	{ZoombiniText::kDialogBodyReallyQuit, 0x8BCE0, 28},
	{ZoombiniText::kDialogBodySaveBeforeQuit, 0x8BB7F, 87},
	{ZoombiniText::kDialogHelpTitle, 0x8BD3A, 4},
	{ZoombiniText::kDialogButtonPrev, 0x8BD3F, 8},
	{ZoombiniText::kDialogButtonNext, 0x8BD48, 4},
	{ZoombiniText::kDialogHelpLevel, 0x8636C, 5},
	{ZoombiniText::kNotiBoxMusicOn, 0x8C77B, 8},
	{ZoombiniText::kNotiBoxMusicOff, 0x8C784, 9},
	{ZoombiniText::kNotiBoxSoundOn, 0x8C78E, 8},
	{ZoombiniText::kNotiBoxSoundOff, 0x8C797, 9},
	{ZoombiniText::kNotiBoxLessAction, 0x8C7A1, 11},
	{ZoombiniText::kNotiBoxMoreAction, 0x8C7AD, 11},
	{ZoombiniText::kNotiBoxHideCursor, 0x8C7B9, 11},
	{ZoombiniText::kNotiBoxShowCursor, 0x8C7C5, 11},
	{ZoombiniText::kNotiBoxStickeyMouse, 0x8C7D1, 12},
	{ZoombiniText::kNotiBoxNonStickeyMouse, 0x8C7DE, 16},
	{ZoombiniText::kNotiBoxTransitionsOn, 0x8C7EF, 14},
	{ZoombiniText::kNotiBoxTransitionsOff, 0x8C7FE, 15},
	{ZoombiniText::kNotiBoxAutoStickeyOn, 0x8C80E, 14},
	{ZoombiniText::kNotiBoxAutoStickeyOff, 0x8C81D, 15},
};

static const Common::Array<ExeTextEntry> kEnglish20ExeTextEntries = {
	{ZoombiniText::kTown, 0x92680, 13},
	{ZoombiniText::kPicker, 0x928C4, 13},
	{ZoombiniText::kBridge, 0x92264, 15},
	{ZoombiniText::kTunnels, 0x92283, 16},
	{ZoombiniText::kPizza, 0x92894, 10},
	{ZoombiniText::kBasecamp1, 0x92884, 12},
	{ZoombiniText::kFerry, 0x92868, 25},
	{ZoombiniText::kLilly, 0x92850, 22},
	{ZoombiniText::kSlides, 0x92844, 10},
	{ZoombiniText::kFleens, 0x9283C, 7},
	{ZoombiniText::kHotel, 0x91F72, 14},
	{ZoombiniText::kNet, 0x91ED6, 12},
	{ZoombiniText::kBasecamp2, 0x92810, 10},
	{ZoombiniText::kCaves, 0x92800, 15},
	{ZoombiniText::kSmoke, 0x91DEE, 14},
	{ZoombiniText::kMaze, 0x91E75, 18},
	{ZoombiniText::kNewGame, 0x919F0, 8},
	{ZoombiniText::kPracticeMode, 0x927BC, 13},
	{ZoombiniText::kContinueJourney, 0x9274C, 16},
	{ZoombiniText::kPracticeTitle, 0x92760, 13},
	{ZoombiniText::kPracticeDesc1, 0x927A8, 18},
	{ZoombiniText::kPracticeDesc2, 0x92794, 17},
	{ZoombiniText::kPracticeDesc3, 0x92780, 16},
	{ZoombiniText::kPracticeDesc4, 0x92770, 12},
	{ZoombiniText::kTerrainKey, 0x92740, 11},
	{ZoombiniText::kChooseLevel, 0x92730, 14},
	{ZoombiniText::kLevel1, 0x92724, 11},
	{ZoombiniText::kLevel2, 0x92718, 11},
	{ZoombiniText::kLevel3, 0x92702, 9},
	{ZoombiniText::kLevel4, 0x926FC, 15},
	{ZoombiniText::kRoute1, 0x926DC, 31},
	{ZoombiniText::kRoute2, 0x926D0, 11},
	{ZoombiniText::kRoute3, 0x926BC, 17},
	{ZoombiniText::kRoute4, 0x926A4, 20},
	{ZoombiniText::kXferVillePopulation, 0x92680, 24},
	{ZoombiniText::kMemorialJanuary, 0x92678, 7},
	{ZoombiniText::kMemorialFebruary, 0x9266C, 8},
	{ZoombiniText::kMemorialMarch, 0x92664, 5},
	{ZoombiniText::kMemorialApril, 0x9265C, 5},
	{ZoombiniText::kMemorialMay, 0x92658, 3},
	{ZoombiniText::kMemorialJune, 0x92650, 4},
	{ZoombiniText::kMemorialJuly, 0x92648, 4},
	{ZoombiniText::kMemorialAugust, 0x92640, 6},
	{ZoombiniText::kMemorialSeptember, 0x92634, 9},
	{ZoombiniText::kMemorialOctober, 0x9262C, 7},
	{ZoombiniText::kMemorialNovember, 0x92620, 8},
	{ZoombiniText::kMemorialDecember, 0x92614, 8},
	{ZoombiniText::kMemorialWhenLevel, 0x92600, 18},
	{ZoombiniText::kMemorialHonorMonument, 0x925CC, 50},
	{ZoombiniText::kMemorialHonorWindmill, 0x92594, 53},
	{ZoombiniText::kMemorialHonorObservatory, 0x92564, 44},
	{ZoombiniText::kMemorialHonorBowlingAlley, 0x92534, 44},
	{ZoombiniText::kMemorialHonorGeneralStore, 0x924FC, 53},
	{ZoombiniText::kMemorialHonorSwimmingPool, 0x924CC, 45},
	{ZoombiniText::kMemorialHonorPlayground, 0x92494, 55},
	{ZoombiniText::kMemorialHonorBandShell, 0x9245C, 53},
	{ZoombiniText::kMemorialHonorSchool, 0x92430, 43},
	{ZoombiniText::kMemorialHonorLibrary, 0x923FC, 51},
	{ZoombiniText::kMemorialHonorFire, 0x923D0, 40},
	{ZoombiniText::kMemorialHonorOpera, 0x92398, 52},
	{ZoombiniText::kMemorialHonorCityHall, 0x92368, 44},
	{ZoombiniText::kMemorialHonorClockTower, 0x92330, 55},
	{ZoombiniText::kMemorialHonorMuseum, 0x922F8, 54},
	{ZoombiniText::kMemorialHonorCourt, 0x922C0, 54},
	{ZoombiniText::kMemorialRoute1Level1, 0x92258, 100},
	{ZoombiniText::kMemorialRoute1Level2, 0x921EC, 106},
	{ZoombiniText::kMemorialRoute1Level3, 0x92184, 102},
	{ZoombiniText::kMemorialRoute1Level4, 0x92110, 113},
	{ZoombiniText::kMemorialRoute2Level1, 0x920CC, 66},
	{ZoombiniText::kMemorialRoute2Level2, 0x92078, 81},
	{ZoombiniText::kMemorialRoute2Level3, 0x92018, 92},
	{ZoombiniText::kMemorialRoute2Level4, 0x920CC, 66},
	{ZoombiniText::kMemorialRoute3Level1, 0x91FAC, 106},
	{ZoombiniText::kMemorialRoute3Level2, 0x91F4C, 93},
	{ZoombiniText::kMemorialRoute3Level3, 0x91EEC, 94},
	{ZoombiniText::kMemorialRoute3Level4, 0x91E88, 98},
	{ZoombiniText::kMemorialRoute4Level1, 0x91E20, 103},
	{ZoombiniText::kMemorialRoute4Level2, 0x91DB0, 110},
	{ZoombiniText::kMemorialRoute4Level3, 0x91D54, 91},
	{ZoombiniText::kMemorialRoute4Level4, 0x91CCC, 134},
	{ZoombiniText::kDialogBodyGoMapWillLost, 0x91C88, 64},
	{ZoombiniText::kDialogButtonLoseThem, 0x91C7C, 9},
	{ZoombiniText::kDialogButtonKeepThem, 0x91C70, 9},
	{ZoombiniText::kDialogButtonOkay, 0x91C6C, 2},
	{ZoombiniText::kDialogButtonCancel, 0x91C64, 6},
	{ZoombiniText::kDialogButtonYes, 0x91A8C, 3},
	{ZoombiniText::kDialogButtonNo, 0x91A28, 2},
	{ZoombiniText::kDialogButtonQuit, 0x9191E, 4},
	{ZoombiniText::kDialogButtonLoad, 0x91AE4, 4},
	{ZoombiniText::kDialogButtonSave, 0x91AF0, 4},
	{ZoombiniText::kOptionsTitle, 0x91C40, 7},
	{ZoombiniText::kOptionsLegendOn, 0x91C38, 4},
	{ZoombiniText::kOptionsLegendOff, 0x91C30, 5},
	{ZoombiniText::kOptionsNewGame, 0x91C0C, 17},
	{ZoombiniText::kOptionsLoadGame, 0x91BF8, 18},
	{ZoombiniText::kOptionsSaveGame, 0x91BE4, 18},
	{ZoombiniText::kOptionsQuit, 0x91BD4, 13},
	{ZoombiniText::kOptionsToggle, 0x91C20, 15},
	{ZoombiniText::kOptionsSound, 0x91BB8, 26},
	{ZoombiniText::kOptionsMusic, 0x91B9C, 25},
	{ZoombiniText::kOptionsStickyMouse, 0x91B84, 21},
	{ZoombiniText::kOptionsTransitions, 0x91B6C, 20},
	{ZoombiniText::kOptionsCredits, 0x91AA8, 7},
	{ZoombiniText::kDialogBodyNoSavedGames, 0x91B5C, 14},
	{ZoombiniText::kDialogBodyCreateAndSaveNewGame, 0x91B20, 56},
	{ZoombiniText::kDialogButtonNewGame, 0x919F0, 8},
	{ZoombiniText::kDialogButtonReplaceTitle, 0x91B0C, 7},
	{ZoombiniText::kDialogTitleSave, 0x91B00, 11},
	{ZoombiniText::kDialogTitleSaveAs, 0x91AF0, 13},
	{ZoombiniText::kDialogTitleLoad, 0x91AE4, 11},
	{ZoombiniText::kDialogBodyReplaceGame, 0x91AB0, 48},
	{ZoombiniText::kDialogBodySaveCurrentGame, 0x91A90, 23},
	{ZoombiniText::kDialogBodySaveDirtyGame, 0x918C8, 60},
	{ZoombiniText::kDialogBodyCannotSaveInPractice, 0x919FC, 42},
	{ZoombiniText::kDialogBodyCreateNewGame, 0x919D0, 42},
	{ZoombiniText::kDialogBodyCannotSaveMoreGame, 0x91988, 70},
	{ZoombiniText::kDialogBodyCannotLoadInPractice, 0x9195C, 42},
	{ZoombiniText::kDialogBodyCannotCreateNewInPractice, 0x91928, 48},
	{ZoombiniText::kDialogBodyNewGame, 0x919F0, 8},
	{ZoombiniText::kDialogBodyReallyQuit, 0x91908, 28},
	{ZoombiniText::kDialogBodySaveBeforeQuit, 0x91A2C, 87},
	{ZoombiniText::kDialogHelpTitle, 0x91874, 4},
	{ZoombiniText::kDialogButtonPrev, 0x918B4, 8},
	{ZoombiniText::kDialogHelpLevel, 0x8B9D0, 5},
	{ZoombiniText::kNotiBoxMusicOn, 0x907B4, 8},
	{ZoombiniText::kNotiBoxMusicOff, 0x907A8, 9},
	{ZoombiniText::kNotiBoxSoundOn, 0x9079C, 8},
	{ZoombiniText::kNotiBoxSoundOff, 0x90790, 9},
	{ZoombiniText::kNotiBoxLessAction, 0x90784, 11},
	{ZoombiniText::kNotiBoxMoreAction, 0x90778, 11},
	{ZoombiniText::kNotiBoxHideCursor, 0x9076C, 11},
	{ZoombiniText::kNotiBoxShowCursor, 0x90760, 11},
	{ZoombiniText::kNotiBoxStickeyMouse, 0x90740, 12},
	{ZoombiniText::kNotiBoxNonStickeyMouse, 0x9073C, 16},
	{ZoombiniText::kNotiBoxTransitionsOn, 0x9072C, 14},
	{ZoombiniText::kNotiBoxTransitionsOff, 0x9071C, 15},
	{ZoombiniText::kNotiBoxAutoStickeyOn, 0x9070C, 14},
	{ZoombiniText::kNotiBoxAutoStickeyOff, 0x906FC, 15},
	{ZoombiniText::kOptionsTitle, 0x91C40, 17},
	{ZoombiniText::kOptionsHelpAudio, 0x91874, 19},
	{ZoombiniText::kOptionsTouchSense, 0x91888, 30},
	{ZoombiniText::kDialogHelpTitle, 0x918C0, 4},
	{ZoombiniText::kDialogButtonOkay, 0x918A8, 2},
	{ZoombiniText::kDialogButtonNext, 0x918AC, 4},
	{ZoombiniText::kDialogBodyRemoveGame, 0x91848, 42},
	{ZoombiniText::kNotiBoxHelpAudioOn, 0x906CC, 13},
	{ZoombiniText::kNotiBoxHelpAudioOff, 0x906BC, 14},
	{ZoombiniText::kNotiBoxTouchSenseOn, 0x906EC, 14},
	{ZoombiniText::kNotiBoxTouchSenseOff, 0x906DC, 15},
};

static const Common::Array<ExeTextEntry> kKorean11ExeTextEntries = {
	{ZoombiniText::kPicker, 0x8B8EC, 9},
	{ZoombiniText::kBridge, 0x8B8F6, 13},
	{ZoombiniText::kCaves, 0x8B97D, 9},
	{ZoombiniText::kPizza, 0x8B915, 9},
	{ZoombiniText::kBasecamp1, 0x8B91F, 13},
	{ZoombiniText::kFerry, 0x8B92D, 9},
	{ZoombiniText::kLilly, 0x8B937, 11},
	{ZoombiniText::kSlides, 0x8B943, 9},
	{ZoombiniText::kFleens, 0x8B94D, 11},
	{ZoombiniText::kHotel, 0x8B959, 9},
	{ZoombiniText::kNet, 0x8B963, 11},
	{ZoombiniText::kBasecamp2, 0x8B96F, 13},
	{ZoombiniText::kTunnels, 0x8B904, 16},
	{ZoombiniText::kSmoke, 0x8B987, 9},
	{ZoombiniText::kMaze, 0x8B991, 11},
	{ZoombiniText::kTown, 0x8B99D, 11},
	{ZoombiniText::kNewGame, 0x8A866, 7},
	{ZoombiniText::kPracticeMode, 0x8BAFD, 11},
	{ZoombiniText::kContinueJourney, 0x8BB09, 14},
	{ZoombiniText::kPracticeTitle, 0x8B9DB, 9},
	{ZoombiniText::kPracticeDesc1, 0x8B9E5, 15},
	{ZoombiniText::kPracticeDesc2, 0x8B9F5, 16},
	{ZoombiniText::kPracticeDesc3, 0x8BA06, 18},
	{ZoombiniText::kPracticeDesc4, 0x8BA19, 17},
	{ZoombiniText::kTerrainKey, 0x8BB18, 11},
	{ZoombiniText::kChooseLevel, 0x8BB24, 11},
	{ZoombiniText::kLevel1, 0x8BB30, 5},
	{ZoombiniText::kLevel2, 0x8BB36, 5},
	{ZoombiniText::kLevel3, 0x8BB3C, 5},
	{ZoombiniText::kLevel4, 0x8BB42, 5},
	{ZoombiniText::kRoute1, 0x8BB48, 11},
	{ZoombiniText::kRoute2, 0x8BB54, 11},
	{ZoombiniText::kRoute3, 0x8BB60, 16},
	{ZoombiniText::kRoute4, 0x8BB71, 11},
	{ZoombiniText::kXferVillePopulation, 0x8BB7D, 17},
	{ZoombiniText::kMemorialJanuary, 0x8BB8F, 3},
	{ZoombiniText::kMemorialFebruary, 0x8BB93, 3},
	{ZoombiniText::kMemorialMarch, 0x8BB97, 3},
	{ZoombiniText::kMemorialApril, 0x8BB9B, 3},
	{ZoombiniText::kMemorialMay, 0x8BB9F, 3},
	{ZoombiniText::kMemorialJune, 0x8BBA3, 3},
	{ZoombiniText::kMemorialJuly, 0x8BBA7, 3},
	{ZoombiniText::kMemorialAugust, 0x8BBAB, 3},
	{ZoombiniText::kMemorialSeptember, 0x8BBAF, 3},
	{ZoombiniText::kMemorialOctober, 0x8BBB3, 4},
	{ZoombiniText::kMemorialNovember, 0x8BBB8, 4},
	{ZoombiniText::kMemorialDecember, 0x8BBBD, 4},
	{ZoombiniText::kMemorialWhenLevel, 0x8BBC2, 26},
	{ZoombiniText::kMemorialHonorMonument, 0x8BBDD, 53},
	{ZoombiniText::kMemorialHonorWindmill, 0x8BC13, 51},
	{ZoombiniText::kMemorialHonorObservatory, 0x8BC47, 53},
	{ZoombiniText::kMemorialHonorBowlingAlley, 0x8BC7D, 53},
	{ZoombiniText::kMemorialHonorGeneralStore, 0x8BCB3, 51},
	{ZoombiniText::kMemorialHonorSwimmingPool, 0x8BCE7, 53},
	{ZoombiniText::kMemorialHonorPlayground, 0x8BD1D, 53},
	{ZoombiniText::kMemorialHonorBandShell, 0x8BD53, 53},
	{ZoombiniText::kMemorialHonorSchool, 0x8BD89, 51},
	{ZoombiniText::kMemorialHonorLibrary, 0x8BDBD, 53},
	{ZoombiniText::kMemorialHonorFire, 0x8BDF3, 53},
	{ZoombiniText::kMemorialHonorOpera, 0x8BE29, 60},
	{ZoombiniText::kMemorialHonorCityHall, 0x8BE66, 51},
	{ZoombiniText::kMemorialHonorClockTower, 0x8BE9A, 53},
	{ZoombiniText::kMemorialHonorMuseum, 0x8BED0, 53},
	{ZoombiniText::kMemorialHonorCourt, 0x8BF06, 51},
	{ZoombiniText::kMemorialRoute1Level1, 0x8BF3A, 109},
	{ZoombiniText::kMemorialRoute1Level2, 0x8BFA8, 109},
	{ZoombiniText::kMemorialRoute1Level3, 0x8C016, 124},
	{ZoombiniText::kMemorialRoute1Level4, 0x8C093, 109},
	{ZoombiniText::kMemorialRoute2Level1, 0x8C22C, 91},
	{ZoombiniText::kMemorialRoute2Level2, 0x8C15C, 88},
	{ZoombiniText::kMemorialRoute2Level3, 0x8C1B5, 118},
	{ZoombiniText::kMemorialRoute2Level4, 0x8C22C, 91},
	{ZoombiniText::kMemorialRoute3Level1, 0x8C288, 138},
	{ZoombiniText::kMemorialRoute3Level2, 0x8C313, 118},
	{ZoombiniText::kMemorialRoute3Level3, 0x8C38A, 100},
	{ZoombiniText::kMemorialRoute3Level4, 0x8C3EF, 107},
	{ZoombiniText::kMemorialRoute4Level1, 0x8C45B, 103},
	{ZoombiniText::kMemorialRoute4Level2, 0x8C4C3, 118},
	{ZoombiniText::kMemorialRoute4Level3, 0x8C53A, 117},
	{ZoombiniText::kMemorialRoute4Level4, 0x8C5B0, 116},
	{ZoombiniText::kDialogBodyGoMapWillLost, 0x8C625, 52},
	{ZoombiniText::kDialogButtonLoseThem, 0x8C65A, 9},
	{ZoombiniText::kDialogButtonKeepThem, 0x8C664, 10},
	{ZoombiniText::kDialogButtonOkay, 0x8C66F, 4},
	{ZoombiniText::kDialogButtonCancel, 0x8C674, 4},
	{ZoombiniText::kDialogButtonYes, 0x8C82C, 2},
	{ZoombiniText::kDialogButtonNo, 0x8C82F, 6},
	{ZoombiniText::kDialogButtonQuit, 0x8C82F, 6},
	{ZoombiniText::kDialogButtonLoad, 0x8C679, 8},
	{ZoombiniText::kDialogButtonSave, 0x8C682, 8},
	{ZoombiniText::kOptionsTitle, 0x8C68B, 4},
	{ZoombiniText::kOptionsLegendOn, 0x8C690, 4},
	{ZoombiniText::kOptionsLegendOff, 0x8C695, 4},
	{ZoombiniText::kOptionsToggle, 0x8C69A, 7},
	{ZoombiniText::kOptionsNewGame, 0x8C6A2, 18},
	{ZoombiniText::kOptionsLoadGame, 0x8C6B5, 17},
	{ZoombiniText::kOptionsSaveGame, 0x8C6C7, 17},
	{ZoombiniText::kOptionsQuit, 0x8C6D9, 15},
	{ZoombiniText::kOptionsSound, 0x8C6E9, 18},
	{ZoombiniText::kOptionsMusic, 0x8C6FC, 18},
	{ZoombiniText::kOptionsStickyMouse, 0x8C70F, 22},
	{ZoombiniText::kOptionsTransitions, 0x8C726, 18},
	{ZoombiniText::kOptionsCredits, 0x8C803, 10},
	{ZoombiniText::kDialogBodyNoSavedGames, 0x8C739, 23},
	{ZoombiniText::kDialogBodyCreateAndSaveNewGame, 0x8C756, 58},
	{ZoombiniText::kDialogButtonNewGame, 0x8C791, 11},
	{ZoombiniText::kDialogButtonReplaceTitle, 0x8C79D, 8},
	{ZoombiniText::kDialogTitleSave, 0x8C682, 8},
	{ZoombiniText::kDialogTitleSaveAs, 0x8C7AF, 18},
	{ZoombiniText::kDialogTitleLoad, 0x8C679, 8},
	{ZoombiniText::kDialogBodyReplaceGame, 0x8C7CB, 55},
	{ZoombiniText::kDialogBodySaveCurrentGame, 0x8C80E, 29},
	{ZoombiniText::kDialogBodySaveDirtyGame, 0x8C967, 62},
	{ZoombiniText::kDialogBodyCannotSaveInPractice, 0x8C884, 41},
	{ZoombiniText::kDialogBodyCreateNewGame, 0x8C8AE, 25},
	{ZoombiniText::kDialogBodyCannotSaveMoreGame, 0x8C8C8, 35},
	{ZoombiniText::kDialogBodyCannotLoadInPractice, 0x8C8EC, 48},
	{ZoombiniText::kDialogBodyCannotCreateNewInPractice, 0x8C91D, 44},
	{ZoombiniText::kDialogBodyNewGame, 0x8C8AE, 7},
	{ZoombiniText::kDialogBodyReallyQuit, 0x8C952, 20},
	{ZoombiniText::kDialogBodySaveBeforeQuit, 0x8C836, 67},
	{ZoombiniText::kDialogHelpTitle, 0x8C9A6, 6},
	{ZoombiniText::kDialogButtonPrev, 0x8C9AD, 4},
	{ZoombiniText::kDialogButtonNext, 0x8C9B2, 4},
	{ZoombiniText::kDialogHelpLevel, 0x8B9F6, 4},
	{ZoombiniText::kNotiBoxMusicOn, 0x8D4AB, 12},
	{ZoombiniText::kNotiBoxMusicOff, 0x8D4B8, 12},
	{ZoombiniText::kNotiBoxSoundOn, 0x8D4C5, 12},
	{ZoombiniText::kNotiBoxSoundOff, 0x8D4D2, 12},
	{ZoombiniText::kNotiBoxLessAction, 0x8D4DF, 11},
	{ZoombiniText::kNotiBoxMoreAction, 0x8D4EB, 11},
	{ZoombiniText::kNotiBoxHideCursor, 0x8D4F7, 11},
	{ZoombiniText::kNotiBoxShowCursor, 0x8D503, 11},
	{ZoombiniText::kNotiBoxStickeyMouse, 0x8D50F, 13},
	{ZoombiniText::kNotiBoxNonStickeyMouse, 0x8D51D, 11},
	{ZoombiniText::kNotiBoxTransitionsOn, 0x8D529, 11},
	{ZoombiniText::kNotiBoxTransitionsOff, 0x8D535, 11},
	{ZoombiniText::kNotiBoxAutoStickeyOn, 0x8D541, 16},
	{ZoombiniText::kNotiBoxAutoStickeyOff, 0x8D552, 16},
};

static const Common::Array<ExeCreditPointerRange> kEnglish20CreditPointerRanges = {
	{21, 241},
	{255, 332},
	{333, 358}};

bool ZoombiniText::readExecutableData(Common::SeekableReadStream *exeStream, Common::Array<byte> &data) {
	int64 exeSize = exeStream->size();
	if (exeSize < 1 || static_cast<int64>(UINT32_MAX) < exeSize)
		return false;

	data.resize(static_cast<uint32>(exeSize));
	return exeStream->seek(0) && exeStream->read(data.data(), static_cast<uint32>(data.size())) == data.size();
}

int64 ZoombiniText::getExeTextEntryOffset(const ExeTextSource &source, const ExeTextEntry &entry) {
	if (source.entryOffsetDelta == kEnglish11Win16TextOffsetDelta && entry.key == ZoombiniText::kDialogHelpLevel)
		return 0xD9E9C;
	if (source.entryOffsetDelta == kKorean11Win16TextOffsetDelta && entry.key == ZoombiniText::kNewGame)
		return 0x793C2;
	return static_cast<int64>(entry.offset) + source.entryOffsetDelta;
}

bool ZoombiniText::findBytes(const Common::Array<byte> &data, const char *needle, uint32 &offset) {
	const uint32 needleLength = static_cast<uint32>(strlen(needle));
	if (needleLength < 1 || data.size() < needleLength)
		return false;

	for (uint32 byteIndex = 0; byteIndex <= data.size() - needleLength; byteIndex++) {
		if (memcmp(data.data() + byteIndex, needle, needleLength) == 0) {
			offset = byteIndex;
			return true;
		}
	}
	return false;
}

CreditParagraphSplitMap ZoombiniText::buildCreditParagraphSplits() {
	CreditParagraphSplitMap patches;
	patches[CreditLineAddress(31, 14)] = 1; // blankLineCount = 1 before that address
	return patches;
}

Common::U32String ZoombiniText::decodeCreditStringBytes(const byte *bytes, uint32 length, Common::CodePage codePage) {
	Common::CodePage decodeCodePage = codePage;
	if (codePage == Common::kWindows949) {
		uint32 extendedByteCount = 0;
		for (uint32 byteIndex = 0; byteIndex < length; byteIndex++) {
			if (0x80 <= bytes[byteIndex])
				extendedByteCount++;
		}

		// If there are just one extended bytes, then the text is likely Windows-1252 - Ex) 'Ø' (0xD8) of the 'BrØderbund'.
		// In that case, always decode that string with Windows-1252.
		if (extendedByteCount <= 1)
			decodeCodePage = Common::kWindows1252;
	}

	return Common::U32String(reinterpret_cast<const char *>(bytes), length, decodeCodePage);
}

bool ZoombiniText::readExecutableStringAt(const Common::Array<byte> &data, uint32 offset, Common::CodePage codePage, Common::U32String &text) {
	if (data.size() <= offset)
		return false;

	uint32 endOffset = offset;
	while (endOffset < data.size() && data[endOffset] != 0)
		endOffset++;
	if (data.size() <= endOffset)
		return false;

	text = decodeCreditStringBytes(data.data() + offset, endOffset - offset, codePage);
	return true;
}

bool ZoombiniText::isCreditTerminator(const Common::U32String &text) {
	return text.size() == 1 && text[0] == U'*';
}

bool ZoombiniText::readCreditStringsFromAnchor(const Common::Array<byte> &data, Common::CodePage codePage, const char *anchor, Common::Array<Common::U32String> &creditStrings) {
	uint32 offset = 0;
	if (!findBytes(data, anchor, offset))
		return false;

	creditStrings.clear();
	while (offset < data.size()) {
		Common::U32String text;
		if (!readExecutableStringAt(data, offset, codePage, text))
			return false;

		creditStrings.push_back(text);
		if (isCreditTerminator(text))
			return true;

		while (offset < data.size() && data[offset] != 0)
			offset++;
		offset++;
	}

	return false;
}

bool ZoombiniText::readCreditStringsFromPointerTable(const Common::Array<byte> &data, const ExeTextSource &source, Common::Array<Common::U32String> &creditStrings) {
	creditStrings.clear();
	for (uint rangeIndex = 0; rangeIndex < source.creditPointerRanges.size(); rangeIndex++) {
		const ExeCreditPointerRange &range = source.creditPointerRanges[rangeIndex];
		for (uint32 pointerIndex = range.firstIndex; pointerIndex <= range.lastIndex; pointerIndex++) {
			const uint32 tableEntryOffset = source.creditPointerTableOffset + pointerIndex * 4;
			if (data.size() < tableEntryOffset + 4)
				return false;

			uint32 stringAddress = static_cast<uint32>(data[tableEntryOffset]) |
								   (static_cast<uint32>(data[tableEntryOffset + 1]) << 8) |
								   (static_cast<uint32>(data[tableEntryOffset + 2]) << 16) |
								   (static_cast<uint32>(data[tableEntryOffset + 3]) << 24);
			if (source.creditPointerBlankAddress && stringAddress == source.creditPointerBlankAddress) {
				creditStrings.push_back(Common::U32String());
				continue;
			}
			if (stringAddress < source.creditPointerBaseAddress)
				return false;

			Common::U32String text;
			if (!readExecutableStringAt(data, stringAddress - source.creditPointerBaseAddress, source.codePage, text))
				return false;
			creditStrings.push_back(text);
		}
	}
	return !creditStrings.empty();
}

bool ZoombiniText::buildCreditParagraphsFromStrings(const Common::Array<Common::U32String> &creditStrings, Common::Array<CreditParagraph> &creditParagraphs) {
	creditParagraphs.clear();
	Common::Array<Common::U32String> lines;
	uint32 blankLineCount = 0;

	for (const Common::U32String &text : creditStrings) {
		if (isCreditTerminator(text))
			break;

		if (text.empty()) {
			if (!lines.empty())
				blankLineCount++;
			continue;
		}

		if (!lines.empty() && blankLineCount != 0) {
			creditParagraphs.push_back(CreditParagraph(lines, blankLineCount));
			lines.clear();
			blankLineCount = 0;
		}

		lines.push_back(text);
	}

	if (!lines.empty())
		creditParagraphs.push_back(CreditParagraph(lines, blankLineCount));

	return !creditParagraphs.empty();
}

bool ZoombiniText::loadOriginalExecutableCredits(const Common::Array<byte> &data, const ExeTextSource &source, Common::Array<CreditParagraph> &creditParagraphs) {
	Common::Array<Common::U32String> creditStrings;
	if (!source.creditPointerRanges.empty()) {
		if (!readCreditStringsFromPointerTable(data, source, creditStrings))
			return false;
	} else if (source.creditAnchor) {
		if (!readCreditStringsFromAnchor(data, source.codePage, source.creditAnchor, creditStrings))
			return false;
	} else {
		return false;
	}

	return buildCreditParagraphsFromStrings(creditStrings, creditParagraphs);
}

bool ZoombiniText::applyOriginalExecutableCreditLinePatches(Common::Array<CreditParagraph> &creditParagraphs, const CreditLinePatchMap &creditLinePatches) {
	for (const auto &entry : creditLinePatches) {
		const CreditLineAddress &address = entry._key;
		if (!address.isValid())
			return false;

		const uint32 paragraph = static_cast<uint32>(address.groupIndex);
		const uint32 line = static_cast<uint32>(address.inGroupLineIndex);

		if (creditParagraphs.size() <= paragraph || creditParagraphs[paragraph]._lines.size() <= line)
			return false;

		creditParagraphs[paragraph]._lines[line] = entry._value;
	}

	return true;
}

bool ZoombiniText::applyCreditParagraphSplit(Common::Array<CreditParagraph> &creditParagraphs, const CreditLineAddress &address, uint32 newParagraphBlankLineCount) {
	if (!address.isValid())
		return false;

	const uint32 paragraphIndex = static_cast<uint32>(address.groupIndex);
	const uint32 lineIndex = static_cast<uint32>(address.inGroupLineIndex);
	if (creditParagraphs.size() <= paragraphIndex)
		return false;

	CreditParagraph &paragraph = creditParagraphs[paragraphIndex];
	if (lineIndex == 0 || paragraph._lines.size() <= lineIndex)
		return false;

	Common::Array<Common::U32String> retainedLines;
	retainedLines.reserve(lineIndex);
	for (uint32 currentLineIndex = 0; currentLineIndex < lineIndex; currentLineIndex++)
		retainedLines.push_back(paragraph._lines[currentLineIndex]);

	Common::Array<Common::U32String> movedLines;
	movedLines.reserve(paragraph._lines.size() - lineIndex);
	for (uint32 currentLineIndex = lineIndex; currentLineIndex < paragraph._lines.size(); currentLineIndex++)
		movedLines.push_back(paragraph._lines[currentLineIndex]);

	paragraph._lines = retainedLines;
	creditParagraphs.insert_at(paragraphIndex + 1, CreditParagraph(movedLines, newParagraphBlankLineCount));
	return true;
}

bool ZoombiniText::applyCreditParagraphSplitPatches(Common::Array<CreditParagraph> &creditParagraphs, const CreditParagraphSplitMap &creditParagraphSplits) {
	Common::Array<CreditLineAddress> addresses;
	addresses.reserve(creditParagraphSplits.size());
	for (const auto &entry : creditParagraphSplits)
		addresses.push_back(entry._key);

	Common::sort(addresses.begin(), addresses.end(), [](const CreditLineAddress &left, const CreditLineAddress &right) {
		if (left.groupIndex != right.groupIndex)
			return left.groupIndex < right.groupIndex;
		return left.inGroupLineIndex < right.inGroupLineIndex;
	});

	for (uint addressIndex = addresses.size(); addressIndex != 0; addressIndex--) {
		const CreditLineAddress &address = addresses[addressIndex - 1];
		uint32 blankLineCount = 0;
		if (!creditParagraphSplits.tryGetVal(address, blankLineCount))
			return false;
		if (!applyCreditParagraphSplit(creditParagraphs, address, blankLineCount))
			return false;
	}

	return true;
}

bool ZoombiniText::parseUnsignedDecimalString(const Common::String &text, uint32 &value) {
	if (text.empty())
		return false;

	uint32 parsedValue = 0;
	for (uint charIndex = 0; charIndex < text.size(); charIndex++) {
		const char ch = text[charIndex];
		if (ch < '0' || '9' < ch)
			return false;

		const uint32 digit = static_cast<uint32>(ch - '0');
		if (parsedValue > (0xFFFFFFFFu - digit) / 10)
			return false;
		parsedValue = parsedValue * 10 + digit;
	}

	value = parsedValue;
	return true;
}

ZoombiniText::ZoombiniText(MohawkEngine_Zoombini *vm, Common::Language lang) : _vm(vm), _lang(lang) {
	// Users have to source the required font themselves!
	Common::String srcInst;
	switch (_lang) {
	case Common::EN_ANY:
	default:
		// English Zoombini string resources are encoded as CP1252
		_codePage = Common::kWindows1252;

		// English Zoombini used CornerStone font, bundled in installshield archives or install location.
		// - 1.1: found in /ZBARC16.Z, /ZBARC32.Z, or /SETUP/data1.cab
		// - 2.0: found in /INSTALL/HD/CORNER.TTF
		if (_vm->isGameVariant(GF_ZMB_TLC))
			srcInst = "Please provide an ISO root containing '/INSTALL/HD/CORNER.TTF' from the installer disk.";
		else
			srcInst = "Please provide an ISO root containing one of '/ZBARC32.Z', '/ZBARC16.Z', or '/SETUP/data1.cab' from the installer disk.";
		if (_vm->isGameVariant(GF_ZMB_TLC)) {
			_optimalTTFLoaders.push_back(new FileTTFLoader("INSTALL/HD/CORNER.TTF", "CornerStone", srcInst, false));
		} else {
			_optimalTTFLoaders.push_back(new ISZTTFLoader("ZBARC32.Z", "CORNER.TTF", "CornerStone", srcInst, false));
			_optimalTTFLoaders.push_back(new ISZTTFLoader("ZBARC16.Z", "CORNER.TTF", "CornerStone", srcInst, false));
			_optimalTTFLoaders.push_back(new ISCabTTFLoader("SETUP/data1.cab", "CORNER.TTF", "CornerStone", srcInst, false));
		}
		_textFontPoint = 13;
		_titleFontPoint = 18;
		_fallbackTTFLoaders.push_back(new ArchiveTTFLoader("LiberationMono-Bold.ttf", "Liberation Mono"));
		_fallbackTTFLoaders.push_back(new ArchiveTTFLoader("LiberationMono-Bold.ttf", "Liberation Mono"));

		// Initialize string maps
		initEnglishStrings();
		break;
	case Common::KO_KOR:
		// Korean Zoombini string resources are encoded as CP949
		_codePage = Common::kWindows949;

		// Korean Zoombini used GulimChe font. The original ISO does not include gulim.ttc.
		srcInst = "Please provide gulim.ttc at the ISO root or in '/DATA'.";
		_optimalTTFLoaders.push_back(new FileTTFLoader("gulim.ttc", "GulimChe", srcInst, true, 1));
		_optimalTTFLoaders.push_back(new FileTTFLoader("DATA/gulim.ttc", "GulimChe", srcInst, true, 1));
		_textFontPoint = 12;
		_titleFontPoint = 18;
		_fallbackTTFLoaders.push_back(new FileTTFLoader("D2CodingBold.ttf", "D2Coding", true));
		_fallbackTTFLoaders.push_back(new ArchiveTTFLoader("NotoSansKR-Bold.otf", "Noto Sans KR Bold"));

		// Initialize string maps
		initKoreanStrings();
		break;
	}

	if (!initOriginalExecutableStrings())
		warning("ZoombiniText: failed to load text from the original executable");

	// Check if ScummVM can access required fonts, and print warning message box if they are not found.
	loadFont(_optimalTTFLoaders, _fallbackTTFLoaders, _textFontPoint, true, _textFontCacheName);
	loadFont(_optimalTTFLoaders, _fallbackTTFLoaders, _titleFontPoint, false, _titleFontCacheName);

	// Map page type to text
	initPageKeyMap();
}

ZoombiniText::~ZoombiniText() {
	for (TTFLoader *loader : _optimalTTFLoaders)
		delete loader;
	_optimalTTFLoaders.clear();

	for (TTFLoader *loader : _fallbackTTFLoaders)
		delete loader;
	_fallbackTTFLoaders.clear();
}

bool ZoombiniText::initOriginalExecutableStrings() {
	static const Common::HashMap<uint32, Common::U32String> englishTextPatches = buildEnglishExeTextPatches();
	static const CreditLinePatchMap englishCreditLinePatches = buildEnglishExeCreditLinePatches();
	static const CreditLinePatchMap koreanCreditLinePatches = buildKoreanExeCreditLinePatches();

	static const ExeTextSource english11Sources[] = {
		ExeTextSource::fromArchiveMember("ZBARC32.Z", "Zoombi32.exe")
			.withTextTable(Common::kWindows1252, kEnglish11ExeTextEntries)
			.withTextPatches(englishTextPatches)
			.withCreditLinePatches(englishCreditLinePatches)
			.withCreditAnchor(EXE_CREDIT_PARAGRAPHS_ANCHOR),
		ExeTextSource::fromArchiveMember("ZBARC16.Z", "ZOOMBINI.EXE")
			.withTextTable(Common::kWindows1252, kEnglish11ExeTextEntries)
			.withEntryOffsetDelta(kEnglish11Win16TextOffsetDelta)
			.withTextPatches(englishTextPatches)
			.withCreditLinePatches(englishCreditLinePatches)
			.withCreditAnchor(EXE_CREDIT_PARAGRAPHS_ANCHOR),
		ExeTextSource::fromFile("Zoombi32.exe")
			.withTextTable(Common::kWindows1252, kEnglish11ExeTextEntries)
			.withTextPatches(englishTextPatches)
			.withCreditLinePatches(englishCreditLinePatches)
			.withCreditAnchor(EXE_CREDIT_PARAGRAPHS_ANCHOR),
		ExeTextSource::fromFile("ZOOMBINI.EXE")
			.withTextTable(Common::kWindows1252, kEnglish11ExeTextEntries)
			.withEntryOffsetDelta(kEnglish11Win16TextOffsetDelta)
			.withTextPatches(englishTextPatches)
			.withCreditLinePatches(englishCreditLinePatches)
			.withCreditAnchor(EXE_CREDIT_PARAGRAPHS_ANCHOR)};
	static const ExeTextSource english20Sources[] = {
		ExeTextSource::fromFile("INSTALL/HD/Zoombinis Logical Journey.exe")
			.withTextTable(Common::kWindows1252, kEnglish20ExeTextEntries)
			.withTextPatches(englishTextPatches)
			.withCreditLinePatches(englishCreditLinePatches)
			.withCreditPointerTable(0x90080, 0x400000, kEnglish20CreditPointerRanges)
			.withCreditPointerBlankAddress(0x4A286C)};
	static const ExeTextSource korean11Sources[] = {
		ExeTextSource::fromFile("SETUP/data1/data32/Zoombi32.exe")
			.withTextTable(Common::kWindows949, kKorean11ExeTextEntries)
			.withCreditLinePatches(koreanCreditLinePatches)
			.withCreditAnchor(EXE_CREDIT_PARAGRAPHS_ANCHOR),
		ExeTextSource::fromFile("SETUP/data1/data16/Zoombini.exe")
			.withTextTable(Common::kWindows949, kKorean11ExeTextEntries)
			.withEntryOffsetDelta(kKorean11Win16TextOffsetDelta)
			.withCreditLinePatches(koreanCreditLinePatches)
			.withCreditAnchor(EXE_CREDIT_PARAGRAPHS_ANCHOR),
		ExeTextSource::fromFile("Zoombi32.exe")
			.withTextTable(Common::kWindows949, kKorean11ExeTextEntries)
			.withCreditLinePatches(koreanCreditLinePatches)
			.withCreditAnchor(EXE_CREDIT_PARAGRAPHS_ANCHOR),
		ExeTextSource::fromFile("Zoombini.exe")
			.withTextTable(Common::kWindows949, kKorean11ExeTextEntries)
			.withEntryOffsetDelta(kKorean11Win16TextOffsetDelta)
			.withCreditLinePatches(koreanCreditLinePatches)
			.withCreditAnchor(EXE_CREDIT_PARAGRAPHS_ANCHOR)};

	const ExeTextSource *sources = nullptr;
	uint sourceCount = 0;
	if (_lang == Common::KO_KOR) {
		sources = korean11Sources;
		sourceCount = ARRAYSIZE(korean11Sources);
	} else if (_vm->isGameVariant(GF_ZMB_TLC)) {
		sources = english20Sources;
		sourceCount = ARRAYSIZE(english20Sources);
	} else {
		sources = english11Sources;
		sourceCount = ARRAYSIZE(english11Sources);
	}

	for (uint sourceIndex = 0; sourceIndex < sourceCount; sourceIndex++) {
		const ExeTextSource &source = sources[sourceIndex];
		Common::SeekableReadStream *exeStream = nullptr;

		if (source.fileName) {
			Common::File *file = new Common::File();
			if (file->open(Common::Path(source.fileName)))
				exeStream = file;
			else
				delete file;
		}

		if (!exeStream && source.archiveName) {
			Common::InstallShieldV3 archive;
			if (archive.open(Common::Path(source.archiveName)))
				exeStream = archive.createReadStreamForMember(Common::Path(source.archiveMemberName));
		}

		if (!exeStream)
			continue;

		Common::Array<byte> exeData;
		if (!readExecutableData(exeStream, exeData)) {
			delete exeStream;
			continue;
		}

		Common::Array<Common::U32String> strings;
		strings.reserve(source.entries.size());
		bool loaded = true;

		for (uint entryIndex = 0; entryIndex < source.entries.size(); entryIndex++) {
			const ExeTextEntry &entry = source.entries[entryIndex];
			const int64 entryOffset = getExeTextEntryOffset(source, entry);
			if (entryOffset < 0 || exeData.size() < static_cast<uint64>(entryOffset) + entry.length) {
				loaded = false;
				break;
			}

			strings.push_back(Common::U32String(reinterpret_cast<const char *>(exeData.data() + entryOffset), entry.length, source.codePage));
		}

		Common::Array<CreditParagraph> executableCredits;
		bool creditsLoaded = loaded && loadOriginalExecutableCredits(exeData, source, executableCredits);
		if (creditsLoaded && !applyOriginalExecutableCreditLinePatches(executableCredits, source.creditLinePatches))
			creditsLoaded = false;

		delete exeStream;

		if (!loaded)
			continue;

		for (uint entryIndex = 0; entryIndex < source.entries.size(); entryIndex++)
			_strMap[source.entries[entryIndex].key] = strings[entryIndex];
		applyOriginalExecutableTextPatches(source.textPatches);

		if (creditsLoaded)
			_creditParagraphs = executableCredits;
		else
			warning("ZoombiniText: failed to load credits from the original executable");

		return true;
	}

	return false;
}

void ZoombiniText::applyOriginalExecutableTextPatches(const Common::HashMap<uint32, Common::U32String> &textPatches) {
	for (const auto &entry : textPatches)
		_strMap[entry._key] = entry._value;
}

Common::Array<Common::String> ZoombiniText::tokenizeLines(const Common::String &text) {
	Common::Array<Common::String> lines;
	for (size_t lastIdx = 0; lastIdx < text.size();) {
		size_t chIdx = text.findFirstOf(Common::String("\r\n"), lastIdx);
		if (chIdx == Common::String::npos) {
			chIdx = text.findFirstOf('\r', lastIdx);
		}
		if (chIdx == Common::String::npos) {
			chIdx = text.findFirstOf('\n', lastIdx);
		}

		if (chIdx != Common::String::npos) {
			lines.push_back(text.substr(lastIdx, chIdx - lastIdx));
			lastIdx = chIdx + 1;
		} else {
			lines.push_back(text.substr(lastIdx, text.size() - lastIdx));
			break;
		}
	}
	return lines;
}

Common::Array<Common::U32String> ZoombiniText::tokenizeLines(const Common::U32String &text) {
	Common::Array<Common::U32String> lines;
	for (size_t lastIdx = 0; lastIdx < text.size();) {
		size_t chIdx = text.findFirstOf(Common::U32String("\r\n"), lastIdx);
		if (chIdx == Common::U32String::npos) {
			chIdx = text.findFirstOf('\r', lastIdx);
		}
		if (chIdx == Common::U32String::npos) {
			chIdx = text.findFirstOf('\n', lastIdx);
		}

		if (chIdx != Common::U32String::npos) {
			lines.push_back(text.substr(lastIdx, chIdx - lastIdx));
			lastIdx = chIdx + 1;
		} else {
			lines.push_back(text.substr(lastIdx, text.size() - lastIdx));
			break;
		}
	}
	return lines;
}

Common::String ZoombiniText::formatCreditLineKey(const CreditLineAddress &address) {
	return Common::String::format("credit-g%02d-%03d", address.groupIndex, address.inGroupLineIndex);
}

Common::String ZoombiniText::formatCreditLineKey(uint32 paragraphIndex, uint32 lineIndex) {
	return formatCreditLineKey(CreditLineAddress(static_cast<int>(paragraphIndex), static_cast<int>(lineIndex)));
}

bool ZoombiniText::parseCreditLineKey(const Common::String &creditKey, CreditLineAddress &address) {
	const Common::String prefix = "credit-g";
	if (creditKey.size() <= prefix.size() || creditKey.find(prefix) != 0)
		return false;

	const size_t separatorIndex = creditKey.find('-', prefix.size());
	if (separatorIndex == Common::String::npos || separatorIndex == prefix.size() || separatorIndex + 1 >= creditKey.size())
		return false;

	uint32 groupIndex = 0;
	uint32 lineIndex = 0;
	if (!parseUnsignedDecimalString(creditKey.substr(prefix.size(), separatorIndex - prefix.size()), groupIndex) ||
		!parseUnsignedDecimalString(creditKey.substr(separatorIndex + 1, creditKey.size() - separatorIndex - 1), lineIndex))
		return false;

	address = CreditLineAddress(static_cast<int>(groupIndex), static_cast<int>(lineIndex));
	return true;
}

bool ZoombiniText::parseCreditLineKey(const Common::String &creditKey, uint32 &paragraphIndex, uint32 &lineIndex) {
	CreditLineAddress address;
	if (!parseCreditLineKey(creditKey, address))
		return false;

	paragraphIndex = static_cast<uint32>(address.groupIndex);
	lineIndex = static_cast<uint32>(address.inGroupLineIndex);
	return true;
}

const Graphics::Font *ZoombiniText::getTextFont() {
	return FontMan.getFontByName(_textFontCacheName);
}

const Graphics::Font *ZoombiniText::getTitleFont() {
	return FontMan.getFontByName(_titleFontCacheName);
}

const Graphics::Font *ZoombiniText::getFont(ZoombiniFontUsage fontUsage) {
	switch (fontUsage) {
	case ZoombiniFontUsage::kFontDebugTitle: // For debug console
		return FontMan.getFontByUsage(Graphics::FontManager::kBigGUIFont);
	case ZoombiniFontUsage::kFontDebugText:
		return FontMan.getFontByUsage(Graphics::FontManager::kGUIFont);
	case ZoombiniFontUsage::kFontTitle:
		return getTitleFont();
	case ZoombiniFontUsage::kFontText:
		return getTextFont();
	default:
		error("Zoombini: not supported ZmbFontKind %u", static_cast<uint32>(fontUsage));
		return nullptr;
	}
}

const Graphics::Font *ZoombiniText::loadFont(const Common::Array<TTFLoader *> &optimalTTFLoaders, const Common::Array<TTFLoader *> &fallbackTTFLoaders, int point, bool showWarnMsgBox, Common::String &cacheName) {
	TTFLoader *firstOptimalLoader = _optimalTTFLoaders.front();
	assert(firstOptimalLoader != nullptr);
	for (TTFLoader *loader : _optimalTTFLoaders) {
		const Graphics::Font *font = loader->loadFont(point);
		if (!font)
			continue;

		cacheName = loader->getCacheName(point);
		return font;
	}

	// Cannot access the optimal font -> lookup for fallback fonts
	Common::String noFontMsg = Common::String::format("%s version of Zoombini requires the font '%s' to display texts properly, but ScummVM cannot access it.",
													  Common::getLanguageDescription(_lang), firstOptimalLoader->_filePath.baseName().c_str());

	for (TTFLoader *loader : _fallbackTTFLoaders) {
		const Graphics::Font *font = loader->loadFont(point);
		if (!font)
			continue;

		if (showWarnMsgBox) {
			const char *srcInst = firstOptimalLoader->_srcInst.c_str();
			if (srcInst[0] == '\0')
				srcInst = loader->_srcInst.c_str();

			Common::String warnMsg = Common::String::format(
				"%s\nScummVM will use the fallback font '%s' instead, text layouts would break!\n\n%s",
				noFontMsg.c_str(), loader->_filePath.baseName().c_str(), srcInst);
			warning("%s", warnMsg.c_str());
			GUI::MessageDialog dialog(warnMsg);
			dialog.runModal();
		}

		cacheName = loader->getCacheName(point);
		return font;
	}

	// Fallback TTF configurations must have prevented to reach here!
	error("%s\n\n%s", noFontMsg.c_str(), firstOptimalLoader->_srcInst.c_str());
	return nullptr;
}

Common::U32String ZoombiniText::toU32String(const byte *buf) const {
	return toU32String(reinterpret_cast<const char *>(buf));
}

Common::U32String ZoombiniText::toU32String(const byte *buf, int32 len) const {
	return toU32String(reinterpret_cast<const char *>(buf), len);
}

Common::U32String ZoombiniText::toU32String(const char *str) const {
	return Common::String(str).decode(_codePage);
}

Common::U32String ZoombiniText::toU32String(const char *str, int32 len) const {
	// Create a temporary null-terminated buffer to avoid warning of
	// "WARNING: Adding \0 to String. This is permitted, but can have unwanted consequences"
	char *buf = new char[len + 1];
	memcpy(buf, str, len);
	buf[len] = '\0';
	const Common::U32String &u32str = Common::String(buf).decode(_codePage);
	delete[] buf;
	return u32str;
}

Common::U32String ZoombiniText::toU32String(const Common::String &str) const {
	return str.decode(_codePage);
}

Common::String ZoombiniText::fromU32String(const Common::U32String &ustr) const {
	return ustr.encode(_codePage);
}

bool ZoombiniText::getStrl(Common::Array<Common::U32String> &outStrs, ZmbResource resource) {
	// STRL format: <COUNT: uint8> <null-terminated string> ...
	// readString(\x00) stops as soon as it hits \x00, consuming it.
	Common::SeekableReadStream *stringStream = _vm->getResource(ID_STRL, resource);
	if (!stringStream)
		return false;

	byte subStrCount = stringStream->readByte();

	outStrs.clear();
	for (uint16 i = 0; i < subStrCount && !stringStream->eos(); i++) {
		// Read until the null terminator (consumed but not returned)
		const Common::String &str = stringStream->readString('\0');
		const Common::U32String &ustr = toU32String(str);
		outStrs.push_back(ustr);
	}

	delete stringStream;

	return !outStrs.empty();
}

bool ZoombiniText::getStrl(Common::U32String &outStr, ZmbResource resource, uint16 subStrIdx) {
	// STRL format: <COUNT: uint8> <null-terminated string> ...
	// readString(\x00) stops as soon as it hits \x00, consuming it.

	Common::SeekableReadStream *stringStream = _vm->getResource(ID_STRL, resource);
	if (!stringStream)
		return false;

	byte subStrCount = stringStream->readByte();

	outStr.clear();
	for (uint16 i = 0; i < subStrCount && !stringStream->eos(); i++) {
		const Common::String &str = stringStream->readString('\0');
		if (i == subStrIdx) {
			outStr = toU32String(str);
			break;
		}
	}

	delete stringStream;

	return !outStr.empty();
}

Common::U32String ZoombiniText::getZoombiniName(int16 snoidId) {
	// snoidId: 0 ~ 624
	if (625 <= snoidId)
		error("SnoidID(%d) must be smaller than 625", snoidId);

	auto it = _nameCache.find(snoidId);
	if (it != _nameCache.end())
		return it->_value;

	int16 nameResId = snoidId / 100 + ZoombiniPage::kResStrl30000_ZoombiniNames; // 30000 ~ 30006
	int16 nameStrId = snoidId % 100;

	Common::Array<Common::U32String> zmbNames;
	if (!getStrl(zmbNames, ZmbResource(ZmbArchiveKind::kSystem, nameResId)) || static_cast<int16>(zmbNames.size()) <= nameStrId)
		error("Cannot get name of SnoidID(%d)", snoidId);

	int16 baseKey = snoidId - (snoidId % 100);
	for (int16 i = 0; i < static_cast<int16>(zmbNames.size()); i++)
		_nameCache[baseKey + i] = zmbNames[i];

	return zmbNames[nameStrId];
}

void ZoombiniText::cacheAllZoombiniNames() {
	if (!_nameIndexCache.empty())
		return; // Already built
	for (int16 i = 0; i < 625; i++) {
		Common::U32String name = getZoombiniName(i); // populates _nameCache in batches
		_nameIndexCache[name] = i;
	}
}

int16 ZoombiniText::findZoombiniNameId(const Common::U32String &name) const {
	auto it = _nameIndexCache.find(name);
	if (it == _nameIndexCache.end())
		return -1;
	return it->_value;
}

void ZoombiniText::clearNameCache() {
	_nameCache.clear();
	_nameIndexCache.clear();
}

/**
 * English Zoombini name generation tables, extracted from Zoombi32.exe v1.1.
 *
 * The English editions do not ship STRL 30000-30006 (the Korean name pool).
 * Instead, names are generated procedurally from these syllable tables.
 *
 * The algorithm alternates consonant and vowel syllables:
 *   - 39% chance to start with a vowel, 61% consonant.
 *   - Consonants at position <= 1: 67% single, 33% blend.
 *   - Consonants at position > 1: always blend.
 *   - If a blend lands on the last position, replace the final char with a
 *     simple vowel so the name ends softly.
 *   - If the first two characters are identical, retry from position 1.
 *   - Name length: random [4, 8].
 */

// 31 entries, 2 bytes each.  When the 2nd byte is a space (0x20),
// only the 1st byte is emitted (weighted single vowel).
static const char kVowelPairs[] =
	"a a a "   // a x3
	"e e e e " // e x4
	"i i i "   // i x3
	"o o o "   // o x3
	"u u "     // u x2
	"y "       // y x1
	"ee"
	"oo"
	"yo"
	"ya"
	"ye"
	"ei"
	"ie"
	"ai"
	"ia"
	"au"
	"ua"
	"uo"
	"ou"
	"ae"
	"ea";

// 32 weighted single consonants (some appear multiple times).
static const char kSingleConsonants[] = "bbccdddfghjkkllmmnnprrssssttvwxz";

// 6 simple vowels, used to replace a trailing consonant blend.
static const char kSimpleVowels[] = "aeiouy";

// 40 consonant blends, 2 bytes each.
static const char kConsonantBlends[] =
	"bl"
	"br"
	"ch"
	"cl"
	"cr"
	"dr"
	"dw"
	"fl"
	"fr"
	"gh"
	"gl"
	"gr"
	"kl"
	"kn"
	"kr"
	"kw"
	"ld"
	"mp"
	"nd"
	"nh"
	"nn"
	"ph"
	"pl"
	"pr"
	"qu"
	"qu"
	"rh"
	"rn"
	"sc"
	"sl"
	"sm"
	"sn"
	"sp"
	"sr"
	"st"
	"sw"
	"th"
	"tr"
	"tw"
	"wr";

static const int kNumVowelPairs = 31;
static const int kNumSingleConsonants = 32;
static const int kNumSimpleVowels = 6;
static const int kNumConsonantBlends = 40;
static const int kMaxNameLen = 10;

Common::U32String ZoombiniText::generateRandomName() {
	char buf[kMaxNameLen];
	memset(buf, 0, sizeof(buf));

	// Name length: random [4, kMaxNameLen - 2] = [4, 8]
	int16 nameLen = _vm->_rnd->getRandomNumber(4, kMaxNameLen - 2);
	int pos = 0;

	// 39% chance to start with a vowel (original: randomRange(1,100) < 40)
	bool nextIsVowel = _vm->_rnd->getRandomNumber(1, 100) < 40;

	while (pos < nameLen) {
		bool usedBlend = false;

		if (nextIsVowel) {
			nextIsVowel = false;
			int idx = 2 * _vm->_rnd->getRandomNumber(0, kNumVowelPairs - 1);
			char c1 = kVowelPairs[idx];
			if (kVowelPairs[idx + 1] != ' ') {
				// Digraph vowel (ee, oo, yo, etc.)
				buf[pos++] = c1;
				c1 = kVowelPairs[idx + 1];
			}
			buf[pos++] = c1;
		} else {
			nextIsVowel = true;
			if (pos <= 1 && _vm->_rnd->getRandomNumber(1, 100) > 33) {
				// 67% chance: single consonant at the start of the name
				buf[pos++] = kSingleConsonants[_vm->_rnd->getRandomNumber(0, kNumSingleConsonants - 1)];
			} else {
				// Consonant blend
				int idx = 2 * _vm->_rnd->getRandomNumber(0, kNumConsonantBlends - 1);
				buf[pos++] = kConsonantBlends[idx];
				buf[pos++] = kConsonantBlends[idx + 1];
				usedBlend = true;
			}
		}

		// If a blend pushed us to/past the target length, replace the last
		// character with a simple vowel so the name ends softly.
		if (usedBlend && pos >= nameLen) {
			buf[pos - 1] = kSimpleVowels[_vm->_rnd->getRandomNumber(0, kNumSimpleVowels - 1)];
		}

		// If the first two characters are identical, back up and retry.
		if (pos == 2 && buf[0] == buf[1]) {
			pos = 1;
		}
	}

	// Capitalize the first letter.
	if (buf[0] >= 'a' && buf[0] <= 'z')
		buf[0] -= 'a' - 'A';

	return Common::U32String(buf, Common::kASCII);
}

Common::U32String ZoombiniText::pickNextZoombiniName() {
	if (!_vm->hasResource(ID_STRL, ZmbResource(ZmbArchiveKind::kSystem, ZoombiniPage::kResStrl30000_ZoombiniNames)))
		return generateRandomName();

	// Korean: draw from the 625-slot name pool, tracking which names have been used.
	byte *nameTable = _vm->_state->_zoombiniNameGeneratedTable;

	bool allGenerated = true;
	for (int i = 0; i < 625; i++) {
		if (!nameTable[i]) {
			allGenerated = false;
			break;
		}
	}
	if (allGenerated)
		_vm->_state->buildNameGeneratedTable();

	uint16 nameId;
	do {
		nameId = static_cast<uint16>(_vm->_rnd->getRandomNumber(624));
	} while (nameTable[nameId]);
	nameTable[nameId] = 1;

	return getZoombiniName(nameId);
}

Common::U32String ZoombiniText::getPageName(ZoombiniPageType pageType) const {
	auto it = _pageKeyMap.find(pageType);
	if (it == _pageKeyMap.end())
		return Common::U32String("Unimplemented", Common::kUtf8);
	return _strMap[it->_value];
}

Common::U32String ZoombiniText::getLocalizedString(uint32 textKey) const {
	auto it = _strMap.find(textKey);
	if (it == _strMap.end())
		return Common::U32String("Unimplemented", Common::kUtf8);
	return it->_value;
}

void ZoombiniText::getLocalizedStrings(Common::Array<LocalizedString> &strings) const {
	strings.clear();
	for (const auto &entry : _strMap)
		strings.push_back(LocalizedString(entry._key, entry._value));
	Common::sort(strings.begin(), strings.end(), [](const LocalizedString &left, const LocalizedString &right) {
		return left._key < right._key;
	});
}

void ZoombiniText::getLocalizedCredits(Common::Array<CreditParagraph> &paragraphs) const {
	paragraphs = _creditParagraphs;
}

bool ZoombiniText::patchLocalizedText(const Common::String &textKey, const Common::U32String &text) {
	CreditLineAddress address;
	if (parseCreditLineKey(textKey, address))
		return patchCreditLine(static_cast<uint32>(address.groupIndex), static_cast<uint32>(address.inGroupLineIndex), text);

	uint32 numericKey = 0;
	if (!parseUnsignedDecimalString(textKey, numericKey))
		return false;

	patchLocalizedString(numericKey, text);
	return true;
}

bool ZoombiniText::patchLocalizedText(const Common::String &textKey, const char *utf8Text) {
	return patchLocalizedText(textKey, Common::U32String(utf8Text, Common::kUtf8));
}

bool ZoombiniText::patchLocalizedTexts(const Common::HashMap<Common::String, Common::U32String> &patches) {
	bool allPatched = true;
	for (const auto &entry : patches)
		allPatched = patchLocalizedText(entry._key, entry._value) && allPatched;
	return allPatched;
}

bool ZoombiniText::patchLocalizedTexts(const Common::HashMap<Common::String, Common::String> &patches) {
	bool allPatched = true;
	for (const auto &entry : patches)
		allPatched = patchLocalizedText(entry._key, entry._value.c_str()) && allPatched;
	return allPatched;
}

void ZoombiniText::patchLocalizedString(uint32 textKey, const Common::U32String &text) {
	_strMap[textKey] = text;
}

void ZoombiniText::patchLocalizedString(uint32 textKey, const char *utf8Text) {
	patchLocalizedString(textKey, Common::U32String(utf8Text, Common::kUtf8));
}

bool ZoombiniText::patchCreditLine(uint32 paragraphIndex, uint32 lineIndex, const Common::U32String &text) {
	if (_creditParagraphs.size() <= paragraphIndex || _creditParagraphs[paragraphIndex]._lines.size() <= lineIndex)
		return false;

	_creditParagraphs[paragraphIndex]._lines[lineIndex] = text;
	return true;
}

bool ZoombiniText::patchCreditLine(uint32 paragraphIndex, uint32 lineIndex, const char *utf8Text) {
	return patchCreditLine(paragraphIndex, lineIndex, Common::U32String(utf8Text, Common::kUtf8));
}

bool ZoombiniText::patchCreditLine(const Common::String &creditKey, const Common::U32String &text) {
	CreditLineAddress address;
	if (!parseCreditLineKey(creditKey, address))
		return false;

	return patchCreditLine(static_cast<uint32>(address.groupIndex), static_cast<uint32>(address.inGroupLineIndex), text);
}

bool ZoombiniText::patchCreditLine(const Common::String &creditKey, const char *utf8Text) {
	return patchCreditLine(creditKey, Common::U32String(utf8Text, Common::kUtf8));
}

bool ZoombiniText::splitCreditParagraph(uint32 paragraphIndex, uint32 lineIndex, uint32 newParagraphBlankLineCount) {
	return applyCreditParagraphSplit(_creditParagraphs, CreditLineAddress(static_cast<int>(paragraphIndex), static_cast<int>(lineIndex)), newParagraphBlankLineCount);
}

bool ZoombiniText::splitCreditParagraph(const Common::String &creditKey, uint32 newParagraphBlankLineCount) {
	CreditLineAddress address;
	if (!parseCreditLineKey(creditKey, address))
		return false;

	return applyCreditParagraphSplit(_creditParagraphs, address, newParagraphBlankLineCount);
}

bool ZoombiniText::patchCreditParagraph(uint32 paragraphIndex, const CreditParagraph &paragraph) {
	if (_creditParagraphs.size() <= paragraphIndex)
		return false;

	_creditParagraphs[paragraphIndex] = paragraph;
	return true;
}

void ZoombiniText::patchLocalizedCredits(const Common::Array<CreditParagraph> &paragraphs) {
	_creditParagraphs = paragraphs;
}

void ZoombiniText::initLocalizedCredits() {
	_creditParagraphs.clear();
}

void ZoombiniText::initPageKeyMap() {
	_pageKeyMap[ZoombiniPageType::kPicker] = ZoombiniText::kPicker;
	_pageKeyMap[ZoombiniPageType::kBridge] = ZoombiniText::kBridge;
	_pageKeyMap[ZoombiniPageType::kCaves] = ZoombiniText::kCaves;
	_pageKeyMap[ZoombiniPageType::kPizza] = ZoombiniText::kPizza;
	_pageKeyMap[ZoombiniPageType::kBasecamp1] = ZoombiniText::kBasecamp1;
	_pageKeyMap[ZoombiniPageType::kFerry] = ZoombiniText::kFerry;
	_pageKeyMap[ZoombiniPageType::kLilly] = ZoombiniText::kLilly;
	_pageKeyMap[ZoombiniPageType::kSlides] = ZoombiniText::kSlides;
	_pageKeyMap[ZoombiniPageType::kFleens] = ZoombiniText::kFleens;
	_pageKeyMap[ZoombiniPageType::kHotel] = ZoombiniText::kHotel;
	_pageKeyMap[ZoombiniPageType::kNet] = ZoombiniText::kNet;
	_pageKeyMap[ZoombiniPageType::kBasecamp2] = ZoombiniText::kBasecamp2;
	_pageKeyMap[ZoombiniPageType::kTunnels] = ZoombiniText::kTunnels;
	_pageKeyMap[ZoombiniPageType::kSmoke] = ZoombiniText::kSmoke;
	_pageKeyMap[ZoombiniPageType::kMaze] = ZoombiniText::kMaze;
	_pageKeyMap[ZoombiniPageType::kTown] = ZoombiniText::kTown;
}

#if 0
void ZoombiniText::initEnglishStrings() {
	// English version of Zoombini uses CornerStone font, which prints lowercase alphabet as uppercase.
	_strMap[kTown] = U"zoombiniville";
	_strMap[kPicker] = U"zoombini isle";
	_strMap[kBridge] = U"allergic cliffs";
	_strMap[kTunnels] = U"stone cold caves";
	_strMap[kPizza] = U"pizza pass";
	_strMap[kBasecamp1] = U"shelter rock";
	_strMap[kFerry] = U"captain cajun's ferryboat";
	_strMap[kLilly] = U"titanic tattooed toads";
	_strMap[kSlides] = U"stone rise";
	_strMap[kFleens] = U"fleens!";
	_strMap[kHotel] = U"hotel dimensia";
	_strMap[kNet] = U"mudball wall";
	_strMap[kBasecamp2] = U"shade tree";
	_strMap[kCaves] = U"the lion's lair";
	_strMap[kSmoke] = U"mirror machine";
	_strMap[kMaze] = U"bubblewonder abyss";
	_strMap[kNewGame] = U"NEW GAME";
	_strMap[kPracticeMode] = U"practice mode";
	_strMap[kContinueJourney] = U"Continue journey";
	_strMap[kPracticeTitle] = U"Practice mode";
	_strMap[kPracticeDesc1] = U" To Practice: Pick";
	_strMap[kPracticeDesc2] = U" a level and then";
	_strMap[kPracticeDesc3] = U" pick a location";
	_strMap[kPracticeDesc4] = U" on the map.";
	_strMap[kTerrainKey] = U"terrain key";
	_strMap[kChooseLevel] = U"choose a level";
	_strMap[kLevel1] = U"not so easy";
	_strMap[kLevel2] = U"oh, so hard";
	_strMap[kLevel3] = U"very hard";
	_strMap[kLevel4] = U"very, very hard";
	_strMap[kRoute1] = U"the big, the bad\nand the hungry";
	_strMap[kRoute2] = U"who's bayou";
	_strMap[kRoute3] = U"deep, dark\nforest";
	_strMap[kRoute4] = U"mountains\nof despair";
	_strMap[kXferVillePopulation] = U"zoombiniville\rpopulation";
	_strMap[kMemorialJanuary] = U"january";
	_strMap[kMemorialFebruary] = U"february";
	_strMap[kMemorialMarch] = U"march";
	_strMap[kMemorialApril] = U"april";
	_strMap[kMemorialMay] = U"may";
	_strMap[kMemorialJune] = U"june";
	_strMap[kMemorialJuly] = U"july";
	_strMap[kMemorialAugust] = U"august";
	_strMap[kMemorialSeptember] = U"september";
	_strMap[kMemorialOctober] = U"october";
	_strMap[kMemorialNovember] = U"november";
	_strMap[kMemorialDecember] = U"december";
	_strMap[kMemorialWhenLevel] = U"when traveling was";
	_strMap[kMemorialHonorMonument] = U"this monument was made to\rhonor the zoombinis who:";
	_strMap[kMemorialHonorWindmill] = U"this windmill was wrought\rto honor the zoombinis who:";
	_strMap[kMemorialHonorObservatory] = U"this observatory observes\rthe zoombinis who:";
	_strMap[kMemorialHonorBowlingAlley] = U"this bowling alley\rhonors the zoombinis who:";
	_strMap[kMemorialHonorGeneralStore] = U"this general store was\rerected for the zoombinis who:";
	_strMap[kMemorialHonorSwimmingPool] = U"this swimming pool\rsalutes the zoombinis who:";
	_strMap[kMemorialHonorPlayground] = U"this playground was pitched\rto honor the zoombinis who:";
	_strMap[kMemorialHonorBandShell] = U"this band shell was built to\rhonor the zoombinis who:";
	_strMap[kMemorialHonorSchool] = U"this schoolhouse salutes\rthe zoombinis who:";
	_strMap[kMemorialHonorLibrary] = U"this library was raised to\rhonor the zoombinis who:";
	_strMap[kMemorialHonorFire] = U"this firehouse honors\rthe zoombinis who:";
	_strMap[kMemorialHonorOpera] = U"this opera house sings\rpraises to the zoombinis who:";
	_strMap[kMemorialHonorCityHall] = U"this city hall celebrates\rthe zoombinis who:";
	_strMap[kMemorialHonorClockTower] = U"this clock tower was\rconstructed for the zoombinis who:";
	_strMap[kMemorialHonorMuseum] = U"this paper clip museum was made\rfor the zoombinis who:";
	_strMap[kMemorialHonorCourt] = U"this courthouse was constructed\rfor the zoombinis who:";
	_strMap[kMemorialRoute1Level1] = U"ambled past allergic cliffs,\rcruised on by\rstone cold caves,\rand\rappeased arno the\ralmost omnivorous";
	_strMap[kMemorialRoute1Level2] = U"braved blustery bridges,\routsmarted onyx's\rstone faced crew,\rand\rwon over willomaen\rthe pizza eating troll";
	_strMap[kMemorialRoute1Level3] = U"bested bridge watchers,\rcrept cautiously\rpast cave guards, \rand\rsatiated shyler the\rpizza loving troll";
	_strMap[kMemorialRoute1Level4] = U"outsmarted sneezing cliffs, conquered crusty\rcave guards,\rand\rplacated picky pizza trolls\rwithout hearing\r\"Yuck!\"";
	_strMap[kMemorialRoute2Level1] = U"calmed captain cajun,\rrode tattooed toads,\rand\rknew how to network";
	_strMap[kMemorialRoute2Level2] = U"finagled the ferryboat,\rsuccessfully swapped\rlily pads,\rand\rconnected the current";
	_strMap[kMemorialRoute2Level3] = U"finessed the ferryboat,\rcrept cautiously past\rlily pad crabs,\rand\rsurmounted\rstone elevators";
	_strMap[kMemorialRoute2Level4] = U"calmed captain cajun,\rrode tattooed toads,\rand\rknew how to network";
	_strMap[kMemorialRoute3Level1] = U"flushed the finicky fleens,\rin hotel dimensia had\rpleasant dreams\rand\rcatapulted cleanly\rover Mudball Wall";
	_strMap[kMemorialRoute3Level2] = U"flustered the fleens,\rdidn't dally at\rhotel dimensia,\rand\rmastered the\rmudball making machine";
	_strMap[kMemorialRoute3Level3] = U"sent the fleens flying,\rwrangled with\rransacked rooms,\rand\rvaulted the wall\rwith hardly a fall";
	_strMap[kMemorialRoute3Level4] = U"finally foiled the fleens,\rresolved the hotel\rrooming scene,\rand\rmastered the\rmudball wall machine";
	_strMap[kMemorialRoute4Level1] = U"did not lag in lion's lair,\rsolved the secrets of\rthe mirror machine,\rand\rflew above\rbubblewonder abyss";
	_strMap[kMemorialRoute4Level2] = U"overcame their fear\rin lion's lair,\rhad things go fine\rin the mirror machine,\rand\rrode a wonderous\rbubble ship";
	_strMap[kMemorialRoute4Level3] = U"deciphered the lion's logic,\rcorrectly calculated\rthe crystals,\rand\rascended the airy abyss";
	_strMap[kMemorialRoute4Level4] = U"raised up high the lion's paw,\rlined up the right crystals\rthat they clearly saw,\rand\rmastered bubblewonder\rwithout\rfalling in its maw";
	_strMap[kDialogBodyGoMapWillLost] = U"THE CURRENT PARTY OF ZOOMBINIS WILL BE LOST IF YOU GO TO THE MAP";
	_strMap[kDialogButtonLoseThem] = U"LOSE ' EM";
	_strMap[kDialogButtonKeepThem] = U"KEEP ' EM";
	_strMap[kDialogButtonOkay] = U"OK";
	_strMap[kDialogButtonCancel] = U"CANCEL";
	_strMap[kDialogButtonYes] = U"YES";
	_strMap[kDialogButtonNo] = U"NO";
	_strMap[kDialogButtonQuit] = U"QUIT";
	_strMap[kDialogButtonLoad] = U"LOAD";
	_strMap[kDialogButtonSave] = U"SAVE";
	_strMap[kOptionsTitle] = U"OPTIONS";
	_strMap[kOptionsLegendOn] = U"= ON";
	_strMap[kOptionsLegendOff] = U"= OFF";
	_strMap[kOptionsNewGame] = U"NEW GAME (CTRL N)";
	_strMap[kOptionsLoadGame] = U"LOAD GAME (CTRL L)";
	_strMap[kOptionsSaveGame] = U"SAVE GAME (CTRL S)";
	_strMap[kOptionsQuit] = U"QUIT (CTRL Q)";
	_strMap[kOptionsToggle] = U"ON/OFF TOGGLES:";
	_strMap[kOptionsSound] = U"DIALOG & SOUND FX (CTRL D)";
	_strMap[kOptionsMusic] = U"BACKGROUND MUSIC (CTRL B)";
	_strMap[kOptionsStickyMouse] = U"STICKY MOUSE (CTRL J)";
	_strMap[kOptionsTransitions] = U"TRANSITIONS (CTRL T)";
	_strMap[kOptionsCredits] = U"CREDITS";
	_strMap[kDialogBodyNoSavedGames] = U"NO SAVED GAMES";
	_strMap[kDialogBodyCreateAndSaveNewGame] = U"The current game has not been saved.\rCreate a new game ?";
	_strMap[kDialogButtonNewGame] = U"NEW GAME";
	_strMap[kDialogButtonReplaceTitle] = U"REPLACE";
	_strMap[kDialogTitleSave] = U"SAVE A GAME";
	_strMap[kDialogTitleSaveAs] = U"SAVE GAME AS:";
	_strMap[kDialogTitleLoad] = U"LOAD A GAME";
	_strMap[kDialogBodyReplaceGame] = U"are you sure you want to\rreplace existing game\r\"";
	_strMap[kDialogBodySaveCurrentGame] = U"save the current game ?";
	_strMap[kDialogBodySaveDirtyGame] = U"the current game\rhas not been saved.\rdo you want to save it?";
	_strMap[kDialogBodyCannotSaveInPractice] = U"cannot save a game while in practice mode.";
	_strMap[kDialogBodyCreateNewGame] = U"ARE YOU SURE YOU WANT TO MAKE A NEW GAME ?";
	_strMap[kDialogBodyCannotSaveMoreGame] = U"cannot save more games than the number of states in the united states.";
	_strMap[kDialogBodyCannotLoadInPractice] = U"cannot load a game while in practice mode.";
	_strMap[kDialogBodyCannotCreateNewInPractice] = U"cannot create a new game while in practice mode.";
	_strMap[kDialogBodyNewGame] = U"NEW GAME";
	_strMap[kDialogBodyReallyQuit] = U"DO YOU REALLY WANT TO QUIT ?";
	_strMap[kDialogBodySaveBeforeQuit] = U"the current game\rhas not been saved.\rdo you want to save your progress before quitting?";
	_strMap[kDialogHelpTitle] = U"HELP";
	_strMap[kDialogButtonPrev] = U"previous";
	_strMap[kDialogButtonNext] = U"NEXT";
	_strMap[kDialogHelpLevel] = U"Level"; // FIXME: Find a proper translation
	_strMap[kNotiBoxMusicOn] = U"music on";
	_strMap[kNotiBoxMusicOff] = U"music off";
	_strMap[kNotiBoxSoundOn] = U"sound on";
	_strMap[kNotiBoxSoundOff] = U"sound off";
	_strMap[kNotiBoxLessAction] = U"less action";
	_strMap[kNotiBoxMoreAction] = U"more action";
	_strMap[kNotiBoxHideCursor] = U"hide cursor";
	_strMap[kNotiBoxShowCursor] = U"show cursor";
	_strMap[kNotiBoxStickeyMouse] = U"sticky mouse";
	_strMap[kNotiBoxNonStickeyMouse] = U"non-sticky mouse";
	_strMap[kNotiBoxTransitionsOn] = U"transitions on";
	_strMap[kNotiBoxTransitionsOff] = U"transitions off";
	_strMap[kNotiBoxAutoStickeyOn] = U"auto sticky on";
	_strMap[kNotiBoxAutoStickeyOff] = U"auto sticky off";
}

void ZoombiniText::initEnglishTlcStrings() {
	_strMap[kOptionsTitle] = U"OPTIONS (SHIFT ?)";
	_strMap[kOptionsHelpAudio] = U"HELP AUDIO (CTRL A)";
	_strMap[kOptionsTouchSense] = U"IMMERSION TOUCHSENSEª (CTRL K)";
	_strMap[kDialogHelpTitle] = U"help";
	_strMap[kDialogButtonOkay] = U"ok";
	_strMap[kDialogButtonNext] = U"next";
	_strMap[kDialogBodyRemoveGame] = U"are you sure you want to\rremove this game?";
	_strMap[kNotiBoxHelpAudioOn] = U"help audio on";
	_strMap[kNotiBoxHelpAudioOff] = U"help audio off";
	_strMap[kNotiBoxTouchSenseOn] = U"touch sense on";
	_strMap[kNotiBoxTouchSenseOff] = U"touch sense off";
}
#endif

void ZoombiniText::initEnglishStrings() {
	if (!_vm->isGameVariant(GF_ZMB_TLC)) {
		// ScummVM addition for v1.x version
		_strMap[kDialogBodyRemoveGame] = U"are you sure you want to\rremove this game?";
	}
}

#if 0
void ZoombiniText::getEnglishCredits(Common::Array<CreditParagraph> &paragraphs) const {
	paragraphs.clear();

	Common::Array<Common::U32String> lines;
	lines.push_back(Common::U32String("PRODUCT CONCEPT AND DESIGN", Common::kUtf8));
	lines.push_back(Common::U32String("chris hancock and", Common::kUtf8));
	lines.push_back(Common::U32String("scot osterweil", Common::kUtf8));
	lines.push_back(Common::U32String("of TERC", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 10));

	lines.clear();
	lines.push_back(Common::U32String("ART DIRECTOR", Common::kUtf8));
	lines.push_back(Common::U32String("michelle bushneff", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("PROGRAMMING LEAD", Common::kUtf8));
	lines.push_back(Common::U32String("michael g. rivard", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("SOUND DIRECTOR", Common::kUtf8));
	lines.push_back(Common::U32String("jonelle adkisson", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("PRODUCT MANAGER", Common::kUtf8));
	lines.push_back(Common::U32String("dennis leahy", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));
	lines.clear();

	lines.push_back(Common::U32String("EXECUTIVE PUBLISHER", Common::kUtf8));
	lines.push_back(Common::U32String("laurie strand", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 5));

	lines.clear();
	lines.push_back(Common::U32String("ADDITIONAL DESIGN", Common::kUtf8));
	lines.push_back(Common::U32String("michelle bushneff", Common::kUtf8));
	lines.push_back(Common::U32String("daniel goodwin", Common::kUtf8));
	lines.push_back(Common::U32String("dennis leahy", Common::kUtf8));
	lines.push_back(Common::U32String("michael g. rivard", Common::kUtf8));
	lines.push_back(Common::U32String("rod nelsen", Common::kUtf8));
	lines.push_back(Common::U32String("jonelle adkisson", Common::kUtf8));
	lines.push_back(Common::U32String("mark hanson", Common::kUtf8));
	lines.push_back(Common::U32String("karen boylan", Common::kUtf8));
	lines.push_back(Common::U32String("bob king", Common::kUtf8));
	lines.push_back(Common::U32String("michelle graham", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 3));

	lines.clear();
	lines.push_back(Common::U32String("PROTOTYPER", Common::kUtf8));
	lines.push_back(Common::U32String("daniel goodwin", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("LEAD ANIMATOR", Common::kUtf8));
	lines.push_back(Common::U32String("bob king", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("CHARACTER ANIMATORS", Common::kUtf8));
	lines.push_back(Common::U32String("bob king", Common::kUtf8));
	lines.push_back(Common::U32String("jason sadler", Common::kUtf8));
	lines.push_back(Common::U32String("kevin dooley", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("COMBINATORIAL ANIMATORS", Common::kUtf8));
	lines.push_back(Common::U32String("michelle graham", Common::kUtf8));
	lines.push_back(Common::U32String("daniel goodwin", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("EFFECTS ANIMATOR", Common::kUtf8));
	lines.push_back(Common::U32String("michelle graham", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("LAYOUT ARTISTS", Common::kUtf8));
	lines.push_back(Common::U32String("jason sadler", Common::kUtf8));
	lines.push_back(Common::U32String("kim farrah", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("BACKGROUND PAINTER", Common::kUtf8));
	lines.push_back(Common::U32String("kim farrah", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("ANIMATION CLEAN-UP", Common::kUtf8));
	lines.push_back(Common::U32String("michelle shelfer", Common::kUtf8));
	lines.push_back(Common::U32String("monica dacany", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("GRAPHICS TECHNICIAN", Common::kUtf8));
	lines.push_back(Common::U32String("suzanne runo", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 3));

	lines.clear();
	lines.push_back(Common::U32String("PROGRAMMING", Common::kUtf8));
	lines.push_back(Common::U32String("michael g. rivard", Common::kUtf8));
	lines.push_back(Common::U32String("mark hanson", Common::kUtf8));
	lines.push_back(Common::U32String("rod nelsen", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("PROGRAMMING PROJECT LEAD", Common::kUtf8));
	lines.push_back(Common::U32String("lance groody", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 3));

	lines.clear();
	lines.push_back(Common::U32String("ZOOMBINI MUSIC", Common::kUtf8));
	lines.push_back(Common::U32String("jonelle adkisson", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("SOUND EFFECTS", Common::kUtf8));
	lines.push_back(Common::U32String("chris clanin", Common::kUtf8));
	lines.push_back(Common::U32String("gary schwantes", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("DIALOG EDITORS", Common::kUtf8));
	lines.push_back(Common::U32String("phillip royer", Common::kUtf8));
	lines.push_back(Common::U32String("chris clanin", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("RECORDING ENGINEER", Common::kUtf8));
	lines.push_back(Common::U32String(" chris clanin", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("FLUTE AND SAXOPHONE", Common::kUtf8));
	lines.push_back(Common::U32String("gary schwantes", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("CHARACTER VOICES", Common::kUtf8));
	lines.push_back(Common::U32String("randall nazarian", Common::kUtf8));
	lines.push_back(Common::U32String("max trax", Common::kUtf8));
	lines.push_back(Common::U32String("lorrin jones", Common::kUtf8));
	lines.push_back(Common::U32String("racer stevens", Common::kUtf8));
	lines.push_back(Common::U32String("deborah sale", Common::kUtf8));
	lines.push_back(Common::U32String("randy williams", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("ZOOMBINI VOICES", Common::kUtf8));
	lines.push_back(Common::U32String("jonelle adkisson", Common::kUtf8));
	lines.push_back(Common::U32String("michelle graham", Common::kUtf8));
	lines.push_back(Common::U32String("tom rettig", Common::kUtf8));
	lines.push_back(Common::U32String("norm macqueen", Common::kUtf8));
	lines.push_back(Common::U32String("haroon tahir", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 3));

	lines.clear();
	lines.push_back(Common::U32String("DIALOG WRITERS", Common::kUtf8));
	lines.push_back(Common::U32String("karen boylan", Common::kUtf8));
	lines.push_back(Common::U32String("dennis leahy", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("ADDITIONAL WRITERS", Common::kUtf8));
	lines.push_back(Common::U32String("amanda silber", Common::kUtf8));
	lines.push_back(Common::U32String("jonelle adkisson", Common::kUtf8));
	lines.push_back(Common::U32String("scot osterweil", Common::kUtf8));
	lines.push_back(Common::U32String("chris hancock", Common::kUtf8));
	lines.push_back(Common::U32String("scott jones", Common::kUtf8));
	lines.push_back(Common::U32String("doug van ommeran", Common::kUtf8));
	lines.push_back(Common::U32String("matt o'hara", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("ASSISTANT PRODUCT MANAGER", Common::kUtf8));
	lines.push_back(Common::U32String("karen boylan", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("MARKETING", Common::kUtf8));
	lines.push_back(Common::U32String("linda dalton", Common::kUtf8));
	lines.push_back(Common::U32String("aline yu", Common::kUtf8));
	lines.push_back(Common::U32String("jennifer apy", Common::kUtf8));
	lines.push_back(Common::U32String("veronica bowers", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("QUALITY ASSURANCE MANAGER", Common::kUtf8));
	lines.push_back(Common::U32String("ginny walters", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("QUALITY ASSURANCE LEADS", Common::kUtf8));
	lines.push_back(Common::U32String("john crowell", Common::kUtf8));
	lines.push_back(Common::U32String("warren yamashita", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("KEY QA FOLKS", Common::kUtf8));
	lines.push_back(Common::U32String("lisa irwin", Common::kUtf8));
	lines.push_back(Common::U32String("john hamele", Common::kUtf8));
	lines.push_back(Common::U32String("margaret coholan", Common::kUtf8));
	lines.push_back(Common::U32String("joy southern", Common::kUtf8));
	lines.push_back(Common::U32String("drew garske", Common::kUtf8));
	lines.push_back(Common::U32String("juan torres", Common::kUtf8));
	lines.push_back(Common::U32String("anne sete", Common::kUtf8));
	lines.push_back(Common::U32String("lisa bonelli", Common::kUtf8));
	lines.push_back(Common::U32String("mario magliocco", Common::kUtf8));
	lines.push_back(Common::U32String("jeffrey 'hammer' blain", Common::kUtf8));
	lines.push_back(Common::U32String("joe lawrence", Common::kUtf8));
	lines.push_back(Common::U32String("kirk roulston", Common::kUtf8));
	lines.push_back(Common::U32String("brian campbell", Common::kUtf8));
	lines.push_back(Common::U32String("LEGAL COUNSEL", Common::kUtf8));
	lines.push_back(Common::U32String("brett robertson", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("PACKAGE DESIGN", Common::kUtf8));
	lines.push_back(Common::U32String("marcus badgley", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("MANUAL DESIGN", Common::kUtf8));
	lines.push_back(Common::U32String("marcus badgley", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("MANUAL WRITER", Common::kUtf8));
	lines.push_back(Common::U32String("karen boylan", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("Parents' Corner:", Common::kUtf8));
	lines.push_back(Common::U32String("scot osterweil", Common::kUtf8));
	lines.push_back(Common::U32String("chris hancock", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("SPECIAL THANKS TO:", Common::kUtf8));
	lines.push_back(Common::U32String("zoombini team families", Common::kUtf8));
	lines.push_back(Common::U32String("harry wilker", Common::kUtf8));
	lines.push_back(Common::U32String("jan gullett", Common::kUtf8));
	lines.push_back(Common::U32String("mason woodbury", Common::kUtf8));
	lines.push_back(Common::U32String("john baker", Common::kUtf8));
	lines.push_back(Common::U32String("barbara samson", Common::kUtf8));
	lines.push_back(Common::U32String("dabney standley", Common::kUtf8));
	lines.push_back(Common::U32String("mickey mantle", Common::kUtf8));
	lines.push_back(Common::U32String("tom marcus", Common::kUtf8));
	lines.push_back(Common::U32String("marylyn rosenblum", Common::kUtf8));
	lines.push_back(Common::U32String("lucinda ray", Common::kUtf8));
	lines.push_back(Common::U32String("tom rettig", Common::kUtf8));
	lines.push_back(Common::U32String("alex tkaczevski", Common::kUtf8));
	lines.push_back(Common::U32String("BrØderbund's QA department", Common::kUtf8));
	lines.push_back(Common::U32String("ImageBuilder Software, Inc.", Common::kUtf8));
	lines.push_back(Common::U32String("tomoko harada", Common::kUtf8));
	lines.push_back(Common::U32String("esteban ahn", Common::kUtf8));
	lines.push_back(Common::U32String("mike foulger", Common::kUtf8));
	lines.push_back(Common::U32String("seth jacobson", Common::kUtf8));
	lines.push_back(Common::U32String("matt o'hara", Common::kUtf8));
	lines.push_back(Common::U32String("hilary nation", Common::kUtf8));
	lines.push_back(Common::U32String("wendy McWilliams", Common::kUtf8));
	lines.push_back(Common::U32String("jim krouskop", Common::kUtf8));
	lines.push_back(Common::U32String("mandy crispel", Common::kUtf8));
	lines.push_back(Common::U32String("bacich elementary", Common::kUtf8));
	lines.push_back(Common::U32String("kent middle school", Common::kUtf8));
	lines.push_back(Common::U32String("madera elementary", Common::kUtf8));
	lines.push_back(Common::U32String("apple blossom elementary", Common::kUtf8));
	lines.push_back(Common::U32String("cafe west", Common::kUtf8));
	lines.push_back(Common::U32String());
	lines.push_back(Common::U32String("the fletcher school", Common::kUtf8));
	lines.push_back(Common::U32String("cambridge, MA", Common::kUtf8));
	lines.push_back(Common::U32String());
	lines.push_back(Common::U32String("the lincoln school", Common::kUtf8));
	lines.push_back(Common::U32String("brookline, MA", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 3));

	lines.clear();
	lines.push_back(Common::U32String("copyright 1996", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("BrØderbund Software, Inc.", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 1));

	lines.clear();
	lines.push_back(Common::U32String("and TERC", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 2));

	lines.clear();
	lines.push_back(Common::U32String("all rights reserved", Common::kUtf8));
	paragraphs.push_back(CreditParagraph(lines, 14));
}

void ZoombiniText::getEnglishTlcCredits(Common::Array<CreditParagraph> &paragraphs) const {
	getEnglishCredits(paragraphs);
	assert(4 <= paragraphs.size());
	paragraphs.resize(paragraphs.size() - 4);

	const char *const version2Team[] = { "VERSION 2 TEAM" };
	addCreditParagraph(paragraphs, version2Team, 3);
	const char *const programming[] = { "PROGRAMMING", "kari ann imamura", "william gayer", "cuong nguyen", "darrell fetzer" };
	addCreditParagraph(paragraphs, programming, 1);
	const char *const educationalDesigner[] = { "EDUCATIONAL DESIGNER", "alexander watson" };
	addCreditParagraph(paragraphs, educationalDesigner, 1);
	const char *const qaLead[] = { "QA LEAD", "tracy gibson" };
	addCreditParagraph(paragraphs, qaLead, 1);
	const char *const qaTesters[] = { "QA TESTERS", "ericka west", "dean coronado" };
	addCreditParagraph(paragraphs, qaTesters, 1);
	const char *const qaSupervisor[] = { "QA SUPERVISOR", "carlos molina" };
	addCreditParagraph(paragraphs, qaSupervisor, 1);
	const char *const qaManager[] = { "QA MANAGER", "dan mizuba" };
	addCreditParagraph(paragraphs, qaManager, 1);
	const char *const brandProducer[] = { "BRAND PRODUCER", "elizabeth perrault" };
	addCreditParagraph(paragraphs, brandProducer, 1);
	const char *const seniorBrandProducer[] = { "SENIOR BRAND PRODUCER", "mary ann duringer" };
	addCreditParagraph(paragraphs, seniorBrandProducer, 1);
	const char *const developmentProducer[] = { "DEVELOPMENT PRODUCER", "susan nachand-prestidge" };
	addCreditParagraph(paragraphs, developmentProducer, 1);
	const char *const artist[] = { "ARTIST", "gerald broas", "barry prioste", "fred dianda" };
	addCreditParagraph(paragraphs, artist, 1);
	const char *const associateBrandManager[] = { "ASSOCIATE BRAND MANAGER", "kathy degan" };
	addCreditParagraph(paragraphs, associateBrandManager, 1);
	const char *const seniorBrandManager[] = { "SENIOR BRAND MANAGER", "sara horton" };
	addCreditParagraph(paragraphs, seniorBrandManager, 1);
	const char *const designServicesManager[] = { "DESIGN SERVICES MANAGER", "sally mark" };
	addCreditParagraph(paragraphs, designServicesManager, 1);
	const char *const editorialManager[] = { "EDITORIAL MANAGER", "gabrielle rennie" };
	addCreditParagraph(paragraphs, editorialManager, 1);
	const char *const productionEngineer[] = { "PRODUCTION ENGINEER", "greg kitamura" };
	addCreditParagraph(paragraphs, productionEngineer, 1);
	const char *const voiceTalent[] = { "VOICE TALENT", "sara rene martin", "jenny rae" };
	addCreditParagraph(paragraphs, voiceTalent, 1);
	const char *const soundDesign[] = { "SOUND DESIGN", "andrew kawamura", "jamie hert" };
	addCreditParagraph(paragraphs, soundDesign, 1);
	const char *const touchSenseEngineer[] = { "IMMERSION TOUCHSENSEª ENGINEER", "margie luong" };
	addCreditParagraph(paragraphs, touchSenseEngineer, 1);
	const char *const creativeDevelopment[] = { "DIR of CREATIVE DEVELOPMENT", "drayson nowlan" };
	addCreditParagraph(paragraphs, creativeDevelopment, 1);
	const char *const vpEngineering[] = { "VP of ENGINEERING", "hugo paz" };
	addCreditParagraph(paragraphs, vpEngineering, 1);
	const char *const vpResearchDevelopment[] = { "VP of R&D", "derek miyahara" };
	addCreditParagraph(paragraphs, vpResearchDevelopment, 1);
	const char *const generalManager[] = { "SR VP, GENERAL MANAGER", "eric stone" };
	addCreditParagraph(paragraphs, generalManager, 1);
	const char *const legalText[] = {
		"© 2001 TLC Education Properties LLC, and its",
		"licensors. All rights reserved. Uses Miles",
		"Sound System. Copyright © 1991-2001 by RAD",
		"Game Tools, Inc. Uses Bink Video Technology.",
		"Copyright © 1997-2001 by RAD Game Tools, Inc.",
		"The Learning Company and Logical Journey of",
		"the Zoombinis are registered trademarks and",
		"Zoombinis Logical Journey is a trademark of",
		"TLC Education Properties LLC. Windows and Win",
		"are either registered trademarks or trademarks",
		"of Microsoft Corporation in the United States",
		"and/or other countries. Macintosh and Mac are",
		"registered trademarks of Apple Computer, Inc.",
		"All other trademarks are the property of",
		"their respective owners.",
		"",
		" DirectX is a proprietary tool of Microsoft",
		"Corporation and its suppliers and may only",
		"be used in conjunction with Microsoft operating",
		"system products. All intellectual property",
		"rights in the DirectX are owned by Microsoft",
		"Corporation and its suppliers and are protected",
		"by United States copyright laws and international",
		"treaty provisions.",
		"Copyright © 2001 Microsoft Corporation.",
		"All rights reserved"
	};
	addCreditParagraph(paragraphs, legalText, 14);
}

#endif

#if 0
void ZoombiniText::initKoreanStrings() {
	_strMap[kPicker] = U"줌비니 섬";
	_strMap[kBridge] = U"알레르기 절벽";
	_strMap[kCaves] = U"사자 동굴";
	_strMap[kPizza] = U"피자 파티";
	_strMap[kBasecamp1] = U"너럭바위 쉼터";
	_strMap[kFerry] = U"통통 나루";
	_strMap[kLilly] = U"두꺼비 연못";
	_strMap[kSlides] = U"소슬 바위";
	_strMap[kFleens] = U"삐따기 동네";
	_strMap[kHotel] = U"숲속의 방";
	_strMap[kNet] = U"널뛰기 돌벽";
	_strMap[kBasecamp2] = U"나무그늘 쉼터";
	_strMap[kTunnels] = U"차가운 바위 동굴";
	_strMap[kSmoke] = U"마술 거울";
	_strMap[kMaze] = U"거품의 심연";
	_strMap[kTown] = U"줌비니 동산";
	_strMap[kNewGame] = U"새 게임";
	_strMap[kPracticeMode] = U"연습 해보기";
	_strMap[kContinueJourney] = U"여행 계속 하기";
	_strMap[kPracticeTitle] = U"연습 모드";
	_strMap[kPracticeDesc1] = U" 연습을 하려면:";
	_strMap[kPracticeDesc2] = U" 단계를 선택하고";
	_strMap[kPracticeDesc3] = U" 지도상에서 원하는";
	_strMap[kPracticeDesc4] = U" 위치를 누르세요.";
	_strMap[kTerrainKey] = U"난이도 구분";
	_strMap[kChooseLevel] = U"난이도 선택";
	_strMap[kLevel1] = U"1단계";
	_strMap[kLevel2] = U"2단계";
	_strMap[kLevel3] = U"3단계";
	_strMap[kLevel4] = U"4단계";
	_strMap[kRoute1] = U"희망의 길목";
	_strMap[kRoute2] = U"손에 손잡고";
	_strMap[kRoute3] = U"어둠의 숲을 지나";
	_strMap[kRoute4] = U"절망의 산맥";
	_strMap[kXferVillePopulation] = U"줌비니 동산\r인구 ";
	_strMap[kMemorialJanuary] = U"1월";
	_strMap[kMemorialFebruary] = U"2월";
	_strMap[kMemorialMarch] = U"3월";
	_strMap[kMemorialApril] = U"4월";
	_strMap[kMemorialMay] = U"5월";
	_strMap[kMemorialJune] = U"6월";
	_strMap[kMemorialJuly] = U"7월";
	_strMap[kMemorialAugust] = U"8월";
	_strMap[kMemorialSeptember] = U"9월";
	_strMap[kMemorialOctober] = U"10월";
	_strMap[kMemorialNovember] = U"11월";
	_strMap[kMemorialDecember] = U"12월";
	_strMap[kMemorialWhenLevel] = U"그들이 여행을 했던 단계는 ";
	_strMap[kMemorialHonorMonument] = U"이 기념비는 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorWindmill] = U"이 풍차는 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorObservatory] = U"이 관측소는 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorBowlingAlley] = U"이 볼링장은 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorGeneralStore] = U"이 상점은 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorSwimmingPool] = U"이 수영장은 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorPlayground] = U"이 운동장은 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorBandShell] = U"이 공연장은 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorSchool] = U"이 학교는 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorLibrary] = U"이 도서관은 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorFire] = U"이 소방서는 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorOpera] = U"이 오페라 하우스는 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorCityHall] = U"이 시청은 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorClockTower] = U"이 시계탑은 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorMuseum] = U"이 박물관은 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialHonorCourt] = U"이 법원은 아래의 줌비니들을\r기념하기 위해 세워졌다:";
	_strMap[kMemorialRoute1Level1] = U"알레르기 절벽을 조심스럽게 지나,\r차가운 바위 동굴을\r순항해서,\r대식가 퉁퉁이를 달래가며\r여행했던\r착한 줌비니들";
	_strMap[kMemorialRoute1Level2] = U"구름다리를 용감히 건너,\r바위 얼굴 문지기들을\r웃겨 가며,\r피자 파티에서\r깐깐이를 상대해 \r당당히 이겼던 줌비니들";
	_strMap[kMemorialRoute1Level3] = U"알레르기 절벽을 가볍게 지나,\r동굴지기들의 눈치를 봐가며, \r피자 파티에서 덩구리를 \r잔뜩 먹여 놓고\r여행을 마친\r우리의 줌비니들";
	_strMap[kMemorialRoute1Level4] = U"알레르기 절벽을 압도하고,무뚝뚝한\r동굴지기들을 굴복시키며,\r까다로운 피자 먹보들을 \r달래가며\r여행했던 줌비니들";
	_strMap[kMemorialRoute2Level1] = U"노지기 선장을 잠재우고\r두꺼비 등에 올라타기도 하며,\r소슬 바위를 뛰어 넘은\r용감했던 줌비니들";
	_strMap[kMemorialRoute2Level2] = U"나룻배를 탈없이 타고,\r두꺼비 연못을\r단숨에 건너,\r서로를 잘 이어가며\r여행을 끝낸 줌비니들";
	_strMap[kMemorialRoute2Level3] = U"꾀를 잘 써서 통통나루를 건너,\r두꺼비 연못을 \r살금살금 지나,\r소슬 바위 엘리베이터를 타고\r줌비니 동산에 도착한\r줌비니들 ";
	_strMap[kMemorialRoute2Level4] = U"노지기 선장을 잠재우고\r두꺼비 등에 올라타기도 하며,\r소슬 바위를 뛰어 넘은\r용감했던 줌비니들";
	_strMap[kMemorialRoute3Level1] = U"심술꾸러기 삐딱이들을 떨어뜨리고,\r숲 속의 방에 들러\r달콤한 꿈을 꾸며\r널뛰기 절벽에 이르러\r진흙 대포를 신나게 쏘아대며\r여행을 마친 줌비니들";
	_strMap[kMemorialRoute3Level2] = U"삐따기들을 혼란에 빠뜨리고,\r숲속의 방에서도\r우물쭈물거리지 않고,\r널뛰기 절벽을\r완벽하게 뛰어 넘어\r동산을 찾은 줌비니들";
	_strMap[kMemorialRoute3Level3] = U"삐따기들을 날려 보내고,\r방을 찾기 위해\r모진 고생을 하며,\r돌벽을 가뿐이 뛰어 넘었던\r용맹스런 줌비니들";
	_strMap[kMemorialRoute3Level4] = U"삐따기들의 허를 찌르고,\r숲 속의 방에서\r제대로 방을 찾았고,\r널뛰기 절벽을\r눈감고 뛰어 넘은\r지혜로운 줌비니들";
	_strMap[kMemorialRoute4Level1] = U"사자 동굴에서도 당황하지 않고,\r수정 거울의 비밀을\r풀어 가며,\r거품의 심연을\r날듯이 건너 온\r줌비니 형제들";
	_strMap[kMemorialRoute4Level2] = U"사자 동굴에서\r서로를 의지해 가며,\r마법의 거울에서도\r흔들림 없이,\r수레를 타고\r거품에 몸을 싣고\r어려움을 극복한 줌비니들";
	_strMap[kMemorialRoute4Level3] = U"사자 동굴의 암호를 해독하고,\r수정 거울에서도\r실수를 저지르지 않고,\r거품의 심연을\r요리조리 피해\r이곳에 도착한 줌비니들";
	_strMap[kMemorialRoute4Level4] = U"기어이 사자의 발톱을 들게 하고,\r마법의 거울을 \r줄지어 빠져 나와,\r거품의 비밀을\r완벽하게 파헤져 버린\r현명한 줌비니들 ";
	_strMap[kDialogBodyGoMapWillLost] = U"지도를 보면, 지금 있는 줌비니들을 잃어버리게 됩니다.";
	_strMap[kDialogButtonLoseThem] = U"지도 보기";
	_strMap[kDialogButtonKeepThem] = U"되돌아가기";
	_strMap[kDialogButtonOkay] = U"확인";
	_strMap[kDialogButtonCancel] = U"취소";
	_strMap[kDialogButtonYes] = U"예";
	_strMap[kDialogButtonNo] = U"아니오";
	_strMap[kDialogButtonQuit] = U"아니오"; // Not a typo, Arisumedia translated QUIT as 아니오
	_strMap[kDialogButtonLoad] = U"불러오기";
	_strMap[kDialogButtonSave] = U"저장하기";
	_strMap[kOptionsTitle] = U"설정";
	_strMap[kOptionsLegendOn] = U"= 켬";
	_strMap[kOptionsLegendOff] = U"= 끔";
	_strMap[kOptionsToggle] = U"스위치:";
	_strMap[kOptionsNewGame] = U"새로 시작 (CTRL N)";
	_strMap[kOptionsLoadGame] = U"불러오기 (CTRL L)";
	_strMap[kOptionsSaveGame] = U"저장하기 (CTRL S)";
	_strMap[kOptionsQuit] = U"끝내기 (CTRL Q)";
	_strMap[kOptionsSound] = U"음향 효과 (CTRL D)";
	_strMap[kOptionsMusic] = U"배경 음악 (CTRL B)";
	_strMap[kOptionsStickyMouse] = U"끈끈이 마우스 (CTRL J)";
	_strMap[kOptionsTransitions] = U"화면 전환 (CTRL T)";
	_strMap[kOptionsCredits] = U"만든사람들";
	_strMap[kDialogBodyNoSavedGames] = U"저장된 게임이 없습니다.";
	_strMap[kDialogBodyCreateAndSaveNewGame] = U"현재 게임이 저장되지 않았습니다.\r새롭게 시작하시겠습니까 ?";
	_strMap[kDialogButtonNewGame] = U"새로운 게임";
	_strMap[kDialogButtonReplaceTitle] = U"덮어쓰기";
	_strMap[kDialogTitleSave] = U"저장하기";
	_strMap[kDialogTitleSaveAs] = U"다른 이름으로 저장";
	_strMap[kDialogTitleLoad] = U"불러오기";
	_strMap[kDialogBodyReplaceGame] = U"이전에 저장된 게임을\r현재 게임으로 대체하시겠습니까?\r\" ";
	_strMap[kDialogBodySaveCurrentGame] = U"현재 게임을 저장하시겠습니까?";
	_strMap[kDialogBodySaveDirtyGame] = U"현재 게임이 저장되지 않았습니다.\r현재 게임을 저장하시겠습니까?";
	_strMap[kDialogBodyCannotSaveInPractice] = U"연습모드에서는 게임을 저장할 수 없습니다.";
	_strMap[kDialogBodyCreateNewGame] = U"새 게임을 만드시겠습니까?";
	_strMap[kDialogBodyCannotSaveMoreGame] = U"게임은 52개까지 저장할 수 있습니다.";
	_strMap[kDialogBodyCannotLoadInPractice] = U"연습모드에서는 저장된 게임을 실행할 수 없습니다.";
	_strMap[kDialogBodyCannotCreateNewInPractice] = U"연습모드에서는 새 게임을 시작할 수 없습니다.";
	_strMap[kDialogBodyNewGame] = U"새 게임";
	_strMap[kDialogBodyReallyQuit] = U"정말 끝내시겠습니까?";
	_strMap[kDialogBodySaveBeforeQuit] = U"게임이 저장되지 않았습니다.\r그만두기 전에 게임을 \r저장하시겠습니까?";
	_strMap[kDialogHelpTitle] = U"도움말";
	_strMap[kDialogButtonPrev] = U"이전";
	_strMap[kDialogButtonNext] = U"다음";
	_strMap[kDialogHelpLevel] = U"단계";
	_strMap[kNotiBoxMusicOn] = U"배경 음악 켬";
	_strMap[kNotiBoxMusicOff] = U"배경 음악 끔";
	_strMap[kNotiBoxSoundOn] = U"음향 효과 켬";
	_strMap[kNotiBoxSoundOff] = U"음향 효과 끔";
	_strMap[kNotiBoxLessAction] = U"less action";
	_strMap[kNotiBoxMoreAction] = U"more action";
	_strMap[kNotiBoxHideCursor] = U"커서 숨기기";
	_strMap[kNotiBoxShowCursor] = U"커서 보이기";
	_strMap[kNotiBoxStickeyMouse] = U"끈끈이 마우스";
	_strMap[kNotiBoxNonStickeyMouse] = U"끈끈이 해제";
	_strMap[kNotiBoxTransitionsOn] = U"장면전환 켬";
	_strMap[kNotiBoxTransitionsOff] = U"장면전환 끔";
	_strMap[kNotiBoxAutoStickeyOn] = U"자동 끈끈이 켜기";
	_strMap[kNotiBoxAutoStickeyOff] = U"자동 끈끈이 끄기";
}
#endif

void ZoombiniText::initKoreanStrings() {
	if (!_vm->isGameVariant(GF_ZMB_TLC)) {
		// ScummVM addition for v1.x version
		_strMap[kDialogBodyRemoveGame] = U"이 저장된 게임을\r삭제하시겠습니까?";
	}
}

#if 0
void ZoombiniText::getKoreanCredits(Common::Array<CreditParagraph> &paragraphs) const {
	getEnglishCredits(paragraphs);

	// Patch "SPECIAL THANKS TO:" section
	CreditParagraph &paragraph = paragraphs[paragraphs.size() - 5];
	assert(0 < paragraph._lines.size());
	assert(paragraph._lines[0].equals(Common::U32String("SPECIAL THANKS TO:")));

	// Should be placed between "alex tkaczevski" ~ "BrØderbund's QA department" without any blank lines.
	Common::Array<Common::U32String> lines;
	lines.push_back(Common::U32String("줌비니 수학 논리 여행 한글판 제작팀", Common::kUtf8));
	lines.push_back(Common::U32String("황인영(기획)", Common::kUtf8));
	lines.push_back(Common::U32String("이덕용(그래픽 디자이너)", Common::kUtf8));
	lines.push_back(Common::U32String("김주옥(음악,편집)", Common::kUtf8));
	lines.push_back(Common::U32String("김희성(프로그래머)", Common::kUtf8));
	lines.push_back(Common::U32String("이민선(프로그래머", Common::kUtf8));
	lines.push_back(Common::U32String("이재경(프로그래머)", Common::kUtf8));
	lines.push_back(Common::U32String("박영은(프로그래머)", Common::kUtf8));
	lines.push_back(Common::U32String("최재훈(프로그래머)", Common::kUtf8));
	lines.push_back(Common::U32String("이유권(QA)", Common::kUtf8));
	assert(13 < paragraph._lines.size());
	assert(paragraph._lines[13].equals(Common::U32String("alex tkaczevski")));
	paragraph._lines.insert_at(14, lines);
}

#endif

} // End of namespace Mohawk
