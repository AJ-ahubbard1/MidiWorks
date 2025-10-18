#include <wx/wx.h>
#include <wx/display.h>
#include "MainFrame/MainFrame.h"

class App : public wxApp
{
public:
	bool OnInit()
	{
		mMainFrame = new MainFrame();
		SetScreenSizeAndPosition();
		mMainFrame->Show();
		return true;
	}

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

wxIMPLEMENT_APP(App);
