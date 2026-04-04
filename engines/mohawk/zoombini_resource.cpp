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

#include "mohawk/zoombini_resource.h"
#include "zoombini_resource.h"

#include <errno.h>

namespace Mohawk {

bool ZmbResource::parseInt(const char *str, int32 &result) {
	if (!str || *str == '\0') {
		warning("Error: Empty string\n");
		return false;
	}

	char *endPtr = nullptr;
	int base = 10;

	// Check if it's a hexadecimal number (starts with "0x" or "0X")
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		base = 16;
	}

	errno = 0;
	long parsed = strtol(str, &endPtr, base);

	// Check for conversion errors
	if (errno != 0) {
		warning("Error: Integer overflow or underflow in '%s'\n", str);
		return false;
	}

	// Check if any characters were converted
	if (endPtr == str) {
		warning("Error: '%s' is not a valid integer\n", str);
		return false;
	}

	// Check if there are trailing characters
	if (*endPtr != '\0') {
		warning("Error: '%s' contains invalid characters\n", str);
		return false;
	}

	result = static_cast<int32>(parsed);
	return true;
}

bool ZmbResource::parse(const char *str, ZmbResource &outRes) {
	bool success = true;
	int32 parsedId = 0;

	// Ex) s:4100, p:4100, s:0x1004, p:0x1004
	if (2 < strlen(str) && str[1] == ':') {
		if (str[0] == 's' || str[0] == 'S')
			outRes._archiveKind = ZmbArchiveKind::kSystem;
		else if (str[0] == 'p' || str[0] == 'P')
			outRes._archiveKind = ZmbArchiveKind::kPage;
		else
			success = false;
	
		if (success && !parseInt(str + 2, parsedId))
			success = false;
	} else {
		// Defaults to page, Ex) 4100, 0x1004
		outRes._archiveKind = ZmbArchiveKind::kPage;

		if (!parseInt(str, parsedId))
			success = false;
	}
	
	if (success) {
		if (parsedId < 0 || parsedId > 0xFFFF) {
			warning("Error: Resource ID %d is out of range (0-65535)\n", parsedId);
			success = false;
		} else {
			outRes._id = static_cast<uint16>(parsedId);
		}
	}
	
	if (!success)
		warning("Cannot parse string(%s), try <UINT16>, <s:UINT16> or <p:UINT16> pattern (hex supported with 0x prefix)\n", str);
	return success;
}

bool ZmbResource::operator==(const ZmbResource &other) const {
	return _archiveKind == other._archiveKind && _id == other._id;
}

bool ZmbResource::operator!=(const ZmbResource &other) const {
	return !(*this == other);
}

bool ZmbResource::hasId() const {
	return _id != 0;
}

Common::String ZmbResource::toString() const {
	char archiveKind = 'p';
	switch (_archiveKind) {
	case ZmbArchiveKind::kPage:
		archiveKind = 'p';
		break;
	case ZmbArchiveKind::kSystem:
		archiveKind = 's';
		break;
	default:
		archiveKind = '?';
		break;
	}
	return Common::String::format("%c%u", archiveKind, _id);
}

} // End of namespace Mohawk
