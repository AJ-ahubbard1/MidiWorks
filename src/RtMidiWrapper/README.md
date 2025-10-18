# Setup for RtMidi for Windows build
1. Add RtMidi.h and RtMidi.cpp file into Solution  
2. Add the following two lines at the start of the RtMidi.h file  
	#define __WINDOWS_MM__  
	#pragma comment (lib, "winmm.lib")  
3. In the Solutions Properties:  
	Linker > Input >> Additional dependencies: add winmm.lib 


# Design Principles

## AppModel Contains the data of the App
All logic that reads/manipulates that data should be inside AppModel,
or the corresponding member of AppModel (SoundBank, PanelInfo, etc.).

### The Panels contain the layout of the views and the event bindings.
An eventhandler can contain wxlogic and call the functions created in AppModel to 
manipulate data. 



