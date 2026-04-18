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

#include "mohawk/resource.h"

#include "common/events.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_page.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_scripts.h"
#include "mohawk/zoombini_sound.h"

namespace Mohawk {

void ZmbNode::parseStream(Common::SeekableReadStream *stream) {
	uint16 numPoints = stream->readUint16BE();
	assert(0 < numPoints);

	uint16 idx = 0;
	for (idx = 0; idx < numPoints && !stream->eos(); idx++) {
		int16 coordX = stream->readSint16BE();
		int16 coordY = stream->readSint16BE();
		_waypoints.push_back(Common::Point(coordX, coordY));
	}
	assert(idx == numPoints);

	delete stream;
}

void ZmbNode::parsePathStream(Common::SeekableReadStream *stream) {
	// PATH resource: uint16BE path count, then M × 24 bytes (1-indexed waypoint refs, 0=empty).
	// IDA: dword_4A48A0 points to this block; snoidPath_initRoute_454CA9 reads path[j][k] as
	// *(char*)(dword_4A48A0 + 24*j + k + 2) (the +2 skips the count header).
	uint16 pathCount = stream->readUint16BE();
	_paths.resize(pathCount);
	for (uint16 j = 0; j < pathCount; j++) {
		_paths[j].resize(24);
		for (int k = 0; k < 24; k++)
			_paths[j][k] = stream->readByte();
	}
	delete stream;
}

Common::Array<int16> ZmbRegs::parseStream(Common::SeekableReadStream *stream) {
	Common::Array<int16> coord;

	while (!stream->eos()) {
		int16 delta = stream->readSint16BE();
		coord.push_back(delta);
	}

	delete stream;
	return coord;
}

void ZmbRegs::parseStreams(MohawkEngine_Zoombini *vm, ZmbArchiveKind archiveKind, uint16 resIdX, uint16 resIdY) {
	// Opening two Common::SeekableReadStream for the same resource will disrupt each other,
	// so we need to read the two streams separately and combine them in memory.
	const Common::Array<int16> &coordsX = parseStream(vm->getResource(ID_REGS, ZmbResource(archiveKind, resIdX)));
	const Common::Array<int16> &coordsY = parseStream(vm->getResource(ID_REGS, ZmbResource(archiveKind, resIdY)));

	assert(coordsX.size() == coordsY.size());

	for (uint32 i = 0; i < coordsX.size(); i++) {
		_offsets.push_back(Common::Point(coordsX[i], coordsY[i]));
	}
}

Common::Point ZmbRegs::getSubImageDelta(uint16 subImage) const {
	// subImage is 0-based index, and offsets array is 1-based
	if (_offsets.size() <= subImage + 1u)
		return Common::Point(0, 0);
	return _offsets[subImage + 1];
}

Common::Point ZmbRegs::getShapeDelta(uint16 shapeIdx) const {
	// shapeIdx is 1-based index, and offsets array is also 1-based
	if (_offsets.size() <= shapeIdx)
		return Common::Point(0, 0);
	return _offsets[shapeIdx];
}

Common::Point ZmbRegs::getHotspotDelta(const ZmbHotspot &hotspot) const {
	return getShapeDelta(hotspot._shapeIdx);
}

ZmbHotspotGroup::~ZmbHotspotGroup() {
	clear();
}

ZmbHotspot &ZmbHotspotGroup::getHotspot(uint32 hsId) {
	return _hotspots[hsId];
}

ZmbHotspot &ZmbHotspotGroup::operator[](uint32 hsId) {
	return _hotspots[hsId];
}

void ZmbHotspotGroup::appendHotspot(const ZmbHotspot &hs) {
	_hotspots.push_back(hs);
}

void ZmbHotspotGroup::setHotspots(const Common::Array<ZmbHotspot> &hotspots) {
	_hotspots = hotspots;
}

void ZmbHotspotGroup::clear() {
	_hotspots.clear();
}

static ZmbResource resolveSoundId(int16 soundResId) {
	if (1000 <= soundResId && soundResId < 20000)
		return ZmbResource(ZmbArchiveKind::kPage, (uint16)soundResId);
	return ZmbResource(ZmbArchiveKind::kSystem, (uint16)soundResId);
}
ZmbFeature::ZmbFeature(MohawkEngine_Zoombini *vm, uint16 scrbId, uint32 frameInterval, uint32 flags, ZmbResource imgResource) : _vm(vm), _id(scrbId), _frameInterval(frameInterval), _flags(flags), _imgResource(imgResource) {
}

ZmbFeature::ZmbFeature(MohawkEngine_Zoombini *vm, uint16 scrbId, uint32 frameInterval, const Common::Point &pointRef, uint32 flags, ZmbResource imgResource) : _vm(vm), _id(scrbId), _frameInterval(frameInterval), _pointLoc(pointRef), _flags(flags), _imgResource(imgResource) {
}

ZmbFeature::ZmbFeature(MohawkEngine_Zoombini *vm, uint16 scrbId, uint32 flags, ZmbResource imgResource) : _vm(vm), _id(scrbId), _flags(flags), _imgResource(imgResource) {
}

ZmbFeature::ZmbFeature(MohawkEngine_Zoombini *vm, uint16 scrbId, uint32 flags) : _vm(vm), _id(scrbId), _flags(flags) {
}

ZmbFeature::~ZmbFeature() {
	clear();
}

void ZmbFeature::initValues() {
	// Handle flags
	if (hasFlag(ZmbFeature::FLAG_00008000_LOOP_ANIM) && 0 < _frameIdxMax) {
		activateRender();
		activateAnimate();
	}

	if (hasFlag(ZmbFeature::FLAG_02000000_RANDOM_FRAME) && 0 < _frameIdxMax) {
		activateRender();
		activateAnimate();
	}

	// IDA: PLAY_ONCE is NOT checked in the initial-load path of
	// runner_preRenderStandard. It only fires at end-of-animation-cycle (0x461CA3).
	// The feature renders normally from the start; animation advancement is controlled
	// by other flags (LOOP_ANIM activates it, DEFER_ANIM defers it).
	// Must NOT deactivateAnimate here — that would break the common
	// LOOP_ANIM | PLAY_ONCE combo (bridge, xfer, rodmap, basecamp2).
	if (hasFlag(ZmbFeature::FLAG_00100000_PLAY_ONCE)) {
		activateRender();
	}

	if (hasFlag(ZmbFeature::FLAG_00020000_SKIP_RENDER)) {
		deactivateRender();
	}

	// IDA runner_preRenderStandard 0x461B56: DEFER_ANIM suppresses
	// wBoolDoRender on initial load. Feature starts invisible (dormant) until
	// game code triggers it via activateRender()+activateAnimate().
	if (hasFlag(ZmbFeature::FLAG_00080000_DEFER_ANIM)) {
		deactivateRender();
		deactivateAnimate();
	}

	if (hasFlag(ZmbFeature::FLAG_01000000_DEFER_RENDER)) {
		deactivateRender();
	}

	// Set guessing fields
	if (!_hsFrameMap.empty()) {
		_hasClickRect = 0;
		_skipFirstAdvance = 1;
		if (hasFlag(ZmbFeature::FLAG_00800000_POS_DELTA)) {
			// IDA loadSCRB (0x460384) at 0x4604DC:
			//   if (POS_DELTA set) pos2 = hsArr[0].pos;
			// Only pos2 (_pointRef) is updated to the SCRB's first hotspot position.
			// posLoc (_pointLoc) retains the layout position from the constructor
			// (set in runner_registerAndAllocate from corePosUnion).
			// Delta = posLoc - pos2 offsets all hotspots so the first one appears
			// at the constructor-provided layout position.
			assert(getHotspotGroup(0) != nullptr);
			assert(0 < getHotspotGroup(0)->getHotspotCount());
			_pointRef = getHotspotGroup(0)->getHotspot(0).getPos();
		}
	}
}

void ZmbFeature::setEventHooks(const EventHooks &hooks) {
	_eventHooks = hooks;
	if (!_eventHooks._renderFunc)
		_eventHooks._renderFunc = &ZoombiniPage::blitShapes;
	if (!_eventHooks._selectRenderFrameFunc)
		_eventHooks._selectRenderFrameFunc = &ZoombiniPage::selectRenderFrame;
}

void ZmbFeature::onPreRender(ZoombiniPage *page) {
	if (!page)
		return;

	// IDA: runner_preRenderStandard (0x4619A1) — pre-render pass.
	// Calls custom preRender callback (e.g., scroll SM, state machine tick),
	// then runs standard animation logic via page->preRenderFeature().
	if (_eventHooks._preRenderFunc) {
		if ((page->*(_eventHooks._preRenderFunc))(this) == false)
			return;
	}

	page->preRenderFeature(this);
}

ZmbRenderResult ZmbFeature::onPostRender(ZoombiniPage *page) {
	if (!page)
		return ZmbRenderResult::kSkipped;

	// IDA: runner_postRenderStandard (0x46182F) — post-render pass.
	// Blits shapes to screen, then calls custom postRender callback.
	ZmbRenderResult ret = (page->*(_eventHooks._renderFunc))(this);
	if (ret == ZmbRenderResult::kRendered && _eventHooks._postRenderFunc)
		(page->*(_eventHooks._postRenderFunc))(this);
	return ret;
}

int32 ZmbFeature::onSelectRenderFrame(ZoombiniPage *page) {
	if (!_eventHooks._selectRenderFrameFunc || !page)
		return 0;
	return (page->*(_eventHooks._selectRenderFrameFunc))(this);
}

void ZmbFeature::onPreRenderShape(ZoombiniPage *page, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots) {
	if (!_eventHooks._preRenderShapeFunc || !page)
		return;
	(page->*(_eventHooks._preRenderShapeFunc))(this, hsGroup, hotspots);
}

ZmbEventHandleResult ZmbFeature::onMouseMove(ZoombiniPage *page, const Common::Point &absPos, const Common::Point &relPos) {
	if (!_eventHooks._mouseMoveFunc || !page)
		return ZmbEventHandleResult::kPassthrough;
	return (page->*(_eventHooks._mouseMoveFunc))(this, absPos, relPos);
}

ZmbEventHandleResult ZmbFeature::onLButtonDown(ZoombiniPage *page, const Common::Point &absPos, const Common::Point &relPos) {
	if (!_eventHooks._lButtonDownFunc || !page)
		return ZmbEventHandleResult::kPassthrough;
	return (page->*(_eventHooks._lButtonDownFunc))(this, absPos, relPos);
}

ZmbEventHandleResult ZmbFeature::onLButtonUp(ZoombiniPage *page, const Common::Point &absPos, const Common::Point &relPos) {
	if (!_eventHooks._lButtonUpFunc || !page)
		return ZmbEventHandleResult::kPassthrough;
	return (page->*(_eventHooks._lButtonUpFunc))(this, absPos, relPos);
}

ZmbEventHandleResult ZmbFeature::onKeyDown(ZoombiniPage *page, const Common::KeyState &kbd, bool kbdRepeat) {
	if (!_eventHooks._keyDownFunc || !page)
		return ZmbEventHandleResult::kPassthrough;
	return (page->*(_eventHooks._keyDownFunc))(this, kbd, kbdRepeat);
}

ZmbEventHandleResult ZmbFeature::onKeyUp(ZoombiniPage *page, const Common::KeyState &kbd, bool kbdRepeat) {
	if (!_eventHooks._keyUpFunc || !page)
		return ZmbEventHandleResult::kPassthrough;
	return (page->*(_eventHooks._keyUpFunc))(this, kbd, kbdRepeat);
}

ZmbEventHandleResult ZmbFeature::onWheelUp(ZoombiniPage *page, const Common::Point &absPos) {
	if (!_eventHooks._wheelUpFunc || !page)
		return ZmbEventHandleResult::kPassthrough;
	return (page->*(_eventHooks._wheelUpFunc))(this, absPos);
}

ZmbEventHandleResult ZmbFeature::onWheelDown(ZoombiniPage *page, const Common::Point &absPos) {
	if (!_eventHooks._wheelDownFunc || !page)
		return ZmbEventHandleResult::kPassthrough;
	return (page->*(_eventHooks._wheelDownFunc))(this, absPos);
}

void ZmbFeature::parseStream(Common::SeekableReadStream *stream) {
	if (!stream) {
		error("ZmbScrb: Invalid stream");
	}

	clear();

	// Check scrb frame count: how many FF00 should appear?
	uint16 scrbFrameCount = stream->readUint16BE();
	if (scrbFrameCount < 0x0001)
		error("ZmbScrb: Invalid SCRB Frame Count(%u)", scrbFrameCount);

	parseFrames(stream, scrbFrameCount);

	delete stream;
}

void ZmbFeature::parseFrames(Common::SeekableReadStream *stream, uint16 frameCount) {
	// Each Hotspot entry: [ID_UINT16] [X_UINT16] [Y_UINT16] ... [FF 00]
	uint32 totalCount = 0;
	for (int32 frameIdx = 0; frameIdx < static_cast<int32>(frameCount); frameIdx++) {
		ZmbHotspotGroup *hsGroup = new ZmbHotspotGroup(_id, frameIdx);
		_hsFrameMap[frameIdx] = hsGroup;
		// Every declared frame counts toward `_frameIdxMax` - matches IDA's
		// `wScriptFrameCount = pScrsData->wGroupCount` semantics. Trailing
		// terminator-only frames (e.g. Ferry SCRS 1900 frames 11-24 where
		// frame 24's `ff03` carries the case-2 chain event) MUST be reached
		// by the animation cycle; without this, `isEndOfAnimationCycle` and
		// the snoid SCRS state machine would stop early at the last frame
		// with shape data and the chain event would never dispatch.
		_frameIdxMax = MAX(_frameIdxMax, frameIdx);

		for (uint16 idx = 0; !stream->eos(); idx++) {
			int16 shapeid = stream->readSint16BE();
			if (shapeid < 0) {
				// 0xFF00: end of frame
				// 0xFFxx: end of frame, with event code in low byte
				// 0xFExx: end of frame, with extra int16 (sound resource id)
				if (shapeid < -256) {
					int16 soundResId = stream->readSint16BE();
					hsGroup->assignSoundRes(resolveSoundId(soundResId));
				} else if (shapeid > -256) {
					hsGroup->assignEventCode(static_cast<uint8>(shapeid & 0xFF));
				}
				break;
			}

			int16 x = stream->readSint16BE();
			int16 y = stream->readSint16BE();
			ZmbHotspot hs = ZmbHotspot(idx, shapeid, frameIdx, x, y);
			hsGroup->appendHotspot(hs);
			// `_lastShapeFrameIdx` tracks the highest frame that contains a
			// positive-shape hotspot. PLAY_ONCE freezes here so the captain /
			// boat / etc. settles on a frame that actually has visible shapes
			// rather than a trailing terminator-only frame (which would render
			// either the previous-frame fallback at a stale anchor or nothing
			// at all, producing the "captain stretched/distorted" freeze the
			// reject-flight controller SCRBs leave behind).
			if (shapeid > 0)
				_lastShapeFrameIdx = MAX(_lastShapeFrameIdx, frameIdx);
			totalCount += 1;
		}
	}
}

void ZmbFeature::setVirtualHotspots(const Common::Array<ZmbHotspot> &hotspots) {
	clear();

	ZmbHotspotGroup *hsGroup = new ZmbHotspotGroup(_id, 0);
	_hsFrameMap[0] = hsGroup;
	_frameIdxMax = 0;
	_lastShapeFrameIdx = 0;
	_lastFrameIdx = 0;
	_isAnimateActivated = false;

	hsGroup->setHotspots(hotspots);

	debug(1, "ZmbScrb: Set virtual feature with %u hotspots", hotspots.size());
}

void ZmbFeature::loadScrbData(Common::SeekableReadStream *stream, bool scheduleRender) {
	// IDA: scrb_loadOnRunner (0x460384) — swap SCRB data on an existing runner.
	// Clears existing hotspot data & draw records, parses new SCRB stream,
	// resets animation state, and re-runs initValues().
	// Preserves: identity (_id), flags, callbacks, _pointRef (immutable position).

	// Increment generation counter so the PLAY_ONCE handler in
	// preRenderFeature() can detect that a new SCRB was loaded during
	// the -1 callback and avoid a stale markAnimEndCallbackFired().
	_scrbLoadGeneration++;

	// Clear existing hotspot data and draw records (like the runner being reloaded)
	clearDrawRecords();
	for (auto it = _hsFrameMap.begin(); it != _hsFrameMap.end(); it++)
		delete it->_value;
	_hsFrameMap.clear();

	// Reset animation state (IDA: wGroupFrameIdx=0, dwHotspotIdx=1, dNextRenderFrame=0)
	_lastFrameIdx = 0;
	_frameIdxMax = 0;
	_lastShapeFrameIdx = 0;
	_lastSoundedFrameIdx = -1;
	_nextRenderFrame = 0;
	_frameTimingReady = true;

	// IDA: bHasClickRect=0, _skipFirstAdvance=1
	_hasClickRect = false;
	_skipFirstAdvance = true;

	// Parse new SCRB data
	parseStream(stream);

	// Re-initialize flag-dependent state
	initValues();

	// IDA: wBoolDoRender = wBoolScheduleRender
	if (scheduleRender) {
		activateRender();
		activateAnimate();
	}
}

ZmbHotspotGroup *ZmbFeature::getHotspotGroupExact(int32 frameId) const {
	if (frameId < 0)
		return nullptr;
	auto it = _hsFrameMap.find(frameId);
	if (it == _hsFrameMap.end())
		return nullptr;
	return it->_value;
}

ZmbHotspotGroup *ZmbFeature::getHotspotGroup(int32 frameId) {
	if (frameId < 0)
		return nullptr;

	ZmbHotspotGroup *hsGroup = nullptr;

	Common::HashMap<int32, ZmbHotspotGroup *>::iterator it = _hsFrameMap.find(frameId);
	if (it != _hsFrameMap.end())
		hsGroup = it->_value;

	if (!hsGroup)
		return nullptr;

	if (hsGroup->getHotspotCount() == 0 && 0 < frameId) {
		// No hotspots for the frame - Fall back to the highest-indexed frame that has at least one hotspot.
		for (int32 altFrameId = frameId - 1; 0 <= altFrameId && hsGroup->getHotspotCount() == 0; altFrameId--) {
			it = _hsFrameMap.find(altFrameId);
			assert(it != _hsFrameMap.end());
			hsGroup = it->_value;
		}
	}

	return hsGroup;
}

uint32 ZmbFeature::getHotspotTotalCount() const {
	uint32 count = 0;
	Common::HashMap<int32, ZmbHotspotGroup *>::iterator it = _hsFrameMap.begin();
	for (; it != _hsFrameMap.end(); it++) {
		ZmbHotspotGroup *hsGroup = it->_value;
		count += hsGroup->getHotspotCount();
	}
	return count;
}

uint16 ZmbFeature::getHotspotIdCount() const {
	return _hsFrameMap.size();
}

int32 ZmbFeature::defaultSelectRenderFrame(uint32 currentFrameCounter) {
	// IDA runner_preRenderStandard 0x461B0C + 0x461D24–D6F:
	// Timing gate: dNextRenderFrame <= scrb_dwFrameRenderTime.  In the original
	// this is a local (wBoolDoRender[0]) that gates the entire preRender body.
	// Without the paired hotspot slot system (wHotspotIdxToDraw / renderPhaseArr),
	// it reduces to the dNextRenderFrame check.  _frameTimingReady propagates
	// the gate result to preRenderFeature().

	// RANDOM_FRAME: pick a random frame index each qualifying tick.
	// IDA 0x461D24: nextRand(wScriptFrameCount) + scrb_seekToFrameGroup
	if (hasFlag(ZmbFeature::FLAG_02000000_RANDOM_FRAME)) {
		_frameTimingReady = (_nextRenderFrame <= currentFrameCounter);
		if (_frameTimingReady) {
			_nextRenderFrame = currentFrameCounter + _frameInterval;
			_lastFrameIdx = _vm->_rnd->getRandomNumber(_frameIdxMax);
		}
		return _lastFrameIdx;
	}

	// DEFER_ANIM: dormant until externally activated via activateAnimate().
	if (hasFlag(ZmbFeature::FLAG_00080000_DEFER_ANIM) && !_isAnimateActivated)
		return 0;

	if (isAnimateActivated()) {
		// Timing gate: IDA dNextRenderFrame <= scrb_dwFrameRenderTime
		_frameTimingReady = (_nextRenderFrame <= currentFrameCounter);
		if (_frameTimingReady) {
			_nextRenderFrame = currentFrameCounter + _frameInterval;

			// IDA 0x461C05–D6F: end-of-cycle check and frame advance are
			// mutually exclusive.  _skipFirstAdvance only applies in the advance branch
			// (when _lastFrameIdx < _frameIdxMax).  When at/past max, always
			// increment to signal end-of-cycle to preRenderFeature.
			if (_lastFrameIdx < _frameIdxMax) {
				// IDA 0x461D60: _skipFirstAdvance — skip first frame advance after SCRB load
				if (_skipFirstAdvance) {
					_skipFirstAdvance = false;
				} else {
					// IDA 0x461D66: ++wGroupFrameIdx
					_lastFrameIdx++;
				}
			} else {
				// At or past max: advance past _frameIdxMax to trigger end-of-cycle
				// detection in preRenderFeature.
				_lastFrameIdx++;
			}
		}
	}
	return _lastFrameIdx;
}

Common::Point ZmbFeature::getPosDelta() const {
	if (!hasFlag(ZmbFeature::FLAG_00800000_POS_DELTA))
		return Common::Point(0, 0);
	return _pointLoc - _pointRef;
}

ZmbDrawRecord *ZmbFeature::setDrawRecord(ZmbHotspotGroup *hsGroup, const ZmbHotspot &hs, const Common::Rect &drawnRect) {
	ZmbDrawRecord *record = new ZmbDrawRecord(this, hsGroup, hs, drawnRect);
	uint32 h = hs.hash();
	auto existing = _drawnRecordMap.find(h);
	if (existing != _drawnRecordMap.end())
		delete existing->second;
	_drawnRecordMap[h] = record;
	return record;
}

ZmbDrawRecord *ZmbFeature::getDrawRecord(uint16 frame, uint16 hsIdx) {
	uint32 hash = ZmbHotspot::hash(frame, hsIdx);
	auto it = _drawnRecordMap.find(hash);
	if (it == _drawnRecordMap.end())
		return nullptr;

	return it->second;
}

void ZmbFeature::eraseDrawRecord(uint16 frame, uint16 hsIdx) {
	uint32 hash = ZmbHotspot::hash(frame, hsIdx);
	auto it = _drawnRecordMap.find(hash);
	if (it == _drawnRecordMap.end())
		return;

	ZmbDrawRecord *record = it->second;
	_drawnRecordMap.erase(it);
	delete record;
}

void ZmbFeature::clearDrawRecords() {
	for (auto it = _drawnRecordMap.begin(); it != _drawnRecordMap.end(); it++) {
		delete it->second;
	}
	_drawnRecordMap.clear();
}

ZmbDrawRecord *ZmbFeature::findDrawRecordAtPoint(const Common::Point &absPos) {
	for (auto it = _drawnRecordMap.begin(); it != _drawnRecordMap.end(); it++) {
		ZmbDrawRecord *record = it->second;
		if (record->_drawnRect.contains(absPos))
			return record;
	}
	return nullptr;
}

void ZmbFeature::findDrawRecordsAtPoint(const Common::Point &absPos, Common::Array<ZmbDrawRecord *> &foundRecords) {
	for (auto it = _drawnRecordMap.begin(); it != _drawnRecordMap.end(); it++) {
		ZmbDrawRecord *record = it->second;
		if (record->_drawnRect.contains(absPos))
			foundRecords.push_back(record);
	}
}

ZmbDrawRecord *ZmbFeature::findDrawRecordByHotspotIdx(uint16 hsIdx) {
	for (auto it = _drawnRecordMap.begin(); it != _drawnRecordMap.end(); it++) {
		ZmbDrawRecord *record = it->second;
		if (record->_hs._hsId == hsIdx)
			return record;
	}
	return nullptr;
}

ZmbDrawRecord *ZmbFeature::findDrawRecordByHotspotIdx(uint16 hsIdx1, uint16 hsIdx2) {
	for (auto it = _drawnRecordMap.begin(); it != _drawnRecordMap.end(); it++) {
		ZmbDrawRecord *record = it->second;
		if (record->_hs._hsId == hsIdx1 || record->_hs._hsId == hsIdx2)
			return record;
	}
	return nullptr;
}

ZmbDrawRecord *ZmbFeature::findDrawRecordByHotspotIdx(Common::Array<uint16> hsIdxArr) {
	for (auto it = _drawnRecordMap.begin(); it != _drawnRecordMap.end(); it++) {
		ZmbDrawRecord *record = it->second;

		for (uint32 i = 0; i < hsIdxArr.size(); i++) {
			if (record->_hs._hsId == hsIdxArr[i])
				return record;
		}
	}
	return nullptr;
}

ZmbDrawRecord *ZmbFeature::findDrawRecordByShapeId(uint16 shapeId) {
	for (auto it = _drawnRecordMap.begin(); it != _drawnRecordMap.end(); it++) {
		ZmbDrawRecord *record = it->second;
		if (record->_hs._shapeIdx == shapeId)
			return record;
	}
	return nullptr;
}

ZmbDrawRecord *ZmbFeature::findDrawRecordByShapeId(uint16 shapeId1, uint16 shapeId2) {
	for (auto it = _drawnRecordMap.begin(); it != _drawnRecordMap.end(); it++) {
		ZmbDrawRecord *record = it->second;
		if (record->_hs._shapeIdx == shapeId1 || record->_hs._shapeIdx == shapeId2)
			return record;
	}
	return nullptr;
}

ZmbDrawRecord *ZmbFeature::findDrawRecordByShapeId(Common::Array<uint16> shapeIdArr) {
	for (auto it = _drawnRecordMap.begin(); it != _drawnRecordMap.end(); it++) {
		ZmbDrawRecord *record = it->second;

		for (uint32 i = 0; i < shapeIdArr.size(); i++) {
			if (record->_hs._shapeIdx == shapeIdArr[i])
				return record;
		}
	}
	return nullptr;
}

void ZmbFeature::activateSubFeature() {
	;
}

void ZmbFeature::clear() {
	clearDrawRecords();

	if (_refSubFeature) {
		delete _refSubFeature;
		_refSubFeature = nullptr;
	}

	for (auto it = _hsFrameMap.begin(); it != _hsFrameMap.end(); it++) {
		delete it->_value;
	}
	_hsFrameMap.clear();
}

void ZmbFeature::activateAnimate() {
	_isAnimateActivated = true;
	_animEndCallbackFired = false;
	_frameTimingReady = true;
	_lastSoundedFrameIdx = -1;
	scheduleAnimateForFrames(_frameIdxMax);
}

void ZmbFeature::deactivateAnimate() {
	_isAnimateActivated = false;
}

void ZmbFeature::setSelectRenderFrameFunc(OnSelectRenderFrameFunc func) {
	_eventHooks._selectRenderFrameFunc = func;
}

bool ZmbFeature::isAnimateActivated() const {
	return _isAnimateActivated;
}

void ZmbFeature::scheduleAnimateForFrames(uint16 frames) {
	// IDA: wGroupFrameIdx = 0 after SCRB load — start at first frame.
	_lastFrameIdx = 0;
	_frameIdxMax = frames;
}

bool ZmbFeature::isAnimationCycleRunning() const {
	if (!isAnimateActivated())
		return false;
	if (hasFlag(ZmbFeature::FLAG_00008000_LOOP_ANIM))
		return true;
	return _lastFrameIdx <= _frameIdxMax;
}

bool ZmbFeature::isEndOfAnimationCycle() const {
	if (!isAnimateActivated())
		return false;
	// IDA runner_preRenderStandard 0x461C03: end-of-cycle fires when
	// wGroupFrameIdx >= wScriptFrameCount.  With the incremental model,
	// defaultSelectRenderFrame advances _lastFrameIdx past _frameIdxMax
	// to signal end-of-cycle.  The last valid frame (== _frameIdxMax) is
	// displayed on the previous tick; end-of-cycle fires one tick later.
	return _lastFrameIdx > _frameIdxMax;
}

void ZmbFeature::runSubFeature(ZoombiniPage *page) {
	assert(hasFlag(ZmbFeature::FLAG_00040000_CHAIN_SCRIPT));

	if (!_refSubFeature)
		return;

	// Do nothing if the sub-feature is already running in the page's feature list
	if (_refSubFeature->isSubFeatureRunning())
		return;

	// Reset and start the sub-feature's animation from the beginning
	_refSubFeature->activateRender();
	_refSubFeature->activateAnimate();

	// Register it into the page's active scrb feature map for independent rendering.
	// Ownership stays with this parent feature; the page will detach (not delete) it
	// once the animation cycle ends.
	_refSubFeature->setSubFeatureRunning(true);
	page->attachSubFeature(_refSubFeature);
}

ZmbSnoid::ZmbSnoid(MohawkEngine_Zoombini *vm, uint16 snoidId, uint32 flags) : _vm(vm), _id(snoidId), ZmbFeature(vm, snoidId, flags) {
	assert(hasFlag(FLAG_00000001_TYPE_SNOID));
}

ZmbSnoid::~ZmbSnoid() {
}

void ZmbSnoid::parseScrsStream(Common::SeekableReadStream *stream) {
	if (!stream) {
		error("ZmbScrs: Invalid stream");
	}

	clear();

	uint16 scrsFrameCount = stream->readUint16BE();
	if (scrsFrameCount < 0x0001)
		error("ZmbScrs: Invalid SCRS Frame Count(%u)", scrsFrameCount);

	// SCRS has an extra uint16BE variant field after frame count
	_variant = stream->readUint16BE();

	parseFrames(stream, scrsFrameCount);

	delete stream;
}

void ZmbSnoid::startScrsPlayback(Common::SeekableReadStream *scrsStream, bool hideOnComplete,
                                  bool rejectState, const Common::Point *initPos) {
	// Save original position for restoration when SCRS finishes
	_scrsOrigPointLoc = getPointLoc();

	// Parse SCRS data (calls clear() which destroys any prior idle/SCRS hotspot data)
	parseScrsStream(scrsStream);

	// After parsing, shape indices are raw SCRS values; getBodyLayerBaseOffset is needed
	_usesVirtualHotspots = false;

	// IDA snoidScript_initAndPlay_455C0D anchor selection:
	//   pInitPos == NULL:  anchor = first frame's first positive-shape hotspot.
	//                      pos2 = (firstAnchor - posLoc), so frame 0 renders AT posLoc.
	//   pInitPos != NULL:  scan groups from END (iFrameScanIdx = -1, -2, ...) for the
	//                      LAST group with a positive-shape hotspot. Use that anchor and
	//                      *pInitPos as the reference. pos2 = (lastAnchor - *pInitPos),
	//                      so the SCRS animation ENDS at *pInitPos (used by Ferry reject-
	//                      flight landing SCRS at IDA 0x41B6F9 with &dword_4AB124).
	// During SCRS rendering, position = SCRS_xy + (-pos2) - REGS.
	ZmbHotspotGroup *anchorFrame = nullptr;
	Common::Point anchorRefPoint = _scrsOrigPointLoc;
	if (initPos) {
		anchorRefPoint = *initPos;
		// Scan from the highest frame index downward for a frame whose first hotspot has
		// a positive shape (matches IDA's negative-index walk via scrb_seekToFrameGroup).
		for (int32 frameId = static_cast<int32>(getFrameCount()) - 1; 0 <= frameId; frameId--) {
			ZmbHotspotGroup *cand = getHotspotGroupExact(frameId);
			if (!cand || cand->getHotspotCount() == 0)
				continue;
			if (cand->getHotspot(0)._shapeIdx > 0) {
				anchorFrame = cand;
				break;
			}
		}
	}
	if (!anchorFrame) {
		// Fallback / pInitPos==NULL path: use frame 0 first-positive-shape anchor.
		anchorFrame = getHotspotGroup(0);
	}
	if (anchorFrame && anchorFrame->getHotspotCount() > 0) {
		Common::Array<ZmbHotspot> hotspots = anchorFrame->copyHotspots();
		for (uint32 i = 0; i < hotspots.size(); i++) {
			if (hotspots[i]._shapeIdx > 0) {
				setPointLoc(Common::Point(
					anchorRefPoint.x - hotspots[i]._x,
					anchorRefPoint.y - hotspots[i]._y));
				break;
			}
		}
	}

	// IDA snoidScript_initAndPlay_455C0D stores the first parameter (chIsFacingLeft)
	// into chRand_64_0, NOT into the actual chIsFacingLeft field. The actual facing
	// direction is preserved from before playback. chRand_64_0==1 causes the snoid
	// to be hidden (render deactivated) when the SCRS finishes.
	_scrsHideOnComplete = hideOnComplete;

	_animState = rejectState ? kSnoidAnimScriptReject : kSnoidAnimScriptNormal;
	_scrsAnimCycleCount = 0;

	// Start at frame 0. activateAnimate enables voice/sound event processing.
	setLastFrameIdx(0);
	setLastSoundedFrameIdx(-1);
	activateAnimate();

	// Set the select-render-frame hook so blitShapes reads _lastFrameIdx
	// instead of time-cycling via defaultSelectRenderFrame.
	setSelectRenderFrameFunc(&ZoombiniPage::selectScrsRenderFrame);
}

void ZmbSnoid::finishScrsPlayback() {
	// Restore original position
	setPointLoc(_scrsOrigPointLoc);
	// Clear the SCRS frame selection hook (idle snoids use frame 0 / virtual hotspots)
	setSelectRenderFrameFunc(nullptr);
	deactivateAnimate();
}

void ZmbSnoid::setAnimState(SnoidAnimState state, const Common::Point *pos) {
	// Clamp unknown states to idle (IDA: wAnimKind > 0x0A → 0)
	if (state > kSnoidAnimArrivalMotion && state != kSnoidAnimPath)
		state = kSnoidAnimIdle;

	// Reset walk animation cycle when entering a walking state
	if (state == kSnoidAnimDepart) {
		_walkPhase = 0;
		// If departing and target is to the right, ensure facing right for first walk frame
		if (pos) {
			if (pos->x > getPointLoc().x)
				_isFacingLeft = false;
			else if (pos->x < getPointLoc().x)
				_isFacingLeft = true;
		}
	}

	// Restore idle pose when returning to idle.
	// Use the correct (normal or small) idle hotspot table.
	// IDA: animateZoombini_455E76(0, 0, pZmb) rebuilds idle hotspots unconditionally.
	// IDA wBool_0x122=0 on every idle transition — all snoids face right when settling.
	if (state == kSnoidAnimIdle) {
		_isFacingLeft = false;
		_needsIdleRedraw = true;
		if (_useSmallShapeRegs)
			setupSmallIdleHotspots();
		else
			setupIdleHotspots();
	}

	_animState = state;
	_flipCounter = 0;

	// IDA: animateZoombini_455E76 flip (state 3) initialisation — compute
	// shadow shapes from trait-specific categories (425–445 range) and store
	// in _flipShadowShapes[]. Each tick onSnoidAnimTick swaps the main
	// hotspot shapes with these shadows for 6 ticks.
	if (state == kSnoidAnimFlip) {
		_shapeImageIdx = 1;
		// Layer order: 0=foot, 1=body, 2=nose, 3=eye, 4=head.
		// IDA: iLayer 0 → cFoot+435, 2 → cNose+440, 3 → cEye+430, 4 → cHead+425
		// Layer 1 (body) always copies from main.
		ZmbHotspotGroup *hsGroup = getHotspotGroup(0);
		if (hsGroup && hsGroup->getHotspotCount() >= 5) {
			_flipShadowShapes[0] = static_cast<int16>(_trait._foot + 435);
			_flipShadowShapes[1] = hsGroup->getHotspot(1)._shapeIdx; // body: copy from main
			_flipShadowShapes[2] = static_cast<int16>(_trait._nose + 440);
			_flipShadowShapes[3] = static_cast<int16>(_trait._eye + 430);
			_flipShadowShapes[4] = static_cast<int16>(_trait._head + 425);
		}
	}

	if (pos)
		setPointLoc(*pos);

	// Reset frame counters for fresh animation playback
	// (IDA: pZmb->wGroupFrameIdx0098 = 0, dwHotspotIdx009A = 2)
}

// Compute per-animation-interval walk/path speed from source to destination.
// Mirrors snoidPath_stepAndComputeVelocity_4548DF direction+speed calculation from IDA.
// IDA uses a slope-based direction bucket (0=up, 1=up-right, 2=right, 3=down-right, 4=down)
// then a fixed speed table; the dominant axis uses the table value directly and the
// minor axis is a proportional fraction.
// Speed values are raw pixels per animation tick.  onSnoidAnimTick() uses a time-based
// deadline (IDA: dNextRenderFrame <= scrb_dwFrameRenderTime) with kAnimInterval=6, matching
// IDA dFrameInterval=6 from zmb_registerSnoidFeatureRunner.  The original's frame counter
// is getMillis()/17 (ceil(1000/60), 486SX integer approx), so one animation tick = 6*17ms
// = 102ms of wall clock time.  ScummVM uses getCurrentFrameCounter() = getMillis()/16.667,
// giving 6*16.667ms ≈ 100ms — close enough to the original.
static void calcPathSpeed(int16 dx, int16 dy, int16 &speedX, int16 &speedY) {
	// dy here = curY - targetY (positive means target is above current pos on screen)
	int slope;
	if (dx != 0) {
		slope = ((int)dy << 10) / ABS(dx);
	} else if (dy >= 0) {
		slope = 1410;
	} else {
		slope = -1410;
	}

	// Direction bucket + speed table from IDA snoidPath_stepAndComputeVelocity_4548DF:
	// dir 0 (≤-1409): mostly upward        → sx=5,  sy=-15
	// dir 1 (-1409..-333): steep up-right  → sx=13, sy=-10
	// dir 2 (-332..331):   mostly horizontal→ sx=16, sy=8
	// dir 3 (332..1408):   steep down-right → sx=13, sy=10
	// dir 4 (≥1409): mostly downward        → sx=5,  sy=15
	int16 sx, sy;
	if (slope <= -1409) {
		sx = 5;
		sy = -15;
	} else if (slope <= -332) {
		sx = 13;
		sy = -10;
	} else if (slope < 332) {
		sx = 16;
		sy = 8;
	} else if (slope < 1409) {
		sx = 13;
		sy = 10;
	} else {
		sx = 5;
		sy = 15;
	}

	// Dominant-axis clamping: IDA snoidPath_stepAndComputeVelocity_4548DF
	// scale = |dominant_dist| / |template_speed| = number of frames to cross dominant axis.
	// The minor axis speed is then dy/scale (or dx/scale) so both axes finish together.
	if (ABS(sx) >= ABS(sy)) {
		// X dominates
		speedX = sx;
		int scale = ABS(dx) / ABS(sx); // frames to cross dx at template rate
		speedY = (scale != 0) ? (int16)(dy / scale) : (int16)dy;
		if (speedY == 0 && dy != 0)
			speedY = (dy > 0) ? 1 : -1;
	} else {
		// Y dominates
		speedY = sy;
		int scale = ABS(dy) / ABS(sy); // frames to cross dy at template rate
		speedX = (scale != 0) ? (int16)(dx / scale) : (int16)dx;
		if (speedX == 0 && dx != 0)
			speedX = (dx > 0) ? 1 : -1;
	}

	// Speed values are raw pixels-per-animation-interval from the IDA speed table
	// (snoidPath_stepAndComputeVelocity_4548DF).  onSnoidAnimTick() fires every
	// kAnimInterval=6 frame-counter units (~100ms), matching IDA dFrameInterval=6.
	// No extra scaling needed.
	speedX = ABS(speedX);
	if (speedY != 0) {
		int16 sgnY = (speedY > 0) ? 1 : -1;
		speedY = sgnY * ABS(speedY);
	}
}

void ZmbSnoid::initWalkToTarget(const Common::Point &target) {
	// IDA: animateZoombini(0, 7, core) — sets DEPARTING state which will
	// initialise waypoint routing (or straight-line walk if no NODE data)
	// and compute dynamic speed in the kSnoidAnimDepart tick handler.
	_animTargetPos = target;
	setAnimState(kSnoidAnimDepart);
}

static int computeWalkDirBucket(int16 dx, int16 dy);

bool ZmbSnoid::onSnoidAnimTick(ZoombiniPage *page) {
	// IDA: onRender_ZoombiniAnimation_452B9C checks dNextRenderFrame <= scrb_dwFrameRenderTime
	// (updated by gfx_renderFrame as game_getFrameCounterOrDelta(), which returns
	// elapsed_ms/17).  dFrameInterval=6 from zmb_registerSnoidFeatureRunner, so animation
	// fires every 6 frame-counter units = 6*17ms = 102ms of wall-clock time.
	//
	// The original's timer is absolute-time-based (scrb_dwFrameRenderTime = getMillis()/17),
	// NOT frame-count-based.  This is critical because the render loop rate varies, and a
	// simple "count 6 calls" approach drifts slower when onAnimFrame() fires at slightly
	// longer intervals than 16.67ms (e.g. 18-22ms due to main-loop alignment).  We mirror
	// the original by comparing against getCurrentFrameCounter() (= getMillis()/16.667).
	if (_delayUntilFrame != 0) {
		if (page->getCurrentFrameCounter() < _delayUntilFrame)
			return false;
		// Delay expired: clear the delay gate.
		// IDA: dNextRenderFrame gates only the pre-render (animation tick)
		// in onRender_ZoombiniAnimation_452B9C.  The post-render callback
		// (onPostRender_ZoombiniAnimation_452ADD) always draws when
		// wBoolDoRender=1, so _isRenderActivated is NOT changed by
		// stagger delays — only animation timing is gated.
		_delayUntilFrame = 0;
	}
	// IDA 0x452BBC: hidden snoids (wBoolDoRender=0) skip the entire animation
	// state machine.  Without this, SCRS pool snoids that were hidden after
	// script completion keep ticking their idle counter and can trigger fidget
	// voice SFX with uninitialised traits, producing wrong sounds.
	if (!isRenderActivated())
		return false;
	// IDA: dFrameInterval = 6.  Time-based deadline, not frame-count.
	static constexpr uint32 kAnimInterval = 6;
	uint32 currentFrame = page->getCurrentFrameCounter();
	if (currentFrame < _nextAnimFrame)
		return false;
	_nextAnimFrame = currentFrame + kAnimInterval;

	bool needsRedraw = false;

	switch (_animState) {
	case kSnoidAnimIdle:
		// IDA LABEL_80: on first tick after re-entering idle, clear leftover
		// wAnimBaseFlag00F5 (set to 1 by animateZoombini on idle entry) and
		// mark redraw.  Note: chZmbAnimShapeCommonImageIdx (_shapeImageIdx) is
		// intentionally NOT cleared here — the original preserves it across
		// idle ticks so the fidget set selection (A vs B) depends on walk history.
		if (_needsIdleRedraw) {
			_needsIdleRedraw = false;
			needsRedraw = true;
		}
		// Fidget roll: periodically advance counter, 10% chance to trigger fidget.
		// IDA: word_4A4764 is the global fidget threshold (default 64, halved on
		// idle >3600 ticks, 0 = disabled).
		if (_vm->_fidgetThreshold) {
			_idleTickCounter++;
			if (_idleTickCounter > _vm->_fidgetThreshold) {
				_idleTickCounter = 0;
				if (_vm->_rnd->getRandomNumber(99) < 10) {
					// IDA animateZoombini(kind=6): chZmbAnimShapeCommonImageIdx
					// cycle: 0→set to 1 (Set A), 1→Set A, 2→Set B.
					if (_shapeImageIdx == 0)
						_shapeImageIdx = 1;
					_fidgetValue = _vm->_rnd->getRandomNumber(6);
					setAnimState(kSnoidAnimFidget);
					// IDA LABEL_52: play voice group 4 or 5 (50/50 random),
					// matching the original "happy idle" sound trigger.
					int16 voiceGroup = (_vm->_rnd->getRandomNumber(1) == 0) ? 4 : 5;
					// IDA word_4B762C: preload SND 100–424 every 32 triggers.
					// Original engine cached sound data in memory for faster
					// playback on 486-era hardware. ScummVM's archive system
					// handles on-demand loading efficiently, so no actual
					// preload I/O is needed — just maintain the counter state.
					_vm->_fidgetSoundPreloadCounter = (_vm->_fidgetSoundPreloadCounter + 1) % 32;
					int16 sndResId = getVoiceResId(voiceGroup);
					if (sndResId > 0)
						_vm->_sound->playZmbSound(
							ZmbResource(ZmbArchiveKind::kSystem, static_cast<uint16>(sndResId)),
							Audio::Mixer::kSFXSoundType);
					needsRedraw = true;
				}
			}
		}
		break;

	case kSnoidAnimTurnRight:
	case kSnoidAnimTurnLeft: {
		// IDA cases 1/2 (0x453043–0x4530D8): post-arrival turn-around animation.
		// Cycles _shapeImageIdx and _isFacingLeft before settling to idle.
		// State 1 enters facing right, flips to left, then idles.
		// State 2 enters facing left, flips to right, then idles.
		// Falls through to idle tick (fidget check) in the same tick.
		_idleTickCounter = 0;
		needsRedraw = true;

		if (_animState == kSnoidAnimTurnRight) {
			// State 1: settling condition = facing left
			if (_isFacingLeft) {
				_shapeImageIdx = 1;
				_animState = kSnoidAnimIdle;
			} else {
				if (_shapeImageIdx == 2) {
					_shapeImageIdx = 1;
				} else {
					_shapeImageIdx = 0;
					_isFacingLeft = true;
				}
			}
		} else {
			// State 2: settling condition = facing right
			if (_isFacingLeft) {
				if (_shapeImageIdx == 2) {
					_shapeImageIdx = 1;
				} else {
					_shapeImageIdx = 0;
					_isFacingLeft = false;
				}
			} else {
				_shapeImageIdx = 1;
				_animState = kSnoidAnimIdle;
			}
		}

		// IDA: falls through to LABEL_80 (idle tick) — run fidget check in same tick.
		// ScummVM fix: the original's render callback (snoidScript_renderFrame_4562B2)
		// dynamically computes sprite shapes from wAnimHotspotSetIdx + chZmbAnimShapeCommonImageIdx,
		// so the direct _animState = kSnoidAnimIdle above works transparently.
		// ScummVM pre-bakes shapes into _hsFrameMap[0] via setupIdleHotspots(), so we must
		// explicitly rebuild idle hotspots here — otherwise the walk-frame shapes remain
		// in _hsFrameMap[0] and the snoid appears stuck in the last walk frame.
		if (_animState == kSnoidAnimIdle) {
			_needsIdleRedraw = true;
			if (_useSmallShapeRegs)
				setupSmallIdleHotspots();
			else
				setupIdleHotspots();
		}
		break;
	}

	case kSnoidAnimFlip: {
		// IDA case 3 (0x4531F8): swap the 5 shape-layer slots with the shadow
		// slots each tick. After 6 swaps → idle. Does NOT fall through to redraw.
		if (_flipCounter >= 6) {
			setAnimState(kSnoidAnimIdle);
			_idleTickCounter = 0;
		} else {
			// Swap main hotspot shapes with shadow shapes
			ZmbHotspotGroup *hsGroup = getHotspotGroup(0);
			if (hsGroup && hsGroup->getHotspotCount() >= 5) {
				for (int i = 0; i < 5; i++) {
					int16 tmp = hsGroup->getHotspot(i)._shapeIdx;
					hsGroup->getHotspot(i)._shapeIdx = _flipShadowShapes[i];
					_flipShadowShapes[i] = tmp;
				}
			}
			_flipCounter++;
		}
		needsRedraw = true;
		break;
	}

	case kSnoidAnimArrive: {
		// IDA case 4 (0x45317E): teleport snoid to target, then enter arrivalTurnState.
		// The original does NOT animate a walk; it just copies pos2→pos1 immediately.
		needsRedraw = true;
		Common::Point pos = getPointLoc();
		if (pos == _animTargetPos) {
			_idleTickCounter = 0;
			// IDA: animateZoombini(0, word_4B6D4A, pZmb) — enter the global
			// post-arrival turn-around state (0=idle, 1=turnRight, 2=turnLeft).
			setAnimState(static_cast<SnoidAnimState>(_vm->_arrivalTurnState));
		} else {
			// IDA: *(a2+289)=1 (step-phase reset), *(a2+290)=0 (wBool_0x122=0 → facing RIGHT)
			// then copy target coordinates to current position (teleport)
			_isFacingLeft = false;
			setPointLoc(_animTargetPos);
		}
		break;
	}

	case kSnoidAnimDrag: {
		// IDA case 5: Position is set externally by mouse handler.
		// Use holding animation (SCRS 146–150) based on foot type.
		// IDA: wGroupFrameIdx0098 advances each tick in snoidScript_renderFrame.
		// When phase >= wScriptFrameCount, reset to frame 2 and loop (0 for small snoid).
		// This cycles through all holding animation frames for feet animation.
		needsRedraw = true;
		if (page) {
			const ZmbWalkAnim &anim = page->getHoldingAnim(_trait._foot);
			if (anim.frameCount > 0) {
				// Advance phase each tick and wrap to 2 (looping frames start at 2)
				// IDA: When wGroupFrameIdx0098 >= wScriptFrameCount, reset to 2 and seek.
				if (_holdingAnimPhase >= anim.frameCount) {
					// Reset to frame 2 for looping (frame 0 and 1 are direction-based entry poses)
					// IDA: Small snoid mode (word_4A48B6) resets to 0 instead of 2
					_holdingAnimPhase = 2;
				}
				updateHoldingHotspots(page);
				++_holdingAnimPhase;
			}
		}
		break;
	}

	case kSnoidAnimFidget: {
		// IDA case 6: play SCRS fidget frames (wGroupFrameIdx0098 advances each tick),
		// then animateZoombini(0,0,...) → idle when wGroupFrameIdx0098 >= wScriptFrameCount.
		// _shapeImageIdx=1 → set A (SCRS 130–136); _shapeImageIdx=2 → set B (SCRS 138–144).
		needsRedraw = true;
		if (page) {
			int fidgetSet = (_shapeImageIdx >= 2) ? 1 : 0;
			const ZmbWalkAnim &anim = page->getFidgetAnim(fidgetSet, static_cast<int>(_fidgetValue));
			if (anim.frameCount > 0 && _flipCounter < anim.frameCount) {
				updateFidgetHotspots(page, fidgetSet, static_cast<int>(_fidgetValue), static_cast<int>(_flipCounter));
				++_flipCounter;
			} else {
				setAnimState(kSnoidAnimIdle);
			}
		} else {
			// Fallback when page is unavailable: wait ~20 ticks.
			if (++_flipCounter >= 20)
				setAnimState(kSnoidAnimIdle);
		}
		break;
	}

	case kSnoidAnimDepart: {
		// IDA case 7: calls snoidPath_initRoute_454CA9 (select path + walk direction) then
		// snoidPath_stepAndComputeVelocity_4548DF (compute speed), then transitions to state 112 and falls through.
		//
		// IDA snoidPath_initRoute_454CA9 path-routing algorithm (translated from original binary):
		// 1. Find v16 = 1-indexed waypoint nearest to finalDest (a1 = a2+278 in IDA).
		// 2. Among all paths containing v16, find the path j where the member
		//    nearest to curPos (position v2) has minimum squared distance.
		// 3. Determine walk direction: +1 if v2→k is forward, -1 if backward.
		//    Rule: dir=-1 when k≠0 AND v2≥k (curPos is at or past dest in path order).
		// 4. Start reading from position v2+1 (skip nearest-to-curPos, already there)
		//    and walk toward k, stopping when k is reached.
		_pathWaypoints.clear();
		_pathWaypointIdx = 0;

		if (page) {
			ZmbNode *node = page->getFirstNode();
			if (node && !node->_waypoints.empty()) {
				const Common::Point curPos = getPointLoc();
				const Common::Array<Common::Point> &wps = node->_waypoints;

				if (!node->_paths.empty()) {
					// --- snoidPath_initRoute_454CA9 correct routing ---
					// Step 1: v16 = nearest waypoint (1-indexed) to finalDest
					uint8 v16 = 0;
					int32 minDestDist = INT32_MAX;
					for (uint32 i = 0; i < wps.size(); i++) {
						int32 ddx = wps[i].x - _animTargetPos.x;
						int32 ddy = wps[i].y - _animTargetPos.y;
						int32 dist = ddx * ddx + ddy * ddy;
						if (dist <= minDestDist) {
							minDestDist = dist;
							v16 = (uint8)(i + 1);
						}
					}

					// Step 2: find best path + starting position (snoidPath_initRoute_454CA9 inner scan)
					int bestPath = -1;
					int bestV2 = 0; // 0-indexed position in path of nearest-to-curPos member
					int bestK = 0;  // 0-indexed position in path of v16
					int bestDir = 1;
					int32 bestCurDist = INT32_MAX; // v13

					for (int j = 0; j < (int)node->_paths.size(); j++) {
						const Common::Array<uint8> &path = node->_paths[j];
						// Find k: position of v16 in this path
						int k = -1;
						for (int idx = 0; idx < (int)path.size() && path[idx] != 0; idx++) {
							if (path[idx] == v16) {
								k = idx;
								break;
							}
						}
						if (k < 0)
							continue; // v16 not in this path

						// Scan all path members for nearest to curPos (v13 / bestCurDist)
						for (int v2 = 0; v2 < (int)path.size() && path[v2] != 0; v2++) {
							const Common::Point &wp = wps[path[v2] - 1]; // 1-indexed → 0-indexed
							int32 ddx = wp.x - curPos.x;
							int32 ddy = wp.y - curPos.y;
							int32 dist = ddx * ddx + ddy * ddy;
							if (dist <= bestCurDist) {
								bestCurDist = dist;
								bestPath = j;
								bestV2 = v2;
								bestK = k;
								bestDir = 1;
								// IDA: if (k != 0 && v2 >= k) dir = -1
								if (k != 0 && v2 >= k)
									bestDir = -1;
							}
						}
					}

					// Step 3: build path waypoints from bestV2+1 walking toward bestK
					if (bestPath >= 0) {
						const Common::Array<uint8> &path = node->_paths[bestPath];
						int pos = bestV2 + 1;
						while (pos >= 0 && pos < (int)path.size() && path[pos] != 0) {
							_pathWaypoints.push_back(wps[path[pos] - 1]);
							if (pos == bestK)
								break; // reached dest-nearest waypoint
							pos += bestDir;
						}
					}
				} else {
					// Fallback (no PATH data): linear traversal between nearest waypoints.
					// Find waypoint nearest to snoid's current position (entry into the path)
					uint32 iStart = 0;
					int32 minStartDist = INT32_MAX;
					for (uint32 i = 0; i < wps.size(); i++) {
						int32 ddx = wps[i].x - curPos.x;
						int32 ddy = wps[i].y - curPos.y;
						int32 dist = ddx * ddx + ddy * ddy;
						if (dist < minStartDist) {
							minStartDist = dist;
							iStart = i;
						}
					}
					// Find waypoint nearest to final destination (exit from the path)
					uint32 iEnd = 0;
					int32 minEndDist = INT32_MAX;
					for (uint32 i = 0; i < wps.size(); i++) {
						int32 ddx = wps[i].x - _animTargetPos.x;
						int32 ddy = wps[i].y - _animTargetPos.y;
						int32 dist = ddx * ddx + ddy * ddy;
						if (dist < minEndDist) {
							minEndDist = dist;
							iEnd = i;
						}
					}
					if (iStart <= iEnd) {
						for (uint32 i = iStart; i <= iEnd; i++)
							_pathWaypoints.push_back(wps[i]);
					} else {
						for (int32 i = static_cast<int32>(iStart); i >= static_cast<int32>(iEnd); i--)
							_pathWaypoints.push_back(wps[i]);
					}
				}
			}
		}

		// IDA: snoidPath_stepAndComputeVelocity_4548DF bUseSubTargetFinalDest check:
		// The first call to stepAndComputeVelocity (state 7) reads the waypoint at
		// path[bestV2+1] and checks whether finalDest is closer to curPos than that
		// waypoint.  If so it sets pos2 = finalDest, skipping the entire path.
		// Without this, walk-in snoids overshoot their seats by visiting every path
		// waypoint before the distance shortcut fires at waypoint arrival.
		if (_pathWaypointIdx < _pathWaypoints.size()) {
			const Common::Point curPos = getPointLoc();
			int32 dxFd = _animTargetPos.x - curPos.x;
			int32 dyFd = _animTargetPos.y - curPos.y;
			int32 dxWp = _pathWaypoints[_pathWaypointIdx].x - curPos.x;
			int32 dyWp = _pathWaypoints[_pathWaypointIdx].y - curPos.y;
			if (dxFd * dxFd + dyFd * dyFd <= dxWp * dxWp + dyWp * dyWp)
				_pathWaypointIdx = (uint32)_pathWaypoints.size(); // cull all waypoints
		}

		// IDA: state 7 (departing) falls through directly to LABEL_20 (movement code)
		// in the same tick — route init + first movement step happen simultaneously.
		// Matching this: transition to kSnoidAnimPath then immediately apply the first step.
		needsRedraw = true;
		_animState = kSnoidAnimPath;
		// IDA: snoidPath_stepAndComputeVelocity_4548DF is called here to set dVelocityXY
		// and wAnimBaseFlag00F5 (direction bucket) — both constant for the first segment.
		// Compute initial direction bucket AND speed toward the first waypoint (or final target).
		{
			const Common::Point curPos = getPointLoc();
			const Common::Point nextWp = (_pathWaypointIdx < _pathWaypoints.size()) ? _pathWaypoints[_pathWaypointIdx] : _animTargetPos;
			int16 initDx = nextWp.x - curPos.x;
			int16 initDy = curPos.y - nextWp.y; // curY - targetY (IDA convention)
			_walkDirBucket = computeWalkDirBucket(initDx, initDy);
			calcPathSpeed(initDx, initDy, _animSpeedX, _animSpeedY);
			// Ensure facing direction is set before first walk frame
			_isFacingLeft = (initDx < 0);
		}
		updateWalkHotspots(page, _walkDirBucket, 0);
		// Fall through: apply first movement step in this same tick (IDA LABEL_20 fallthrough).
		{
			Common::Point pos = getPointLoc();
			const Common::Point subTarget = (_pathWaypointIdx < _pathWaypoints.size()) ? _pathWaypoints[_pathWaypointIdx] : _animTargetPos;
			int16 dx = subTarget.x - pos.x;
			int16 dy = pos.y - subTarget.y;
			if (dx != 0 || dy != 0) {
				if (dx != 0) {
					int16 step = MIN<int16>(ABS(dx), ABS(_animSpeedX));
					pos.x += (dx > 0) ? step : -step;
					_isFacingLeft = (dx < 0);
				}
				if (dy != 0) {
					int16 step = MIN<int16>(ABS(dy), ABS(_animSpeedY));
					pos.y += (dy > 0) ? -step : step;
				}
				setPointLoc(pos);
				++_walkPhase;
				updateWalkHotspots(page, _walkDirBucket, _walkPhase);
			}
		}
		break;
	}

	case kSnoidAnimPath: {
		// IDA LABEL_20 / state 112: move along NODE waypoints toward final destination.
		// Sub-target advances through _pathWaypoints; after the last waypoint,
		// walks straight to _animTargetPos.
		needsRedraw = true;
		Common::Point pos = getPointLoc();

		// Pick current sub-target
		Common::Point subTarget = _animTargetPos;
		if (_pathWaypointIdx < _pathWaypoints.size())
			subTarget = _pathWaypoints[_pathWaypointIdx];

		int16 dx = subTarget.x - pos.x;
		int16 dy = pos.y - subTarget.y; // IDA convention: curY - targetY

		if (dx == 0 && dy == 0) {
			if (_pathWaypointIdx < _pathWaypoints.size()) {
				// Reached this waypoint — advance to the next, skipping any
				// occupied by a stationary snoid.
				// IDA threshold 500 = squared distance (~radius √500 ≈ 22px).
				_pathWaypointIdx++;
				if (page) {
					while (_pathWaypointIdx < _pathWaypoints.size() &&
						   page->isPointOccupiedByOtherSnoid(this, _pathWaypoints[_pathWaypointIdx], 500))
						_pathWaypointIdx++;
				}
				// IDA finalDest distance shortcut (snoidPath_stepAndComputeVelocity_4548DF):
				// After advancing the slot, check: if distSq(curPos, finalDest) <=
				// distSq(curPos, nextWaypoint), skip remaining waypoints and go straight
				// to finalDest.  This culls waypoints once the destination becomes the
				// nearest point, exactly matching the binary's bUseSubTargetFinalDest logic.
				// Without this, ScummVM visits every waypoint even when finalDest is
				// already closer — causing ~50% slower walks on the Picker path.
				if (_pathWaypointIdx < _pathWaypoints.size()) {
					const Common::Point &nextWp = _pathWaypoints[_pathWaypointIdx];
					int32 dxFd = _animTargetPos.x - pos.x;
					int32 dyFd = _animTargetPos.y - pos.y;
					int32 dxWp = nextWp.x - pos.x;
					int32 dyWp = nextWp.y - pos.y;
					if (dxFd * dxFd + dyFd * dyFd <= dxWp * dxWp + dyWp * dyWp)
						_pathWaypointIdx = (uint32)_pathWaypoints.size(); // cull remaining
				}
				// IDA: snoidPath_stepAndComputeVelocity_4548DF() advances pos2 to the next
				// sub-target AND recomputes dVelocityXY + wAnimBaseFlag00F5 for the new segment.
				// Update bucket, speed, and sprite direction now — all held constant until the
				// next waypoint is reached (eliminating mid-segment direction drift).
				subTarget = (_pathWaypointIdx < _pathWaypoints.size()) ? _pathWaypoints[_pathWaypointIdx] : _animTargetPos;
				int16 newDx = subTarget.x - pos.x;
				int16 newDy = pos.y - subTarget.y;
				if (newDx != 0 || newDy != 0) {
					int newBucket = computeWalkDirBucket(newDx, newDy);
					if (newBucket != _walkDirBucket) {
						_walkDirBucket = newBucket;
						updateWalkHotspots(page, _walkDirBucket, _walkPhase);
					}
					calcPathSpeed(newDx, newDy, _animSpeedX, _animSpeedY);
					if (newDx != 0)
						_isFacingLeft = (newDx < 0);
				}
			} else {
				// Reached the final destination — enter arrivalTurnState, face right.
				// IDA: wBool_0x122=0 on arrival (same as kSnoidAnimArrive teleport path).
				// IDA: animateZoombini(0, word_4B6D4A, pZmb) — enter global turn-around state.
				_isFacingLeft = false;
				_idleTickCounter = 0;
				setAnimState(static_cast<SnoidAnimState>(_vm->_arrivalTurnState));
				// IDA: if (ui_bDragLockActive > 0) --ui_bDragLockActive;
				if (_vm->_walkersInProgress > 0)
					--_vm->_walkersInProgress;
			}
		} else {
			// Speed (_animSpeedX/_animSpeedY) and direction (_walkDirBucket) are fixed for
			// the current segment — set in kSnoidAnimDepart and when a waypoint is reached,
			// matching IDA's dVelocityXY / wAnimBaseFlag00F5 which are written only by
			// snoidPath_stepAndComputeVelocity_4548DF and held constant mid-segment.
			// Recomputing from the shrinking dx/dy each tick caused the slope to drift toward
			// 0 near waypoints (integer rounding exhausts one axis before the other),
			// making the sprite appear to walk horizontally instead of diagonally.
			if (dx != 0) {
				int16 step = MIN<int16>(ABS(dx), ABS(_animSpeedX));
				pos.x += (dx > 0) ? step : -step;
				_isFacingLeft = (dx < 0);
			}
			// IDA: dy>=0 → curY--, dy<0 → curY++
			if (dy != 0) {
				int16 step = MIN<int16>(ABS(dy), ABS(_animSpeedY));
				pos.y += (dy > 0) ? -step : step;
			}
			setPointLoc(pos);

			// Advance walk animation phase once per interval fire.
			// IDA: wGroupFrameIdx0098 advances each snoidScript_renderFrame_4562B2 call,
			// which fires every dFrameInterval (=6 × ~20ms ≈ 120ms).
			++_walkPhase;
			updateWalkHotspots(page, _walkDirBucket, _walkPhase);
		}
		break;
	}

	case kSnoidAnimScriptReject:
	case kSnoidAnimScriptNormal: {
		// IDA: onRender_ZoombiniAnimation_452B9C advances one SCRS frame per render-timer
		// fire (dFrameInterval=6).  When wGroupFrameIdx0098 >= wScriptFrameCount, the
		// snoid either hides (chRand_64_0==1) or reverts to idle.
		//
		// Use the SCRS-declared frame count (`getFrameCount() - 1`) as the
		// terminal index - NOT `getMaxFrameIdx()`. After the parseFrames fix
		// these are equal for all features, but keep the explicit cast here
		// to express intent: IDA `wScriptFrameCount` (= pScrsData->wGroupCount)
		// is the header count, and the snoid must traverse every declared frame
		// so terminator-only trailing frames (Ferry SCRS 1900/1904/1906 frame
		// 24's `ff03` carrying case 2 of `ferry_rejectFlightSCRBCallback`) get
		// reached. preRenderFeature handles the actual event dispatch as each
		// frame is rendered.
		const int32 lastFrame = static_cast<int32>(getFrameCount()) - 1;
		if (getLastFrameIdx() < lastFrame) {
			setLastFrameIdx(getLastFrameIdx() + 1);
			needsRedraw = true;
		} else {
			// SCRS animation finished.
			// IDA: if chRand_64_0==1, hide the snoid (wBoolDoRender=0) without
			// restoring position. Otherwise animateZoombini(0,0,...) -> idle.
			if (_scrsHideOnComplete) {
				deactivateRender();
				setSelectRenderFrameFunc(nullptr);
				deactivateAnimate();
				_animState = kSnoidAnimIdle;
			} else {
				finishScrsPlayback();
				setAnimState(kSnoidAnimIdle);
			}
			_scrsHideOnComplete = false;
			// IDA: fires onHotspotShapeOrFrameFunc(-1) completion callback.
			if (page)
				page->onFeatureAnimEvent(this, kZmbAnimEventM1_End);
			needsRedraw = true;
		}
		break;
	}

	case kSnoidAnimArrivalMotion:
		// IDA case 10 (0x453255): calls animateZoombini_455E76(0, 7, ...) → kSnoidAnimDepart,
		// increments the global "walkers in progress" counter, then returns.
		// No redraw is set; the depart state picks up on the next tick.
		setAnimState(kSnoidAnimDepart);
		++_vm->_walkersInProgress;
		break;

	default:
		break;
	}

	return needsRedraw;
}

void ZmbSnoid::setupIdleHotspots() {
	// IDA-reversed lookup tables: trait value (1-5) → combined shapeIdx offset.
	// Index 0 is unused (TRAIT_NONE); traits are 1-indexed.
	// Source: wArrZmbBodyFoot_4A4770, wArrZmbBodyNose_4A477C,
	//         wArrZmbBodyEye_4A4788,  wArrZmbBodyHead_4A4794
	static const uint16 kFootTable[6] = {0, 191, 246, 335, 360, 411};
	static const uint16 kNoseTable[6] = {0, 171, 175, 179, 183, 187};
	static const uint16 kEyeTable[6] = {0, 91, 107, 123, 139, 155};
	static const uint16 kHeadTable[6] = {0, 11, 27, 43, 59, 75};

	// rawShapeFromData = 2: corresponds to SCRS_101 (index 1 in zmbAnimHotspotArr_4B7094),
	// which is the idle/seated pose selected by animateZoombini_455E76 for wAnimKind==3.
	// IDA forces chZmbAnimShapeCommonImageIdx=1 → SCRS_101 → shapeId=2 for all layers.
	// (rawShapeFromData=1 / SCRS_100 is a front/center-facing pose, not the seated idle.)
	static constexpr uint16 kRawShapeIdle = 2;

	uint8 foot = _trait._foot;
	uint8 nose = _trait._nose;
	uint8 eye = _trait._eye;
	uint8 head = _trait._head;

	if (foot < 1 || foot > 5)
		foot = 1;
	if (nose < 1 || nose > 5)
		nose = 1;
	if (eye < 1 || eye > 5)
		eye = 1;
	if (head < 1 || head > 5)
		head = 1;

	Common::Array<ZmbHotspot> hotspots;
	// Layer order matching IDA's zmbRunner_setAnimShape_456785 sub-kind=0:
	// slot 0=foot, slot 1=body(0), slot 2=nose, slot 3=eye, slot 4=head
	hotspots.push_back(ZmbHotspot(0, kFootTable[foot] + kRawShapeIdle, 0, 0, 0));
	hotspots.push_back(ZmbHotspot(1, 0 + kRawShapeIdle, 0, 0, 0)); // body anchor
	hotspots.push_back(ZmbHotspot(2, kNoseTable[nose] + kRawShapeIdle, 0, 0, 0));
	hotspots.push_back(ZmbHotspot(3, kEyeTable[eye] + kRawShapeIdle, 0, 0, 0));
	hotspots.push_back(ZmbHotspot(4, kHeadTable[head] + kRawShapeIdle, 0, 0, 0));

	_usesVirtualHotspots = true;
	setVirtualHotspots(hotspots);
}

void ZmbSnoid::setupSmallIdleHotspots() {
	_useSmallShapeRegs = true;
	// Small-snoid body-part index tables from original binary (word_4A48B8..DC).
	// IDA: sub_4572C5(0) swaps global wArrZmbBody* tables to these values.
	// Used with SHPL resource 3200 (0xC80) in the System/Common MHK.
	// Trait value (1-5) → combined shapeIdx base offset into resource 3200.
	static const uint16 kSmallFootTable[6] = {0, 131, 174, 227, 235, 278};
	static const uint16 kSmallNoseTable[6] = {0, 111, 115, 119, 123, 127};
	static const uint16 kSmallEyeTable[6] = {0, 91, 95, 99, 103, 107};
	static const uint16 kSmallHeadTable[6] = {0, 11, 27, 43, 59, 75};

	// Same idle rawShape as normal: index 2 = SCRS_101 idle pose.
	static constexpr uint16 kRawShapeIdle = 2;

	uint8 foot = _trait._foot;
	uint8 nose = _trait._nose;
	uint8 eye = _trait._eye;
	uint8 head = _trait._head;

	if (foot < 1 || foot > 5)
		foot = 1;
	if (nose < 1 || nose > 5)
		nose = 1;
	if (eye < 1 || eye > 5)
		eye = 1;
	if (head < 1 || head > 5)
		head = 1;

	Common::Array<ZmbHotspot> hotspots;
	hotspots.push_back(ZmbHotspot(0, kSmallFootTable[foot] + kRawShapeIdle, 0, 0, 0));
	hotspots.push_back(ZmbHotspot(1, 0 + kRawShapeIdle, 0, 0, 0));
	hotspots.push_back(ZmbHotspot(2, kSmallNoseTable[nose] + kRawShapeIdle, 0, 0, 0));
	hotspots.push_back(ZmbHotspot(3, kSmallEyeTable[eye] + kRawShapeIdle, 0, 0, 0));
	hotspots.push_back(ZmbHotspot(4, kSmallHeadTable[head] + kRawShapeIdle, 0, 0, 0));

	_usesVirtualHotspots = true;
	setVirtualHotspots(hotspots);
}

int16 ZmbSnoid::getBodyLayerBaseOffset(uint8 layer, uint8 layerShift) const {
	// General trait tables (IDA: wArrZmbBody*_4A4770-4A4794, animKind != 9)
	static const int16 kFootTable[6] = {0, 191, 246, 335, 360, 411};
	static const int16 kNoseTable[6] = {0, 171, 175, 179, 183, 187};
	static const int16 kEyeTable[6] = {0, 91, 107, 123, 139, 155};
	static const int16 kHeadTable[6] = {0, 11, 27, 43, 59, 75};

	// NORMAL-specific trait tables (IDA: wArrZmbBody*_4A47A0-4A47C4, animKind == 9)
	static const int16 kNormalFootTable[6] = {0, 288, 306, 324, 342, 360};
	static const int16 kNormalNoseTable[6] = {0, 198, 216, 234, 252, 270};
	static const int16 kNormalEyeTable[6] = {0, 108, 126, 144, 162, 180};
	static const int16 kNormalHeadTable[6] = {0, 18, 72, 36, 54, 90};

	// Small-snoid tables (IDA: word_4A48B8..DC, installed by sub_4572C5(0))
	// Used when general tables are swapped to small variants (XFER_0).
	static const int16 kSmallFootTable[6] = {0, 131, 174, 227, 235, 278};
	static const int16 kSmallNoseTable[6] = {0, 111, 115, 119, 123, 127};
	static const int16 kSmallEyeTable[6] = {0, 91, 95, 99, 103, 107};
	static const int16 kSmallHeadTable[6] = {0, 11, 27, 43, 59, 75};

	// Apply IDA p_wUnk00C2 shift: for NORMAL scripts when first shape > 18,
	// the trait offset array is shifted by 1 (layer 0 = no offset).
	int effectiveLayer = static_cast<int>(layer) - static_cast<int>(layerShift);
	if (effectiveLayer < 0 || effectiveLayer > 4)
		return 0;

	uint8 foot = (_trait._foot >= 1 && _trait._foot <= 5) ? _trait._foot : 1;
	uint8 nose = (_trait._nose >= 1 && _trait._nose <= 5) ? _trait._nose : 1;
	uint8 eye = (_trait._eye >= 1 && _trait._eye <= 5) ? _trait._eye : 1;
	uint8 head = (_trait._head >= 1 && _trait._head <= 5) ? _trait._head : 1;

	// Table selection based on animation state, not variant.
	// IDA: state 9 (NORMAL) uses wArrZmbBody*_4A47A0 (NORMAL tables, never swapped).
	// State 8 (REJECT) and others use wArrZmbBody*_4A4770 (general tables).
	// When sub_4572C5(0) has been called (_useSmallShapeRegs), the general tables
	// have been swapped to small-snoid tables.
	const int16 *footTbl, *noseTbl, *eyeTbl, *headTbl;
	if (_animState == kSnoidAnimScriptNormal) {
		footTbl = kNormalFootTable;
		noseTbl = kNormalNoseTable;
		eyeTbl = kNormalEyeTable;
		headTbl = kNormalHeadTable;
	} else if (_useSmallShapeRegs) {
		footTbl = kSmallFootTable;
		noseTbl = kSmallNoseTable;
		eyeTbl = kSmallEyeTable;
		headTbl = kSmallHeadTable;
	} else {
		footTbl = kFootTable;
		noseTbl = kNoseTable;
		eyeTbl = kEyeTable;
		headTbl = kHeadTable;
	}

	// Slot mapping depends on variant (wAnimKind from zmbRunner_setAnimShape_456785).
	// IDA decompile at 0x456785 confirms three body-part orderings:
	//   variant 0: [foot, body(0), nose, eye, head]
	//   variant 1: [foot, nose, body(0), eye, head]
	//   variant 2: [body(0), eye, nose, foot, head]
	switch (_variant) {
	case 1:
		switch (effectiveLayer) {
		case 0:
			return footTbl[foot];
		case 1:
			return noseTbl[nose];
		case 2:
			return 0; // body
		case 3:
			return eyeTbl[eye];
		case 4:
			return headTbl[head];
		default:
			return 0;
		}
		break;
	case 2:
		switch (effectiveLayer) {
		case 0:
			return 0; // body
		case 1:
			return eyeTbl[eye];
		case 2:
			return noseTbl[nose];
		case 3:
			return footTbl[foot];
		case 4:
			return headTbl[head];
		default:
			return 0;
		}
		break;
	default: // variant 0 (most common)
		switch (effectiveLayer) {
		case 0:
			return footTbl[foot];
		case 1:
			return 0; // body
		case 2:
			return noseTbl[nose];
		case 3:
			return eyeTbl[eye];
		case 4:
			return headTbl[head];
		default:
			return 0;
		}
		break;
	}
}

int16 ZmbSnoid::getVoiceResId(int16 voiceGroup) const {
	// IDA getZoombiniVoiceResId_456FCB: maps voice group (0-17) to SND resource ID.
	// Groups 0-15: base SND ID in steps of 25 (100, 125, ..., 475) + trait-based offset.
	// Group 16: random SND in range [1800, 1814].
	// Group 17: fixed SND resource 99.
	static const int16 kVoiceGroupBase[16] = {
		100, 125, 150, 175, 200, 225, 250, 275,
		300, 325, 350, 375, 400, 475, 450, 425};

	int16 base = 0;
	bool applyTraitOffset = true;

	if (voiceGroup >= 0 && voiceGroup <= 15) {
		base = kVoiceGroupBase[voiceGroup];
	} else if (voiceGroup == 16) {
		base = _vm->_rnd->getRandomNumber(1800, 1814);
		applyTraitOffset = false;
	} else if (voiceGroup == 17) {
		base = 99;
		applyTraitOffset = false;
	}

	if (base == 0)
		return 0;

	if (applyTraitOffset) {
		// IDA disasm 0x4570b7: movsx edx, byte ptr [ebx+0BCh] → switch on traits._head (offset 0xBC).
		// ZmbTrait is at offset 0xBC in FeatureCore259: head(0xBC), eye(0xBD), nose(0xBE), foot(0xBF).
		// Head encodes gender: heads 1–3 = male, 4–5 = female → distinct SND block offsets.
		uint8 head = _trait._head;
		switch (head) {
		case 2:
			base += 5;
			break;
		case 3:
			base += 20;
			break;
		case 4:
			base += 15;
			break;
		case 5:
			base += 10;
			break;
		default:
			break; // head 0, 1: no additional offset
		}
		// IDA disasm 0x4570fa: movsx edx, byte ptr [ebx+0BEh] → add traits._nose (offset 0xBE).
		base += static_cast<int16>(_trait._nose) - 1;
	}

	return base;
}

/** Compute IDA snoidPath_stepAndComputeVelocity_4548DF direction bucket (0–4) from a movement vector.
 *  dy = curY - targetY (positive = target is above on screen).
 *  Slope thresholds are the IDA fixed-point values (<<10 scale).
 */
static int computeWalkDirBucket(int16 dx, int16 dy) {
	int32 slope;
	if (dx != 0) {
		slope = ((int32)dy << 10) / ABS(dx);
	} else {
		slope = (dy >= 0) ? 1410 : -1410;
	}
	if (slope <= -1409)
		return 0;
	if (slope <= -332)
		return 1;
	if (slope < 332)
		return 2;
	if (slope < 1409)
		return 3;
	return 4;
}

void ZmbSnoid::updateWalkHotspots(ZoombiniPage *page, int dirBucket, int phase) {
	if (!page)
		return;

	// Normal-size body-part tables (wArrZmbBodyFoot_4A4770 etc.)
	static const uint16 kFootTable[6] = {0, 191, 246, 335, 360, 411};
	static const uint16 kNoseTable[6] = {0, 171, 175, 179, 183, 187};
	static const uint16 kEyeTable[6] = {0, 91, 107, 123, 139, 155};
	static const uint16 kHeadTable[6] = {0, 11, 27, 43, 59, 75};

	// Small-snoid tables (word_4A48B8..DC, installed by sub_4572C5(0)).
	// Used when the snoid is walking with resource 3200 (XFER_0 picker-to-bridge).
	static const uint16 kSmallFootTable[6] = {0, 131, 174, 227, 235, 278};
	static const uint16 kSmallNoseTable[6] = {0, 111, 115, 119, 123, 127};
	static const uint16 kSmallEyeTable[6] = {0, 91, 95, 99, 103, 107};
	static const uint16 kSmallHeadTable[6] = {0, 11, 27, 43, 59, 75};

	const uint16 *footTbl = _useSmallShapeRegs ? kSmallFootTable : kFootTable;
	const uint16 *noseTbl = _useSmallShapeRegs ? kSmallNoseTable : kNoseTable;
	const uint16 *eyeTbl = _useSmallShapeRegs ? kSmallEyeTable : kEyeTable;
	const uint16 *headTbl = _useSmallShapeRegs ? kSmallHeadTable : kHeadTable;

	uint8 foot = CLIP<uint8>(_trait._foot, 1, 5);
	uint8 nose = CLIP<uint8>(_trait._nose, 1, 5);
	uint8 eye = CLIP<uint8>(_trait._eye, 1, 5);
	uint8 head = CLIP<uint8>(_trait._head, 1, 5);

	const ZmbWalkAnim &anim = page->getWalkAnim(foot, dirBucket);
	if (anim.frameCount == 0 || anim.frames.empty())
		return;

	const ZmbWalkFrame &fr = anim.frames[phase % static_cast<int>(anim.frameCount)];
	// Empty frame: keep current visual state (IDA leaves hotspots unchanged when no entries)
	if (fr.entryCount == 0)
		return;

	// IDA: zmb_setBodyLayerShapes: variant (wAnimKind from SCRS) determines slot layout.
	// On direction change, the original calls zmb_setBodyLayerShapes(pNewDirHotspots->hsArr[0].shapeid, pZmb)
	// which rearranges wArrBodyLayerShapeId[] based on the variant from the walk SCRS.
	//   variant 0: [foot, body(0), nose, eye, head]  — dirs 0,1,2 (down/horizontal)
	//   variant 1: [foot, nose, body(0), eye, head]  — dirs 3,4 (upward)
	//   variant 2: [body(0), eye, nose, foot, head]  — (unused in walk anims)
	int16 traitBase[5];
	if (anim.variant == 1) {
		traitBase[0] = static_cast<int16>(footTbl[foot]);
		traitBase[1] = static_cast<int16>(noseTbl[nose]); // slot 1 = nose
		traitBase[2] = 0;                                 // slot 2 = body base
		traitBase[3] = static_cast<int16>(eyeTbl[eye]);
		traitBase[4] = static_cast<int16>(headTbl[head]);
	} else {
		traitBase[0] = static_cast<int16>(footTbl[foot]);
		traitBase[1] = 0;                                 // slot 1 = body base
		traitBase[2] = static_cast<int16>(noseTbl[nose]); // slot 2 = nose
		traitBase[3] = static_cast<int16>(eyeTbl[eye]);
		traitBase[4] = static_cast<int16>(headTbl[head]);
	}

	Common::Array<ZmbHotspot> hotspots;
	for (int s = 0; s < 5; s++)
		hotspots.push_back(ZmbHotspot(s, traitBase[s] + fr.shape[s], 0, fr.x[s], fr.y[s]));

	_usesVirtualHotspots = true;
	setVirtualHotspots(hotspots);
}

void ZmbSnoid::updateFidgetHotspots(ZoombiniPage *page, int fidgetSet, int variant, int frameIdx) {
	if (!page)
		return;

	static const uint16 kFootTable[6] = {0, 191, 246, 335, 360, 411};
	static const uint16 kNoseTable[6] = {0, 171, 175, 179, 183, 187};
	static const uint16 kEyeTable[6] = {0, 91, 107, 123, 139, 155};
	static const uint16 kHeadTable[6] = {0, 11, 27, 43, 59, 75};

	uint8 foot = CLIP<uint8>(_trait._foot, 1, 5);
	uint8 nose = CLIP<uint8>(_trait._nose, 1, 5);
	uint8 eye = CLIP<uint8>(_trait._eye, 1, 5);
	uint8 head = CLIP<uint8>(_trait._head, 1, 5);

	const ZmbWalkAnim &anim = page->getFidgetAnim(fidgetSet, variant);
	if (anim.frameCount == 0 || anim.frames.empty())
		return;

	const ZmbWalkFrame &fr = anim.frames[frameIdx % static_cast<int>(anim.frameCount)];
	// Empty frame: keep current visual state (IDA leaves hotspots unchanged when no entries)
	if (fr.entryCount == 0)
		return;

	// Fidget SCRSes all have variant=0; apply arrangement 0 unconditionally here too.
	int16 traitBase[5];
	traitBase[0] = static_cast<int16>(kFootTable[foot]);
	traitBase[1] = 0;
	traitBase[2] = static_cast<int16>(kNoseTable[nose]);
	traitBase[3] = static_cast<int16>(kEyeTable[eye]);
	traitBase[4] = static_cast<int16>(kHeadTable[head]);

	Common::Array<ZmbHotspot> hotspots;
	for (int s = 0; s < 5; s++)
		hotspots.push_back(ZmbHotspot(s, traitBase[s] + fr.shape[s], 0, fr.x[s], fr.y[s]));

	_usesVirtualHotspots = true;
	setVirtualHotspots(hotspots);
}

void ZmbSnoid::updateHoldingHotspots(ZoombiniPage *page) {
	if (!page)
		return;

	// IDA: Holding (drag) uses SCRS 146–150, selected by foot type.
	// wAnimHotspotSetIdx = pZmb->footTrait + 45 → foot 1 → index 46 → SCRS 146
	// Frame cycling: _holdingAnimPhase advances each tick in onSnoidAnimTick.
	// IDA: wGroupFrameIdx0098 cycles through all frames, loops from 2 when >= frameCount.
	static const uint16 kFootTable[6] = {0, 191, 246, 335, 360, 411};
	static const uint16 kNoseTable[6] = {0, 171, 175, 179, 183, 187};
	static const uint16 kEyeTable[6] = {0, 91, 107, 123, 139, 155};
	static const uint16 kHeadTable[6] = {0, 11, 27, 43, 59, 75};

	uint8 foot = CLIP<uint8>(_trait._foot, 1, 5);
	uint8 nose = CLIP<uint8>(_trait._nose, 1, 5);
	uint8 eye = CLIP<uint8>(_trait._eye, 1, 5);
	uint8 head = CLIP<uint8>(_trait._head, 1, 5);

	const ZmbWalkAnim &anim = page->getHoldingAnim(foot);
	if (anim.frameCount == 0 || anim.frames.empty())
		return;

	// Use _holdingAnimPhase for frame cycling during drag.
	// IDA: wGroupFrameIdx0098 advances each tick, wrapping at frameCount.
	// Phase resets to 2 when looping (frames 0-1 are entry poses, 2+ are cycling).
	int frameIdx = CLIP<int>(static_cast<int>(_holdingAnimPhase), 0, static_cast<int>(anim.frameCount) - 1);
	const ZmbWalkFrame &fr = anim.frames[frameIdx];

	// Empty frame: keep current visual state
	if (fr.entryCount == 0)
		return;

	// Holding SCRSes use variant=0 (normal arrangement).
	int16 traitBase[5];
	traitBase[0] = static_cast<int16>(kFootTable[foot]);
	traitBase[1] = 0;
	traitBase[2] = static_cast<int16>(kNoseTable[nose]);
	traitBase[3] = static_cast<int16>(kEyeTable[eye]);
	traitBase[4] = static_cast<int16>(kHeadTable[head]);

	Common::Array<ZmbHotspot> hotspots;
	for (int s = 0; s < 5; s++)
		hotspots.push_back(ZmbHotspot(s, traitBase[s] + fr.shape[s], 0, fr.x[s], fr.y[s]));

	_usesVirtualHotspots = true;
	setVirtualHotspots(hotspots);
}

} // End of namespace Mohawk
