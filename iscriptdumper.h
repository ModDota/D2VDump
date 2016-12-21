/**
* =============================================================================
* D2VDump
* Copyright (C) 2016 Nicholas Hastings
* =============================================================================
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License, version 2.0 or later, as published
* by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program.  If not, see <http://www.gnu.org/licenses/>.
*
* As a special exception, you are also granted permission to link the code
* of this program (as well as its derivative works) to "Dota 2," the
* "Source Engine, and any Game MODs that run on software by the Valve Corporation.
* You must obey the GNU General Public License in all respects for all other
* code used.  Additionally, this exception is granted to all derivative works.
*/

#pragma once

#include "common.h"
#include <filesystem.h>

#undef strdup
#include <vscript/ivscript.h>

class IScriptDumper
{
public:
	virtual void Clear(VMType v) = 0;
	virtual const char *GetOutputTypeName() const = 0;
	virtual void AddClass(ScriptClassDesc_t &classDesc, VMType v) = 0;
	virtual void AddFunction(ScriptFuncDescriptor_t &funcDesc, VMType v) = 0;
	virtual void SaveFunctionsToDisk(FileHandle_t f, VMType v) = 0;
	virtual void AddValue(const char *pszName, const ScriptVariant_t &value, VMType v) = 0;
	virtual void AddEnumValue(const char *pszEnumName, const char *pszName, const char *pszDesc, int value, VMType v) = 0;
	virtual void SaveValuesToDisk(FileHandle_t f, VMType v) = 0;
};

// Not using ScriptFieldTypeName because we have some custom type names
inline const char *NameForType(ScriptDataType_t type)
{
	const char *result;

	switch (type)
	{
	case FIELD_VOID:
		result = "void";
		break;
	case FIELD_FLOAT:
		result = "float";
		break;
	case FIELD_VECTOR:
		result = "vector";
		break;
	case FIELD_QUATERNION:
		result = "quaternion";
		break;
	case FIELD_INTEGER:
		result = "int";
		break;
	case FIELD_BOOLEAN:
		result = "bool";
		break;
	case FIELD_CHARACTER:
		result = "char";
		break;
	case FIELD_COLOR32:
		result = "color";
		break;
	case FIELD_EHANDLE:
		result = "ehandle";
		break;
	case FIELD_VECTOR2D:
		result = "vector2d";
		break;
	case FIELD_VECTOR4D:
		result = "vector4d";
		break;
	case FIELD_INTEGER64:
		result = "int64";
		break;
	case FIELD_RESOURCE:
		result = "resourcehandle";
		break;
	case FIELD_CSTRING:
		result = "cstring";
		break;
	case FIELD_HSCRIPT:
		result = "handle";
		break;
	case FIELD_VARIANT:
		result = "variant";
		break;
	case FIELD_UINT64:
		result = "uint64";
		break;
	case FIELD_FLOAT64:
		result = "float64";
		break;
	case FIELD_UINT:
		result = "uint";
		break;
	case FIELD_UTLSTRINGTOKEN:
		result = "utlstringtoken";
		break;
	case FIELD_QANGLE:
		result = "qangle";
		break;
	case FIELD_TYPEUNKNOWN:
		result = "<unknown>";
		break;
	default:
	{
		static char szTypeName[32];
		Q_snprintf(szTypeName, sizeof(szTypeName), "unknown_variant_type_%d", type);
		result = &szTypeName[0];
		break;
	}
	}
	return result;
}
