#pragma once

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>

static void suiDebugMsgBox(char characters[])
{
	MessageBox(NULL, characters, "Title", MB_OK);
}

static void suiDebugMsgBoxAddress(DWORD addr)
{
	char szBuffer[1024];
	sprintf_s(szBuffer, "Address: %02x", addr);
	MessageBox(NULL, szBuffer, "Title", MB_OK);
}

static MODULEINFO suiGetModuleInfo(char * moduleName)
{
	MODULEINFO modInfo = { 0 };
	HMODULE hModule = GetModuleHandle(moduleName);

	if (hModule == 0)
		return modInfo;

	GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));
	return modInfo;
}

static DWORD suiGetBaseAddressForModuleInfo(char * moduleName)
{
	MODULEINFO modInfo = { 0 };
	HMODULE hModule = GetModuleHandle(moduleName);

	if (hModule == 0)
		return (DWORD)0;

	GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));
	return (DWORD)modInfo.lpBaseOfDll;
}

static void UnprotectModule(HMODULE p_Module)
{
	//This function was provided by Orfeasz
	PIMAGE_DOS_HEADER s_Header = (PIMAGE_DOS_HEADER)p_Module;
	PIMAGE_NT_HEADERS s_NTHeader = (PIMAGE_NT_HEADERS)((DWORD)p_Module + s_Header->e_lfanew);

	SIZE_T s_ImageSize = s_NTHeader->OptionalHeader.SizeOfImage;

	DWORD s_OldProtect;
	VirtualProtect((LPVOID)p_Module, s_ImageSize, PAGE_EXECUTE_READWRITE, &s_OldProtect);
}

static bool Hook(DWORD targetToHook, void * ourFunction, DWORD * returnAddress, int overrideLenght)
{
	if (overrideLenght < 5)
		return false;

	*returnAddress = targetToHook + overrideLenght;

	DWORD curProtectionFlag;
	VirtualProtect((void*)targetToHook, overrideLenght, PAGE_EXECUTE_READWRITE, &curProtectionFlag);
	memset((void*)targetToHook, 0x90, overrideLenght);
	DWORD relativeAddress = ((DWORD)ourFunction - (DWORD)targetToHook) - 5;

	*(BYTE*)targetToHook = 0xE9;
	*(DWORD*)((DWORD)targetToHook + 1) = relativeAddress;

	DWORD temp;
	VirtualProtect((void*)targetToHook, overrideLenght, curProtectionFlag, &temp);
	return true;
}

static bool Hook(DWORD targetToHook, void * ourFunction, int overrideLenght)
{
	if (overrideLenght < 5)
		return false;

	DWORD curProtectionFlag;
	VirtualProtect((void*)targetToHook, overrideLenght, PAGE_EXECUTE_READWRITE, &curProtectionFlag);
	memset((void*)targetToHook, 0x90, overrideLenght);
	DWORD relativeAddress = ((DWORD)ourFunction - (DWORD)targetToHook) - 5;

	*(BYTE*)targetToHook = 0xE9;
	*(DWORD*)((DWORD)targetToHook + 1) = relativeAddress;

	DWORD temp;
	VirtualProtect((void*)targetToHook, overrideLenght, curProtectionFlag, &temp);
	return true;
}
