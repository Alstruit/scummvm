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
#include "mohawk/mohawk.h"
#include "mohawk/sound.h"
#include "mohawk/video.h"

#include "mohawk/zoombini.h"
#include "mohawk/zoombini_graphics.h"
#include "mohawk/zoombini_pages/dialog_debug.h"
#include "mohawk/zoombini_random.h"
#include "mohawk/zoombini_sound.h"
#include "mohawk/zoombini_state.h"

namespace Mohawk {

ZoombiniDialogDebug::ZoombiniDialogDebug(MohawkEngine_Zoombini *vm, const ZoombiniDebugCommand &cmd) : ZoombiniDialog(vm, ZoombiniPageType::kDialogDebug), _cmd(cmd) {
}

ZoombiniDialogDebug::~ZoombiniDialogDebug() {
}

void ZoombiniDialogDebug::setBackgroundBitmap() {
	Graphics::Surface *shapeScreen = _vm->_gfx->getShapeScreen();
	shapeScreen->fillRect(shapeScreen->getRect(), ZoombiniGraphics::kColor0A_White);
}

void ZoombiniDialogDebug::loadFeatures() {
	switch (_cmd._type) {
	case ZoombiniDebugCommand::kDrawCursor: { // [*] Virtual Feature - drawCursor
		ZmbFeature::EventHooks hooksDrawCursor;
		hooksDrawCursor.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniDialogDebug::drawCursor_render));

		loadVirtualFeature(kVirtualFeatureDrawImage, 0,
					   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						   hooksDrawCursor);
		break;
	}
	case ZoombiniDebugCommand::kDrawImage: { // [*] Virtual Feature - drawImage
		ZmbFeature::EventHooks hooksDrawImage;
		hooksDrawImage.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniDialogDebug::drawImage_render));
		hooksDrawImage.setKeyDownFunc(reinterpret_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogDebug::common_onKeyDown));

		loadVirtualFeature(kVirtualFeatureDrawImage, 0,
					   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						   hooksDrawImage);
		break;
	}
	case ZoombiniDebugCommand::kDrawShape: { // [*] Virtual Feature - drawShape
		ZmbFeature::EventHooks hooksDrawShape;
		hooksDrawShape.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniDialogDebug::drawShape_render));
		hooksDrawShape.setKeyDownFunc(reinterpret_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogDebug::common_onKeyDown));

		loadVirtualFeature(kVirtualFeatureDrawShape, 0,
					   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						   hooksDrawShape);
		break;
	}
	case ZoombiniDebugCommand::kDrawShapes: { // [*] Virtual Feature - drawShapes
		ZmbFeature::EventHooks hooksDrawShapes;
		hooksDrawShapes.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniDialogDebug::drawShapes_render));
		hooksDrawShapes.setKeyDownFunc(reinterpret_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogDebug::drawShapes_onKeyDown));

		loadVirtualFeature(kVirtualFeatureDrawShapes, 0,
						   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						   hooksDrawShapes);

		if (1 < _cmd._subId)
			_drawShapesPrevShapeIdxStack.push(_cmd._subId);
		break;
	}
	case ZoombiniDebugCommand::kDrawFeature: { // [*] Virtual Feature - drawFeature
		ZmbFeature::EventHooks hooksDrawFeature;
		hooksDrawFeature.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniDialogDebug::drawFeature_render));
		hooksDrawFeature.setKeyDownFunc(reinterpret_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogDebug::drawFeature_onKeyDown));
		loadScrbFeature(_cmd._resource, _cmd._subId, 0, 
					   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						   hooksDrawFeature);
		break;
	}
	case ZoombiniDebugCommand::kPlotPoint: { // [*] Virtual Feature - plotPoint
		ZmbFeature::EventHooks hooksPlotPoint;
		hooksPlotPoint.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniDialogDebug::plotPoint_render));
		hooksPlotPoint.setKeyDownFunc(reinterpret_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogDebug::common_onKeyDown));

		loadVirtualFeature(kVirtualFeaturePlotPoint, 0,
					   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						   hooksPlotPoint);
		break;
	}
	case ZoombiniDebugCommand::kPlotLine: { // [*] Virtual Feature - plotLine
		ZmbFeature::EventHooks hooksPlotLine;
		hooksPlotLine.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniDialogDebug::plotLine_render));
		hooksPlotLine.setKeyDownFunc(reinterpret_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogDebug::common_onKeyDown));

		loadVirtualFeature(kVirtualFeaturePlotLine, 0,
					   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						   hooksPlotLine);
		break;
	}
	case ZoombiniDebugCommand::kPlotRect: { // [*] Virtual Feature - plotRect
		ZmbFeature::EventHooks hooksPlotRect;
		hooksPlotRect.setRenderFunc(reinterpret_cast<ZmbFeature::OnRenderFunc>(&ZoombiniDialogDebug::plotRect_render));
		hooksPlotRect.setKeyDownFunc(reinterpret_cast<ZmbFeature::OnKeyDownFunc>(&ZoombiniDialogDebug::common_onKeyDown));

		loadVirtualFeature(kVirtualFeaturePlotRect, 0,
					   ZmbFeature::FLAG_04000000_OVERLAY | ZmbFeature::FLAG_00001000_TOPMOST,
						   hooksPlotRect);
		break;
	}
	default:
		error("Invalid debug command: %u", _cmd._type);
		break;
	}
}

void ZoombiniDialogDebug::close() {
	_vm->_cursor->setDefaultCursor();
	ZoombiniDialog::close();
}

ZmbEventHandleResult ZoombiniDialogDebug::onLButtonDown(const Common::Point &absPos, const Common::Point &relPos) {
	close();
	return ZmbEventHandleResult::kConsumed;
}

ZmbEventHandleResult ZoombiniDialogDebug::common_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	switch (kbd.keycode) {
	case Common::KEYCODE_ESCAPE:
	case Common::KEYCODE_RETURN:
	case Common::KEYCODE_KP_ENTER:
		close();
		return ZmbEventHandleResult::kConsumed;
	default:
		return ZmbEventHandleResult::kPassthrough;
	}
}

void ZoombiniDialogDebug::drawTitleText(ZmbFeature *feature, const Common::U32String &titleText) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	ZoombiniGraphics::TextConf tc;
	tc._fontUsage = ZoombiniFontUsage::kFontDebugTitle;
	_vm->_gfx->fillArea(screenKind, _titleRect, ZoombiniGraphics::kColor0A_White);
	_vm->_gfx->drawText(screenKind, titleText, _titleRect, tc);
}

void ZoombiniDialogDebug::drawEscText(ZmbFeature *feature, const Common::U32String &keyLegendText) {
	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	Common::U32String text;
	if (keyLegendText.empty()) {
		text = _escText;
	} else {
		text = keyLegendText;
		if (keyLegendText.lastChar() != U' ')
			text += U" ";
		text.append(_escText);
	}

	ZoombiniGraphics::TextConf tc;
	tc._fontUsage = ZoombiniFontUsage::kFontDebugText;
	tc._hAlign = Graphics::kTextAlignRight;
	tc._fillBackground = true;
	tc._fillBackgroundPalette = ZoombiniGraphics::kColor0A_White;
	_vm->_gfx->drawText(screenKind, text, _titleRect, tc);
}

ZmbRenderResult ZoombiniDialogDebug::drawCursor_render(ZmbFeature *feature) {
	if (!_updateScreen)
		return ZmbRenderResult::kSkipped;
	_updateScreen = false;

	_vm->_cursor->setCursor(_cmd._resource._id);

	drawEscText(feature);

	return ZmbRenderResult::kRendered;
}

ZmbRenderResult ZoombiniDialogDebug::drawImage_render(ZmbFeature *feature) {
	if (!_updateScreen)
		return ZmbRenderResult::kSkipped;
	_updateScreen = false;

	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	// Setup palette if it has one
	if (_vm->hasResource(ID_SHPL, _cmd._resource))
		_vm->_gfx->setPalette(_cmd._resource._id);

	_vm->_gfx->drawBackground(screenKind, _cmd._resource._id);

	drawEscText(feature);

	return ZmbRenderResult::kRendered;
}

ZmbRenderResult ZoombiniDialogDebug::drawShape_render(ZmbFeature *feature) {
	if (!_updateScreen)
		return ZmbRenderResult::kSkipped;
	_updateScreen = false;

	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;
	_vm->_gfx->clearScreen(screenKind);

	uint32 shapeCount = _vm->_gfx->getShapeCount(_cmd._resource);
	Common::Rect shapeSize = _vm->_gfx->getShapeSize(_cmd._resource, _cmd._subId);
	shapeSize.moveTo(0, _titleRect.height());

	drawTitleText(feature, Common::String::format("Shape (%03u/%03u) of tBMP %s (w=%u h=%u)", _cmd._subId, shapeCount, _cmd._resource.toString().c_str(), shapeSize.width(), shapeSize.height()));
	drawEscText(feature);

	_vm->_gfx->fillArea(screenKind, shapeSize, ZoombiniGraphics::kColor0A_White);
	_vm->_gfx->drawShape(screenKind, _cmd._resource, _cmd._subId, Common::Point(0, _titleRect.height()));

	return ZmbRenderResult::kRendered;
}

ZmbRenderResult ZoombiniDialogDebug::drawShapes_render(ZmbFeature *feature) {
	// Skip _updateScreen check, instead check _multiScreenNextOp
	_updateScreen = false;

	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	uint16 startShapeIdx = 0;
	uint32 shapeCount = _vm->_gfx->getShapeCount(_cmd._resource);
	MultiScreenOperation multiScreenOp = _multiScreenNextOp;
	_multiScreenNextOp = kMultiScreenOpNone;

	switch (multiScreenOp) {
	case kMultiScreenOpNone: // Do nothing
		return ZmbRenderResult::kSkipped;
	case kMultiScreenOpInit:
		startShapeIdx = _cmd._subId;
		break;
	case kMultiScreenOpPrev:
		if (_drawShapesPrevShapeIdxStack.empty())
			startShapeIdx = 1;
		else
			startShapeIdx = _drawShapesPrevShapeIdxStack.pop();
		assert(startShapeIdx <= shapeCount);
		break;
	case kMultiScreenOpNext:
		if (_drawShapesNextShapeIdx == 0)
			return ZmbRenderResult::kSkipped;
		startShapeIdx = _drawShapesNextShapeIdx;
		break;
	default:
		error("Invalid drawShapesNextOp: %u", multiScreenOp);
		break;
	};

	_vm->_gfx->clearScreen(screenKind);
	_vm->_gfx->fillArea(screenKind, ZoombiniGraphics::kColor0A_White);

	ZoombiniGraphics::TextConf tc;
	tc._fontUsage = ZoombiniFontUsage::kFontDebugText;

	const uint16 textWidth = 24;
	uint16 maxWidth = 0;
	_drawShapesNextShapeIdx = 0;

	uint32 endShapeIdx = 0;

	Common::Point pos = Common::Point(0, _titleRect.height());
	for (uint32 shapeIdx = startShapeIdx; shapeIdx <= shapeCount && pos.x < ZoombiniGraphics::kScreenWidth; shapeIdx++) {
		Common::Rect shapeSize = _vm->_gfx->getShapeSize(_cmd._resource, shapeIdx);
		if (ZoombiniGraphics::kScreenHeight <= pos.y + shapeSize.height()) {
			pos.x += textWidth + maxWidth;
			pos.y = _titleRect.height();
			maxWidth = 0;
		}
		maxWidth = MAX(maxWidth, static_cast<uint16>(shapeSize.width()));

		if (ZoombiniGraphics::kScreenWidth < pos.x + textWidth + maxWidth)
			_drawShapesNextShapeIdx = shapeIdx;

		Common::Rect textRect = Common::Rect(pos, textWidth, shapeSize.height());
		_vm->_gfx->drawText(screenKind, Common::String::format("%u", shapeIdx), textRect, tc);

		Common::Point shapePos = Common::Point(pos.x + textWidth, pos.y);
		_vm->_gfx->drawShape(screenKind, _cmd._resource, shapeIdx, shapePos);

		pos.y += shapeSize.height();
		endShapeIdx = MAX(endShapeIdx, shapeIdx);
	}

	if (multiScreenOp != kMultiScreenOpPrev && 0 < _drawShapesNextShapeIdx)
		_drawShapesPrevShapeIdxStack.push(startShapeIdx);
	
	drawTitleText(feature, Common::String::format("[Shapes] tBMP(%s) shape(%u-%u/%u)", 
		_cmd._resource.toString().c_str(), 
		startShapeIdx, endShapeIdx, shapeCount));
	
	Common::U32String keyLegendText;
	if (endShapeIdx < shapeCount)
		keyLegendText = Common::U32String(U"[<-/->] shape");
	drawEscText(feature, keyLegendText);

	return ZmbRenderResult::kRendered;
}

ZmbEventHandleResult ZoombiniDialogDebug::drawShapes_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;

	switch (kbd.keycode) {
	case Common::KEYCODE_ESCAPE:
	case Common::KEYCODE_RETURN:
	case Common::KEYCODE_KP_ENTER:
		close();
		result = ZmbEventHandleResult::kConsumed;
		break;
	case Common::KEYCODE_LEFT:
		_multiScreenNextOp = kMultiScreenOpPrev;
		result = ZmbEventHandleResult::kConsumed;
		break;
	case Common::KEYCODE_KP4:
		if ((kbd.flags & Common::KBD_NUM) == 0) {
			_multiScreenNextOp = kMultiScreenOpPrev;
			result = ZmbEventHandleResult::kConsumed;
		}
		break;
	case Common::KEYCODE_RIGHT:
		_multiScreenNextOp = kMultiScreenOpNext;
		result = ZmbEventHandleResult::kConsumed;
		break;
	case Common::KEYCODE_KP6:
		if ((kbd.flags & Common::KBD_NUM) == 0) {
			_multiScreenNextOp = kMultiScreenOpNext;
			result = ZmbEventHandleResult::kConsumed;
		}
		break;
	default:
		break;
	}

	return result;
}

ZmbRenderResult ZoombiniDialogDebug::drawFeature_render(ZmbFeature *feature) {
	// Skip _updateScreen check, instead check _multiScreenNextOp
	_updateScreen = false;

	uint16 startHsId = 0;
	MultiScreenOperation multiScreenOp = _multiScreenNextOp;
	_multiScreenNextOp = kMultiScreenOpNone;

	switch (multiScreenOp) {
	case kMultiScreenOpNone: // Do nothing
		return ZmbRenderResult::kSkipped;
	case kMultiScreenOpInit:
		startHsId = 0;
		break;
	case kMultiScreenOpPrev:
		if (_drawFeaturePrevHsIdxStack.empty())
			startHsId = 0;
		else
			startHsId = _drawFeaturePrevHsIdxStack.pop();
		break;
	case kMultiScreenOpNext:
		if (_drawFeatureNextHsIdx == 0)
			return ZmbRenderResult::kSkipped;
		startHsId = _drawFeatureNextHsIdx;
		break;
	case kMultiScreenOpUp:
		if (_drawFeatureFrame == 0)
			return ZmbRenderResult::kSkipped;
		do {
			_drawFeatureFrame -= 1u;
			ZmbHotspotGroup *nextGroup = feature->getHotspotGroup(_drawFeatureFrame);
			if (nextGroup && nextGroup->getHotspotCount() != 0)
				break; // Found non-empty previous frame
		} while (0 < _drawFeatureFrame);
		break;
	case kMultiScreenOpDown:
		if (feature->getFrameCount() <= _drawFeatureFrame + 1u)
			return ZmbRenderResult::kSkipped;
		do {
			_drawFeatureFrame += 1u;
			ZmbHotspotGroup *nextGroup = feature->getHotspotGroup(_drawFeatureFrame);
			if (nextGroup && nextGroup->getHotspotCount() != 0)
				break; // Found non-empty next frame
		} while (_drawFeatureFrame + 1u < feature->getFrameCount());
		break;
	default:
		error("Invalid drawFeatureNextOp: %u", multiScreenOp);
		break;
	};

	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	_vm->_gfx->clearScreen(screenKind);
	_vm->_gfx->fillArea(screenKind, ZoombiniGraphics::kColor0A_White);

	ZmbHotspotGroup *hsGroup = feature->getHotspotGroup(_drawFeatureFrame);
	uint32 hsCount = hsGroup->getHotspotCount();
	assert(startHsId < hsCount);

	ZoombiniGraphics::TextConf tc;
	tc._fontUsage = ZoombiniFontUsage::kFontDebugText;

	const uint16 textWidth = 80;
	uint16 maxWidth = 0;
	uint32 endHsId = 0;
	_drawFeatureNextHsIdx = 0;

	Common::Point pos = Common::Point(0, _titleRect.height());
	for (uint32 hsId = startHsId; hsId < hsCount && pos.x < ZoombiniGraphics::kScreenWidth; hsId++) {
		ZmbHotspot &hs = hsGroup->getHotspot(hsId);
		if (hs._shapeIdx == 0)
			continue; // Skip empty hotspot
		Common::Rect shapeSize = _vm->_gfx->getShapeSize(_cmd._resource, hs._shapeIdx);
		if (ZoombiniGraphics::kScreenHeight <= pos.y + shapeSize.height()) {
			pos.x += textWidth + maxWidth;
			pos.y = _titleRect.height();
			maxWidth = 0;
		}
		maxWidth = MAX(maxWidth, static_cast<uint16>(shapeSize.width()));

		if (ZoombiniGraphics::kScreenWidth < pos.x + textWidth + maxWidth)
			_drawFeatureNextHsIdx = hsId;

		Common::Rect textRect = Common::Rect(pos, textWidth, shapeSize.height());
		_vm->_gfx->drawText(screenKind, Common::String::format("%2u (%3u, %3u)", hs._hsId, hs._x, hs._y), textRect, tc);

		Common::Point shapePos = Common::Point(pos.x + textWidth, pos.y);
		_vm->_gfx->drawShape(screenKind, _cmd._resource, hs._shapeIdx, shapePos);

		pos.y += shapeSize.height();

		endHsId = MAX(endHsId, hsId);
	}

	drawTitleText(feature, Common::String::format("[Feature] tBMP(%s) SCRB(%03u) hotspot(%u-%u/%u) frame(%u/%u)", 
		_cmd._resource.toString().c_str(), _cmd._subId, 
		startHsId, endHsId, hsCount, 
		_drawFeatureFrame, feature->getFrameCount()));
	Common::U32String keyLegendText;
	if (endHsId + 1 < hsCount)
		keyLegendText = Common::U32String(U"[<-/->] hs ");
	if (1 < feature->getFrameCount())
		keyLegendText += Common::U32String(U"[UP/DN] frame ");
	drawEscText(feature, keyLegendText);

	return ZmbRenderResult::kRendered;
}

ZmbEventHandleResult ZoombiniDialogDebug::drawFeature_onKeyDown(ZmbFeature *feature, const Common::KeyState &kbd, bool kbdRepeat) {
	ZmbEventHandleResult result = ZmbEventHandleResult::kPassthrough;

	switch (kbd.keycode) {
	case Common::KEYCODE_ESCAPE:
	case Common::KEYCODE_RETURN:
	case Common::KEYCODE_KP_ENTER:
		close();
		result = ZmbEventHandleResult::kConsumed;
		break;
	case Common::KEYCODE_LEFT:
		_multiScreenNextOp = kMultiScreenOpPrev;
		result = ZmbEventHandleResult::kConsumed;
		break;
	case Common::KEYCODE_KP4:
		if ((kbd.flags & Common::KBD_NUM) == 0) {
			_multiScreenNextOp = kMultiScreenOpPrev;
			result = ZmbEventHandleResult::kConsumed;
		}
		break;
	case Common::KEYCODE_RIGHT:
		_multiScreenNextOp = kMultiScreenOpNext;
		result = ZmbEventHandleResult::kConsumed;
		break;
	case Common::KEYCODE_KP6:
		if ((kbd.flags & Common::KBD_NUM) == 0) {
			_multiScreenNextOp = kMultiScreenOpNext;
			result = ZmbEventHandleResult::kConsumed;
		}
		break;
	case Common::KEYCODE_UP:
		_multiScreenNextOp = kMultiScreenOpUp;
		result = ZmbEventHandleResult::kConsumed;
		break;
	case Common::KEYCODE_KP8:
		if ((kbd.flags & Common::KBD_NUM) == 0) {
			_multiScreenNextOp = kMultiScreenOpUp;
			result = ZmbEventHandleResult::kConsumed;
		}
		break;
	case Common::KEYCODE_DOWN:
		_multiScreenNextOp = kMultiScreenOpDown;
		result = ZmbEventHandleResult::kConsumed;
		break;
	case Common::KEYCODE_KP2:
		if ((kbd.flags & Common::KBD_NUM) == 0) {
			_multiScreenNextOp = kMultiScreenOpDown;
			result = ZmbEventHandleResult::kConsumed;
		}
		break;
	default:
		break;
	}

	return result;
}

ZmbRenderResult ZoombiniDialogDebug::plotPoint_render(ZmbFeature *feature) {
	if (!_updateScreen)
		return ZmbRenderResult::kSkipped;
	_updateScreen = false;

	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	// Draw the point
	Graphics::Surface *screen = _vm->_gfx->getScreen(screenKind);
	screen->setPixel(_cmd._x1, _cmd._y1, _cmd._color);

	// Draw title
	drawTitleText(feature, Common::String::format("[Plot Point] at (%d, %d) with color %u", 
		_cmd._x1, _cmd._y1, _cmd._color));
	drawEscText(feature);

	return ZmbRenderResult::kRendered;
}

ZmbRenderResult ZoombiniDialogDebug::plotLine_render(ZmbFeature *feature) {
	if (!_updateScreen)
		return ZmbRenderResult::kSkipped;
	_updateScreen = false;

	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	// Draw the line (note: _width and _height store x1 and y1)
	_vm->_gfx->drawLine(screenKind, Common::Point(_cmd._x1, _cmd._y1), 
		Common::Point(_cmd._x2, _cmd._y2), _cmd._color);

	// Draw title
	drawTitleText(feature, Common::String::format("[Plot Line] from (%d, %d) to (%d, %d) with color %u", 
		_cmd._x1, _cmd._y1, _cmd._x2, _cmd._y2, _cmd._color));
	drawEscText(feature);

	return ZmbRenderResult::kRendered;
}

ZmbRenderResult ZoombiniDialogDebug::plotRect_render(ZmbFeature *feature) {
	if (!_updateScreen)
		return ZmbRenderResult::kSkipped;
	_updateScreen = false;

	ZoombiniGraphics::ScreenKind screenKind = ZoombiniGraphics::kShapeScreen;

	// Draw the rectangle
	Common::Rect rect(_cmd._x1, _cmd._y1, _cmd._x2, _cmd._y2);
	Graphics::Surface *screen = _vm->_gfx->getScreen(screenKind);
	screen->frameRect(rect, _cmd._color);

	// Draw title
	drawTitleText(feature, Common::String::format("[Plot Rect] from (%d, %d) to (%d, %d) with color %u", 
		_cmd._x1, _cmd._y1, _cmd._x2, _cmd._y2, _cmd._color));
	drawEscText(feature);

	return ZmbRenderResult::kRendered;
}

} // End of namespace Mohawk
