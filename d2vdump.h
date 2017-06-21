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

#include <ISmmPlugin.h>

#include "common.h"
#include "jsondumper.h"

#include <vscript/ivscript.h>

#include <vector>

class D2VDump : public ISmmPlugin
{
public: // ISmmPlugin
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) override;
	bool Unload(char *error, size_t maxlen) override;
	const char *GetAuthor() override;
	const char *GetName() override;
	const char *GetDescription() override;
	const char *GetURL() override;
	const char *GetLicense() override;
	const char *GetVersion() override;
	const char *GetDate() override;
	const char *GetLogTag() override;

private:
	bool InitGlobals(char *error, size_t maxlen);
	void InitHooks();
	void ShutdownHooks();
	VMType VMToVMType(IScriptVM *pVM);

private:
	void Hook_RegisterFunction(ScriptFunctionBinding_t *pScriptFunction);
	bool Hook_RegisterScriptClass(ScriptClassDesc_t *pClassDesc);
	HSCRIPT Hook_RegisterInstance(ScriptClassDesc_t *pDesc, void *pInstance);
	IScriptVM *Hook_CreateVM(ScriptLanguage_t language);
	void Hook_DestroyVM(IScriptVM *pVM);
	bool Hook_SetValue1(HSCRIPT hScope, const char *pszKey, const char *pszValue);
	bool Hook_SetValue2(HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value);
	bool Hook_SetValue3(HSCRIPT hScope, int nIndex, const ScriptVariant_t &value);
	bool Hook_SetEnumValue(HSCRIPT hScope, const char *pszEnumName, const char *pszValueName, int value, const char *pszDescription);
	bool Hook_SetEnumValue_Post(HSCRIPT hScope, const char *pszEnumName, const char *pszValueName, int value, const char *pszDescription);

private:
	IScriptVM *m_VMs[VM_Count];
	bool m_bInSetEnumValue = false;
	std::vector<IScriptDumper *> m_Dumpers;
};

inline VMType D2VDump::VMToVMType(IScriptVM *pVM)
{
	for (size_t i = 0; i < VM_Count; ++i)
	{
		if (pVM == m_VMs[i])
		{
			return VMType(i);
		}
	}

	return VM_Unknown;
}

PLUGIN_GLOBALVARS();
