#pragma once

typedef union ListOutMsg
{
	static const int MaxLines = 0x100;
	enum WindowMessage
	{
		WM_MidiMsg = WM_USER + 0x100,
		WM_MidiUpdate,
		WM_MidiErr,
	};

	//UINT32 Tick, Track;
	//union {
	//	UINT32 Msg : 24;
	//	struct { UINT8 Cmd, Param1, Param2; };
	//};
	struct {
		UINT32 Cmd : 7;
		UINT32 Param1 : 7;
		UINT32 Param2 : 7;
		UINT32 Track : 11;
	};
	UINT32 Msg;
} ListOutMsg, *PListOutMsg, &RListOutMsg;

extern const LPCTSTR EqualizerNames[];
extern const INT8 EqualizerPresets[][12];
extern const LPCTSTR MatrixerNames[];
extern const INT16 MatrixerPresets[][4];
