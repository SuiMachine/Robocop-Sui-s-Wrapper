#include <windows.h>
#include <d3d8.h>
#include <math.h>
#include "SuisHacks.h"
#include "Direct3D8Wrapper.h"
#include "Direct3DDevice8Wrapper.h"
#include "CameraHacks.h"
#include "inireader/IniReader.h"

//Sources used from:
//https://bitbucket.org/andrewcooper/windower_open

bool bForceWindowedMode;
bool bDirect3D8DisableMaximizedWindowedModeShim;
bool bFPSLimit;
float fFPSLimit;
int iWidth = 1600;
int iHeight = 900;
int iPositionX = 300;
int iPositionY = 0;
int iAnisotropicLevel = 0;
float fAspectRatio = 1.33333f;
float fFogMultiplier = 1.0f;
DWORD dwStyleOverride = WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

struct d3d8_dll
{
    HMODULE dll;
    FARPROC DebugSetMute;
    FARPROC Direct3D8EnableMaximizedWindowedModeShim;
    FARPROC Direct3DCreate8;
    FARPROC ValidatePixelShader;
    FARPROC ValidateVertexShader;
} d3d8;

__declspec(naked) void _DebugSetMute() { _asm { jmp[d3d8.DebugSetMute] } }
__declspec(naked) void _Direct3D8EnableMaximizedWindowedModeShim() { _asm { jmp[d3d8.Direct3D8EnableMaximizedWindowedModeShim] } }
__declspec(naked) void _Direct3DCreate8() { _asm { jmp[d3d8.Direct3DCreate8] } }
__declspec(naked) void _ValidatePixelShader() { _asm { jmp[d3d8.ValidatePixelShader] } }
__declspec(naked) void _ValidateVertexShader() { _asm { jmp[d3d8.ValidateVertexShader] } }
IDirect3D8*	(WINAPI *OriginalDirect3DCreate8) (UINT SDKVersion);

IDirect3D8* WINAPI Direct3DCreate8Callback(UINT SDKVersion)
{
    IDirect3D8* Direct3D = OriginalDirect3DCreate8(SDKVersion);
    IDirect3D8* WrappedDirect3D = new Direct3D8Wrapper(Direct3D);
    return WrappedDirect3D;
}

HRESULT Direct3DDevice8Wrapper::Present(CONST RECT *pSourceRect, CONST RECT *pDestRect, HWND hDestWindowOverride, CONST RGNDATA *pDirtyRegion)
{
    if (bFPSLimit)
    {
        static LARGE_INTEGER PerformanceCount1;
        static LARGE_INTEGER PerformanceCount2;
        static bool bOnce1 = false;
        static double targetFrameTime = 1000.0 / fFPSLimit;
        static double t = 0.0;
        static DWORD i = 0;

        if (!bOnce1)
        {
            bOnce1 = true;
            QueryPerformanceCounter(&PerformanceCount1);
            PerformanceCount1.QuadPart = PerformanceCount1.QuadPart >> i;
        }

        while (true)
        {
            QueryPerformanceCounter(&PerformanceCount2);
            if (t == 0.0)
            {
                LARGE_INTEGER PerformanceCount3;
                static bool bOnce2 = false;

                if (!bOnce2)
                {
                    bOnce2 = true;
                    QueryPerformanceFrequency(&PerformanceCount3);
                    i = 0;
                    t = 1000.0 / (double)PerformanceCount3.QuadPart;
                    auto v = t * 2147483648.0;
                    if (60000.0 > v)
                    {
                        while (true)
                        {
                            ++i;
                            v *= 2.0;
                            t *= 2.0;
                            if (60000.0 <= v)
                                break;
                        }
                    }
                }
                SleepEx(0, 1);
                break;
            }

            if (((double)((PerformanceCount2.QuadPart >> i) - PerformanceCount1.QuadPart) * t) >= targetFrameTime)
                break;

            SleepEx(0, 1);
        }
        QueryPerformanceCounter(&PerformanceCount2);
        PerformanceCount1.QuadPart = PerformanceCount2.QuadPart >> i;
        return Direct3DDevice8->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    }
    else
        return Direct3DDevice8->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

void ForceWindowed(D3DPRESENT_PARAMETERS *pPresentationParameters)
{
    HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, &info);
    int DesktopResX = info.rcMonitor.right - info.rcMonitor.left;
    int DesktopResY = info.rcMonitor.bottom - info.rcMonitor.top;

    int left = (int)(((float)DesktopResX / 2.0f) - ((float)pPresentationParameters->BackBufferWidth / 2.0f));
    int top = (int)(((float)DesktopResY / 2.0f) - ((float)pPresentationParameters->BackBufferHeight / 2.0f));

    pPresentationParameters->Windowed = true;

    SetWindowPos(pPresentationParameters->hDeviceWindow, HWND_NOTOPMOST, left, top, pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight, SWP_SHOWWINDOW);
}

HRESULT Direct3D8Wrapper::CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DDevice8 **ppReturnedDeviceInterface) {
	IDirect3DDevice8* Direct3DDevice8;

	if (bForceWindowedMode)
		ForceWindowed(pPresentationParameters);

	HRESULT Result = Direct3D8->CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, &Direct3DDevice8);
	auto returnedDeviceInterface = new Direct3DDevice8Wrapper(&Direct3DDevice8, pPresentationParameters);
	returnedDeviceInterface->FogEndMultiplier = fFogMultiplier;
	returnedDeviceInterface->AnisotropicLevel = iAnisotropicLevel;
	*ppReturnedDeviceInterface = returnedDeviceInterface;

    return Result;
}

HRESULT Direct3DDevice8Wrapper::Reset(D3DPRESENT_PARAMETERS *pPresentationParameters) {
    OutputDebugString("Device reset called.");

    if (bForceWindowedMode)
        ForceWindowed(pPresentationParameters);

    return Direct3DDevice8->Reset(PresentationParameters);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
#if _DEBUG
	AllocConsole();
	FILE *file = nullptr;
	freopen_s(&file, "CONOUT$", "w", stdout);
#endif	

    char path[MAX_PATH];
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        CopyMemory(path + GetSystemDirectory(path, MAX_PATH - 9), "\\d3d8.dll", 10);
        d3d8.dll = LoadLibrary(path);
        if (d3d8.dll == false)
        {
            ExitProcess(0);
        }

        d3d8.DebugSetMute = GetProcAddress(d3d8.dll, "DebugSetMute");
        d3d8.Direct3D8EnableMaximizedWindowedModeShim = GetProcAddress(d3d8.dll, "Direct3D8EnableMaximizedWindowedModeShim");
        d3d8.Direct3DCreate8 = GetProcAddress(d3d8.dll, "Direct3DCreate8");
        d3d8.ValidatePixelShader = GetProcAddress(d3d8.dll, "ValidatePixelShader");
        d3d8.ValidateVertexShader = GetProcAddress(d3d8.dll, "ValidateVertexShader");

        //wrapping Direct3DCreate8
        OriginalDirect3DCreate8 = (IDirect3D8 *(__stdcall *)(UINT))GetProcAddress(d3d8.dll, "Direct3DCreate8");
        d3d8.Direct3DCreate8 = (FARPROC)Direct3DCreate8Callback;

        //ini
        char path[MAX_PATH];
        HMODULE hm = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&Direct3DCreate8Callback, &hm);
        GetModuleFileNameA(hm, path, sizeof(path));
        *strrchr(path, '\\') = '\0';
        strcat_s(path, "\\d3d8.ini");
		CIniReader configReader(path);

		//Original variables
        bForceWindowedMode = configReader.ReadInteger("MAIN", "ForceWindowedMode", 0) != 0;
        bDirect3D8DisableMaximizedWindowedModeShim = configReader.ReadInteger("MAIN", "Direct3D8DisableMaximizedWindowedModeShim", 0) != 0;
        fFPSLimit = static_cast<float>(configReader.ReadInteger("MAIN", "FPSLimit", 0));
		bForceWindowedMode = configReader.ReadInteger("MAIN", "ForceWindowedMode", 0) != 0;

		//Robocop Variables
		iWidth = configReader.ReadInteger("MAIN", "Width", 1280);
		iHeight = configReader.ReadInteger("MAIN", "Height", 960);
		iPositionX = configReader.ReadInteger("MAIN", "PositionX", 0);
		iPositionY = configReader.ReadInteger("MAIN", "PositionY", 0);
		fFogMultiplier = configReader.ReadFloat("MAIN", "FogEndMultiplier", 1);
		iAnisotropicLevel = configReader.ReadInteger("MAIN", "AnisotropicLevel", 0);


        if (fFPSLimit)
            bFPSLimit = true;

		HMODULE baseModule = GetModuleHandle(NULL);
		UnprotectModule(baseModule);

		//Window's Width x Height
		*(int*)((DWORD)baseModule + 0x2E12E) = iWidth -16;
		*(int*)((DWORD)baseModule + 0x2E141) = iHeight - 38;

		//X and Y of Window and Style override
		*(int*)((DWORD)baseModule + 0x302B9) = iPositionX;
		*(int*)((DWORD)baseModule + 0x302B4) = iPositionY;
		*(DWORD*)((DWORD)baseModule + 0x302BE) = dwStyleOverride;

		float fovMultiplier = configReader.ReadFloat("MAIN", "FovMultiplier", 1);
		CameraHacks(baseModule, fovMultiplier, iWidth * 1.0f / iHeight);

		printf("Override window resolution: %d x %d\nwith a start positionposition of x=%d, y=%d and style = 0x%04X\n", iWidth, iHeight, iPositionX, iPositionY, dwStyleOverride);

        if (bDirect3D8DisableMaximizedWindowedModeShim)
        {
            auto addr = (uintptr_t)GetProcAddress(d3d8.dll, "Direct3D8EnableMaximizedWindowedModeShim");
            if (addr)
            {
                DWORD Protect;
                VirtualProtect((LPVOID)(addr + 6), 4, PAGE_EXECUTE_READWRITE, &Protect);
                *(unsigned*)(addr + 6) = 0;
                *(unsigned*)(*(unsigned*)(addr + 2)) = 0;
                VirtualProtect((LPVOID)(addr + 6), 4, Protect, &Protect);
                bForceWindowedMode = 0;
            }
        }
    }
    break;

    case DLL_PROCESS_DETACH:
        FreeLibrary(d3d8.dll);
        break;
    }
    return TRUE;
}