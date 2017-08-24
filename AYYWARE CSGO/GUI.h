/*
Syn's AyyWare Framework 2015
*/

#pragma once

#include "CommonIncludes.h"

#include <map>

class CControl;
class CTab;
class CWindow;
class CGUI;
class CRadar;

extern CGUI GUI;
extern CRadar *Radar;

enum UIFlags
{
	UI_None = 0x00,
	UI_Drawable = 0x01,
	UI_Clickable = 0x02,
	UI_Focusable = 0x04,
	UI_RenderFirst = 0x08,
	UI_SaveFile = 0x10
};

enum UIControlTypes
{
	UIC_CheckBox = 1,
	UIC_Slider,
	UIC_KeyBind,
	UIC_ComboBox
};

// Base class for GUI controls
class CControl
{
	friend class CGUI;
	friend class CTab;
	friend class CWindow;
public:
	void SetPosition(int x, int y);
	void SetSize(int w, int h);
	void GetSize(int &w, int &h);
	void SetFileId(std::string fid);
	inline void SetFileID2(DWORD ID) { FileID = ID; }

	bool Flag(int f);
protected:
	int m_x, m_y;
	int m_iWidth, m_iHeight;
	int m_Flags;
	CWindow* parent;
	DWORD FileID;

	std::string FileIdentifier;
	int FileControlType;

	virtual void Draw(bool) = 0;
	virtual void OnUpdate() = 0;
	virtual void OnClick() = 0;

	POINT GetAbsolutePos();
};

// A GUI Control Container
class CTab
{
	friend class CControl;
	friend class CGUI;
	friend class CWindow;
public:
	void SetTitle(std::string name);
	void RegisterControl(CControl* control);
private:
	std::string Title;
	std::vector<CControl*> Controls;
	CWindow* parent;
};

// Base class for a simple GUI window
class CWindow
{
	friend class CControl;
	friend class CGUI;
public:
	void SetPosition(int x, int y);
	void SetSize(int w, int h);
	void SetTitle(std::string title);
	void Open();
	void Close();
	void Toggle();
	CControl* GetFocus();

	void RegisterTab(CTab* Tab);

	RECT GetClientArea();
	RECT GetTabArea();
	RECT GetDragArea();
	RECT GetClientArea1();

private:
	void DrawControls();

	bool m_bIsOpen;

	bool m_HasTabs;

	std::vector<CTab*> Tabs;
	CTab* SelectedTab;

	bool IsFocusingControl;
	CControl* FocusedControl;

	std::string Title;
	int m_x;
	int m_y;
	int m_iWidth;
	int m_iHeight;

};

class CRadar {
	friend class CControl;
	friend class CGUI;
public:
	void SetPosition(int x, int y);
	void SetSize(int w, int h);
	void SetTitle(std::string title);

private:
	std::string Title;
	int m_x;
	int m_y;
	int m_iWidth;
	int m_iHeight;
};


// User interface manager
class CGUI
{
public:
	CGUI();

	// Draws all windows 
	void Draw();

	// Handle all input etc
	void Update();

	// Draws a single window

	bool DrawWindow(CWindow* window, int menu);
	// Draws radar window
	bool DrawRadar();


	// Registers a window
	void RegisterWindow(CWindow* window);

	// Config saving/loading
	void SaveWindowState(CWindow* window, std::string Filename);
	void LoadWindowState(CWindow* window, std::string Filename);

	// Window Binds
	void BindWindow(unsigned char Key, CWindow* window);

	// Input
	bool GetKeyPress(unsigned int key);
	bool GetKeyState(unsigned int key);
	bool IsMouseInRegion(int x, int y, int x2, int y2);
	bool IsMouseInRegion(RECT region);
	POINT GetMouse();



private:
	// Input
	// keyboard
	bool keys[256];
	bool oldKeys[256];
	// Mouse
	POINT Mouse;
	bool MenuOpen;

	// Window Dragging
	bool IsDraggingWindow;
	CWindow* DraggingWindow;
	CRadar* DraggingRadar;
	int DragOffsetX; int DragOffsetY;

	// Windows
	std::vector<CWindow*> Windows;

	// KeyBinds -> Windows Map
	std::map<int, CWindow*> WindowBinds;

};

namespace Globals
{
	extern float MenuAlpha2;
}