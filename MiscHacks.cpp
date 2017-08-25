/*
Syn's AyyWare Framework 2015
*/

#define _CRT_SECURE_NO_WARNINGS

#include "MiscHacks.h"
#include "Interfaces.h"
#include "RenderManager.h"

#include <time.h>

template<class T, class U>
inline T clamp(T in, U low, U high)
{
	if (in <= low)
		return low;
	else if (in >= high)
		return high;
	else
		return in;
}

inline float bitsToFloat(unsigned long i)
{
	return *reinterpret_cast<float*>(&i);
}

inline float FloatNegate(float f)
{
	return bitsToFloat(FloatBits(f) ^ 0x80000000);
}

Vector AutoStrafeView;

void CMiscHacks::Init()
{
	// Any init
}

void CMiscHacks::Draw()
{
	// Any drawing	
	// Spams
	switch (Menu::Window.MiscTab.OtherChatSpam.GetIndex())
	{
	case 0:
		// No Chat Spam
		break;
	case 1:
		// Namestealer
		ChatSpamName();
		break;
	case 2:
		// Regular
		ChatSpamRegular();
		break;
	case 3:
		// Interwebz
		ChatSpamInterwebz();
		break;
	case 4:
		// Aimware
		ChatSpamDisperseName();
		break;
	case 5:
		// c0caine
		ChatSpamAdvertise();
		break;
	}
}

void CMiscHacks::Move(CUserCmd *pCmd, bool &bSendPacket)
{
	// Any Move Stuff
	
	// Bhop
	switch (Menu::Window.MiscTab.OtherAutoJump.GetState())
	{
	case 0:
		break;
	case 1:
		AutoJump(pCmd);
		break;
	}

	// AutoStrafe
	Interfaces::Engine->GetViewAngles(AutoStrafeView);
	switch (Menu::Window.MiscTab.OtherAutoStrafe.GetIndex())
	{
	case 0:
		// Off
		break;
	case 1:
		LegitStrafe(pCmd);
		break;
	case 2:
		RageStrafe(pCmd);
		break;
	}

	//Fake Lag
	if (Menu::Window.MiscTab.FakeLagEnable.GetState())
		Fakelag(pCmd, bSendPacket);
}

static __declspec(naked) void __cdecl Invoke_NET_SetConVar(void* pfn, const char* cvar, const char* value)
{
	__asm 
	{
		push    ebp
			mov     ebp, esp
			and     esp, 0FFFFFFF8h
			sub     esp, 44h
			push    ebx
			push    esi
			push    edi
			mov     edi, cvar
			mov     esi, value
			jmp     pfn
	}
}
// Color(0, 255, 0, 255),
// Color(255, 0, 0, 255),


void DECLSPEC_NOINLINE NET_SetConVar(const char* value, const char* cvar)
{
	static DWORD setaddr = Utilities::Memory::FindPattern("engine.dll", (PBYTE)"\x8D\x4C\x24\x1C\xE8\x00\x00\x00\x00\x56", "xxxxx????x");
	if (setaddr != 0) 
	{
		void* pvSetConVar = (char*)setaddr;
		Invoke_NET_SetConVar(pvSetConVar, cvar, value);
	}
}

void change_name(const char* name)
{
	if (Interfaces::Engine->IsInGame() && Interfaces::Engine->IsConnected())
		NET_SetConVar(name, "name");
}

void CMiscHacks::AutoJump(CUserCmd *pCmd)
{
	if (pCmd->buttons & IN_JUMP && GUI.GetKeyState(VK_SPACE))
	{
		int iFlags = hackManager.pLocal()->GetFlags();
		if (!(iFlags & FL_ONGROUND))
			pCmd->buttons &= ~IN_JUMP;

		if (hackManager.pLocal()->GetVelocity().Length() <= 50)
		{
			pCmd->forwardmove = 450.f;
		}
	}
}

void CMiscHacks::LegitStrafe(CUserCmd *pCmd)
{
	IClientEntity* pLocal = hackManager.pLocal();
	if (!(pLocal->GetFlags() & FL_ONGROUND))
	{
		pCmd->forwardmove = 0.0f;

		if (pCmd->mousedx < 0)
		{
			pCmd->sidemove = -450.0f;
		}
		else if (pCmd->mousedx > 0)
		{
			pCmd->sidemove = 450.0f;
		}
	}
}

void CMiscHacks::RageStrafe(CUserCmd *pCmd)
{
	float strafespeed = Menu::Window.MiscTab.OtherStrafeSpeed.GetValue();

	IClientEntity* pLocal = hackManager.pLocal();

	bool bKeysPressed = true;
	if (GUI.GetKeyState(0x41) || GUI.GetKeyState(0x57) || GUI.GetKeyState(0x53) || GUI.GetKeyState(0x44)) bKeysPressed = false;

	if ((GetAsyncKeyState(VK_SPACE) && !(pLocal->GetFlags() & FL_ONGROUND)) && bKeysPressed)
	{
		pCmd->forwardmove = (1550.f * strafespeed) / pLocal->GetVelocity().Length2D();
		pCmd->sidemove = (pCmd->command_number % 2) == 0 ? -450.f : 450.f;
		if (pCmd->forwardmove > 450.f)
			pCmd->forwardmove = 450.f;
	}
}

Vector GetAutostrafeView()
{
	return AutoStrafeView;
}

void CMiscHacks::circlestrafer(CUserCmd * cmd, Vector& Originalview)
{
	int CircleKey = (GetAsyncKeyState('C'));
	IClientEntity* pLocal = hackManager.pLocal();
	Vector View(cmd->viewangles);
	float blackcock = 0;
	if (GUI.GetKeyState(CircleKey))
	{
		cmd->forwardmove = 450.f;
		int random = rand() % 100;
		int random2 = rand() % 1000;
		static bool dir;
		static float current_y = View.y;
		if (pLocal->GetVelocity().Length() > 50.f)
		{
			blackcock += 0.00007;
			current_y += 3 - blackcock;
		}
		else
		{
			blackcock = 0;
		}
		View.y = current_y;
		if (random == random2)
			View.y += random;
	}
	else
	{
		float blackcock = 0;
	}
}

void RotateMovement(CUserCmd* cmd, float rotation)
{
rotation = DEG2RAD(rotation);
float cosr, sinr;
cosr = cos(rotation);
sinr = sin(rotation);
float forwardmove, sidemove;
forwardmove = (cosr * cmd->forwardmove) - (sinr * cmd->sidemove);
sidemove = (sinr * cmd->forwardmove) - (cosr * cmd->sidemove);
cmd->forwardmove = forwardmove;
cmd->sidemove = sidemove;
}

void CMiscHacks::ChatSpamInterwebz()
{
	static clock_t start_t = clock();
	double timeSoFar = (double)(clock() - start_t) / CLOCKS_PER_SEC;
	if (timeSoFar < 0.001)
		return;

	static bool wasSpamming = true;
	//static std::string nameBackup = "INTERWEBZ";

	if (wasSpamming)
	{
		static bool useSpace = true;
		if (useSpace)
		{
			change_name ("---------------------------------------projectvoid---------------------------------------");
			useSpace = !useSpace;
		}
		else
		{
			change_name("---------------------------------------projectvoid---------------------------------------");
			useSpace = !useSpace;
		}
	}

	start_t = clock();
}

void CMiscHacks::ChatSpamDisperseName()
{
	static clock_t start_t = clock();
	double timeSoFar = (double)(clock() - start_t) / CLOCKS_PER_SEC;
	if (timeSoFar < 0.001)
		return;

	static bool wasSpamming = true;
	//static std::string nameBackup = "INTERWEBZ";

	if (wasSpamming)
	{
		static bool useSpace = true;
		if (useSpace)
		{
			change_name("™AIMWARE.NET");
			useSpace = !useSpace;
		}
		else
		{
			change_name("©AIMWARE.NET");
			useSpace = !useSpace;
		}
	}

	start_t = clock();
}

void CMiscHacks::ChatSpamAdvertise()
{
	static clock_t start_t = clock();
	double timeSoFar = (double)(clock() - start_t) / CLOCKS_PER_SEC;
	if (timeSoFar < 0.001)
		return;

	static bool wasSpamming = true;

	if (wasSpamming)
	{
		static bool useSpace = true;
		if (useSpace)
		{
			change_name("C0CAINE.XYZ");
			useSpace = !useSpace;
		}
		else
		{
			change_name("c0caine.xyz");
			useSpace = !useSpace;
		}
	}

	start_t = clock();
}

void CMiscHacks::ChatSpamName()
{
	static clock_t start_t = clock();
	double timeSoFar = (double)(clock() - start_t) / CLOCKS_PER_SEC;
	if (timeSoFar < 0.001)
		return;

	std::vector < std::string > Names;

	for (int i = 0; i < Interfaces::EntList->GetHighestEntityIndex(); i++)
	{
		// Get the entity
		IClientEntity *entity = Interfaces::EntList->GetClientEntity(i);

		player_info_t pInfo;
		// If it's a valid entity and isn't the player
		if (entity && hackManager.pLocal()->GetTeamNum() == entity->GetTeamNum() && entity != hackManager.pLocal())
		{
			ClientClass* cClass = (ClientClass*)entity->GetClientClass();

			// If entity is a player
			if (cClass->m_ClassID == (int)CSGOClassID::CCSPlayer)
			{
				if (Interfaces::Engine->GetPlayerInfo(i, &pInfo))
				{
					if (!strstr(pInfo.name, "GOTV"))
						Names.push_back(pInfo.name);
				}
			}
		}
	}

	static bool wasSpamming = true;
	//static std::string nameBackup = "INTERWEBZ.CC";

	int randomIndex = rand() % Names.size();
	char buffer[128];
	sprintf_s(buffer, "%s ", Names[randomIndex].c_str());

	if (wasSpamming)
	{
		change_name(buffer);
	}
	else
	{
		change_name ("p$i 1337");
	}

	start_t = clock();
}

void CMiscHacks::ChatSpamRegular()
{
	// Don't spam it too fast so you can still do stuff
	static clock_t start_t = clock();
	int spamtime = Menu::Window.MiscTab.OtherChatDelay.GetValue();
	double timeSoFar = (double)(clock() - start_t) / CLOCKS_PER_SEC;
	if (timeSoFar < spamtime)
		return;

	static bool holzed = true;

	if (Menu::Window.MiscTab.OtherTeamChat.GetState())
	{
		SayInTeamChat("INTERWEBZ.CC OWNS ME AND ALL");
	}
	else
	{
		SayInChat("INTERWEBZ.CC OWNS ME AND ALL");
	}

	start_t = clock();
}

void CMiscHacks::Fakelag(CUserCmd *pCmd, bool &bSendPacket)
{
	IClientEntity* pLocal = hackManager.pLocal();
	if (pCmd->buttons & IN_ATTACK) return;
	if (pLocal->GetVelocity() == Vector(0, 0, 0)) return;

	int iChoke = Menu::Window.MiscTab.FakeLagChoke.GetValue();

	static int iFakeLag = -1;
	iFakeLag++;

	if (iFakeLag <= iChoke && iFakeLag > -1)
	{
		bSendPacket = false;
	}
	else
	{
		bSendPacket = true;
		iFakeLag = -1;
	}
}