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

// Self
#include "d2vdump.h"

// SDK
#include <filesystem.h>
#include <icvar.h>
#include <tier0/platform.h>
#include <tier1/fmtstr.h>
#include <tier1/iconvar.h>

static D2VDump g_D2VDump;

IFileSystem *filesystem;
static IScriptManager *scriptmgr;

SH_DECL_HOOK1(IScriptManager, CreateVM, SH_NOATTRIB, 0, IScriptVM *, ScriptLanguage_t);
SH_DECL_HOOK1_void(IScriptManager, DestroyVM, SH_NOATTRIB, 0, IScriptVM *);

SH_DECL_HOOK1_void(IScriptVM, RegisterFunction, SH_NOATTRIB, 0, ScriptFunctionBinding_t *);
SH_DECL_HOOK1(IScriptVM, RegisterScriptClass, SH_NOATTRIB, 0, bool, ScriptClassDesc_t *);
SH_DECL_HOOK2(IScriptVM, RegisterInstance, SH_NOATTRIB, 0, HSCRIPT, ScriptClassDesc_t *, void *);
SH_DECL_HOOK3(IScriptVM, SetValue, SH_NOATTRIB, 0, bool, HSCRIPT, const char *, const char *);
SH_DECL_HOOK3(IScriptVM, SetValue, SH_NOATTRIB, 1, bool, HSCRIPT, const char *, const ScriptVariant_t &);
SH_DECL_HOOK5(IScriptVM, SetEnumValue, SH_NOATTRIB, 0, bool, HSCRIPT, const char *, const char *, int, const char *);


static class BaseAccessor : public IConCommandBaseAccessor
{
public:
	bool RegisterConCommandBase(ConCommandBase *pVar)
	{
		return META_REGCVAR(pVar);
	}
} s_BaseAccessor;

PLUGIN_EXPOSE(D2VDump, g_D2VDump);

bool D2VDump::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	if (!InitGlobals(error, maxlen))
	{
		ismm->Format(error, maxlen, "Failed to setup globals");
		return false;
	}

	memset(m_VMs, 0, sizeof(m_VMs));
	m_Dumpers.push_back(new JSONScriptDumper());

	if (!filesystem->IsDirectory("vdump", "DEFAULT_WRITE_PATH"))
		filesystem->CreateDirHierarchy("vdump", "DEFAULT_WRITE_PATH");

	InitHooks();

	return true;
}

bool D2VDump::InitGlobals(char *error, size_t maxlen)
{
	ISmmAPI *ismm = g_SMAPI;

	GET_V_IFACE_CURRENT(GetFileSystemFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, scriptmgr, IScriptManager, VSCRIPT_INTERFACE_VERSION);

	ICvar *icvar;
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	g_pCVar = icvar;
	ConVar_Register(0, &s_BaseAccessor);

	return true;
}

void D2VDump::InitHooks()
{
	SH_ADD_HOOK(IScriptManager, CreateVM, scriptmgr, SH_MEMBER(this, &D2VDump::Hook_CreateVM), false);
	SH_ADD_HOOK(IScriptManager, DestroyVM, scriptmgr, SH_MEMBER(this, &D2VDump::Hook_DestroyVM), false);
}

void D2VDump::ShutdownHooks()
{
	SH_REMOVE_HOOK(IScriptManager, DestroyVM, scriptmgr, SH_MEMBER(this, &D2VDump::Hook_DestroyVM), false);
	SH_REMOVE_HOOK(IScriptManager, CreateVM, scriptmgr, SH_MEMBER(this, &D2VDump::Hook_CreateVM), false);
}

bool D2VDump::Unload(char *error, size_t maxlen)
{
	ShutdownHooks();

	for (size_t i = 0; i < VM_Count; ++i)
	{
		for (auto d : m_Dumpers)
		{
			FileHandle_t f;
			f = filesystem->Open(CFmtStr("vdump/out%u.%s", i, d->GetOutputTypeName()), "w", "DEFAULT_WRITE_PATH");
			d->SaveFunctionsToDisk(f, VMType(i));
			filesystem->Close(f);

			f = filesystem->Open(CFmtStr("vdump/values%u.%s", i, d->GetOutputTypeName()), "w", "DEFAULT_WRITE_PATH");
			d->SaveValuesToDisk(f, VMType(i));
			filesystem->Close(f);
		}
	}

	for (auto d : m_Dumpers)
		delete d;

	m_Dumpers.clear();

	return true;
}

void D2VDump::Hook_RegisterFunction(ScriptFunctionBinding_t *pScriptFunction)
{
	VMType v = VMToVMType(META_IFACEPTR(IScriptVM));
	if (v != VM_Unknown)
	{
		for (auto d : m_Dumpers)
		{
			d->AddFunction(pScriptFunction->m_desc, v);
		}
	}

	RETURN_META(MRES_IGNORED);
}

bool D2VDump::Hook_RegisterScriptClass(ScriptClassDesc_t *pClassDesc)
{
	VMType v = VMToVMType(META_IFACEPTR(IScriptVM));
	if (v != VM_Unknown)
	{
		for (auto d : m_Dumpers)
		{
			d->AddClass(*pClassDesc, v);
		}
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}

HSCRIPT D2VDump::Hook_RegisterInstance(ScriptClassDesc_t *pDesc, void *pInstance)
{
	VMType v = VMToVMType(META_IFACEPTR(IScriptVM));
	if (v != VM_Unknown)
	{
		for (auto d : m_Dumpers)
		{
			d->AddClass(*pDesc, v);
		}
	}

	RETURN_META_VALUE(MRES_IGNORED, INVALID_HSCRIPT);
}

IScriptVM *D2VDump::Hook_CreateVM(ScriptLanguage_t language)
{
	IScriptVM *pVM = SH_CALL(scriptmgr, &IScriptManager::CreateVM)(language);
	VMType vmType = VM_Unknown;

	// Main VM is always (re)created first, at map start. Bot VM is created after lobby data is received, if lobby uses lua bots.
	if (!m_VMs[VM_Main])
	{
		vmType = VM_Main;
	}
	else if (!m_VMs[VM_Bot])
	{
		vmType = VM_Bot;
	}
	else
	{
		DevMsg("D2V: Got new unknown lua VM at 0x%p\n", pVM);
	}

	if (vmType != VM_Unknown)
	{
		m_VMs[vmType] = pVM;
		for (auto d : m_Dumpers)
		{
			d->Clear(vmType);
		}

		SH_ADD_HOOK(IScriptVM, RegisterFunction, pVM, SH_MEMBER(this, &D2VDump::Hook_RegisterFunction), false);
		SH_ADD_HOOK(IScriptVM, RegisterScriptClass, pVM, SH_MEMBER(this, &D2VDump::Hook_RegisterScriptClass), false);
		SH_ADD_HOOK(IScriptVM, RegisterInstance, pVM, SH_MEMBER(this, &D2VDump::Hook_RegisterInstance), false);
		SH_ADD_HOOK(IScriptVM, SetValue, pVM, SH_MEMBER(this, &D2VDump::Hook_SetValue1), false);
		SH_ADD_HOOK(IScriptVM, SetValue, pVM, SH_MEMBER(this, &D2VDump::Hook_SetValue2), false);
		SH_ADD_HOOK(IScriptVM, SetEnumValue, pVM, SH_MEMBER(this, &D2VDump::Hook_SetEnumValue), false);
		SH_ADD_HOOK(IScriptVM, SetEnumValue, pVM, SH_MEMBER(this, &D2VDump::Hook_SetEnumValue_Post), true);
	}

	RETURN_META_VALUE(MRES_SUPERCEDE, pVM);
}

void D2VDump::Hook_DestroyVM(IScriptVM *pVM)
{
	VMType v = VMToVMType(pVM);
	if (v != VM_Unknown)
	{
		SH_REMOVE_HOOK(IScriptVM, RegisterFunction, pVM, SH_MEMBER(this, &D2VDump::Hook_RegisterFunction), false);
		SH_REMOVE_HOOK(IScriptVM, RegisterScriptClass, pVM, SH_MEMBER(this, &D2VDump::Hook_RegisterScriptClass), false);
		SH_REMOVE_HOOK(IScriptVM, RegisterInstance, pVM, SH_MEMBER(this, &D2VDump::Hook_RegisterInstance), false);

		SH_REMOVE_HOOK(IScriptVM, SetValue, pVM, SH_MEMBER(this, &D2VDump::Hook_SetValue1), false);
		SH_REMOVE_HOOK(IScriptVM, SetValue, pVM, SH_MEMBER(this, &D2VDump::Hook_SetValue2), false);
		SH_REMOVE_HOOK(IScriptVM, SetEnumValue, pVM, SH_MEMBER(this, &D2VDump::Hook_SetEnumValue), false);
		SH_REMOVE_HOOK(IScriptVM, SetEnumValue, pVM, SH_MEMBER(this, &D2VDump::Hook_SetEnumValue_Post), true);

		m_VMs[v] = nullptr;
	}

	RETURN_META(MRES_IGNORED);
}

bool D2VDump::Hook_SetValue1(HSCRIPT hScope, const char *pszKey, const char *pszValue)
{
	if (!m_bInSetEnumValue)
	{
		DevMsg("SV!: (HSCRIPT: %p) (Name: \"%s\")\n", hScope, pszKey);
		VMType v = VMToVMType(META_IFACEPTR(IScriptVM));
		if (v != VM_Unknown)
		{
			for (auto d : m_Dumpers)
			{
				d->AddValue(pszKey, ScriptVariant_t(pszValue), v);
			}
		}
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool D2VDump::Hook_SetValue2(HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value)
{
	if (!m_bInSetEnumValue)
	{
		DevMsg("SV2: (HSCRIPT: %p) (Name: \"%s\")\n", hScope, pszKey);
		VMType v = VMToVMType(META_IFACEPTR(IScriptVM));
		if (v != VM_Unknown)
		{
			for (auto d : m_Dumpers)
			{
				d->AddValue(pszKey, value, v);
			}
		}
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool D2VDump::Hook_SetEnumValue(HSCRIPT hScope, const char *pszEnumName, const char *pszValueName, int value, const char *pszDescription)
{
	DevMsg("SEV: (HSCRIPT: %p) (Name: \"%s\") (Value: (\"%s\")\n", hScope, pszValueName, pszValueName);
	m_bInSetEnumValue = true;

	VMType v = VMToVMType(META_IFACEPTR(IScriptVM));
	if (v != VM_Unknown)
	{
		for (auto d : m_Dumpers)
		{
			d->AddEnumValue(pszEnumName, pszValueName, pszDescription, value, v);
		}
	}

	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool D2VDump::Hook_SetEnumValue_Post(HSCRIPT hScope, const char *pszEnumName, const char *pszValueName, int value, const char *pszDescription)
{
	m_bInSetEnumValue = false;

	return true;
}

const char *D2VDump::GetLicense()
{
	return "GPLv3";
}

const char *D2VDump::GetVersion()
{
	return "1.0.0.0";
}

const char *D2VDump::GetDate()
{
	return __DATE__;
}

const char *D2VDump::GetLogTag()
{
	return "D2VDUMP";
}

const char *D2VDump::GetAuthor()
{
	return "Nicholas Hastings";
}

const char *D2VDump::GetDescription()
{
	return "Dumps Dota 2 VScript classes, functions, and variables.";
}

const char *D2VDump::GetName()
{
	return "Dota 2 VScript Dumper";
}

const char *D2VDump::GetURL()
{
	return "https://moddota.com";
}
