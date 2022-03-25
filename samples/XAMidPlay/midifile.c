//=============================================================================
//作者：0xAA55
//论坛：http://www.0xaa55.com/
//版权所有(C) 2013-2015 技术宅的结界
//请保留原作者信息，否则视为侵权。
//-----------------------------------------------------------------------------
#include"midifile.h"

#include<stdio.h>
#include<malloc.h>
#include<memory.h>

#pragma pack(push,1)
//MIDI文件头的结构体
typedef struct
{
	uint32_t	dwFlag;             //MThd标识
	uint32_t	dwRestLen;          //剩余部分长度
	uint16_t	wType;              //类型
	uint16_t	wNbTracks;          //音轨数
	uint16_t	wTicksPerCrotchet;  //每四分音符的Tick数
}MIDIHeader,*MIDIHeaderP;
#pragma pack(pop,1)

//MIDI文件中出现过的标识
#define MIDI_MThd   0x6468544D
#define MIDI_MTrk   0x6B72544D

//各种长度的Big-Endian到Little-Endian的转换
#define BSwapW(x)   ((((x) & 0xFF)<<8)|(((x) & 0xFF00)>>8))
#define BSwap24(x)  ((((x) & 0xFF)<<16)|((x) & 0xFF00)|(((x) & 0xFF0000)>>16))
#define BSwapD(x)   ((((x) & 0xFF)<<24)|(((x) & 0xFF00)<<8)|(((x) & 0xFF0000)>>8)|(((x) & 0xFF000000)>>24))

//用于读取文件的宏。要使用它，函数必须要有MidiParser_p pParser参数
#define ErrC(e) {pParser->LastErr=(e);pParser->pfnOnError((e),pParser->pUserData);goto BadRet;}
#define Tell(x) if(!pParser->pfnGetFilePtr((x),pParser->pUserData))\
	ErrC(ME_TellFail)
#define Seek(x) if(!pParser->pfnSetFilePtr((x),pParser->pUserData))\
	ErrC(ME_SeekFail)
#define Read(x) if(!pParser->pfnReadFile(&(x),sizeof((x)),pParser->pUserData))\
	ErrC(ME_ReadFail)
#define Skip(x) {fpos_t __CurPos;Tell(&__CurPos);__CurPos+=(x);Seek(__CurPos);}

//=============================================================================
//ReadDynamicByte:
//读取动态字节，最多读取4字节。返回读取的字节数
//-----------------------------------------------------------------------------
static size_t ReadDynamicByte(MidiParser_p pParser,uint32_t*pOut)
{
	size_t BytesRead;//已读取的字节数
	uint32_t uVal=0;
	for(BytesRead=1;BytesRead<=4;BytesRead++)//最多读取4字节
	{
		uint8_t Value;
		Read(Value);//读取一个字节
		uVal=(uVal<<7)|(Value & 0x7F);//新读入的是低位
		if(!(Value & 0x80))//如果没有后续字节
			break;//就停止读取。
	}
	*pOut=uVal;
	return BytesRead;//返回读取的字节数
BadRet:
	*pOut=uVal;
	return 0;
}

//=============================================================================
//GetCommandNbParams:
//取得MIDI命令的参数个数
//-----------------------------------------------------------------------------
static uint8_t GetCommandNbParams(uint8_t bEvent)
{
		 if(bEvent <= 0x7F)return 0;//上个命令的参数
	else if(bEvent <= 0x8F)return 2;//两个参数
	else if(bEvent <= 0x9F)return 2;//两个参数
	else if(bEvent <= 0xAF)return 2;//两个参数
	else if(bEvent <= 0xBF)return 2;//两个参数
	else if(bEvent <= 0xCF)return 1;//一个参数
	else if(bEvent <= 0xDF)return 1;//一个参数
	else if(bEvent <= 0xEF)return 2;//两个参数
	else if(bEvent <= 0xFF)return 0;//机器码
	else return 2;//元数据
}

//=============================================================================
//DefOnError:
//默认的错误提示函数
//-----------------------------------------------------------------------------
static void Midi_cb DefOnError(MidiErrCode_t Err, Midi_UserData_p pUserData)
{
	//什么也不做
}

//=============================================================================
//MidiParserStart:
//分析MIDI文件，为播放做准备。失败返回零，成功返回非零
//-----------------------------------------------------------------------------
Midi_fn(MidiParserStart,int)
(
	MidiParser_p pParser
)
{
	MIDIHeader midiHeader;
	size_t i;

	pParser->wType=0;
	pParser->wNbTracks=0;
	pParser->wTicksPerCrotchet=0;
	pParser->pTrackData=NULL;
	pParser->Velocity=0;
	pParser->TotalTicks=0;
	pParser->EndOfFile=0;
	pParser->LastErr=ME_OK;

	if(!pParser->pfnOnError)
		pParser->pfnOnError=DefOnError;

	//MIDI文件头就是一个MIDIHeader结构体。
	//但是要注意其中的数值都是Big-Endian存储的
	//需要进行转换

	//读取MIDI文件头
	Read(midiHeader);
	
	//检查文件格式
	if(midiHeader.dwFlag!=MIDI_MThd)//标识必须是"MThd"
		ErrC(ME_BadFile);

	//转换为Little-Endian
	midiHeader.dwRestLen=           BSwapD(midiHeader.dwRestLen);
	midiHeader.wType=               BSwapW(midiHeader.wType);
	midiHeader.wNbTracks=           BSwapW(midiHeader.wNbTracks);
	midiHeader.wTicksPerCrotchet=   BSwapW(midiHeader.wTicksPerCrotchet);

	//分析器数据
	pParser->wType=midiHeader.wType;
	pParser->wNbTracks=midiHeader.wNbTracks;
	pParser->wTicksPerCrotchet=midiHeader.wTicksPerCrotchet;

	//跳转到MIDI音轨的位置
	Seek(8+midiHeader.dwRestLen);

	//轨道信息
	i=sizeof(TrackData_t)*pParser->wNbTracks;//轨道信息总大小
	pParser->pTrackData=(TrackData_p)malloc(i);//分配内存
	if(!pParser->pTrackData)
		ErrC(ME_OutOfMem);
	memset(pParser->pTrackData,0,i);//清零

	//准备读取各个音轨
	for(i=0;i<midiHeader.wNbTracks;i++)
	{
		uint32_t dwTrackFlag;
		uint32_t dwTrackLen;

		uint8_t LastEvent=0;//上一个事件
		uint8_t EndOfTrack;//是否读取到了音轨的结束位置

		//每个音轨的开头都是一个dwTrackFlag和一个dwTrackLen
		//dwTrackFlag的值必须是MIDI_MTrk
		//dwTrackLen标记了下一个音轨的位置

		fpos_t TrackStartPos;

		//轨道标记
		Read(dwTrackFlag);
		if(dwTrackFlag!=MIDI_MTrk)
			ErrC(ME_BadFile);

		//轨道长度
		Read(dwTrackLen);
		dwTrackLen=BSwapD(dwTrackLen);//转换Big-Endian

		Tell(&TrackStartPos);//记录当前位置

		pParser->pTrackData[i].StartPos=TrackStartPos;
		pParser->pTrackData[i].TrackSize=dwTrackLen;
		pParser->pTrackData[i].NextNotePos=TrackStartPos;
		pParser->pTrackData[i].LastNoteTick=0;
		pParser->pTrackData[i].EndOfTrack=0;

		//音轨的重要内容是事件数据
		for(EndOfTrack=0;!EndOfTrack;)//循环读取事件
		{
			uint32_t dwDelay;
			uint8_t bEvent;

			//每个事件的构成都是：
			//延时，事件号，参数
			//其中的延时是动态字节，参数大小随事件号而定

			//读取延时
			if(!ReadDynamicByte(pParser,&dwDelay))
				ErrC(ME_BadFile);

			//统计当前音轨总时钟数
			pParser->pTrackData[i].TotalTicks+=dwDelay;

			//读取事件号
			Read(bEvent);

			//分析事件
			if(bEvent<=0x7F)
			{//0到0x7F:和上一个事件的事件号相同，而读取的这个字节就是上一个事件
			//号的参数的第一个字节
				Skip(GetCommandNbParams(LastEvent)-1);
				bEvent=LastEvent;
			}
			else if(bEvent <= 0xEF)
			{//基本的MIDI命令
				Skip(GetCommandNbParams(bEvent));
			}
			else if(bEvent == 0xF0)
			{//0xF0:系统码
				uint8_t bSysCode=0;
				do
				{
					Read(bSysCode);
				}while(bSysCode!=0xF7);
			}
			else if(bEvent == 0xFF)
			{//元事件
				uint8_t bType,Bytes;
				fpos_t CurrentPos;
				
				Read(bType);//元数据类型
				Read(Bytes);//元数据字节数

				Tell(&CurrentPos);//记录元数据读取的位置

				switch(bType)
				{
				case 0x2F://音轨结束标识
					EndOfTrack=1;
					break;
				case 0x51://速度
					{
						uint8_t bVelocity1,bVelocity2,bVelocity3;

						//速度是24位整数，一个四分音符的微秒数。
						Read(bVelocity1);
						Read(bVelocity2);
						Read(bVelocity3);

						if(!pParser->Velocity)
							pParser->Velocity=
							(bVelocity3|
							(bVelocity2<<8)|
							(bVelocity1<<16))/pParser->wTicksPerCrotchet;
					}
					break;
				default:
					break;
				}

				Seek(CurrentPos+Bytes);//读取完成后正确跳到下一个事件的位置。
			}
			else//其它事件，未知事件
			{
				ErrC(ME_BadFile);
			}
			LastEvent=bEvent;
		}//回去继续读取事件。

		//如果是异步多音轨的话，记录整个Midi文件的播放时长
		if(pParser->wType==MIDIT_MultiAsync)
		{
			//异步多音轨，同级总时长
			pParser->TotalTicks+=pParser->pTrackData[i].TotalTicks;
		}
		else//否则（单音轨、同步多音轨）以时长最长的音轨为整个音乐的长度。
		{
			//同步多音轨或单音轨，找到时长最长的音轨。
			if(pParser->pTrackData[i].TotalTicks>pParser->TotalTicks)
				pParser->TotalTicks=pParser->pTrackData[i].TotalTicks;
		}
		Seek(TrackStartPos+dwTrackLen);//转到下一个音轨
	}

	return 1;
BadRet:
	MidiParserShutdown(pParser);
	return 0;
}

//=============================================================================
//MidiParserUpdate:
//更新播放状态到当前时间
//-----------------------------------------------------------------------------
Midi_fn(MidiParserUpdate,int)
(
	MidiParser_p	pParser,
	MidiTime_t		dbMilliSeconds,
	uint8_t			uSilent
)
{
	uint16_t CurTrack;
	uint32_t CurTick;

	if(dbMilliSeconds<0)
		dbMilliSeconds=0;

	//当前时钟数
	CurTick=(uint32_t)(dbMilliSeconds*1000.0/pParser->Velocity);

	//检查是否播放到了结尾
	if(CurTick>pParser->TotalTicks)
	{
		pParser->EndOfFile=1;
		return 1;
	}

	for(CurTrack=0;CurTrack<pParser->wNbTracks;CurTrack++)
	{
		TrackData_p pData=&(pParser->pTrackData[CurTrack]);

		//每个事件的构成都是：
		//延时，事件号，参数
		//其中的延时是动态字节，参数大小随事件号而定

		//异步多音轨单独处理
		if(pParser->wType==MIDIT_MultiAsync)
		{
			if(CurTick>=pData->TotalTicks)
			{
				CurTick-=pData->TotalTicks;
				continue;
			}
		}
		for(;!pData->EndOfTrack;)
		{
			uint32_t dwDelay;
			uint8_t bEvent;

			//定位到当前轨道的下一个音符的位置
			Seek(pData->NextNotePos);

			//读取延时
			if(!ReadDynamicByte(pParser,&dwDelay))
				ErrC(ME_BadFile);

			if(CurTick>=pData->LastNoteTick+dwDelay)
			{
				pData->LastNoteTick+=dwDelay;
				//读取事件号
				Read(bEvent);

				//分析事件
				if(bEvent <= 0x7F)
				{	//0到0x7F:和上一个事件的事件号相同，而读取的这个字节就是上一
					//个事件号的参数的第一个字节
					uint8_t NbParams=GetCommandNbParams(pData->LastCommand);
					if(NbParams==1)
					{
						pData->LastParam1=bEvent;
						if(!uSilent)
						{
							pParser->pfnSendMidiMsg(pData->LastNoteTick,
								CurTrack,pData->LastCommand,bEvent,0,
								pParser->pUserData);
						}
					}
					else if(NbParams==2)
					{
						uint8_t bParam2;
						Read(bParam2);
						pData->LastParam1=bEvent;
						pData->LastParam2=bParam2;
						if(!uSilent)
						{
							pParser->pfnSendMidiMsg(pData->LastNoteTick,
								CurTrack,pData->LastCommand,bEvent,bParam2,
								pParser->pUserData);
						}
					}
				}
				else if(bEvent<=0xEF)
				{
					uint8_t NbParams=GetCommandNbParams(bEvent);
					pData->LastCommand=bEvent;
					if(NbParams==1)
					{
						uint8_t bParam1;
						Read(bParam1);
						pData->LastParam1=bParam1;
						if(!uSilent)
						{
							pParser->pfnSendMidiMsg(pData->LastNoteTick,
								CurTrack,bEvent,bParam1,0,pParser->pUserData);
						}
					}
					else if(NbParams==2)
					{
						uint8_t bParam1,bParam2;
						Read(bParam1);
						Read(bParam2);
						pData->LastParam1=bParam1;
						pData->LastParam2=bParam2;
						if(!uSilent)
						{
							pParser->pfnSendMidiMsg(pData->LastNoteTick,
								CurTrack,bEvent,bParam1,bParam2,
								pParser->pUserData);
						}
					}
				}
				else if(bEvent == 0xF0)
				{//0xF0:系统码
					uint8_t bSysCode=0;
					do
					{
						Read(bSysCode);
					}while(bSysCode!=0xF7);
				}
				else if(bEvent == 0xFF)
				{//元事件
					uint8_t bType,Bytes;
					fpos_t CurrentPos;
				
					Read(bType);//元数据类型
					Read(Bytes);//元数据字节数

					Tell(&CurrentPos);//记录元数据读取的位置

					switch(bType)
					{
					case 0x2F://音轨结束标识
						pData->EndOfTrack=1;
						break;
					}

					Seek(CurrentPos+Bytes);//读取完成后正确跳到下一个事件的位置
				}
				else//其它事件，未知事件
				{
					ErrC(ME_BadFile);
				}
				Tell(&(pData->NextNotePos));
				if(pData->NextNotePos>=pData->StartPos+pData->TrackSize)
					pData->EndOfTrack=1;
			}
			else
				break;
		}

		//异步多音轨单独处理
		if(pParser->wType==MIDIT_MultiAsync)
		{
			//防止同步播放其它音轨
			if(CurTick<pData->TotalTicks)
				break;
		}
	}

	return 1;
BadRet:
	MidiParserShutdown(pParser);
	return 0;
}

//=============================================================================
//MidiParserReset:
//重置Midi解析器状态
//-----------------------------------------------------------------------------
Midi_fn(MidiParserReset,void)
(
	MidiParser_p pParser
)
{
	uint16_t CurTrack;

	for(CurTrack=0;CurTrack<pParser->wNbTracks;CurTrack++)
	{
		pParser->pTrackData[CurTrack].NextNotePos=
			pParser->pTrackData[CurTrack].StartPos;
		pParser->pTrackData[CurTrack].LastNoteTick=0;
		pParser->pTrackData[CurTrack].EndOfTrack=0;
	}

	pParser->EndOfFile=0;
}

//=============================================================================
//MidiParserShutdown:
//关闭Midi解析器
//-----------------------------------------------------------------------------
Midi_fn(MidiParserShutdown,void)
(
	MidiParser_p pParser
)
{
	if(pParser)
	{
		free(pParser->pTrackData);
		pParser->pTrackData=NULL;

		pParser->wType=0;
		pParser->wNbTracks=0;
		pParser->wTicksPerCrotchet=0;
		pParser->Velocity=0;
		pParser->TotalTicks=0;
		pParser->EndOfFile=0;
	}
}

#undef ErrC
#undef Tell
#undef Seek
#undef Read
