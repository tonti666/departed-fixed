#include "Hooks.h"
#include "Hacks.h"
#include "Chams.h"
#include "Menu.h"

#include "Interfaces.h"
#include "RenderManager.h"
#include "MiscHacks.h"
#include "CRC32.h"
#include "Resolver.h"
#include "IPrediction.h"

#include <math.h>
#include <ctime>
#include <intrin.h>
#define MDLCACHE_INTERFACE_VERSION "MDLCache004"
#ifndef IMDLCACHE_H
#define IMDLCACHE_H

#ifdef _WIN32
#pragma once
#endif
#endif
Vector LastAngleAA;
#define M_PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679821480865132823066470938446095505822317253594081284811174502841027019385211055596446229489549303819644288109756659334461284756482337867831652712019091456485669234603486104543266482133936072602491412737245870066063155881748815209209628292540917153643678925903600113305305488204665213841469519415116094330572703657595919530921861173819326117931051185480744623799627495673518857527248912279381830119491298336733624406566430860213949463952247371907021798609437027705392171762931767523846748184676694051320005681271452635608277857713427577896091736371787214684409012249534301465495853710507922796892589235420199561121290219608640344181598136297747713099605187072113499999983729780499510597317328160963185950244594553469083026425223082533446850352619311881710100031378387528865875332083814206171776691473035982534904287554687311595628638823537875937519577818577805321712268066130019278766111959092164201989380952572010654858632788659361533818279682303019520353018529689957736225994138912497217752834791315155748572424541506959508295331168617278558890750983817546374649393192550604009277016711390098488240128583616035637076601047101819429555961989467678374494482553797747268471040475346462080466842590694912933136770289891521047521620569660240580
inline float DegreesToRadians(float Angle) { return Angle * M_PI / 180.0f; }

// Funtion Typedefs
typedef void(__thiscall* DrawModelEx_)(void*, void*, void*, const ModelRenderInfo_t&, matrix3x4*);
typedef void(__thiscall* PaintTraverse_)(PVOID, unsigned int, bool, bool);
typedef bool(__thiscall* InPrediction_)(PVOID);
typedef void(__stdcall *FrameStageNotifyFn)(ClientFrameStage_t);
typedef void(__thiscall* RenderViewFn)(void*, CViewSetup&, CViewSetup&, int, int);

using OverrideViewFn = void(__fastcall*)(void*, void*, CViewSetup*);
typedef float(__stdcall *oGetViewModelFOV)();

// Function Pointers to the originals
PaintTraverse_ oPaintTraverse;
DrawModelEx_ oDrawModelExecute;
FrameStageNotifyFn oFrameStageNotify;
OverrideViewFn oOverrideView;
RenderViewFn oRenderView;

// Hook function prototypes
void __fastcall PaintTraverse_Hooked(PVOID pPanels, int edx, unsigned int vguiPanel, bool forceRepaint, bool allowForce);
bool __stdcall Hooked_InPrediction();
void __fastcall Hooked_DrawModelExecute(void* thisptr, int edx, void* ctx, void* state, const ModelRenderInfo_t &pInfo, matrix3x4 *pCustomBoneToWorld);
bool __stdcall CreateMoveClient_Hooked(/*void* self, int edx,*/ float frametime, CUserCmd* pCmd);
void  __stdcall Hooked_FrameStageNotify(ClientFrameStage_t curStage);
void __fastcall Hooked_OverrideView(void* ecx, void* edx, CViewSetup* pSetup);
float __stdcall GGetViewModelFOV();
void __fastcall Hooked_RenderView(void* ecx, void* edx, CViewSetup &setup, CViewSetup &hudViewSetup, int nClearFlags, int whatToDraw);

// VMT Managers
namespace Hooks
{
	// VMT Managers
	Utilities::Memory::VMTManager VMTPanel; // Hooking drawing functions
	Utilities::Memory::VMTManager VMTClient; // Maybe CreateMove
	Utilities::Memory::VMTManager VMTClientMode; // CreateMove for functionality
	Utilities::Memory::VMTManager VMTModelRender; // DrawModelEx for chams
	Utilities::Memory::VMTManager VMTPrediction; // InPrediction for no vis recoil
	Utilities::Memory::VMTManager VMTPlaySound; // Autoaccept 
	Utilities::Memory::VMTManager VMTRenderView;
};
// Undo our hooks
void Hooks::UndoHooks()
{
	VMTPanel.RestoreOriginal();
	VMTPrediction.RestoreOriginal();
	VMTModelRender.RestoreOriginal();
	VMTClientMode.RestoreOriginal();
}

// Initialise all our hooks
void Hooks::Initialise()
{
	// Panel hooks for drawing to the screen via surface functions
	VMTPanel.Initialise((DWORD*)Interfaces::Panels);
	oPaintTraverse = (PaintTraverse_)VMTPanel.HookMethod((DWORD)&PaintTraverse_Hooked, Offsets::VMT::Panel_PaintTraverse);
	//Utilities::Log("Paint Traverse Hooked");

	// No Visual Recoi	l
	VMTPrediction.Initialise((DWORD*)Interfaces::Prediction);
	VMTPrediction.HookMethod((DWORD)&Hooked_InPrediction, 14);
	//Utilities::Log("InPrediction Hooked");

	// Chams
	VMTModelRender.Initialise((DWORD*)Interfaces::ModelRender);
	oDrawModelExecute = (DrawModelEx_)VMTModelRender.HookMethod((DWORD)&Hooked_DrawModelExecute, Offsets::VMT::ModelRender_DrawModelExecute);
	//Utilities::Log("DrawModelExecute Hooked");

	// Setup ClientMode Hooks
	VMTClientMode.Initialise((DWORD*)Interfaces::ClientMode);
	VMTClientMode.HookMethod((DWORD)CreateMoveClient_Hooked, 24);

	oOverrideView = (OverrideViewFn)VMTClientMode.HookMethod((DWORD)&Hooked_OverrideView, 18);
	VMTClientMode.HookMethod((DWORD)&GGetViewModelFOV, 35);

	// Setup client hooks
	VMTClient.Initialise((DWORD*)Interfaces::Client);
	oFrameStageNotify = (FrameStageNotifyFn)VMTClient.HookMethod((DWORD)&Hooked_FrameStageNotify, 36);

}

void MovementCorrection(CUserCmd* pCmd)
{

}

//---------------------------------------------------------------------------------------------------------
//                                         Hooked Functions
//---------------------------------------------------------------------------------------------------------

// Animated ClanTag
void SetClanTag(const char* tag, const char* name)
{
	static auto pSetClanTag = reinterpret_cast<void(__fastcall*)(const char*, const char*)>(((DWORD)Utilities::Memory::FindPattern("engine.dll", (PBYTE)"\x53\x56\x57\x8B\xDA\x8B\xF9\xFF\x15\x00\x00\x00\x00\x6A\x24\x8B\xC8\x8B\x30", "xxxxxxxxx????xxxxxx")));
	pSetClanTag(tag, name);
}
void NoClantag()
{
	SetClanTag("           ", "           ");
}

void ClanTag()
{
	static int counter = 0;
	switch (Menu::Window.MiscTab.OtherClantag.GetIndex())
	{
	case 0:
		// No 
		break;
	case 1:
	{
		SetClanTag("Departed.cf", "Departed.cf");
	}
	break;
	case 2:
	{SetClanTag("Departed > All", "Departed > All");
		
	}
	break;
	case 3:
	{
		static int motion = 0;
		int ServerTime = (float)Interfaces::Globals->interval_per_tick * hackManager.pLocal()->GetTickBase() * 3;

		if (counter % 48 == 0)
			motion++;
		int value = ServerTime % 17;
		switch (value) {
		case 0:SetClanTag("          ", "skeet.cc"); break;
		case 1:SetClanTag("         s", "skeet.cc"); break;
		case 2:SetClanTag("        sk", "skeet.cc"); break;
		case 3:SetClanTag("       ske", "skeet.cc"); break;
		case 4:SetClanTag("      skee", "skeet.cc"); break;
		case 5:SetClanTag("     skeet", "skeet.cc"); break;
		case 6:SetClanTag("    skeet.", "skeet.cc"); break;
		case 7:SetClanTag("   skeet.c", "skeet.cc"); break;
		case 8:SetClanTag(" skeet.cc", "skeet.cc"); break;
		case 9:SetClanTag("skeet.cc ", "skeet.cc"); break;
		case 10:SetClanTag("keet.cc  ", "skeet.cc"); break;
		case 11:SetClanTag("eet.cc   ", "skeet.cc"); break;
		case 12:SetClanTag("et.cc    ", "skeet.cc"); break;
		case 13:SetClanTag("t.cc     ", "skeet.cc"); break;
		case 14:SetClanTag(".cc      ", "skeet.cc"); break;
		case 15:SetClanTag("cc       ", "skeet.cc"); break;
		case 16:SetClanTag("c        ", "skeet.cc"); break;
		case 17:SetClanTag("         ", "skeet.cc"); break;
		}
		counter++;
	}
	break;
	case 4:
		// stainless
		SetClanTag("STAINLESS'", "STAINLESS'");
		break;
	case 5:
		SetClanTag("[VALV\xE1\xB4\xB1]", "Valve");
		break;
	case 6:
	{
		static int motion = 0;
		int ServerTime = (float)Interfaces::Globals->interval_per_tick * hackManager.pLocal()->GetTickBase() * 3;

		if (counter % 48 == 0)
			motion++;
		int value = ServerTime % 30;
		switch (value) {
		case 0:SetClanTag("                 ", "Project Void"); break;
		case 1:SetClanTag("                d", "Project Void"); break;
		case 2:SetClanTag("              d e", "Project Void"); break;
		case 3:SetClanTag("             d e ", "Project Void"); break;
		case 4:SetClanTag("            d e p", "Project Void"); break;
		case 5:SetClanTag("           d e p ", "Project Void"); break;
		case 6:SetClanTag("          d e p a", "Project Void"); break;
		case 7:SetClanTag("         d e p a ", "Project Void"); break;
		case 8:SetClanTag("        d e p a r", "Project Void"); break;
		case 9:SetClanTag("       d e p a r ", "Project Void"); break;
		case 10:SetClanTag("      d e p a r t", "Project Void"); break;
		case 11:SetClanTag("     d e p a r t ", "Project Void"); break;
		case 12:SetClanTag("    d e p a r t e", "Project Void"); break;
		case 13:SetClanTag("   d e p a r t e ", "Project Void"); break;
		case 14:SetClanTag(" d e p a r t e d", "Project Void"); break;
		case 15:SetClanTag("d e p a r t e d ", "Project Void"); break;
		case 16:SetClanTag(" e p a r t e d  ", "Project Void"); break;
		case 17:SetClanTag("e p a r t e d   ", "Project Void"); break;
		case 18:SetClanTag(" p a r t e d    ", "Project Void"); break;
		case 19:SetClanTag("p a r t e d     ", "Project Void"); break;
		case 20:SetClanTag(" a r t e d      ", "Project Void"); break;
		case 21:SetClanTag("a r t e d       ", "Project Void"); break;
		case 22:SetClanTag(" r t e d        ", "Project Void"); break;
		case 23:SetClanTag("r t e d         ", "Project Void"); break;
		case 24:SetClanTag(" t e d          ", "Project Void"); break;
		case 25:SetClanTag("t e d           ", "Project Void"); break;
		case 26:SetClanTag(" e d            ", "Project Void"); break;
		case 27:SetClanTag("e d             ", "Project Void"); break;
		case 28:SetClanTag(" d              ", "Project Void"); break;
		case 29:SetClanTag("d               ", "Project Void"); break;
		case 30:SetClanTag("                ", "Project Void"); break;
		}
		counter++;
	}
	break;
	}
}



// Rank Revealer
void MsgFunc_ServerRankRevealAll()
{
	using MsgFunc_ServerRankRevealAllFn = bool(__cdecl*)(float*);
	static MsgFunc_ServerRankRevealAllFn MsgFunc_ServerRankRevealAll = reinterpret_cast<MsgFunc_ServerRankRevealAllFn>((PDWORD)Utilities::Memory::FindPattern("client.dll", (PBYTE)"\x55\x8B\xEC\x8B\x0D\x00\x00\x00\x00\x68\x00\x00\x00\x00", "xxxxx????x????"));

	float fArray[3];
	fArray[0] = 0.f;
	fArray[1] = 0.f;
	fArray[2] = 0.f;

	MsgFunc_ServerRankRevealAll(fArray);
}

BYTE bMoveData[0x200];

void Prediction(CUserCmd* pCmd, IClientEntity* LocalPlayer)
{
	if (Interfaces::MoveHelper && Menu::Window.RageBotTab.AimbotEnable.GetState() && Menu::Window.RageBotTab.AccuracyPrediction.GetState() && LocalPlayer->IsAlive())
	{
		float curtime = Interfaces::Globals->curtime;
		float frametime = Interfaces::Globals->frametime;
		int iFlags = LocalPlayer->GetFlags();

		Interfaces::Globals->curtime = (float)LocalPlayer->GetTickBase() * Interfaces::Globals->interval_per_tick;
		Interfaces::Globals->frametime = Interfaces::Globals->interval_per_tick;

		Interfaces::MoveHelper->SetHost(LocalPlayer);

		Interfaces::GamePrediction->SetupMove(LocalPlayer, pCmd, nullptr, bMoveData);
		Interfaces::GameMovement->ProcessMovement(LocalPlayer, bMoveData);
		Interfaces::GamePrediction->FinishMove(LocalPlayer, pCmd, bMoveData);

		Interfaces::MoveHelper->SetHost(0);

		Interfaces::Globals->curtime = curtime;
		Interfaces::Globals->frametime = frametime;
		*LocalPlayer->GetPointerFlags() = iFlags;
	}
}

// Create moves
bool __stdcall CreateMoveClient_Hooked(/*void* self, int edx,*/ float frametime, CUserCmd* pCmd)
{
	if (!pCmd->command_number)
		return true;

	// Lowerbody Yaw
	/* if (Interfaces::Engine->IsInGame() && curStage == FRAME_NET_UPDATE_POSTDATAUPDATE_START)
	{
		if (pCmd->viewangles.y == GetLowerBodyTarget())
		{
			::LowerBodyWasUpdate = true;
		}
		else
		{
			G::LowerBodyWasUpdate = false;
		}
	} */

	// Create Move for Rank Revealer
	if (Interfaces::Engine->IsInGame() && Interfaces::Engine->IsConnected() && Interfaces::Globals->curtime) {
		if (pCmd->buttons & IN_SCORE && Menu::Window.MiscTab.RankRevealer.GetState()) {
			MsgFunc_ServerRankRevealAll();
		}
	}

	if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame())
	{

		PVOID pebp;
		__asm mov pebp, ebp;
		bool* pbSendPacket = (bool*)(*(DWORD*)pebp - 0x1C);
		bool& bSendPacket = *pbSendPacket;
		CClientState* pClient;
		INetChannel* pNet;
		CInput* pInput;

		if (Menu::Window.MiscTab.OtherClantag.GetIndex() > 0)
			ClanTag();

		//	CUserCmd* cmdlist = *(CUserCmd**)((DWORD)Interfaces::pInput + 0xEC);
		//	CUserCmd* pCmd = &cmdlist[sequence_number % 150];


			// Backup for safety
		Vector origView = pCmd->viewangles;
		Vector viewforward, viewright, viewup, aimforward, aimright, aimup;
		Vector qAimAngles;
		qAimAngles.Init(0.0f, pCmd->viewangles.y, 0.0f);
		AngleVectors(qAimAngles, &viewforward, &viewright, &viewup);

		// Do da hacks
		IClientEntity *pLocal = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
		if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame() && pLocal && pLocal->IsAlive())
			Hacks::MoveHacks(pCmd, bSendPacket);

		// Movement Fix
		qAimAngles.Init(0.0f, GetAutostrafeView().y, 0.0f);
		AngleVectors(qAimAngles, &viewforward, &viewright, &viewup);
		qAimAngles.Init(0.0f, pCmd->viewangles.y, 0.0f);
		AngleVectors(qAimAngles, &aimforward, &aimright, &aimup);
		Vector vForwardNorm;		Normalize(viewforward, vForwardNorm);
		Vector vRightNorm;			Normalize(viewright, vRightNorm);
		Vector vUpNorm;				Normalize(viewup, vUpNorm);

		// Original shit for movement correction
		float forward = pCmd->forwardmove;
		float right = pCmd->sidemove;
		float up = pCmd->upmove;
		if (forward > 450) forward = 450;
		if (right > 450) right = 450;
		if (up > 450) up = 450;
		if (forward < -450) forward = -450;
		if (right < -450) right = -450;
		if (up < -450) up = -450;
		pCmd->forwardmove = DotProduct(forward * vForwardNorm, aimforward) + DotProduct(right * vRightNorm, aimforward) + DotProduct(up * vUpNorm, aimforward);
		pCmd->sidemove = DotProduct(forward * vForwardNorm, aimright) + DotProduct(right * vRightNorm, aimright) + DotProduct(up * vUpNorm, aimright);
		pCmd->upmove = DotProduct(forward * vForwardNorm, aimup) + DotProduct(right * vRightNorm, aimup) + DotProduct(up * vUpNorm, aimup);

		// Angle normalisation
		if (Menu::Window.MiscTab.OtherSafeModeEnable.GetState())
		{
			GameUtils::NormaliseViewAngle(pCmd->viewangles);

			if (pCmd->viewangles.z != 0.0f)
			{
				pCmd->viewangles.z = 0.00;
			}

			if (pCmd->viewangles.x < -89 || pCmd->viewangles.x > 89 || pCmd->viewangles.y < -180 || pCmd->viewangles.y > 180)
			{
				GameUtils::NormaliseViewAngle(pCmd->viewangles);
				Beep(750, 800);
				if (pCmd->viewangles.x < -89 || pCmd->viewangles.x > 89 || pCmd->viewangles.y < -180 || pCmd->viewangles.y > 180)
				{
					pCmd->viewangles = origView;
					pCmd->sidemove = right;
					pCmd->forwardmove = forward;
				}
			}
		}

		if (pCmd->viewangles.x > 90)
		{
			pCmd->forwardmove = -pCmd->forwardmove;
		}

		if (pCmd->viewangles.x < -90)
		{
			pCmd->forwardmove = -pCmd->forwardmove;
		}

		if (bSendPacket)
			LastAngleAA = pCmd->viewangles;
	}

	return false;
}


// Paint Traverse Hooked function
void __fastcall PaintTraverse_Hooked(PVOID pPanels, int edx, unsigned int vguiPanel, bool forceRepaint, bool allowForce)
{

	if (Interfaces::Engine->IsInGame() && Interfaces::Engine->IsConnected())
	{
		if (Menu::Window.VisualsTab.OtherNoScope.GetState() && strcmp("HudZoom", Interfaces::Panels->GetName(vguiPanel)) == 0)
			return;

		if (Menu::Window.VisualsTab.OtherNoScope.GetState())
		{
			if (hackManager.pLocal()->IsAlive() && hackManager.pLocal()->IsScoped())
			{
				int width, height;
				Interfaces::Engine->GetScreenSize(width, height);
				Render::Line(width / 2, 0, width / 2, height, Color(0, 0, 0, 255));
				Render::Line(0, height / 2, width, height / 2, Color(0, 0, 0, 255));
			}
		}
	}


	oPaintTraverse(pPanels, vguiPanel, forceRepaint, allowForce);

	static unsigned int FocusOverlayPanel = 0;
	static bool FoundPanel = false;

	if (!FoundPanel)
	{
		PCHAR szPanelName = (PCHAR)Interfaces::Panels->GetName(vguiPanel);
		if (strstr(szPanelName, "MatSystemTopPanel"))
		{
			FocusOverlayPanel = vguiPanel;
			FoundPanel = true;
		}
	}
	else if (FocusOverlayPanel == vguiPanel)
	{	
		static float rainbow;
		rainbow += 0.005f;
		if (rainbow > 1.f) rainbow = 0.f;
		if (Menu::Window.MiscTab.Watermark.GetState() == 1)
		{
			Render::Text(10, 10, Color(255, 255, 255, 255), Render::Fonts::Watermark, "Depar");
			Render::Text(52, 10, Color(0, 161, 255, 255), Render::Fonts::Watermark, "ted.cf");
		}

		if (Menu::Window.MiscTab.OtherSafeModeEnable.GetState() == true)
		{
			Render::Text(10, 30, Color(33, 255, 0, 255), Render::Fonts::MenuBold, "Anti-Untrusted: On");
		}
		else
		{
			Render::Text(10, 30, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Anti-Untrusted: Off");
		}
		
		if (Menu::Window.RageBotTab.AntiAimEnable.GetState() == true) 
		{
		Render::Text(10, 40, Color(242, 255, 0, 255), Render::Fonts::MenuBold, "Anti Aim: On");
		}
		else
		{
			Render::Text(10, 40, Color(255, 255, 255, 255), Render::Fonts::MenuBold, "Anti Aim: Off");
		}
		if (Menu::Window.RageBotTab.AntiAimPitch.GetIndex() == 0)
		{
			Render::Text(10, 50, Color(255, 255, 255, 255), Render::Fonts::MenuBold, "Pitch: None");
		}

		if (Menu::Window.RageBotTab.AntiAimPitch.GetIndex() == 1)
		{
			Render::Text(10, 50, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Pitch: Down");
		}
		if (Menu::Window.RageBotTab.AntiAimPitch.GetIndex() == 2)
		{
			Render::Text(10, 50, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Pitch: Half Down");
		}
		if (Menu::Window.RageBotTab.AntiAimPitch.GetIndex() == 3)
		{
			Render::Text(10, 50, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Pitch: Meme");
		}
		if (Menu::Window.RageBotTab.AntiAimPitch.GetIndex() == 4)
		{
			Render::Text(10, 50, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Pitch: SMAC/Casual Safe");
		}
		if (Menu::Window.RageBotTab.AntiAimPitch.GetIndex() == 5)
		{
			Render::Text(10, 50, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Pitch: Jitter");
		}
		if (Menu::Window.RageBotTab.AntiAimPitch.GetIndex() == 6)
		{
			Render::Text(10, 50, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Pitch: Static Down");
		}
		if (Menu::Window.RageBotTab.AntiAimPitch.GetIndex() == 7)
		{
			Render::Text(10, 50, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Pitch: Static Jitter");
		}
		
		if (Menu::Window.RageBotTab.AntiAimYaw.GetIndex() == 0)
		{
			Render::Text(10, 60, Color(255, 255, 255, 255), Render::Fonts::MenuBold, "Yaw: None");
		}
		if (Menu::Window.RageBotTab.AntiAimYaw.GetIndex() == 1)
		{
			Render::Text(10, 60, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Yaw: Backward");
		}
		if (Menu::Window.RageBotTab.AntiAimYaw.GetIndex() == 2)
		{
			Render::Text(10, 60, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Yaw: Fake Backward");
		}
		if (Menu::Window.RageBotTab.AntiAimYaw.GetIndex() == 3)
		{
			Render::Text(10, 60, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Yaw: Fake Forward");
		}
		if (Menu::Window.RageBotTab.AntiAimYaw.GetIndex() == 4)
		{
			Render::Text(10, 60, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Yaw: Fake Sideways");
		}
		if (Menu::Window.RageBotTab.AntiAimYaw.GetIndex() == 5)
		{
			Render::Text(10, 60, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Yaw: Fake Static");
		}
		if (Menu::Window.RageBotTab.AntiAimYaw.GetIndex() == 6)
		{
			Render::Text(10, 60, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Yaw: Fake LBY");
		}
		if (Menu::Window.RageBotTab.AntiAimYaw.GetIndex() == 7)
		{
			Render::Text(10, 60, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Yaw: Fake Rotate");
		}
		if (Menu::Window.RageBotTab.AntiAimYaw.GetIndex() == 8)
		{
			Render::Text(10, 60, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Yaw: Fake IR");
		}
		if (Menu::Window.RageBotTab.AntiAimYaw.GetIndex() == 9)
		{
			Render::Text(10, 60, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Yaw: Fake Two Step");
		}
		if (Menu::Window.RageBotTab.AntiAimYaw.GetIndex() == 10)
		{
			Render::Text(10, 60, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Yaw: Fake BBC");
		}
		if (Menu::Window.RageBotTab.AntiAimYaw.GetIndex() == 11)
		{
			Render::Text(10, 60, Color(255, 4, 0, 255), Render::Fonts::MenuBold, "Yaw: Fake BBC");
		}
		
		if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame())
			Hacks::DrawHacks();

		
		Menu::DoUIFrame();
		
	}
}

// InPrediction Hooked Function
bool __stdcall Hooked_InPrediction()
{
	bool result;
	static InPrediction_ origFunc = (InPrediction_)Hooks::VMTPrediction.GetOriginalFunction(14);
	static DWORD *ecxVal = Interfaces::Prediction;
	result = origFunc(ecxVal);

	// If we are in the right place where the player view is calculated
	// Calculate the change in the view and get rid of it
	if (Menu::Window.VisualsTab.OtherNoVisualRecoil.GetState() && (DWORD)(_ReturnAddress()) == Offsets::Functions::dwCalcPlayerView)
	{
		IClientEntity* pLocalEntity = NULL;

		float* m_LocalViewAngles = NULL;

		__asm
		{
			MOV pLocalEntity, ESI
			MOV m_LocalViewAngles, EBX
		}

		Vector viewPunch = pLocalEntity->localPlayerExclusive()->GetViewPunchAngle();
		Vector aimPunch = pLocalEntity->localPlayerExclusive()->GetAimPunchAngle();

		m_LocalViewAngles[0] -= (viewPunch[0] + (aimPunch[0] * 2 * 0.4499999f));
		m_LocalViewAngles[1] -= (viewPunch[1] + (aimPunch[1] * 2 * 0.4499999f));
		m_LocalViewAngles[2] -= (viewPunch[2] + (aimPunch[2] * 2 * 0.4499999f));
		return true;
	}

	return result;
}

// DrawModelExec for chams and shit
void __fastcall Hooked_DrawModelExecute(void* thisptr, int edx, void* ctx, void* state, const ModelRenderInfo_t &pInfo, matrix3x4 *pCustomBoneToWorld)
{
	Color color;
	float flColor[3] = { 0.f };
	static IMaterial* CoveredLit = CreateMaterial(true);
	static IMaterial* OpenLit = CreateMaterial(false);
	static IMaterial* CoveredFlat = CreateMaterial(true, false);
	static IMaterial* OpenFlat = CreateMaterial(false, false);
	bool DontDraw = false;

	const char* ModelName = Interfaces::ModelInfo->GetModelName((model_t*)pInfo.pModel);
	IClientEntity* pModelEntity = (IClientEntity*)Interfaces::EntList->GetClientEntity(pInfo.entity_index);
	IClientEntity* pLocal = (IClientEntity*)Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());

	if (Menu::Window.VisualsTab.Active.GetState())
	{
		// Player Chams
		int ChamsStyle = Menu::Window.VisualsTab.OptionsChams.GetIndex();
		int HandsStyle = Menu::Window.VisualsTab.OtherNoHands.GetIndex();
		if (ChamsStyle != 0 && Menu::Window.VisualsTab.FiltersPlayers.GetState() && strstr(ModelName, "models/player"))
		{
			if (pLocal /* && (!Menu::Window.VisualsTab.FiltersEnemiesOnly.GetState() || pModelEntity->GetTeamNum() != pLocal->GetTeamNum())*/)
			{
				IMaterial *covered = ChamsStyle == 1 ? CoveredLit : CoveredFlat;
				IMaterial *open = ChamsStyle == 1 ? OpenLit : OpenFlat;

				IClientEntity* pModelEntity = (IClientEntity*)Interfaces::EntList->GetClientEntity(pInfo.entity_index);
				if (pModelEntity)
				{
					IClientEntity *local = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
					if (local)
					{
						if (pModelEntity->IsAlive() && pModelEntity->GetHealth() > 0 /*&& pModelEntity->GetTeamNum() != local->GetTeamNum()*/)
						{
							float alpha = 1.f;

							if (pModelEntity->HasGunGameImmunity())
								alpha = 0.5f;

							if (pModelEntity->GetTeamNum() == 2)
							{
								flColor[0] = 240.f / 255.f;
								flColor[1] = 30.f / 255.f;
								flColor[2] = 35.f / 255.f;
							}
							else
							{
								flColor[0] = 63.f / 255.f;
								flColor[1] = 72.f / 255.f;
								flColor[2] = 205.f / 255.f;
							}

							Interfaces::RenderView->SetColorModulation(flColor);
							Interfaces::RenderView->SetBlend(alpha);
							Interfaces::ModelRender->ForcedMaterialOverride(covered);
							oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

							if (pModelEntity->GetTeamNum() == 2)
							{
								flColor[0] = 247.f / 255.f;
								flColor[1] = 180.f / 255.f;
								flColor[2] = 20.f / 255.f;
							}
							else
							{
								flColor[0] = 32.f / 255.f;
								flColor[1] = 180.f / 255.f;
								flColor[2] = 57.f / 255.f;
							}

							Interfaces::RenderView->SetColorModulation(flColor);
							Interfaces::RenderView->SetBlend(alpha);
							Interfaces::ModelRender->ForcedMaterialOverride(open);
						}
						else
						{
							color.SetColor(255, 255, 255, 255);
							ForceMaterial(color, open);
						}
					}
				}
			}
		}
		else if (HandsStyle != 0 && strstr(ModelName, "arms"))
		{
			if (HandsStyle == 1)
			{
				DontDraw = true;
			}
			else if (HandsStyle == 2)
			{
				Interfaces::RenderView->SetBlend(0.3);
			}
			else if (HandsStyle == 3)
			{
				IMaterial *covered = ChamsStyle == 1 ? CoveredLit : CoveredFlat;
				IMaterial *open = ChamsStyle == 1 ? OpenLit : OpenFlat;
				if (pLocal)
				{
					if (pLocal->IsAlive())
					{
						int alpha = pLocal->HasGunGameImmunity() ? 150 : 255;

						if (pLocal->GetTeamNum() == 2)
							color.SetColor(240, 30, 35, alpha);
						else
							color.SetColor(63, 72, 205, alpha);

						ForceMaterial(color, covered);
						oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

						if (pLocal->GetTeamNum() == 2)
							color.SetColor(247, 180, 20, alpha);
						else
							color.SetColor(32, 180, 57, alpha);
					}
					else
					{
						color.SetColor(255, 255, 255, 255);
					}

					ForceMaterial(color, open);
				}
			}
			else
			{
				static int counter = 0;
				static float colors[3] = { 1.f, 0.f, 0.f };

				if (colors[counter] >= 1.0f)
				{
					colors[counter] = 1.0f;
					counter += 1;
					if (counter > 2)
						counter = 0;
				}
				else
				{
					int prev = counter - 1;
					if (prev < 0) prev = 2;
					colors[prev] -= 0.05f;
					colors[counter] += 0.05f;
				}

				Interfaces::RenderView->SetColorModulation(colors);
				Interfaces::RenderView->SetBlend(0.3);
				Interfaces::ModelRender->ForcedMaterialOverride(OpenLit);
			}
		}
		else if (ChamsStyle != 0 && Menu::Window.VisualsTab.FiltersWeapons.GetState() && strstr(ModelName, "_dropped.mdl"))
		{
			IMaterial *covered = ChamsStyle == 1 ? CoveredLit : CoveredFlat;
			color.SetColor(255, 255, 255, 255);
			ForceMaterial(color, covered);
		}
	}

	if (!DontDraw)
		oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);
	Interfaces::ModelRender->ForcedMaterialOverride(NULL);
}


// Hooked FrameStageNotify for removing visual recoil
void  __stdcall Hooked_FrameStageNotify(ClientFrameStage_t curStage)
{
	DWORD eyeangles = NetVar.GetNetVar(0xBFEA4E7B);
	IClientEntity *pLocal = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());

	if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame() && curStage == FRAME_RENDER_START)
	{

		if (pLocal->IsAlive())
		{
			if (*(bool*)((DWORD)Interfaces::pInput + 0xA5))
				*(Vector*)((DWORD)pLocal + 0x31C8) = LastAngleAA;
		}

		if (Menu::Window.VisualsTab.OtherThirdperson.GetState())
		{
			static bool rekt = false;
			if (!rekt)
			{
				ConVar* sv_cheats = Interfaces::CVar->FindVar("sv_cheats");
				SpoofedConvar* sv_cheats_spoofed = new SpoofedConvar(sv_cheats);
				sv_cheats_spoofed->SetInt(1);
				rekt = true;
			}
		}

		static bool rekt1 = false;
		if (Menu::Window.VisualsTab.OtherThirdperson.GetState() && pLocal->IsAlive() && pLocal->IsScoped() == 0)
		{
			if (!rekt1)
			{
				Interfaces::Engine->ClientCmd_Unrestricted("thirdperson");
				rekt1 = true;
			}
		}
		else if (!Menu::Window.VisualsTab.OtherThirdperson.GetState())
		{
			rekt1 = false;
		}

		static bool rekt = false;
		if (!Menu::Window.VisualsTab.OtherThirdperson.GetState() || pLocal->IsAlive() == 0 || pLocal->IsScoped())
		{
			if (!rekt)
			{
				Interfaces::Engine->ClientCmd_Unrestricted("firstperson");
				rekt = true;
			}
		}
		else if (Menu::Window.VisualsTab.OtherThirdperson.GetState() || pLocal->IsAlive() || pLocal->IsScoped() == 0)
		{
			rekt = false;
		}

		static bool meme = false;
		if (Menu::Window.VisualsTab.OtherThirdperson.GetState() && pLocal->IsScoped() == 0)
		{
			if (!meme)
			{
				Interfaces::Engine->ClientCmd_Unrestricted("thirdperson");
				meme = true;
			}
		}
		else if (pLocal->IsScoped())
		{
			meme = false;
		}

		static bool kek = false;
		if (Menu::Window.VisualsTab.OtherThirdperson.GetState() && pLocal->IsAlive())
		{
			if (!kek)
			{
				Interfaces::Engine->ClientCmd_Unrestricted("thirdperson");
				kek = true;
			}
		}
		else if (pLocal->IsAlive() == 0)
		{
			kek = false;
		}
	}

	if (curStage == FRAME_RENDER_START)
	{
		for (int i = 1; i <= Interfaces::Globals->maxClients; i++)
		{
			if (i == Interfaces::Engine->GetLocalPlayer()) continue;

			IClientEntity* pCurEntity = Interfaces::EntList->GetClientEntity(i);
			if (!pCurEntity) continue;

			*(int*)((uintptr_t)pCurEntity + 0xA30) = Interfaces::Globals->framecount; //we'll skip occlusion checks now
			*(int*)((uintptr_t)pCurEntity + 0xA28) = 0;//clear occlusion flags
		}
	}

	if (Interfaces::Engine->IsInGame())
	{
		ConVar* postprocessing = Interfaces::CVar->FindVar("sv_skyname");
		if (Menu::Window.VisualsTab.CustomSky.GetState())
		{
			postprocessing->SetValue("sky_csgo_night02");
		}
		else
		{
			postprocessing->SetValue("sky_csgo_night02b");
		}
	}

	if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame() && curStage == FRAME_NET_UPDATE_POSTDATAUPDATE_START) 
	{
		IClientEntity *pLocal = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());

		if (Menu::Window.SkinTab.SkinEnable.GetState() && pLocal)
		{
		IClientEntity* WeaponEnt = Interfaces::EntList->GetClientEntityFromHandle(pLocal->GetActiveWeaponHandle());
		// Utilities::Log("APPLY SKIN APPLY SKIN");
		int iBayonet = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_bayonet.mdl");
		int iButterfly = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_butterfly.mdl");
		int iFlip = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_flip.mdl");
		int iGut = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_gut.mdl");
		int iKarambit = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_karam.mdl");
		int iM9Bayonet = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_m9_bay.mdl");
		int iHuntsman = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_tactical.mdl");
		int iFalchion = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_falchion_advanced.mdl");
		int iDagger = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_push.mdl");
		int iBowie = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_survival_bowie.mdl");

		for (int i = 0; i <= Interfaces::EntList->GetHighestEntityIndex(); i++) // CHANGE
		{
			IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);

			IClientEntity* WeaponEnt = Interfaces::EntList->GetClientEntityFromHandle(pLocal->GetActiveWeaponHandle());

			CBaseCombatWeapon* Weapon = (CBaseCombatWeapon*)WeaponEnt;

			if (pEntity)
			{
				ULONG hOwnerEntity = *(PULONG)((DWORD)pEntity + 0x148);

				IClientEntity* pOwner = Interfaces::EntList->GetClientEntityFromHandle((HANDLE)hOwnerEntity);

				if (pOwner)
				{
					if (pOwner == pLocal)
					{
						std::string sWeapon = Interfaces::ModelInfo->GetModelName(pEntity->GetModel());

						if (!(sWeapon.find("models/weapons", 0) != std::string::npos))
							continue;

						if (sWeapon.find("c4_planted", 0) != std::string::npos)
							continue;

						if (sWeapon.find("thrown", 0) != std::string::npos)
							continue;

						if (sWeapon.find("smokegrenade", 0) != std::string::npos)
							continue;

						if (sWeapon.find("flashbang", 0) != std::string::npos)
							continue;

						if (sWeapon.find("fraggrenade", 0) != std::string::npos)
							continue;

						if (sWeapon.find("molotov", 0) != std::string::npos)
							continue;

						if (sWeapon.find("decoy", 0) != std::string::npos)
							continue;

						if (sWeapon.find("incendiarygrenade", 0) != std::string::npos)
							continue;

						if (sWeapon.find("ied", 0) != std::string::npos)
							continue;

						if (sWeapon.find("w_eq_", 0) != std::string::npos)
							continue;

						CBaseCombatWeapon* pWeapon = (CBaseCombatWeapon*)pEntity;

						ClientClass *pClass = Interfaces::Client->GetAllClasses();

						if (Menu::Window.SkinTab.SkinEnable.GetState())
						{
							int Model = Menu::Window.SkinTab.KnifeModel.GetIndex();

							int M41S = Menu::Window.SkinTab.M41SSkin.GetIndex();
							int M4A4 = Menu::Window.SkinTab.M4A4Skin.GetIndex();
							int AK47 = Menu::Window.SkinTab.AK47Skin.GetIndex();
							int AWP = Menu::Window.SkinTab.AWPSkin.GetIndex();
							int GLOCK = Menu::Window.SkinTab.GLOCKSkin.GetIndex();
							int USPS = Menu::Window.SkinTab.USPSSkin.GetIndex();
							int DEAGLE = Menu::Window.SkinTab.DEAGLESkin.GetIndex();
							int FIVE7 = Menu::Window.SkinTab.FIVESEVENSkin.GetIndex();
							int AUG = Menu::Window.SkinTab.AUGSkin.GetIndex();
							int FAMAS = Menu::Window.SkinTab.FAMASSkin.GetIndex();
							int G3SG1 = Menu::Window.SkinTab.G3SG1Skin.GetIndex();
							int Galil = Menu::Window.SkinTab.GALILSkin.GetIndex();
							int M249 = Menu::Window.SkinTab.M249Skin.GetIndex();
							int MAC10 = Menu::Window.SkinTab.MAC10Skin.GetIndex();
							int P90 = Menu::Window.SkinTab.P90Skin.GetIndex();
							int UMP45 = Menu::Window.SkinTab.UMP45Skin.GetIndex();
							int XM1014 = Menu::Window.SkinTab.XM1014Skin.GetIndex();
							int BIZON = Menu::Window.SkinTab.BIZONSkin.GetIndex();
							int MAG7 = Menu::Window.SkinTab.MAG7Skin.GetIndex();
							int NEGEV = Menu::Window.SkinTab.NEGEVSkin.GetIndex();
							int SAWEDOFF = Menu::Window.SkinTab.SAWEDOFFSkin.GetIndex();
							int TEC9 = Menu::Window.SkinTab.TECNINESkin.GetIndex();
							int P2000 = Menu::Window.SkinTab.P2000Skin.GetIndex();
							int MP7 = Menu::Window.SkinTab.MP7Skin.GetIndex();
							int MP9 = Menu::Window.SkinTab.MP9Skin.GetIndex();
							int NOVA = Menu::Window.SkinTab.NOVASkin.GetIndex();
							int P250 = Menu::Window.SkinTab.P250Skin.GetIndex();
							int SCAR20 = Menu::Window.SkinTab.SCAR20Skin.GetIndex();
							int SG553 = Menu::Window.SkinTab.SG553Skin.GetIndex();
							int SSG08 = Menu::Window.SkinTab.SSG08Skin.GetIndex();
							int Magnum = Menu::Window.SkinTab.DEAGLESkin.GetIndex();
							int DUAL = Menu::Window.SkinTab.DUALSSkin.GetIndex();


							int weapon = *pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex();

							switch (weapon)
							{
							case 7: // AK47 
							{
								switch (AK47)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 341;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 14;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 44;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 172;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 180;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 394;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 300;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 226;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 282;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 302;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 316;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 340;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 380;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 422;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 456;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 474;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 490;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 506;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 524;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 600;
									break;
								default:
									break;
								}
							}
							break;
							case 16: // M4A4
							{
								switch (M4A4)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 155;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 187;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 255;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 309;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 215;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 336;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 384;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 400;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 449;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 471;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 480;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 512;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 533;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 588;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 632;
									break;
								default:
									break;
								}
							}
							break;
							case 60: // M4A1
							{
								switch (M41S)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 60;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 430;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 77;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 235;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 254;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 189;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 301;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 217;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 257;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 321;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 326;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 360;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 383;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 440;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 445;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 497;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 548;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 587;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 631;
									break;
								default:
									break;
								}
							}
							break;
							case 9: // AWP
							{
								switch (AWP)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 174;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 344;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 84;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 30;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 51;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 72;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 181;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 259;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 395;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 212;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 227;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 251;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 279;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 424;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 446;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 451;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 475;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 525;
									break;
								default:
									break;
								}
							}
							break;
							case 61: // USP
							{
								switch (USPS)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 25;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 60;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 235;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 183;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 339;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 217;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 221;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 236;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 277;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 290;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 313;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 318;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 332;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 364;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 454;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 489;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 504;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 540;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 637;
									break;
								default:
									break;
								}
							}
							break;
							case 4: // Glock
							{
								switch (GLOCK)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 2;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 3;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 38;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 40;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 48;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 437;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 99;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 159;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 399;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 208;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 230;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 278;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 293;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 353;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 367;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 381;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 479;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 495;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 532;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 607;
									break;
								case 20:
									*pWeapon->FallbackPaintKit() = 586;
									break;
								default:
									break;
								}
							}
							break;
							case 1: // Deagle
							{
								switch (DEAGLE)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 37;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 347;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 468;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 469;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 5;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 232;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 17;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 40;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 61;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 90;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 235;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 185;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 231;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 237;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 397;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 328;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 273;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 296;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 351;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 425;
									break;
								case 20:
									*pWeapon->FallbackPaintKit() = 470;
									break;
								case 21:
									*pWeapon->FallbackPaintKit() = 509;
									break;
								case 22:
									*pWeapon->FallbackPaintKit() = 527;
									break;
								default:
									break;
								}
							}
							break;
							case 2: // Duals 
							{
								switch (DUAL)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 28; // Anodized Navy
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 36;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 43;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 46;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 47;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 153;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 491;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 190;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 248;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 249;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 220;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 396;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 261;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 276;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 307;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 330;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 447;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 450;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 528;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 544;
									break;
								case 20:
									*pWeapon->FallbackPaintKit() = 625;
									break;
								default:
									break;
								}
							}
							break;
							case 3: // Five Seven
							{
								switch (FIVE7)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 3;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 27;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 44;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 46;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 78;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 141;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 151;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 254;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 248;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 210;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 223;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 252;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 265;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 274;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 464;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 352;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 377;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 387;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 427;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 510;
									break;
								case 20:
									*pWeapon->FallbackPaintKit() = 530;
									break;
								default:
									break;
								}
							}
							break;
							case 8: // AUG
							{
								switch (AUG)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 9;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 33;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 280;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 305;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 375;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 442;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 444;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 455;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 507;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 541;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 601;
									break;
								default:
									break;
								}
							}
							break;
							case 10: // Famas
							{
								switch (FAMAS)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 22;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 47;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 92;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 429;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 154;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 178;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 194;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 244;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 218;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 260;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 288;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 371;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 477;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 492;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 529;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 604;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 626;
									break;
								default:
									break;
								}
							}
							break;
							case 11: // G3SG1
							{
								switch (G3SG1)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 8;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 6;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 27;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 46;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 72;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 74;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 147;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 235;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 170;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 195;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 229;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 294;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 465;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 464;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 382;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 438;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 493;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 511;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 545;
									break;
								default:
									break;
								}
							}
							break;
							case 13: // Galil
							{
								switch (Galil)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 5;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 22;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 83;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 428;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 76;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 119;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 235;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 235;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 398;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 192;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 308;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 216;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 237;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 241;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 264;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 297;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 379;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 460;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 478;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 494;
									break;
								case 20:
									*pWeapon->FallbackPaintKit() = 546;
									break;
								default:
									break;
								}
							}
							break;
							case 14: // M249
							{
								switch (M249)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 22;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 75;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 202;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 243;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 266;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 401;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 452;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 472;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 496;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 547;
									break;
								default:
									break;
								}
							}
							break;
							case 17: // Mac 10
							{
								switch (MAC10)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 101;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 3;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 32;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 5;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 17;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 38;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 433;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 98;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 157;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 188;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 337;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 246;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 284;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 310;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 333;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 343;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 372;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 402;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 498;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 534;
									break;
								default:
									break;
								}
							}
							break;
							case 19: // P90
							{
								switch (P90)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 342;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 20;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 22;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 100;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 67;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 111;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 124;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 156;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 234;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 169;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 175;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 182;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 244;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 228;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 283;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 311;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 335;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 359;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 486;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 516;
									break;
								case 20:
									*pWeapon->FallbackPaintKit() = 636;
									break;
								default:
									break;
								}
							}
							break;
							case 24: // UMP-45
							{
								switch (UMP45)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 37;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 5;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 15;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 17;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 436;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 70;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 93;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 169;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 175;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 193;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 392;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 281;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 333;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 362;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 441;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 488;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 556;
									break;
								default:
									break;
								}
							}
							break;
							case 25: // XM1014
							{
								switch (XM1014)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 166;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 238;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 27;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 42;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 96;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 95;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 135;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 151;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 235;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 235;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 169;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 205;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 240;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 251;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 393;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 320;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 314;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 348;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 370;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 407;
									break;
								case 20:
									*pWeapon->FallbackPaintKit() = 505;
									break;
								case 21:
									*pWeapon->FallbackPaintKit() = 521;
									break;
								case 22:
									*pWeapon->FallbackPaintKit() = 557;
									break;
								default:
									break;
								}
							}
							break;
							case 26: // Bizon
							{
								switch (BIZON)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 13;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 164;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 25;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 27;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 70;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 148;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 149;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 159;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 235;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 171;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 203;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 224;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 236;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 267;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 306;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 323;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 349;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 376;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 457;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 459;
									break;
								case 20:
									*pWeapon->FallbackPaintKit() = 508;
									break;
								case 21:
									*pWeapon->FallbackPaintKit() = 526;
									break;
								case 22:
									*pWeapon->FallbackPaintKit() = 542;
									break;
								default:
									break;
								}
							}
							break;
							case 27: // Mag 7
							{
								switch (MAG7)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 462;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 34;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 32;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 100;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 39;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 431;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 99;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 171;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 177;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 198;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 291;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 385;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 473;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 499;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 535;
									break;
								default:
									break;
								}
							}
							break;
							case 28: // Negev
							{
								switch (NEGEV)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 28;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 432;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 157;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 235;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 201;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 240;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 285;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 298;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 317;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 355;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 369;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 483;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 514;
									break;
								default:
									break;
								}
							}
							break;
							case 29: // Sawed Off
							{
								switch (SAWEDOFF)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 345;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 5;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 22;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 30;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 83;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 38;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 41;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 434;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 119;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 235;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 171;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 204;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 405;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 246;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 250;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 390;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 256;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 323;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 458;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 459;
									break;
								case 20:
									*pWeapon->FallbackPaintKit() = 517;
									break;
								case 21:
									*pWeapon->FallbackPaintKit() = 552;
									break;
								case 22:
									*pWeapon->FallbackPaintKit() = 638;
									break;
								default:
									break;
								}
							}
							break;
							case 30: // Tec 9
							{
								switch (TEC9)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 101;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 2;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 5;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 463;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 17;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 36;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 439;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 159;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 235;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 179;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 248;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 206;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 216;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 242;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 272;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 289;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 303;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 374;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 459;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 520;
									break;
								case 20:
									*pWeapon->FallbackPaintKit() = 539;
									break;
								case 21:
									*pWeapon->FallbackPaintKit() = 555;
									break;
								case 22:
									*pWeapon->FallbackPaintKit() = 614;
									break;
								default:
									break;
								}
							}
							break;
							case 32: // P2000
							{
								switch (P2000)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 104;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 32;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 21;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 25;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 36;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 485;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 38;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 71;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 95;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 184;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 211;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 338;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 246;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 275;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 327;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 346;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 357;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 389;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 442;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 443;
									break;
								case 20:
									*pWeapon->FallbackPaintKit() = 515;
									break;
								case 21:
									*pWeapon->FallbackPaintKit() = 550;
									break;
								case 22:
									*pWeapon->FallbackPaintKit() = 591;
									break;
								default:
									break;
								}
							}
							break;
							case 33: // MP7
							{
								switch (MP7)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 2;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 102;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 5;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 28;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 11;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 15;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 22;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 27;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 36;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 141;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 235;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 245;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 209;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 213;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 250;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 354;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 365;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 423;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 442;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 481;
									break;
								case 20:
									*pWeapon->FallbackPaintKit() = 500;
									break;
								case 21:
									*pWeapon->FallbackPaintKit() = 536;
									break;
								default:
									break;
								}
							}
							break;
							case 34: // MP9
							{
								switch (MP9)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 482;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 27;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 33;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 100;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 39;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 61;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 148;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 141;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 199;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 329;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 262;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 366;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 368;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 386;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 403;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 448;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 549;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 609;
									break;
								default:
									break;
								}
							}
							break;
							case 35: // Nova
							{
								switch (NOVA)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 3;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 166;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 164;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 25;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 62;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 99;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 107;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 158;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 170;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 191;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 214;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 225;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 263;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 286;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 294;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 299;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 356;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 450;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 484;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 537;
									break;
								default:
									break;
								}
							}
							break;
							case 36: // P250
							{
								switch (P250)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 102;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 34;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 162;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 15;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 164;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 27;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 77;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 99;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 168;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 258;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 207;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 219;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 404;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 230;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 271;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 295;
									break;
								case 16:
									*pWeapon->FallbackPaintKit() = 464;
									break;
								case 17:
									*pWeapon->FallbackPaintKit() = 358;
									break;
								case 18:
									*pWeapon->FallbackPaintKit() = 373;
									break;
								case 19:
									*pWeapon->FallbackPaintKit() = 388;
									break;
								case 20:
									*pWeapon->FallbackPaintKit() = 426;
									break;
								case 21:
									*pWeapon->FallbackPaintKit() = 466;
									break;
								case 22:
									*pWeapon->FallbackPaintKit() = 467;
									break;
								case 23:
									*pWeapon->FallbackPaintKit() = 501;
									break;
								case 24:
									*pWeapon->FallbackPaintKit() = 551;
									break;
								default:
									break;
								}
							}
							break;
							case 38: // Scar 20
							{
								switch (SCAR20)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 165;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 100;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 46;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 70;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 116;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 157;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 196;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 232;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 391;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 298;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 312;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 406;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 453;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 502;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 518;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 597;
									break;
								default:
									break;
								}
							}
							break;
							case 39: // SG553
							{
								switch (SG553)
								{
								case 1:
									*pWeapon->FallbackPaintKit() = 39;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 98;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 410;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 347;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 287;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 298;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 363;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 378;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 487;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 519;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 553;
									break;
								default:
									break;
								}
							}
							break;
							case 40: // SSG08
							{
								switch (SSG08)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 26;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 60;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 96;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 99;
									break;
								case 4:
									*pWeapon->FallbackPaintKit() = 157;
									break;
								case 5:
									*pWeapon->FallbackPaintKit() = 200;
									break;
								case 6:
									*pWeapon->FallbackPaintKit() = 222;
									break;
								case 7:
									*pWeapon->FallbackPaintKit() = 233;
									break;
								case 8:
									*pWeapon->FallbackPaintKit() = 253;
									break;
								case 9:
									*pWeapon->FallbackPaintKit() = 304;
									break;
								case 10:
									*pWeapon->FallbackPaintKit() = 319;
									break;
								case 11:
									*pWeapon->FallbackPaintKit() = 361;
									break;
								case 12:
									*pWeapon->FallbackPaintKit() = 503;
									break;
								case 13:
									*pWeapon->FallbackPaintKit() = 538;
									break;
								case 14:
									*pWeapon->FallbackPaintKit() = 554;
									break;
								case 15:
									*pWeapon->FallbackPaintKit() = 624;
									break;
								default:
									break;
								}
							}
							break;
							case 64: // Revolver
							{
								switch (Magnum)
								{
								case 0:
									*pWeapon->FallbackPaintKit() = 27;
									break;
								case 1:
									*pWeapon->FallbackPaintKit() = 12;
									break;
								case 2:
									*pWeapon->FallbackPaintKit() = 522;
									break;
								case 3:
									*pWeapon->FallbackPaintKit() = 523;
									break;
								default:
									break;
								}
							}
							break;
							default:
								break;
							}



							if (pEntity->GetClientClass()->m_ClassID == (int)CSGOClassID::CKnife)
							{
								if (Model == 0) // Bayonet
								{
									*pWeapon->ModelIndex() = iBayonet; // m_nModelIndex
									*pWeapon->ViewModelIndex() = iBayonet;
									*pWeapon->WorldModelIndex() = iBayonet + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 500;

									int Skin = Menu::Window.SkinTab.KnifeSkin.GetIndex();

									if (Skin == 0)
									{
										*pWeapon->FallbackPaintKit() = 0; // Forest DDPAT
									}
									else if (Skin == 1)
									{
										*pWeapon->FallbackPaintKit() = 12; // Crimson Web
									}
									else if (Skin == 2)
									{
										*pWeapon->FallbackPaintKit() = 27; // Bone Mask
									}
									else if (Skin == 3)
									{
										*pWeapon->FallbackPaintKit() = 38; // Fade
									}
									else if (Skin == 4)
									{
										*pWeapon->FallbackPaintKit() = 40; // Night
									}
									else if (Skin == 5)
									{
										*pWeapon->FallbackPaintKit() = 42; // Blue Steel
									}
									else if (Skin == 6)
									{
										*pWeapon->FallbackPaintKit() = 43; // Stained
									}
									else if (Skin == 7)
									{
										*pWeapon->FallbackPaintKit() = 44; // Case Hardened
									}
									else if (Skin == 8)
									{
										*pWeapon->FallbackPaintKit() = 59; // Slaughter
									}
									else if (Skin == 9)
									{
										*pWeapon->FallbackPaintKit() = 72; // Safari Mesh
									}
									else if (Skin == 10)
									{
										*pWeapon->FallbackPaintKit() = 77; // Boreal Forest
									}
									else if (Skin == 11)
									{
										*pWeapon->FallbackPaintKit() = 98; // Ultraviolet
									}
									else if (Skin == 12)
									{
										*pWeapon->FallbackPaintKit() = 143; // Urban Masked
									}
									else if (Skin == 13)
									{
										*pWeapon->FallbackPaintKit() = 175; // Scorched
									}
									else if (Skin == 14)
									{
										*pWeapon->FallbackPaintKit() = 323; // Rust Coat
									}
									else if (Skin == 15)
									{
										*pWeapon->FallbackPaintKit() = 409; // Tiger Tooth
									}
									else if (Skin == 16)
									{
										*pWeapon->FallbackPaintKit() = 410; // Damascus Steel
									}
									else if (Skin == 17)
									{
										*pWeapon->FallbackPaintKit() = 411; // Damascus Steel
									}
									else if (Skin == 18)
									{
										*pWeapon->FallbackPaintKit() = 413; // Marble Fade
									}
									else if (Skin == 19)
									{
										*pWeapon->FallbackPaintKit() = 414; // Rust Coat
									}
									else if (Skin == 20)
									{
										*pWeapon->FallbackPaintKit() = 415; // Doppler Ruby
									}
									else if (Skin == 21)
									{
										*pWeapon->FallbackPaintKit() = 416; // Doppler Sapphire
									}
									else if (Skin == 22)
									{
										*pWeapon->FallbackPaintKit() = 417; // Doppler Blackpearl
									}
									else if (Skin == 23)
									{
										*pWeapon->FallbackPaintKit() = 418; // Doppler Phase 1
									}
									else if (Skin == 24)
									{
										*pWeapon->FallbackPaintKit() = 419; // Doppler Phase 2
									}
									else if (Skin == 25)
									{
										*pWeapon->FallbackPaintKit() = 420; // Doppler Phase 3
									}
									else if (Skin == 26)
									{
										*pWeapon->FallbackPaintKit() = 421; // Doppler Phase 4
									}
									else if (Skin == 27)
									{
										*pWeapon->FallbackPaintKit() = 569; // Gamma Doppler Phase1
									}
									else if (Skin == 28)
									{
										*pWeapon->FallbackPaintKit() = 570; // Gamma Doppler Phase2
									}
									else if (Skin == 29)
									{
										*pWeapon->FallbackPaintKit() = 571; // Gamma Doppler Phase3
									}
									else if (Skin == 30)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Phase4
									}
									else if (Skin == 31)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Emerald
									}
									else if (Skin == 32)
									{
										*pWeapon->FallbackPaintKit() = 558; // Lore
									}
								}
								else if (Model == 1) // Bowie Knife
								{
									*pWeapon->ModelIndex() = iBowie; // m_nModelIndex
									*pWeapon->ViewModelIndex() = iBowie;
									*pWeapon->WorldModelIndex() = iBowie + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 514;

									int Skin = Menu::Window.SkinTab.KnifeSkin.GetIndex();

									if (Skin == 0)
									{
										*pWeapon->FallbackPaintKit() = 0; // Forest DDPAT
									}
									else if (Skin == 1)
									{
										*pWeapon->FallbackPaintKit() = 12; // Crimson Web
									}
									else if (Skin == 2)
									{
										*pWeapon->FallbackPaintKit() = 27; // Bone Mask
									}
									else if (Skin == 3)
									{
										*pWeapon->FallbackPaintKit() = 38; // Fade
									}
									else if (Skin == 4)
									{
										*pWeapon->FallbackPaintKit() = 40; // Night
									}
									else if (Skin == 5)
									{
										*pWeapon->FallbackPaintKit() = 42; // Blue Steel
									}
									else if (Skin == 6)
									{
										*pWeapon->FallbackPaintKit() = 43; // Stained
									}
									else if (Skin == 7)
									{
										*pWeapon->FallbackPaintKit() = 44; // Case Hardened
									}
									else if (Skin == 8)
									{
										*pWeapon->FallbackPaintKit() = 59; // Slaughter
									}
									else if (Skin == 9)
									{
										*pWeapon->FallbackPaintKit() = 72; // Safari Mesh
									}
									else if (Skin == 10)
									{
										*pWeapon->FallbackPaintKit() = 77; // Boreal Forest
									}
									else if (Skin == 11)
									{
										*pWeapon->FallbackPaintKit() = 98; // Ultraviolet
									}
									else if (Skin == 12)
									{
										*pWeapon->FallbackPaintKit() = 143; // Urban Masked
									}
									else if (Skin == 13)
									{
										*pWeapon->FallbackPaintKit() = 175; // Scorched
									}
									else if (Skin == 14)
									{
										*pWeapon->FallbackPaintKit() = 323; // Rust Coat
									}
									else if (Skin == 15)
									{
										*pWeapon->FallbackPaintKit() = 409; // Tiger Tooth
									}
									else if (Skin == 16)
									{
										*pWeapon->FallbackPaintKit() = 410; // Damascus Steel
									}
									else if (Skin == 17)
									{
										*pWeapon->FallbackPaintKit() = 411; // Damascus Steel
									}
									else if (Skin == 18)
									{
										*pWeapon->FallbackPaintKit() = 413; // Marble Fade
									}
									else if (Skin == 19)
									{
										*pWeapon->FallbackPaintKit() = 414; // Rust Coat
									}
									else if (Skin == 20)
									{
										*pWeapon->FallbackPaintKit() = 415; // Doppler Ruby
									}
									else if (Skin == 21)
									{
										*pWeapon->FallbackPaintKit() = 416; // Doppler Sapphire
									}
									else if (Skin == 22)
									{
										*pWeapon->FallbackPaintKit() = 417; // Doppler Blackpearl
									}
									else if (Skin == 23)
									{
										*pWeapon->FallbackPaintKit() = 418; // Doppler Phase 1
									}
									else if (Skin == 24)
									{
										*pWeapon->FallbackPaintKit() = 419; // Doppler Phase 2
									}
									else if (Skin == 25)
									{
										*pWeapon->FallbackPaintKit() = 420; // Doppler Phase 3
									}
									else if (Skin == 26)
									{
										*pWeapon->FallbackPaintKit() = 421; // Doppler Phase 4
									}
									else if (Skin == 27)
									{
										*pWeapon->FallbackPaintKit() = 569; // Gamma Doppler Phase1
									}
									else if (Skin == 28)
									{
										*pWeapon->FallbackPaintKit() = 570; // Gamma Doppler Phase2
									}
									else if (Skin == 29)
									{
										*pWeapon->FallbackPaintKit() = 571; // Gamma Doppler Phase3
									}
									else if (Skin == 30)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Phase4
									}
									else if (Skin == 31)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Emerald
									}
									else if (Skin == 32)
									{
										*pWeapon->FallbackPaintKit() = 558; // Lore
									}

								}
								else if (Model == 2) // Butterfly Knife
								{
									*pWeapon->ModelIndex() = iButterfly; // m_nModelIndex
									*pWeapon->ViewModelIndex() = iButterfly;
									*pWeapon->WorldModelIndex() = iButterfly + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 515;

									int Skin = Menu::Window.SkinTab.KnifeSkin.GetIndex();

									if (Skin == 0)
									{
										*pWeapon->FallbackPaintKit() = 0; // Forest DDPAT
									}
									else if (Skin == 1)
									{
										*pWeapon->FallbackPaintKit() = 12; // Crimson Web
									}
									else if (Skin == 2)
									{
										*pWeapon->FallbackPaintKit() = 27; // Bone Mask
									}
									else if (Skin == 3)
									{
										*pWeapon->FallbackPaintKit() = 38; // Fade
									}
									else if (Skin == 4)
									{
										*pWeapon->FallbackPaintKit() = 40; // Night
									}
									else if (Skin == 5)
									{
										*pWeapon->FallbackPaintKit() = 42; // Blue Steel
									}
									else if (Skin == 6)
									{
										*pWeapon->FallbackPaintKit() = 43; // Stained
									}
									else if (Skin == 7)
									{
										*pWeapon->FallbackPaintKit() = 44; // Case Hardened
									}
									else if (Skin == 8)
									{
										*pWeapon->FallbackPaintKit() = 59; // Slaughter
									}
									else if (Skin == 9)
									{
										*pWeapon->FallbackPaintKit() = 72; // Safari Mesh
									}
									else if (Skin == 10)
									{
										*pWeapon->FallbackPaintKit() = 77; // Boreal Forest
									}
									else if (Skin == 11)
									{
										*pWeapon->FallbackPaintKit() = 98; // Ultraviolet
									}
									else if (Skin == 12)
									{
										*pWeapon->FallbackPaintKit() = 143; // Urban Masked
									}
									else if (Skin == 13)
									{
										*pWeapon->FallbackPaintKit() = 175; // Scorched
									}
									else if (Skin == 14)
									{
										*pWeapon->FallbackPaintKit() = 323; // Rust Coat
									}
									else if (Skin == 15)
									{
										*pWeapon->FallbackPaintKit() = 409; // Tiger Tooth
									}
									else if (Skin == 16)
									{
										*pWeapon->FallbackPaintKit() = 410; // Damascus Steel
									}
									else if (Skin == 17)
									{
										*pWeapon->FallbackPaintKit() = 411; // Damascus Steel
									}
									else if (Skin == 18)
									{
										*pWeapon->FallbackPaintKit() = 413; // Marble Fade
									}
									else if (Skin == 19)
									{
										*pWeapon->FallbackPaintKit() = 414; // Rust Coat
									}
									else if (Skin == 20)
									{
										*pWeapon->FallbackPaintKit() = 415; // Doppler Ruby
									}
									else if (Skin == 21)
									{
										*pWeapon->FallbackPaintKit() = 416; // Doppler Sapphire
									}
									else if (Skin == 22)
									{
										*pWeapon->FallbackPaintKit() = 417; // Doppler Blackpearl
									}
									else if (Skin == 23)
									{
										*pWeapon->FallbackPaintKit() = 418; // Doppler Phase 1
									}
									else if (Skin == 24)
									{
										*pWeapon->FallbackPaintKit() = 419; // Doppler Phase 2
									}
									else if (Skin == 25)
									{
										*pWeapon->FallbackPaintKit() = 420; // Doppler Phase 3
									}
									else if (Skin == 26)
									{
										*pWeapon->FallbackPaintKit() = 421; // Doppler Phase 4
									}
									else if (Skin == 27)
									{
										*pWeapon->FallbackPaintKit() = 569; // Gamma Doppler Phase1
									}
									else if (Skin == 28)
									{
										*pWeapon->FallbackPaintKit() = 570; // Gamma Doppler Phase2
									}
									else if (Skin == 29)
									{
										*pWeapon->FallbackPaintKit() = 571; // Gamma Doppler Phase3
									}
									else if (Skin == 30)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Phase4
									}
									else if (Skin == 31)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Emerald
									}
									else if (Skin == 32)
									{
										*pWeapon->FallbackPaintKit() = 558; // Lore
									}

								}
								else if (Model == 3) // Falchion Knife
								{
									*pWeapon->ModelIndex() = iFalchion; // m_nModelIndex
									*pWeapon->ViewModelIndex() = iFalchion;
									*pWeapon->WorldModelIndex() = iFalchion + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 512;

									int Skin = Menu::Window.SkinTab.KnifeSkin.GetIndex();

									if (Skin == 0)
									{
										*pWeapon->FallbackPaintKit() = 0; // Forest DDPAT
									}
									else if (Skin == 1)
									{
										*pWeapon->FallbackPaintKit() = 12; // Crimson Web
									}
									else if (Skin == 2)
									{
										*pWeapon->FallbackPaintKit() = 27; // Bone Mask
									}
									else if (Skin == 3)
									{
										*pWeapon->FallbackPaintKit() = 38; // Fade
									}
									else if (Skin == 4)
									{
										*pWeapon->FallbackPaintKit() = 40; // Night
									}
									else if (Skin == 5)
									{
										*pWeapon->FallbackPaintKit() = 42; // Blue Steel
									}
									else if (Skin == 6)
									{
										*pWeapon->FallbackPaintKit() = 43; // Stained
									}
									else if (Skin == 7)
									{
										*pWeapon->FallbackPaintKit() = 44; // Case Hardened
									}
									else if (Skin == 8)
									{
										*pWeapon->FallbackPaintKit() = 59; // Slaughter
									}
									else if (Skin == 9)
									{
										*pWeapon->FallbackPaintKit() = 72; // Safari Mesh
									}
									else if (Skin == 10)
									{
										*pWeapon->FallbackPaintKit() = 77; // Boreal Forest
									}
									else if (Skin == 11)
									{
										*pWeapon->FallbackPaintKit() = 98; // Ultraviolet
									}
									else if (Skin == 12)
									{
										*pWeapon->FallbackPaintKit() = 143; // Urban Masked
									}
									else if (Skin == 13)
									{
										*pWeapon->FallbackPaintKit() = 175; // Scorched
									}
									else if (Skin == 14)
									{
										*pWeapon->FallbackPaintKit() = 323; // Rust Coat
									}
									else if (Skin == 15)
									{
										*pWeapon->FallbackPaintKit() = 409; // Tiger Tooth
									}
									else if (Skin == 16)
									{
										*pWeapon->FallbackPaintKit() = 410; // Damascus Steel
									}
									else if (Skin == 17)
									{
										*pWeapon->FallbackPaintKit() = 411; // Damascus Steel
									}
									else if (Skin == 18)
									{
										*pWeapon->FallbackPaintKit() = 413; // Marble Fade
									}
									else if (Skin == 19)
									{
										*pWeapon->FallbackPaintKit() = 414; // Rust Coat
									}
									else if (Skin == 20)
									{
										*pWeapon->FallbackPaintKit() = 415; // Doppler Ruby
									}
									else if (Skin == 21)
									{
										*pWeapon->FallbackPaintKit() = 416; // Doppler Sapphire
									}
									else if (Skin == 22)
									{
										*pWeapon->FallbackPaintKit() = 417; // Doppler Blackpearl
									}
									else if (Skin == 23)
									{
										*pWeapon->FallbackPaintKit() = 418; // Doppler Phase 1
									}
									else if (Skin == 24)
									{
										*pWeapon->FallbackPaintKit() = 419; // Doppler Phase 2
									}
									else if (Skin == 25)
									{
										*pWeapon->FallbackPaintKit() = 420; // Doppler Phase 3
									}
									else if (Skin == 26)
									{
										*pWeapon->FallbackPaintKit() = 421; // Doppler Phase 4
									}
									else if (Skin == 27)
									{
										*pWeapon->FallbackPaintKit() = 569; // Gamma Doppler Phase1
									}
									else if (Skin == 28)
									{
										*pWeapon->FallbackPaintKit() = 570; // Gamma Doppler Phase2
									}
									else if (Skin == 29)
									{
										*pWeapon->FallbackPaintKit() = 571; // Gamma Doppler Phase3
									}
									else if (Skin == 30)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Phase4
									}
									else if (Skin == 31)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Emerald
									}
									else if (Skin == 32)
									{
										*pWeapon->FallbackPaintKit() = 558; // Lore
									}
								}
								else if (Model == 4) // Flip Knife
								{
									*pWeapon->ModelIndex() = iFlip; // m_nModelIndex
									*pWeapon->ViewModelIndex() = iFlip;
									*pWeapon->WorldModelIndex() = iFlip + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 505;

									int Skin = Menu::Window.SkinTab.KnifeSkin.GetIndex();

									if (Skin == 0)
									{
										*pWeapon->FallbackPaintKit() = 0; // Forest DDPAT
									}
									else if (Skin == 1)
									{
										*pWeapon->FallbackPaintKit() = 12; // Crimson Web
									}
									else if (Skin == 2)
									{
										*pWeapon->FallbackPaintKit() = 27; // Bone Mask
									}
									else if (Skin == 3)
									{
										*pWeapon->FallbackPaintKit() = 38; // Fade
									}
									else if (Skin == 4)
									{
										*pWeapon->FallbackPaintKit() = 40; // Night
									}
									else if (Skin == 5)
									{
										*pWeapon->FallbackPaintKit() = 42; // Blue Steel
									}
									else if (Skin == 6)
									{
										*pWeapon->FallbackPaintKit() = 43; // Stained
									}
									else if (Skin == 7)
									{
										*pWeapon->FallbackPaintKit() = 44; // Case Hardened
									}
									else if (Skin == 8)
									{
										*pWeapon->FallbackPaintKit() = 59; // Slaughter
									}
									else if (Skin == 9)
									{
										*pWeapon->FallbackPaintKit() = 72; // Safari Mesh
									}
									else if (Skin == 10)
									{
										*pWeapon->FallbackPaintKit() = 77; // Boreal Forest
									}
									else if (Skin == 11)
									{
										*pWeapon->FallbackPaintKit() = 98; // Ultraviolet
									}
									else if (Skin == 12)
									{
										*pWeapon->FallbackPaintKit() = 143; // Urban Masked
									}
									else if (Skin == 13)
									{
										*pWeapon->FallbackPaintKit() = 175; // Scorched
									}
									else if (Skin == 14)
									{
										*pWeapon->FallbackPaintKit() = 323; // Rust Coat
									}
									else if (Skin == 15)
									{
										*pWeapon->FallbackPaintKit() = 409; // Tiger Tooth
									}
									else if (Skin == 16)
									{
										*pWeapon->FallbackPaintKit() = 410; // Damascus Steel
									}
									else if (Skin == 17)
									{
										*pWeapon->FallbackPaintKit() = 411; // Damascus Steel
									}
									else if (Skin == 18)
									{
										*pWeapon->FallbackPaintKit() = 413; // Marble Fade
									}
									else if (Skin == 19)
									{
										*pWeapon->FallbackPaintKit() = 414; // Rust Coat
									}
									else if (Skin == 20)
									{
										*pWeapon->FallbackPaintKit() = 415; // Doppler Ruby
									}
									else if (Skin == 21)
									{
										*pWeapon->FallbackPaintKit() = 416; // Doppler Sapphire
									}
									else if (Skin == 22)
									{
										*pWeapon->FallbackPaintKit() = 417; // Doppler Blackpearl
									}
									else if (Skin == 23)
									{
										*pWeapon->FallbackPaintKit() = 418; // Doppler Phase 1
									}
									else if (Skin == 24)
									{
										*pWeapon->FallbackPaintKit() = 419; // Doppler Phase 2
									}
									else if (Skin == 25)
									{
										*pWeapon->FallbackPaintKit() = 420; // Doppler Phase 3
									}
									else if (Skin == 26)
									{
										*pWeapon->FallbackPaintKit() = 421; // Doppler Phase 4
									}
									else if (Skin == 27)
									{
										*pWeapon->FallbackPaintKit() = 569; // Gamma Doppler Phase1
									}
									else if (Skin == 28)
									{
										*pWeapon->FallbackPaintKit() = 570; // Gamma Doppler Phase2
									}
									else if (Skin == 29)
									{
										*pWeapon->FallbackPaintKit() = 571; // Gamma Doppler Phase3
									}
									else if (Skin == 30)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Phase4
									}
									else if (Skin == 31)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Emerald
									}
									else if (Skin == 32)
									{
										*pWeapon->FallbackPaintKit() = 559; // Lore
									}

								}
								else if (Model == 5) // Gut Knife
								{
									*pWeapon->ModelIndex() = iGut; // m_nModelIndex
									*pWeapon->ViewModelIndex() = iGut;
									*pWeapon->WorldModelIndex() = iGut + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 506;

									int Skin = Menu::Window.SkinTab.KnifeSkin.GetIndex();

									if (Skin == 0)
									{
										*pWeapon->FallbackPaintKit() = 0; // Forest DDPAT
									}
									else if (Skin == 1)
									{
										*pWeapon->FallbackPaintKit() = 12; // Crimson Web
									}
									else if (Skin == 2)
									{
										*pWeapon->FallbackPaintKit() = 27; // Bone Mask
									}
									else if (Skin == 3)
									{
										*pWeapon->FallbackPaintKit() = 38; // Fade
									}
									else if (Skin == 4)
									{
										*pWeapon->FallbackPaintKit() = 40; // Night
									}
									else if (Skin == 5)
									{
										*pWeapon->FallbackPaintKit() = 42; // Blue Steel
									}
									else if (Skin == 6)
									{
										*pWeapon->FallbackPaintKit() = 43; // Stained
									}
									else if (Skin == 7)
									{
										*pWeapon->FallbackPaintKit() = 44; // Case Hardened
									}
									else if (Skin == 8)
									{
										*pWeapon->FallbackPaintKit() = 59; // Slaughter
									}
									else if (Skin == 9)
									{
										*pWeapon->FallbackPaintKit() = 72; // Safari Mesh
									}
									else if (Skin == 10)
									{
										*pWeapon->FallbackPaintKit() = 77; // Boreal Forest
									}
									else if (Skin == 11)
									{
										*pWeapon->FallbackPaintKit() = 98; // Ultraviolet
									}
									else if (Skin == 12)
									{
										*pWeapon->FallbackPaintKit() = 143; // Urban Masked
									}
									else if (Skin == 13)
									{
										*pWeapon->FallbackPaintKit() = 175; // Scorched
									}
									else if (Skin == 14)
									{
										*pWeapon->FallbackPaintKit() = 323; // Rust Coat
									}
									else if (Skin == 15)
									{
										*pWeapon->FallbackPaintKit() = 409; // Tiger Tooth
									}
									else if (Skin == 16)
									{
										*pWeapon->FallbackPaintKit() = 410; // Damascus Steel
									}
									else if (Skin == 17)
									{
										*pWeapon->FallbackPaintKit() = 411; // Damascus Steel
									}
									else if (Skin == 18)
									{
										*pWeapon->FallbackPaintKit() = 413; // Marble Fade
									}
									else if (Skin == 19)
									{
										*pWeapon->FallbackPaintKit() = 414; // Rust Coat
									}
									else if (Skin == 20)
									{
										*pWeapon->FallbackPaintKit() = 415; // Doppler Ruby
									}
									else if (Skin == 21)
									{
										*pWeapon->FallbackPaintKit() = 416; // Doppler Sapphire
									}
									else if (Skin == 22)
									{
										*pWeapon->FallbackPaintKit() = 417; // Doppler Blackpearl
									}
									else if (Skin == 23)
									{
										*pWeapon->FallbackPaintKit() = 418; // Doppler Phase 1
									}
									else if (Skin == 24)
									{
										*pWeapon->FallbackPaintKit() = 419; // Doppler Phase 2
									}
									else if (Skin == 25)
									{
										*pWeapon->FallbackPaintKit() = 420; // Doppler Phase 3
									}
									else if (Skin == 26)
									{
										*pWeapon->FallbackPaintKit() = 421; // Doppler Phase 4
									}
									else if (Skin == 27)
									{
										*pWeapon->FallbackPaintKit() = 569; // Gamma Doppler Phase1
									}
									else if (Skin == 28)
									{
										*pWeapon->FallbackPaintKit() = 570; // Gamma Doppler Phase2
									}
									else if (Skin == 29)
									{
										*pWeapon->FallbackPaintKit() = 571; // Gamma Doppler Phase3
									}
									else if (Skin == 30)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Phase4
									}
									else if (Skin == 31)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Emerald
									}
									else if (Skin == 32)
									{
										*pWeapon->FallbackPaintKit() = 560; // Lore
									}


								}
								else if (Model == 6) // Huntsman Knife
								{
									*pWeapon->ModelIndex() = iHuntsman; // m_nModelIndex
									*pWeapon->ViewModelIndex() = iHuntsman;
									*pWeapon->WorldModelIndex() = iHuntsman + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 509;

									int Skin = Menu::Window.SkinTab.KnifeSkin.GetIndex();

									if (Skin == 0)
									{
										*pWeapon->FallbackPaintKit() = 0; // Forest DDPAT
									}
									else if (Skin == 1)
									{
										*pWeapon->FallbackPaintKit() = 12; // Crimson Web
									}
									else if (Skin == 2)
									{
										*pWeapon->FallbackPaintKit() = 27; // Bone Mask
									}
									else if (Skin == 3)
									{
										*pWeapon->FallbackPaintKit() = 38; // Fade
									}
									else if (Skin == 4)
									{
										*pWeapon->FallbackPaintKit() = 40; // Night
									}
									else if (Skin == 5)
									{
										*pWeapon->FallbackPaintKit() = 42; // Blue Steel
									}
									else if (Skin == 6)
									{
										*pWeapon->FallbackPaintKit() = 43; // Stained
									}
									else if (Skin == 7)
									{
										*pWeapon->FallbackPaintKit() = 44; // Case Hardened
									}
									else if (Skin == 8)
									{
										*pWeapon->FallbackPaintKit() = 59; // Slaughter
									}
									else if (Skin == 9)
									{
										*pWeapon->FallbackPaintKit() = 72; // Safari Mesh
									}
									else if (Skin == 10)
									{
										*pWeapon->FallbackPaintKit() = 77; // Boreal Forest
									}
									else if (Skin == 11)
									{
										*pWeapon->FallbackPaintKit() = 98; // Ultraviolet
									}
									else if (Skin == 12)
									{
										*pWeapon->FallbackPaintKit() = 143; // Urban Masked
									}
									else if (Skin == 13)
									{
										*pWeapon->FallbackPaintKit() = 175; // Scorched
									}
									else if (Skin == 14)
									{
										*pWeapon->FallbackPaintKit() = 323; // Rust Coat
									}
									else if (Skin == 15)
									{
										*pWeapon->FallbackPaintKit() = 409; // Tiger Tooth
									}
									else if (Skin == 16)
									{
										*pWeapon->FallbackPaintKit() = 410; // Damascus Steel
									}
									else if (Skin == 17)
									{
										*pWeapon->FallbackPaintKit() = 411; // Damascus Steel
									}
									else if (Skin == 18)
									{
										*pWeapon->FallbackPaintKit() = 413; // Marble Fade
									}
									else if (Skin == 19)
									{
										*pWeapon->FallbackPaintKit() = 414; // Rust Coat
									}
									else if (Skin == 20)
									{
										*pWeapon->FallbackPaintKit() = 415; // Doppler Ruby
									}
									else if (Skin == 21)
									{
										*pWeapon->FallbackPaintKit() = 416; // Doppler Sapphire
									}
									else if (Skin == 22)
									{
										*pWeapon->FallbackPaintKit() = 417; // Doppler Blackpearl
									}
									else if (Skin == 23)
									{
										*pWeapon->FallbackPaintKit() = 418; // Doppler Phase 1
									}
									else if (Skin == 24)
									{
										*pWeapon->FallbackPaintKit() = 419; // Doppler Phase 2
									}
									else if (Skin == 25)
									{
										*pWeapon->FallbackPaintKit() = 420; // Doppler Phase 3
									}
									else if (Skin == 26)
									{
										*pWeapon->FallbackPaintKit() = 421; // Doppler Phase 4
									}
									else if (Skin == 27)
									{
										*pWeapon->FallbackPaintKit() = 569; // Gamma Doppler Phase1
									}
									else if (Skin == 28)
									{
										*pWeapon->FallbackPaintKit() = 570; // Gamma Doppler Phase2
									}
									else if (Skin == 29)
									{
										*pWeapon->FallbackPaintKit() = 571; // Gamma Doppler Phase3
									}
									else if (Skin == 30)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Phase4
									}
									else if (Skin == 31)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Emerald
									}
									else if (Skin == 32)
									{
										*pWeapon->FallbackPaintKit() = 559; // Lore
									}


								}
								else if (Model == 7) // Karambit
								{
									*pWeapon->ModelIndex() = iKarambit; // m_nModelIndex
									*pWeapon->ViewModelIndex() = iKarambit;
									*pWeapon->WorldModelIndex() = iKarambit + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 507;

									int Skin = Menu::Window.SkinTab.KnifeSkin.GetIndex();

									if (Skin == 0)
									{
										*pWeapon->FallbackPaintKit() = 0; // Forest DDPAT
									}
									else if (Skin == 1)
									{
										*pWeapon->FallbackPaintKit() = 12; // Crimson Web
									}
									else if (Skin == 2)
									{
										*pWeapon->FallbackPaintKit() = 27; // Bone Mask
									}
									else if (Skin == 3)
									{
										*pWeapon->FallbackPaintKit() = 38; // Fade
									}
									else if (Skin == 4)
									{
										*pWeapon->FallbackPaintKit() = 40; // Night
									}
									else if (Skin == 5)
									{
										*pWeapon->FallbackPaintKit() = 42; // Blue Steel
									}
									else if (Skin == 6)
									{
										*pWeapon->FallbackPaintKit() = 43; // Stained
									}
									else if (Skin == 7)
									{
										*pWeapon->FallbackPaintKit() = 44; // Case Hardened
									}
									else if (Skin == 8)
									{
										*pWeapon->FallbackPaintKit() = 59; // Slaughter
									}
									else if (Skin == 9)
									{
										*pWeapon->FallbackPaintKit() = 72; // Safari Mesh
									}
									else if (Skin == 10)
									{
										*pWeapon->FallbackPaintKit() = 77; // Boreal Forest
									}
									else if (Skin == 11)
									{
										*pWeapon->FallbackPaintKit() = 98; // Ultraviolet
									}
									else if (Skin == 12)
									{
										*pWeapon->FallbackPaintKit() = 143; // Urban Masked
									}
									else if (Skin == 13)
									{
										*pWeapon->FallbackPaintKit() = 175; // Scorched
									}
									else if (Skin == 14)
									{
										*pWeapon->FallbackPaintKit() = 323; // Rust Coat
									}
									else if (Skin == 15)
									{
										*pWeapon->FallbackPaintKit() = 409; // Tiger Tooth
									}
									else if (Skin == 16)
									{
										*pWeapon->FallbackPaintKit() = 410; // Damascus Steel
									}
									else if (Skin == 17)
									{
										*pWeapon->FallbackPaintKit() = 411; // Damascus Steel
									}
									else if (Skin == 18)
									{
										*pWeapon->FallbackPaintKit() = 413; // Marble Fade
									}
									else if (Skin == 19)
									{
										*pWeapon->FallbackPaintKit() = 414; // Rust Coat
									}
									else if (Skin == 20)
									{
										*pWeapon->FallbackPaintKit() = 415; // Doppler Ruby
									}
									else if (Skin == 21)
									{
										*pWeapon->FallbackPaintKit() = 416; // Doppler Sapphire
									}
									else if (Skin == 22)
									{
										*pWeapon->FallbackPaintKit() = 417; // Doppler Blackpearl
									}
									else if (Skin == 23)
									{
										*pWeapon->FallbackPaintKit() = 418; // Doppler Phase 1
									}
									else if (Skin == 24)
									{
										*pWeapon->FallbackPaintKit() = 419; // Doppler Phase 2
									}
									else if (Skin == 25)
									{
										*pWeapon->FallbackPaintKit() = 420; // Doppler Phase 3
									}
									else if (Skin == 26)
									{
										*pWeapon->FallbackPaintKit() = 421; // Doppler Phase 4
									}
									else if (Skin == 27)
									{
										*pWeapon->FallbackPaintKit() = 570; // Doppler Phase 4
									}
									else if (Skin == 28)
									{
										*pWeapon->FallbackPaintKit() = 568; // Doppler Phase 4
									}
									else if (Skin == 27)
									{
										*pWeapon->FallbackPaintKit() = 569; // Gamma Doppler Phase1
									}
									else if (Skin == 28)
									{
										*pWeapon->FallbackPaintKit() = 570; // Gamma Doppler Phase2
									}
									else if (Skin == 29)
									{
										*pWeapon->FallbackPaintKit() = 571; // Gamma Doppler Phase3
									}
									else if (Skin == 30)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Phase4
									}
									else if (Skin == 31)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Emerald
									}
									else if (Skin == 32)
									{
										*pWeapon->FallbackPaintKit() = 561; // Lore
									}

								}
								else if (Model == 8) // M9 Bayonet
								{
									*pWeapon->ModelIndex() = iM9Bayonet; // m_nModelIndex
									*pWeapon->ViewModelIndex() = iM9Bayonet;
									*pWeapon->WorldModelIndex() = iM9Bayonet + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 508;

									int Skin = Menu::Window.SkinTab.KnifeSkin.GetIndex();

									if (Skin == 0)
									{
										*pWeapon->FallbackPaintKit() = 0; // Forest DDPAT
									}
									else if (Skin == 1)
									{
										*pWeapon->FallbackPaintKit() = 12; // Crimson Web
									}
									else if (Skin == 2)
									{
										*pWeapon->FallbackPaintKit() = 27; // Bone Mask
									}
									else if (Skin == 3)
									{
										*pWeapon->FallbackPaintKit() = 38; // Fade
									}
									else if (Skin == 4)
									{
										*pWeapon->FallbackPaintKit() = 40; // Night
									}
									else if (Skin == 5)
									{
										*pWeapon->FallbackPaintKit() = 42; // Blue Steel
									}
									else if (Skin == 6)
									{
										*pWeapon->FallbackPaintKit() = 43; // Stained
									}
									else if (Skin == 7)
									{
										*pWeapon->FallbackPaintKit() = 44; // Case Hardened
									}
									else if (Skin == 8)
									{
										*pWeapon->FallbackPaintKit() = 59; // Slaughter
									}
									else if (Skin == 9)
									{
										*pWeapon->FallbackPaintKit() = 72; // Safari Mesh
									}
									else if (Skin == 10)
									{
										*pWeapon->FallbackPaintKit() = 77; // Boreal Forest
									}
									else if (Skin == 11)
									{
										*pWeapon->FallbackPaintKit() = 98; // Ultraviolet
									}
									else if (Skin == 12)
									{
										*pWeapon->FallbackPaintKit() = 143; // Urban Masked
									}
									else if (Skin == 13)
									{
										*pWeapon->FallbackPaintKit() = 175; // Scorched
									}
									else if (Skin == 14)
									{
										*pWeapon->FallbackPaintKit() = 323; // Rust Coat
									}
									else if (Skin == 15)
									{
										*pWeapon->FallbackPaintKit() = 409; // Tiger Tooth
									}
									else if (Skin == 16)
									{
										*pWeapon->FallbackPaintKit() = 410; // Damascus Steel
									}
									else if (Skin == 17)
									{
										*pWeapon->FallbackPaintKit() = 411; // Damascus Steel
									}
									else if (Skin == 18)
									{
										*pWeapon->FallbackPaintKit() = 413; // Marble Fade
									}
									else if (Skin == 19)
									{
										*pWeapon->FallbackPaintKit() = 414; // Rust Coat
									}
									else if (Skin == 20)
									{
										*pWeapon->FallbackPaintKit() = 415; // Doppler Ruby
									}
									else if (Skin == 21)
									{
										*pWeapon->FallbackPaintKit() = 416; // Doppler Sapphire
									}
									else if (Skin == 22)
									{
										*pWeapon->FallbackPaintKit() = 417; // Doppler Blackpearl
									}
									else if (Skin == 23)
									{
										*pWeapon->FallbackPaintKit() = 418; // Doppler Phase 1
									}
									else if (Skin == 24)
									{
										*pWeapon->FallbackPaintKit() = 419; // Doppler Phase 2
									}
									else if (Skin == 25)
									{
										*pWeapon->FallbackPaintKit() = 420; // Doppler Phase 3
									}
									else if (Skin == 26)
									{
										*pWeapon->FallbackPaintKit() = 421; // Doppler Phase 4
									}
									else if (Skin == 27)
									{
										*pWeapon->FallbackPaintKit() = 570; // Doppler Phase 4
									}
									else if (Skin == 28)
									{
										*pWeapon->FallbackPaintKit() = 568; // Doppler Phase 4
									}
									else if (Skin == 27)
									{
										*pWeapon->FallbackPaintKit() = 569; // Gamma Doppler Phase1
									}
									else if (Skin == 28)
									{
										*pWeapon->FallbackPaintKit() = 570; // Gamma Doppler Phase2
									}
									else if (Skin == 29)
									{
										*pWeapon->FallbackPaintKit() = 571; // Gamma Doppler Phase3
									}
									else if (Skin == 30)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Phase4
									}
									else if (Skin == 31)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Emerald
									}
									else if (Skin == 32)
									{
										*pWeapon->FallbackPaintKit() = 562; // Lore
									}

								}


								else if (Model == 10) // Shadow Daggers
								{
									*pWeapon->ModelIndex() = iDagger; // m_nModelIndex
									*pWeapon->ViewModelIndex() = iDagger;
									*pWeapon->WorldModelIndex() = iDagger + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 516;

									int Skin = Menu::Window.SkinTab.KnifeSkin.GetIndex();

									if (Skin == 0)
									{
										*pWeapon->FallbackPaintKit() = 5; // Forest DDPAT
									}
									else if (Skin == 1)
									{
										*pWeapon->FallbackPaintKit() = 12; // Crimson Web
									}
									else if (Skin == 2)
									{
										*pWeapon->FallbackPaintKit() = 27; // Bone Mask
									}
									else if (Skin == 3)
									{
										*pWeapon->FallbackPaintKit() = 38; // Fade
									}
									else if (Skin == 4)
									{
										*pWeapon->FallbackPaintKit() = 40; // Night
									}
									else if (Skin == 5)
									{
										*pWeapon->FallbackPaintKit() = 42; // Blue Steel
									}
									else if (Skin == 6)
									{
										*pWeapon->FallbackPaintKit() = 43; // Stained
									}
									else if (Skin == 7)
									{
										*pWeapon->FallbackPaintKit() = 44; // Case Hardened
									}
									else if (Skin == 8)
									{
										*pWeapon->FallbackPaintKit() = 59; // Slaughter
									}
									else if (Skin == 9)
									{
										*pWeapon->FallbackPaintKit() = 72; // Safari Mesh
									}
									else if (Skin == 10)
									{
										*pWeapon->FallbackPaintKit() = 77; // Boreal Forest
									}
									else if (Skin == 11)
									{
										*pWeapon->FallbackPaintKit() = 98; // Ultraviolet
									}
									else if (Skin == 12)
									{
										*pWeapon->FallbackPaintKit() = 143; // Urban Masked
									}
									else if (Skin == 13)
									{
										*pWeapon->FallbackPaintKit() = 175; // Scorched
									}
									else if (Skin == 14)
									{
										*pWeapon->FallbackPaintKit() = 323; // Rust Coat
									}
									else if (Skin == 15)
									{
										*pWeapon->FallbackPaintKit() = 409; // Tiger Tooth
									}
									else if (Skin == 16)
									{
										*pWeapon->FallbackPaintKit() = 410; // Damascus Steel
									}
									else if (Skin == 17)
									{
										*pWeapon->FallbackPaintKit() = 411; // Damascus Steel
									}
									else if (Skin == 18)
									{
										*pWeapon->FallbackPaintKit() = 413; // Marble Fade
									}
									else if (Skin == 19)
									{
										*pWeapon->FallbackPaintKit() = 414; // Rust Coat
									}
									else if (Skin == 20)
									{
										*pWeapon->FallbackPaintKit() = 415; // Doppler Ruby
									}
									else if (Skin == 21)
									{
										*pWeapon->FallbackPaintKit() = 416; // Doppler Sapphire
									}
									else if (Skin == 22)
									{
										*pWeapon->FallbackPaintKit() = 417; // Doppler Blackpearl
									}
									else if (Skin == 23)
									{
										*pWeapon->FallbackPaintKit() = 418; // Doppler Phase 1
									}
									else if (Skin == 24)
									{
										*pWeapon->FallbackPaintKit() = 419; // Doppler Phase 2
									}
									else if (Skin == 25)
									{
										*pWeapon->FallbackPaintKit() = 420; // Doppler Phase 3
									}
									else if (Skin == 26)
									{
										*pWeapon->FallbackPaintKit() = 421; // Doppler Phase 4
									}
									else if (Skin == 27)
									{
										*pWeapon->FallbackPaintKit() = 569; // Gamma Doppler Phase1
									}
									else if (Skin == 28)
									{
										*pWeapon->FallbackPaintKit() = 570; // Gamma Doppler Phase2
									}
									else if (Skin == 29)
									{
										*pWeapon->FallbackPaintKit() = 571; // Gamma Doppler Phase3
									}
									else if (Skin == 30)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Phase4
									}
									else if (Skin == 31)
									{
										*pWeapon->FallbackPaintKit() = 568; // Gamma Doppler Emerald
									}
									else if (Skin == 32)
									{
										*pWeapon->FallbackPaintKit() = 561; // Lore
									}
								}
							}

							*pWeapon->OwnerXuidLow() = 0;
							*pWeapon->OwnerXuidHigh() = 0;
							*pWeapon->FallbackWear() = 0.001f;
							*pWeapon->m_AttributeManager()->m_Item()->ItemIDHigh() = 1;

							}
						}
					}
				}
			}
		}
	}
	oFrameStageNotify(curStage);

	/* if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame() && curStage == FRAME_NET_UPDATE_POSTDATAUPDATE_START)
	{
		IClientEntity *pLocal = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());

		if (Menu::Window.
		.KnifeEnable.GetState() && pLocal)
		{
			IClientEntity* WeaponEnt = Interfaces::EntList->GetClientEntityFromHandle(pLocal->GetActiveWeaponHandle());
			CBaseCombatWeapon* Weapon = (CBaseCombatWeapon*)WeaponEnt;

			int iBayonet = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_bayonet.mdl");
			int iButterfly = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_butterfly.mdl");
			int iFlip = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_flip.mdl");
			int iGut = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_gut.mdl");
			int iKarambit = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_karam.mdl");
			int iM9Bayonet = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_m9_bay.mdl");
			int iHuntsman = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_tactical.mdl");
			int iFalchion = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_falchion_advanced.mdl");
			int iDagger = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_push.mdl");
			int iBowie = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_survival_bowie.mdl");

			int Model = Menu::Window.MiscTab.KnifeModel.GetIndex();
			int Skin = Menu::Window.MiscTab.KnifeSkin.GetIndex();

			if (Weapon)
			{
				if (WeaponEnt->GetClientClass()->m_ClassID == (int)CSGOClassID::CKnife)
				{
					if (Model == 0) // Karambit
					{
						*Weapon->ModelIndex() = iKarambit; // m_nModelIndex
						*Weapon->ViewModelIndex() = iKarambit;
						*Weapon->WorldModelIndex() = iKarambit + 1;
						*Weapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 507;

						if (Skin == 0)
							*Weapon->FallbackPaintKit() = 416; // Doppler Sapphire
						else if (Skin == 1)
							*Weapon->FallbackPaintKit() = 415; // Doppler Ruby
						else if (Skin == 2)
							*Weapon->FallbackPaintKit() = 409; // Tiger Tooth
						else if (Skin == 3)
							*Weapon->FallbackPaintKit() = 558; // Lore
					}
					else if (Model == 1) // Karambit
					{
						*Weapon->ModelIndex() = iBayonet; // m_nModelIndex
						*Weapon->ViewModelIndex() = iBayonet;
						*Weapon->WorldModelIndex() = iBayonet + 1;
						*Weapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 500;

						if (Skin == 0)
							*Weapon->FallbackPaintKit() = 416; // Doppler Sapphire
						else if (Skin == 1)
							*Weapon->FallbackPaintKit() = 415; // Doppler Ruby
						else if (Skin == 2)
							*Weapon->FallbackPaintKit() = 409; // Tiger Tooth
						else if (Skin == 3)
							*Weapon->FallbackPaintKit() = 558; // Lore
					}

					*Weapon->OwnerXuidLow() = 0;
					*Weapon->OwnerXuidHigh() = 0;
					*Weapon->FallbackWear() = 0.001f;
					*Weapon->m_AttributeManager()->m_Item()->ItemIDHigh() = 1;
				}
			}
		}
		if (pLocal->IsAlive())
			R::Resolve();
	}

	oFrameStageNotify(curStage); */
}

void __fastcall Hooked_OverrideView(void* ecx, void* edx, CViewSetup* pSetup)
{
	IClientEntity* pLocal = (IClientEntity*)Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());

	if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame())
	{
		if (Menu::Window.VisualsTab.Active.GetState() && pLocal->IsAlive() && !pLocal->IsScoped())
		{
			if (pSetup->fov = 90)
				pSetup->fov = Menu::Window.VisualsTab.OtherFOV.GetValue();
		}

		oOverrideView(ecx, edx, pSetup);
	}

}

void GetViewModelFOV(float& fov)
{
	IClientEntity* localplayer = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());

	if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame())
	{

		if (!localplayer)
			return;


		if (Menu::Window.VisualsTab.Active.GetState())
		fov += Menu::Window.VisualsTab.OtherViewmodelFOV.GetValue();
	}
}

float __stdcall GGetViewModelFOV()
{
	float fov = Hooks::VMTClientMode.GetMethod<oGetViewModelFOV>(35)();

	GetViewModelFOV(fov);

	return fov;
}

void __fastcall Hooked_RenderView(void* ecx, void* edx, CViewSetup &setup, CViewSetup &hudViewSetup, int nClearFlags, int whatToDraw)
{
	static DWORD oRenderView = Hooks::VMTRenderView.GetOriginalFunction(6);

	IClientEntity* pLocal = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());

	__asm
	{
		PUSH whatToDraw
		PUSH nClearFlags
		PUSH hudViewSetup
		PUSH setup
		MOV ECX, ecx
		CALL oRenderView
	}
}