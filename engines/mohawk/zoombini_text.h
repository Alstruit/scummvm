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

#include "common/hash-str.h"
#include "common/hashmap.h"
#include "common/language.h"
#include "common/scummsys.h"
#include "common/str-enc.h"
#include "graphics/fontman.h"

#include "mohawk/resource.h"

namespace Common {
class SeekableReadStream;
}

namespace Mohawk {

class TTFLoader;
class MohawkEngine_Zoombini;
struct ExeTextEntry;
struct ExeCreditPointerRange;
struct ExeTextSource;

struct CreditLineAddress {
	int groupIndex = -1;
	int inGroupLineIndex = -1;

	CreditLineAddress() {}
	CreditLineAddress(int sourceGroupIndex, int sourceInGroupLineIndex) : groupIndex(sourceGroupIndex), inGroupLineIndex(sourceInGroupLineIndex) {}

	bool isValid() const {
		return 0 <= groupIndex && 0 <= inGroupLineIndex;
	}
};

struct CreditLineAddressHash : public Common::UnaryFunction<CreditLineAddress, uint> {
	uint operator()(const CreditLineAddress &key) const {
		return (Common::Hash<int>()(key.groupIndex) * 1009u) ^ Common::Hash<int>()(key.inGroupLineIndex);
	}
};

struct CreditLineAddressEqual : public Common::BinaryFunction<CreditLineAddress, CreditLineAddress, bool> {
	bool operator()(const CreditLineAddress &left, const CreditLineAddress &right) const {
		return left.groupIndex == right.groupIndex && left.inGroupLineIndex == right.inGroupLineIndex;
	}
};

typedef Common::HashMap<CreditLineAddress, Common::U32String, CreditLineAddressHash, CreditLineAddressEqual> CreditLinePatchMap;
typedef Common::HashMap<CreditLineAddress, uint32, CreditLineAddressHash, CreditLineAddressEqual> CreditParagraphSplitMap;

/**
 * Extract the localized hard-coded strings from the Zoombinis executable, and provide access to them by key.
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
	static Common::String formatCreditLineKey(const CreditLineAddress &address);
	static Common::String formatCreditLineKey(uint32 paragraphIndex, uint32 lineIndex);
	static bool parseCreditLineKey(const Common::String &creditKey, CreditLineAddress &address);
	static bool parseCreditLineKey(const Common::String &creditKey, uint32 &paragraphIndex, uint32 &lineIndex);

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
		kOptionsHelpAudio,  // TLC-only English text key.
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
		kNotiBoxHelpAudioOn,   // TLC-only English text key.
		kNotiBoxHelpAudioOff,  // TLC-only English text key.
		kNotiBoxTouchSenseOn,  // TLC-only English text key; ScummVM does not implement TouchSense.
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

		CreditParagraph() {}
		CreditParagraph(const Common::Array<Common::U32String> &lines, uint32 blankLineCount) : _lines(lines), _blankLineCount(blankLineCount) {}

		uint32 getTotalLineCount() const { return _lines.size() + _blankLineCount; }
	};

	struct LocalizedString {
		uint32 _key = 0;
		Common::U32String _text;

		LocalizedString() {}
		LocalizedString(uint32 key, const Common::U32String &text) : _key(key), _text(text) {}
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
	Common::U32String toU32String(const Common::String &str) const;
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
	void getLocalizedStrings(Common::Array<LocalizedString> &strings) const;
	void getLocalizedCredits(Common::Array<CreditParagraph> &paragraphs) const;

	bool patchLocalizedText(const Common::String &textKey, const Common::U32String &text);
	bool patchLocalizedText(const Common::String &textKey, const char *utf8Text);
	bool patchLocalizedTexts(const Common::HashMap<Common::String, Common::U32String> &patches);
	bool patchLocalizedTexts(const Common::HashMap<Common::String, Common::String> &patches);
	void patchLocalizedString(uint32 textKey, const Common::U32String &text);
	void patchLocalizedString(uint32 textKey, const char *utf8Text);
	bool patchCreditLine(uint32 paragraphIndex, uint32 lineIndex, const Common::U32String &text);
	bool patchCreditLine(uint32 paragraphIndex, uint32 lineIndex, const char *utf8Text);
	bool patchCreditLine(const Common::String &creditKey, const Common::U32String &text);
	bool patchCreditLine(const Common::String &creditKey, const char *utf8Text);
	bool splitCreditParagraph(uint32 paragraphIndex, uint32 lineIndex, uint32 newParagraphBlankLineCount);
	bool splitCreditParagraph(const Common::String &creditKey, uint32 newParagraphBlankLineCount);
	bool patchCreditParagraph(uint32 paragraphIndex, const CreditParagraph &paragraph);
	void patchLocalizedCredits(const Common::Array<CreditParagraph> &paragraphs);

private:
	MohawkEngine_Zoombini *_vm;
	const Graphics::Font *loadFont(const Common::Array<TTFLoader *> &optimalTTFLoaders, const Common::Array<TTFLoader *> &fallbackTTFLoaders, int point, bool showWarnMsgBox, Common::String &cacheName);

	void initPageKeyMap();
	void initLocalizedCredits();
	void initEnglishStrings();
	void initEnglishTlcStrings();
	bool initOriginalExecutableStrings();
	void applyOriginalExecutableTextPatches(const Common::HashMap<uint32, Common::U32String> &textPatches);
	static bool applyOriginalExecutableCreditLinePatches(Common::Array<CreditParagraph> &creditParagraphs, const CreditLinePatchMap &creditLinePatches);
	static bool applyCreditParagraphSplit(Common::Array<CreditParagraph> &creditParagraphs, const CreditLineAddress &address, uint32 newParagraphBlankLineCount);
	static bool applyCreditParagraphSplitPatches(Common::Array<CreditParagraph> &creditParagraphs, const CreditParagraphSplitMap &creditParagraphSplits);
	static bool readExecutableData(Common::SeekableReadStream *exeStream, Common::Array<byte> &data);
	static int64 getExeTextEntryOffset(const ExeTextSource &source, const ExeTextEntry &entry);
	static bool findBytes(const Common::Array<byte> &data, const char *needle, uint32 &offset);
	static Common::U32String decodeCreditStringBytes(const byte *bytes, uint32 length, Common::CodePage codePage);
	static bool readExecutableStringAt(const Common::Array<byte> &data, uint32 offset, Common::CodePage codePage, Common::U32String &text);
	static bool isCreditTerminator(const Common::U32String &text);
	static bool readCreditStringsFromAnchor(const Common::Array<byte> &data, Common::CodePage codePage, const char *anchor, Common::Array<Common::U32String> &creditStrings);
	static bool readCreditStringsFromPointerTable(const Common::Array<byte> &data, const ExeTextSource &source, Common::Array<Common::U32String> &creditStrings);
	static bool buildCreditParagraphsFromStrings(const Common::Array<Common::U32String> &creditStrings, Common::Array<CreditParagraph> &creditParagraphs);
	static bool loadOriginalExecutableCredits(const Common::Array<byte> &data, const ExeTextSource &source, Common::Array<CreditParagraph> &creditParagraphs);
	static bool parseUnsignedDecimalString(const Common::String &text, uint32 &value);
	static Common::HashMap<uint32, Common::U32String> buildEnglishExeTextPatches();
	static CreditLinePatchMap buildEnglishExeCreditLinePatches();
	static CreditLinePatchMap buildKoreanExeCreditLinePatches();
	static CreditParagraphSplitMap buildCreditParagraphSplits();
	template<size_t size>
	static void addCreditParagraph(Common::Array<CreditParagraph> &paragraphs, const char *const (&creditLines)[size], uint32 blankLineCount) {
		Common::Array<Common::U32String> lines;
		for (uint i = 0; i < size; i++)
			lines.push_back(Common::U32String(creditLines[i], Common::kUtf8));
		paragraphs.push_back(CreditParagraph(lines, blankLineCount));
	}
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
	Common::Array<CreditParagraph> _creditParagraphs;
	Common::HashMap<ZoombiniPageType, Key> _pageKeyMap;
};

} // End of namespace Mohawk

#endif
