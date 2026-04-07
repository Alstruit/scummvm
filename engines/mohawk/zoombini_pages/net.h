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

#ifndef MOHAWK_ZOOMBINI_PAGES_NET_H
#define MOHAWK_ZOOMBINI_PAGES_NET_H

#include "mohawk/zoombini_pages/interactive_base.h"

namespace Mohawk {

/**
 * Net puzzle page (ZoombiniPageType::kNet).
 *
 * Route 4, Puzzle 3: Zoombinis must navigate a sorting net with
 * attribute-based column filters. The puzzle uses 2-3 columns
 * depending on difficulty, with animated column slots.
 *
 * IDA entry: puzzleNet_4361D4 (0x4361d4)
 */
class ZoombiniInteractiveNet : public ZoombiniInteractive {
public:
	ZoombiniInteractiveNet(MohawkEngine_Zoombini *vm);
	~ZoombiniInteractiveNet() override;

	void open() override;
	void setBackgroundMusic() override;
	void setBackgroundBitmap() override;
	void loadFeatures() override;

protected:
	void onGoButtonActivated() override;
	void onFeatureAnimEvent(ZmbFeature *feature, int16 eventCode) override;
	void onEveryFrame() override;

private:
	void loadZoombinisFromPack();
	void registerColumnRunners();

	// Animation event dispatch helpers
	// IDA: net_scrbAnimCallback (0x43105B) — SCRB feature traversal events
	void processScrbAnimEvent(ZmbFeature *feature, int16 eventCode);
	// IDA: net_zmbAnimCallback (0x438EA1) — Zoombini snoid animation events
	void processSnoidAnimEvent(ZmbFeature *feature, int16 eventCode);

	// Attribute slot render callbacks
	bool attrSlots_preRender(ZmbFeature *feature);
	ZmbRenderResult attrSlots_render(ZmbFeature *feature);

	static const Common::Point kSnoidPositions[16];

	/** Route difficulty level (0-3). IDA: net_difficultyLevel */
	int16 _difficultyLevel = 0;

	// --- Puzzle state (IDA: puzzleNet_4361D4 globals) ---
	
	/** Total slots: 25 (diff<=1) or 125 (diff>1). IDA: net_totalSlotCount */
	int16 _totalSlotCount = 25;
	
	/** Number of columns: 2 (diff<=1) or 3 (diff>1). IDA: net_columnCount */
	int16 _columnCount = 2;
	
	/** Whether "Go" button should be enabled. IDA: net_advanceReady */
	bool _bAdvanceReady = false;
	
	/** Random attr column offsets (0-4). IDA: net_randAttrColOffset0/1/2 */
	int16 _randAttrColOffset[3] = {0, 0, 0};
	
	/** Previous attr column offsets. IDA: net_prevAttrColOffset0/1/2 */
	int16 _prevAttrColOffset[3] = {-1, -1, -1};

	// --- Feature runners ---
	
	/** Column SCRB runners (5 entries). IDA: net_columnScrbRunners */
	ZmbFeature *_columnScrbFeatures[5] = {};
	
	/** Entry SCRB runner. IDA: net_entryScrbRunner */
	ZmbFeature *_entryScrbFeature = nullptr;
	
	/** Label SCRB runner. IDA: net_labelScrbRunner */
	ZmbFeature *_labelScrbFeature = nullptr;
	
	/** Attribute animation SCRB runner. IDA: net_attrAnimScrbRunner */
	ZmbFeature *_attrAnimScrbFeature = nullptr;
	
	/** Feedback SCRB runner. IDA: net_feedbackScrbRunner */
	ZmbFeature *_feedbackScrbFeature = nullptr;
	
	/** Attribute column SCRB runners (3 entries). IDA: net_attrCol0/1/2ScrbRunner */
	ZmbFeature *_attrColScrbFeatures[3] = {};
	
	/** Exit SCRB runner. IDA: net_exitScrbRunner */
	ZmbFeature *_exitScrbFeature = nullptr;

	// --- Internal dirty flags for attr slot rendering ---
	
	/** Dirty flag for advance button rect. IDA: unk_4A28AC */
	bool _advanceButtonDirty = false;
	
	/** Dirty flag for column label rect. IDA: unk_4A28AE */
	bool _columnLabelDirty = false;

	// --- Animation event state ---

	/** Pending body arrangement shape (1-4, or 0 for none). IDA: net_pendingAnimShape */
	int16 _pendingBodyArrangement = 0;

	// Celebration state (IDA: net_onFrameTick @ 0x43728B)
	/** Idle animation trigger flag. IDA: net_idleAnimTrigger */
	bool _idleAnimTrigger = false;
	/** Number of idle animations played. IDA: net_idleAnimCount */
	int16 _idleAnimCount = 0;
	/** Target idle animation count. IDA: net_idleAnimMax */
	int16 _idleAnimMax = 0;
	/** Non-repeat random pool bitmask. IDA: dword_4B0B44 */
	uint32 _idleAnimPoolState = 0;
	/** Frame counter of last idle animation. IDA: dword_4B0B3C */
	uint32 _idleAnimLastFrame = 0;
	/** Number of loaded Zoombinis (pool size). IDA: net_zoombiniCount */
	int16 _loadedZmbCount = 0;
	/** Whether current round is complete (allows celebration on "locked" snoids). IDA: net_roundCompletedFlag */
	bool _roundCompletedFlag = false;

	enum {
		kResSound996_DepartSFX = 996
	};
};

} // End of namespace Mohawk

#endif
