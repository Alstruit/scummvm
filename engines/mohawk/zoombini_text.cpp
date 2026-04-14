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

#include "mohawk/resource.h"
#include "mohawk/ttfloader.h"
#include "mohawk/zoombini.h"
#include "mohawk/zoombini_page.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_state.h"
#include "mohawk/zoombini_text.h"

namespace Mohawk {

ZoombiniText::ZoombiniText(MohawkEngine_Zoombini *vm, Common::Language lang) : _vm(vm), _lang(lang) {
	// Users have to source the required font themselves!
	Common::String srcInst;
	switch (_lang) {
	case Common::EN_ANY:
	default:
		// Korean Zoombini string resources are encoded as CP1252
		_codePage = Common::kWindows1252;

		// English Zoombini used ConerStone font, bundled in installshield cabs or install location.
		// - 1.1: found in ZBARC16.Z or ZBARC32.Z, or SETUP/data1.cab
		// - 2.0: found in INSTALL/HD/CORNER.TTF
		if (_vm->isGameVariant(GF_ZMB_TLC))
			srcInst = "Please provide '/INSTALL/HD/CORNER.TTF' from the installer disk.";	
		else
			srcInst = "Please provide one of '/ZBARC32.Z', '/ZBARC16.Z', or '/SETUP/data1.cab' from the installer disk.";
		_optimalTTFLoaders.push_back(new FileTTFLoader("CORNER.TTF", "CornerStone", srcInst, false));
		_optimalTTFLoaders.push_back(new ISCabTTFLoader("data1.cab", "CORNER.TTF", "CornerStone", srcInst, false));
		_optimalTTFLoaders.push_back(new ISZTTFLoader("ZBARC32.Z", "CORNER.TTF", "CornerStone", srcInst, false));
		_optimalTTFLoaders.push_back(new ISZTTFLoader("ZBARC16.Z", "CORNER.TTF", "CornerStone", srcInst, false));
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

		// Korean Zoombini used GulimChe (굴림체) font, bundled in every Windows starting from 3.1.
		// To support every Hangul characters, gulim.ttc from Windows 98 or later is recommended.
		// - 1.11: gulim.ttc from Windows
		_optimalTTFLoaders.push_back(new FileTTFLoader("gulim.ttc", "GulimChe", true, 1));
		_textFontPoint = 12;
		_titleFontPoint = 18;
		_fallbackTTFLoaders.push_back(new WinSysTTFLoader("gulim.ttc", "GulimChe", "ScummVM loaded the font from the Windows Font archive, but this is not recommended.", true, 1));
		_fallbackTTFLoaders.push_back(new FileTTFLoader("D2CodingBold.ttf", "D2Coding", true));
		_fallbackTTFLoaders.push_back(new ArchiveTTFLoader("NotoSansKR-Bold.otf", "Noto Sans KR Bold"));

		// Initialize string maps
		initKoreanStrings();
		break;
	}

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

Common::Array<Common::String> ZoombiniText::tokenizeLines(const Common::String &text) {
	Common::Array<Common::String> lines;
	for (size_t lastIdx = 0; lastIdx < text.size(); ) {
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
	for (size_t lastIdx = 0; lastIdx < text.size(); ) {
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

const Graphics::Font *ZoombiniText::getTextFont() {
	return FontMan.getFontByName(_textFontCacheName);
}

const Graphics::Font *ZoombiniText::getTitleFont() {
	return FontMan.getFontByName(_titleFontCacheName);
}

const Graphics::Font *ZoombiniText::getFont(ZoombiniFontUsage fontUsage) {
	switch (fontUsage) {
	case ZoombiniFontUsage::kFontDebugTitle : // For debug console
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
	return toU32String(reinterpret_cast<const char*>(buf));
}

Common::U32String ZoombiniText::toU32String(const byte *buf, int32 len) const {
	return toU32String(reinterpret_cast<const char*>(buf), len);
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
	"a a a "  // a x3
	"e e e e " // e x4
	"i i i "  // i x3
	"o o o "  // o x3
	"u u "    // u x2
	"y "      // y x1
	"ee" "oo" "yo" "ya" "ye" "ei" "ie" "ai"
	"ia" "au" "ua" "uo" "ou" "ae" "ea";

// 32 weighted single consonants (some appear multiple times).
static const char kSingleConsonants[] = "bbccdddfghjkkllmmnnprrssssttvwxz";

// 6 simple vowels, used to replace a trailing consonant blend.
static const char kSimpleVowels[] = "aeiouy";

// 40 consonant blends, 2 bytes each.
static const char kConsonantBlends[] =
	"bl" "br" "ch" "cl" "cr" "dr" "dw" "fl" "fr" "gh"
	"gl" "gr" "kl" "kn" "kr" "kw" "ld" "mp" "nd" "nh"
	"nn" "ph" "pl" "pr" "qu" "qu" "rh" "rn" "sc" "sl"
	"sm" "sn" "sp" "sr" "st" "sw" "th" "tr" "tw" "wr";

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

void ZoombiniText::getLocalizedCredits(Common::Array<CreditParagraph> &paragraphs) const {
	switch (_lang) {
	case Common::EN_ANY:
	case Common::EN_USA:
	case Common::EN_GRB:
	default:
		getEnglishCredits(paragraphs);
		break;
	case Common::KO_KOR:
		getKoreanCredits(paragraphs);
		break;
	}
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
	_strMap[kNotiBoxStickeyMouse] = U"stickey mouse";
	_strMap[kNotiBoxNonStickeyMouse] = U"non-stickey mouse";
	_strMap[kNotiBoxTransitionsOn] = U"transitions on";
	_strMap[kNotiBoxTransitionsOff] = U"transitions off";
	_strMap[kNotiBoxAutoStickeyOn] = U"auto stickey on";
	_strMap[kNotiBoxAutoStickeyOff] = U"auto stickey off";
}

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
	_strMap[kPracticeDesc1] = U"연습을 하려면";
	_strMap[kPracticeDesc2] = U"단계를 선택하고";
	_strMap[kPracticeDesc3] = U"지도상에서 원하는";
	_strMap[kPracticeDesc4] = U"위치를 누르세요";
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
	_strMap[kXferVillePopulation] = U"줌비니 동산\n인구";
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
	_strMap[kMemorialWhenLevel] = U"그들이 여행을 했던 단계는";
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
	_strMap[kMemorialRoute2Level1] = U"노지기 선장을 잠재우고두꺼비 등에 올라타기도 하며,\r소슬 바위를 뛰어 넘은\r용감했던 줌비니들";
	_strMap[kMemorialRoute2Level2] = U"나룻배를 탈없이 타고,\r두꺼비 연못을\r단숨에 건너,\r서로를 잘 이어가며\r여행을 끝낸 줌비니들";
	_strMap[kMemorialRoute2Level3] = U"꾀를 잘 써서 통통나루를 건너,\r두꺼비 연못을 \r살금살금 지나,\r소슬 바위 엘리베이터를 타고\r줌비니 동산에 도착한\r줌비니들";
	_strMap[kMemorialRoute2Level4] = U"노지기 선장을 잠재우고\r두꺼비 등에 올라타기도 하며,\r소슬 바위를 뛰어 넘은\r용감했던 줌비니들";
	_strMap[kMemorialRoute3Level1] = U"심술꾸러기 삐딱이들을 떨어뜨리고,\r숲 속의 방에 들러\r달콤한 꿈을 꾸며\r널뛰기 절벽에 이르러\r진흙 대포를 신나게 쏘아대며\r여행을 마친 줌비니들";
	_strMap[kMemorialRoute3Level2] = U"삐따기들을 혼란에 빠뜨리고,\r숲속의 방에서도\r우물쭈물거리지 않고,\r널뛰기 절벽을\r완벽하게 뛰어 넘어\r동산을 찾은 줌비니들";
	_strMap[kMemorialRoute3Level3] = U"삐따기들을 날려 보내고,\r방을 찾기 위해\r모진 고생을 하며,\r돌벽을 가뿐이 뛰어 넘었던\r용맹스런 줌비니들";
	_strMap[kMemorialRoute3Level4] = U"삐따기들의 허를 찌르고,\r숲 속의 방에서\r제대로 방을 찾았고,\r널뛰기 절벽을\r눈감고 뛰어 넘은\r지혜로운 줌비니들";
	_strMap[kMemorialRoute4Level1] = U"사자 동굴에서도 당황하지 않고,\r수정 거울의 비밀을\r풀어 가며,\r거품의 심연을\r날듯이 건너 온\r줌비니 형제들";
	_strMap[kMemorialRoute4Level2] = U"사자 동굴에서\r서로를 의지해 가며,\r마법의 거울에서도\r흔들림 없이,\r수레를 타고\r거품에 몸을 싣고\r어려움을 극복한 줌비니들";
	_strMap[kMemorialRoute4Level3] = U"사자 동굴의 암호를 해독하고,\r수정 거울에서도\r실수를 저지르지 않고,\r거품의 심연을\r요리조리 피해\r이곳에 도착한 줌비니들";
	_strMap[kMemorialRoute4Level4] = U"기어이 사자의 발톱을 들게 하고,\r마법의 거울을 \r줄지어 빠져 나와,\r거품의 비밀을\r완벽하게 파헤져 버린\r현명한 줌비니들";
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
	_strMap[kDialogBodyReplaceGame] = U"이전에 저장된 게임을\r현재 게임으로 대체하시겠습니까?";
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

} // End of namespace Mohawk
