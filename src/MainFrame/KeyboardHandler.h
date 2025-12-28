// KeyboardHandler.h
#pragma once
#include <wx/wx.h>
#include <memory>
#include "MainFrameIDs.h"

class MainFrame;  // Forward declaration
class AppModel;   // Forward declaration

class KeyboardHandler
{
public:
	KeyboardHandler(MainFrame* mainFrame, std::shared_ptr<AppModel> appModel);

	void Initialize();

private:
	MainFrame* mMainFrame;
	std::shared_ptr<AppModel> mAppModel;

	void SetupShortCuts();
};

