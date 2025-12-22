#include <wx/wx.h>
#include <wx/display.h>
#include "MainFrame/MainFrame.h"


// Simple entry point for wx widgets application
class App : public wxApp
{
public:
	// Starts MainFrame, the primary view of MidiWorks
	bool OnInit()
	{
		mMainFrame = new MainFrame();
		SetScreenSizeAndPosition();
		mMainFrame->Show();
		return true;
	}

	// Starts window in center of screen at 2/3 the width and height
	void SetScreenSizeAndPosition()
	{
		wxDisplay display;
		wxRect screenRect = display.GetGeometry();
		int screenWidth  = screenRect.GetWidth();
		int screenHeight = screenRect.GetHeight();
		mMainFrame->SetClientSize(screenWidth * 2 / 3, screenHeight * 2 / 3);
		mMainFrame->CenterOnScreen();
	}

private: 
	MainFrame* mMainFrame;
};

// Line needed to run wx app 
wxIMPLEMENT_APP(App);
