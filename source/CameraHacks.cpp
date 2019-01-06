#include "CameraHacks.h"

HMODULE pBaseModule;
DWORD pCameraHackReturn;
float fFOVMultiplier;
float fAspectratio;

void __declspec(naked) cameraDetour()
{
	//Camera struct (ebx + var):
	//0x0 Unknown
	//0x4 Unknown
	//0x8 Width
	//0xC Height
	//0x10 Unknown
	//0x14 Uknown (float)
	//0x18 Uknown (float)
	//0x1C Uknown (float)
	//0x20 Aspect ratio
	//0x24 Uknown (float)

	__asm
	{
		fld [ebx + 0x24]
		fmul DS:[fFOVMultiplier]
		fstp [ebx + 0x24]
		fld DS:[fAspectratio]
		fstp[ebx + 0x20]
		mov eax, ebx
		push esi
		push edi
		mov ecx, [eax]
		jmp[pCameraHackReturn]
	}
}

CameraHacks::CameraHacks(HMODULE baseModule, float FOVMultiplier, float AspectRatio)
{
	fFOVMultiplier = FOVMultiplier * (AspectRatio / 1.3333333333f);
	fAspectratio = AspectRatio;
	pBaseModule = baseModule;
	Hook((DWORD)pBaseModule + 0xAB278, cameraDetour, &pCameraHackReturn, 6);
}

CameraHacks::~CameraHacks()
{
}
