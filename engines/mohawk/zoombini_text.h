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

#ifndef MOHAWK_ZOOMBINI_TEXT_H
#define MOHAWK_ZOOMBINI_TEXT_H

#include "common/scummsys.h"
#include "common/language.h"
#include "common/str-enc.h"
#include "common/hashmap.h"
#include "graphics/fontman.h"

#include "mohawk/resource.h"

namespace Mohawk {

class TTFLoader;
class MohawkEngine_Zoombini;

/**
 * Localize hard-coded strings of Zoombini
 * 
 * Unfortunately, some of the Zoombini texts are hard-coded in the game code.
 * This class provides localized versions of those strings.
 */
class ZoombiniText {

public:
	ZoombiniText(MohawkEngine_Zoombini *vm, Common::Language lang);
	~ZoombiniText();

	/**
	 * Tokenize a Common::String by CRLF, CR or LF.
	 */
	static Common::Array<Common::String> tokenizeLines(const Common::String &text);
	/**
	 * Tokenize a Common::U32String by CRLF, CR or LF.
	 */
	static Common::Array<Common::U32String> tokenizeLines(const Common::U32String &text);

	enum Key : uint32 {
		kNone = 0,
		// RODMAP: Page Names
		kPicker = 1,
		kBridge = 2,
		kCaves = 3,
		kPizza = 4,
		kBasecamp1 = 5,
		kFerry = 6,
		kLilly = 7,
		kSlides = 8,
		kFleens = 9,
		kHotel = 10,
		kNet = 11,
		kBasecamp2 = 12,
		kTunnels = 13,
		kSmoke = 14,
		kMaze = 16,
		kTown,
		// RODMAP: Practice/Journey Stat
		kNewGame = 100,
		kPracticeMode,
		kContinueJourney,
		kPracticeTitle,
		kPracticeDesc1,
		kPracticeDesc2,
		kPracticeDesc3,
		kPracticeDesc4,
		// RODMAP: Legend
		kTerrainKey,
		kChooseLevel,
		kLevel1,
		kLevel2,
		kLevel3,
		kLevel4,
		// RODMAP: Route Names
		kRoute1 = 200,
		kRoute2,
		kRoute3,
		kRoute4,
		// XFER: Entrance to Zoombiniville
		kXferVillePopulation,
		// TOWN: Memorial
		kMemorialJanuary = 300,
		kMemorialFebruary,
		kMemorialMarch,
		kMemorialApril,
		kMemorialMay,
		kMemorialJune,
		kMemorialJuly,
		kMemorialAugust,
		kMemorialSeptember,
		kMemorialOctober,
		kMemorialNovember,
		kMemorialDecember,
		kMemorialWhenLevel,
		kMemorialHonorMonument,
		kMemorialHonorWindmill,
		kMemorialHonorObservatory,
		kMemorialHonorBowlingAlley,
		kMemorialHonorGeneralStore,
		kMemorialHonorSwimmingPool,
		kMemorialHonorPlayground,
		kMemorialHonorBandShell,
		kMemorialHonorSchool,
		kMemorialHonorLibrary,
		kMemorialHonorFire,
		kMemorialHonorOpera,
		kMemorialHonorCityHall,
		kMemorialHonorClockTower,
		kMemorialHonorMuseum,
		kMemorialHonorCourt,
		kMemorialRoute1Level1,
		kMemorialRoute1Level2,
		kMemorialRoute1Level3,
		kMemorialRoute1Level4,
		kMemorialRoute2Level1,
		kMemorialRoute2Level2,
		kMemorialRoute2Level3,
		kMemorialRoute2Level4,
		kMemorialRoute3Level1,
		kMemorialRoute3Level2,
		kMemorialRoute3Level3,
		kMemorialRoute3Level4,
		kMemorialRoute4Level1,
		kMemorialRoute4Level2,
		kMemorialRoute4Level3,
		kMemorialRoute4Level4,
		// DIALOG: OPTIONS
		kOptionsTitle = 400,
		kOptionsLegendOn,
		kOptionsLegendOff,
		kOptionsToggle,
		kOptionsNewGame,
		kOptionsLoadGame,
		kOptionsSaveGame,
		kOptionsQuit,
		kOptionsSound,
		kOptionsMusic,
		kOptionsStickyMouse,
		kOptionsTransitions,
		kOptionsCredits,
		kOptionsHelpAudio, // TLC-only English text key.
		kOptionsTouchSense, // TLC-only English text key; ScummVM does not implement TouchSense.
		// DIALOG: MsgBox
		kDialogTitleSave = 500,
		kDialogTitleSaveAs,
		kDialogTitleLoad,
		kDialogBodyGoMapWillLost,
		kDialogBodyNoSavedGames,
		kDialogBodyCreateAndSaveNewGame,
		kDialogBodyReplaceGame,
		kDialogBodySaveCurrentGame,
		kDialogBodySaveDirtyGame,
		kDialogBodyCannotSaveInPractice,
		kDialogBodyCreateNewGame,
		kDialogBodyCannotSaveMoreGame,
		kDialogBodyCannotLoadInPractice,
		kDialogBodyCannotCreateNewInPractice,
		kDialogBodyNewGame,
		kDialogBodyReallyQuit,
		kDialogBodySaveBeforeQuit,
		kDialogBodyRemoveGame, // TLC-only English text key.
		kDialogButtonNewGame = 600,
		kDialogButtonReplaceTitle,
		kDialogButtonLoseThem,
		kDialogButtonKeepThem,
		kDialogButtonOkay,
		kDialogButtonCancel,
		kDialogButtonYes,
		kDialogButtonNo,
		kDialogButtonQuit,
		kDialogButtonLoad,
		kDialogButtonSave,
		kDialogButtonReplace,
		// DIALOG: Help
		kDialogHelpTitle,
		kDialogHelpLevel,
		kDialogButtonPrev,
		kDialogButtonNext,
		// Notification Box
		kNotiBoxMusicOn,
		kNotiBoxMusicOff,
		kNotiBoxSoundOn,
		kNotiBoxSoundOff,
		kNotiBoxLessAction,
		kNotiBoxMoreAction,
		kNotiBoxHideCursor,
		kNotiBoxShowCursor,
		kNotiBoxStickeyMouse,
		kNotiBoxNonStickeyMouse,
		kNotiBoxTransitionsOn,
		kNotiBoxTransitionsOff,
		kNotiBoxAutoStickeyOn,
		kNotiBoxAutoStickeyOff,
		kNotiBoxHelpAudioOn, // TLC-only English text key.
		kNotiBoxHelpAudioOff, // TLC-only English text key.
		kNotiBoxTouchSenseOn, // TLC-only English text key; ScummVM does not implement TouchSense.
		kNotiBoxTouchSenseOff, // TLC-only English text key; ScummVM does not implement TouchSense.
	};

	enum FontKind : uint32 {
		kFontDebugTitle = 0,
		kFontDebugText,
		kFontTitle,
		kFontText,
	};

	struct CreditParagraph {
		Common::Array<Common::U32String> _lines;
		uint32 _blankLineCount = 1;

		CreditParagraph() { }
		CreditParagraph(const Common::Array<Common::U32String> &lines, uint32 blankLineCount) :
			_lines(lines), _blankLineCount(blankLineCount) { }
		
		uint32 getTotalLineCount() const { return _lines.size() + _blankLineCount; }
	};

	Common::CodePage getCodePage() { return _codePage; }
	const Graphics::Font *getTextFont();
	const Graphics::Font *getTitleFont();
	const Graphics::Font *getFont(ZoombiniFontUsage fontUsage);

	// [*] Convert to U32String with the code page
	Common::U32String toU32String(const byte *buf) const;
	Common::U32String toU32String(const byte *buf, int32 len) const;
	Common::U32String toU32String(const char *str) const;
	Common::U32String toU32String(const char *str, int32 len) const;
	Common::U32String toU32String(const Common::String& str) const;
	Common::String fromU32String(const Common::U32String &ustr) const;

	// [*] STRL resource
	bool getStrl(Common::Array<Common::U32String> &outStrs, ZmbResource resource);
	bool getStrl(Common::U32String &outStr, ZmbResource resource, uint16 subStrIdx);
	Common::U32String getZoombiniName(int16 zmbid);
	/**
	 * Loads all 625 Zoombini names into the forward cache (snoidId -> name)
	 * and builds the reverse index (name -> snoidId) in one pass.
	 * Must be called before findZoombiniNameId().
	 */
	void cacheAllZoombiniNames();
	/**
	 * Returns the snoidId (0..624) for the given name, or -1 if not found.
	 * Requires cacheAllZoombiniNames() to have been called first.
	 */
	int16 findZoombiniNameId(const Common::U32String &name) const;
	void clearNameCache();

	/**
	 * Pick the next name for a preview Zoombini.
	 * Korean: draws from the 625-slot STRL name pool, tracking used names in
	 *         _zoombiniNameGeneratedTable (rebuilds the table when exhausted).
	 * English: generates a name procedurally from syllable tables.
	 */
	Common::U32String pickNextZoombiniName();

	Common::U32String getPageName(ZoombiniPageType pageType) const;
	Common::U32String getLocalizedString(uint32 textKey) const;
	void getLocalizedCredits(Common::Array<CreditParagraph> &paragraphs) const;

private:
	MohawkEngine_Zoombini *_vm;	
	const Graphics::Font *loadFont(const Common::Array<TTFLoader *> &optimalTTFLoaders, const Common::Array<TTFLoader *> &fallbackTTFLoaders, int point, bool showWarnMsgBox, Common::String &cacheName);

	void initPageKeyMap();
	void initEnglishStrings();
	void initEnglishTlcStrings();
	bool initOriginalExecutableStrings(const Common::HashMap<uint32, Common::U32String> &builtInStrings);
	void applyOriginalExecutableStringPatches(const Common::HashMap<uint32, Common::U32String> &builtInStrings, const Key *patchKeys, uint patchKeyCount);
	void getEnglishCredits(Common::Array<CreditParagraph> &paragraphs) const;
	void getEnglishTlcCredits(Common::Array<CreditParagraph> &paragraphs) const;
	void initKoreanStrings();
	void getKoreanCredits(Common::Array<CreditParagraph> &paragraphs) const;

	Common::U32String generateRandomName();

	Common::Language _lang;
	Common::CodePage _codePage;

	Common::String _textFontCacheName;
	Common::String _titleFontCacheName;
	int _textFontPoint = 0;
	int _titleFontPoint = 0;
	Common::Array<TTFLoader *> _optimalTTFLoaders;
	Common::Array<TTFLoader *> _fallbackTTFLoaders;

	Common::HashMap<int16, Common::U32String> _nameCache;
	Common::HashMap<Common::U32String, int16> _nameIndexCache;

	Common::HashMap<uint32, Common::U32String> _strMap;
	Common::HashMap<ZoombiniPageType, Key> _pageKeyMap;
};

} // End of namespace Mohawk

#endif
