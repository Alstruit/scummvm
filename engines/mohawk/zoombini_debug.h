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

#ifndef MOHAWK_ZOOMBINI_DEBUG_H
#define MOHAWK_ZOOMBINI_DEBUG_H

#include "common/scummsys.h"
#include "mohawk/zoombini_resource.h"

namespace Mohawk {

struct ZoombiniDebugCommand {
public:
	enum DebugCommandType { 
		kNone = 0,
		kDrawCursor,
		kDrawImage,
		kDrawShape,
		kDrawShapes,
		kDrawFeature,
		kPlotPoint,
		kPlotLine,
		kPlotRect,
	};

	void setDrawCursor(ZmbResource resource);
	void setDrawImage(ZmbResource resource);
	void setDrawShape(ZmbResource resource, uint16 shapeIdx);
	void setDrawShapes(ZmbResource resource, uint16 startShapeIdx);
	void setDrawFeature(ZmbResource resource, uint16 scrbId);
	void setPlotPoint(int16 x, int16 y, uint32 color);
	void setPlotLine(int16 x0, int16 y0, int16 x1, int16 y1, uint32 color);
	void setPlotRect(int16 x, int16 y, int16 width, int16 height, uint32 color);

	DebugCommandType _type = kNone;
	ZmbResource _resource;
	uint16 _subId = 1;
	int16 _x1 = 0;
	int16 _y1 = 0;
	int16 _x2 = 0;
	int16 _y2 = 0;
	uint32 _color = 0;
};

} // End of namespace Mohawk

#endif // MOHAWK_ZOOMBINI_DEBUG_H
