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

#ifndef MOHAWK_ZOOMBINI_PAGES_DIALOG_DEBUG_H
#define MOHAWK_ZOOMBINI_PAGES_DIALOG_DEBUG_H

#include "mohawk/zoombini_pages/dialog_base.h"
#include "mohawk/zoombini_debug.h"

namespace Mohawk {

/**
 * ScummVM-specific dialog page for display debugged information of Zoombinis.
 */
class ZoombiniDialogDebug : public ZoombiniDialog {
public:
	ZoombiniDialogDebug(MohawkEngine_Zoombini *vm, const ZoombiniDebugCommand &cmd);
	~ZoombiniDialogDebug() override;

	void setBackgroundBitmap() override;
	void loadFeatures() override;
	void close() override;

	virtual ZmbEventHandleResult onLButtonDown(const Common::Point &absPos, const Common::Point &relPos);

protected:
	ZoombiniDebugCommand _cmd;
	bool _updateScreen = true;

	const Common::Rect _titleRect = Common::Rect(0, 0, ZoombiniGraphics::kScreenWidth, 14);
	const Common::U32String _escText = Common::U32String("[ESC] close");
	/**
	 * Print top left message
	 */
	void drawTitleText(ZmbFeature *feature, const Common::U32String &titleText);
	/**
	 * Print top right message
	 */
	void drawEscText(ZmbFeature *feature, const Common::U32String &keyLegendText = Common::U32String());

	// [*] drawCursor
	ZmbRenderResult drawCursor_render(ZmbFeature *feature);

	// [*] drawImage
	ZmbRenderResult drawImage_render(ZmbFeature *feature);

	// [*] drawShape
	ZmbRenderResult drawShape_render(ZmbFeature *feature);
	ZmbEventHandleResult common_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);

	// [*] drawShapes
	ZmbRenderResult drawShapes_render(ZmbFeature *feature);
	ZmbEventHandleResult drawShapes_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);
	Common::Stack<uint16> _drawShapesPrevShapeIdxStack;
	uint16 _drawShapesNextShapeIdx = 0;

	// [*] drawFeature
	ZmbRenderResult drawFeature_render(ZmbFeature *feature);
	ZmbEventHandleResult drawFeature_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat);
	Common::Stack<uint16> _drawFeaturePrevHsIdxStack;
	uint16 _drawFeatureNextHsIdx = 1;
	uint16 _drawFeatureFrame = 0;

	// [*] plotPoint
	ZmbRenderResult plotPoint_render(ZmbFeature *feature);

	// [*] plotLine
	ZmbRenderResult plotLine_render(ZmbFeature *feature);

	// [*] plotRect
	ZmbRenderResult plotRect_render(ZmbFeature *feature);

	enum MultiScreenOperation {
		kMultiScreenOpNone = 0,
		kMultiScreenOpInit,
		kMultiScreenOpPrev,
		kMultiScreenOpNext,
		kMultiScreenOpUp,
		kMultiScreenOpDown,
	};
	MultiScreenOperation _multiScreenNextOp = kMultiScreenOpInit;

};

} // End of namespace Mohawk

#endif
