#include "Visuals.h"
#include "Interfaces.h"
#include "RenderManager.h"

// Any init here
void CVisuals::Init()
{
	// Idk
}

// Don't really need to do anything in here
void CVisuals::Move(CUserCmd *pCmd, bool &bSendPacket){}

// Main ESP Drawing loop
void CVisuals::Draw()
{
	NoScopeBorder();

	// Crosshair
	DrawCrosshair();

	// Recoil Crosshair
	DrawRecoilCrosshair();

}



void CVisuals::NoScopeBorder()
{
	ConVar* matpostprocessing = Interfaces::CVar->FindVar("mat_postprocess_enable");

	if (Menu::Window.VisualsTab.OtherNoScope.GetState())
	{
		matpostprocessing->SetValue("0");
		if (hackManager.pLocal()->IsScoped() && hackManager.pLocal()->IsAlive())
		{
			int Width;
			int Height;
			Interfaces::Engine->GetScreenSize(Width, Height);

			Color cColor = Color(0, 0, 0, 255);
			Render::Line(Width / 2, 0, Width / 2, Height, cColor);
			Render::Line(0, Height / 2, Width, Height / 2, cColor);

		}
		else
		{

		}
	}
}

// Draw a basic crosshair
void CVisuals::DrawCrosshair()
{
	if (Menu::Window.VisualsTab.OtherCrosshair.GetIndex() == 1)
	{
		static float rainbow;
		rainbow += 0.005f;
		if (rainbow > 1.f) rainbow = 0.f;

		RECT View = Render::GetViewport();
		int MidX = View.right / 2;
		int MidY = View.bottom / 2;
		Render::Line(MidX - 4, MidY - 4, MidX + 4, MidY + 4, Color::FromHSB(rainbow, 1.f, 1.f));
		Render::Line(MidX + 4, MidY - 4, MidX - 4, MidY + 4, Color::FromHSB(rainbow, 1.f, 1.f));
	}
	else if (Menu::Window.VisualsTab.OtherCrosshair.GetIndex() == 2)
	{
		static float rainbow;
		rainbow += 0.005f;
		if (rainbow > 1.f) rainbow = 0.f;

		RECT View = Render::GetViewport();
		int drX = View.right / 2;
		int drY = View.bottom / 2;
		// Color color(255, 0, 0, 255);

		Render::Line(drX, drY, drX, drY - 30, Color::FromHSB(rainbow, 1.f, 1.f));
		Render::Line(drX, drY - 30, drX + 30, drY - 30, Color::FromHSB(rainbow, 1.f, 1.f));

		Render::Line(drX, drY, drX + 30, drY, Color::FromHSB(rainbow, 1.f, 1.f));
		Render::Line(drX + 30, drY, drX + 30, drY + 30, Color::FromHSB(rainbow, 1.f, 1.f));

		Render::Line(drX, drY, drX, drY + 30, Color::FromHSB(rainbow, 1.f, 1.f));
		Render::Line(drX, drY + 30, drX - 30, drY + 30, Color::FromHSB(rainbow, 1.f, 1.f));

		Render::Line(drX, drY, drX - 30, drY, Color::FromHSB(rainbow, 1.f, 1.f));
		Render::Line(drX - 30, drY, drX - 30, drY - 30, Color::FromHSB(rainbow, 1.f, 1.f));
	}
	else if (Menu::Window.VisualsTab.OtherCrosshair.GetIndex() == 3)
	{
			RECT View = Render::GetViewport();
			int MidX = View.right / 2;
			int MidY = View.bottom / 2;

			IClientEntity* pLocal = hackManager.pLocal();
			CBaseCombatWeapon* pWeapon = (CBaseCombatWeapon*)Interfaces::EntList->GetClientEntityFromHandle(pLocal->GetActiveWeaponHandle());

			if (GameUtils::IsSniper(pWeapon))
			{
				Render::Line(MidX - 1000, MidY, MidX + 1000, MidY, Color(0, 0, 0, 255));
				Render::Line(MidX, MidY - 1000, MidX, MidY + 1000, Color(0, 0, 0, 255));
			}
	}
	else if (Menu::Window.VisualsTab.OtherCrosshair.GetIndex() == 4)
	{
		// For future codes
	}
	else if (Menu::Window.VisualsTab.OtherCrosshair.GetIndex() == 5)
	{
		// For future codes
	}
}


// Recoil crosshair
void CVisuals::DrawRecoilCrosshair()
{
	if (Menu::Window.VisualsTab.OtherRecoilCrosshair.GetIndex() == 0)
	{
		// No recoil crosshair selected in menu.
	}

	else if (Menu::Window.VisualsTab.OtherRecoilCrosshair.GetIndex() == 1)
	{
		// Recoil Crosshair 1

		IClientEntity *pLocal = hackManager.pLocal();

		// Get the view with the recoil
		Vector ViewAngles;
		Interfaces::Engine->GetViewAngles(ViewAngles);
		ViewAngles += pLocal->localPlayerExclusive()->GetAimPunchAngle() * 2.f;

		// Build a ray going fowards at that angle
		Vector fowardVec;
		AngleVectors(ViewAngles, &fowardVec);
		fowardVec *= 10000;

		// Get ray start / end
		Vector start = pLocal->GetOrigin() + pLocal->GetViewOffset();
		Vector end = start + fowardVec, endScreen;

		if (Render::WorldToScreen(end, endScreen) && pLocal->IsAlive())
		{
			Render::Line(endScreen.x - 4, endScreen.y - 4, endScreen.x + 4, endScreen.y + 4, Color(0, 255, 0, 255));
			Render::Line(endScreen.x + 4, endScreen.y - 4, endScreen.x - 4, endScreen.y + 4, Color(0, 255, 0, 255));
			Render::Line(endScreen.x - 2, endScreen.y - 2, endScreen.x + 2, endScreen.y + 2, Color(0, 0, 0, 200));
			Render::Line(endScreen.x + 2, endScreen.y - 2, endScreen.x - 2, endScreen.y + 2, Color(0, 0, 0, 200));
		}
	}
}

void CVisuals::DrawCenterRadar()
{
}
