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

#ifndef MOHAWK_ZOOMBINI_RESOURCE_H
#define MOHAWK_ZOOMBINI_RESOURCE_H

#include "common/scummsys.h"
#include "common/hashmap.h"

namespace Mohawk {

// Engine Debug Flags
enum {
	kZmbDebugVariable = 1,
	kZmbDebugSaveLoad,
	kZmbDebugPage,
	kZmbDebugResource,
	kZmbDebugRender,
	kZmbDebugScript,
	kZmbDebugHelp,
};

/**
 * In Zoombinis, Common archive and page archive cannot share a unified resource id namespace
 * due to a few resource id conflicts, unlike other Mohawk games.
 * Ex) XFER.MHK tBMP 3000 (Background) vs ZOOMBINI.MHK tBMP 3000 (Shapes)
 * So let's separate each namespace.
 */
enum class ZmbArchiveKind: uint16 {
	/**
	 * Resource from active archives (MohawkEngine::_mhks)
	 */
	kPage = 0,
	/**
	 * Resources from ZOOMBINI.MHK
	 */
	kSystem = 1,
};

struct ZmbResource {
	static constexpr uint16 RESOURCE_NONE = 0;

	ZmbArchiveKind _archiveKind = ZmbArchiveKind::kPage;
	uint16 _id = RESOURCE_NONE;

	ZmbResource() = default;
	ZmbResource(ZmbArchiveKind archiveKind, uint16 id) : _archiveKind(archiveKind), _id(id) { }

	static bool parseInt(const char *str, int32 &result);
	static bool parse(const char *str, ZmbResource &outRes);

	bool operator==(const ZmbResource &other) const;
	bool operator!=(const ZmbResource &other) const;
	bool hasId() const;
	Common::String toString() const;
};

enum class ZoombiniPageCategory : uint16 {
	kNone = 0,
	/**
	 * Normal rest page or puzzle page, usually has three buttons.
	 * Clears loaded page archives when closed.
	 * Ex) Rest pages like PICKER, BASECAMP1, BASECAMP2, TOWN
	 * Ex) Route pages like BRIDGE, CAVES, PIZZA, FERRY, LILLY, SLIDES, FLEENS, HOTEL, NET, TUNNELS, SMOKE, MAZE
	 */
	kInteractive = 1,
	/**
	 * Transition pages between puzzles, a simple click or keypress finishes the page.
	 * Clears loaded page archives when closed.
	 * Ex) XFER, LOGO, etc.
	 */
	kTransition = 2,
	/**
	 * Modal dialog pages.
	 * Does not clear loaded page archives when closed.
	 */
	kDialog = 3,
};

enum class ZoombiniPageType : uint16 {
	kNone = 0,
	/**
	 * Map
	 */
	kRodMap = 1,
	/**
	 * Intersection
	 */
	kXfer = 2,
	/**
	 * Zoombini Isle
	 */
	kPicker = 3,
	/**
	 * Shelter Rock
	 */
	kBasecamp1 = 4,
	/**
	 * Shade Tree
	 */
	kBasecamp2 = 5,
	/**
	 * Zoombiniville
	 */
	kTown = 6,
	// Route: The Big, the Bad, and the Hungry
	/**
	 * Allergic Cliffs
	 */
	kBridge = 7,
	/**
	 * The Lion's Lair (TUNNELS.MHK)
	 */
	kTunnels = 8,
	/**
	 * Pizza Pass
	 */
	kPizza = 9,
	// Route: Who's Bayou (North)
	/**
	 * Captain Cajun's Ferryboat
	 */
	kFerry = 10,
	/**
	 * Titanic Tattooed Toads
	 */
	kLilly = 11,
	/**
	 * Stone Rise
	 */
	kSlides = 12,
	// Route: Deep, Dark Forest (South)
	/**
	 * Fleens
	 */
	kFleens = 13,
	/**
	 * Hotel Dimensia
	 */
	kHotel = 14,
	/**
	 * Mudball Wall
	 */
	kNet = 15,
	// Route: Mountains of Despair
	/**
	 * Stone Cold Caves (CAVES.MHK)
	 */
	kCaves = 16,
	/**
	 * Mirror Machine
	 */
	kSmoke = 17,
	/**
	 * Bubblewonder Abyss
	 */
	kMaze = 18,
	/**
	 * Virtual page for a logo movie
	 */
	kLogo = 19,
	/**
	 * Virtual page for a option dialog
	 */
	kDialogOptions = 20,
	/**
	 * Virtual page for a save/load dialog
	 */
	kDialogSaveLoad = 21,
	/**
	 * Virtual page for a message box dialog
	 */
	kDialogMsgBox = 22,
	/**
	 * Virtual page for a credits dialog
	 */
	kCreditScreen = 23,
	/**
	 * Virtual page for a help dialog
	 */
	kDialogHelp = 24,
	/**
	 * Virtual page for a console debug screen
	 */
	kDialogDebug = 26,
};

// [*] Modal Dialogs
enum class ZoombiniDialogType : uint16 {
	kNone = 0,
	kOptions = 1,
	kLoad = 2,
	kSave = 3,
	kAsk = 4,
	kHelp = 5,
	kCredits = 6,
};

enum class ZoombiniFontUsage : uint32 {
	kFontDebugTitle = 0,
	kFontDebugText,
	kFontTitle,
	kFontText,
};

enum class ZmbEventHandleResult : uint32 {
	kPassthrough = 0,
	kConsumed,
};

enum class ZmbRenderResult : uint32 {
	/**
	 * Render was a success, postRender hook will be called.
	 */
	kRendered = 0,
	/**
	 * Render was skipped, postRender hook will NOT be called.
	 */
	kSkipped = 1,
};

enum class ZoombiniFrameResult : uint32 {
	kSuccess = 0,
	kRedraw,
};

enum class ZoombiniMsgBoxType : uint32 {
	kNone = 0,
	kAlertNoSavedGame,
	kAskCreateAndSaveNewGame,
	kAskCreateNewGame,
	kAskReplaceSave,
	kAskSaveCurrentGame,
	kAskSaveBeforeQuit,
	kAlertCannotSaveInPractice,
	kAlertCannotSaveMoreGames,
	kAlertCannotLoadInPractice,
	kAlertCannotCreateNewInPractice,
	kAskReallyQuit,
	kAskSaveDirtyGame,
	kAskGoMapWillLost,
};

enum class ZoombiniDialogResult : uint32 {
	kNone = 0,
	kYes = 1,
	kNo = 2,
};

static constexpr uint16 ZMB_ENDIAN_MAGIC = 0x006B; // 6B 00 in LE, 00 6B in BE
static constexpr byte ZMB_LITTLE_ENDIAN_MAGIC_BYTES[2] = { 0x6B, 0x00 };
static constexpr byte ZMB_BIG_ENDIAN_MAGIC_BYTES[2] = { 0x00, 0x6B };

} // End of namespace Mohawk

namespace Common {

template<>
struct Hash<Mohawk::ZoombiniPageType> {
	uint operator()(const Mohawk::ZoombiniPageType k) const {
		return static_cast<uint>(k);
	}
};

} // End of namesapce Common

#endif // MOHAWK_ZOOMBINI_H
