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

#include "jsondumper.h"
#include <tier1/fmtstr.h>

JSONScriptDumper::JSONScriptDumper()
{
	for (size_t i = 0; i < VM_Count; ++i)
	{
		m_Json[i] = json_object();
		m_GlobalFuncs[i] = json_object();
	}
}

JSONScriptDumper::~JSONScriptDumper()
{
	for (size_t i = 0; i < VM_Count; ++i)
	{
		json_decref(m_Json[i]);
	}
}

void JSONScriptDumper::Clear(VMType v)
{
	// Only need to clear out globals and enums. Class defs are already protected against dupes.
	m_GlobalConstants[v].clear();
	m_Enums[v].clear();
}

void JSONScriptDumper::SaveFunctionsToDisk(FileHandle_t f, VMType v)
{
	auto *pGlobal = json_object();

	json_object_set_new(pGlobal, "functions", m_GlobalFuncs[v]);

	json_object_set_new(m_Json[v], "Global", pGlobal);

	json_dump_callback(m_Json[v],
		[](const char *buffer, size_t size, void *data) -> int {
			filesystem->Write(buffer, size, (FileHandle_t)data);
			return 0;
		},
		f, JSON_INDENT(4) | JSON_SORT_KEYS);
}

inline json_t *VariantToJSON(const ScriptVariant_t &value)
{
	switch (value.m_type)
	{
	case FIELD_CSTRING:
		return json_string(value.m_pszString);
	case FIELD_INTEGER:
		return json_integer(value.m_int);
	case FIELD_FLOAT:
		return json_real(value.m_float);
	case FIELD_HSCRIPT:
		return json_string("<handle>");
	case FIELD_UINT:
		return json_integer(value.m_uint);
	case FIELD_VECTOR:
	{
		auto *pValues = json_array();
		json_array_append_new(pValues, json_real(value.m_pVector->x));
		json_array_append_new(pValues, json_real(value.m_pVector->y));
		json_array_append_new(pValues, json_real(value.m_pVector->z));
		return pValues;
	}
	}

	return json_string(CFmtStr("<unhandled_variant_type_%d>", value.m_type));
}

void JSONScriptDumper::SaveValuesToDisk(FileHandle_t f, VMType v)
{
	auto *pJson = json_object();

	auto *pLooseGlobals = json_array();
	for (auto &i : m_GlobalConstants[v])
	{
		auto *j = json_object();
		json_object_set_new(j, "key", json_string(i.name.c_str()));
		json_object_set_new(j, "value", VariantToJSON(i.value));
		if (i.desc.length())
			json_object_set_new(j, "description", json_string(i.desc.c_str()));

		json_array_append_new(pLooseGlobals, j);
	}
	json_object_set_new(pJson, "_Unscoped", pLooseGlobals);

	for (auto &i : m_Enums[v])
	{
		auto *pEnum = json_array();
		
		for (auto &j : i.second)
		{
			auto *pEnumValue = json_object();
			json_object_set_new(pEnumValue, "key", json_string(j.name.c_str()));
			json_object_set_new(pEnumValue, "value", VariantToJSON(j.value));
			if (j.desc.length()	)
				json_object_set_new(pEnumValue, "description", json_string(j.desc.c_str()));

			json_array_append_new(pEnum, pEnumValue);
		}

		json_object_set_new(pJson, i.first, pEnum);
	}

	json_dump_callback(pJson,
		[](const char *buffer, size_t size, void *data) -> int {
			filesystem->Write(buffer, size, (FileHandle_t)data);
			return 0;
		},
		f, JSON_INDENT(4) | JSON_SORT_KEYS);

	json_decref(pJson);
}

json_t *JSONScriptDumper::FuncDescToJSON(ScriptFuncDescriptor_t &scriptFunc)
{
	auto *pFunc = json_object();

	if (scriptFunc.m_pszDescription && scriptFunc.m_pszDescription[0])
	{
		json_object_set_new(pFunc, "description", json_string(scriptFunc.m_pszDescription));
	}

	json_object_set_new(pFunc, "return", json_string(NameForType(scriptFunc.m_ReturnType)));

	auto *pArgs = json_array();
	for (size_t i = 0; i < scriptFunc.m_iParamCount; ++i)
	{
		json_array_append_new(pArgs, json_string(NameForType(scriptFunc.m_Parameters[i])));
	}
	json_object_set_new(pFunc, "args", pArgs);

	if (scriptFunc.m_pszParameterNames)
	{
		auto *pArgNames = json_array();
		size_t iParamNameStart = 0;
		for (size_t i = 0; i < scriptFunc.m_iParamCount; ++i)
		{
			static char szParamName[64];
			if (scriptFunc.m_pszParameterNames)
			{
				iParamNameStart += (1 + Q_snprintf(szParamName, sizeof(szParamName), "%s", &scriptFunc.m_pszParameterNames[iParamNameStart]));
			}
			else
			{
				szParamName[0] = 0;
			}
			json_array_append_new(pArgNames, json_string(szParamName));
		}
		json_object_set_new(pFunc, "arg_names", pArgNames);
	}

	return pFunc;
}

void JSONScriptDumper::AddClass(ScriptClassDesc_t &classDesc, VMType v)
{
	if (m_Classes[v].find(classDesc.m_pszScriptName) != m_Classes[v].end())
		return;

	if (classDesc.m_pBaseDesc)
	{
		AddClass(*classDesc.m_pBaseDesc, v);
	}

	auto *pClass = json_object();
	if (classDesc.m_pBaseDesc)
	{
		json_object_set_new(pClass, "extends", json_string(classDesc.m_pBaseDesc->m_pszScriptName));
	}
	if (classDesc.m_pszDescription)
	{
		json_object_set_new(pClass, "description", json_string(classDesc.m_pszDescription));
	}

	auto *pFuncs = json_object();

	FOR_EACH_VEC(classDesc.m_FunctionBindings, i)
	{
		json_object_set_new(pFuncs, classDesc.m_FunctionBindings[i].m_desc.m_pszScriptName, FuncDescToJSON(classDesc.m_FunctionBindings[i].m_desc));
	}

	json_object_set_new(pClass, "functions", pFuncs);

	json_object_set_new(m_Json[v], classDesc.m_pszScriptName, pClass);

	m_Classes[v].insert(classDesc.m_pszScriptName);
}

void JSONScriptDumper::AddFunction(ScriptFuncDescriptor_t &funcDesc, VMType v)
{
	if (m_Funcs[v].find(funcDesc.m_pszScriptName) != m_Funcs[v].end())
		return;

	json_object_set_new(m_GlobalFuncs[v], funcDesc.m_pszScriptName, FuncDescToJSON(funcDesc));
	m_Funcs[v].insert(funcDesc.m_pszScriptName);
}

void JSONScriptDumper::AddValue(const char *pszName, const ScriptVariant_t &value, VMType v)
{
	ScriptConstant_t sc;
	sc.name = pszName;
	sc.desc = "";
	sc.value = value;

	m_GlobalConstants[v].push_back(sc);
}

void JSONScriptDumper::AddEnumValue(const char *pszEnumName, const char *pszName, const char *pszDesc, int value, VMType v)
{
	ScriptConstant_t sc;
	sc.name = pszName;
	if (pszDesc)
		sc.desc = pszDesc;
	else
		sc.desc = "";
	sc.value = ScriptVariant_t(value);
	m_Enums[v][pszEnumName].push_back(sc);
}
