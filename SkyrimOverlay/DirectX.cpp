#include "DirectX.h"
#include <Psapi.h>
#include "modulemgr.h"
#include "main.h"
#include "Hooks_reclass.h"

directx_t DirectX;
ID3DXFont* pFont;
LPD3DXFONT font_interface = NULL;
IDirect3D9Ex* p_Object = 0;
IDirect3DDevice9Ex* p_Device = 0;
D3DPRESENT_PARAMETERS p_Params;
RECT rc;

bool bInitialized = false;

bool bKeys[255];
bool bKeyPrev[255];

bool bMenuEnabled = true;
bool nameEspEnabled = false;
bool noclipEnabled = false;
DWORD dwOldProtect;
DWORD viewMatrix = 0x01B3EA10;
float relX, relY, relZ, newX, newY, newZ;

//atualizar quando o jogo for atualizado
MODULEINFO modInfo = ModMgr->getModuleInfo("TESV.exe");
unsigned long gameResolution = (unsigned long)modInfo.lpBaseOfDll + 0xECF5B0;
unsigned long godMode = (unsigned long)modInfo.lpBaseOfDll + 0x17398C4;
unsigned long gravity = (unsigned long)modInfo.lpBaseOfDll + 0xED65B4;

DWORD wayPointX = 0x1B40298;
DWORD wayPointY = 0x1B4029C;
DWORD wayPointZ = 0x1B402A0;

#define MENUITEMS 5
bool bMenuItems[MENUITEMS];
int iSelectedItem = 0;
wchar_t wMenuItems[MENUITEMS][255] =
{
	L"ESP",
	L"NOCLIP",
	L"MAP TELEPORT",
	L"ITEM DUPE",
	L"EXIT"
};

void DrawString(char* String, int x, int y, int a, int r, int g, int b, ID3DXFont* font)
{
	RECT FontPos;
	FontPos.left = x;
	FontPos.top = y;
	font->DrawTextA(0, String, strlen(String), &FontPos, DT_NOCLIP, D3DCOLOR_ARGB(a, r, g, b));
}

bool WorldToScreen(vec3 pos, vec2 &screen, float matrix[16], int windowWidth, int windowHeight)
{
	//Matrix-vector Product, multiplying world(eye) coordinates by projection matrix = clipCoords
	vec4 clipCoords;
	clipCoords.x = pos.x*matrix[0] + pos.y*matrix[1] + pos.z*matrix[2] + matrix[3];
	clipCoords.y = pos.x*matrix[4] + pos.y*matrix[5] + pos.z*matrix[6] + matrix[7];
	clipCoords.z = pos.x*matrix[8] + pos.y*matrix[9] + pos.z*matrix[10] + matrix[11];
	clipCoords.w = pos.x*matrix[12] + pos.y*matrix[13] + pos.z*matrix[14] + matrix[15];

	if (clipCoords.w < 0.1f)
		return false;

	//perspective division, dividing by clip.W = Normalized Device Coordinates
	vec3 NDC;
	NDC.x = clipCoords.x / clipCoords.w;
	NDC.y = clipCoords.y / clipCoords.w;
	NDC.z = clipCoords.z / clipCoords.w;

	screen.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
	screen.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
	return true;
}

void DirectxFunctions::DirectXInit(HWND hwnd)
{
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &DirectX.Object)))
		exit(1);

	ZeroMemory(&DirectX.Param, sizeof(DirectX.Param));
	DirectX.Param.Windowed = true;
	DirectX.Param.BackBufferFormat = D3DFMT_A8R8G8B8;
	DirectX.Param.BackBufferHeight = Overlay.Height;
	DirectX.Param.BackBufferWidth = Overlay.Width;
	DirectX.Param.EnableAutoDepthStencil = true;
	DirectX.Param.AutoDepthStencilFormat = D3DFMT_D16;
	DirectX.Param.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	DirectX.Param.SwapEffect = D3DSWAPEFFECT_DISCARD;
	

	if (FAILED(DirectX.Object->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &DirectX.Param, 0, &DirectX.Device)))
		exit(1);


	D3DXCreateFont(DirectX.Device, 20, 0, FW_BOLD, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &DirectX.Font);
	D3DXCreateFont(DirectX.Device, 13, 0, 0, 0, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &DirectX.espFont);

	if (!DirectX.Line)
		D3DXCreateLine(DirectX.Device, &DirectX.Line);
}

void DirectxFunctions::RenderDirectX()
{
	DirectX.Device->BeginScene();
	if (GetForegroundWindow() == Target.Window)
	{
		for (int i = 0; i < 255; i++)
		{
			if (GetAsyncKeyState(i) != 0)
			{
				if (bKeyPrev[i] == false)
				{
					bKeyPrev[i] = true;
					bKeys[i] = true;
				}
				else
				{
					bKeys[i] = false;
				}
			}
			else
			{
				bKeys[i] = false;
				bKeyPrev[i] = false;
			}
		}
		if (bKeys[VK_INSERT])
		{
			bMenuEnabled = !bMenuEnabled;
		}

		if (noclipEnabled)
		{
			axis = zAxisPtr + 0xE8;
			
			if (GetAsyncKeyState(VK_SPACE))
			{
				*(float*)axis += 0.5f;
			}
			
			relX = 0.0f;
			relY = 0.0f;
			relZ = 0.0f;

			if (GetAsyncKeyState(VK_UP))
			{
				relZ += 0.5f;
			}

			if (GetAsyncKeyState(VK_DOWN))
			{
				relZ -= 0.5f;
			}

			if (GetAsyncKeyState(VK_LEFT))
			{
				relX -= 0.5f;
			}
			
			if (GetAsyncKeyState(VK_RIGHT))
			{
				relX += 0.5f;
			}

			newX = (*(float*)viewMatrix        )*relX + (*(float*)(viewMatrix + 0x10))*relY + (*(float*)(viewMatrix + 0x20))*relZ;
			newY = (*(float*)(viewMatrix + 0x4))*relX + (*(float*)(viewMatrix + 0x14))*relY + (*(float*)(viewMatrix + 0x24))*relZ;
			newZ = (*(float*)(viewMatrix + 0x8))*relX + (*(float*)(viewMatrix + 0x18))*relY + (*(float*)(viewMatrix + 0x28))*relZ;
			
			axis = zAxisPtr + 0xE0;
			*(float*)axis += newX;
			axis += 0x4;
			*(float*)axis += newY;
			axis += 0x4;
			*(float*)axis += newZ;
			
			Sleep(1);
		}

		char* entityName;
		float Matrix[16];
		int rWidth, rHeight;

		if (nameEspEnabled)
		{
			memcpy(&rWidth, (PBYTE*)gameResolution, sizeof(rWidth));
			memcpy(&rHeight, (PBYTE*)(gameResolution + 0x4), sizeof(rHeight));
			memcpy(&Matrix, (PBYTE*)viewMatrix, sizeof(Matrix));

			for (int i = 0; i < 254; i++)
			{
				if (ents[i] != 0)
				{
					vec2 vScreen;
					if (WorldToScreen(ents[i]->coords, vScreen, Matrix, rWidth, rHeight))
					{
						if (ents[i] != 0)
						{
							entityName = ents[i]->namePtr->name;

							DrawString(entityName, vScreen.x-2, vScreen.y, 5, 255, 0, 0, DirectX.espFont);
							DrawString(entityName, vScreen.x, vScreen.y, 5, 255, 0, 0, DirectX.espFont);
							DrawString(entityName, vScreen.x-1, vScreen.y+1, 5, 255, 0, 0, DirectX.espFont);
							DrawString(entityName, vScreen.x-1, vScreen.y+1, 5, 255, 0, 0, DirectX.espFont);
							DrawString(entityName, vScreen.x-1, vScreen.y, 255, 255, 0, 0, DirectX.espFont);
						}
					}
				}
			}
			Sleep(1);
		}

		if (bMenuEnabled)
		{
			//EXAMPLES to draw boxes or rectangles!
			//DrawRect(p_Device, 20, 20, 200, 200, D3DCOLOR_ARGB(186, 206, 168, 168));
			//DrawBorderBox(10, 10, 200, 200, 1, 255, 9, 255, 0);
			//DrawFilledRectangle(10, 10, 170, 200, 255, 30, 92, 192);

			RECT pos;
			pos.right = 1001;
			pos.bottom = 1001;

			pos.left = 20;
			pos.top = 20;

			//bg
			Drawing::FilledRect(18, 20, 205, 200, D3DCOLOR_ARGB(5, 255, 0, 0));
			//outer rect
			Drawing::BorderedRect(17, 19, 205, 200, 1, 1, 1, 1, D3DCOLOR_ARGB(255, 255, 255, 255));
			Drawing::FilledRect(17, 19, 205, 19, D3DCOLOR_ARGB(255, 255, 255, 255));
			DirectX.Font->DrawTextW(NULL, L"Skyrim Tutorial", -1, &pos, 0, D3DCOLOR_ARGB(255, 5, 5, 5));
			pos.top += 20;

			wchar_t swf[255];
			wchar_t swf2[255];

			for (int i = 0; i < MENUITEMS; i++)
			{
				D3DCOLOR color;
				if (iSelectedItem == i)
					color = D3DCOLOR_ARGB(237, 243, 243, 24);
				else
					color = D3DCOLOR_ARGB(237, 198, 24, 24);
				swprintf(swf, wMenuItems[i]);
				swprintf(swf2, L" %s", bMenuItems[i] ? L"ON" : L"OFF");
				wcscat(swf, swf2);
				DirectX.Font->DrawTextW(NULL, swf, -1, &pos, 0, color);

				pos.top += 16;
			}

			if (bKeys[VK_HOME])
			{
				if (iSelectedItem > 0)
				{
					iSelectedItem--;
				}
			}
			if (bKeys[VK_END])
			{
				if (iSelectedItem < MENUITEMS - 1)
				{
					iSelectedItem++;
				}
			}

			if (bKeys[VK_DELETE] || bKeys[VK_NEXT])
			{
				bMenuItems[iSelectedItem] = !bMenuItems[iSelectedItem];

				//ESPs
				if (iSelectedItem == 0)
				{
					if (bMenuItems[iSelectedItem] == true)
					{
						//ON CODE
						nameEspEnabled = true;
					}
					else
					{
						//OFF
						nameEspEnabled = false;
					}
				}

				//NOCLIP
				if (iSelectedItem == 1)
				{
					if (bMenuItems[iSelectedItem] == true)
					{
						//ON CODE
						*(float*)gravity = 0.0f;
						*(int*)godMode = 256;
						noclipEnabled = true;
					}
					else
					{
						//OFF
						*(float*)gravity = 1.350000024f;
						*(int*)godMode = 0;
						noclipEnabled = false;
					}
				}

				//MAP TELEPORT
				if (iSelectedItem == 2)
				{
					if (bMenuItems[iSelectedItem] == true)
					{
						axis = zAxisPtr + 0xE0;
						*(float*)axis		  = *((float*)wayPointX) / 70;
						*(float*)(axis + 0x4) = *((float*)wayPointY) / 70;
						*(float*)(axis + 0x8) = *((float*)wayPointZ) / 70;
					}
					else
					{
						//OFF
					}
				}

				//MAP TELEPORT
				if (iSelectedItem == 3)
				{
					if (bMenuItems[iSelectedItem] == true)
					{
						VirtualProtect((PBYTE)0x47F89F, 3, PAGE_EXECUTE_READWRITE, &dwOldProtect);
						memcpy((PBYTE)0x47F89F, (PBYTE)("\x01\x41\x08"), 3);
						VirtualProtect((PBYTE)0x47F89F, 3, dwOldProtect, &dwOldProtect);
					}
					else
					{
						VirtualProtect((PBYTE)0x47F89F, 3, PAGE_EXECUTE_READWRITE, &dwOldProtect);
						memcpy((PBYTE)0x47F89F, (PBYTE)("\x29\x41\x08"), 3);
						VirtualProtect((PBYTE)0x47F89F, 3, dwOldProtect, &dwOldProtect);
					}
				}

				//EXIT
				if (iSelectedItem == 4)
				{
					if (bMenuItems[iSelectedItem] == true)
					{
						//ON CODE..we close the application
						//remove the dll from game
					}
				}
			}
		}
	}
	DirectX.Device->EndScene();
	DirectX.Device->PresentEx(0, 0, 0, 0, 0);
	DirectX.Device->Clear(0, 0, D3DCLEAR_TARGET, 0, 1.0f, 0);
}

//Print our pattern scan results if necessary
void MsgBoxAddy(DWORD addy)
{
	char szBuffer[1024];
	sprintf(szBuffer, "%02x", addy);
	MessageBox(NULL, szBuffer, "Title", MB_OK);

	//copy addy to clipboardd
	const char* output = szBuffer;
	const size_t len = strlen(output) + 1;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
	memcpy(GlobalLock(hMem), output, len);
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}

#pragma region Mid Function Hook/Code cave
/*Credits to InSaNe on MPGH for the original function*/
//We make Length at the end optional as most jumps will be 5 or less bytes
void PlaceJMP(BYTE *Address, DWORD jumpTo, DWORD length = 5)
{
	DWORD dwOldProtect, dwBkup, dwRelAddr;

	//give that address read and write permissions and store the old permissions at oldProtection
	VirtualProtect(Address, length, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	// Calculate the "distance" we're gonna have to jump - the size of the JMP instruction
	dwRelAddr = (DWORD)(jumpTo - (DWORD)Address) - 5;

	// Write the JMP opcode @ our jump position...
	*Address = 0xE9;

	// Write the offset to where we're gonna jump
	//The instruction will then become JMP ff002123 for example
	*((DWORD *)(Address + 0x1)) = dwRelAddr;

	// Overwrite the rest of the bytes with NOPs
	//ensuring no instruction is Half overwritten(To prevent any crashes)
	for (DWORD x = 0x5; x < length; x++)
		*(Address + x) = 0x90;

	// Restore the default permissions
	VirtualProtect(Address, length, dwOldProtect, &dwBkup);
}

#pragma region PATTERN SCANNING
//Get all module related info, this will include the base DLL. 
//and the size of the module
MODULEINFO GetModuleInfo(char *szModule)
{
	MODULEINFO modinfo = { 0 };
	HMODULE hModule = GetModuleHandle(szModule);
	if (hModule == 0)
		return modinfo;
	GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
	return modinfo;
}

DWORD FindPattern(char *module, char *pattern, char *mask)
{
	//Get all module related information
	MODULEINFO mInfo = GetModuleInfo(module);

	//Assign our base and module size
	//Having the values right is ESSENTIAL, this makes sure
	//that we don't scan unwanted memory and leading our game to crash
	DWORD base = (DWORD)mInfo.lpBaseOfDll;
	DWORD size = (DWORD)mInfo.SizeOfImage;

	//Get length for our mask, this will allow us to loop through our array
	DWORD patternLength = (DWORD)strlen(mask);

	for (DWORD i = 0; i < size - patternLength; i++)
	{
		bool found = true;
		for (DWORD j = 0; j < patternLength; j++)
		{
			//if we have a ? in our mask then we have true by default, 
			//or if the bytes match then we keep searching until finding it or not
			found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);
		}

		//found = true, our entire pattern was found
		//return the memory addy so we can write to it
		if (found)
		{
			return base + i;
		}
	}

	return NULL;
}

overlay_t Overlay;
target_t Target;

LRESULT CALLBACK WinProcedure(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_PAINT:
		DirectxFunctions::RenderDirectX();
		break;

	case WM_DESTROY:
		PostQuitMessage(1);
		break;

	default:
		return DefWindowProc(hWnd, Message, wParam, lParam);
		break;
	}
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	ProcessFunctions::CheckDWM();
	OverlayFunctions::GetTargetWindow();
	OverlayFunctions::CreateClass(WinProcedure, "overlay");
	OverlayFunctions::CreateWindowOverlay();
	DirectxFunctions::DirectXInit(Overlay.Window);

	for (;;)
	{
		if (PeekMessage(&Overlay.Message, Overlay.Window, 0, 0, PM_REMOVE))
		{
			DispatchMessage(&Overlay.Message);
			TranslateMessage(&Overlay.Message);
		}
		Sleep(1);
		OverlayFunctions::GetTargetWindow();
	}

	return 0;
}

DWORD WINAPI InitiateHooks(LPVOID param)
{
	//entityhook
	DWORD entityAddy = FindPattern("TESV.exe",
		"\x8B\x56\x34\x8B\x46\x38\x8B\x4E\x3C\x8D\x7E\x34", "xxxxxxxxxxxx");
	//MsgBoxAddy(entityAddy);
	EntlistJmpBack = entityAddy + 0x6;
	PlaceJMP((BYTE*)entityAddy, (DWORD)entityhook, 6);

	//noclip
	DWORD noclipAddy = FindPattern("TESV.exe",
		"\x8B\x52\x78\xD9\x42\x0C\x8B\x01", "xxxxxxxx");
	//MsgBoxAddy(entityAddy);
	noclipJmpBack = noclipAddy + 0x6;
	PlaceJMP((BYTE*)noclipAddy, (DWORD)nocliphook, 6);

	return NULL;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		hInstance = hModule;
		CreateThread(0, NULL, ThreadProc, (LPVOID)L"X", NULL, NULL);
		CreateThread(0, 0, InitiateHooks, 0, 0, 0);
	}
	return TRUE;
}


