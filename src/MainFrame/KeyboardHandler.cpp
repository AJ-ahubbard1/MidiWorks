// KeyboardHandler.cpp
#include "KeyboardHandler.h"
#include "MainFrame.h"

KeyboardHandler::KeyboardHandler(MainFrame* mainFrame, std::shared_ptr<AppModel> appModel)
	: mMainFrame(mainFrame)
	, mAppModel(appModel)
{
}

void KeyboardHandler::Initialize()
{
	wxAcceleratorEntry entries[5];
	entries[0].Set(wxACCEL_NORMAL, WXK_SPACE, ID_KEYBOARD_TOGGLE_PLAY);		// Spacebar = Toggle Play
	entries[1].Set(wxACCEL_NORMAL, 'R', ID_KEYBOARD_RECORD);				// R = Record
	entries[2].Set(wxACCEL_NORMAL, 'Q', ID_KEYBOARD_QUANTIZE);				// Q = Quantize
	entries[3].Set(wxACCEL_NORMAL, WXK_LEFT, ID_KEYBOARD_PREVIOUS_MEASURE); // LEFT Arrow = Previous Measure
	entries[4].Set(wxACCEL_NORMAL, WXK_RIGHT, ID_KEYBOARD_NEXT_MEASURE);	// RIGHT Arrow = Next Measure

	wxAcceleratorTable accelTable(5, entries);
	mMainFrame->SetAcceleratorTable(accelTable);

	SetupShortCuts();
}

void KeyboardHandler::SetupShortCuts()
{
	// Transport
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnTogglePlay, mMainFrame, ID_KEYBOARD_TOGGLE_PLAY);
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnStartRecord, mMainFrame, ID_KEYBOARD_RECORD);
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnPreviousMeasure, mMainFrame, ID_KEYBOARD_PREVIOUS_MEASURE);
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnNextMeasure, mMainFrame, ID_KEYBOARD_NEXT_MEASURE);
}
