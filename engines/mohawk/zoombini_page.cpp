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

#include "mohawk/cursors.h"
#include "mohawk/resource.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_page.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"
#include "mohawk/zoombini_text.h"

namespace Mohawk {

ZoombiniPage::ZoombiniPage(MohawkEngine_Zoombini *vm, ZoombiniPageCategory pageCategory, ZoombiniPageType pageType) : _vm(vm), _pageCategory(pageCategory), _pageType(pageType) {
	_pageStartFrameTime = _vm->_system->getMillis();
	_pageStartFrameCounter = _pageStartFrameTime / MohawkEngine_Zoombini::kAnimateFrameTimeMs;
	_currentFrameTime = _pageStartFrameTime;
	_currentFrameCounter = _pageStartFrameCounter;
	_lastFrameTime = _pageStartFrameTime;
	_lastFrameCounter = _pageStartFrameCounter;
}

ZoombiniPage::~ZoombiniPage() {
	clear();
}

void ZoombiniPage::onAnimFrame() {
	// Tick every snoid's animation state machine before rendering.
	// IDA: onRender_ZoombiniAnimation_452B9C is invoked once per animation frame per snoid.
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		it->second->onSnoidAnimTick(this);
	}

	renderFeatures();
	checkCloseFeatures();
}

bool ZoombiniPage::isClosed() {
	return _isClosed;
}

void ZoombiniPage::close() {
	if (_isClosed)
		return;

	onFadeOut();
	_isClosed = true;
}

void ZoombiniPage::onFrame() {
	_currentFrameTime = _vm->_system->getMillis();
	_currentFrameCounter = _currentFrameTime / MohawkEngine_Zoombini::kAnimateFrameTimeMs;

	onEveryFrame();

	uint32 frameElapsed = _currentFrameTime - _lastFrameTime;
	if (MohawkEngine_Zoombini::kAnimateFrameTimeMs <= frameElapsed || _doForceRedraw) {
		do {
			_doForceRedraw = false;
			onAnimFrame();
		} while (_doForceRedraw);

		_lastFrameTime = _currentFrameTime;
		_lastFrameCounter = _currentFrameCounter;
	}
}

void ZoombiniPage::openArchive(const Common::String &mhkName) {
	MohawkArchive *mhk = new MohawkArchive();
	if (!mhk->openFile(Common::Path(mhkName))) {
		error("Cannot open file '%s'", mhkName.c_str());
	}

	_vm->addPageArchive(mhk);
}

ZmbEventHandleResult ZoombiniPage::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;
	Common::Array<ZmbFeature *> eventList;
	buildSortedEventList(eventList);
	for (ZmbFeature *feature : eventList) {
		result = feature->onLButtonDown(this, absPos, relPos);
		if (result == ZmbEventHandleResult::kConsumed)
			break;
	}
	return result;
}

ZmbEventHandleResult ZoombiniPage::onLButtonUp(const Common::Point &absPos, const Common::Point &relPos) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;
	Common::Array<ZmbFeature *> eventList;
	buildSortedEventList(eventList);
	for (ZmbFeature *feature : eventList) {
		result = feature->onLButtonUp(this, absPos, relPos);
		if (result == ZmbEventHandleResult::kConsumed)
			break;
	}
	return result;
}

ZmbEventHandleResult ZoombiniPage::onWheelUp(const Common::Point &absPos) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;
	Common::Array<ZmbFeature *> eventList;
	buildSortedEventList(eventList);
	for (ZmbFeature *feature : eventList) {
		result = feature->onWheelUp(this, absPos);
		if (result == ZmbEventHandleResult::kConsumed)
			break;
	}
	return result;
}

ZmbEventHandleResult ZoombiniPage::onWheelDown(const Common::Point &absPos) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;
	Common::Array<ZmbFeature *> eventList;
	buildSortedEventList(eventList);
	for (ZmbFeature *feature : eventList) {
		result = feature->onWheelDown(this, absPos);
		if (result == ZmbEventHandleResult::kConsumed)
			break;
	}
	return result;
}

ZmbEventHandleResult ZoombiniPage::onMouseMove(const Common::Point &absPos, const Common::Point &relPos) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;
	Common::Array<ZmbFeature *> eventList;
	buildSortedEventList(eventList);
	for (ZmbFeature *feature : eventList) {
		result = feature->onMouseMove(this, absPos, relPos);
		if (result == ZmbEventHandleResult::kConsumed)
			break;
	}
	return result;
}

ZmbEventHandleResult ZoombiniPage::onKeyDown(const Common::KeyState &kbd, bool kbdRepeat) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;
	Common::Array<ZmbFeature *> eventList;
	buildSortedEventList(eventList);
	for (ZmbFeature *feature : eventList) {
		result = feature->onKeyDown(this, kbd, kbdRepeat);
		if (result == ZmbEventHandleResult::kConsumed)
			break;
	}
	return result;
}

ZmbEventHandleResult ZoombiniPage::onKeyUp(const Common::KeyState &kbd, bool kbdRepeat) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;
	Common::Array<ZmbFeature *> eventList;
	buildSortedEventList(eventList);
	for (ZmbFeature *feature : eventList) {
		result = feature->onKeyUp(this, kbd, kbdRepeat);
		if (result == ZmbEventHandleResult::kConsumed)
			break;
	}
	return result;
}

ZmbEventHandleResult ZoombiniPage::onQuit() {
	// No per-feature quit dispatch — the original engine has no per-feature
	// quit callback. Page subclasses can override this for custom cleanup.
	return ZmbEventHandleResult::kPassthrough;
}

ZmbFeature *ZoombiniPage::loadScrbFeature(ZmbResource imgResource, uint16 scrbId, uint32 frameInterval, uint32 flags, const ZmbFeature::EventHooks &eventHooks) {
	return registerFeature(this, _scrbFeatureMap, imgResource, scrbId, frameInterval, Common::Point(0, 0), flags, true, nullptr, eventHooks);
}

ZmbFeature *ZoombiniPage::loadScrbFeature(ZmbResource imgResource, uint16 scrbId, uint32 frameInterval, const Common::Point &pointRef, uint32 flags, const ZmbFeature::EventHooks &eventHooks) {
	return registerFeature(this, _scrbFeatureMap, imgResource, scrbId, frameInterval, pointRef, flags, true, nullptr, eventHooks);
}

ZmbFeature *ZoombiniPage::loadVirtualFeature(ZmbResource imgResource, uint16 virtFeatureId, const Common::Array<ZmbHotspot> &hotspots, uint32 frameInterval, uint32 flags, const ZmbFeature::EventHooks &eventHooks) {
	return registerFeature(this, _virtualFeatureMap, imgResource, virtFeatureId, frameInterval, Common::Point(0, 0), flags, false, &hotspots, eventHooks);
}

ZmbFeature *ZoombiniPage::loadVirtualFeature(uint16 virtFeatureId, uint32 frameInterval, uint32 flags, const ZmbFeature::EventHooks &eventHooks) {
	return registerFeature(this, _virtualFeatureMap, ZmbResource(ZmbArchiveKind::kPage, 0), virtFeatureId, frameInterval, Common::Point(0, 0), flags, false, nullptr, eventHooks);
}

ZmbFeature *ZoombiniPage::registerFeature(ZoombiniPage *page, Common::StableMap<uint16, ZmbFeature *> &featureMap, ZmbResource imgResource, uint16 scrbId, uint32 frameInterval, const Common::Point &pointRef, uint32 flags, bool isPhysicalScrb, const Common::Array<ZmbHotspot> *virtualHotspots, const ZmbFeature::EventHooks &eventHooks) {
	auto it = featureMap.find(scrbId);
	if (it != featureMap.end()) {
		error("Duplicated feature id %u", scrbId);
		return nullptr;
	}

	ZmbFeature *feature = new ZmbFeature(page->_vm, scrbId, frameInterval, pointRef, flags, imgResource);
	featureMap[scrbId] = feature;

	if (isPhysicalScrb) {
		Common::SeekableReadStream *scrbStream = page->_vm->getResource(ID_SCRB, ZmbResource(imgResource._archiveKind, scrbId));
		feature->parseStream(scrbStream);
		scrbStream = nullptr;
	} else if (virtualHotspots) {
		feature->setVirtualHotspots(*virtualHotspots);
	}

	feature->initValues();

	// Always register hooks for normal per-frame rendering
	feature->setEventHooks(eventHooks);

	// Binary: registerSCRB_45F60C calls onPreRenderSRCB_standard immediately for
	// FLAG_00002000_DRAW_ON_REG features, which invokes loadSCRB_460384 → updates
	// clickRect from the drawn output. This gives the feature a valid sort rect
	// for the very first frame's Z-sort, before the regular render loop runs.
	if (feature->hasFlag(ZmbFeature::FLAG_00002000_DRAW_ON_REG)) {
		ZmbFeature::OnRenderFunc renderFunc = eventHooks._renderFunc;
		if (renderFunc == nullptr)
			renderFunc = &ZoombiniPage::blitShapes;
		(page->*renderFunc)(feature);
	}

	return feature;
}

ZmbFeature *ZoombiniPage::loadSubFeature(ZmbFeature *parentFeature, ZmbResource imgResource, uint16 scrbId) {
	uint32 flags = parentFeature->getFlags();
	if ((flags & ZmbFeature::FLAG_02000000_RANDOM_FRAME) != 0) {
		flags &= ~ZmbFeature::FLAG_02000000_RANDOM_FRAME;
	}

	// SubFeature inherits the frame interval and flags from the parent feature (with some adjustments)
	ZmbFeature *subFeature = new ZmbFeature(_vm, scrbId, parentFeature->getFrameInterval(), flags, imgResource);

	Common::SeekableReadStream *scrbStream = _vm->getResource(ID_SCRB, ZmbResource(imgResource._archiveKind, scrbId));
	subFeature->parseStream(scrbStream);
	subFeature->initValues();
	subFeature->setEventHooks(ZmbFeature::EventHooks());

	parentFeature->setSubFeature(subFeature);

	return subFeature;
}

ZmbFeature *ZoombiniPage::createMainFeatureHead(uint32 flags) {
	ZmbFeature *head = new ZmbFeature(_vm, 0, 0, flags, ZmbResource(ZmbArchiveKind::kPage, 0));
	head->initValues();
	_mainFeatureHeads.push_back(head);
	return head;
}

void ZoombiniPage::unloadScrbFeature(uint16 scrbId) {
	deregisterFeature(_scrbFeatureMap, scrbId);
}

void ZoombiniPage::unloadVirtualFeature(uint16 virtFeatureId) {
	deregisterFeature(_virtualFeatureMap, virtFeatureId);
}

void ZoombiniPage::loadScrbOntoFeature(ZmbFeature *feature, uint16 newScrbId, bool scheduleRender) {
	// IDA: scrb_loadOnRunner (0x460384) — page-level convenience wrapper.
	// Resolves the SCRB resource and delegates to ZmbFeature::loadScrbData().
	if (!feature)
		return;

	// IDA: if scriptId == 0, reload the runner's current wResId
	uint16 scrbId = newScrbId;
	if (scrbId == 0)
		scrbId = feature->getId();

	Common::SeekableReadStream *stream = _vm->getResource(ID_SCRB, ZmbResource(feature->getResource()._archiveKind, scrbId));
	if (!stream) {
		warning("loadScrbOntoFeature: cannot load SCRB %u", scrbId);
		return;
	}

	feature->loadScrbData(stream, scheduleRender);
}

void ZoombiniPage::attachSubFeature(ZmbFeature *subFeature) {
	uint16 id = subFeature->getId();

	// Guard against duplicate registration (e.g. user clicks before animation finishes)
	if (_subFeatureMap.find(id) != _subFeatureMap.end())
		return;

	// Insert without taking ownership - the parent feature still owns this pointer
	_subFeatureMap[id] = subFeature;
}

void ZoombiniPage::deregisterFeature(Common::StableMap<uint16, ZmbFeature *> &featureMap, uint16 featureId) {
	auto it = featureMap.find(featureId);
	if (it == featureMap.end()) {
		error("Cannot unload a not loaded feature id %u", featureId);
		return;
	}

	ZmbFeature *feature = it->second;
	featureMap.erase(it);
	delete feature;
}

void ZoombiniPage::loadNODE(ZmbArchiveKind archiveKind, uint16 nodeResId) {
	ZmbNode *node = new ZmbNode();
	Common::SeekableReadStream *stream = _vm->getResource(ID_NODE, ZmbResource(archiveKind, nodeResId));
	node->parseStream(stream);
	// Load the companion PATH resource (same ID, PATH tag) if present.
	// IDA: dword_4A48A0 = PATH data, pMhkResNode_4A48A4 = NODE data.  Both share the same
	// resource number and are used together by snoidPath_initRoute_454CA9/snoidPath_stepAndComputeVelocity_4548DF for path routing.
	if (_vm->hasResource(ID_PATH, ZmbResource(archiveKind, nodeResId))) {
		Common::SeekableReadStream *pathStream = _vm->getResource(ID_PATH, ZmbResource(archiveKind, nodeResId));
		node->parsePathStream(pathStream);
	}
	_nodeMap[nodeResId] = node;
}

void ZoombiniPage::loadREGS(ZmbArchiveKind archiveKind, uint16 baseResId) {
	ZmbRegs *regs = new ZmbRegs();
	regs->parseStreams(_vm, archiveKind, baseResId, baseResId + 1);
	_regsMap[baseResId] = regs;
}

/**
 * Categorize the feature into one of the 4 render groups based on its flags.
 * IDA: runner_zsortPartitionAndSort (0x4608AF) — check order matters.
 *
 * Priority: LOOP_ANIM → pre-existing OVERLAY → entity type → normalList.
 * LOOP_ANIM is checked FIRST — SCRB features with LOOP_ANIM go to loopAnimList
 * (unsorted, rendered behind sorted entries). Snoids must NOT have LOOP_ANIM;
 * they should have bare TYPE_SNOID (0x1) so the exact-match check routes them
 * to entityList for proper depth sorting by (bottom, left).
 */
void ZoombiniPage::categorizeFeature(ZmbFeature *feature, Common::Array<ZmbFeature *> &loopAnimList, Common::Array<ZmbFeature *> &normalList, Common::Array<ZmbFeature *> &entityList, Common::Array<ZmbFeature *> &overlayList) {
	// IDA runner_zsortPartitionAndSort 0x4608AF: check order matters.
	// LOOP_ANIM is for SCRB features only. Snoids use bare TYPE_SNOID (0x1)
	// → entityList (sorted by depth). See zmb_registerSnoidFeatureRunner 0x452A64.
	if (feature->hasFlag(ZmbFeature::FLAG_00008000_LOOP_ANIM)) {
		loopAnimList.push_back(feature);
	} else if (feature->hasFlag(ZmbFeature::FLAG_04000000_OVERLAY)) {
		// IDA 0x460902: pre-existing OVERLAY flag → overlayList (unsorted,
		// rendered between loopAnim and sorted normal+entity features).
		overlayList.push_back(feature);
	} else if (feature->getFlags() == ZmbFeature::FLAG_00000001_TYPE_SNOID || feature->getFlags() == ZmbFeature::FLAG_00000002_TYPE_TOWN_ENTITY) {
		// IDA 0x46092C: entity type → entityList.
		// Binary uses exact 32-bit match: `cmp dword ptr [eax+20h], 1` / `cmp dword ptr [eax+20h], 2`.
		// Only bare TYPE_SNOID (0x1) or TYPE_TOWN_ENTITY (0x2) with NO other flags.
		// Snoids with additional flags (e.g., POS_DELTA 0x800001) go to normalList.
		entityList.push_back(feature);
	} else {
		// Normal feature: force-set OVERLAY so it draws to the overlay screen,
		// unless it is a TOPMOST button/dialog that belongs on the shape screen.
		if (!feature->hasFlag(ZmbFeature::FLAG_00001000_TOPMOST))
			feature->addFlag(ZmbFeature::FLAG_04000000_OVERLAY);
		normalList.push_back(feature);
	}
}

/**
 * Sort features ascending by (sortRect.bottom, sortRect.left).
 *
 * Features with FLAG_00001000_TOPMOST skip past all sorted entries and are appended at the tail of their group (render last within the group).
 * sortRect is populated from clickRect updated during the previous frame.
 */
void ZoombiniPage::insertionSortFeatures(Common::Array<ZmbFeature *> &list) {
	if (list.size() <= 1)
		return;

	// IDA zsort_insertionSortByDepthAndX (0x4609F7):
	// Insertion sort ascending by (sortRect.bottom, sortRect.left).
	// TOPMOST check is on the INCOMING node only (test byte ptr [edx+21h], 10h).
	// When TOPMOST, skip comparison → traverse to tail → append at end.
	// Non-TOPMOST incoming walks past ALL existing nodes (including TOPMOST ones)
	// without barrier behavior — TOPMOST existing nodes are never checked.
	for (uint32 i = 1; i < list.size(); i++) {
		ZmbFeature *key = list[i];

		if (key->hasFlag(ZmbFeature::FLAG_00001000_TOPMOST)) {
			// TOPMOST incoming: binary skips sort comparison and appends at tail.
			// Since we process left-to-right and TOPMOST items never shift left,
			// leaving it in place is correct — it's already at the rightmost
			// position seen so far and will stay at the tail.
			continue;
		}

		const Common::Rect &keyRect = key->getSortRect();
		int32 j = (int32)i - 1;
		while (j >= 0) {
			const Common::Rect &cRect = list[j]->getSortRect();
			// Shift right when existing has larger sort key.
			// No TOPMOST barrier check on existing — binary walks past them.
			if (cRect.bottom > keyRect.bottom ||
				(cRect.bottom == keyRect.bottom && cRect.left > keyRect.left)) {
				list[j + 1] = list[j];
				j--;
			} else {
				break;
			}
		}
		list[j + 1] = key;
	}
}

/**
 * IDA: zsort_mergeEntityListIntoSortedList (0x460AD5)
 *
 * Merges a sorted incoming list into an existing list (which may contain
 * loopAnim + overlay + previously merged entries) using the (bottom, left)
 * sort key, with ZSORT constraint checking on existing entries.
 *
 * Binary behavior:
 * - Skips past LOOP_ANIM entries in existing list (scan starts after them)
 * - TOPMOST incoming → append at tail
 * - TOPMOST existing → insert incoming before it
 * - Sort key match → check ZSORT constraints on existing entry:
 *   - No vertical overlap (incoming.bottom < existing.top) → always allow
 *   - ZSORT_LEFT:   block if incoming.left < existing.left
 *   - ZSORT_RIGHT:  block if incoming.right > existing.right
 *   - ZSORT_BOTTOM: block if incoming.top < existing.top
 * - After insertion, scan position advances to the existing node we
 *   inserted before (not past it), since incoming list is sorted.
 *
 * Called TWICE by runner_zsortPartitionAndSort:
 *   1. Merge sorted normalList into (loopAnim + overlay)
 *   2. Merge sorted entityList into the combined result
 */
void ZoombiniPage::mergeSortedListInto(Common::Array<ZmbFeature *> &existingList, const Common::Array<ZmbFeature *> &incomingList) {
	if (incomingList.empty())
		return;

	// Find scan start: skip past LOOP_ANIM entries in existing list.
	// IDA 0x460AE7: walks forward while [eax+21h] & 0x80 and [eax+4] != 0.
	uint32 scanStart = 0;
	while (scanStart < existingList.size() &&
	       existingList[scanStart]->hasFlag(ZmbFeature::FLAG_00008000_LOOP_ANIM) &&
	       scanStart + 1 < existingList.size()) {
		scanStart++;
	}

	uint32 scanPos = scanStart;

	for (uint32 k = 0; k < incomingList.size(); k++) {
		ZmbFeature *incoming = incomingList[k];

		// TOPMOST incoming: append at tail (IDA 0x460B3D–0x460B5C).
		if (incoming->hasFlag(ZmbFeature::FLAG_00001000_TOPMOST)) {
			existingList.push_back(incoming);
			continue;
		}

		const Common::Rect &inRect = incoming->getSortRect();
		uint32 insertPos = existingList.size(); // default: append at end
		bool found = false;

		for (uint32 i = scanPos; i < existingList.size(); i++) {
			ZmbFeature *existing = existingList[i];

			// TOPMOST existing: insert incoming before it (IDA 0x460B69–0x460B7F).
			if (existing->hasFlag(ZmbFeature::FLAG_00001000_TOPMOST)) {
				insertPos = i;
				found = true;
				break;
			}

			const Common::Rect &exRect = existing->getSortRect();

			// Sort key comparison: incoming should go before existing?
			// IDA 0x460BB0: incoming.bottom < existing.bottom, or
			// (equal bottom and incoming.left < existing.left).
			bool candidatePosition =
				inRect.bottom < exRect.bottom ||
				(inRect.bottom == exRect.bottom && inRect.left < exRect.left);
			if (!candidatePosition)
				continue;

			// ZSORT constraint check on existing entry (IDA 0x460BCE–0x460C0F).
			uint32 exFlags = existing->getFlags();

			// No vertical overlap → always allow (IDA 0x460BD5: jl → 0x460C11).
			bool noVerticalOverlap = inRect.bottom < exRect.top;

			if (!noVerticalOverlap) {
				// Check each active ZSORT constraint; any violation blocks insertion.
				if ((exFlags & ZmbFeature::FLAG_40000000_ZSORT_LEFT) && inRect.left < exRect.left)
					continue; // ZSORT_LEFT violated
				if ((exFlags & ZmbFeature::FLAG_10000000_ZSORT_RIGHT) && inRect.right > exRect.right)
					continue; // ZSORT_RIGHT violated
				if ((exFlags & ZmbFeature::FLAG_20000000_ZSORT_BOTTOM) && inRect.top < exRect.top)
					continue; // ZSORT_BOTTOM violated
			}

			insertPos = i;
			found = true;
			break;
		}

		existingList.insert_at(insertPos, incoming);

		// After insertion, binary sets var_4 = the existing node we inserted
		// before (IDA 0x460C29). Next incoming starts scanning from there.
		// If appended at end (not found), scanPos stays where it was.
		if (found) {
			scanPos = insertPos + 1; // +1 because we inserted before it, shifting it right
		}
	}
}

/**
 * IDA: runner_zsortPartitionAndSort (0x4608AF)
 *
 * Build the final render list matching the binary's exact merge order:
 *   1. Partition into loopAnimList, overlayList, normalList, entityList
 *   2. Assemble combined = loopAnimList + overlayList (unsorted)
 *   3. Sort normalList → merge into combined (with ZSORT constraints)
 *   4. Sort entityList → merge into combined (with ZSORT constraints)
 *
 * Both merge passes use zsort_mergeEntityListIntoSortedList (0x460AD5),
 * which interleaves entries by sort key while respecting ZSORT protection
 * flags on existing entries. This means normal entries can be interleaved
 * with overlay entries (not kept strictly separate).
 */
void ZoombiniPage::buildSortedRenderList(Common::Array<ZmbFeature *> &outList) {
	Common::Array<ZmbFeature *> loopAnimList, normalList, entityList, overlayList;

	// Step 1: Categorize features into render buckets.
	// IDA check order: LOOP_ANIM → pre-existing OVERLAY → entity type → normalList.
	for (auto it = _scrbFeatureMap.begin(); it != _scrbFeatureMap.end(); it++)
		categorizeFeature(it->second, loopAnimList, normalList, entityList, overlayList);
	for (auto it = _subFeatureMap.begin(); it != _subFeatureMap.end(); it++)
		categorizeFeature(it->second, loopAnimList, normalList, entityList, overlayList);
	for (auto it = _virtualFeatureMap.begin(); it != _virtualFeatureMap.end(); it++)
		categorizeFeature(it->second, loopAnimList, normalList, entityList, overlayList);
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); it++)
		categorizeFeature(it->second, loopAnimList, normalList, entityList, overlayList);

	// Step 2: Assemble combined list = loopAnim + overlay (unsorted).
	// IDA 0x4609A1–0x4609BC: overlay entries appended after loopAnim entries.
	outList.clear();
	for (ZmbFeature *feature : loopAnimList)
		outList.push_back(feature);
	for (ZmbFeature *feature : overlayList)
		outList.push_back(feature);

	// Step 3: Sort normalList, merge into combined.
	// IDA 0x4609BE–0x4609D2: insertionSort(normalList) → merge(sorted, combined)
	insertionSortFeatures(normalList);
	mergeSortedListInto(outList, normalList);

	// Step 4: Sort entityList, merge into combined.
	// IDA 0x4609D7–0x4609EB: insertionSort(entityList) → merge(sorted, combined)
	insertionSortFeatures(entityList);
	mergeSortedListInto(outList, entityList);
}

void ZoombiniPage::buildSortedEventList(Common::Array<ZmbFeature *> &outList) {
	Common::Array<ZmbFeature *> loopAnimList, normalList, entityList, overlayList;

	for (auto it = _scrbFeatureMap.begin(); it != _scrbFeatureMap.end(); it++)
		categorizeFeature(it->second, loopAnimList, normalList, entityList, overlayList);
	for (auto it = _virtualFeatureMap.begin(); it != _virtualFeatureMap.end(); it++)
		categorizeFeature(it->second, loopAnimList, normalList, entityList, overlayList);

	outList.clear();
	for (ZmbFeature *feature : loopAnimList)
		outList.push_back(feature);
	for (ZmbFeature *feature : overlayList)
		outList.push_back(feature);

	insertionSortFeatures(normalList);
	mergeSortedListInto(outList, normalList);

	insertionSortFeatures(entityList);
	mergeSortedListInto(outList, entityList);
}

void ZoombiniPage::renderFeatures() {
	// IDA gfx_renderFrame (0x45F070) — two-pass architecture:
	//
	// Pass 1 (preRender): Animation logic for ALL features in registration order.
	//   Binary iterates zmb_pRunnerListHead calling pPreRenderFunc (0x45F2C6).
	//   Handles frame advancement, end-of-cycle, flag checks, sound dispatch.
	//   Runs BEFORE Z-sort so sort rects from custom preRender callbacks
	//   (e.g., scroll state machines) are up-to-date.
	//
	// Pass 2 (postRender): Shape blitting in Z-sorted order.
	//   Binary calls zsort_partitionAndSortFeatureRunners (0x45F2F1),
	//   blits background, then iterates calling pPostRenderFunc (0x45F35F).

	// Pass 1: Pre-render all features — animation logic
	for (auto it = _scrbFeatureMap.begin(); it != _scrbFeatureMap.end(); it++)
		it->second->onPreRender(this);
	for (auto it = _subFeatureMap.begin(); it != _subFeatureMap.end(); it++)
		it->second->onPreRender(this);
	for (auto it = _virtualFeatureMap.begin(); it != _virtualFeatureMap.end(); it++)
		it->second->onPreRender(this);
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); it++)
		it->second->onPreRender(this);

	// IDA gfx_renderFrame 0x45F352: copy background port → blitter port.
	// Binary blits persistent background into composite buffer before shapes.
	_vm->_gfx->copyBackToShapeScreen();

	// Z-sort: partition and sort feature runners
	Common::Array<ZmbFeature *> renderList;
	buildSortedRenderList(renderList);

	// Pass 2: Post-render — blit shapes in Z-sorted order
	for (ZmbFeature *feature : renderList)
		feature->onPostRender(this);

	_doForceRedraw = false;
}

void ZoombiniPage::checkCloseFeatures() {
	Common::StableMap<uint16, ZmbFeature *> *deleteMaps[2] = {
		&_scrbFeatureMap,
		&_virtualFeatureMap,
	};

	for (uint32 i = 0; i < ARRAYSIZE(deleteMaps); i++) {
		Common::StableMap<uint16, ZmbFeature *> *mapPtr = deleteMaps[i];

		Common::Array<uint16> deleteIds;
		for (auto it = mapPtr->begin(); it != mapPtr->end(); it++) {
			if (it->second->isCloseScheduled())
				deleteIds.push_back(it->second->getId());
		}
		for (uint16 deleteId : deleteIds)
			deregisterFeature(*mapPtr, deleteId);
	}

	// Detach sub-features: erase from _subFeatureMap but do NOT delete - the parent feature owns the pointer
	Common::Array<uint16> detachIds;
	for (auto it = _subFeatureMap.begin(); it != _subFeatureMap.end(); it++) {
		if (it->second->isDetachScheduled())
			detachIds.push_back(it->second->getId());
	}
	for (uint16 detachId : detachIds) {
		auto it = _subFeatureMap.find(detachId);
		if (it == _subFeatureMap.end())
			continue;
		ZmbFeature *subFeature = it->second;
		_subFeatureMap.erase(it);
		subFeature->clearDetach();
		subFeature->setSubFeatureRunning(false);
	}
}

/**
 * Pre-render pass for a single feature: animation logic.
 * IDA: runner_preRenderStandard (0x4619A1) — called for ALL features
 * BEFORE Z-sorting in gfx_renderFrame (0x45F070).
 *
 * Order of operations matches the binary:
 *   1. Frame selection (advance animation via incremental ++)
 *   2. End-of-cycle handling (CHAIN_SCRIPT, PLAY_ONCE)
 *      — end-of-cycle and frame advance are MUTUALLY EXCLUSIVE in the
 *      original.  Here, defaultSelectRenderFrame increments past _frameIdxMax
 *      to signal end-of-cycle, and the handler resets _lastFrameIdx to 0.
 *   3. Sound dispatch (using the final _lastFrameIdx after any reset)
 *   4. Per-frame flag checks (SKIP_RENDER, SKIP_ONCE)
 */
void ZoombiniPage::preRenderFeature(ZmbFeature *feature) {
	// IDA runner_preRenderStandard 0x4619BE: early return when wBoolDoRender=0.
	// Hotspot positions are NOT updated; _lastFrameIdx stays at last processed value.
	// postRenderStandard still draws for non-DEFER_RENDER features (see blitShapes gate).
	if (!feature->isRenderActivated())
		return;

	// 1. Frame selection — advance animation state
	// IDA 0x461D60–0x461D6F: frame advancement (wGroupFrameIdx++)
	// defaultSelectRenderFrame increments _lastFrameIdx.  When _lastFrameIdx
	// goes past _frameIdxMax, isEndOfAnimationCycle() returns true below.
	// It also sets _frameTimingReady (IDA: wBoolDoRender[0] local at 0x461B0C).
	int32 frameIdx = feature->onSelectRenderFrame(this);
	// Store result — custom selectRenderFrame hooks may not call setLastFrameIdx themselves.
	feature->setLastFrameIdx(frameIdx);

	// IDA 0x4619F5–0x461B12: timing gate.
	// In the original, wBoolDoRender[0] (local) is computed from the
	// dNextRenderFrame timing check, optionally modulated by the paired
	// hotspot slot system (wHotspotIdxToDraw / hotspot_renderPhaseArr).
	// Without paired slots, it reduces to dNextRenderFrame <= currentTime.
	// When timing is not ready, the original returns here — no end-of-cycle,
	// no dirty rect merge, no sound dispatch, no flag checks.
	// IDA 0x461BB7–0x461BF3: dirty rect merge (chGetDrawnRect = 1).
	// The original merges the previous frame's clickRect / wRectRgnB / RgnR
	// into the per-feature dirty region for partial screen refresh.  ScummVM
	// does full Z-sorted redraws, so the dirty rect merge is a no-op.
	if (!feature->isFrameTimingReady())
		return;

	if (feature->isAnimateActivated()) {
		// 2. End-of-cycle handling (IDA 0x461C05–0x461D06)
		// In the original, end-of-cycle fires when wGroupFrameIdx >= wScriptFrameCount.
		// With the incremental model, _lastFrameIdx was advanced past _frameIdxMax by
		// defaultSelectRenderFrame.  The last valid frame was displayed on the previous tick.
		bool didChainScript = false;

		if (feature->isEndOfAnimationCycle()) {

			// IDA 0x461C13: CHAIN_SCRIPT — swap SCRB data on the same feature
			if (feature->hasFlag(ZmbFeature::FLAG_00040000_CHAIN_SCRIPT)) {
				int16 chainedId = feature->getChainedScrbId();
				if (chainedId != 0) {
					feature->setChainedScrbId(0);
					if (chainedId >= 0) {
						loadScrbOntoFeature(feature, static_cast<uint16>(chainedId));
					} else {
						// IDA 0x461C47: negative = negate and set RANDOM_FRAME
						loadScrbOntoFeature(feature, static_cast<uint16>(-chainedId));
						feature->addFlag(ZmbFeature::FLAG_02000000_RANDOM_FRAME);
					}
					// IDA 0x461C6C: DRAW_ON_REG → disable render after chain
					if (feature->hasFlag(ZmbFeature::FLAG_00002000_DRAW_ON_REG))
						feature->deactivateRender();
				}
				// IDA 0x461C7F: if frame count < 2 → disable render
				if (feature->getMaxFrameIdx() < 1)
					feature->deactivateRender();
				// IDA 0x461C8A–0x461C93: wGroupFrameIdx=0, dwHotspotIdx=1
				feature->setLastFrameIdx(0);
				feature->setLastSoundedFrameIdx(-1);
				didChainScript = true;
			}

			// IDA 0x461CA3: PLAY_ONCE — stop rendering at end of cycle
			if (feature->hasFlag(ZmbFeature::FLAG_00100000_PLAY_ONCE)) {
				// IDA 0x461CDD: pFeatureRunner->core188.wBoolDoRender = 0
				feature->deactivateRender();
				// IDA 0x461CE9–0x461D06: callback fires and RETURNS EARLY
				// only if CHAIN_SCRIPT did NOT run (v5=1).
				// One-shot: onHotspotShapeOrFrameFunc cleared to 0 after firing.
				if (!didChainScript) {
					if (!feature->hasAnimEndCallbackFired()) {
						onFeatureAnimEvent(feature, -1);
						feature->markAnimEndCallbackFired();
					}
					return;
				}
			} else if (!didChainScript) {
				// IDA 0x461D0B: neither CHAIN_SCRIPT nor PLAY_ONCE → loop from beginning
				feature->setLastFrameIdx(0);
				feature->setLastSoundedFrameIdx(-1);
			}

			// Sub-feature detach (ScummVM-only: for sub-features running independently)
			if (feature->isSubFeatureRunning())
				feature->scheduleDetach();
		}

		// 3. Sound dispatch — fire sounds/events for newly reached frames
		// IDA 0x461EB6–0x461EDA: sound enqueue + event code dispatch
		// Re-read _lastFrameIdx: end-of-cycle may have reset it to 0.
		frameIdx = feature->getLastFrameIdx();
		if (feature->isAnimationCycleRunning()) {
			if (feature->getLastSoundedFrameIdx() < frameIdx) {
				feature->setLastSoundedFrameIdx(frameIdx);
				ZmbHotspotGroup *soundGroup = feature->getHotspotGroupExact(frameIdx);
				if (soundGroup) {
					if (soundGroup->hasAssignedSoundRes())
						_vm->_sound->playZmbSound(soundGroup->getAssignedSoundRes(), Audio::Mixer::kSFXSoundType);

					// Process SCRS event codes (0xFFxx frame terminators where xx != 0).
					// IDA: event code dispatch is gated by onHotspotShapeOrFrameFunc != null.
					// After the one-shot -1 fires and clears the pointer, subsequent event
					// codes stop dispatching.  _animEndCallbackFired models the null-pointer state.
					// Voice SFX (codes 200-239) use a separate sound table and are NOT gated.
					if (soundGroup->hasAssignedEventCode() && !feature->hasAnimEndCallbackFired()) {
						uint8 eventCode = soundGroup->getAssignedEventCode();
						uint8 adjustedCode = eventCode - 1;
						if (adjustedCode >= 200 && adjustedCode <= 239 && feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
							static const int16 kVoiceGroupMap[18] = {
								8, 6, 7, 10, 2, 12, 1, 9,
								0, 4, 5, 3, 11, 13, 14, 15, 16, 17};

							uint8 voiceIdx = adjustedCode - 200;
							int16 voiceGroup = (voiceIdx < 18) ? kVoiceGroupMap[voiceIdx] : 0;
							if (voiceGroup != 0) {
								const ZmbSnoid *snoid = static_cast<const ZmbSnoid *>(feature);
								int16 sndResId = snoid->getVoiceResId(voiceGroup);
								if (sndResId > 0) {
									debug(5, "ZmbSnoid: event code %u -> voice group %d -> SND %d",
										  eventCode, voiceGroup, sndResId);
									ZmbResource voiceRes(ZmbArchiveKind::kSystem, static_cast<uint16>(sndResId));
									_vm->_sound->playZmbSound(voiceRes, Audio::Mixer::kSFXSoundType);
								}
							}
						} else {
							debug(5, "ZmbSnoid: event code %u dispatched (adjusted %d)", eventCode, adjustedCode);
							onFeatureAnimEvent(feature, static_cast<int16>(adjustedCode));
						}
					}
				}
			}
		}

		// 3b. Deferred -1 callback for CHAIN_SCRIPT (IDA 0x461F51–0x461F67)
		// In the original, the -1 callback fires at the END of the hotspot frame walk
		// (LABEL_70), AFTER processing frame 0 event codes of the chained SCRB.
		// One-shot: onHotspotShapeOrFrameFunc is cleared to 0 after firing.
		if (didChainScript && !feature->hasAnimEndCallbackFired()) {
			onFeatureAnimEvent(feature, -1);
			feature->markAnimEndCallbackFired();
		}
	}

	// 4. Per-frame flag checks (IDA 0x461D73–0x461DAB)
	// These run AFTER end-of-cycle, BEFORE hotspot/shape processing.
	if (feature->hasFlag(ZmbFeature::FLAG_00020000_SKIP_RENDER))
		feature->deactivateRender();

	if (feature->hasFlag(ZmbFeature::FLAG_00010000_SKIP_ONCE)) {
		feature->removeFlag(ZmbFeature::FLAG_00010000_SKIP_ONCE);
		feature->setLastFrameIdx(0);
		feature->deactivateRender();
	}
}

/**
 * Post-render pass for a single feature: shape blitting only.
 * IDA: runner_postRenderStandard (0x46182F) — called in Z-sorted order
 * AFTER pre-render pass. Reads the frame index computed during preRender,
 * processes hotspot positions, blits shapes to screen, and updates sort rect.
 */
ZmbRenderResult ZoombiniPage::blitShapes(ZmbFeature *feature) {
	// IDA runner_postRenderStandard 0x461846:
	//   if (wBoolDoRender || (HIBYTE(bitmask) & 1) == 0) { draw }
	// = skip only when wBoolDoRender=0 AND FLAG_01000000_DEFER_RENDER is set.
	// Features without DEFER_RENDER always draw (their positions come from last preRender run).
	//
	// EXCEPTION: Snoids use a different callback in the original (onPostRender_ZoombiniAnimation_452ADD)
	// which only checks wBoolDoRender WITHOUT the bitmask check (IDA 0x452ae6):
	//   if ( featureRunner->core188.wBoolDoRender ) { ... render ... }
	// So for snoids, skip rendering when _isRenderActivated=false regardless of flags.
	if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
		if (!feature->isRenderActivated())
			return ZmbRenderResult::kSkipped;
	} else {
		if (!feature->isRenderActivated() && feature->hasFlag(ZmbFeature::FLAG_01000000_DEFER_RENDER))
			return ZmbRenderResult::kSkipped;
	}

	// Use frame index computed during preRender pass
	int32 frameIdx = feature->getLastFrameIdx();

	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	// Render sprites (Shape)
	ZmbHotspotGroup *hsGroup = feature->getHotspotGroup(frameIdx);
	if (!hsGroup)
		return ZmbRenderResult::kRendered;

	// Copy hotspots, because they need to be modifiable by preRenderShapeFunc
	Common::Array<ZmbHotspot> hotspots = hsGroup->copyHotspots();
	feature->onPreRenderShape(this, hsGroup, hotspots);

	// Draw shapes to screen
	feature->clearDrawRecords();
	Common::Rect sortRect;
	bool hasSortRect = false;

	// IDA snoidScript_renderFrame_4562B2: p_wScrsBaseShapeId layer shift.
	uint8 snoidLayerShift = 0;
	if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
		const ZmbSnoid *snoid = static_cast<const ZmbSnoid *>(feature);
		if (!snoid->hasCombinedShapeIndices() &&
		    snoid->getAnimState() == kSnoidAnimScriptNormal) {
			if (!hotspots.empty() && hotspots[0]._shapeIdx > 18)
				snoidLayerShift = 1;
		}
	}

	for (uint32 i = 0; i < hotspots.size(); i++) {
		ZmbHotspot &hs = hotspots[i];
		if (hs._shapeIdx == ZmbHotspot::kShapeNone)
			continue;

		if (feature->hasFlag(ZmbFeature::FLAG_00800000_POS_DELTA)) {
			const Common::Point &posDelta = feature->getPosDelta();
			hs._x += posDelta.x;
			hs._y += posDelta.y;
		}

		if (feature->hasFlag(ZmbFeature::FLAG_00000001_TYPE_SNOID)) {
			const ZmbSnoid *snoid = static_cast<const ZmbSnoid *>(feature);
			hs._x += snoid->getPointLoc().x;
			hs._y += snoid->getPointLoc().y;
			if (hs._shapeIdx > 0) {
				if (!snoid->hasCombinedShapeIndices()) {
					hs._shapeIdx += snoid->getBodyLayerBaseOffset(hs._hsId, snoidLayerShift);
				}
				if (hs._shapeIdx > 0) {
					if (snoid->isFacingLeft())
						hs._shapeIdx = 2 * hs._shapeIdx;
					else
						hs._shapeIdx = 2 * hs._shapeIdx - 1;

					ZmbRegs *activeRegs = snoid->_useSmallShapeRegs ? _vm->_smallSnoidShapeRegs : _vm->_snoidShapeRegs;
					if (activeRegs) {
						const Common::Point delta = activeRegs->getShapeDelta(hs._shapeIdx);
						hs._x -= delta.x;
						hs._y -= delta.y;
					}
				}
			}
			// Binary validation (IDA 0x45662E): snoidScript_renderFrame checks
			// *pRMapLayerData >= shapeid before processing each layer. If the
			// final shape ID exceeds the resource's shape count, skip this slot.
			// This happens with small snoids (resource 3200) whose walk frame
			// shapes combined with trait bases can exceed the 568-entry limit.
			if (hs._shapeIdx > 0 &&
			    static_cast<uint32>(hs._shapeIdx) > _vm->_gfx->getShapeCount(feature->getResource())) {
				continue;
			}
		}

		bool clearBeforeRender = false;
		const Common::Rect &drawnRect = _vm->_gfx->drawShape(screenKind, feature->getResource(), &hs, clearBeforeRender);

		feature->setDrawRecord(hsGroup, hs, drawnRect);

		if (hasSortRect) {
			sortRect.extend(drawnRect);
		} else {
			sortRect = drawnRect;
			hasSortRect = true;
		}
	}
	if (hasSortRect) {
		feature->setSortRect(sortRect);
		// IDA runner_preRenderStandard 0x4620F5–0x462172: clickRect is set from the first frame's
		// hotspot bounding box (hotspot pos + REGS shape width/height). In ScummVM, we derive the
		// same bounding box from the actual drawn rect, which is equivalent. Only set once — the
		// first time a feature renders, its sort rect becomes the permanent clickRect.
		if (!feature->hasClickRect())
			feature->setClickRect(sortRect);
	}

	return ZmbRenderResult::kRendered;
}

int32 ZoombiniPage::selectRenderFrame(ZmbFeature *feature) {
	return feature->defaultSelectRenderFrame(_currentFrameCounter);
}

int32 ZoombiniPage::selectScrsRenderFrame(ZmbFeature *feature) {
	// For SCRS script playback, return the frame index set by onSnoidAnimTick.
	// Unlike defaultSelectRenderFrame, this does NOT overwrite _lastFrameIdx
	// with time-based cycling — the tick function drives frame advancement.
	return feature->getLastFrameIdx();
}

Common::Rect ZoombiniPage::renderStoredSnoid(ZoombiniGraphics::ScreenKind screenKind, const ZmbTrait &trait, const Common::Point &pos) {
	// Mirrors IDA: zmbRunner_setAnimShape_456785(sub-kind=0) + snoidScript_renderFrame_4562B2
	// as used in onPostRenderVirtualSCRB_storage_tBMP2000_41265F.
	//
	// Build the same 5-hotspot idle pose that setupIdleHotspots() would produce,
	// then apply the same blitShapes() transform: posLoc offset, right-facing
	// mirror (2*idx-1), and REGS registration-point correction.
	static const uint16 kFootTable[6] = {0, 191, 246, 335, 360, 411};
	static const uint16 kNoseTable[6] = {0, 171, 175, 179, 183, 187};
	static const uint16 kEyeTable[6] = {0, 91, 107, 123, 139, 155};
	static const uint16 kHeadTable[6] = {0, 11, 27, 43, 59, 75};
	static const uint16 kRawShapeIdle = 2;

	uint8 foot = CLIP<uint8>(trait._foot, 1, 5);
	uint8 nose = CLIP<uint8>(trait._nose, 1, 5);
	uint8 eye = CLIP<uint8>(trait._eye, 1, 5);
	uint8 head = CLIP<uint8>(trait._head, 1, 5);

	// Combined shape indices (same as setupIdleHotspots): traitOffset + kRawShapeIdle
	const uint16 combined[5] = {
		(uint16)(kFootTable[foot] + kRawShapeIdle), // slot 0: foot
		(uint16)(0 + kRawShapeIdle),                // slot 1: body anchor
		(uint16)(kNoseTable[nose] + kRawShapeIdle), // slot 2: nose
		(uint16)(kEyeTable[eye] + kRawShapeIdle),   // slot 3: eye
		(uint16)(kHeadTable[head] + kRawShapeIdle), // slot 4: head
	};

	ZmbResource snoidRes(ZmbArchiveKind::kSystem, 3000);
	Common::Rect sortRect;
	bool hasSortRect = false;

	for (int layer = 0; layer < 5; layer++) {
		uint16 shapeIdx = combined[layer];
		if (shapeIdx == ZmbHotspot::kShapeNone)
			continue;

		int16 sx = pos.x;
		int16 sy = pos.y;

		// Right-facing snoid: 2*shapeIdx - 1 (combinedIdx already includes trait offset)
		uint16 finalShape = 2 * shapeIdx - 1;

		// Apply REGS registration-point correction
		if (_vm->_snoidShapeRegs) {
			const Common::Point delta = _vm->_snoidShapeRegs->getShapeDelta(finalShape);
			sx -= (int16)delta.x;
			sy -= (int16)delta.y;
		}

		ZmbHotspot hs(layer, finalShape, 0, sx, sy);
		const Common::Rect &drawnRect = _vm->_gfx->drawShape(screenKind, snoidRes, &hs, false);

		if (hasSortRect)
			sortRect.extend(drawnRect);
		else {
			sortRect = drawnRect;
			hasSortRect = true;
		}
	}

	return sortRect;
}

void ZoombiniPage::clear() {
	clearSubFeatures();
	clearScrbFeatures();
	clearMainFeatureHeads();
	clearVirtualFeatures();
	clearSnoids();
	clearRegs();
	clearNode();
	clearTerrainBitmap();
}

void ZoombiniPage::clearScrbFeatures() {
	for (auto it = _scrbFeatureMap.begin(); it != _scrbFeatureMap.end(); it++) {
		ZmbFeature *feature = it->second;
		delete feature;
	}
	_scrbFeatureMap.clear();
}

void ZoombiniPage::clearMainFeatureHeads() {
	for (uint i = 0; i < _mainFeatureHeads.size(); i++) {
		delete _mainFeatureHeads[i];
	}
	_mainFeatureHeads.clear();
}

void ZoombiniPage::clearSubFeatures() {
	// Sub-features are owned by their parent features - only clear the map, do NOT delete.
	for (auto it = _subFeatureMap.begin(); it != _subFeatureMap.end(); it++) {
		it->second->setSubFeatureRunning(false);
		it->second->clearDetach();
	}
	_subFeatureMap.clear();
}

void ZoombiniPage::clearVirtualFeatures() {
	for (auto it = _virtualFeatureMap.begin(); it != _virtualFeatureMap.end(); it++) {
		ZmbFeature *feature = it->second;
		delete feature;
	}
	_virtualFeatureMap.clear();
}

void ZoombiniPage::clearSnoids() {
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); it++) {
		ZmbSnoid *snoid = it->second;
		delete snoid;
	}
	_snoidMap.clear();
}

ZmbSnoid *ZoombiniPage::loadSnoid(ZmbResource imgResource, uint16 scrsId, uint32 flags, const ZmbFeature::EventHooks &eventHooks) {
	return loadSnoid(imgResource, scrsId, Common::Point(0, 0), flags, eventHooks);
}

ZmbSnoid *ZoombiniPage::loadSnoid(ZmbResource imgResource, uint16 scrsId, const Common::Point &point, uint32 flags, const ZmbFeature::EventHooks &eventHooks) {
	auto it = _snoidMap.find(scrsId);
	if (it != _snoidMap.end()) {
		error("Duplicated snoid id %u", scrsId);
		return nullptr;
	}

	ZmbSnoid *snoid = new ZmbSnoid(_vm, scrsId, flags);
	_snoidMap[scrsId] = snoid;

	snoid->setPointLoc(point);
	snoid->setResource(imgResource);

	Common::SeekableReadStream *scrsStream = _vm->getResource(ID_SCRS, ZmbResource(imgResource._archiveKind, scrsId));
	snoid->parseScrsStream(scrsStream);
	scrsStream = nullptr;

	snoid->initValues();
	snoid->setEventHooks(eventHooks);

	return snoid;
}

ZmbSnoid *ZoombiniPage::loadSnoidFromPack(uint16 snoidId, const Common::Point &point, uint32 flags, const ZmbFeature::EventHooks &eventHooks) {
	auto it = _snoidMap.find(snoidId);
	if (it != _snoidMap.end()) {
		error("Duplicated snoid id %u", snoidId);
		return nullptr;
	}

	ZmbSnoid *snoid = new ZmbSnoid(_vm, snoidId, flags);
	_snoidMap[snoidId] = snoid;

	snoid->setPointLoc(point);
	// Seed the z-sort rect so the very first frame sorts correctly by position.
	// blitShapes() will overwrite this with the actual bounding box each frame.
	snoid->setSortRect(Common::Rect(point.x, point.y, point.x + 1, point.y + 1));
	// Pack snoids use the global system shapes (ZOOMBINI.MHK tBMP 3000).
	// Hotspot data is built programmatically from traits via setupIdleHotspots().
	snoid->setResource(ZmbResource(ZmbArchiveKind::kSystem, 3000));

	// No SCRS resource parsing — traits/name are set by the caller from pack data

	snoid->initValues();
	snoid->setEventHooks(eventHooks);

	return snoid;
}

ZmbSnoid *ZoombiniPage::loadSnoidFromScrb(ZmbResource imgResource, uint16 snoidId, uint16 scrbId, const Common::Point &point, uint32 flags, const ZmbFeature::EventHooks &eventHooks) {
	auto it = _snoidMap.find(snoidId);
	if (it != _snoidMap.end()) {
		error("Duplicated snoid id %u", snoidId);
		return nullptr;
	}

	ZmbSnoid *snoid = new ZmbSnoid(_vm, snoidId, flags);
	_snoidMap[snoidId] = snoid;

	snoid->setPointLoc(point);
	snoid->setSortRect(Common::Rect(point.x, point.y, point.x + 1, point.y + 1));
	snoid->setResource(imgResource);

	// Inhabitants use SCRB (not SCRS); parseStream() handles the SCRB format.
	Common::SeekableReadStream *scrbStream = _vm->getResource(ID_SCRB, ZmbResource(imgResource._archiveKind, scrbId));
	snoid->parseStream(scrbStream);
	scrbStream = nullptr;

	snoid->initValues();
	snoid->setEventHooks(eventHooks);

	return snoid;
}

void ZoombiniPage::unloadSnoid(uint16 scrsId) {
	auto it = _snoidMap.find(scrsId);
	if (it == _snoidMap.end())
		return;

	ZmbSnoid *snoid = it->second;
	_snoidMap.erase(it);
	delete snoid;
}

ZmbSnoid *ZoombiniPage::getSnoid(uint16 scrsId) const {
	auto it = _snoidMap.find(scrsId);
	if (it == _snoidMap.end())
		return nullptr;
	return it->second;
}

bool ZoombiniPage::isPointOccupiedByOtherSnoid(const ZmbSnoid *self, const Common::Point &pt, int32 distSquared) const {
	// IDA: zmbRunner_setPosAndIdx_mesaureDistance_456ACA — only considers snoids in
	// states 0 (idle), 3 (flip), or 6 (fidget); all three are stationary.
	// Threshold is a direct squared distance (original: 500); comparison is strict <.
	for (auto it = _snoidMap.begin(); it != _snoidMap.end(); ++it) {
		const ZmbSnoid *other = it->second;
		if (other == self)
			continue;
		// Only check stationary snoids (idle / flip / fidget)
		SnoidAnimState st = other->getAnimState();
		if (st != kSnoidAnimIdle && st != kSnoidAnimFlip && st != kSnoidAnimFidget)
			continue;
		const Common::Point &opos = other->getPointLoc();
		int32 dx = opos.x - pt.x;
		int32 dy = opos.y - pt.y;
		if (dx * dx + dy * dy < distSquared)
			return true;
	}
	return false;
}

void ZoombiniPage::beginSnoidDrag(ZmbSnoid *snoid) {
	// IDA: beginDragFeatureRunner_45360F 0x4536BD–0x4536CD:
	// Save original bitmask, then OR 0x4001000 (TOPMOST + OVERLAY).
	// TOPMOST (0x1000) makes the z-sort merge treat this snoid as a barrier,
	// keeping it at the tail of the render list so it draws on top of everything.
	// OVERLAY (0x4000000) routes it into the overlay render bucket.
	_dragSavedSnoidFlags = snoid->getFlags();
	snoid->addFlag(ZmbFeature::FLAG_00001000_TOPMOST);
	snoid->addFlag(ZmbFeature::FLAG_04000000_OVERLAY);

	snoid->setAnimState(kSnoidAnimDrag);
	_vm->_cursor->hideCursor();
}

void ZoombiniPage::endSnoidDrag(ZmbSnoid *snoid) {
	// IDA: beginDragFeatureRunner_45360F 0x453CCF:
	// Restore original bitmask (removes TOPMOST + OVERLAY added during drag).
	// Use removeFlag for the two drag-specific flags rather than full restore,
	// preserving any flags legitimately modified during drag by other code.
	snoid->removeFlag(ZmbFeature::FLAG_00001000_TOPMOST);
	snoid->removeFlag(ZmbFeature::FLAG_04000000_OVERLAY);

	_vm->_cursor->showCursor();
}

void ZoombiniPage::loadTerrainBitmap(uint16 resId) {
	clearTerrainBitmap();

	// IDA: rmap_loadTerrainArchive (0x46001A) — loads tBMP "Terrain" resource.
	// The bitmap is 160x120 (screen / 4), 8bpp. Pixel value 1 = walkable.
	// The surface is cached by GraphicsManager; we just store a pointer.
	_terrainBitmap = _vm->_gfx->findImage(ZmbResource(ZmbArchiveKind::kPage, resId));
	if (_terrainBitmap) {
		const Graphics::Surface *surface = _terrainBitmap->getSurface();
		debug(2, "Loaded terrain barrier bitmap: %dx%d (resId %u)",
			surface->w, surface->h, resId);
	}
}

bool ZoombiniPage::isTerrainWalkable(int16 x, int16 y) const {
	if (!_terrainBitmap)
		return false; // No terrain loaded = position invalid (IDA: returns 0)

	const Graphics::Surface *surface = _terrainBitmap->getSurface();
	int16 terrainX = x / 4;
	int16 terrainY = y / 4;

	// Clamp to bitmap bounds (IDA: terrain_validateAndPlaceSnoid 0x453D28)
	terrainX = CLIP<int16>(terrainX, 0, surface->w - 1);
	terrainY = CLIP<int16>(terrainY, 0, surface->h - 1);

	const byte *pixels = (const byte *)surface->getBasePtr(terrainX, terrainY);
	return *pixels == 1;
}

bool ZoombiniPage::validateTerrainDrop(ZmbSnoid *snoid) {
	// IDA: terrain_validateAndPlaceSnoid (0x453D28)
	// 1. Check terrain walkability at snoid position
	// 2. If walkable, check collision with idle snoids (threshold=36)
	// 3. If colliding, find non-colliding position
	// Returns true if drop position is valid (on walkable terrain).

	static const int32 kTerrainCollisionThreshold = 36; // ~6px radius

	const Common::Point pos = snoid->getPointLoc();

	if (!isTerrainWalkable(pos.x, pos.y))
		return false;

	// Terrain is walkable — check collision with idle snoids
	if (isPointOccupiedByOtherSnoid(snoid, pos, kTerrainCollisionThreshold)) {
		// Collision found — find a non-colliding position
		Common::Point adjusted = findNonCollidingPosition(snoid, pos, kTerrainCollisionThreshold);
		snoid->setPointLoc(adjusted);
	}

	return true;
}

Common::Point ZoombiniPage::findNonCollidingPosition(const ZmbSnoid *self, const Common::Point &origin, int32 distSquared) const {
	// IDA: snoid_findNonCollidingPos (0x456C95)
	// When called from terrain_validateAndPlaceSnoid: threshold=36, randSeed=0, gridParam=NULL.
	// Scans a 5x4 grid pattern (20 iterations). Each candidate:
	//   x = origin.x + 4 * random(-5, 5),  y = origin.y
	// Keeps first non-colliding position. If all collide, returns last candidate.

	Common::Point best = origin;

	// 5 columns x 4 rows = 20 attempts (IDA: v13=1..5, v12=1..4)
	for (int row = 1; row <= 4; row++) {
		for (int col = 1; col <= 5; col++) {
			// Random x offset in [-20, +20] in steps of 4 (IDA: 4 * nextRand(5, -5))
			int16 randOffset = static_cast<int16>(4 * (_vm->_rnd->getRandomNumber(10) - 5));
			Common::Point candidate(origin.x + randOffset, origin.y);

			// Clamp to screen bounds
			candidate.x = CLIP<int16>(candidate.x, 0, 640);
			candidate.y = CLIP<int16>(candidate.y, 0, 480);

			if (!isPointOccupiedByOtherSnoid(self, candidate, distSquared))
				return candidate;

			best = candidate;
		}
	}

	// Grid exhausted — return last candidate
	return best;
}

void ZoombiniPage::clearTerrainBitmap() {
	// Not owned by us — cached by GraphicsManager, freed on archive clear.
	_terrainBitmap = nullptr;
}

void ZoombiniPage::loadWalkAnims() {
	if (_walkAnimsLoaded)
		return;
	_walkAnimsLoaded = true;

	// SCRS resource IDs: 100 + dirBucket + 5*footType
	//   footType 1–5, dirBucket 0–4  →  SCRS 105–129 (in ZOOMBINI.MHK = kSystem)
	for (int ft = 1; ft <= 5; ft++) {
		for (int dir = 0; dir < 5; dir++) {
			uint16 scrsId = static_cast<uint16>(100 + dir + 5 * ft);
			ZmbWalkAnim &anim = _walkAnims[ft - 1][dir];

			Common::SeekableReadStream *stream = _vm->getResource(
				ID_SCRS, ZmbResource(ZmbArchiveKind::kSystem, scrsId));
			if (!stream)
				continue;

			anim.frameCount = stream->readUint16BE();
			anim.variant = static_cast<uint8>(stream->readUint16BE());

			for (int f = 0; f < anim.frameCount; f++) {
				ZmbWalkFrame frame = {};
				int idx = 0;
				while (!stream->eos()) {
					int16 shapeid = stream->readSint16BE();
					if (shapeid < 0) {
						// Sentinel: end of frame
						// shapeid < -256: also consumes an attached sound resource id
						if (shapeid < -256)
							stream->readSint16BE();
						break;
					}
					int16 x = stream->readSint16BE();
					int16 y = stream->readSint16BE();
					if (idx < 5) {
						frame.shape[idx] = shapeid;
						frame.x[idx] = x;
						frame.y[idx] = y;
					}
					idx++;
				}
				frame.entryCount = static_cast<uint8>(MIN(idx, 5));
				anim.frames.push_back(frame);
			}

			delete stream;
		}
	}
}

const ZmbWalkAnim &ZoombiniPage::getWalkAnim(uint8 footType, int dirBucket) {
	loadWalkAnims();
	int ft = CLIP<int>(static_cast<int>(footType), 1, 5) - 1;
	int dir = CLIP<int>(dirBucket, 0, 4);
	return _walkAnims[ft][dir];
}

void ZoombiniPage::loadFidgetAnims() {
	if (_fidgetAnimsLoaded)
		return;
	_fidgetAnimsLoaded = true;

	// Fidget SCRS IDs (from sub_452455 loading zmbAnimHotspotArr[i] = SCRS(100+i)):
	//   Set A (chZmbAnimShapeCommonImageIdx=1): indices 30-36 → SCRS 130-136
	//   Set B (chZmbAnimShapeCommonImageIdx=2): indices 38-44 → SCRS 138-144
	// Same binary format as walk SCRS (parsed by snoidScript_renderFrame_4562B2).
	for (int setIdx = 0; setIdx < 2; setIdx++) {
		for (int variant = 0; variant < 7; variant++) {
			uint16 scrsId = static_cast<uint16>((setIdx == 0) ? 130 + variant : 138 + variant);
			ZmbWalkAnim &anim = _fidgetAnims[setIdx][variant];

			Common::SeekableReadStream *stream = _vm->getResource(
				ID_SCRS, ZmbResource(ZmbArchiveKind::kSystem, scrsId));
			if (!stream)
				continue;

			anim.frameCount = stream->readUint16BE();
			anim.variant = static_cast<uint8>(stream->readUint16BE());

			for (int f = 0; f < anim.frameCount; f++) {
				ZmbWalkFrame frame = {};
				int idx = 0;
				while (!stream->eos()) {
					int16 shapeid = stream->readSint16BE();
					if (shapeid < 0) {
						if (shapeid < -256)
							stream->readSint16BE();
						break;
					}
					int16 x = stream->readSint16BE();
					int16 y = stream->readSint16BE();
					if (idx < 5) {
						frame.shape[idx] = shapeid;
						frame.x[idx] = x;
						frame.y[idx] = y;
					}
					idx++;
				}
				frame.entryCount = static_cast<uint8>(MIN(idx, 5));
				anim.frames.push_back(frame);
			}

			delete stream;
		}
	}
}

const ZmbWalkAnim &ZoombiniPage::getFidgetAnim(int fidgetSet, int variant) {
	loadFidgetAnims();
	int set = CLIP<int>(fidgetSet, 0, 1);
	int var = CLIP<int>(variant, 0, 6);
	return _fidgetAnims[set][var];
}

void ZoombiniPage::loadHoldingAnims() {
	if (_holdingAnimsLoaded)
		return;
	_holdingAnimsLoaded = true;

	// Holding (drag) SCRS IDs (from IDA animateZoombini_455E76 case 5):
	//   wAnimHotspotSetIdx = pZmb->footTrait + 45
	//   Foot type 1 → index 46 → SCRS 146
	//   Foot type 2 → index 47 → SCRS 147
	//   ... through foot type 5 → index 50 → SCRS 150
	// Same binary format as walk/fidget SCRS.
	for (int footIdx = 0; footIdx < 5; footIdx++) {
		uint16 scrsId = static_cast<uint16>(146 + footIdx);
		ZmbWalkAnim &anim = _holdingAnims[footIdx];

		Common::SeekableReadStream *stream = _vm->getResource(
			ID_SCRS, ZmbResource(ZmbArchiveKind::kSystem, scrsId));
		if (!stream)
			continue;

		anim.frameCount = stream->readUint16BE();
		anim.variant = static_cast<uint8>(stream->readUint16BE());

		for (int f = 0; f < anim.frameCount; f++) {
			ZmbWalkFrame frame = {};
			int idx = 0;
			while (!stream->eos()) {
				int16 shapeid = stream->readSint16BE();
				if (shapeid < 0) {
					if (shapeid < -256)
						stream->readSint16BE();
					break;
				}
				int16 x = stream->readSint16BE();
				int16 y = stream->readSint16BE();
				if (idx < 5) {
					frame.shape[idx] = shapeid;
					frame.x[idx] = x;
					frame.y[idx] = y;
				}
				idx++;
			}
			frame.entryCount = static_cast<uint8>(MIN(idx, 5));
			anim.frames.push_back(frame);
		}

		delete stream;
	}
}

const ZmbWalkAnim &ZoombiniPage::getHoldingAnim(uint8 footType) {
	loadHoldingAnims();
	int ft = CLIP<int>(static_cast<int>(footType), 1, 5) - 1;
	return _holdingAnims[ft];
}

void ZoombiniPage::clearRegs() {
	for (auto it = _regsMap.begin(); it != _regsMap.end(); it++) {
		ZmbRegs *regs = it->_value;
		delete regs;
	}
	_regsMap.clear();
}

void ZoombiniPage::clearNode() {
	for (auto it = _nodeMap.begin(); it != _nodeMap.end(); it++) {
		ZmbNode *node = it->_value;
		delete node;
	}
	_nodeMap.clear();
}

void ZoombiniPage::onFadeIn() {
	if (!_useFadeEffect)
		return;

	_vm->_gfx->queueFadeEffect(ZoombiniGraphics::kFadeIn, 500);
	_vm->_gfx->startMouseCursorEyeAnimation(_currentFrameTime);
}

void ZoombiniPage::onFadeOut() {
	if (!_useFadeEffect)
		return;

	_vm->_gfx->queueFadeEffect(ZoombiniGraphics::kFadeOut, 500);
}

void ZoombiniPage::genericButton_selectShapes(ZmbFeature *feature, Common::Array<ZmbHotspot> &hotspots, Common::StableMap<uint32, ButtonState> &buttonStateMap, uint16 pressedDeltaX, uint16 pressedDeltaY) {
	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		ButtonState &bs = it->second;

		if (!bs._drawEnabled)
			continue;

		if (bs.hasDisabledState() && bs._isPressDisabled)
			hotspots[bs._hsNormalId]._shapeIdx = bs._shapeDisabledIdx;

		ZmbHotspot &hsNormal = hotspots[bs._hsNormalId];
		ZmbHotspot &hsPressed = hotspots[bs._hsPressedId];

		bool disableNormalHotspot = false;
		if (bs.isAnimating()) {
			uint32 elapsedFrames = _currentFrameCounter - bs._animationStartFrame;
			if (elapsedFrames < bs._animationFrameCount) {
				disableNormalHotspot = (elapsedFrames < bs._animationFrameCount - 1);
			} else {
				bs._animationStartFrame = 0;
				bs._firePostAnimationEvent = true;
			}
		}

		if (disableNormalHotspot) {
			hsNormal._shapeIdx = ZmbHotspot::kShapeNone;
			hsPressed._x += pressedDeltaX;
			hsPressed._y += pressedDeltaY;
		} else {
			hsPressed._shapeIdx = ZmbHotspot::kShapeNone;
		}
	}
}

void ZoombiniPage::genericButton_textRender(ZmbFeature *feature, Common::StableMap<uint32, ButtonState> &buttonStateMap, Graphics::TextAlign textAlign, int16 normalDeltaY, int16 pressedDeltaY) {
	ZoombiniGraphics::TextConf tc;
	tc._hAlign = textAlign;
	tc._vAlign = textAlign;
	genericButton_textRender(feature, buttonStateMap, tc, normalDeltaY, pressedDeltaY);
}

void ZoombiniPage::genericButton_textRender(ZmbFeature *feature, Common::StableMap<uint32, ButtonState> &buttonStateMap, const ZoombiniGraphics::TextConf &tc, int16 normalDeltaY, int16 pressedDeltaY) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		ButtonState &bs = it->second;

		if (!bs._drawEnabled)
			continue;

		ZmbDrawRecord *record = feature->findDrawRecordByHotspotIdx(bs._hsNormalId, bs._hsPressedId);
		if (record == nullptr)
			continue;
		Common::Rect textRect = record->_drawnRect;

		// Apply text offset when button is pressed
		if (bs.isAnimating()) {
			textRect.top += pressedDeltaY;
			textRect.bottom += pressedDeltaY;
		} else {
			textRect.top += normalDeltaY;
			textRect.bottom += normalDeltaY;
		}

		if (bs._textKey != ZoombiniText::kNone)
			_vm->_gfx->drawText(screenKind, bs._textKey, textRect, tc);
	}
}

void ZoombiniPage::genericButton_textRender(ZmbFeature *feature, Common::StableMap<uint32, ButtonState> &buttonStateMap, ButtonGetRectFunc textRectFunc, const ZoombiniGraphics::TextConf &tc) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		ButtonState &bs = it->second;

		if (!bs._drawEnabled)
			continue;

		ZmbDrawRecord *record = feature->findDrawRecordByHotspotIdx(bs._hsNormalId, bs._hsPressedId);
		if (record == nullptr)
			continue;
		Common::Rect textRect = (this->*textRectFunc)(feature, it->first, bs, record->_drawnRect);

		if (bs._textKey != ZoombiniText::kNone)
			_vm->_gfx->drawText(screenKind, bs._textKey, textRect, tc);
	}
}

void ZoombiniPage::genericButton_action(ZmbFeature *feature, Common::StableMap<uint32, ButtonState> &buttonStateMap, OnButtonActionFunc onPostAnimationFunc) {
	// [Post-Animation Events]
	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		uint32 bsIdx = it->first;
		ButtonState &bs = it->second;

		if (!bs._drawEnabled)
			continue;

		if (bs.hasDisabledState() && bs._isPressDisabled)
			continue;

		if (!bs._firePostAnimationEvent)
			continue;
		bs._firePostAnimationEvent = false;

		(this->*onPostAnimationFunc)(feature, bsIdx, bs);
	}
}

ZmbEventHandleResult ZoombiniPage::genericButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, ButtonState> &buttonStateMap, OnButtonActionFunc onButtonActionFunc) {
	ZmbDrawRecord *drawRecord = feature->findDrawRecordAtPoint(absPos);
	if (!drawRecord)
		return ZmbEventHandleResult::kPassthrough;

	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		uint32 bsIdx = it->first;
		ButtonState &bs = it->second;

		if (!bs._drawEnabled)
			continue;

		// Find the pressed button
		if (drawRecord->_hs._hsId != bs._hsNormalId && drawRecord->_hs._hsId != bs._hsPressedId)
			continue;

		if (bs.hasDisabledState() && bs._isPressDisabled) {
			// Zoombini has some buttons that can be clicked even when they are disabled,
			// but they won't trigger the button press animation and will directly trigger the action event.
			// Ex) Go button on the PICKER page, when not enough zoombinis are selected
			if (onButtonActionFunc != nullptr)
				(this->*onButtonActionFunc)(feature, bsIdx, bs);
			return ZmbEventHandleResult::kConsumed;
		}

		bs.press(_vm, _currentFrameCounter);
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniPage::genericButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, ButtonState> &buttonStateMap, const Common::HashMap<uint32, Common::Rect> &buttonRectMap, OnButtonActionFunc onButtonActionFunc) {
	// onLButtonDown with button rects instead of drawn rects
	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		uint32 bsIdx = it->first;
		ButtonState &bs = it->second;

		if (!bs._drawEnabled)
			continue;

		auto rit = buttonRectMap.find(bsIdx);
		if (rit == buttonRectMap.end())
			continue;
		const Common::Rect &buttonRect = rit->_value;
		if (!buttonRect.contains(absPos))
			continue;

		if (bs.hasDisabledState() && bs._isPressDisabled) {
			// Zoombini has some buttons that can be clicked even when they are disabled,
			// but they won't trigger the button press animation and will directly trigger the action event.
			// Ex) Go button on the PICKER page, when not enough zoombinis are selected
			if (onButtonActionFunc != nullptr)
				(this->*onButtonActionFunc)(feature, bsIdx, bs);
			return ZmbEventHandleResult::kConsumed;
		}

		bs.press(_vm, _currentFrameCounter);
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniPage::genericButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, ButtonState> &buttonStateMap, ButtonGetRectFunc getRectFunc, OnButtonActionFunc onButtonActionFunc) {
	// onLButtonDown with button rects instead of drawn rects
	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		uint32 bsIdx = it->first;
		ButtonState &bs = it->second;

		if (!bs._drawEnabled)
			continue;

		ZmbDrawRecord *record = feature->findDrawRecordByHotspotIdx(bs._hsNormalId, bs._hsPressedId);

		Common::Rect buttonRect = (this->*getRectFunc)(feature, bsIdx, bs, record->_drawnRect);
		if (!buttonRect.contains(absPos))
			continue;

		if (bs.hasDisabledState() && bs._isPressDisabled) {
			// Zoombini has some buttons that can be clicked even when they are disabled,
			// but they won't trigger the button press animation and will directly trigger the action event.
			// Ex) Go button on the PICKER page, when not enough zoombinis are selected
			if (onButtonActionFunc != nullptr)
				(this->*onButtonActionFunc)(feature, bsIdx, bs);
			return ZmbEventHandleResult::kConsumed;
		}

		bs.press(_vm, _currentFrameCounter);
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

void ZoombiniPage::genericToggleButton_selectShapes(ZmbFeature *feature, Common::Array<ZmbHotspot> &hotspots, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, uint16 pressedDeltaX, uint16 pressedDeltaY) {
	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		ToggleButtonState &bs = it->second;

		if (!bs._enabled)
			continue;

		ZmbHotspot &hsNormal = hotspots[bs._hsNormalId];
		ZmbHotspot &hsPressed = hotspots[bs._hsPressedId];
		if (!bs._toggleState) {
			hsNormal._shapeIdx = bs._offNormalShapeIdx;
			hsPressed._shapeIdx = bs._offPressedShapeIdx;
		}

		bool disableNormalHotspot = false;
		if (bs.isAnimating()) {
			uint32 elapsedFrames = _currentFrameCounter - bs._animationStartFrame;
			if (elapsedFrames < bs._animationFrameCount) {
				disableNormalHotspot = (elapsedFrames < bs._animationFrameCount - 1);
			} else {
				bs._animationStartFrame = 0;
				bs._firePostAnimationEvent = true;
			}
		}

		if (disableNormalHotspot) {
			hsNormal._shapeIdx = ZmbHotspot::kShapeNone;
			hsPressed._x += pressedDeltaX;
			hsPressed._y += pressedDeltaY;
		} else {
			hsPressed._shapeIdx = ZmbHotspot::kShapeNone;
		}
	}
}

void ZoombiniPage::genericToggleButton_textRender(ZmbFeature *feature, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, Graphics::TextAlign textAlign) {
	ZoombiniGraphics::TextConf tc;
	tc._hAlign = textAlign;
	tc._vAlign = textAlign;
	genericToggleButton_textRender(feature, buttonStateMap, tc);
}

void ZoombiniPage::genericToggleButton_textRender(ZmbFeature *feature, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, const ZoombiniGraphics::TextConf &tc) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		ToggleButtonState &bs = it->second;

		if (!bs._enabled)
			continue;

		ZmbDrawRecord *record = feature->findDrawRecordByHotspotIdx(bs._hsNormalId, bs._hsPressedId);
		if (record == nullptr)
			continue;

		if (bs._textKey != ZoombiniText::kNone)
			_vm->_gfx->drawText(screenKind, bs._textKey, record->_drawnRect, tc);
	}
}

void ZoombiniPage::genericToggleButton_textRender(ZmbFeature *feature, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, ToggleButtonGetRectFunc textRectFunc, const ZoombiniGraphics::TextConf &tc) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		ToggleButtonState &bs = it->second;

		if (!bs._enabled)
			continue;

		ZmbDrawRecord *record = feature->findDrawRecordByHotspotIdx(bs._hsNormalId, bs._hsPressedId);
		if (record == nullptr)
			continue;
		Common::Rect textRect = (this->*textRectFunc)(feature, it->first, bs, record->_drawnRect);

		if (bs._textKey != ZoombiniText::kNone)
			_vm->_gfx->drawText(screenKind, bs._textKey, textRect, tc);
	}
}

void ZoombiniPage::genericToggleButton_postAnimation(ZmbFeature *feature, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, OnToggleButtonPostAnimationFunc onPostAnimationFunc) {
	// [Post-Animation Events]
	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		ToggleButtonState &bs = it->second;

		if (!bs._enabled)
			continue;

		if (!bs._firePostAnimationEvent)
			continue;
		bs._firePostAnimationEvent = false;

		(this->*onPostAnimationFunc)(feature, it->first, bs);
	}
}

ZmbEventHandleResult ZoombiniPage::genericToggleButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap) {
	ZmbDrawRecord *drawRecord = feature->findDrawRecordAtPoint(absPos);
	if (!drawRecord)
		return ZmbEventHandleResult::kPassthrough;

	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		ToggleButtonState &bs = it->second;

		if (!bs._enabled)
			continue;

		// Find the pressed button
		if (drawRecord->_hs._hsId != bs._hsNormalId && drawRecord->_hs._hsId != bs._hsPressedId)
			continue;

		bs.press(_vm, _currentFrameCounter);
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniPage::genericToggleButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, const Common::HashMap<uint32, Common::Rect> &buttonRectMap) {
	// onLButtonDown with button rects instead of drawn rects
	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		uint32 bsIdx = it->first;
		ToggleButtonState &bs = it->second;

		if (!bs._enabled)
			continue;

		auto rit = buttonRectMap.find(bsIdx);
		if (rit == buttonRectMap.end())
			continue;
		const Common::Rect &buttonRect = rit->_value;
		if (!buttonRect.contains(absPos))
			continue;

		bs.press(_vm, _currentFrameCounter);
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

ZmbEventHandleResult ZoombiniPage::genericToggleButton_onLButtonDown(ZmbFeature *feature, const Common::Point &absPos, Common::StableMap<uint32, ToggleButtonState> &buttonStateMap, ToggleButtonGetRectFunc getRectFunc) {
	// onLButtonDown with button rects instead of drawn rects
	for (auto it = buttonStateMap.begin(); it != buttonStateMap.end(); it++) {
		uint32 bsIdx = it->first;
		ToggleButtonState &bs = it->second;

		if (!bs._enabled)
			continue;

		ZmbDrawRecord *record = feature->findDrawRecordByHotspotIdx(bs._hsNormalId, bs._hsPressedId);
		if (record == nullptr)
			continue;

		Common::Rect buttonRect = (this->*getRectFunc)(feature, bsIdx, bs, record->_drawnRect);
		if (!buttonRect.contains(absPos))
			continue;

		bs.press(_vm, _currentFrameCounter);
		return ZmbEventHandleResult::kConsumed;
	}

	return ZmbEventHandleResult::kPassthrough;
}

void ZoombiniPage::ButtonState::press(MohawkEngine_Zoombini *vm, uint32 frameCounter) {
	animate(frameCounter);
	if (_pressSoundId.hasId())
		vm->_sound->playZmbSound(_pressSoundId, Audio::Mixer::kSFXSoundType, false);
}

void ZoombiniPage::ToggleButtonState::press(MohawkEngine_Zoombini *vm, uint32 frameCounter) {
	animate(frameCounter);
	if (_pressSoundId.hasId())
		vm->_sound->playZmbSound(_pressSoundId, Audio::Mixer::kSFXSoundType, false);
}

} // End of namespace Mohawk
