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

#ifndef MOHAWK_ZOOMBINI_SCRIPTS_H
#define MOHAWK_ZOOMBINI_SCRIPTS_H

#include "common/array.h"
#include "common/ptr.h"
#include "common/rect.h"
#include "common/stack.h"
#include "common/stablemap.h"
#include "common/substream.h"

#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_state.h"
#include "mohawk/zoombini_resource.h"

namespace Common {

class KeyState;

} // End of namespace Common

namespace Mohawk {

class MohawkEngine_Zoombini;
class ZoombiniPage;

/**
 * Represents NODE resource in .MHK archive, which defines a set of waypoints.
 * Zoombinis can move along the waypoints defined in a NODE resource.
 * 
 * Structure:
 * - (UINT16_BE) number of points (times 4 = size of following data)
 * - (INT16_BE, INT16_BE) * numPoints: array of points (x, y) 
 */
class ZmbNode {
public:
	Common::Array<Common::Point> _waypoints;

	/**
	 * Paths from the companion PATH resource (same resource ID, different tag).
	 * IDA: dword_4A48A0 / pMhkResNode_4A48A4 pair used by snoidPath_initRoute_454CA9 / snoidPath_stepAndComputeVelocity_4548DF.
	 * Each path is up to 24 entries of 1-indexed waypoint indices (0 = empty slot).
	 * Paths define safe routes between waypoints; snoidPath_initRoute_454CA9 selects the path
	 * containing the destination-nearest waypoint, then walks the selected path.
	 */
	Common::Array<Common::Array<uint8>> _paths;

	void parseStream(Common::SeekableReadStream *stream);
	void parsePathStream(Common::SeekableReadStream *stream);
};

/**
 * REGS - Registration Point Offsets
 * 
 * Structure: Array of sint16BE values
 * - First value is often 0x0000, and skipped (no offset for shape index 0, or reserved/unused)
 * - Each value represents a registration offset (X or Y coordinate) for a shape at that index
 * - Two REGS with consequent ids always goes together.
 *   - First REGS correspond to X, and the second REGS correspond to Y.
 * - Registration points are used to align/position shapes correctly when drawing
 * - Typically come in pairs: one resource for X offsets, another (ID+1) for Y offsets
 * - Offsets are subtracted from the target position: finalPos = pos - offset
 * 
 */
class ZmbRegs {
public:
	/**
	 * Array of registration point offsets. The index of the array corresponds to the shape index (1-based).
	 */
	Common::Array<Common::Point> _offsets;

	void parseStreams(MohawkEngine_Zoombini *vm, ZmbArchiveKind archiveKind, uint16 resIdX, uint16 resIdY);

	Common::Point getSubImageDelta(uint16 subImage) const;
	Common::Point getShapeDelta(uint16 shapeIdx) const;
	Common::Point getHotspotDelta(const ZmbHotspot &hotspot) const;

private:
	Common::Array<int16> parseStream(Common::SeekableReadStream *stream);
};

/**
 * Represents a hotspot in a feature script (SCRB).
 */
struct ZmbHotspot {
public:
	enum ReservedId: uint16 {
		kIndexNone = UINT16_MAX,
		kShapeNone = 0,
		kLengthAuto = 0,
		kDrawnRectVirtual = UINT16_MAX,
	};
	/**
	 * 0-based index of the hotspot.
	 */
	uint16 _hsId = kIndexNone;
	/**
	 * 1-based index of the shape.
	 */
	int16 _shapeIdx = kShapeNone;
	/**
	 * 0-based index of the frame.
	 */
	uint16 _frame = 0;
	/**
	 * X position of the hotspot.
	 */
	int16 _x = 0;
	/**
	 * Y position of the hotspot.
	 */
	int16 _y = 0;

	ZmbHotspot() { }
	/**
	 * Represents a physical hotspot entry from .MHK archive.
	 * @param hsId 0-based index
	 * @param shapeid 1-based shape index
	 * @param frame 0-based frame index
	 * @param x X position
	 * @param y Y position
	 */
	ZmbHotspot(uint16 hsId, int16 shapeid, uint16 frame, int16 x, int16 y) :
		_hsId(hsId), _shapeIdx(shapeid), _frame(frame), _x(x), _y(y) { }
	/**
	 * Represents a virtual hotspot entry from MapRects.
	 * @param hsId 0-based index
	 * @param shapeid 1-based shape index
	 * @param frame 0-based frame index
	 * @param rect Rectangle defining the hotspot area
	 */
	ZmbHotspot(uint16 hsId, int16 shapeid, uint16 frame, const Common::Rect &rect) :
		_hsId(hsId), _shapeIdx(shapeid), _frame(frame), _x(rect.left), _y(rect.top) { }
	
	/**
	 * Convert 1-based _shapeId to 0-based subImage id.
	 * @return 0-based subImage id
	 */
	uint16 getSubImageId() const { 
		return _shapeIdx - 1;
	}

	/**
	 * Get the position of the hotspot.
	 * @return Position of the hotspot
	 */
	Common::Point getPos() const {
		return Common::Point(_x, _y);
	}

	/**
	 * Compute hash function to uniquely identify a hotspot in a Feature.
	 * @return uint32 hash value
	 */
	uint32 hash() const {
		return hash(_frame, _hsId);
	}

	/**
	 * Compute hash function to uniquely identify a hotspot in a Feature.
	 * @return uint32 hash value
	 */
	static uint32 hash(uint16 frame, uint16 hsIdx) {
		return (frame << 16) + hsIdx;
	}
};

/**
 * Group of hotspots for a specific frame in a feature script (SCRB).
 */
struct ZmbHotspotGroup {
public:
	/**
	 * SCRB id.
	 */
	uint16 _scrbId = 0;
	/**
	 * Frame index. 0-based, -1 means no frame is selected.
	 */
	int32 _frameIdx = -1;

	ZmbHotspotGroup(uint16 scrbId, int32 frameIdx) :
		_scrbId(scrbId), _frameIdx(frameIdx) { }
	~ZmbHotspotGroup();

	/**
	 * Get a snapshot copy of all hotspots in this group.
	 * @return Array of hotspots
	 */
	Common::Array<ZmbHotspot> copyHotspots() { return _hotspots; }
	/**
	 * Get the number of shapes in this group.
	 * @return Number of hotspots
	 */
	uint32 getHotspotCount() const { return _hotspots.size(); }
	/**
	 * Get a hotspot by its 0-based index.
	 * @param hsId 0-based index of the hotspot
	 * @return The hotspot at the given index
	 */
	ZmbHotspot &getHotspot(uint32 hsId);
	/**
	 * Get a hotspot by its 0-based index.
	 * @param hsId 0-based index of the hotspot
	 * @return The hotspot at the given index
	 */
	ZmbHotspot &operator[](uint32 hsId);
	/**
	 * Append a hotspot to the group.
	 * @param hs The hotspot to append
	 */
	void appendHotspot(const ZmbHotspot &hs);
	/**
	 * Set hotspots to the group.
	 * @param hotspots Array of hotspots to set
	 */
	void setHotspots(const Common::Array<ZmbHotspot> &hotspots);
	/**
	 * Clear all hotspots from the group.
	 */
	void clear();

	void assignSoundRes(ZmbResource soundRes) { _soundRes = soundRes; }
	bool hasAssignedSoundRes() const { return _soundRes.hasId(); }
	ZmbResource getAssignedSoundRes() const { return _soundRes; }

	void assignEventCode(uint8 eventCode) { _eventCode = eventCode; }
	bool hasAssignedEventCode() const { return _eventCode != 0; }
	uint8 getAssignedEventCode() const { return _eventCode; }

	// Iterator
	typedef Common::Array<ZmbHotspot>::iterator ArrayIterator;
	typedef Common::Array<ZmbHotspot>::const_iterator ConstArrayIterator;
	ArrayIterator begin() { return _hotspots.begin(); }
	ArrayIterator end() { return _hotspots.end(); }
	ConstArrayIterator begin() const { return _hotspots.begin(); }
	ConstArrayIterator end() const { return _hotspots.end(); }

private:
	Common::Array<ZmbHotspot> _hotspots;
	ZmbResource _soundRes;
	uint8 _eventCode = 0;
};

class ZmbFeature;
class ZmbDrawRecord {
public:
	ZmbFeature *_scrb = nullptr;
	ZmbHotspotGroup *_hsGroup = nullptr;
	ZmbHotspot _hs;
	Common::Rect _drawnRect;

	ZmbDrawRecord() { }
	ZmbDrawRecord(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, const ZmbHotspot &hs, const Common::Rect &drawnRect) :
		_scrb(feature), _hsGroup(hsGroup), _hs(hs), _drawnRect(drawnRect) { }

	bool isEmpty() { return !_scrb || !_hsGroup; }
	bool isVirtualZone() { return _hs._frame == ZmbHotspot::kDrawnRectVirtual; }
};

struct ZmbPreparedRenderHotspot {
	ZmbHotspot _hotspot;
	ZmbResource _resource;
};

class ZmbFeature {
public:	
	enum Flag : uint32 {
		/**
		 * Simple static shapes.
		 * @remarks Mutually exclusive with FLAG_00000001_TYPE_SNOID and FLAG_00000002_TYPE_TOWN_ENTITY.
		 */
		FLAG_00000000_TYPE_SHAPES = 0x00000000,
		/**
		 * Zoombini entity.
		 * In render sorting, goes into a separate entity render list.
		 * @remarks Mutually exclusive with FLAG_00000000_TYPE_SHAPES and FLAG_00000002_TYPE_TOWN_ENTITY.
		 */
		FLAG_00000001_TYPE_SNOID = 0x00000001,
		/**
		 * Larger entity type.
		 * Sorted into the same render list as SNOID entities.
		 * @remarks Mutually exclusive with FLAG_00000000_TYPE_SHAPES and FLAG_00000001_TYPE_SNOID.
		 */
		FLAG_00000002_TYPE_TOWN_ENTITY = 0x00000002,
		/**
		 * Renders the feature last (topmost) in the render order.
		 * Features with this flag are appended to the end of the sorted render list.
		 * Also prevents automatic OVERLAY (0x4000000) force-set during render sorting.
		 * Used for buttons, dialogs, notification boxes, and page picker UI elements.
		 */
		FLAG_00001000_TOPMOST = 0x00001000,
		/**
		 * On registration, immediately triggers pre-render and records position.
		 * When CHAIN_SCRIPT chain completes, disables rendering.
		 */
		FLAG_00002000_DRAW_ON_REG = 0x00002000,
		/**
		 * When NOT set, getDrawnRect is called before rendering to allow rect invalidation.
		 */
		FLAG_00004000_NO_DIRTY_MERGE = 0x00004000,
		/**
		 * Loop animation shapes continuously.
		 * In the render pipeline, features with this flag stay in the animation runner list
		 * and are NOT sorted into the normal/overlay/entity render lists.
		 */
		FLAG_00008000_LOOP_ANIM = 0x00008000,
		/**
		 * One-time render skip. Skips rendering once, then auto-clears itself.
		 * Resets frame index to 0 and hotspot index to 1 when triggered.
		 */
		FLAG_00010000_SKIP_ONCE = 0x00010000,
		/**
		 * Persistent render skip. Sets doRender = false without clearing the flag.
		 */
		FLAG_00020000_SKIP_RENDER = 0x00020000,
		/**
		 * Reference other SCRB. Stores otherScriptId for chaining.
		 * At end-of-animation, loads the chained SCRB.
		 */
		FLAG_00040000_CHAIN_SCRIPT = 0x00040000,
		/**
		 * Animate the feature after some event is toggled.
		 * On initial load, rendering is disabled. Mainly used in easter eggs.
		 */
		FLAG_00080000_DEFER_ANIM = 0x00080000,
		/**
		 * Play-once animation. At end-of-animation, clears hotspot/shape data,
		 * and fires onHotspotShapeOrFrameFunc(-1) callback.
		 */
		FLAG_00100000_PLAY_ONCE = 0x00100000,
		/**
		 * Position delta mode. Copies hotspot pos to pos2 on load,
		 * then computes delta = posLoc - pos2 and applies to all hotspot positions.
		 */
		FLAG_00800000_POS_DELTA = 0x00800000,
		/**
		 * Render the feature after some event is toggled.
		 * On initial load, rendering is disabled.
		 */
		FLAG_01000000_DEFER_RENDER = 0x01000000,
		/**
		 * Select random frame when rendering.
		 * If combined with CHAIN_SCRIPT, clears RANDOM_FRAME and negates script ID on load.
		 */
		FLAG_02000000_RANDOM_FRAME = 0x02000000,
		/**
		 * Draw to the overlay screen instead of the shape screen.
		 * In render sorting, features with this flag go into the overlay render list.
		 * Automatically force-set on normal features that lack FLAG_00001000_TOPMOST.
		 */
		FLAG_04000000_OVERLAY = 0x04000000,
		/**
		 * Allocates an RgnR RMap instance (size 0x90) for per-shape bounding rect tracking.
		 */
		FLAG_08000000_REGION_TRACK = 0x08000000,
		/**
		 * Z-order sort protection: right edge.
		 * Prevents a feature from being sorted in front of existing features
		 * that extend further right. Features with this flag are appended to end
		 * of the render list and are not auto-assigned OVERLAY.
		 */
		FLAG_10000000_ZSORT_RIGHT = 0x10000000,
		/**
		 * Z-order sort protection: bottom edge.
		 * Prevents a feature from being sorted in front of existing features
		 * that extend further down.
		 */
		FLAG_20000000_ZSORT_BOTTOM = 0x20000000,
		/**
		 * Z-order sort protection: left edge.
		 * Prevents a feature from being sorted in front of existing features
		 * that extend further left.
		 */
		FLAG_40000000_ZSORT_LEFT = 0x40000000,
	};

	/**
	 * Represents a full SCRB, which is responsible for drawing shapes and executing event hooks.
	 * @param vm Pointer to the MohawkEngine_Zoombini instance
	 * @param scrbId The SCRB identifier
	 * @param frameInterval The frame interval
	 * @param flags Feature flags
	 * @param archiveKind The archive kind
	 * @param shapeImageId The shape image identifier
	 */
	ZmbFeature(MohawkEngine_Zoombini *vm, uint16 scrbId, uint32 frameInterval, uint32 flags, ZmbResource imgResource);
	/**
	 * Represents a full SCRB with a point of reference.
	 * @param vm Pointer to the MohawkEngine_Zoombini instance
	 * @param scrbId The SCRB identifier
	 * @param frameInterval The frame interval
	 * @param pointRef The point of reference
	 * @param flags Feature flags
	 * @param archiveKind The archive kind
	 * @param shapeImageId The shape image identifier
	 */
	ZmbFeature(MohawkEngine_Zoombini *vm, uint16 scrbId, uint32 frameInterval, const Common::Point &pointRef, uint32 flags, ZmbResource imgResource);
	/**
	 * Represents a virtual SCRB with shapeImageId assigned, which is used to draw MapRect shapes.
	 * @param vm Pointer to the MohawkEngine_Zoombini instance
	 * @param scrbId The SCRB identifier
	 * @param flags Feature flags
	 * @param archiveKind The archive kind
	 * @param shapeImageId The shape image identifier
	 */
	ZmbFeature(MohawkEngine_Zoombini *vm, uint16 scrbId, uint32 flags, ZmbResource imgResource);
	/**
	 * Represents an empty virtual SCRB, which is used to run event hooks.
	 * It also serves as a base constructor for ZmbSnoid.
	 * @param vm Pointer to the MohawkEngine_Zoombini instance
	 * @param scrbId The SCRB identifier
	 * @param flags Feature flags
	 */
	ZmbFeature(MohawkEngine_Zoombini *vm, uint16 scrbId, uint32 flags);
	virtual ~ZmbFeature();
public:

	void initValues();

	// ── Render callback typedefs ──────────────────────────────────────
	// These correspond to the 4 function pointers in the original engine's
	// CFeatureRunnerBase structure (see runner_registerAndAllocate 0x45F60C):
	//
	//   Original pPreRenderFunc (+0x0C)
	//     → _preRenderFunc: boolean gate hook that runs BEFORE the standard
	//       pre-render logic (preRenderFeature). Return false to skip standard
	//       logic entirely, modelling the original's "custom pre-render replaces
	//       runner_preRenderStandard" pattern.
	//     → _selectRenderFrameFunc: frame selection extracted from the standard
	//       pre-render logic. No separate original callback; the default
	//       selectRenderFrame() mirrors the original's integrated frame advance.
	//
	//   Original onPreRenderShapeFunc (+0x14)
	//     → _preRenderShapeFunc: called per-frame after hotspot data is parsed,
	//       before shape rendering. Direct equivalent.
	//
	//   Original pPostRenderFunc (+0x08)
	//     → _renderFunc: shape blitting (default: blitShapes). Direct equivalent
	//       of runner_postRenderStandard.
	//     → _postRenderFunc: additional processing after blit (ScummVM split).
	//
	//   Original onHotspotShapeOrFrameFunc (+0x10)
	//     → Modelled as the virtual method onFeatureAnimEvent() on ZoombiniPage,
	//       NOT as an EventHook. One-shot semantics (cleared after -1 fires)
	//       are tracked by _animEndCallbackFired.
	//
	typedef bool (ZoombiniPage::*OnPreRenderFunc)(ZmbFeature *feature);
	typedef int32 (ZoombiniPage::*OnSelectRenderFrameFunc)(ZmbFeature *feature);
	typedef void (ZoombiniPage::*OnPreRenderShapeFunc)(ZmbFeature *feature, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	typedef ZmbRenderResult (ZoombiniPage::*OnRenderFunc)(ZmbFeature *feature);
	typedef void (ZoombiniPage::*OnPostRenderFunc)(ZmbFeature *feature);

	// ── Input event callback typedefs ─────────────────────────────────
	// These have NO per-feature equivalent in the original engine. In the
	// original, mouse/keyboard events are dispatched centrally through each
	// puzzle's CPuzzleFuncTable (funcOnHover + funcOnKeyInput). ScummVM uses
	// per-feature dispatch for cleaner OOP design.
	//
	typedef ZmbEventHandleResult (ZoombiniPage::*OnMouseMoveFunc)(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	typedef ZmbEventHandleResult (ZoombiniPage::*OnLButtonDownFunc)(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	typedef ZmbEventHandleResult (ZoombiniPage::*OnLButtonUpFunc)(ZmbFeature *feature, const Common::Point &absPos, const Common::Point &relPos);
	typedef ZmbEventHandleResult (ZoombiniPage::*OnKeyDownFunc)(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);
	typedef ZmbEventHandleResult (ZoombiniPage::*OnKeyUpFunc)(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);
	typedef ZmbEventHandleResult (ZoombiniPage::*OnWheelUpFunc)(ZmbFeature *feature, const Common::Point &absPos);
	typedef ZmbEventHandleResult (ZoombiniPage::*OnWheelDownFunc)(ZmbFeature *feature, const Common::Point &absPos);

	/**
	 * Event hooks for the feature script.
	 *
	 * Render hooks map to the original engine's CFeatureRunnerBase callbacks
	 * (runner_registerAndAllocate 0x45F60C). Input event hooks are ScummVM
	 * extensions — the original dispatches input centrally via CPuzzleFuncTable.
	 */
	struct EventHooks {
		// ── Render hooks (original engine equivalents) ────────────────
		OnPreRenderFunc _preRenderFunc = nullptr;             ///< IDA: pPreRenderFunc (+0x0C) — boolean gate
		OnSelectRenderFrameFunc _selectRenderFrameFunc = nullptr; ///< Extracted from runner_preRenderStandard
		OnPreRenderShapeFunc _preRenderShapeFunc = nullptr;   ///< IDA: onPreRenderShapeFunc (+0x14)
		OnRenderFunc _renderFunc = nullptr;                   ///< IDA: pPostRenderFunc (+0x08) — shape blitting
		OnPostRenderFunc _postRenderFunc = nullptr;           ///< ScummVM split of pPostRenderFunc

		// ── Input event hooks (ScummVM extensions) ────────────────────
		OnMouseMoveFunc _mouseMoveFunc = nullptr;
		OnLButtonDownFunc _lButtonDownFunc = nullptr;
		OnLButtonUpFunc _lButtonUpFunc = nullptr;
		OnKeyDownFunc _keyDownFunc = nullptr;
		OnKeyUpFunc _keyUpFunc = nullptr;
		OnWheelUpFunc _wheelUpFunc = nullptr;
		OnWheelDownFunc _wheelDownFunc = nullptr;

		// ── Render hook setters ───────────────────────────────────────

		/**
		 * IDA: pPreRenderFunc (+0x0C).
		 * Boolean gate that runs before standard pre-render logic.
		 * Return false to skip preRenderFeature() entirely.
		 */
		void setPreRenderFunc(OnPreRenderFunc preRenderFunc) { _preRenderFunc = preRenderFunc; }
		/**
		 * Frame selection hook, extracted from runner_preRenderStandard.
		 * Default selectRenderFrame() mirrors the original integrated frame advance.
		 */
		void setSelectRenderFrameFunc(OnSelectRenderFrameFunc onSelectRenderFrameFunc) { _selectRenderFrameFunc = onSelectRenderFrameFunc; }
		/**
		 * IDA: onPreRenderShapeFunc (+0x14).
		 * Called per-frame after hotspot data is parsed, before shape rendering.
		 */
		void setPreRenderShapeFunc(OnPreRenderShapeFunc preRenderShapeFunc) { _preRenderShapeFunc = preRenderShapeFunc; }
		/**
		 * IDA: pPostRenderFunc (+0x08) — shape blitting.
		 * Default is blitShapes (runner_postRenderStandard equivalent).
		 */
		void setRenderFunc(OnRenderFunc renderFunc) { _renderFunc = renderFunc; }
		/**
		 * Additional post-render processing (ScummVM split of pPostRenderFunc).
		 * Called after renderFunc completes successfully.
		 */
		void setPostRenderFunc(OnPostRenderFunc postRenderFunc) { _postRenderFunc = postRenderFunc; }

		// ── Input event hook setters (ScummVM extensions) ─────────────

		void setMouseMoveFunc(OnMouseMoveFunc mouseMoveFunc) { _mouseMoveFunc = mouseMoveFunc; }
		void setLButtonDownFunc(OnLButtonDownFunc lButtonDownFunc) { _lButtonDownFunc = lButtonDownFunc; }
		void setLButtonUpFunc(OnLButtonUpFunc lButtonUpFunc) { _lButtonUpFunc = lButtonUpFunc; }
		void setKeyDownFunc(OnKeyDownFunc keyDownFunc) { _keyDownFunc = keyDownFunc; }
		void setKeyUpFunc(OnKeyUpFunc keyUpFunc) { _keyUpFunc = keyUpFunc; }
		void setWheelUpFunc(OnWheelUpFunc wheelUpFunc) { _wheelUpFunc = wheelUpFunc; }
		void setWheelDownFunc(OnWheelDownFunc wheelDownFunc) { _wheelDownFunc = wheelDownFunc; }
	};

	/**
	 * Set event hooks for the feature script.
	 * @param hooks The event hooks to set
	 */
	void setEventHooks(const EventHooks &hooks);

	/**
	 * Set the pre-render shape callback on an existing feature.
	 * IDA: runner->onPreRenderShapeFunc = funcPtr
	 * Used when a callback needs to be set after feature construction.
	 * @param func The pre-render shape callback
	 */
	void setPreRenderShapeFunc(OnPreRenderShapeFunc func) { _eventHooks._preRenderShapeFunc = func; }

	/**
	 * Pre-render pass: animation logic.
	 * IDA: runner_preRenderStandard (0x4619A1) — runs BEFORE Z-sort.
	 * Calls custom preRender callback, advances frame selection, handles
	 * end-of-cycle (CHAIN_SCRIPT, PLAY_ONCE), per-frame flag checks
	 * (SKIP_RENDER, SKIP_ONCE), and sound dispatch.
	 * @param page The page context
	 */
	void onPreRender(ZoombiniPage *page);

	/**
	 * Post-render pass: shape blitting + custom postRender callback.
	 * IDA: runner_postRenderStandard (0x46182F) — runs AFTER Z-sort.
	 * Blits shapes to screen and calls custom postRender callback.
	 * @param page The page to render to
	 * @return kRendered if shapes were drawn, kSkipped if render is deactivated
	 */
	ZmbRenderResult onPostRender(ZoombiniPage *page);

	/**
	 * Invoke the select-render-frame event hook.
	 * @param page The page to handle the event
	 * @return The frame index to render
	 */
	int32 onSelectRenderFrame(ZoombiniPage *page);
	/**
	 * Invoke the pre-render-shape event hook.
	 * @param page The page to handle the event
	 * @param hsGroup The hotspot group to handle the event
	 * @param hotspots The hotspots to handle the event
	 */
	void onPreRenderShape(ZoombiniPage *page, ZmbHotspotGroup *hsGroup, Common::Array<ZmbHotspot> &hotspots);
	/**
	 * Invoke the mouse move event hook.
	 * @param page The page to handle the event
	 */
	ZmbEventHandleResult onMouseMove(ZoombiniPage *page, const Common::Point &absPos, const Common::Point &relPos);
	/**
	 * Invoke the mouse left button down event hook.
	 * @param page The page to handle the event
	 */
	ZmbEventHandleResult onLButtonDown(ZoombiniPage *page, const Common::Point &absPos, const Common::Point &relPos);
	/**
	 * Invoke the mouse left button up event hook.
	 * @param page The page to handle the event
	 */
	ZmbEventHandleResult onLButtonUp(ZoombiniPage *page, const Common::Point &absPos, const Common::Point &relPos);
	/**
	 * Invoke the key down event hook.
	 * @param page The page to handle the event
	 */
	ZmbEventHandleResult onKeyDown(ZoombiniPage *page, const Common::KeyState &kbd, bool kbdRepeat);
	/**
	 * Invoke the key up event hook.
	 * @param page The page to handle the event
	 */
	ZmbEventHandleResult onKeyUp(ZoombiniPage *page, const Common::KeyState &kbd, bool kbdRepeat);
	/**
	 * Invoke the mouse wheel up event hook.
	 * @param page The page to handle the event
	 */
	ZmbEventHandleResult onWheelUp(ZoombiniPage *page, const Common::Point &absPos);
	/**
	 * Invoke the mouse wheel down event hook.
	 * @param page The page to handle the event
	 */
	ZmbEventHandleResult onWheelDown(ZoombiniPage *page, const Common::Point &absPos);

	/**
	 * Parse hotspot groups from a SCRB resource stream.
	 * @param stream The stream to read from
	 */
	void parseStream(Common::SeekableReadStream *stream);
	/**
	 * Set virtual hotspots for the virtual feature script.
	 * @param hotspots The hotspots to set
	 */
	void setVirtualHotspots(const Common::Array<ZmbHotspot> &hotspots);

	/**
	 * Swap the SCRB data on this feature, matching IDA scrb_loadOnRunner (0x460384).
	 *
	 * Clears existing hotspot data, parses new SCRB stream, resets animation state
	 * (frame index, sound index, render timers), and re-runs initValues().
	 * Preserves: identity (_id), flags, callbacks, position (unless POS_DELTA recalculates).
	 *
	 * @param stream               New SCRB resource stream (takes ownership; deleted internally)
	 * @param scheduleRender       If true, activates rendering after swap (IDA: wBoolScheduleRender=1)
	 */
	void loadScrbData(Common::SeekableReadStream *stream, bool scheduleRender = true);

	uint16 getId() const { return _id; }
	ZmbResource getResource() const { return _imgResource; }
	void setResource(ZmbResource res) { _imgResource = res; }

	/**
	 * Set per-tBMP REGS for shape registration-point offsets.
	 * IDA: runner_preRenderStandard 0x461F86 subtracts REGS[shapeId].
	 * @param regs Weak (non-owning) pointer to the REGS data
	 */
	void setShapeRegs(ZmbRegs *regs) { _shapeRegs = regs; }
	ZmbRegs *getShapeRegs() const { return _shapeRegs; }

	ZmbHotspotGroup *getHotspotGroup(int32 frameid);
	/**
	 * Look up the hotspot group for the exact frame index, without falling back
	 * to a previous non-empty frame. Returns nullptr if the frame has no entry.
	 */
	ZmbHotspotGroup *getHotspotGroupExact(int32 frameid) const;
	uint32 getHotspotTotalCount() const;
	uint16 getHotspotIdCount() const;

	uint32 getFrameCount() { return _hsFrameMap.size(); }
	int32 getMaxFrameIdx() { return _frameIdxMax; }
	/** Highest frame index that contains a positive-shape hotspot.
	 *  Used by `PLAY_ONCE` end-of-cycle to settle on a visible frame instead of
	 *  a trailing terminator-only frame. May differ from `getMaxFrameIdx()`
	 *  when SCRS/SCRB resources pad with empty terminator frames (e.g. Ferry
	 *  SCRS 1900 declares 25 frames but only frames 0-10 carry shapes). */
	int32 getLastShapeFrameIdx() const { return _lastShapeFrameIdx; }
	int32 getLastFrameIdx() const { return _lastFrameIdx; }
	void setLastFrameIdx(int32 idx) { _lastFrameIdx = idx; }
	int32 defaultSelectRenderFrame(uint32 currentFrameCounter);
	uint32 getFrameInterval() const { return _frameInterval; }
	void setFrameInterval(uint32 interval) { _frameInterval = interval; }
	Common::Point getPointLoc() const { return _pointLoc; }
	void setPointLoc(const Common::Point &pointLoc) { _pointLoc = pointLoc; }
	Common::Point getPosDelta() const;

	Common::Rect getClickRect() const { return _clickRect; }
	void setClickRect(const Common::Rect &rect) { 
		_hasClickRect = true; 
		_clickRect = rect;
	}
	bool hasClickRect() const { return _hasClickRect; }
	bool isPointInClickRect(const Common::Point &absPos) const {
		return _hasClickRect && _clickRect.contains(absPos);
	}

	void clear();

	ZmbDrawRecord *setDrawRecord(ZmbHotspotGroup *hsGroup, const ZmbHotspot &hs, const Common::Rect &drawnRect);
	ZmbDrawRecord *getDrawRecord(uint16 frame, uint16 hsIdx);
	void eraseDrawRecord(uint16 frame, uint16 hsIdx);
	void clearDrawRecords();
	bool hasDrawRecords() const { return !_drawnRecordMap.empty(); }
	void collectDrawRecordRects(Common::Array<Common::Rect> &rects) const;
	ZmbDrawRecord *findDrawRecordAtPoint(const Common::Point& absPos);
	void findDrawRecordsAtPoint(const Common::Point& absPos, Common::Array<ZmbDrawRecord*> &foundRecords);
	ZmbDrawRecord *findDrawRecordByHotspotIdx(uint16 hsIdx);
	ZmbDrawRecord *findDrawRecordByHotspotIdx(uint16 hsIdx1, uint16 hsIdx2);
	ZmbDrawRecord *findDrawRecordByHotspotIdx(Common::Array<uint16> hsIdxArr);
	ZmbDrawRecord *findDrawRecordByShapeId(uint16 shapeId);
	ZmbDrawRecord *findDrawRecordByShapeId(uint16 shapeId1, uint16 shapeId2);
	ZmbDrawRecord *findDrawRecordByShapeId(Common::Array<uint16> shapeIdArr);

	bool isCloseScheduled() const { return _isCloseScheduled; }
	void scheduleClose() { _isCloseScheduled = true; }

	void activateSubFeature();

	void activateRender() { _isRenderActivated = true; }
	void deactivateRender() { _isRenderActivated = false; }
	bool isRenderActivated() const { return _isRenderActivated; }
	void activateAnimate();
	void deactivateAnimate();
	void setSelectRenderFrameFunc(OnSelectRenderFrameFunc func);

	// IDA: chGetDrawnRect — dirty-rect tracking for the render pipeline.
	bool needsRedraw() const { return _needsRedraw; }
	void setNeedsRedraw(bool v) { _needsRedraw = v; }

	/**
	 * IDA one-shot callback state.  After the end-of-cycle -1 callback fires
	 * once (matching onHotspotShapeOrFrameFunc = 0 in the original), this flag
	 * suppresses further -1 dispatches until the next activateAnimate() call.
	 */
	bool hasAnimEndCallbackFired() const { return _animEndCallbackFired; }
	void markAnimEndCallbackFired() { _animEndCallbackFired = true; }
	uint32 getScrbLoadGeneration() const { return _scrbLoadGeneration; }

	/**
	 * IDA: wBoolDoRender[0] local in runner_preRenderStandard.  True when the
	 * dNextRenderFrame timing gate passed on this tick.  Set by
	 * defaultSelectRenderFrame(), used by preRenderFeature() to gate all
	 * animation processing (matching the original where the timing gate wraps
	 * the entire function body).  Custom selectRenderFrame hooks leave this
	 * as true (no timing gate — the hook drives frame advancement directly).
	 */
	bool isFrameTimingReady() const { return _frameTimingReady; }
	/** Reset dNextRenderFrame to 0 so the timing gate passes on the next tick. */
	void resetNextRenderFrame() { _nextRenderFrame = 0; }
	/**
	 * Check if this feature should be animated.
	 * @return True if this feature is being animated in animation frame
	 */
	bool isAnimateActivated() const;
	/**
	 * Schedule this feature to animate for specific frames.
	 */
	void scheduleAnimateForFrames(uint16 animateFrames);
	/**
	 * Check if an animation cycle is running based on the current frame counter.
	 * @param currentFrameCounter The current frame counter to check against
	 * @return True if an animation cycle is currently running, false otherwise
	 */
	bool isAnimationCycleRunning() const;
	bool isEndOfAnimationCycle() const;

	int32 getLastSoundedFrameIdx() const { return _lastSoundedFrameIdx; }
	void setLastSoundedFrameIdx(int32 idx) { _lastSoundedFrameIdx = idx; }

	// [*] Flags
	uint32 getFlags() const { return _flags; }
	bool hasFlag(Flag flag) const { return (_flags & flag) != 0; }
	void addFlag(Flag flag) { _flags |= static_cast<uint32>(flag); }
	void removeFlag(Flag flag) { _flags &= ~flag; }
	const Common::Rect &getSortRect() const { return _sortRect; }
	void setSortRect(const Common::Rect &rect) { _sortRect = rect; }

	uint32 getRegistrationIndex() const { return _registrationIndex; }
	void setRegistrationIndex(uint32 idx) { _registrationIndex = idx; }

	/**
	 * Get the rect used for Z-sorting and dirty invalidation. The original
	 * recomputes clickRect from current visual shapes each pre-render. ScummVM
	 * keeps manual click zones in _clickRect, so _sortRect carries the current
	 * visual bounds and falls back to _clickRect before the first draw.
	 */
	const Common::Rect &getZSortRect() const { return !_sortRect.isEmpty() ? _sortRect : _clickRect; }

	/**
	 * Set the SCRB ID to chain to at end-of-animation-cycle (CHAIN_SCRIPT).
	 * IDA: wOtherScriptId. 0 = no target. Negative = also set RANDOM_FRAME on load.
	 * Cleared automatically after the swap in preRenderFeature().
	 */
	void setChainedScrbId(int16 id) { _chainedScrbId = id; }
	int16 getChainedScrbId() const { return _chainedScrbId; }

	/**
	 * Link another subFeature to this one, to be used for FLAG_00040000_CHAIN_SCRIPT behaviour.
	 * The subFeature's lifetime is managed internally; this is an owning reference.
	 * @param sub The feature to link
	 */
	void setSubFeature(ZmbFeature *subFeature) { _refSubFeature = subFeature; }
	/**
	 * Get the feature linked via setSubFeature(), or nullptr if none.
	 */
	ZmbFeature *getSubFeature() const { return _refSubFeature; }
	/**
	 * Run the subFeature's render function if it exists.
	 * "Run" means registering the subFeature to the page's active feature list.
	 */
	void runSubFeature(ZoombiniPage *page);

	/**
	 * Returns true if this sub-feature has been registered to the page's active feature list via runSubFeature().
	 */
	bool isSubFeatureRunning() const { return _isSubFeatureRunning; }
	/**
	 * Set whether this sub-feature is currently registered in the page's active feature list.
	 */
	void setSubFeatureRunning(bool v) { _isSubFeatureRunning = v; }
	/**
	 * Schedule this sub-feature to be detached (removed from the page's feature list without being deleted).
	 * The parent feature retains ownership of the sub-feature.
	 */
	void scheduleDetach() { _isDetachScheduled = true; }
	/**
	 * Returns true if this sub-feature is scheduled to be detached from the page's feature list.
	 */
	bool isDetachScheduled() const { return _isDetachScheduled; }
	/**
	 * Clear the detach schedule flag after detachment has occurred.
	 */
	void clearDetach() { _isDetachScheduled = false; }

	// Hotspot group iteration
	typedef Common::HashMap<int32, ZmbHotspotGroup*>::iterator MapIterator;
	typedef Common::HashMap<int32, ZmbHotspotGroup*>::const_iterator ConstMapIterator;
	MapIterator begin() { return _hsFrameMap.begin(); }
	MapIterator find(int32 frameid) { return _hsFrameMap.find(frameid); }
	MapIterator end() { return _hsFrameMap.end(); }
	ConstMapIterator begin() const { return _hsFrameMap.begin(); }
	ConstMapIterator find(int32 frameid) const { return _hsFrameMap.find(frameid); }
	ConstMapIterator end() const { return _hsFrameMap.end(); }


protected:
	/**
	 * Parse hotspot frames from a stream, shared by SCRB and SCRS parsers.
	 * @param stream     The stream to read from (must be positioned at the first frame entry)
	 * @param frameCount Number of frames to read
	 */
	void parseFrames(Common::SeekableReadStream *stream, uint16 frameCount);

private:
	MohawkEngine_Zoombini *_vm;
	int16 _id = 0;

	/**
	 * SCRB ID to chain to at end-of-animation-cycle when FLAG_00040000_CHAIN_SCRIPT is set.
	 * IDA: wOtherScriptId. 0 = no chain target. Negative = set RANDOM_FRAME on load.
	 */
	int16 _chainedScrbId = 0;

	ZmbFeature *_refSubFeature = nullptr;

	/**
	 * True while this sub-feature is registered in the page's active scrb feature map via runSubFeature().
	 */
	bool _isSubFeatureRunning = false;
	/**
	 * True when this sub-feature should be removed from the page's feature map at the next checkCloseFeatures()
	 * without being deleted (the parent feature still owns the pointer).
	 */
	bool _isDetachScheduled = false;

	/**
	 * key: frame id, value: hotspot group for the frame
	 */
	Common::HashMap<int32, ZmbHotspotGroup*> _hsFrameMap;
	/**
	 * key: shape id, value: ZmbDrawRecord
	 */
	Common::StableMap<uint32, ZmbDrawRecord*> _drawnRecordMap;

	/**
	 * IDA: bHasClickRect at runner+0x2D.  Always 0 after scrb_loadOnRunner
	 * (0x4604D0 unconditionally clears it).  The initial-load path in
	 * runner_preRenderStandard (0x461B1E) that checks this field is therefore
	 * DEAD CODE in the original binary.
	 */
	Common::Rect _clickRect;
	bool _hasClickRect = false;

	// [*] Registered Informations
	/**
	 * Interval of frames between two consequent render timings.
	 */
	uint32 _frameInterval = 0;
	uint32 _flags = 0;
	uint32 _registrationIndex = 0;
	ZmbResource _imgResource;
	/**
	 * Per-tBMP REGS: shape registration-point offsets loaded for this feature's
	 * image resource. IDA: preRenderStandard (0x461F86) subtracts REGS[shapeId]
	 * from each hotspot position after onPreRenderShapeFunc callback.
	 * Weak (non-owning) pointer; lifetime managed by ZoombiniPage::_regsMap.
	 */
	ZmbRegs *_shapeRegs = nullptr;
	/**
	 * (FLAG_00800000_POS_DELTA or FLAG_00000001_TYPE_SNOID only)
	 * The position of the feature script, which can be changed when animating.
	 */
	Common::Point _pointLoc;
	/**
	 * (FLAG_00800000_POS_DELTA or FLAG_00000001_TYPE_SNOID only)
	 * The position set when the feature script is created, which can be used as a immutable reference.
	 */
	Common::Point _pointRef;

	// [*] Frame controls for animation
	int32 _lastFrameIdx = 0;
	int32 _frameIdxMax = 0;
	/** Highest frame containing a positive-shape hotspot - see `getLastShapeFrameIdx`. */
	int32 _lastShapeFrameIdx = 0;
	int32 _lastSoundedFrameIdx = -1;
	/**
	 * IDA: dNextRenderFrame — absolute frame counter at which the next
	 * animation advance is allowed.  Compared as _nextRenderFrame <= currentFrameCounter.
	 */
	uint32 _nextRenderFrame = 0;
	/**
	 * IDA: cUnk002E at runner+0x2E.  Set to 1 after SCRB load; causes the first
	 * frame advance to be skipped (holding frame 0 for one extra tick).  Cleared
	 * after the skip in defaultSelectRenderFrame().
	 */
	bool _skipFirstAdvance = false;
	/**
	 * IDA: wBoolDoRender[0] local in runner_preRenderStandard (0x461B0C).
	 * Result of the timing gate: dNextRenderFrame <= scrb_dwFrameRenderTime.
	 * In the original, this local gates the entire preRender body (end-of-cycle,
	 * frame advance, flag checks, hotspot walk, sound dispatch).  Without the
	 * paired hotspot slot system (wHotspotIdxToDraw / hotspot_renderPhaseArr),
	 * it reduces to the dNextRenderFrame timing check.
	 * Set by defaultSelectRenderFrame(), checked by preRenderFeature().
	 */
	bool _frameTimingReady = true;

	// [*] Z-sort rect: bounding box of all shapes drawn in the previous frame.
	// Updated at the end of each blitShapes() call; used by renderFeatures() for sorting.
	Common::Rect _sortRect;

	// [*] State controls
	bool _isCloseScheduled = false;
	bool _isAnimateActivated = false;
	bool _isRenderActivated = true;
	/**
	 * IDA: chGetDrawnRect.  Set during preRenderFeature() when the feature's
	 * animation advances (timing gate passes).  Cleared after the render loop
	 * in renderFeatures().  When set, the feature's visual coverage is merged
	 * into the dirty region and its shapes are redrawn.
	 */
	bool _needsRedraw = false;
	/**
	 * IDA one-shot callback: onHotspotShapeOrFrameFunc (runner offset 0x10) is
	 * cleared to 0 after the end-of-cycle -1 callback fires
	 * (runner_preRenderStandard 0x461F67 / 0x461D03).  In ScummVM the page
	 * virtual onFeatureAnimEvent() is always present, so this flag models the
	 * "pointer already consumed" state.  Reset on activateAnimate().
	 */
	bool _animEndCallbackFired = false;

	/**
	 * Generation counter incremented on each loadScrbData() call.
	 * Used by the PLAY_ONCE handler in preRenderFeature() to detect
	 * whether a new SCRB was loaded during the -1 callback, avoiding
	 * the stale markAnimEndCallbackFired() on the fresh animation.
	 */
	uint32 _scrbLoadGeneration = 0;

	// [*] Callbacks
	EventHooks _eventHooks;
};

/*
uint16BE  numFrames       // Number of animation frames (1–203)
uint16BE  variant         // Snoid script variant (see below)

// Repeated numFrames times — identical to SCRB entry format:
Frame[] {
    Entry[] {
        sint16BE  shapeId   // If >= 0: shape index. If < 0: frame terminator.
        sint16BE  x         // X position (only if shapeId >= 0)
        sint16BE  y         // Y position (only if shapeId >= 0)
    }
    // Terminator semantics (same convention as SCRB):
    //   0xFF00 (-256): plain end of frame (14124 occurrences)
    //   0xFFxx (-255 to -1): end of frame; low byte may carry metadata
    //   0xFExx and below (< -256): end of frame + read extra sint16BE sound res ID
}
*/

/*
### Variant field values

| Value | Meaning | Count | Found in |
|-------|---------|-------|----------|
| 0 | NORMAL | 442 | All directories |
| 1 | REJECT | 95 | FERRY, MAZE2, NET, TUNNELS, ZOOMBINI |
| 2 | (variant 2) | 131 | BRIDGE, CAVES, FERRY, HOTEL, MAZE2, NET, PIZZA, SLIDES, SMOKE, TUNNELS |
| 3 | (variant 3) | 59 | FLEENS only |
| 65535 | (special) | 2 | NET (1), SMOKE (1) |

1. **Variant is not just NORMAL/REJECT** — there are 5 distinct values `{0, 1, 2, 3, 0xFFFF}`, not just 2 as originally stated.

2. **Frame terminators go beyond `FF00`/`FE00`** — many `0xFFxx` terminators carry non-zero low bytes (e.g., `0xFF15`, `0xFFD6`, `0xFFC9`). These are puzzle/page-specific and likely encode additional frame metadata. The `0xFExx` terminators still follow the SCRB convention of reading an extra sint16 sound resource ID.

3. **Entries per frame vary**: 0, 1, 5, 6, 7, 9, 10, 13, or 20 entries per frame
*/

/**
 * Animation state for a ZmbSnoid (zoombini entity).
 * Derived from binary analysis of onRender_ZoombiniAnimation_452B9C.
 */
enum SnoidAnimState : uint8 {
	kSnoidAnimIdle = 0,           ///< Idle: periodically rolls fidget chance (10%)
	kSnoidAnimTurnRight = 1,      ///< Turn-around right: post-arrival facing flip (right→left), then idle
	kSnoidAnimTurnLeft = 2,       ///< Turn-around left: post-arrival facing flip (left→right), then idle
	kSnoidAnimFlip = 3,           ///< Flipping: swaps shape layers for 6 frames
	kSnoidAnimArrive = 4,         ///< Arriving: moves to target, then transitions to idle
	kSnoidAnimDrag = 5,           ///< Being dragged by cursor
	kSnoidAnimFidget = 6,         ///< Playing fidget animation
	kSnoidAnimDepart = 7,         ///< Departing: starts path-walking animation
	kSnoidAnimScriptReject = 8,   ///< Playing SCRS REJECT variant script
	kSnoidAnimScriptNormal = 9,   ///< Playing SCRS NORMAL variant script
	kSnoidAnimArrivalMotion = 10, ///< Arrival motion: moves to target with a specific animation
	kSnoidAnimPath = 112,         ///< Path-walking: following NODE waypoints toward destination
};

/**
 * Represents a SCRS, a Snoid Script resource.
 * SCRS has a similar format to SCRB but includes an extra variant field and is associated with ZmbSnoid traits and names.
 */
class ZmbSnoid : public ZmbFeature {
public:
	ZmbSnoid(MohawkEngine_Zoombini *vm, uint16 snoidId, uint32 flags);
	~ZmbSnoid() override;

	/**
	 * Parse hotspot groups from a SCRS (Snoid Script) resource stream.
	 * SCRS is identical to SCRB except for an extra uint16BE variant field
	 * immediately after the frame count.
	 * @param stream The stream to read from
	 */
	void parseScrsStream(Common::SeekableReadStream *stream);

	uint16 getVariant() const { return _variant; }

	// --- Animation state machine ---

	/**
	 * Set the snoid's animation state. Equivalent to IDA's animateZoombini_455E76.
	 * Resets frame counters and configures the animation for the given state.
	 * @param state The new animation state
	 * @param pos Optional position override (e.g. target position for walking)
	 */
	void setAnimState(SnoidAnimState state, const Common::Point *pos = nullptr);

	/**
	 * Tick the snoid animation state machine. Called once per animation frame
	 * during the pre-render phase. Equivalent to IDA's onRender_ZoombiniAnimation_452B9C.
	 * Updates position for walking states, handles fidget rolls on idle,
	 * and advances SCRS script playback.
	 * @param page The page owning this snoid (for frame counter, NODE access, etc.)
	 * @return true if the snoid's visuals changed and need re-rendering
	 */
	bool onSnoidAnimTick(ZoombiniPage *page);

	SnoidAnimState getAnimState() const { return _animState; }
	bool isFacingLeft() const { return _isFacingLeft; }
	void setFacingLeft(bool facingLeft) { _isFacingLeft = facingLeft; }

	/** Set holding animation frame: 0=middle, 1=left, 2=right. Used during drag. */
	void setHoldingFrameIdx(uint8 idx) { _holdingFrameIdx = CLIP<uint8>(idx, 0, 2); }
	uint8 getHoldingFrameIdx() const { return _holdingFrameIdx; }

	/** Set holding animation phase for feet cycling. IDA: wGroupFrameIdx0098. */
	void setHoldingAnimPhase(uint16 phase) { _holdingAnimPhase = phase; }
	uint16 getHoldingAnimPhase() const { return _holdingAnimPhase; }

	/**
	 * Build virtual hotspots for the generic common-image pose family.
	 * rawShape 1/2/3 correspond to SCRS 100/101/102 in zmbAnimHotspotArr.
	 */
	void setupCommonImageHotspots(uint16 rawShape, bool useSmallShapeRegs);

	/**
	 * Set up virtual hotspots for the idle pose based on the snoid's current traits.
	 * Uses the IDA-reversed lookup tables (foot/nose/eye/head × 5 trait values).
	 * Call this after setting _trait, before the first render frame.
	 * rawShape=2 corresponds to SCRS_101 (index 1), which is the idle right-facing pose
	 * selected by animateZoombini_455E76 for wAnimKind==3 (idle).
	 */
	void setupIdleHotspots();

	/**
	 * Build virtual hotspots for the SMALL-scale idle pose (XFER FromIsle scene).
	 * Uses the compact body-part index tables from the original binary (word_4A48B8..DC),
	 * paired with the small-snoid SHPL resource 3200 (0xC80 in System/Common MHK).
	 * IDA: sub_4572C5(0) swaps wArrZmbBody* tables to these values before snoid load.
	 * Also sets _useSmallShapeRegs = true so the renderer uses the small REGS offsets.
	 */
	void setupSmallIdleHotspots();

	/**
	 * Rebuild hotspots for the current generic common-image pose.
	 * Used by arrival and turn states, which keep the current _shapeImageIdx.
	 */
	void setupCurrentCommonImageHotspots();

	/** True when this snoid uses the small-scale SHPL/REGS (resource 3200, XFER_0 only). */
	bool _useSmallShapeRegs = false;

	/**
	 * Start SCRS script playback on this snoid.
	 * Parses the SCRS stream, computes the pos2-based anchor offset, configures
	 * the rendering to use per-tick frame advancement (states 8/9), and sets
	 * the select-render-frame hook to selectScrsRenderFrame.
	 *
	 * IDA: snoidScript_initAndPlay_455C0D equivalent.
	 *
	 * @param scrsStream      SCRS resource stream (ownership transferred; deleted internally).
	 * @param hideOnComplete  IDA chRand_64_0: when true, the snoid is hidden (render deactivated)
	 *                        after the SCRS finishes instead of reverting to idle.
	 * @param rejectState     True for REJECT script (state 8), false for NORMAL (state 9).
	 * @param initPos         Optional anchor override (IDA pInitPos). When non-null, scans
	 *                        SCRS frame groups from the end for the last group with a
	 *                        positive-shape anchor and aligns that frame to *initPos.
	 *                        Used by Ferry's reject-flight landing SCRS so the animation
	 *                        ENDS at the landing target (rather than starting there).
	 *                        IDA: snoidScript_initAndPlay_455C0D pInitPos parameter,
	 *                        scan loop at 0x455D44-0x455DA1.
	 */
	void startScrsPlayback(Common::SeekableReadStream *scrsStream, bool hideOnComplete,
	                       bool rejectState = true,
	                       const Common::Point *initPos = nullptr);

	/**
	 * Clean up after SCRS playback finishes: restore pointLoc and clear the
	 * select-render-frame hook. Called before setAnimState(kSnoidAnimIdle).
	 */
	void finishScrsPlayback();

	/**
	 * Return the translation applied to raw SCRS hotspot coordinates while rendering.
	 * IDA stores this separately as -pos2 while posLoc tracks the current visible root.
	 */
	Common::Point getScrsRenderOffset() const {
		if (_animState == kSnoidAnimScriptReject || _animState == kSnoidAnimScriptNormal)
			return _scrsRenderOffset;
		return getPointLoc();
	}

	void setPreparedRenderHotspots(const Common::Array<ZmbPreparedRenderHotspot> &hotspots) {
		_preparedRenderHotspots = hotspots;
	}
	const Common::Array<ZmbPreparedRenderHotspot> &getPreparedRenderHotspots() const { return _preparedRenderHotspots; }
	bool hasPreparedRenderHotspots() const { return !_preparedRenderHotspots.empty(); }
	void clearPreparedRenderHotspots() { _preparedRenderHotspots.clear(); }

	/**
	 * Returns true if the hotspot data was synthesised by setupIdleHotspots() and already
	 * contains the combined (traitTableOffset + rawShape) value in _shapeIdx.
	 * Returns false for SCRS-parsed snoids whose _shapeIdx is just rawShapeFromData and
	 * still requires getBodyLayerBaseOffset() to be added before the mirror formula.
	 */
	bool hasCombinedShapeIndices() const { return _usesVirtualHotspots; }

	/**
	 * Returns the IDA wArrAnimShape00C4[layer] value for the given layer slot,
	 * i.e. the trait-based base offset to add to rawShapeFromData for SCRS-loaded snoids.
	 * Uses the wAnimKind=0 (front-face) body layout from zmbRunner_setAnimShape_456785:
	 *   layer 0 = foot, layer 1 = body-center (0), layer 2 = nose, layer 3 = eye, layer 4 = head
	 *
	 * For NORMAL scripts (variant 0 / animKind 9), uses the NORMAL-specific trait tables
	 * (IDA: wArrZmbBody*_4A47A0-4A47C4) instead of the general tables (4A4770-4A4794).
	 *
	 * @param layer      0-based body-part layer index (matches hs._hsId)
	 * @param layerShift IDA p_wUnk00C2 shift: when the first SCRS frame shape > 18 in
	 *                   a NORMAL script, the trait array is shifted by 1, so layer 0 gets
	 *                   no trait offset and layers 1-5 map to the original slots 0-4.
	 */
	int16 getBodyLayerBaseOffset(uint8 layer, uint8 layerShift = 0) const;

	/**
	 * Compute the voice SFX resource ID for this snoid, given a voice group index.
	 * Implements IDA's getZoombiniVoiceResId_456FCB.
	 *
	 * @param voiceGroup Voice group 0-17 (mapped from SCRS event codes 200-217).
	 * @return SND resource ID to play, or 0 if suppressed.
	 */
	int16 getVoiceResId(int16 voiceGroup) const;

	/**
	 * Update virtual hotspots for the current walk animation phase using live SCRS data.
	 * Selects the correct directional SCRS (from SCRS 105–129) for this snoid's foot type
	 * and the current movement direction bucket, then applies trait base offsets.
	 * @param page      The owning page (provides walk SCRS cache via getWalkAnim).
	 * @param dirBucket Direction bucket 0–4 (from movement slope: 0=steeply down, 4=steeply up).
	 * @param phase     Raw walk phase counter (wrapped with % frameCount inside).
	 */
	void updateWalkHotspots(ZoombiniPage *page, int dirBucket, int phase);

	/**
	 * Update virtual hotspots for the current fidget animation frame using SCRS data.
	 * Selects from SCRS 130–136 (set 0) or SCRS 138–144 (set 1) for this snoid's traits.
	 * Empty frames (entryCount==0) are skipped to preserve the current pose.
	 * @param page      The owning page (provides fidget SCRS cache via getFidgetAnim).
	 * @param fidgetSet 0 = normal (chZmbAnimShapeCommonImageIdx=1), 1 = flipped (=2).
	 * @param variant   Random variant 0–6 (wAnimBaseFlag00F5).
	 * @param frameIdx  Current animation frame index (0-based, not wrapped here).
	 */
	void updateFidgetHotspots(ZoombiniPage *page, int fidgetSet, int variant, int frameIdx);

	/**
	 * Update virtual hotspots for the holding (drag) animation using SCRS data.
	 * Selects from SCRS 146–150 (one per foot type 1–5) based on this snoid's foot trait.
	 * IDA: Case 5 in animateZoombini_455E76 uses wAnimHotspotSetIdx = footTrait + 45.
	 * Frame is selected by _holdingAnimPhase (cycles through all frames; see onSnoidAnimTick).
	 * @param page The owning page (provides holding SCRS cache via getHoldingAnim).
	 */
	void updateHoldingHotspots(ZoombiniPage *page);

	Common::Point getAnimTargetPos() const { return _animTargetPos; }
	void setAnimTargetPos(const Common::Point &pos) { _animTargetPos = pos; }

	int16 getAnimSpeedX() const { return _animSpeedX; }
	int16 getAnimSpeedY() const { return _animSpeedY; }
	void setAnimSpeed(int16 speedX, int16 speedY) { _animSpeedX = speedX; _animSpeedY = speedY; }

	/**
	 * Set up a straight-line walk to the given target position.
	 * Sets _animTargetPos and enters kSnoidAnimDepart state, which will
	 * initialise waypoint routing (or walk straight if no NODE data)
	 * with dynamic speed from snoidPath_stepAndComputeVelocity_4548DF.
	 *
	 * IDA equivalent: animateZoombini(0, 7, core) sets DEPARTING.
	 *
	 * @param target The destination position to walk toward.
	 */
	void initWalkToTarget(const Common::Point &target);

	/** Set a deferred-start frame (IDA: CFeatureRunner::dNextRenderFrame). See _delayUntilFrame. */
	void setDelayUntilFrame(uint32 frame) { _delayUntilFrame = frame; }

	/**
	 * Override the body-part arrangement (IDA: wBodyArrangementKind, +0xC0).
	 * Changes which trait tables map to the 5 layer slots in getBodyLayerBaseOffset().
	 * Values: 0=front (foot,body,nose,eye,head), 1=left (foot,nose,body,eye,head),
	 *         2=right (body,eye,nose,foot,head).
	 * IDA: zmbRunner_setAnimShape_456785(wAnimKind, pZmb).
	 */
	void setBodyArrangement(uint16 arrangement) { _variant = arrangement; }
	uint16 getBodyArrangement() const { return _variant; }

	/**
	 * Per-snoid SCRS animation cycle counter. Incremented by the page's
	 * onFeatureAnimEvent when event code 0 fires (visibility toggle).
	 * IDA: *(callbackData + 288) — reuses chPathWalkDir byte during SCRS playback.
	 */
	uint8 _scrsAnimCycleCount = 0;

	ZmbTrait _trait;
	Common::U32String _name;

	/**
	 * IDA: unk00F7. Tracks whether this snoid occupies a pedestal slot (true)
	 * or is a non-occupied entry sitting at an arbitrary position (false).
	 * Set during loadZoombinisFromPack; used by saveSnoidsToPack to rebuild
	 * the active pack with the correct bIsOccupied flags.
	 */
	bool _packIsOccupied = false;

	/**
	 * IDA snoid runner status byte at offset +295 (`*((BYTE*)snoid+295)`):
	 *   0 = idle / draggable
	 *   1 = reject-walk (snoid is animating a reject return; cannot be re-grabbed)
	 *   2 = arrived (snoid finished crossing; cannot be re-dragged on Bridge)
	 *   3-4 = puzzle-specific transient states
	 *   8 = reserved (Bridge `pcStr1[8] != 8` guard — used as "not yet engaged")
	 *
	 * The byte gates drag operations: onLButtonDown checks this to refuse
	 * drag attempts on snoids in transient/non-idle states.
	 */
	uint8 _runnerStatus = 0;

private:
	bool advancePathSubTarget(ZoombiniPage *page, bool forceHotspotUpdate = false);
	void syncScrsPointLoc();

	MohawkEngine_Zoombini *_vm;
	int16 _id = 0;

	/**
	 * SCRS variant field. Known values:
	 * 0 = NORMAL, 1 = REJECT, 2/3/0xFFFF = unknown
	 */
	uint16 _variant = 0;

	// --- Animation state fields ---
	SnoidAnimState _animState = kSnoidAnimIdle;
	bool _isFacingLeft = false;

	/** Target position for walking/arriving animations. */
	Common::Point _animTargetPos;
	/** Walk speed per tick (X component). */
	int16 _animSpeedX = 0;
	/** Walk speed per tick (Y component). */
	int16 _animSpeedY = 0;
	/** Counter for idle fidget timing (incremented each idle tick). */
	uint8 _idleTickCounter = 0;
	/**
	 * IDA: wAnimBaseFlag00F5 idle sub-purpose. Set to true when entering
	 * idle state (via setAnimState); cleared and redraw-marked on the first
	 * idle animation tick. This is separate from _fidgetValue which also
	 * maps to wAnimBaseFlag00F5 during fidget triggering.
	 */
	bool _needsIdleRedraw = false;
	/** Counter for flip animation (0..6, swaps layers each tick). */
	uint16 _flipCounter = 0;
	/**
	 * Shadow shape IDs for flip animation (IDA: hsArr[10..14]).
	 * Computed from trait-specific shape categories (425+head, 430+eye, 435+foot, 440+nose)
	 * on flip entry, then swapped with main hotspot shapes each tick for 6 ticks.
	 */
	int16 _flipShadowShapes[5] = {0, 0, 0, 0, 0};
	/** Random fidget variant value (0–6, maps to wAnimBaseFlag00F5; selects SCRS 130–136 or 138–144). */
	uint16 _fidgetValue = 0;
	/**
	 * IDA chZmbAnimShapeCommonImageIdx (*(a2+293)): tracks animation image state.
	 * 0 = freshly-entered idle (cleared each idle tick), 1 = normal animated state,
	 * 2 = flipped/variant state (set by kSnoidAnimFlip when implemented).
	 * Determines which fidget set is used: 1→set A (SCRS 130–136), 2→set B (SCRS 138–144).
	 */
	uint8 _shapeImageIdx = 0;
	/**
	 * Holding animation frame index during drag: 0=middle, 1=left, 2=right.
	 * Updated by onMouseMove based on movement direction.
	 * IDA: Frame selection in drag state uses chZmbAnimShapeCommonImageIdx (0→0, 1→1, 2→2).
	 */
	uint8 _holdingFrameIdx = 0;
	/**
	 * Holding animation phase counter for feet cycling animation.
	 * IDA: wGroupFrameIdx0098 advances each tick in onRender_ZoombiniAnimation case 5.
	 * When phase >= frameCount, resets to 2 and loops (or 0 for small snoid mode).
	 * This creates the "dangling feet" animation while snoid is held.
	 */
	uint16 _holdingAnimPhase = 0;
	/** Current NODE/PATH route index. IDA: chPathRouteIdx. */
	int16 _pathRouteIdx = -1;
	/** Next PATH slot to read. IDA: chPathSlotIdx. */
	int16 _pathSlotIdx = -1;
	/** PATH slot increment, +1 or -1. IDA: chPathWalkDir. */
	int16 _pathWalkDir = 1;
	/** Current checkpoint or final destination. IDA: pos2. */
	Common::Point _pathSubTarget;
	/**
	 * True when hotspot _shapeIdx values already include the trait-base offset
	 * (set by setupIdleHotspots / updateWalkHotspots). False for SCRS-parsed snoids.
	 */
	bool _usesVirtualHotspots = false;
	/** Walk animation cycle phase (raw counter, wrapped by % frameCount inside updateWalkHotspots). */
	int _walkPhase = 0;
	/** Current walk direction bucket 0–4 (0=slope-down, 2=horizontal, 4=slope-up). */
	int _walkDirBucket = 2;
	/**
	 * Saved _pointLoc before SCRS playback began. Restored by finishScrsPlayback()
	 * so the snoid reappears at its original position when reverting to idle.
	 */
	Common::Point _scrsOrigPointLoc;
	/**
	 * Translation added to raw SCRS hotspot coordinates while rendering.
	 * IDA: -pos2. Kept separate from pointLoc because scripted frames update
	 * pointLoc to the current visible root before callbacks are dispatched.
	 */
	Common::Point _scrsRenderOffset;
	/**
	 * IDA chRand_64_0: when true, the snoid is hidden (render deactivated) after
	 * states 8/9 SCRS playback finishes. Set by startScrsPlayback from the
	 * hideOnComplete parameter. Used by XFER to make celebration-animated snoids
	 * disappear after their scripts complete.
	 */
	bool _scrsHideOnComplete = false;
	Common::Array<ZmbPreparedRenderHotspot> _preparedRenderHotspots;
	/**
	 * Time-based animation deadline (IDA: CFeatureRunner307::dNextRenderFrame).
	 * Animation fires when page->getCurrentFrameCounter() >= _nextAnimFrame,
	 * then sets _nextAnimFrame = currentFrameCounter + kAnimInterval (=6).
	 *
	 * IDA: dFrameInterval = 6 (registerVirtualScrbZoombiniAnimation_452A64), and the
	 * original checks dNextRenderFrame <= scrb_dwFrameRenderTime (= getMillis()/17).
	 * This makes the animation timer wall-clock-based: the interval is always
	 * 6 * 17ms = 102ms of real time, regardless of actual render frame rate.
	 *
	 * Initialized to 0 so the first tick fires immediately, matching the original's
	 * dNextRenderFrame = 0 set in zmb_registerSnoidFeatureRunner.
	 */
	uint32 _nextAnimFrame = 0;
	/**
	 * Deferred start frame counter (IDA: CFeatureRunner::dNextRenderFrame).
	 * When non-zero, ticking and rendering are suppressed until
	 * page->getCurrentFrameCounter() >= _delayUntilFrame, then it is cleared.
	 * Used by the SHIFT+dice "generate all" path to stagger Zoombini walk entries.
	 */
	uint32 _delayUntilFrame = 0;
};

} // End of namespace Mohawk

#endif
