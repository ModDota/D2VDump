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

#include "iscriptdumper.h"
#include <jansson.h>

#include <map>
#include <set>
#include <string>
#include <vector>

class JSONScriptDumper : public IScriptDumper
{
public:
	JSONScriptDumper();
	~JSONScriptDumper();
public: // IScriptDumper
	void Clear(VMType v) override;
	const char *GetOutputTypeName() const override { return "json"; }
	void AddClass(ScriptClassDesc_t &classDesc, VMType v) override;
	void AddFunction(ScriptFuncDescriptor_t &funcDesc, VMType v) override;
	void SaveFunctionsToDisk(FileHandle_t f, VMType v) override;
	void AddValue(const char *pszName, const ScriptVariant_t &value, VMType v) override;
	void AddEnumValue(const char *pszEnumName, const char *pszName, const char *pszDesc, int value, VMType v) override;
	void SaveValuesToDisk(FileHandle_t f, VMType v) override;
private:
	json_t *FuncDescToJSON(ScriptFuncDescriptor_t &scriptFunc);
private:
	json_t *m_Json[VM_Count];
	json_t *m_GlobalFuncs[VM_Count];

	typedef std::set<const char *> StringSet_t;
	StringSet_t m_Classes[VM_Count];
	StringSet_t m_Funcs[VM_Count];

	struct ScriptConstant_t
	{
		std::string name;
		std::string desc;
		ScriptVariant_t value;

		~ScriptConstant_t()
		{
			if (value.m_flags & SV_FREE)
			{
				//free((void *)value.m_pszString);				
			}
		}
	};

	typedef std::vector<ScriptConstant_t> ScriptConstantList_t;
	typedef std::map<const char *, ScriptConstantList_t> ScriptEnumList_t;

	ScriptEnumList_t m_Enums[VM_Count];
	ScriptConstantList_t m_GlobalConstants[VM_Count];
};
