// API hook
//
// GetProcAddress�œ���call��i�֐��{�́j�𒼐ڏ��������A
// �����̃t�b�N�֐���jmp������B
//
// �����Ō���API���g�����́A�R�[�h����x�߂��Ă���call�B
// ������jmp�R�[�h�ɖ߂��B
//
// �}���`�X���b�h�� ������������call�����ƍ���̂ŁA
// CriticalSection�Ŕr�����䂵�Ă����B
//

#include "override.h"
#include "ft.h"
#include "fteng.h"
#include <locale.h>
#include "undocAPI.h"
#include "delayimp.h"
#include <dwrite_2.h>
#include <dwrite_3.h>
#include <VersionHelpers.h>

#pragma comment(lib, "delayimp")

HINSTANCE g_dllInstance;

//PFNLdrGetProcedureAddress LdrGetProcedureAddress = (PFNLdrGetProcedureAddress)GetProcAddress(LoadLibrary(_T("ntdll.dll")),"LdrGetProcedureAddress");
//PFNCreateProcessW nCreateProcessW = (PFNCreateProcessW)MyGetProcAddress(LoadLibrary(_T("kernel32.dll")),"CreateProcessW");
//PFNCreateProcessA nCreateProcessA = (PFNCreateProcessA)MyGetProcAddress(LoadLibrary(_T("kernel32.dll")),"CreateProcessA");
// HMODULE hGDIPP = GetModuleHandleW(L"gdiplus.dll");
// typedef int (WINAPI *PFNGdipCreateFontFamilyFromName)(const WCHAR *name, void *fontCollection, void **FontFamily);
// PFNGdipCreateFontFamilyFromName GdipCreateFontFamilyFromName = hGDIPP? (PFNGdipCreateFontFamilyFromName)GetProcAddress(hGDIPP, "GdipCreateFontFamilyFromName"):0;

#ifdef USE_DETOURS

#include "detours.h"
#pragma comment (lib, "detours.lib")
#pragma comment (lib, "detoured.lib")
// DATA_foo�AORIG_foo �̂Q���܂Ƃ߂Ē�`����}�N��
#define HOOK_MANUALLY HOOK_DEFINE
#define HOOK_DEFINE(rettype, name, argtype) \
	rettype (WINAPI * ORIG_##name) argtype;

#include "hooklist.h"

#undef HOOK_DEFINE
#undef HOOK_MANUALLY

//
#define HOOK_MANUALLY(rettype, name, argtype) ;
#define HOOK_DEFINE(rettype, name, argtype) \
	ORIG_##name = name;
#pragma optimize("s", on)
static void hook_initinternal()
{
#include "hooklist.h"
}
#pragma optimize("", on)
#undef HOOK_DEFINE
#undef HOOK_MANUALLY

#define HOOK_MANUALLY(rettype, name, argtype) ;
#define HOOK_DEFINE(rettype, name, argtype) \
	if (&ORIG_##name) { DetourAttach(&(PVOID&)ORIG_##name, IMPL_##name); }
static LONG hook_init()
{
	DetourRestoreAfterWith();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

#include "hooklist.h"

	LONG error = DetourTransactionCommit();

	if (error != NOERROR) {
		TRACE(_T("hook_init error: %#x\n"), error);
	}
	return error;
}
#undef HOOK_DEFINE
#undef HOOK_MANUALLY

#define HOOK_DEFINE(rettype, name, argtype);
#define HOOK_MANUALLY(rettype, name, argtype) \
	LONG hook_demand_##name(){ \
	DetourRestoreAfterWith(); \
	DetourTransactionBegin(); \
	DetourUpdateThread(GetCurrentThread()); \
	if (&ORIG_##name) { DetourAttach(&(PVOID&)ORIG_##name, IMPL_##name); } \
	LONG error = DetourTransactionCommit(); \
	if (error != NOERROR) { \
	    TRACE(_T("hook_init error: %#x\n"), error); \
    } \
	return error; \
}

#include "hooklist.h"
#undef HOOK_MANUALLY
#undef HOOK_DEFINE

//
#define HOOK_MANUALLY(rettype, name, argtype) ;
#define HOOK_DEFINE(rettype, name, argtype) \
	DetourDetach(&(PVOID&)ORIG_##name, IMPL_##name);
static void hook_term()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

#include "hooklist.h"

	LONG error = DetourTransactionCommit();

	if (error != NOERROR) {
		TRACE(_T("hook_term error: %#x\n"), error);
	}
}
#undef HOOK_DEFINE
#undef HOOK_MANUALLY

#else
#include "easyhook.h"
#ifdef _M_IX86
#pragma comment (lib, "easyhk32.lib")
#else
#pragma comment (lib, "easyhk64.lib")
#endif

#define HOOK_MANUALLY HOOK_DEFINE
#define HOOK_DEFINE(rettype, name, argtype) \
	rettype (WINAPI * ORIG_##name) argtype;

#include "hooklist.h"
#undef HOOK_DEFINE


#define HOOK_DEFINE(rettype, name, argtype) \
	HOOK_TRACE_INFO HOOK_##name = {0};	//����hook�ṹ

#include "hooklist.h"

#undef HOOK_DEFINE
#undef HOOK_MANUALLY
//
#define HOOK_MANUALLY(rettype, name, argtype) ;
#define HOOK_DEFINE(rettype, name, argtype) \
	ORIG_##name = name;
#pragma optimize("s", on)
static void hook_initinternal()
{
#include "hooklist.h"
}
#pragma optimize("", on)
#undef HOOK_DEFINE
#undef HOOK_MANUALLY

#define FORCE(expr) {if(!SUCCEEDED(NtStatus = (expr))) goto ERROR_ABORT;}

#define HOOK_DEFINE(rettype, name, argtype) \
	if (&ORIG_##name) { \
	FORCE(LhInstallHook((PVOID&)ORIG_##name, IMPL_##name, (PVOID)0, &HOOK_##name)); \
	*(void**)&ORIG_##name =  (void*)HOOK_##name.Link->OldProc; \
	FORCE(LhSetExclusiveACL(ACLEntries, 0, &HOOK_##name)); }
#define HOOK_MANUALLY(rettype, name, argtype) ;

static LONG hook_init()
{
	ULONG ACLEntries[1] = {0};
	NTSTATUS NtStatus;

#include "hooklist.h"
#undef HOOK_DEFINE

	FORCE(LhSetGlobalExclusiveACL(ACLEntries, 0));
	return NOERROR;

ERROR_ABORT:
	TRACE(_T("hook_init error: %#x\n"), NtStatus);
	return 1;
}
#undef HOOK_DEFINE
#undef HOOK_MANUALLY

#define HOOK_DEFINE(rettype, name, argtype);
#define HOOK_MANUALLY(rettype, name, argtype) \
	LONG hook_demand_##name(bool bForce = false){ \
	NTSTATUS NtStatus; \
	ULONG ACLEntries[1] = { 0 }; \
	if (bForce) {  \
		memset((void*)&HOOK_##name, 0, sizeof(HOOK_TRACE_INFO));  \
	}  \
	if (&ORIG_##name) {	\
	FORCE(LhInstallHook((PVOID&)ORIG_##name, IMPL_##name, (PVOID)0, &HOOK_##name)); \
	*(void**)&ORIG_##name =  (void*)HOOK_##name.Link->OldProc; \
	FORCE(LhSetExclusiveACL(ACLEntries, 0, &HOOK_##name)); } \
	return NOERROR; \
	ERROR_ABORT: \
	TRACE(_T("hook_init error: %#x\n"), NtStatus); \
	return 1; \
	}

#include "hooklist.h"
#undef HOOK_MANUALLY


#undef HOOK_MANUALLY
#undef HOOK_DEFINE

#define HOOK_MANUALLY(rettype, name, argtype) ;
#define HOOK_DEFINE(rettype, name, argtype) \
	ORIG_##name = name;
#pragma optimize("s", on)
static LONG hook_term()
{
	#include "hooklist.h"
	LhUninstallAllHooks();
	return LhWaitForPendingRemovals();
}
#endif
#pragma optimize("", on)
#undef HOOK_DEFINE
#undef HOOK_MANUALLY

//---

CTlsData<CThreadLocalInfo>	g_TLInfo;
HINSTANCE					g_hinstDLL;
LONG						g_bHookEnabled;
#ifdef _DEBUG
HANDLE						g_hfDbgText;
#endif

//void InstallManagerHook();
//void RemoveManagerHook();

//#include "APITracer.hpp"

//�x�[�X�A�h���X��ς����������[�h�������Ȃ�
#if _DLL
#pragma comment(linker, "/base:0x06540000")
#endif

BOOL AddEasyHookEnv()
{
	TCHAR dir[MAX_PATH];
	int dirlen = GetModuleFileName(GetDLLInstance(), dir, MAX_PATH);
	LPTSTR lpfilename=dir+dirlen;
	while (lpfilename>dir && *lpfilename!=_T('\\') && *lpfilename!=_T('/')) --lpfilename;
	*lpfilename = 0;
	_tcscat(dir, _T(";"));
	dirlen = _tcslen(dir);
	int sz=GetEnvironmentVariable(_T("path"), NULL, 0);
	LPTSTR lpPath = (LPTSTR)malloc((sz+dirlen+2)*sizeof(TCHAR));
	GetEnvironmentVariable(_T("path"), lpPath, sz);
	if (!_tcsstr(lpPath, dir))
	{
		if (lpPath[sz-2]!=_T(';'))
			_tcscat(lpPath, _T(";"));
		_tcscat(lpPath, dir);
		SetEnvironmentVariable(_T("path"), lpPath);
	}
	free(lpPath);
	return true;
}

extern FT_Int * g_charmapCache;
extern BYTE* AACache, *AACacheFull;	
extern HFONT g_alterGUIFont;

extern COLORCACHE* g_AACache2[MAX_CACHE_SIZE]; 
HANDLE hDelayHook = 0;
BOOL WINAPI  DllMain(HINSTANCE instance, DWORD reason, LPVOID lpReserved)
{
	BOOL IsUnload = false, bEnableDW = true;
	switch(reason) {
	case DLL_PROCESS_ATTACH:
#ifdef DEBUG
		//MessageBox(0, L"Load", NULL, MB_OK);
#endif
		g_dllInstance = instance;
		//����������
		//DLL_PROCESS_DETACH�ł͂���̋t���ɂ���
		//1. CRT�֐��̏�����
		//2. �N���e�B�J���Z�N�V�����̏�����
		//3. TLS�̏���
		//4. CGdippSettings�̃C���X�^���X�����AINI�ǂݍ���
		//5. ExcludeModule�`�F�b�N
		// 6. FreeType���C�u�����̏�����
		// 7. FreeTypeFontEngine�̃C���X�^���X����
		// 8. API���t�b�N
		// 9. Manager��GetProcAddress���t�b�N

		//1
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
		//_CrtSetBreakAlloc(100);

		//Opera��~�܂�`
		//Assert(GetModuleHandleA("opera.exe") == NULL);
		
		setlocale(LC_ALL, "");
		g_hinstDLL = instance;
		

//APITracer::Start(instance, APITracer::OutputFile);

		//2, 3
		CCriticalSectionLock::Init();
		COwnedCriticalSectionLock::Init();
		CThreadCounter::Init();
		if (!g_TLInfo.ProcessInit()) {
			return FALSE;
		}

		//4
		{
			CGdippSettings* pSettings = CGdippSettings::CreateInstance();
			if (!pSettings || !pSettings->LoadSettings(instance)) {
				CGdippSettings::DestroyInstance();
				return FALSE;
			}
			IsUnload = IsProcessUnload();
			bEnableDW = pSettings->DirectWrite();
		}
		if (!IsUnload) hook_initinternal();	//�����ص�ģ��Ͳ����κ�����

		//5
		if (!IsProcessExcluded() && !IsUnload) {
			//6 �` 9
			// FreeType
			//�µĻ���
// 			for (int i=0;i<CACHE_SIZE;i++)
// 				g_AACache2[i] = new COLORCACHE;//����Ĭ�ϵ�20������

			//g_charmapCache = (FT_Int*)malloc(100*sizeof(FT_Int));
			//memset(g_charmapCache, 0xff, 100*sizeof(FT_Int));
			//AACache = new BYTE[CACHE_SIZE *3 *256 * 256];
			//AACacheFull = new BYTE[CACHE_SIZE *3 *256 * 256];
			//memset(AACache, 0xcc, CACHE_SIZE *3 *256 * 256);
			//memset(AACacheFull, 0xcc, CACHE_SIZE *3 *256 * 256);
			if (!FontLInit()) {
				return FALSE;
			}
			g_pFTEngine = new FreeTypeFontEngine;
			if (!g_pFTEngine) {
				return FALSE;
			}
			
			//if (!AddEasyHookEnv()) return FALSE;	//fail to load easyhook
			InterlockedExchange(&g_bHookEnabled, TRUE);
			if (hook_init()!=NOERROR)
				return FALSE;
			//hook d2d if already loaded
			if (bEnableDW && IsWindowsVistaOrGreater())	//vista or later
			{
				//ORIG_LdrLoadDll = LdrLoadDll;
				//MessageBox(0, L"Test", NULL, MB_OK);
				HookD2DDll();
				//hook_demand_LdrLoadDll();
			}
//			InstallManagerHook();
		}
		//��õ�ǰ����ģʽ

		if (IsUnload)
		{
			HANDLE mutex_offical = OpenMutex(MUTEX_ALL_ACCESS, false, _T("{46AD3688-30D0-411e-B2AA-CB177818F428}"));
			HANDLE mutex_gditray2 = OpenMutex(MUTEX_ALL_ACCESS, false, _T("Global\\MacType"));
			if (!mutex_gditray2)
				mutex_gditray2 = OpenMutex(MUTEX_ALL_ACCESS, false, _T("MacType"));
			HANDLE mutex_CompMode = OpenMutex(MUTEX_ALL_ACCESS, false, _T("Global\\MacTypeCompMode"));
			if (!mutex_CompMode)			
				mutex_CompMode = OpenMutex(MUTEX_ALL_ACCESS, false, _T("MacTypeCompMode"));
			BOOL HookMode = (mutex_offical || (mutex_gditray2 && mutex_CompMode)) || (!mutex_offical && !mutex_gditray2);	//�Ƿ��ڼ���ģʽ��
			CloseHandle(mutex_CompMode);
			CloseHandle(mutex_gditray2);
			CloseHandle(mutex_offical);
			if (!HookMode)	//�Ǽ���ģʽ�£��ܾ�����
				return false;
		}

//APITracer::Finish();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		g_TLInfo.ThreadTerm();
		break;
	case DLL_PROCESS_DETACH:
//		RemoveManagerHook();
		if (InterlockedExchange(&g_bHookEnabled, FALSE) && lpReserved == NULL) {	//����ǽ�����ֹ������Ҫ�ͷ�
			hook_term();
			//delete AACacheFull;
			//delete AACache;
// 			for (int i=0;i<CACHE_SIZE;i++)
// 				delete g_AACache2[i];	//�������
			//free(g_charmapCache);
		}
#ifndef DEBUG
		if (lpReserved != NULL) return true;
#endif
		
		if (g_pFTEngine) {
			delete g_pFTEngine;
		}
		//if (g_alterGUIFont)
		//	DeleteObject(g_alterGUIFont);
		FontLFree();
/*
#ifndef _WIN64
		__FUnloadDelayLoadedDLL2("easyhk32.dll");
#else
		__FUnloadDelayLoadedDLL2("easyhk64.dll");
#endif*/

		CGdippSettings::DestroyInstance();
		g_TLInfo.ProcessTerm();
		CCriticalSectionLock::Term();
		COwnedCriticalSectionLock::Term();
		break;
	}
	return TRUE;
}
//EOF
