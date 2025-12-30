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
	wxAcceleratorEntry entries[15];
	entries[0].Set(wxACCEL_NORMAL, WXK_SPACE, ID_KEYBOARD_TOGGLE_PLAY);		// Spacebar = Toggle Play
	entries[1].Set(wxACCEL_NORMAL, 'R', ID_KEYBOARD_RECORD);				// R = Record
	entries[2].Set(wxACCEL_NORMAL, 'Q', ID_KEYBOARD_QUANTIZE);				// Q = Quantize
	entries[3].Set(wxACCEL_NORMAL, WXK_LEFT, ID_KEYBOARD_PREVIOUS_MEASURE); // LEFT Arrow = Previous Measure
	entries[4].Set(wxACCEL_NORMAL, WXK_RIGHT, ID_KEYBOARD_NEXT_MEASURE);	// RIGHT Arrow = Next Measure

	// Drum pad triggers (1-0 keys = rows 0-9)
	entries[5].Set(wxACCEL_NORMAL, '1', ID_KEYBOARD_DRUM_PAD_1);			// 1 = Drum Row 0
	entries[6].Set(wxACCEL_NORMAL, '2', ID_KEYBOARD_DRUM_PAD_2);			// 2 = Drum Row 1
	entries[7].Set(wxACCEL_NORMAL, '3', ID_KEYBOARD_DRUM_PAD_3);			// 3 = Drum Row 2
	entries[8].Set(wxACCEL_NORMAL, '4', ID_KEYBOARD_DRUM_PAD_4);			// 4 = Drum Row 3
	entries[9].Set(wxACCEL_NORMAL, '5', ID_KEYBOARD_DRUM_PAD_5);			// 5 = Drum Row 4
	entries[10].Set(wxACCEL_NORMAL, '6', ID_KEYBOARD_DRUM_PAD_6);			// 6 = Drum Row 5
	entries[11].Set(wxACCEL_NORMAL, '7', ID_KEYBOARD_DRUM_PAD_7);			// 7 = Drum Row 6
	entries[12].Set(wxACCEL_NORMAL, '8', ID_KEYBOARD_DRUM_PAD_8);			// 8 = Drum Row 7
	entries[13].Set(wxACCEL_NORMAL, '9', ID_KEYBOARD_DRUM_PAD_9);			// 9 = Drum Row 8
	entries[14].Set(wxACCEL_NORMAL, '0', ID_KEYBOARD_DRUM_PAD_0);			// 0 = Drum Row 9

	wxAcceleratorTable accelTable(15, entries);
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

	// Drum Pad Triggers
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnDrumPad, mMainFrame, ID_KEYBOARD_DRUM_PAD_1);
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnDrumPad, mMainFrame, ID_KEYBOARD_DRUM_PAD_2);
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnDrumPad, mMainFrame, ID_KEYBOARD_DRUM_PAD_3);
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnDrumPad, mMainFrame, ID_KEYBOARD_DRUM_PAD_4);
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnDrumPad, mMainFrame, ID_KEYBOARD_DRUM_PAD_5);
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnDrumPad, mMainFrame, ID_KEYBOARD_DRUM_PAD_6);
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnDrumPad, mMainFrame, ID_KEYBOARD_DRUM_PAD_7);
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnDrumPad, mMainFrame, ID_KEYBOARD_DRUM_PAD_8);
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnDrumPad, mMainFrame, ID_KEYBOARD_DRUM_PAD_9);
	mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnDrumPad, mMainFrame, ID_KEYBOARD_DRUM_PAD_0);
}
