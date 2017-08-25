#pragma once

#include "GUI.h"
#include "Controls.h"

class CRageBotTab : public CTab
{
public:
	void Setup();

	// Master Switch
	CLabel ActiveLabel;
	CCheckBox Active;
	CComboBox AccuracyResolver;
	// Aimbot Settings
	CGroupBox AimbotGroup;
	CCheckBox AimbotEnable;
	CCheckBox AimbotAutoFire;
	CSlider	  AimbotFov;
	CCheckBox AimbotSilentAim;
	CCheckBox AimbotPerfectSilentAim;
	CCheckBox AimbotAutoPistol;
	CCheckBox AimbotAutoRevolver;
	CCheckBox AimbotAimStep;
	CCheckBox AimbotKeyPress;
	CKeyBind  AimbotKeyBind;
	CKeyBind  AimbotStopKey;

	// Target Selection Settings
	CGroupBox TargetGroup;
	CComboBox TargetSelection;
	CCheckBox TargetFriendlyFire;
	CComboBox TargetHitbox;
	CComboBox TargetHitscan;
	CCheckBox TargetMultipoint;
	CSlider   TargetPointscale;

	// Accuracy Settings
	CGroupBox AccuracyGroup;
	CCheckBox AccuracyRecoil;
	CCheckBox AccuracyAutoWall;
	CSlider	  AccuracyMinimumDamage;
	CCheckBox AccuracyAutoStop;
	CCheckBox AccuracyAutoCrouch;
	CCheckBox AccuracyAutoScope;
	CSlider   AccuracyHitchance;
	CComboBox AccuracyAngleFix;
	CComboBox AccuracyAngleFixY;
	CSlider   AccuracySmart;

	// Anti-Aim Settings
	CGroupBox AntiAimGroup;
	CCheckBox AntiAimEnable;
	CComboBox AntiAimPitch;
	CSlider   AntiAimPitchSlider;
	CComboBox AntiAimYaw;
	CSlider   RealAntiAimYawSlider;
	CComboBox FakeAntiAimYaw;
	CSlider   FakeAntiAimYawSlider;
	CCheckBox AccuracyPrediction;
	// CCheckBox AntiAimEdge;

	CSlider   AntiAimTicks;
	CSlider   AntiAimPercentage;

	// CSlider	  AntiAimOffset;
	CSlider   AntiAimSpinspeed;
	CCheckBox AntiAimKnife;
	CCheckBox AntiAimTarget;
};

class CLegitBotTab : public CTab
{
public:
	void Setup();

	// Master Switch
	CLabel ActiveLabel;
	CCheckBox Active;

	// Aimbot Settings
	CGroupBox AimbotGroup;
	CCheckBox AimbotEnable;
	CCheckBox AimbotAutoFire;
	CCheckBox AimbotFriendlyFire;
	CCheckBox AimbotKeyPress;
	CKeyBind  AimbotKeyBind;
	CCheckBox AimbotAutoPistol;
	CSlider   AimbotInaccuracy;
	CKeyBind  AimbotDisableVis;

	// Main
	CGroupBox TriggerGroup;
	CCheckBox TriggerEnable;
	CCheckBox TriggerKeyPress;
	CKeyBind  TriggerKeyBind;
	CSlider   TriggerDelay;

	// Main
	CGroupBox WeaponMainGroup;
	CSlider   WeaponMainSpeed;
	CSlider   WeaponMainFoV;
	CCheckBox WeaponMainRecoil;
	CCheckBox WeaponMainPSilent;
	CSlider   WeaponMainInacc;
	CComboBox WeaponMainHitbox;

	// Pistol
	CGroupBox WeaponPistGroup;
	CSlider   WeaponPistSpeed;
	CSlider   WeaponPistFoV;
	CCheckBox WeaponPistRecoil;
	CCheckBox WeaponPistPSilent;
	CSlider   WeaponPistInacc;
	CComboBox WeaponPistHitbox;

	// Sniper
	CGroupBox WeaponSnipGroup;
	CSlider   WeaponSnipSpeed;
	CSlider   WeaponSnipFoV;
	CCheckBox WeaponSnipRecoil;
	CCheckBox WeaponSnipPSilent;
	CSlider   WeaponSnipInacc;
	CComboBox WeaponSnipSkitbox;
};

class CVisualTab : public CTab
{
public:
	void Setup();

	// Master Switch
	CLabel ActiveLabel;
	CCheckBox Active;

	// Options Settings
	CGroupBox OptionsGroup;
	CCheckBox OptionsBox;
	CComboBox BoxDesign;
	CCheckBox OptionsName;
	CCheckBox OptionsHealth;
	CCheckBox OptionsWeapon;
	CCheckBox OptionsInfo;
	CCheckBox OptionsHelmet;
	CCheckBox OptionsKit;
	CCheckBox OptionsDefuse;
	CCheckBox OptionsAr;
	//CCheckBox OptionsGlow;
	CComboBox OptionsChams;
	CCheckBox OptionsSkeleton;
	CCheckBox OptionsAimSpot;
	CCheckBox OptionsCompRank;

	// Skeleton ESP Colour
	CGroupBox ESPColorSkeleton;
	CSlider   RSkele;
	CSlider   GSkele;
	CSlider   BSkele;
	CSlider   ASkele;

	// Box ESP Colour
	CGroupBox ESPColorBox;
	CSlider   RBoxCT;
	CSlider   GBoxCT;
	CSlider   BBoxCT;
	CSlider   ABoxCT;
	CSlider   RBoxCTV;
	CSlider   GBoxCTV;
	CSlider   BBoxCTV;
	CSlider   ABoxCTV;

	// Filters Settings
	CGroupBox FiltersGroup;
	CCheckBox FiltersAll;
	CCheckBox FiltersPlayers;
	CCheckBox FiltersEnemiesOnly;
	CCheckBox FiltersWeapons;
	CCheckBox FiltersChickens;
	CCheckBox FiltersC4;

	// Other Settings
	CGroupBox OtherGroup;
	CComboBox OtherCrosshair;
	CComboBox OtherRecoilCrosshair;
	CCheckBox OtherHitlerCrosshair;
	CCheckBox OtherHitmarker;
	CCheckBox OtherRadar;
	CCheckBox OtherNoVisualRecoil;
	CCheckBox OtherNoSky;
	CCheckBox OtherNoFlash;
	CCheckBox OtherNoSmoke;
	CCheckBox OtherAsusWalls;
	CComboBox OtherNoHands;
	CCheckBox OtherThirdperson;
	CSlider OtherViewmodelFOV;
	CSlider OtherFOV;
	CCheckBox CustomSky;
	CCheckBox OtherNoScope;
	CCheckBox BulletTrace;
	CSlider   TraceLength;
	

};

class CMiscTab : public CTab
{
public:
	void Setup();

	// Knife Changer
	CGroupBox KnifeGroup;
	CCheckBox KnifeEnable;
	CComboBox KnifeModel;
	CComboBox KnifeSkin;
	CButton   KnifeApply;

	// Other Settings
	CGroupBox OtherGroup;
	CCheckBox OtherSafeModeEnable;
	CComboBox OtherChatSpam;
	CCheckBox OtherTeamChat;
	CSlider	  OtherChatDelay;
	CKeyBind  OtherAirStuck;
	CKeyBind  OtherLagSwitch;
	CCheckBox OtherSpectators;
	CGroupBox ConfigGroup;
	CComboBox ConfigBox;
	//CCheckBox OtherAutoAccept;
	CCheckBox OtherWalkbot;
	CComboBox OtherClantag;

	// Fake Lag Settings
	CGroupBox FakeLagGroup;
	CCheckBox FakeLagEnable;
	CSlider   FakeLagChoke;
	CSlider	  FakeLagSend;
	CCheckBox ChokeRandomize;
	CCheckBox SendRandomize;
	//CCheckBox FakeLagWhileShooting;

	// Teleport shit cause we're cool
	CGroupBox TeleportGroup;
	CCheckBox TeleportEnable;
	CKeyBind  TeleportKey;

	// Strafing
	CGroupBox StrafingGroup;
	CCheckBox OtherAutoJump;
	CComboBox OtherAutoStrafe;
	CSlider   OtherStrafeSpeed;
	CCheckBox OtherEdgeJump;
	CCheckBox Othercirclestrafer;
	CCheckBox Watermark;
	CCheckBox OtherLBYIndicator;

	// Rank Reveal
	CGroupBox RankGroup;
	CCheckBox RankRevealer;



};

class CSkinTab : public CTab
{
public:
	void Setup();

	// Knife Changer/Skin Changer
	CLabel SkinActive;
	CCheckBox SkinEnable;
	CButton   SkinApply;

	// Knife
	CGroupBox KnifeGroup;
	CComboBox KnifeModel;
	CComboBox KnifeSkin;

	// Pistols
	CGroupBox PistolGroup;
	CComboBox GLOCKSkin;
	CComboBox USPSSkin;
	CComboBox DEAGLESkin;
	CComboBox MAGNUMSkin;
	CComboBox DUALSSkin;
	CComboBox FIVESEVENSkin;
	CComboBox TECNINESkin;
	CComboBox P2000Skin;
	CComboBox P250Skin;

	// MPs
	CGroupBox MPGroup;
	CComboBox MAC10Skin;
	CComboBox P90Skin;
	CComboBox UMP45Skin;
	CComboBox BIZONSkin;
	CComboBox MP7Skin;
	CComboBox MP9Skin;

	// Rifles
	CGroupBox Riflegroup;
	CComboBox M41SSkin;
	CComboBox M4A4Skin;
	CComboBox AK47Skin;
	CComboBox AUGSkin;
	CComboBox FAMASSkin;
	CComboBox GALILSkin;
	CComboBox SG553Skin;


	// Machineguns
	CGroupBox MachinegunsGroup;
	CComboBox NEGEVSkin;
	CComboBox M249Skin;

	// Snipers
	CGroupBox Snipergroup;
	CComboBox SCAR20Skin;
	CComboBox G3SG1Skin;
	CComboBox SSG08Skin;
	CComboBox AWPSkin;

	// Shotguns
	CGroupBox Shotgungroup;
	CComboBox MAG7Skin;
	CComboBox XM1014Skin;
	CComboBox SAWEDOFFSkin;
	CComboBox NOVASkin;
};

class CGUITab : public CTab
{
public:
	void Setup();

	// Main Menu
	CGroupBox ColorMain;
	CSlider   RMainMenuA;
	CSlider   GMainMenuA;
	CSlider   BMainMenuA;
	CSlider   AMainMenuA;
	CSlider   RMainMenuB;
	CSlider   GMainMenuB;
	CSlider   BMainMenuB;
	CSlider   AMainMenuB;

	// Menu Text
	CGroupBox ColorText;
	CSlider   RMenuText;
	CSlider   GMenuText;
	CSlider   BMenuText;

	// Inner Window
	CGroupBox ColorInner;
	CSlider   RInner;
	CSlider   GInner;
	CSlider   BInner;
	CSlider   AInner;

	// Tabs
	CGroupBox ColorTabs;
	CSlider   RTabs;
	CSlider   GTabs;
	CSlider   BTabs;

};


class AyyWareWindow : public CWindow
{
public:
	void Setup();

	CRageBotTab RageBotTab;
	CLegitBotTab LegitBotTab;
	CVisualTab VisualsTab;
	CMiscTab MiscTab;
	CSkinTab SkinTab;
	CGUITab GUITab;

	CButton SaveButton;
	CButton LoadButton;
	CButton UnloadButton;
	CComboBox GroupBox;
	CComboBox ConfigBox;
	CGroupBox ConfigGroup;

};

namespace Menu
{
	void SetupMenu();
	void DoUIFrame();

	extern AyyWareWindow Window;
};