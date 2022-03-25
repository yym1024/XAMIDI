//=============================================================================
//作者：0xAA55
//论坛：http://www.0xaa55.com/
//版权所有(C) 2013-2015 技术宅的结界
//请保留原作者信息，否则视为侵权。
//-----------------------------------------------------------------------------
#ifndef _MIDI_Parser_
#define	_MIDI_Parser_

#include<stdint.h>

//=============================================================================
//宏，可以用于调整调用约定、符号导入导出规则等
//-----------------------------------------------------------------------------
//MIDIFILE的函数调用约定
#ifndef Midi_c
#define Midi_c _cdecl
#endif // !Midi_c

//MIDIFILE的回调函数调用约定
#ifndef Midi_cb
#define Midi_cb _cdecl
#endif // !Midi_cb

//MIDIFILE的符号导出规则
#ifndef Midi_x
  #ifdef __cplusplus
  #define Midi_x extern"C"
  #else // !__cplusplus
  #define Midi_x extern
  #endif // !__cplusplus
#endif // !Midi_x

//MIDIFILE的导出函数
#define Midi_fn(fn,rt) Midi_x rt Midi_c fn

//用户自定义数据的格式
#ifndef Midi_UserData_t
#define Midi_UserData_t void
#endif

#ifndef Midi_UserData_p
#define Midi_UserData_p Midi_UserData_t*
#endif

//为了防止出现fpos_t未定义的问题，这里定义一次。
typedef __int64 fpos_t;

//Midi时间刻度
#ifndef MidiTime_t
#define MidiTime_t double
#endif

#ifndef MidiTime_p
#define MidiTime_p MidiTime_t*
#endif
//-----------------------------------------------------------------------------




//=============================================================================
//错误代码
//-----------------------------------------------------------------------------
typedef enum
{
	ME_OK=0,		//无错误
	ME_OutOfMem,	//内存不足
	ME_TellFail,	//取得文件指针失败
	ME_SeekFail,	//设置文件指针失败
	ME_ReadFail,	//读取失败
	ME_BadFile		//文件格式错误
}MidiErrCode_t,*MidiErrCode_p;




//=============================================================================
//用户回调函数，用户必须提供实现的函数
//-----------------------------------------------------------------------------
//读取文件：要求成功返回非零，失败返回0
typedef int(Midi_cb*ReadFile_f)
(
	void	*pBuf,		//缓冲区
	size_t	cb,			//要读取的字节数
	Midi_UserData_p pUserData	//用户数据
);
//-----------------------------------------------------------------------------
//取得文件指针：要求成功返回非零，失败返回0
typedef int(Midi_cb*GetFilePtr_f)
(
	fpos_t	*pPosition,	//取得位置
	Midi_UserData_p pUserData	//用户数据
);
//-----------------------------------------------------------------------------
//设置文件指针：要求成功返回非零，失败返回0
typedef int(Midi_cb*SetFilePtr_f)
(
	fpos_t	Position,	//新的位置
	Midi_UserData_p pUserData	//用户数据
);
//-----------------------------------------------------------------------------
//发送MIDI命令
typedef void(Midi_cb*SendMidiMsg_f)
(
	uint32_t TickCount,	//当前命令时钟
	uint16_t TrackNo,	//当前音轨号
	uint8_t	Command,	//命令
	uint8_t	Param1,		//参数1
	uint8_t	Param2,		//参数2
	Midi_UserData_p pUserData	//用户数据
);
//-----------------------------------------------------------------------------
//错误处理
typedef void(Midi_cb*Error_f)(MidiErrCode_t Err, Midi_UserData_p pUserData);
//-----------------------------------------------------------------------------


//=============================================================================
//MIDI解析器的音轨数据
//-----------------------------------------------------------------------------
typedef struct
{
	fpos_t	StartPos;	//音轨开始的文件偏移
	fpos_t	TrackSize;	//音轨大小
	fpos_t	NextNotePos;//音轨下个音符的文件偏移
	uint32_t LastNoteTick;//上个音符的时钟
	uint32_t TotalTicks;//这个音轨的总时钟数
	uint8_t EndOfTrack;	//是否到达了当前轨道的末尾
	uint8_t	LastCommand;//上个命令
	uint8_t	LastParam1;	//上个命令的参数1
	uint8_t	LastParam2;	//上个命令的参数2
}TrackData_t,*TrackData_p;


//=============================================================================
//MIDI解析器结构体
//-----------------------------------------------------------------------------
typedef struct
{
	//=========================================================================
	//用户需要先填写如下的参数，才能使用MidiFile接口
	ReadFile_f		pfnReadFile;		//读取文件回调
	GetFilePtr_f	pfnGetFilePtr;		//设置文件指针回调
	SetFilePtr_f	pfnSetFilePtr;		//设置文件指针回调
	SendMidiMsg_f	pfnSendMidiMsg;		//发送Midi指令回调
	Error_f			pfnOnError;			//错误处理回调
	Midi_UserData_p	pUserData;			//用户自定义数据
	//-------------------------------------------------------------------------

	uint16_t		wType;              //Midi文件类型
	uint16_t		wNbTracks;          //音轨数
	uint16_t		wTicksPerCrotchet;  //每四分音符的Tick数
	TrackData_p		pTrackData;			//音轨数据
	uint32_t		Velocity;			//播放速度，一个四分音符的微秒数
	uint32_t		TotalTicks;			//整个Midi文件的时钟数（长度）
	uint8_t			EndOfFile;			//是否已经更新到了结尾

	MidiErrCode_t	LastErr;			//错误代码
}MidiParser_t,*MidiParser_p;

//MIDI文件的类型
#define MIDIT_SingleTrack   0   /*单音轨*/
#define MIDIT_MultiSync     1   /*同步多音轨*/
#define MIDIT_MultiAsync    2   /*异步多音轨*/

//=============================================================================
//MidiParserStart:
//分析MIDI文件，为播放做准备。失败返回零，成功返回非零
//-----------------------------------------------------------------------------
Midi_fn(MidiParserStart,int)
(
	MidiParser_p pParser
);

//=============================================================================
//MidiParserUpdate:
//更新播放状态到当前时间
//-----------------------------------------------------------------------------
Midi_fn(MidiParserUpdate,int)
(
	MidiParser_p	pParser,
	MidiTime_t		dbMilliSeconds,
	uint8_t			uSilent
);

//=============================================================================
//MidiParserReset:
//重置Midi解析器状态
//-----------------------------------------------------------------------------
Midi_fn(MidiParserReset,void)
(
	MidiParser_p pParser
);

//=============================================================================
//MidiParserShutdown:
//关闭Midi解析器
//-----------------------------------------------------------------------------
Midi_fn(MidiParserShutdown,void)
(
	MidiParser_p pParser
);

/******************************************************************************
注释

1、播放
·用户需要自己编写读取文件、定位文件指针的函数、发送Midi字节的函数（按照本头文
件所描述）的规格进行编写。然后按照说明填写MidiParser_t结构体的成员。
·填写完成后，调用MidiParserStart，初始化播放器。
·随后，不断调用MidiParserUpdate（并提供播放时间）进行Midi文件的播放。播放时间
的数值只能向后增长。
·播放完最后一个音符后，MidiParser_t的成员EndOfFile将会被置1。如需重新播放，请
调用MidiParserReset，然后重新计时并重复上一个步骤。
·不需要播放的时候，请调用MidiParserShutdown完成最后的处理。
·请注意，如果你在Windows使用mciSendCommand播放Midi音乐，你会遇到跳音符的问题。
而本库所使用的算法比Windows的mciSendCommand好，因此没有这个问题。请使用本库。

2、定位到指定播放位置（慎用：可能导致播放乐曲的效果偏离作曲者的设定）
·调用MidiParserReset重置播放状态。
·随后，调用MidiParserUpdate并提供你要定位的时间值（如果设置Silent参数的话，
MidiParserUpdate将不会发送Midi字节）。

3、本程序也可用于Midi文件的解析器——获取每个Midi指令、时间、轨道等信息。

注：本程序对于格式有问题的Midi文件并不能做到很好的解析——要求：
·每个Midi音轨的最后一个音符必须是元数据——音轨结束标识

******************************************************************************/


#endif // !_MIDI_Parser_
