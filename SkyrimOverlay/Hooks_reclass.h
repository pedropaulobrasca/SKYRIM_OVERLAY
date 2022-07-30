#include "Functions.h"

int ix;
DWORD EntityObjStart = 0x0;
DWORD EntlistJmpBack = 0x0;
DWORD noclipJmpBack = 0x0;
DWORD axis = 0x0;
DWORD zAxisPtr = 0x0;
bool alreadyThere = false;

// Generated using ReClass 2016

class playerEnt;
class namePtrClass;
class N00000216;
class N00000328;

class playerEnt
{
public:
	char pad_0x0000[0x24]; //0x0000
	namePtrClass* namePtr; //0x0024 
	char pad_0x0028[0xC]; //0x0028
	vec3 coords; //0x0034 
	char pad_0x0040[0x400]; //0x0040

}; //Size=0x0440

class namePtrClass
{
public:
	char pad_0x0000[0x88]; //0x0000
	char* name; //0x0088 
	char pad_0x008C[0x378]; //0x008C

}; //Size=0x0404

class N00000216
{
public:
	char pad_0x0000[0x24]; //0x0000
	N00000328* N00000220; //0x0024 
	char pad_0x0028[0x418]; //0x0028

}; //Size=0x0440

class N00000328
{
public:
	char pad_0x0000[0x404]; //0x0000

}; //Size=0x0404

playerEnt * ents[255];
playerEnt * entsptr;

__declspec(naked) void entityhook()
{
	__asm {
		mov edx, [esi + 0x34]
		mov eax, [esi + 0x38]
		mov EntityObjStart, esi
		pushad
	}

	__asm {
		mov eax, EntityObjStart
		mov[entsptr], eax
	}

	if (entsptr == nullptr)
	{
		goto GIVE_UP;
	}

	alreadyThere = false;

	for (ix = 0; ix < 254; ix++)
	{
		if (ents[ix] == entsptr)
		{
			alreadyThere = true;
			break;
		}
	}

	if (alreadyThere)
	{
		goto GIVE_UP;
	}
	else
	{
		for (ix = 0; ix < 254; ix++)
		{
			if (ents[ix] == 0)
			{
				ents[ix] = entsptr;
				break;
			}
		}
	}

GIVE_UP:
	__asm {
		popad
		jmp[EntlistJmpBack]
	}
}

__declspec(naked) void nocliphook()
{
	__asm {
		mov zAxisPtr, esi
		mov edx, [edx + 0x78]
		fld dword ptr[edx + 0x0C]
		pushad
	}
	

	__asm {
		popad
		jmp[noclipJmpBack]
	}
}