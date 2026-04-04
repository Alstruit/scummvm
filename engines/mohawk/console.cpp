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

#include "mohawk/console.h"
#include "mohawk/cursors.h"
#include "mohawk/livingbooks.h"
#include "mohawk/resource.h"
#include "mohawk/sound.h"
#include "mohawk/video.h"

#include "common/system.h"
#include "common/textconsole.h"

#ifdef ENABLE_CSTIME
#include "mohawk/cstime.h"
#endif

#ifdef ENABLE_MYST
#include "mohawk/myst.h"
#include "mohawk/myst_areas.h"
#include "mohawk/myst_card.h"
#include "mohawk/myst_graphics.h"
#include "mohawk/myst_scripts.h"
#include "mohawk/myst_sound.h"
#endif

#ifdef ENABLE_RIVEN
#include "mohawk/riven.h"
#include "mohawk/riven_card.h"
#include "mohawk/riven_sound.h"
#include "mohawk/riven_stack.h"
#include "mohawk/riven_stacks/domespit.h"
#endif

#ifdef ENABLE_ZOOMBINI
#include "common/file.h"
#include "graphics/paletteman.h"
#include "image/bmp.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_page.h"
#include "mohawk/zoombini_debug.h"
#endif

namespace Mohawk {

#ifdef ENABLE_MYST

MystConsole::MystConsole(MohawkEngine_Myst *vm) : GUI::Debugger(), _vm(vm) {
	registerCmd("changeCard",			WRAP_METHOD(MystConsole, Cmd_ChangeCard));
	registerCmd("curCard",			WRAP_METHOD(MystConsole, Cmd_CurCard));
	registerCmd("var",				WRAP_METHOD(MystConsole, Cmd_Var));
	registerCmd("curStack",			WRAP_METHOD(MystConsole, Cmd_CurStack));
	registerCmd("changeStack",		WRAP_METHOD(MystConsole, Cmd_ChangeStack));
	registerCmd("drawImage",			WRAP_METHOD(MystConsole, Cmd_DrawImage));
	registerCmd("drawRect",			WRAP_METHOD(MystConsole, Cmd_DrawRect));
	registerCmd("setResourceEnable",	WRAP_METHOD(MystConsole, Cmd_SetResourceEnable));
	registerCmd("playSound",			WRAP_METHOD(MystConsole, Cmd_PlaySound));
	registerCmd("stopSound",			WRAP_METHOD(MystConsole, Cmd_StopSound));
	registerCmd("playMovie",			WRAP_METHOD(MystConsole, Cmd_PlayMovie));
	registerCmd("disableInitOpcodes",	WRAP_METHOD(MystConsole, Cmd_DisableInitOpcodes));
	registerCmd("cache",				WRAP_METHOD(MystConsole, Cmd_Cache));
	registerCmd("resources",			WRAP_METHOD(MystConsole, Cmd_Resources));
	registerCmd("quickTest",            WRAP_METHOD(MystConsole, Cmd_QuickTest));
	registerVar("show_resource_rects",  &_vm->_showResourceRects);
}

MystConsole::~MystConsole() {
}

bool MystConsole::Cmd_ChangeCard(int argc, const char **argv) {
	if (argc < 2) {
		debugPrintf("Usage: changeCard <card>\n");
		return true;
	}

	_vm->_sound->stopEffect();
	_vm->changeToCard((uint16)atoi(argv[1]), kTransitionCopy);

	return false;
}

bool MystConsole::Cmd_CurCard(int argc, const char **argv) {
	debugPrintf("Current Card: %d\n", _vm->getCard()->getId());
	return true;
}

bool MystConsole::Cmd_Var(int argc, const char **argv) {
	if (argc == 1) {
		debugPrintf("Usage: var <var> (<value>)\n");
		return true;
	}

	if (argc > 2)
		_vm->_stack->setVarValue((uint16)atoi(argv[1]), (uint16)atoi(argv[2]));

	debugPrintf("%d = %d\n", (uint16)atoi(argv[1]), _vm->_stack->getVar((uint16)atoi(argv[1])));

	return true;
}

static const char *mystStackNames[12] = {
	"Channelwood",
	"Credits",
	"Demo",
	"D'ni",
	"Intro",
	"MakingOf",
	"Mechanical",
	"Myst",
	"Selenitic",
	"Slideshow",
	"SneakPreview",
	"Stoneship"
};

static const uint16 default_start_card[12] = {
	3137,
	10000,
	2000,
	5038,
	1,
	1,
	6122,
	4134,
	1282,
	1000,
	3000,
	2029
};

bool MystConsole::Cmd_CurStack(int argc, const char **argv) {
	debugPrintf("Current Stack: %s\n", mystStackNames[_vm->_stack->getStackId()]);
	return true;
}

bool MystConsole::Cmd_ChangeStack(int argc, const char **argv) {
	if (argc != 2 && argc != 3) {
		debugPrintf("Usage: changeStack <stack> [<card>]\n\n");
		debugPrintf("Stacks:\n=======\n");

		for (byte i = 0; i < ARRAYSIZE(mystStackNames); i++)
			debugPrintf(" %s\n", mystStackNames[i]);

		debugPrintf("\n");

		return true;
	}

	byte stackNum = 0;

	for (byte i = 1; i <= ARRAYSIZE(mystStackNames); i++)
		if (!scumm_stricmp(argv[1], mystStackNames[i - 1])) {
			stackNum = i;
			break;
		}

	if (!stackNum) {
		debugPrintf("\'%s\' is not a stack name!\n", argv[1]);
		return true;
	}

	// We need to stop any playing sound when we change the stack
	// as the next card could continue playing it if it.
	_vm->_sound->stopEffect();

	uint16 card = 0;
	if (argc == 3)
		card = (uint16)atoi(argv[2]);
	else
		card = default_start_card[stackNum - 1];

	_vm->changeToStack(static_cast<MystStack>(stackNum - 1), card, 0, 0);

	return false;
}

bool MystConsole::Cmd_DrawImage(int argc, const char **argv) {
	if (argc != 2 && argc != 6) {
		debugPrintf("Usage: drawImage <image> [<left> <top> <right> <bottom>]\n");
		return true;
	}

	Common::Rect rect;

	if (argc == 2)
		rect = Common::Rect(0, 0, 544, 333);
	else
		rect = Common::Rect((uint16)atoi(argv[2]), (uint16)atoi(argv[3]), (uint16)atoi(argv[4]), (uint16)atoi(argv[5]));

	_vm->_gfx->copyImageToScreen((uint16)atoi(argv[1]), rect);
	return false;
}

bool MystConsole::Cmd_DrawRect(int argc, const char **argv) {
	if (argc != 5 && argc != 2) {
		debugPrintf("Usage: drawRect <left> <top> <right> <bottom>\n");
		debugPrintf("Usage: drawRect <resource id>\n");
		return true;
	}

	if (argc == 5) {
		_vm->_gfx->drawRect(Common::Rect((uint16)atoi(argv[1]), (uint16)atoi(argv[2]), (uint16)atoi(argv[3]), (uint16)atoi(argv[4])), kRectEnabled);
	} else if (argc == 2) {
		uint16 resourceId = (uint16)atoi(argv[1]);
		if (resourceId < _vm->getCard()->_resources.size())
			_vm->getCard()->_resources[resourceId]->drawBoundingRect();
	}

	return false;
}

bool MystConsole::Cmd_SetResourceEnable(int argc, const char **argv) {
	if (argc < 3) {
		debugPrintf("Usage: setResourceEnable <resource id> <bool>\n");
		return true;
	}

	_vm->getCard()->setResourceEnabled((uint16)atoi(argv[1]), atoi(argv[2]) == 1);
	return true;
}

bool MystConsole::Cmd_PlaySound(int argc, const char **argv) {
	if (argc == 1) {
		debugPrintf("Usage: playSound <value>\n");

		return true;
	}

	_vm->_sound->playEffect((uint16) atoi(argv[1]));

	return false;
}

bool MystConsole::Cmd_StopSound(int argc, const char **argv) {
	debugPrintf("Stopping Sound\n");

	_vm->_sound->stopEffect();

	return true;
}

bool MystConsole::Cmd_PlayMovie(int argc, const char **argv) {
	if (argc < 3) {
		debugPrintf("Usage: playMovie <name> <stack> [<left> <top>]\n");
		debugPrintf("NOTE: The movie will play *once* in the background.\n");
		return true;
	}

	Common::String fileName = argv[1];
	int8 stackNum = -1;
	for (byte i = 0; i < ARRAYSIZE(mystStackNames); i++)
		if (!scumm_stricmp(argv[2], mystStackNames[i])) {
			stackNum = i;
			break;
		}

	if (stackNum < 0) {
		debugPrintf("\'%s\' is not a stack name!\n", argv[2]);
		return true;
	}

	VideoEntryPtr video = _vm->playMovie(fileName, static_cast<MystStack>(stackNum));

	if (argc == 4) {
		video->setX(atoi(argv[2]));
		video->setY(atoi(argv[3]));
	} else if (argc > 4) {
		video->setX(atoi(argv[3]));
		video->setY(atoi(argv[4]));
	} else {
		video->center();
	}

	return false;
}

bool MystConsole::Cmd_DisableInitOpcodes(int argc, const char **argv) {
	if (argc != 1) {
		debugPrintf("Usage: disableInitOpcodes\n");

		return true;
	}

	_vm->_stack->disablePersistentScripts();

	return true;
}

bool MystConsole::Cmd_Cache(int argc, const char **argv) {
	if (argc > 2) {
		debugPrintf("Usage: cache on/off - Omit parameter to get current state\n");
		return true;
	}

	bool state = false;

	if (argc == 1) {
		state = _vm->getCacheState();
	} else {
		if (!scumm_stricmp(argv[1], "on"))
			state = true;

		_vm->setCacheState(state);
	}

	debugPrintf("Cache: %s\n", state ? "Enabled" : "Disabled");
	return true;
}

bool MystConsole::Cmd_Resources(int argc, const char **argv) {
	debugPrintf("Resources in card %d:\n", _vm->getCard()->getId());

	for (uint i = 0; i < _vm->getCard()->_resources.size(); i++) {
		debugPrintf("#%2d %s\n", i, _vm->getCard()->_resources[i]->describe().c_str());
	}

	return true;
}

bool MystConsole::Cmd_QuickTest(int argc, const char **argv) {
	_debugPauseToken.clear();

	// Go through all the ages, all the views and click random stuff
	for (uint i = 0; i < ARRAYSIZE(mystStackNames); i++) {
		MystStack stackId = static_cast<MystStack>(i);
		if (stackId == kDemoStack || stackId == kMakingOfStack
		    || stackId == kDemoSlidesStack || stackId == kDemoPreviewStack) continue;

		debug("Loading stack %s", mystStackNames[stackId]);
		_vm->changeToStack(stackId, default_start_card[stackId], 0, 0);

		Common::Array<uint16> ids = _vm->getResourceIDList(ID_VIEW);
		for (uint j = 0; j < ids.size(); j++) {
			if (ids[j] == 4632) continue;

			debug("Loading card %d", ids[j]);
			_vm->changeToCard(ids[j], kTransitionCopy);

			_vm->doFrame();

			{
				MystCardPtr card = _vm->getCardPtr();
				int16 resIndex = _vm->_rnd->getRandomNumber(card->_resources.size()) - 1;
				if (resIndex >= 0 && _vm->getCard()->_resources[resIndex]->isEnabled()) {
					card->_resources[resIndex]->handleMouseDown();
					card->_resources[resIndex]->handleMouseUp();
				}
			}

			_vm->doFrame();

			if (_vm->_stack->getStackId() != stackId) {
				// Clicking may have linked us to another age
				_vm->changeToStack(stackId, default_start_card[stackId], 0, 0);
			}
		}
	}

	_debugPauseToken = _vm->pauseEngine();
	return true;
}

#endif // ENABLE_MYST

#ifdef ENABLE_RIVEN

RivenConsole::RivenConsole(MohawkEngine_Riven *vm) : GUI::Debugger(), _vm(vm) {
	registerCmd("changeCard",     WRAP_METHOD(RivenConsole, Cmd_ChangeCard));
	registerCmd("curCard",        WRAP_METHOD(RivenConsole, Cmd_CurCard));
	registerCmd("dumpCard",       WRAP_METHOD(RivenConsole, Cmd_DumpCard));
	registerCmd("var",            WRAP_METHOD(RivenConsole, Cmd_Var));
	registerCmd("playSound",      WRAP_METHOD(RivenConsole, Cmd_PlaySound));
	registerCmd("playSLST",       WRAP_METHOD(RivenConsole, Cmd_PlaySLST));
	registerCmd("stopSound",      WRAP_METHOD(RivenConsole, Cmd_StopSound));
	registerCmd("curStack",       WRAP_METHOD(RivenConsole, Cmd_CurStack));
	registerCmd("dumpStack",      WRAP_METHOD(RivenConsole, Cmd_DumpStack));
	registerCmd("changeStack",    WRAP_METHOD(RivenConsole, Cmd_ChangeStack));
	registerCmd("hotspots",       WRAP_METHOD(RivenConsole, Cmd_Hotspots));
	registerCmd("zipMode",        WRAP_METHOD(RivenConsole, Cmd_ZipMode));
	registerCmd("dumpScript",     WRAP_METHOD(RivenConsole, Cmd_DumpScript));
	registerCmd("listZipCards",   WRAP_METHOD(RivenConsole, Cmd_ListZipCards));
	registerCmd("getRMAP",        WRAP_METHOD(RivenConsole, Cmd_GetRMAP));
	registerCmd("combos",         WRAP_METHOD(RivenConsole, Cmd_Combos));
	registerCmd("sliderState",    WRAP_METHOD(RivenConsole, Cmd_SliderState));
	registerCmd("quickTest",      WRAP_METHOD(RivenConsole, Cmd_QuickTest));
	registerVar("show_hotspots",  &_vm->_showHotspots);
}

RivenConsole::~RivenConsole() {
}


bool RivenConsole::Cmd_ChangeCard(int argc, const char **argv) {
	if (argc < 2) {
		debugPrintf("Usage: changeCard <card>\n");
		return true;
	}

	_vm->_sound->stopSound();
	_vm->_sound->stopAllSLST();
	_vm->changeToCard((uint16)atoi(argv[1]));

	return false;
}

bool RivenConsole::Cmd_CurCard(int argc, const char **argv) {
	debugPrintf("Current Card: %d\n", _vm->getCard()->getId());

	return true;
}

bool RivenConsole::Cmd_Var(int argc, const char **argv) {
	if (argc == 1) {
		debugPrintf("Usage: var <var name> (<value>)\n");
		return true;
	}

	if (!_vm->_vars.contains(argv[1])) {
		debugPrintf("Unknown variable '%s'\n", argv[1]);
		return true;
	}

	uint32 &var = _vm->_vars[argv[1]];

	if (argc > 2)
		var = (uint32)atoi(argv[2]);

	debugPrintf("%s = %d\n", argv[1], var);
	return true;
}

bool RivenConsole::Cmd_PlaySound(int argc, const char **argv) {
	if (argc < 2) {
		debugPrintf("Usage: playSound <value>\n");
		return true;
	}

	_vm->_sound->stopSound();
	_vm->_sound->stopAllSLST();
	_vm->_sound->playSound((uint16)atoi(argv[1]));
	return false;
}

bool RivenConsole::Cmd_PlaySLST(int argc, const char **argv) {
	if (argc < 2) {
		debugPrintf("Usage: playSLST <slst index>\n");

		return true;
	}

	_vm->_sound->stopSound();
	_vm->_sound->stopAllSLST();

	_vm->getCard()->playSound((uint16)atoi(argv[1]));
	return false;
}

bool RivenConsole::Cmd_StopSound(int argc, const char **argv) {
	debugPrintf("Stopping Sound\n");

	_vm->_sound->stopSound();
	_vm->_sound->stopAllSLST();
	return true;
}

bool RivenConsole::Cmd_CurStack(int argc, const char **argv) {
	debugPrintf("Current Stack: %s\n", RivenStacks::getName(_vm->getStack()->getId()));

	return true;
}

bool RivenConsole::Cmd_ChangeStack(int argc, const char **argv) {
	if (argc < 3) {
		debugPrintf("Usage: changeStack <stack> <card>\n\n");
		debugPrintf("Stacks:\n=======\n");

		for (uint i = kStackFirst; i <= kStackLast; i++)
			debugPrintf(" %s\n", RivenStacks::getName(i));

		debugPrintf("\n");

		return true;
	}

	uint stackId = RivenStacks::getId(argv[1]);
	if (stackId == kStackUnknown) {
		debugPrintf("\'%s\' is not a stack name!\n", argv[1]);
		return true;
	}

	_vm->changeToStack(stackId);
	_vm->changeToCard((uint16)atoi(argv[2]));

	return false;
}

bool RivenConsole::Cmd_Hotspots(int argc, const char **argv) {
	Common::Array<RivenHotspot *> hotspots = _vm->getCard()->getHotspots();

	debugPrintf("Current card (%d) has %d hotspots:\n", _vm->getCard()->getId(), hotspots.size());

	for (uint16 i = 0; i < hotspots.size(); i++) {
		RivenHotspot *hotspot = hotspots[i];
		debugPrintf("Hotspot %d, index %d, BLST ID %d (", i, hotspot->getIndex(), hotspot->getBlstId());

		if (hotspot->isEnabled())
			debugPrintf("enabled");
		else
			debugPrintf("disabled");

		Common::Rect rect = hotspot->getRect();
		debugPrintf(") - (%d, %d, %d, %d)\n", rect.left, rect.top, rect.right, rect.bottom);
		debugPrintf("    Name = %s\n", hotspot->getName().c_str());
	}

	return true;
}

bool RivenConsole::Cmd_ZipMode(int argc, const char **argv) {
	uint32 &zipModeActive = _vm->_vars["azip"];
	zipModeActive = !zipModeActive;

	debugPrintf("Zip Mode is ");
	debugPrintf(zipModeActive ? "Enabled" : "Disabled");
	debugPrintf("\n");
	return true;
}

bool RivenConsole::Cmd_DumpCard(int argc, const char **argv) {
	if (argc != 1) {
		debugPrintf("Usage: dumpCard\n");
		return true;
	}

	_vm->getCard()->dump();

	debugPrintf("Card dump complete.\n");

	return true;
}

bool RivenConsole::Cmd_DumpStack(int argc, const char **argv) {
	if (argc != 1) {
		debugPrintf("Usage: dumpStack\n");
		return true;
	}

	_vm->getStack()->dump();

	debugPrintf("Stack dump complete.\n");

	return true;
}

bool RivenConsole::Cmd_DumpScript(int argc, const char **argv) {
	if (argc < 4) {
		debugPrintf("Usage: dumpScript <stack> <CARD or HSPT> <card>\n");
		return true;
	}

	uint16 oldStack = _vm->getStack()->getId();

	uint newStack = RivenStacks::getId(argv[1]);
	if (newStack == kStackUnknown) {
		debugPrintf("\'%s\' is not a stack name!\n", argv[1]);
		return true;
	}

	_vm->changeToStack(newStack);

	// Get CARD/HSPT data and dump their scripts
	if (!scumm_stricmp(argv[2], "CARD")) {
		// Use debugN to print these because the scripts can get very large and would
		// really be useless if the text console is not used. A DumpFile could also
		// theoretically be used, but I (clone2727) typically use this dynamically and
		// don't want countless files laying around without game context. If one would
		// want a file of a script they could just redirect stdout to a file or use
		// deriven.
		debugN("\n\nDumping scripts for %s\'s card %d!\n", argv[1], (uint16)atoi(argv[3]));
		debugN("==================================\n\n");
		Common::SeekableReadStream *cardStream = _vm->getResource(MKTAG('C','A','R','D'), (uint16)atoi(argv[3]));
		cardStream->seek(4);
		RivenScriptList scriptList = _vm->_scriptMan->readScripts(cardStream);
		for (uint32 i = 0; i < scriptList.size(); i++) {
			debugN("Stream Type %d:\n", scriptList[i].type);
			scriptList[i].script->dumpScript(0);
		}
		delete cardStream;
	} else if (!scumm_stricmp(argv[2], "HSPT")) {
		// See above for why this is printed via debugN
		debugN("\n\nDumping scripts for %s\'s card %d hotspots!\n", argv[1], (uint16)atoi(argv[3]));
		debugN("===========================================\n\n");

		Common::SeekableReadStream *hsptStream = _vm->getResource(MKTAG('H','S','P','T'), (uint16)atoi(argv[3]));

		uint16 hotspotCount = hsptStream->readUint16BE();

		for (uint16 i = 0; i < hotspotCount; i++) {
			debugN("Hotspot %d:\n", i);
			hsptStream->seek(22, SEEK_CUR);	// Skip non-script related stuff
			RivenScriptList scriptList = _vm->_scriptMan->readScripts(hsptStream);
			for (uint32 j = 0; j < scriptList.size(); j++) {
				debugN("\tStream Type %d:\n", scriptList[j].type);
				scriptList[j].script->dumpScript(1);
			}
		}

		delete hsptStream;
	} else {
		debugPrintf("%s doesn't have any scripts!\n", argv[2]);
	}

	// See above for why this is printed via debugN
	debugN("\n\n");

	_vm->changeToStack(oldStack);

	debugPrintf("Script dump complete.\n");

	return true;
}

bool RivenConsole::Cmd_ListZipCards(int argc, const char **argv) {
	if (_vm->_zipModeData.size() == 0) {
		debugPrintf("No zip card data.\n");
	} else {
		debugPrintf("Listing zip cards:\n");
		for (uint32 i = 0; i < _vm->_zipModeData.size(); i++)
			debugPrintf("ID = %d, Name = %s\n", _vm->_zipModeData[i].id, _vm->_zipModeData[i].name.c_str());
	}

	return true;
}

bool RivenConsole::Cmd_GetRMAP(int argc, const char **argv) {
	uint32 rmapCode = _vm->getStack()->getCurrentCardGlobalId();
	debugPrintf("RMAP for %s %d = %08x\n", RivenStacks::getName(_vm->getStack()->getId()), _vm->getCard()->getId(), rmapCode);
	return true;
}

bool RivenConsole::Cmd_Combos(int argc, const char **argv) {
	// In the vain of SCUMM's 'drafts' command, this command will list
	// out all combinations needed in Riven, decoded from the variables.
	// You'll need to look up the Rebel Tunnel puzzle on your own; the
	// solution is constant.

	uint32 teleCombo = _vm->_vars["tcorrectorder"];
	uint32 prisonCombo = _vm->_vars["pcorrectorder"];
	uint32 domeCombo = _vm->_vars["adomecombo"];

	debugPrintf("Telescope Combo:\n  ");
	for (int i = 0; i < 5; i++)
		debugPrintf("%d ", _vm->getStack()->getComboDigit(teleCombo, i));

	debugPrintf("\nPrison Combo:\n  ");
	for (int i = 0; i < 5; i++)
		debugPrintf("%d ", _vm->getStack()->getComboDigit(prisonCombo, i));

	debugPrintf("\nDome Combo:\n  ");
	for (int i = 1; i <= 25; i++)
		if (domeCombo & (1 << (25 - i)))
			debugPrintf("%d ", i);

	debugPrintf("\n");
	return true;
}

bool RivenConsole::Cmd_SliderState(int argc, const char **argv) {
	RivenStacks::DomeSpit *domeSpit = dynamic_cast<RivenStacks::DomeSpit *>(_vm->getStack());
	if (!domeSpit) {
		debugPrintf("No dome in this stack\n");
		return true;
	}

	if (argc > 1)
		domeSpit->setDomeSliderState((uint32)atoi(argv[1]));

	debugPrintf("Dome Slider State = %08x\n", domeSpit->getDomeSliderState());
	return true;
}

bool RivenConsole::Cmd_QuickTest(int argc, const char **argv) {
	_debugPauseToken.clear();

	// Go through all the stacks, all the cards and click random stuff
	for (uint16 stackId = kStackFirst; stackId <= kStackLast; stackId++) {

		debug("Loading stack %s", RivenStacks::getName(stackId));
		_vm->changeToStack(stackId);

		Common::Array<uint16> cardIds = _vm->getResourceIDList(ID_CARD);
		for (uint16 i = 0; i < cardIds.size(); i++) {
			if (_vm->shouldQuit()) break;

			uint16 cardId = cardIds[i];
			if (stackId == kStackTspit && cardId == 366) continue; // Cut card with invalid links
			if (stackId == kStackTspit && cardId == 412) continue; // Cut card with invalid links
			if (stackId == kStackTspit && cardId == 486) continue; // Cut card with invalid links
			if (stackId == kStackBspit && cardId == 465) continue; // Cut card with invalid links
			if (stackId == kStackJspit && cardId == 737) continue; // Cut card with invalid links

			debug("Loading card %d", cardId);
			RivenScriptPtr script = _vm->_scriptMan->createScriptFromData(1,
			                            kRivenCommandChangeCard, 1, cardId);
			_vm->_scriptMan->runScript(script, true);

			_vm->_gfx->setTransitionMode(kRivenTransitionModeDisabled);

			while (_vm->_scriptMan->hasQueuedScripts()) {
				_vm->doFrame();
			}

			// Click on a random hotspot
			Common::Array<RivenHotspot *> hotspots = _vm->getCard()->getHotspots();
			if (!hotspots.empty() && _vm->getStack()->getId() != kStackAspit) {
				uint hotspotIndex = _vm->_rnd->getRandomNumberRng(0, hotspots.size() - 1);
				RivenHotspot *hotspot = hotspots[hotspotIndex];
				if (hotspot->isEnabled()) {
					Common::Rect hotspotRect = hotspot->getRect();
					Common::Point hotspotPoint((hotspotRect.left + hotspotRect.right) / 2, (hotspotRect.top + hotspotRect.bottom) / 2);
					_vm->getStack()->onMouseDown(hotspotPoint);
					_vm->getStack()->onMouseUp(hotspotPoint);
				}

				while (_vm->_scriptMan->hasQueuedScripts()) {
					_vm->doFrame();
				}
			}

			if (_vm->getStack()->getId() != stackId) {
				// Clicking may have linked us to another age
				_vm->changeToStack(stackId);
			}
		}
	}

	_debugPauseToken = _vm->pauseEngine();
	return true;
}

#endif // ENABLE_RIVEN

LivingBooksConsole::LivingBooksConsole(MohawkEngine_LivingBooks *vm) : GUI::Debugger(), _vm(vm) {
	registerCmd("playSound",			WRAP_METHOD(LivingBooksConsole, Cmd_PlaySound));
	registerCmd("stopSound",			WRAP_METHOD(LivingBooksConsole, Cmd_StopSound));
	registerCmd("drawImage",			WRAP_METHOD(LivingBooksConsole, Cmd_DrawImage));
	registerCmd("changePage",			WRAP_METHOD(LivingBooksConsole, Cmd_ChangePage));
	registerCmd("changeCursor",			WRAP_METHOD(LivingBooksConsole, Cmd_ChangeCursor));
}

LivingBooksConsole::~LivingBooksConsole() {
}

bool LivingBooksConsole::Cmd_PlaySound(int argc, const char **argv) {
	if (argc == 1) {
		debugPrintf("Usage: playSound <value>\n");
		return true;
	}

	_vm->_sound->stopSound();
	_vm->_sound->playSound((uint16)atoi(argv[1]));
	return false;
}

bool LivingBooksConsole::Cmd_StopSound(int argc, const char **argv) {
	debugPrintf("Stopping Sound\n");

	_vm->_sound->stopSound();
	return true;
}

bool LivingBooksConsole::Cmd_DrawImage(int argc, const char **argv) {
	if (argc == 1) {
		debugPrintf("Usage: drawImage <value>\n");
		return true;
	}

	_vm->_gfx->copyAnimImageToScreen((uint16)atoi(argv[1]));
	_vm->_system->updateScreen();
	return false;
}

bool LivingBooksConsole::Cmd_ChangePage(int argc, const char **argv) {
	if (argc < 2 || argc > 3) {
		debugPrintf("Usage: changePage <page>[.<subpage>] [<mode>]\n");
		return true;
	}

	int page, subpage = 0;
	if (sscanf(argv[1], "%d.%d", &page, &subpage) == 0) {
		debugPrintf("Usage: changePage <page>[.<subpage>] [<mode>]\n");
		return true;
	}
	LBMode mode = argc == 2 ? _vm->getCurMode() : (LBMode)atoi(argv[2]);
	if (subpage == 0) {
		if (_vm->tryLoadPageStart(mode, page))
			return false;
	} else {
		if (_vm->loadPage(mode, page, subpage))
			return false;
	}
	debugPrintf("no such page %d.%d\n", page, subpage);
	return true;
}

bool LivingBooksConsole::Cmd_ChangeCursor(int argc, const char **argv) {
	if (argc == 1) {
		debugPrintf("Usage: changeCursor <value>\n");
		return true;
	}

	_vm->_cursor->setCursor((uint16)atoi(argv[1]));
	return true;
}

#ifdef ENABLE_CSTIME

CSTimeConsole::CSTimeConsole(MohawkEngine_CSTime *vm) : GUI::Debugger(), _vm(vm) {
	registerCmd("playSound",			WRAP_METHOD(CSTimeConsole, Cmd_PlaySound));
	registerCmd("stopSound",			WRAP_METHOD(CSTimeConsole, Cmd_StopSound));
	registerCmd("drawImage",			WRAP_METHOD(CSTimeConsole, Cmd_DrawImage));
	registerCmd("drawSubimage",			WRAP_METHOD(CSTimeConsole, Cmd_DrawSubimage));
	registerCmd("changeCase",			WRAP_METHOD(CSTimeConsole, Cmd_ChangeCase));
	registerCmd("changeScene",			WRAP_METHOD(CSTimeConsole, Cmd_ChangeScene));
	registerCmd("caseVariable",			WRAP_METHOD(CSTimeConsole, Cmd_CaseVariable));
	registerCmd("invItem",			WRAP_METHOD(CSTimeConsole, Cmd_InvItem));
}

CSTimeConsole::~CSTimeConsole() {
}

bool CSTimeConsole::Cmd_PlaySound(int argc, const char **argv) {
	if (argc == 1) {
		debugPrintf("Usage: playSound <value>\n");
		return true;
	}

	_vm->_sound->stopSound();
	_vm->_sound->playSound((uint16)atoi(argv[1]));
	return false;
}

bool CSTimeConsole::Cmd_StopSound(int argc, const char **argv) {
	debugPrintf("Stopping Sound\n");

	_vm->_sound->stopSound();
	return true;
}

bool CSTimeConsole::Cmd_DrawImage(int argc, const char **argv) {
	if (argc == 1) {
		debugPrintf("Usage: drawImage <value>\n");
		return true;
	}

	_vm->_gfx->copyAnimImageToScreen((uint16)atoi(argv[1]));
	_vm->_system->updateScreen();
	return false;
}

bool CSTimeConsole::Cmd_DrawSubimage(int argc, const char **argv) {
	if (argc < 3) {
		debugPrintf("Usage: drawSubimage <value> <subimage>\n");
		return true;
	}

	_vm->_gfx->copyAnimSubImageToScreen((uint16)atoi(argv[1]), (uint16)atoi(argv[2]));
	_vm->_system->updateScreen();
	return false;
}

bool CSTimeConsole::Cmd_ChangeCase(int argc, const char **argv) {
	if (argc < 2) {
		debugPrintf("Usage: changeCase <value>\n");
		return true;
	}

	error("Can't change case yet"); // FIXME
	return false;
}

bool CSTimeConsole::Cmd_ChangeScene(int argc, const char **argv) {
	if (argc < 2) {
		debugPrintf("Usage: changeScene <value>\n");
		return true;
	}

	_vm->addEvent(CSTimeEvent(kCSTimeEventNewScene, 0xffff, atoi(argv[1])));
	return false;
}

bool CSTimeConsole::Cmd_CaseVariable(int argc, const char **argv) {
	if (argc < 2) {
		debugPrintf("Usage: caseVariable <id> [<value>]\n");
		return true;
	}

	if (argc == 2) {
		debugPrintf("case variable %d has value %d\n", atoi(argv[1]), _vm->_caseVariable[atoi(argv[1])]);
	} else {
		_vm->_caseVariable[atoi(argv[1])] = atoi(argv[2]);
	}
	return true;
}

bool CSTimeConsole::Cmd_InvItem(int argc, const char **argv) {
	if (argc < 3) {
		debugPrintf("Usage: invItem <id> <0 or 1>\n");
		return true;
	}

	if (atoi(argv[2])) {
		_vm->addEvent(CSTimeEvent(kCSTimeEventDropItemInInventory, 0xffff, atoi(argv[1])));
	} else {
		_vm->addEvent(CSTimeEvent(kCSTimeEventRemoveItemFromInventory, 0xffff, atoi(argv[1])));
	}
	return false;
}

#endif // ENABLE_CSTIME

#ifdef ENABLE_ZOOMBINI

ZoombiniConsole::ZoombiniConsole(MohawkEngine_Zoombini *vm) : GUI::Debugger(), _vm(vm) {
	registerCmd("playSound",			WRAP_METHOD(ZoombiniConsole, Cmd_PlaySound));
	registerCmd("stopSound",			WRAP_METHOD(ZoombiniConsole, Cmd_StopSound));
#if 0
	registerCmd("dumpSound",			WRAP_METHOD(ZoombiniConsole, Cmd_DumpSound));
#endif
	registerCmd("playMidi",				WRAP_METHOD(ZoombiniConsole, Cmd_PlayMidi));
	registerCmd("stopMidi",				WRAP_METHOD(ZoombiniConsole, Cmd_StopMidi));
	registerCmd("dumpMidi",				WRAP_METHOD(ZoombiniConsole, Cmd_DumpMidi));
	registerCmd("drawCursor",			WRAP_METHOD(ZoombiniConsole, Cmd_DrawCursor));
	registerCmd("drawImage",			WRAP_METHOD(ZoombiniConsole, Cmd_DrawImage));
	registerCmd("dumpImage",			WRAP_METHOD(ZoombiniConsole, Cmd_DumpImage));
	registerCmd("drawShape",			WRAP_METHOD(ZoombiniConsole, Cmd_DrawShape));
	registerCmd("drawShapes",			WRAP_METHOD(ZoombiniConsole, Cmd_DrawShapes));
	registerCmd("dumpShapes",			WRAP_METHOD(ZoombiniConsole, Cmd_DumpShapes));
	registerCmd("printFeature",			WRAP_METHOD(ZoombiniConsole, Cmd_PrintFeature));
	registerCmd("printFeatures",		WRAP_METHOD(ZoombiniConsole, Cmd_PrintFeatures));
	registerCmd("drawFeature",			WRAP_METHOD(ZoombiniConsole, Cmd_DrawFeature));
	registerCmd("dumpFeature",			WRAP_METHOD(ZoombiniConsole, Cmd_DumpFeature));
	registerCmd("dumpFeatures",			WRAP_METHOD(ZoombiniConsole, Cmd_DumpFeatures));
	registerCmd("printSnoidScript",		WRAP_METHOD(ZoombiniConsole, Cmd_PrintSnoidScript));
	registerCmd("printSnoidScripts",	WRAP_METHOD(ZoombiniConsole, Cmd_PrintSnoidScripts));
	registerCmd("dumpSnoidScript",		WRAP_METHOD(ZoombiniConsole, Cmd_DumpSnoidScript));
	registerCmd("dumpSnoidScripts",		WRAP_METHOD(ZoombiniConsole, Cmd_DumpSnoidScripts));
	registerCmd("plotPoint",			WRAP_METHOD(ZoombiniConsole, Cmd_PlotPoint));
	registerCmd("plotLine",				WRAP_METHOD(ZoombiniConsole, Cmd_PlotLine));
	registerCmd("plotRect",				WRAP_METHOD(ZoombiniConsole, Cmd_PlotRect));
	registerCmd("dumpAllResources",		WRAP_METHOD(ZoombiniConsole, Cmd_DumpAllResources));
	registerCmd("goXfer",				WRAP_METHOD(ZoombiniConsole, Cmd_GoXfer));
	registerCmd("goPractice",			WRAP_METHOD(ZoombiniConsole, Cmd_GoPractice));
}

ZoombiniConsole::~ZoombiniConsole() {
}

bool ZoombiniConsole::parseInt(const char *str, int32 &result) {
	bool success = ZmbResource::parseInt(str, result);
	if (!success)
		debugPrintf("Cannot parse resourceId(%s), try <archive>:<UINT16> pattern (hex supported with 0x prefix)\n", str);
	return success;
}

bool ZoombiniConsole::parseResourceId(const char *str, ZmbResource &outRes) {
	bool success = ZmbResource::parse(str, outRes);
	if (!success)
		debugPrintf("Cannot parse resourceId(%s), try <archive>:<UINT16> pattern (hex supported with 0x prefix)\n", str);
	return success;
}

bool ZoombiniConsole::Cmd_PlaySound(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: playSound <value>\n");
		return true;
	}

	ZmbResource resource;
	if (!parseResourceId(argv[1], resource))
		return true;
	
	if (!_vm->hasResource(ID_TWAV, resource)) {
		debugPrintf("Cannot find resource tWAV(%s)\n", argv[1]);
		return true;
	}

	_vm->_sound->stopSound();
	_vm->_sound->playZmbSound(resource, Audio::Mixer::kSFXSoundType);
	return false;
}

bool ZoombiniConsole::Cmd_StopSound(int argc, const char **argv) {
	debugPrintf("Stopping Sound\n");

	_vm->_sound->stopSound();
	return true;
}

#if 0
bool ZoombiniConsole::Cmd_DumpSound(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: dumpSound <value>\n");
		return true;
	}

	ZmbArchiveKind archiveKind = ZmbArchiveKind::kPage;
	uint16 resid = 0;
	if (!parseResourceId(argv[1], archiveKind, resid))
		return true;
	
	if (!_vm->hasResource(archiveKind, ID_TWAV, resid)) {
		debugPrintf("Cannot find resource tWAV(%s)\n", argv[1]);
		return true;
	}

	Common::SeekableReadStream *wavStream = _vm->getResource(archiveKind, ID_TWAV, resid);
	if (!wavStream) {
		debugPrintf("Failed to read tWAV resource(%s)\n", argv[1]);
		return true;
	}

	char chArchive = (archiveKind == ZmbArchiveKind::kCommon) ? 'c' : 'p';
	Common::String filename = Common::String::format("ZOOMBINI_tWAV_%c%04u.wav", chArchive, resid);
	Common::String filepath = "dumps/" + filename;

	Common::DumpFile out;
	if (!out.open(Common::Path(filepath, '/'), true)) {
		debugPrintf("Failed to open file for writing: %s\n", filepath.c_str());
		delete wavStream;
		return true;
	}

	// Copy the entire stream to the file
	uint32 size = wavStream->size();
	byte *buffer = new byte[size];
	wavStream->read(buffer, size);
	out.write(buffer, size);
	delete[] buffer;
	delete wavStream;
	out.close();

	debugPrintf("Successfully exported tWAV to %s\n", filename.c_str());
	return true;
}
#endif

bool ZoombiniConsole::Cmd_PlayMidi(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: playMidi <value>\n");
		return true;
	}

	// MIDI resources are always in the page archive
	errno = 0;
	uint16 resid = static_cast<uint16>(strtoul(argv[1], nullptr, 10));
	if (errno != 0) {
		debugPrintf("Cannot parse argument %s\n", argv[1]);
		return true;
	}
	
	if (!_vm->hasResource(ID_MIDI, ZmbResource(ZmbArchiveKind::kPage, resid))) {
		debugPrintf("Cannot find resource MIDI(%u)\n", resid);
		return true;
	}

	_vm->_midi->stop();
	_vm->_midi->playMidi(resid);
	return false;
}

bool ZoombiniConsole::Cmd_StopMidi(int argc, const char **argv) {
	debugPrintf("Stopping Midi\n");

	_vm->_midi->stop();
	return true;
}

bool ZoombiniConsole::Cmd_DumpMidi(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: dumpMidi <value>\n");
		return true;
	}

	// MIDI resources are always in the page archive MIDIMPC.MHK.
	errno = 0;
	uint16 resid = static_cast<uint16>(strtoul(argv[1], nullptr, 10));
	if (errno != 0) {
		debugPrintf("Cannot parse argument %s\n", argv[1]);
		return true;
	}
	
	if (!_vm->hasResource(ID_MIDI, ZmbResource(ZmbArchiveKind::kPage, resid))) {
		debugPrintf("Cannot find resource MIDI(%s)\n", argv[1]);
		return true;
	}

	Common::SeekableReadStream *midiStream = _vm->getResource(ID_MIDI, ZmbResource(ZmbArchiveKind::kPage, resid));
	if (!midiStream) {
		debugPrintf("Failed to read MIDI resource(%s)\n", argv[1]);
		return true;
	}

	Common::String filename = Common::String::format("ZOOMBINI_MIDI_p%04u.mid", resid);
	Common::String filepath = "dumps/" + filename;

	Common::DumpFile out;
	if (!out.open(Common::Path(filepath, '/'), true)) {
		debugPrintf("Failed to open file for writing: %s\n", filepath.c_str());
		delete midiStream;
		return true;
	}

	// Copy the entire stream to the file
	uint32 size = midiStream->size();
	byte *buffer = new byte[size];
	midiStream->read(buffer, size);
	out.write(buffer, size);
	delete[] buffer;
	delete midiStream;
	out.close();

	debugPrintf("Successfully exported MIDI to %s\n", filename.c_str());
	return true;
}

bool ZoombiniConsole::Cmd_DrawCursor(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: drawCursor <cursorId>\n");
		return true;
	}

	// Cursor is always in system ZOOMBINI.MHK
	errno = 0;
	uint16 cursorId = static_cast<uint16>(strtoul(argv[1], nullptr, 10));
	if (errno != 0) {
		debugPrintf("Cannot parse argument %s\n", argv[1]);
		return true;
	}

	if (!_vm->hasResource(ID_CURS, ZmbResource(ZmbArchiveKind::kSystem, cursorId))) {
		debugPrintf("Cannot find resource CURS(%s)\n", argv[1]);
		return true;
	}
		
	ZoombiniDebugCommand cmd;
	cmd.setDrawCursor(ZmbResource(ZmbArchiveKind::kSystem, cursorId));
	_vm->openDebugDialog(cmd);
	return false;
}

bool ZoombiniConsole::Cmd_DrawImage(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: drawImage <imageId>\n");
		return true;
	}

	// There is no palette resource in system ZOOMBINI.MHK
	errno = 0;
	uint16 imageId = static_cast<uint16>(strtoul(argv[1], nullptr, 10));
	if (errno != 0) {
		debugPrintf("Cannot parse argument %s\n", argv[1]);
		return true;
	}

	if (!_vm->hasResource(ID_SHPL, ZmbResource(ZmbArchiveKind::kPage, imageId))) // palette
		debugPrintf("Cannot find resource SHPL(%s), maybe the bitmap is a compound shape?\n", argv[1]);
	
	if (!_vm->hasResource(ID_TBMP, ZmbResource(ZmbArchiveKind::kPage, imageId))) { // background bitmap
		debugPrintf("Cannot find resource tBMP(%s)\n", argv[1]);
		return true;
	}

	ZoombiniDebugCommand cmd;
	cmd.setDrawImage(ZmbResource(ZmbArchiveKind::kPage, imageId));
	_vm->openDebugDialog(cmd);
	return false;
}

bool ZoombiniConsole::Cmd_DumpImage(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: dumpImage <imageId>\n");
		return true;
	}

	// There is no palette resource in system ZOOMBINI.MHK
	errno = 0;
	uint16 imageId = static_cast<uint16>(strtoul(argv[1], nullptr, 10));
	if (errno != 0) {
		debugPrintf("Cannot parse argument %s\n", argv[1]);
		return true;
	}

	// In DumpImage, SHPL (palette) resource must exist in the page archive
	if (!_vm->hasResource(ID_SHPL, ZmbResource(ZmbArchiveKind::kPage, imageId))) {
		debugPrintf("Cannot find resource SHPL(%s)\n", argv[1]);
		return true;
	}

	if (!_vm->hasResource(ID_TBMP, ZmbResource(ZmbArchiveKind::kPage, imageId))) {
		debugPrintf("Cannot find resource tBMP(%s)\n", argv[1]);
		return true;
	}

	// Read palette
	byte palette[3 * 256];
	memset(palette, 0, ARRAYSIZE(palette));
	{
		if (!_vm->_gfx->readPalette(imageId, palette, ARRAYSIZE(palette))) {
			debugPrintf("Failed to load palette from SHPL %04u\n", imageId);
			return true;
		}
	}

	// Read image surface
	MohawkSurface *imgSurface = _vm->_gfx->findImage(ZmbResource(ZmbArchiveKind::kPage, imageId));
	if (!imgSurface) {
		debugPrintf("Failed to load image %u\n", imageId);
		return true;
	}
	Graphics::Surface *surface = imgSurface->getSurface();
	if (!surface || surface->h == 0 || surface->w == 0) {
		debugPrintf("Invalid surface for image %u\n", imageId);
		return true;
	}

	// Export to BMP
	Common::String filename = Common::String::format("ZOOMBINI_tBMP_p%04u.BMP", imageId);
	if (exportSurfaceToBMP(filename, surface, palette)) {
		debugPrintf("Successfully exported image %u to %s\n", imageId, filename.c_str());
	} else {
		debugPrintf("Failed to export image %u to BMP\n", imageId);
	}

	return true;
}

bool ZoombiniConsole::Cmd_DrawShape(int argc, const char **argv) {
	if (argc != 3) {
		debugPrintf("Usage: drawShape <imageId> <shapeIdx>\n");
		return true;
	}

	ZmbResource resource;
	if (!parseResourceId(argv[1], resource))
		return true;
	
	if (!_vm->hasResource(ID_TBMP, resource)) {
		debugPrintf("Cannot find resource tBMP(%s)\n", argv[1]);
		return true;
	}

	errno = 0;
	uint16 shapeIdx = static_cast<uint16>(strtoul(argv[2], nullptr, 10));
	if (errno != 0) {
		debugPrintf("Cannot parse argument %s\n", argv[2]);
		return true;
	}

	ZoombiniDebugCommand cmd;
	cmd.setDrawShape(resource, shapeIdx);
	_vm->openDebugDialog(cmd);
	return false;
}

bool ZoombiniConsole::Cmd_DrawShapes(int argc, const char **argv) {
	if (!(2 <= argc && argc <= 3)) {
		debugPrintf("Usage: drawShapes <imageId> [startShapeIdx]\n");
		return true;
	}

	ZmbResource resource;
	if (!parseResourceId(argv[1], resource))
		return true;
	uint16 startShapeIdx = 1;
	if (argc == 3) {
		errno = 0;
		startShapeIdx = static_cast<uint16>(strtoul(argv[2], nullptr, 10));
		if (errno != 0) {
			debugPrintf("Cannot parse argument %s\n", argv[2]);
			return true;
		}
		if (startShapeIdx < 1) {
			debugPrintf("[startShapeId] is 1-based idx!\n");
			return true;
		}
	}

	if (!_vm->hasResource(ID_TBMP, resource)) {
		debugPrintf("Cannot find resource tBMP(%s)\n", argv[1]);
		return true;
	}
	
	uint32 shapeCount = _vm->_gfx->getShapeCount(resource);
	if (shapeCount < startShapeIdx) {
		debugPrintf("startShapeIdx exceeded shape count %u\n", shapeCount);
		return true;
	}

	ZoombiniDebugCommand cmd;
	cmd.setDrawShapes(resource, startShapeIdx);
	_vm->openDebugDialog(cmd);
	return false;
}

bool ZoombiniConsole::Cmd_DumpShapes(int argc, const char **argv) {
	if (!(2 <= argc && argc <= 3)) {
		debugPrintf("Usage: dumpShapes <imageId> [shplId]\n");
		return true;
	}

	ZmbResource resource;
	if (!parseResourceId(argv[1], resource))
		return true;
	uint16 shplId = 0;
	if (argc == 3) {
		errno = 0;
		shplId = static_cast<uint16>(strtoul(argv[2], nullptr, 10));
		if (errno != 0) {
			debugPrintf("Cannot parse argument %s!\n", argv[2]);
			return true;
		}
		if (shplId < 1) {
			debugPrintf("[shplId] must be larger then 0!\n");
			return true;
		}
	}

	// Collect palette
	Common::String palLogStr;
	byte palette[3 * 256];
	memset(palette, 0, ARRAYSIZE(palette));
	if (shplId == 0) { // Read current palette
		_vm->_system->getPaletteManager()->grabPalette(palette, 0, 256);
		palLogStr = "with current palettes";
	} else { // Read palette from a SHPL resource
		// There is no SHPL resources in system ZOOMBINI.MHK
		if (!_vm->hasResource(ID_SHPL, ZmbResource(ZmbArchiveKind::kPage, shplId))) {
			debugPrintf("Cannot find resource SHPL %04u\n", shplId);
			return true;
		}

		if (!_vm->_gfx->readPalette(shplId, palette, ARRAYSIZE(palette))) {
			debugPrintf("Failed to load palette from SHPL %04u\n", shplId);
			return true;
		}

		palLogStr = Common::String::format("with SHPL %04u palettes", shplId);
	}
	
	// Read the shape data
	if (!_vm->hasResource(ID_TBMP, resource)) {
		debugPrintf("Cannot find resource tBMP(%s)\n", argv[1]);
		return true;
	}
	
	uint32 shapeCount = _vm->_gfx->getShapeCount(resource);
	
	// Export shape bitmaps
	uint16 exportedCount = 0;
	for (uint16 shapeIdx = 1; shapeIdx <= shapeCount; shapeIdx++) {
		MohawkSurface *shapeSurface = _vm->_gfx->findShape(resource, shapeIdx);
		if (!shapeSurface) {
			debugPrintf("Warning: Failed to load shape %u\n", shapeIdx);
			continue;
		}

		Graphics::Surface *surface = shapeSurface->getSurface();
		if (!surface || surface->h == 0 || surface->w == 0) {
			debugPrintf("Warning: Invalid surface for shape %u\n", shapeIdx);
			continue;
		}

		// Export to BMP
		Common::String filename = Common::String::format("ZOOMBINI_tBMP_%s_shape_%03u.BMP", resource.toString().c_str(), shapeIdx);
		if (exportSurfaceToBMP(filename, surface, palette)) {
			exportedCount++;
			debugPrintf("Successfully exported image %s to %s\n", resource.toString().c_str(), filename.c_str());
		} else {
			debugPrintf("Failed to export image %s to BMP\n", resource.toString().c_str());
		}
	}

	debugPrintf("Successfully exported %03u of %03u shapes from image %s %s\n", exportedCount, shapeCount, resource.toString().c_str(), palLogStr.c_str());
	return true;
}

bool ZoombiniConsole::Cmd_PrintFeature(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: printFeature <scrbId>\n");
		return true;
	}

	ZmbResource resource;
	if (!parseResourceId(argv[1], resource))
		return true;

	if (!_vm->hasResource(ID_SCRB, resource)) {
		debugPrintf("Cannot find resource SCRB(%s)\n", argv[1]);
		return true;
	}

	// Dump feature directly
	Common::SeekableReadStream *scrbStream = _vm->getResource(ID_SCRB, resource);

	ZmbFeature *feature = new ZmbFeature(_vm, resource._id, 0, 0, resource);
	feature->parseStream(scrbStream);
	debugPrintf("SCRB_%04u: FrameCount(%u) MaxFrameIdx(%u)\n", resource._id, feature->getFrameCount(), feature->getMaxFrameIdx());

	for (auto it = feature->begin(); it != feature->end(); it++) {
		ZmbHotspotGroup *hsGroup = it->_value;

		for (auto git = hsGroup->begin(); git != hsGroup->end(); git++) {
			ZmbHotspot hs = *git;
			debugPrintf("  Frame(%u): Hotspot ID(%u) at (%u, %u)\n", hs._frame, hs._shapeIdx, hs._x, hs._y);
		}
	}

	delete feature;
	return true;
}

bool ZoombiniConsole::Cmd_PrintFeatures(int argc, const char **argv) {
	if (argc != 1) {
		debugPrintf("Print all features from the active page\n");
		debugPrintf("Usage: printFeatures\n");
		return true;
	}

	// Get all SCRB resource IDs from the active page
	Common::Array<uint16> resIds = _vm->getResourceIDList(ZmbArchiveKind::kPage, ID_SCRB);
	if (resIds.empty()) {
		debugPrintf("No SCRB resources found in current page\n");
		return true;
	}
	
	// Dump each feature
	for (uint i = 0; i < resIds.size(); i++) {
		uint16 scrbId = resIds[i];
		ZmbResource resource = ZmbResource(ZmbArchiveKind::kPage, scrbId);
		Common::SeekableReadStream *scrbStream = _vm->getResource(ID_SCRB, resource);

		ZmbFeature *feature = new ZmbFeature(_vm, resource._id, 0, 0, resource);
		feature->parseStream(scrbStream);
		debugPrintf("SCRB_%04u: FrameCount(%u) MaxFrameIdx(%u)\n", resource._id, feature->getFrameCount(), feature->getMaxFrameIdx());

		for (auto it = feature->begin(); it != feature->end(); it++) {
			ZmbHotspotGroup *hsGroup = it->_value;

			for (auto git = hsGroup->begin(); git != hsGroup->end(); git++) {
				ZmbHotspot hs = *git;
				debugPrintf("  Frame(%u): Hotspot ID(%u) at (%u, %u)\n", hs._frame, hs._shapeIdx, hs._x, hs._y);
			}
		}

		delete feature;
	}

	return false;
}

bool ZoombiniConsole::Cmd_DrawFeature(int argc, const char **argv) {
	if (argc != 3) {
		debugPrintf("Usage: drawFeature <imageId> <scrbId>\n");
		return true;
	}

	ZmbResource resource;
	if (!parseResourceId(argv[1], resource))
		return true;

	if (!_vm->hasResource(ID_TBMP, resource)) {
		debugPrintf("Cannot find resource tBMP(%s)\n", argv[1]);
		return true;
	}

	errno = 0;
	uint16 scrbId = static_cast<uint16>(strtoul(argv[2], nullptr, 10));
	if (errno != 0) {
		debugPrintf("Cannot parse argument %s!\n", argv[2]);
		return true;
	}

	if (!_vm->hasResource(ID_SCRB, ZmbResource(resource._archiveKind, scrbId))) {
		debugPrintf("Cannot find resource SCRB(%s)\n", argv[2]);
		return true;
	}

	ZoombiniDebugCommand cmd;
	cmd.setDrawFeature(resource, scrbId);
	_vm->openDebugDialog(cmd);
	return false;
}

bool ZoombiniConsole::Cmd_DumpFeature(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: dumpFeature <scrbId>\n");
		return true;
	}

	ZmbResource resource;
	if (!parseResourceId(argv[1], resource))
		return true;

	if (!_vm->hasResource(ID_SCRB, resource)) {
		debugPrintf("Cannot find resource SCRB(%s)\n", argv[1]);
		return true;
	}

	// Create output file
	ZoombiniPage *activePage = _vm->getActivePage();
	Common::String filename = Common::String::format("ZOOMBINI_page%02u_SCRB_%s.txt", static_cast<uint32>(activePage->getPageType()), resource.toString().c_str());
	Common::String filepath = "dumps/" + filename;

	Common::DumpFile out;
	if (!out.open(Common::Path(filepath, '/'), true)) {
		debugPrintf("Failed to open file for writing: %s\n", filepath.c_str());
		return true;
	}

	// Dump feature to file
	Common::SeekableReadStream *scrbStream = _vm->getResource(ID_SCRB, resource);

	ZmbFeature *feature = new ZmbFeature(_vm, resource._id, 0, 0, resource);
	feature->parseStream(scrbStream);

	Common::String lineBuffer = Common::String::format("SCRB_%s: FrameCount(%u) MaxFrameIdx(%u)\n", resource.toString().c_str(), feature->getFrameCount(), feature->getMaxFrameIdx());
	out.write(lineBuffer.c_str(), lineBuffer.size());

	for (auto it = feature->begin(); it != feature->end(); it++) {
		ZmbHotspotGroup *hsGroup = it->_value;

		for (auto git = hsGroup->begin(); git != hsGroup->end(); git++) {
			ZmbHotspot hs = *git;
			lineBuffer = Common::String::format("  Frame(%u): Hotspot ID(%u) at (%u, %u)\n", hs._frame, hs._shapeIdx, hs._x, hs._y);
			out.write(lineBuffer.c_str(), lineBuffer.size());
		}
	}

	delete feature;
	out.close();
	debugPrintf("Successfully exported SCRB feature to %s\n", filename.c_str());
	return false;
}

bool ZoombiniConsole::Cmd_DumpFeatures(int argc, const char **argv) {
	if (argc != 1) {
		debugPrintf("Usage: dumpFeatures\n");
		return true;
	}

	// Get all SCRB resource IDs from the active page
	Common::Array<uint16> resIds = _vm->getResourceIDList(ZmbArchiveKind::kPage, ID_SCRB);
	if (resIds.empty()) {
		debugPrintf("No SCRB resources found in current page\n");
		return true;
	}

	// Create output file
	ZoombiniPage *activePage = _vm->getActivePage();
	Common::String filename = Common::String::format("ZOOMBINI_page%02u_SCRB_features.txt", static_cast<uint32>(activePage->getPageType()));
	Common::String filepath = "dumps/" + filename;

	Common::DumpFile out;
	if (!out.open(Common::Path(filepath, '/'), true)) {
		debugPrintf("Failed to open file for writing: %s\n", filepath.c_str());
		return true;
	}

	// Dump each feature to file
	for (uint i = 0; i < resIds.size(); i++) {
		uint16 scrbId = resIds[i];
		ZmbResource resId = ZmbResource(ZmbArchiveKind::kPage, scrbId);
		Common::SeekableReadStream *scrbStream = _vm->getResource(ID_SCRB, resId);

		ZmbFeature *feature = new ZmbFeature(_vm, scrbId, 0, 0, resId);
		feature->parseStream(scrbStream);

		Common::String lineBuffer = Common::String::format("SCRB_%u: FrameCount(%u) MaxFrameIdx(%u)\n", scrbId, feature->getFrameCount(), feature->getMaxFrameIdx());
		out.write(lineBuffer.c_str(), lineBuffer.size());

		for (auto it = feature->begin(); it != feature->end(); it++) {
			ZmbHotspotGroup *hsGroup = it->_value;

			for (auto git = hsGroup->begin(); git != hsGroup->end(); git++) {
				ZmbHotspot hs = *git;
				lineBuffer = Common::String::format("  Frame(%u): Hotspot ID(%u) at (%u, %u)\n", hs._frame, hs._shapeIdx, hs._x, hs._y);
				out.write(lineBuffer.c_str(), lineBuffer.size());
			}
		}

		delete feature;
	}

	out.close();
	debugPrintf("Successfully exported SCRB features to %s\n", filename.c_str());
	return false;
}

bool ZoombiniConsole::Cmd_PrintSnoidScript(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Print a single SCRS (Snoid Script) resource\n");
		debugPrintf("Usage: printSnoidScript <scrsId>\n");
		return true;
	}

	ZmbResource resource;
	if (!parseResourceId(argv[1], resource))
		return true;

	if (!_vm->hasResource(ID_SCRS, resource)) {
		debugPrintf("Cannot find resource SCRS(%s)\n", argv[1]);
		return true;
	}

	Common::SeekableReadStream *scrsStream = _vm->getResource(ID_SCRS, resource);

	ZmbSnoid *snoid = new ZmbSnoid(_vm, resource._id, ZmbFeature::FLAG_00000001_TYPE_SNOID);
	snoid->parseScrsStream(scrsStream);
	debugPrintf("SCRS_%04u: Variant(%u) FrameCount(%u) MaxFrameIdx(%u)\n", resource._id, snoid->getVariant(), snoid->getFrameCount(), snoid->getMaxFrameIdx());

	for (auto it = snoid->begin(); it != snoid->end(); it++) {
		ZmbHotspotGroup *hsGroup = it->_value;

		for (auto git = hsGroup->begin(); git != hsGroup->end(); git++) {
			ZmbHotspot hs = *git;
			debugPrintf("  Frame(%u): Hotspot ID(%u) at (%d, %d)\n", hs._frame, hs._shapeIdx, hs._x, hs._y);
		}
	}

	delete snoid;
	return true;
}

bool ZoombiniConsole::Cmd_PrintSnoidScripts(int argc, const char **argv) {
	if (argc != 1) {
		debugPrintf("Print all SCRS resources from the active page\n");
		debugPrintf("Usage: printSnoidScripts\n");
		return true;
	}

	Common::Array<uint16> resIds = _vm->getResourceIDList(ZmbArchiveKind::kPage, ID_SCRS);
	if (resIds.empty()) {
		debugPrintf("No SCRS resources found in current page\n");
		return true;
	}

	for (uint i = 0; i < resIds.size(); i++) {
		uint16 scrsId = resIds[i];
		ZmbResource resource = ZmbResource(ZmbArchiveKind::kPage, scrsId);
		Common::SeekableReadStream *scrsStream = _vm->getResource(ID_SCRS, resource);

		ZmbSnoid *snoid = new ZmbSnoid(_vm, resource._id, ZmbFeature::FLAG_00000001_TYPE_SNOID);
		snoid->parseScrsStream(scrsStream);
		debugPrintf("SCRS_%04u: Variant(%u) FrameCount(%u) MaxFrameIdx(%u)\n", resource._id, snoid->getVariant(), snoid->getFrameCount(), snoid->getMaxFrameIdx());

		for (auto it = snoid->begin(); it != snoid->end(); it++) {
			ZmbHotspotGroup *hsGroup = it->_value;

			for (auto git = hsGroup->begin(); git != hsGroup->end(); git++) {
				ZmbHotspot hs = *git;
				debugPrintf("  Frame(%u): Hotspot ID(%u) at (%d, %d)\n", hs._frame, hs._shapeIdx, hs._x, hs._y);
			}
		}

		delete snoid;
	}

	return false;
}

bool ZoombiniConsole::Cmd_DumpSnoidScript(int argc, const char **argv) {
	if (argc != 2) {
		debugPrintf("Usage: dumpSnoidScript <scrsId>\n");
		return true;
	}

	ZmbResource resource;
	if (!parseResourceId(argv[1], resource))
		return true;

	if (!_vm->hasResource(ID_SCRS, resource)) {
		debugPrintf("Cannot find resource SCRS(%s)\n", argv[1]);
		return true;
	}

	ZoombiniPage *activePage = _vm->getActivePage();
	Common::String filename = Common::String::format("ZOOMBINI_page%02u_SCRS_%s.txt", static_cast<uint32>(activePage->getPageType()), resource.toString().c_str());
	Common::String filepath = "dumps/" + filename;

	Common::DumpFile out;
	if (!out.open(Common::Path(filepath, '/'), true)) {
		debugPrintf("Failed to open file for writing: %s\n", filepath.c_str());
		return true;
	}

	Common::SeekableReadStream *scrsStream = _vm->getResource(ID_SCRS, resource);

	ZmbSnoid *snoid = new ZmbSnoid(_vm, resource._id, ZmbFeature::FLAG_00000001_TYPE_SNOID);
	snoid->parseScrsStream(scrsStream);

	Common::String lineBuffer = Common::String::format("SCRS_%s: Variant(%u) FrameCount(%u) MaxFrameIdx(%u)\n", resource.toString().c_str(), snoid->getVariant(), snoid->getFrameCount(), snoid->getMaxFrameIdx());
	out.write(lineBuffer.c_str(), lineBuffer.size());

	for (auto it = snoid->begin(); it != snoid->end(); it++) {
		ZmbHotspotGroup *hsGroup = it->_value;

		for (auto git = hsGroup->begin(); git != hsGroup->end(); git++) {
			ZmbHotspot hs = *git;
			lineBuffer = Common::String::format("  Frame(%u): Hotspot ID(%u) at (%d, %d)\n", hs._frame, hs._shapeIdx, hs._x, hs._y);
			out.write(lineBuffer.c_str(), lineBuffer.size());
		}
	}

	delete snoid;
	out.close();
	debugPrintf("Successfully exported SCRS snoid script to %s\n", filename.c_str());
	return false;
}

bool ZoombiniConsole::Cmd_DumpSnoidScripts(int argc, const char **argv) {
	if (argc != 1) {
		debugPrintf("Usage: dumpSnoidScripts\n");
		return true;
	}

	Common::Array<uint16> resIds = _vm->getResourceIDList(ZmbArchiveKind::kPage, ID_SCRS);
	if (resIds.empty()) {
		debugPrintf("No SCRS resources found in current page\n");
		return true;
	}

	ZoombiniPage *activePage = _vm->getActivePage();
	Common::String filename = Common::String::format("ZOOMBINI_page%02u_SCRS_snoidscripts.txt", static_cast<uint32>(activePage->getPageType()));
	Common::String filepath = "dumps/" + filename;

	Common::DumpFile out;
	if (!out.open(Common::Path(filepath, '/'), true)) {
		debugPrintf("Failed to open file for writing: %s\n", filepath.c_str());
		return true;
	}

	for (uint i = 0; i < resIds.size(); i++) {
		uint16 scrsId = resIds[i];
		ZmbResource resId = ZmbResource(ZmbArchiveKind::kPage, scrsId);
		Common::SeekableReadStream *scrsStream = _vm->getResource(ID_SCRS, resId);

		ZmbSnoid *snoid = new ZmbSnoid(_vm, scrsId, ZmbFeature::FLAG_00000001_TYPE_SNOID);
		snoid->parseScrsStream(scrsStream);

		Common::String lineBuffer = Common::String::format("SCRS_%u: Variant(%u) FrameCount(%u) MaxFrameIdx(%u)\n", scrsId, snoid->getVariant(), snoid->getFrameCount(), snoid->getMaxFrameIdx());
		out.write(lineBuffer.c_str(), lineBuffer.size());

		for (auto it = snoid->begin(); it != snoid->end(); it++) {
			ZmbHotspotGroup *hsGroup = it->_value;

			for (auto git = hsGroup->begin(); git != hsGroup->end(); git++) {
				ZmbHotspot hs = *git;
				lineBuffer = Common::String::format("  Frame(%u): Hotspot ID(%u) at (%d, %d)\n", hs._frame, hs._shapeIdx, hs._x, hs._y);
				out.write(lineBuffer.c_str(), lineBuffer.size());
			}
		}

		delete snoid;
	}

	out.close();
	debugPrintf("Successfully exported SCRS snoid scripts to %s\n", filename.c_str());
	return false;
}

bool ZoombiniConsole::Cmd_PlotPoint(int argc, const char **argv) {
	if (argc < 3 || argc > 4) {
		debugPrintf("Usage: plotPoint <x> <y> [color]\n");
		debugPrintf("  x, y: coordinates (0-based, hex supported with 0x prefix)\n");
		debugPrintf("  color: color value (8-bit palette index, default: %u)\n", ZoombiniGraphics::kColor0A_White);
		return true;
	}

	int32 xVal = 0, yVal = 0, colorVal = 0;
	if (!parseInt(argv[1], xVal) || !parseInt(argv[2], yVal))
		return true;

	uint32 color = static_cast<uint32>(ZoombiniGraphics::kColor0A_White);
	if (argc == 4) {
		if (!parseInt(argv[3], colorVal))
			return true;
		if (colorVal < 0 || colorVal > 0xFF) {
			debugPrintf("Error: Color must be 0-255\n");
			return true;
		}
		color = static_cast<uint32>(colorVal);
	}

	int16 x = static_cast<int16>(xVal);
	int16 y = static_cast<int16>(yVal);

	Graphics::Surface *screen = _vm->_gfx->getScreen(ZoombiniGraphics::kShapeScreen);
	if (x < 0 || x >= screen->w || y < 0 || y >= screen->h) {
		debugPrintf("Coordinates out of bounds (screen size: %d x %d)\n", screen->w, screen->h);
		return true;
	}

	ZoombiniDebugCommand cmd;
	cmd.setPlotPoint(x, y, color);
	_vm->openDebugDialog(cmd);
	return false;
}

bool ZoombiniConsole::Cmd_PlotLine(int argc, const char **argv) {
	if (argc < 5 || argc > 6) {
		debugPrintf("Usage: plotLine <x0> <y0> <x1> <y1> [color]\n");
		debugPrintf("  x0, y0: start coordinates (0-based, hex supported with 0x prefix)\n");
		debugPrintf("  x1, y1: end coordinates (0-based, hex supported with 0x prefix)\n");
		debugPrintf("  color: color value (8-bit palette index, default: %u)\n", ZoombiniGraphics::kColor0A_White);
		return true;
	}

	int32 x0Val = 0, y0Val = 0, x1Val = 0, y1Val = 0, colorVal = 0;
	if (!parseInt(argv[1], x0Val) || !parseInt(argv[2], y0Val) ||
	    !parseInt(argv[3], x1Val) || !parseInt(argv[4], y1Val))
		return true;

	uint32 color = static_cast<uint32>(ZoombiniGraphics::kColor0A_White);
	if (argc == 6) {
		if (!parseInt(argv[5], colorVal))
			return true;
		if (colorVal < 0 || colorVal > 0xFF) {
			debugPrintf("Error: Color must be 0-255\n");
			return true;
		}
		color = static_cast<uint32>(colorVal);
	}

	int16 x0 = static_cast<int16>(x0Val);
	int16 y0 = static_cast<int16>(y0Val);
	int16 x1 = static_cast<int16>(x1Val);
	int16 y1 = static_cast<int16>(y1Val);

	ZoombiniDebugCommand cmd;
	cmd.setPlotLine(x0, y0, x1, y1, color);
	_vm->openDebugDialog(cmd);
	return false;
}

bool ZoombiniConsole::Cmd_PlotRect(int argc, const char **argv) {
	if (argc < 5 || argc > 6) {
		debugPrintf("Usage: plotRect <x1> <y1> <x2> <y2> [color]\n");
		debugPrintf("  x1, y1: top-left corner coordinates (0-based, hex supported with 0x prefix)\n");
		debugPrintf("  x2, y2: bottom-right corner coordinates (0-based, hex supported with 0x prefix)\n");
		debugPrintf("  color: color value (8-bit palette index, default: %u)\n", ZoombiniGraphics::kColor0A_White);
		return true;
	}

	int32 x1Val = 0, y1Val = 0, x2Val = 0, y2Val = 0, colorVal = 0;
	if (!parseInt(argv[1], x1Val) || !parseInt(argv[2], y1Val) ||
	    !parseInt(argv[3], x2Val) || !parseInt(argv[4], y2Val))
		return true;

	uint32 color = static_cast<uint32>(ZoombiniGraphics::kColor0A_White);
	if (argc == 6) {
		if (!parseInt(argv[5], colorVal))
			return true;
		if (colorVal < 0 || colorVal > 0xFF) {
			debugPrintf("Error: Color must be 0-255\n");
			return true;
		}
		color = static_cast<uint32>(colorVal);
	}

	int16 x1 = static_cast<int16>(x1Val);
	int16 y1 = static_cast<int16>(y1Val);
	int16 x2 = static_cast<int16>(x2Val);
	int16 y2 = static_cast<int16>(y2Val);

	if (x2 <= x1 || y2 <= y1) {
		debugPrintf("Invalid rectangle coordinates\n");
		return true;
	}

	ZoombiniDebugCommand cmd;
	cmd.setPlotRect(x1, y1, x2, y2, color);
	_vm->openDebugDialog(cmd);
	return false;
}

bool ZoombiniConsole::Cmd_DumpAllResources(int argc, const char **argv) {
	if (argc > 2) {
		debugPrintf("Dump all raw resources from currently loaded page and/or system archives\n");
		debugPrintf("Usage: dumpAllResources [page|system|all]\n");
		return true;
	}

	bool dumpPage = true;
	bool dumpSystem = true;
	if (argc == 2) {
		if (strcmp(argv[1], "page") == 0) {
			dumpSystem = false;
		} else if (strcmp(argv[1], "system") == 0) {
			dumpPage = false;
		} else if (strcmp(argv[1], "all") != 0) {
			debugPrintf("Usage: dumpAllResources [page|system|all]\n");
			return true;
		}
	}

	ZoombiniPage *activePage = _vm->getActivePage();
	uint32 pageType = (activePage != nullptr) ? static_cast<uint32>(activePage->getPageType()) : 0;

	uint32 totalExported = 0;

	if (dumpPage) {
		uint archiveCount = _vm->getArchiveCount(ZmbArchiveKind::kPage);
		if (archiveCount == 0) {
			debugPrintf("No page archives loaded\n");
		} else {
			Common::String dumpDir = Common::String::format("dumps/ZOOMBINI_dump_page%02u", pageType);
			uint32 globalIndex = 0;

			for (uint archiveIdx = 0; archiveIdx < archiveCount; archiveIdx++) {
				Archive *archive = _vm->getArchive(ZmbArchiveKind::kPage, archiveIdx);
				Common::Array<uint32> types = archive->getResourceTypeList();

				for (uint32 tag : types) {
					Common::Array<uint16> ids = archive->getResourceIDList(tag);

					for (uint16 resId : ids) {
						Common::SeekableReadStream *stream = archive->getResource(tag, resId);
						if (!stream) {
							debugPrintf("Warning: Failed to read %s %u from page archive %u\n", tag2str(tag), resId, archiveIdx);
							continue;
						}

						Common::String filename = Common::String::format("%04u_%02u_%s_%u.bin",
							globalIndex, archiveIdx, tag2str(tag), resId);
						Common::String filepath = dumpDir + "/" + filename;

						Common::DumpFile out;
						if (!out.open(Common::Path(filepath, '/'), true)) {
							debugPrintf("Warning: Failed to open %s for writing\n", filepath.c_str());
							delete stream;
							continue;
						}

						uint32 size = stream->size();
						byte *buffer = new byte[size];
						stream->read(buffer, size);
						out.write(buffer, size);
						delete[] buffer;
						delete stream;
						out.close();

						globalIndex++;
						totalExported++;
					}
				}
			}

			debugPrintf("Dumped %u page resources to %s/\n", globalIndex, dumpDir.c_str());
		}
	}

	if (dumpSystem) {
		Archive *sysArchive = _vm->getArchive(ZmbArchiveKind::kSystem, 0);
		if (!sysArchive) {
			debugPrintf("System archive not available\n");
		} else {
			Common::String dumpDir = "dumps/ZOOMBINI_dump_sys";
			Common::Array<uint32> types = sysArchive->getResourceTypeList();
			uint32 globalIndex = 0;

			for (uint32 tag : types) {
				Common::Array<uint16> ids = sysArchive->getResourceIDList(tag);

				for (uint16 resId : ids) {
					Common::SeekableReadStream *stream = sysArchive->getResource(tag, resId);
					if (!stream) {
						debugPrintf("Warning: Failed to read %s %u from system archive\n", tag2str(tag), resId);
						continue;
					}

					Common::String filename = Common::String::format("%04u_00_%s_%u.bin",
						globalIndex, tag2str(tag), resId);
					Common::String filepath = dumpDir + "/" + filename;

					Common::DumpFile out;
					if (!out.open(Common::Path(filepath, '/'), true)) {
						debugPrintf("Warning: Failed to open %s for writing\n", filepath.c_str());
						delete stream;
						continue;
					}

					uint32 size = stream->size();
					byte *buffer = new byte[size];
					stream->read(buffer, size);
					out.write(buffer, size);
					delete[] buffer;
					delete stream;
					out.close();

					globalIndex++;
					totalExported++;
				}
			}

			debugPrintf("Dumped %u system resources to %s/\n", globalIndex, dumpDir.c_str());
		}
	}

	debugPrintf("Full dump complete. Total: %u resources exported.\n", totalExported);
	return true;
}

bool ZoombiniConsole::Cmd_GoXfer(int argc, const char **argv) {
	static const struct {
		const char *name;
		ZMB_SI_PAGE siPage;
		const char *desc;
	} xferRoutes[] = {
		{ "picker",    ZMB_SI_PICKER_01,     "Picker -> Bridge (From Isle)" },
		{ "bridge",    ZMB_SI_BRIDGE_02,     "Bridge -> Tunnels (Big Bad & Hungry)" },
		{ "tunnels",   ZMB_SI_TUNNELS_03,    "Tunnels -> Pizza (Big Bad & Hungry)" },
		{ "pizza",     ZMB_SI_PIZZA_04,      "Pizza -> Basecamp1 (Big Bad & Hungry)" },
		{ "bc1north",  ZMB_SI_BC1_NORTH_05,  "Basecamp1 North -> Ferry (Who's Bayou)" },
		{ "bc1south",  ZMB_SI_BC1_SOUTH_06,  "Basecamp1 South -> Lilly (Who's Bayou)" },
		{ "ferry",     ZMB_SI_FERRY_07,      "Ferry -> Slides (Who's Bayou)" },
		{ "lilly",     ZMB_SI_LILLY_08,      "Lilly -> Basecamp2 (Who's Bayou)" },
		{ "slides",    ZMB_SI_SLIDES_09,     "Slides -> Fleens (Deep Dark Forest)" },
		{ "fleens",    ZMB_SI_FLEENS_10,     "Fleens -> Hotel (Deep Dark Forest)" },
		{ "hotel",     ZMB_SI_HOTEL_11,      "Hotel -> Net (Deep Dark Forest)" },
		{ "net",       ZMB_SI_NET_12,        "Net -> Basecamp2 (Deep Dark Forest)" },
		{ "bc2",       ZMB_SI_BASECAMP2_13,  "Basecamp2 -> Caves (Mountain of Despair)" },
		{ "caves",     ZMB_SI_CAVES_14,      "Caves -> Smoke (Mountain of Despair)" },
		{ "smoke",     ZMB_SI_SMOKE_15,      "Smoke -> Maze (Mountain of Despair)" },
		{ "maze",      ZMB_SI_MAZE_16,       "Maze -> Town (To Town)" },
	};

	if (argc != 2) {
		debugPrintf("Jump to the xfer (transition) page with a chosen route.\n");
		debugPrintf("Usage: goXfer <route>\n");
		debugPrintf("Available routes:\n");
		for (uint i = 0; i < ARRAYSIZE(xferRoutes); i++) {
			debugPrintf("  %-10s  SI %2d  %s\n", xferRoutes[i].name,
				(int)xferRoutes[i].siPage, xferRoutes[i].desc);
		}
		return true;
	}

	// Match by name (case-insensitive) or SI page number
	ZMB_SI_PAGE targetSi = ZMB_SI_MINUS1;
	int32 numVal;
	if (ZmbResource::parseInt(argv[1], numVal)) {
		// Numeric: treat as SI page index
		for (uint i = 0; i < ARRAYSIZE(xferRoutes); i++) {
			if ((int16)numVal == xferRoutes[i].siPage) {
				targetSi = xferRoutes[i].siPage;
				break;
			}
		}
	} else {
		// Name match
		for (uint i = 0; i < ARRAYSIZE(xferRoutes); i++) {
			if (scumm_stricmp(argv[1], xferRoutes[i].name) == 0) {
				targetSi = xferRoutes[i].siPage;
				break;
			}
		}
	}

	if (targetSi == ZMB_SI_MINUS1) {
		debugPrintf("Unknown route '%s'. Use goXfer without arguments to see available routes.\n", argv[1]);
		return true;
	}

	// Generate 16 random snoids as the active pack (same as practice mode)
	_vm->_state->generateRandomPack();

	// Close the current page and queue the xfer transition
	_vm->_xferSrcSiPage = targetSi;
	_vm->setNextPage(ZoombiniPageType::kXfer);
	if (_vm->getActivePage())
		_vm->getActivePage()->close();

	debugPrintf("Generated 16 random snoids in active pack\n");
	debugPrintf("Jumping to xfer with source SI page %d\n", (int)targetSi);
	return false; // Close the debugger console
}

bool ZoombiniConsole::Cmd_GoPractice(int argc, const char **argv) {
	static const struct {
		const char *name;
		ZoombiniPageType pageType;
		const char *desc;
	} puzzlePages[] = {
		{ "bridge",  ZoombiniPageType::kBridge,  "Allergic Cliffs" },
		{ "tunnels", ZoombiniPageType::kTunnels, "Stone Cold Caves" },
		{ "pizza",   ZoombiniPageType::kPizza,   "Pizza Pass" },
		{ "ferry",   ZoombiniPageType::kFerry,   "Captain Cajun's Ferryboat" },
		{ "lilly",   ZoombiniPageType::kLilly,   "Titanic Tattooed Toads" },
		{ "slides",  ZoombiniPageType::kSlides,  "Stone Rise" },
		{ "fleens",  ZoombiniPageType::kFleens,  "Fleens!" },
		{ "hotel",   ZoombiniPageType::kHotel,   "Hotel Dimensia" },
		{ "net",     ZoombiniPageType::kNet,     "Mudball Wall" },
		{ "caves",   ZoombiniPageType::kCaves,   "Cave of the Bubblesmokers" },
		{ "smoke",   ZoombiniPageType::kSmoke,   "Snack Attack" },
		{ "maze",    ZoombiniPageType::kMaze,    "Mirror Machine" },
	};

	if (argc != 3) {
		debugPrintf("Jump directly to a puzzle page in practice mode at a specified difficulty level.\n");
		debugPrintf("Usage: goPractice <puzzle> <level>\n");
		debugPrintf("  <level> is 1-4 (1=easiest, 4=hardest)\n");
		debugPrintf("Available puzzles:\n");
		for (uint i = 0; i < ARRAYSIZE(puzzlePages); i++) {
			debugPrintf("  %-10s  %s\n", puzzlePages[i].name, puzzlePages[i].desc);
		}
		return true;
	}

	// Parse puzzle name or page type number
	ZoombiniPageType targetPage = ZoombiniPageType::kNone;
	int32 numVal;
	if (ZmbResource::parseInt(argv[1], numVal)) {
		for (uint i = 0; i < ARRAYSIZE(puzzlePages); i++) {
			if ((int16)numVal == (int16)puzzlePages[i].pageType) {
				targetPage = puzzlePages[i].pageType;
				break;
			}
		}
	} else {
		for (uint i = 0; i < ARRAYSIZE(puzzlePages); i++) {
			if (scumm_stricmp(argv[1], puzzlePages[i].name) == 0) {
				targetPage = puzzlePages[i].pageType;
				break;
			}
		}
	}

	if (targetPage == ZoombiniPageType::kNone) {
		debugPrintf("Unknown puzzle '%s'. Use goPractice without arguments to see available puzzles.\n", argv[1]);
		return true;
	}

	// Parse level
	int32 level;
	if (!ZmbResource::parseInt(argv[2], level) || level < 1 || level > 4) {
		debugPrintf("Invalid level '%s'. Must be 1-4.\n", argv[2]);
		return true;
	}

	// Set practice mode at the specified difficulty level
	_vm->_state->_practiceLevel = (uint16)level;

	// Generate 16 random snoids as the active pack
	_vm->_state->generateRandomPack();

	// Navigate directly to the puzzle page (skip xfer transition)
	_vm->setNextPage(targetPage);
	if (_vm->getActivePage())
		_vm->getActivePage()->close();

	debugPrintf("Practice mode: level %d\n", (int)level);
	debugPrintf("Generated 16 random snoids in active pack\n");
	debugPrintf("Jumping directly to puzzle page %d\n", (int)targetPage);
	return false; // Close the debugger console
}

bool ZoombiniConsole::exportSurfaceToBMP(const Common::String &filename, const Graphics::Surface *surface, const byte *palette) {
	Common::String filepath = "dumps/" + filename;

	Common::DumpFile out;
	if (!out.open(Common::Path(filepath, '/'), true)) {
		debugPrintf("Failed to open file %s for writing\n", filepath.c_str());
		return false;
	}

	// Image::writeBMP() expects that palette buffer has 256 colors (768 bytes)
	return Image::writeBMP(out, *surface, palette);
}

#endif

} // End of namespace Mohawk
