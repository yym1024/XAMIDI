
// 所需包含的头文件
#include "stdafx.h"
#include <CommCtrl.h>
#include <XAPOFX.h>
#include <XAMIDIHelper.hpp>
#include "main.h"
#include "player.h"
#define Midi_UserData_t struct MDIDPlayer
#include"midifile.h"
#include "resource.h"

// 所需链接的库文件
#pragma comment(lib, "XAPOFX")
#pragma comment(lib, "XAMIDI")

// 可自动卸载的已下载音色封装（方便 delete[] 时自动卸载数组中所有音色）
union AutoPXAMIDITimbreDownloaded
{
	IXAMIDITimbreDownloaded *pxamtd;
	AutoPXAMIDITimbreDownloaded(IXAMIDITimbreDownloaded *p = nullptr) throw() : pxamtd(p) {}
	~AutoPXAMIDITimbreDownloaded() throw() { if (pxamtd) pxamtd->UnloadTimbre(); }
	//IXAMIDITimbreDownloaded *&Reset() throw() { return pxamtd ? (pxamtd->UnloadTimbre(), pxamtd = nullptr) : pxamtd; }
};

// 切换 MIDI 音乐文件
static HRESULT ChangeMusic(LPCWSTR pPath);
// 切换 DLS 音色库文件
static HRESULT ChangeBanks(LPCWSTR pPath = nullptr);

// 所需要的变量
static IXAudio2 *pxa = nullptr;
static IXAMIDI *pxm = nullptr;
static IXAMIDITimbreBanks *pxamtb = nullptr;
static CXAMIDISynthVoice<> *pxamsv = nullptr;
static IXAudio2MasteringVoice *pxamv = nullptr;
static IXAudio2SubmixVoice *pxaMix = nullptr;
static IXAudio2SubmixVoice *pxaEff = nullptr;
static IXAudio2SourceVoice *pxaSrc = nullptr;
static IXAMIDISynthesizer *pSynth = nullptr;
static AutoPXAMIDITimbreDownloaded *pxamtds = nullptr;
static FXREVERB_PARAMETERS fxrp = { 0.8f, 0.12f, };
static FXECHO_PARAMETERS fxeh = { 1.0f, 0.5f, 500.f, };
static FXEQ_PARAMETERSEX fxeqps = {};
static float fxmats[4] = { 1.f, 0.f, 0.f, 1.f, };
static const float EqualizerCenters[] = { 20, 31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000, 20000, };

// 特效组合索引
enum XAFxIndex
{
	xaiFxReverb,
	xaiFxEcho,
};

// 预先缓存64毫秒的音符（建议>定时器最大周期，以保证音符播放的稳定）
CONST INT PrecacheTime = 0x40;

// 对 A5MIDI 解析库封装
static struct MDIDPlayer
{
	IStream * pStm;
	XAMidiPlayer *pPlayer;
	REFERENCE_TIME rtStartTime;
	MidiParser_t Parser;
} mp = {};

//=============================================================================
//A5ReadFile:
//A5MIDI文件读取函数，用于MidiParser_t
//-----------------------------------------------------------------------------
static int Midi_cb A5ReadFile
(
	void			*pBuf,		//缓冲区
	size_t			cb,			//要读取的字节数
	Midi_UserData_p pUserData	//用户数据
)
{
	ULONG rcb;
	pUserData->pStm->Read(pBuf, cb, &rcb);
	return cb == rcb;
}

//=============================================================================
//A5GetFilePtr:
//A5MIDI取文件指针函数，用于MidiParser_t
//-----------------------------------------------------------------------------
static int Midi_cb A5GetFilePtr
(
	fpos_t	*pPosition,	//取得位置
	Midi_UserData_p pUserData	//用户数据
)
{
	//return !fgetpos(pUserData->pFile, pPosition);
	ULARGE_INTEGER ret;
	pUserData->pStm->Seek(LARGE_INTEGER(), STREAM_SEEK_CUR, &ret);
	*pPosition = ret.QuadPart;
	return true;
}

//=============================================================================
//A5SetFilePtr:
//A5MIDI设置文件指针函数，用于MidiParser_t
//-----------------------------------------------------------------------------
static int Midi_cb A5SetFilePtr
(
	fpos_t	Position,	//新的位置
	Midi_UserData_p pUserData	//用户数据
)
{
	//return !fsetpos(pUserData->pFile, &Position);
	LARGE_INTEGER pos;
	pos.QuadPart = Position;
	return SUCCEEDED(pUserData->pStm->Seek(pos, STREAM_SEEK_SET, nullptr));
}

//=============================================================================
//A5SendMidiMsg:
//A5MIDI发送Midi指令函数，用于MidiParser_t
//-----------------------------------------------------------------------------
static void Midi_cb A5SendMidiMsg
(
	uint32_t TickCount,	//当前命令时钟
	uint16_t TrackNo,	//当前音轨号
	uint8_t	Command,	//命令
	uint8_t	Param1,		//参数1
	uint8_t	Param2,		//参数2
	Midi_UserData_p pUserData	//用户数据
)
{
	uint32_t Msg = 0;
	Msg = ((uint32_t)Command) & 0x000000FFUL;
	Msg |= (((uint32_t)Param1) << 8) & 0x0000FF00UL;
	Msg |= (((uint32_t)Param2) << 16) & 0x00FF0000UL;
	pSynth->OutShortMsg(
		// 输出的 midi 消息
		Msg,
		// 输出 midi 通道组（注意 midi1.0 所有音轨都是共享的16个通道，因为不能把每个音轨的数据分发到多个音轨上去）
		1,
		// 执行当前 midi 命令的绝对时间（每100纳秒1个单位，有可能是根据系统开机时间来算，也有可能是根据XA实际播放采样来计算）
		pUserData->rtStartTime + REFERENCE_TIME(TickCount) * pUserData->Parser.Velocity * 10
	);
	// 向主窗口输出消息
	ListOutMsg lom = { Command, Param1, Param2, TrackNo, };
	PostMessage(pUserData->pPlayer->hWnd, lom.WM_MidiMsg, TickCount, lom.Msg);
}

//=============================================================================
//A5OnError:
//A5MIDI错误信息提示函数，用于MidiParser_t
//-----------------------------------------------------------------------------
static void Midi_cb A5OnError(MidiErrCode_t Err, Midi_UserData_p pUserData)
{
	// 向主窗口输出消息
	PostMessage(pUserData->pPlayer->hWnd, ListOutMsg::WM_MidiErr, Err, NULL);
}

// 初始化 XA 特效链
static HRESULT InitEffectChain(XAMIDI_SYNTH_PARAMETERS &xasp)
{
	HRESULT hr;
	XAUDIO2_EFFECT_DESCRIPTOR saeds[3] = {};
	__try {
		// 设置 XAudio 缓冲区的房间混响特效
		XAUDIO2_EFFECT_CHAIN xaec = { 2, saeds, };
		for each(XAUDIO2_EFFECT_DESCRIPTOR &saed in saeds)
		{
			//saed.InitialState = TRUE;
			saed.OutputChannels = xasp.OutputChannels;
		}
		FAILED_LEAVE(CreateFX(__uuidof(FXReverb), &saeds[xaiFxReverb].pEffect));			// 创建混响特效
		FAILED_LEAVE(CreateFX(__uuidof(FXEcho), &saeds[xaiFxEcho].pEffect));			// 创建混响特效
		FAILED_LEAVE(pxaEff->SetEffectChain(&xaec));
		FAILED_LEAVE(pxaEff->SetEffectParameters(xaiFxReverb, &fxrp, sizeof(fxrp)));
		FAILED_LEAVE(pxaEff->SetEffectParameters(xaiFxEcho, &fxeh, sizeof(fxeh)));
		for (int i = 0; i < ARRAYSIZE(saeds); ++i)
		{
			XAUDIO2_EFFECT_DESCRIPTOR &saed = saeds[i];
			saed.InitialState = TRUE;
			if (saed.pEffect) saed.pEffect = (saed.pEffect->Release(), nullptr);
			FAILED_LEAVE(CreateFX(__uuidof(FXEQ), &saed.pEffect));			// 创建混响特效
		}
		for (int i = 0; i < ARRAYSIZE(fxeqps.all); ++i)
		{
			FXEQ_PARAMETER &fxeqp = fxeqps.all[i];
			fxeqp.FrequencyCenter = EqualizerCenters[i];
			fxeqp.Gain = FXEQ_DEFAULT_GAIN;
			fxeqp.Bandwidth = FXEQ_MAX_BANDWIDTH;
		}
		xaec.EffectCount = 3;
		FAILED_LEAVE(pxaMix->SetEffectChain(&xaec));
	}
	__finally {
		for each(XAUDIO2_EFFECT_DESCRIPTOR &saed in saeds)
			if (saed.pEffect) saed.pEffect->Release();
	}
	return hr;
}

// 子线程入口函数（在后台线程中解析和合成MIDI不会受到窗口消息阻塞等影响导致音乐中断）
STDMETHODIMP_(DWORD) XAMidiPlayer::ThreadMain(void)
{
	HRESULT hr;

	// 初始化COM
	FAILED_RETURN(CoInitialize(nullptr));
	__try {
		bool isexit = false;
		hr = setjmp(ljb);
		if (isexit) __leave;
		isexit = true;
		mp.pPlayer = this;

		// 合成器初始参数设置
		XAMIDI_SYNTH_PARAMETERS xasp;
		xasp.Flags = 0;
		xasp.Effects = XAMIDI_EFFECT_REVERB;				// 启用混响特效（此特效特别对钢琴声的表现很好，可以模拟类似琴弦共鸣的效果）
		xasp.OutputChannels = 2;							// 输出到 XAudio 的声道数（1为单声道，2为立体声，1一般用于3D音效生成动态立体声(适用于游戏)，2一般是用于 midi 自身的静态立体声(适用于播放器)）
		xasp.OutputSampleRate = XAMIDI_MAX_SAMPLE_RATE;		// 输出到 XAudio 的采样率（支持 11025Hz(低音质)、22050Hz(中音质)、44100Hz(高音质) 三种，默认为中等音质）
		xasp.VoiceCount = 256;								// 复音数（允许同时播放多少个 MIDI 音符，默认是32个，最大支持1000个）
		xasp.GroupCount = 1;								// 组数（每个组独立拥有16个 MIDI 通道，默认为2个，最大支持1000个）

		// 初始化（虽然初始化过程比 winmm 的 midiOutOpen 麻烦多了，但是功能要比它强大很多）
		FAILED_LEAVE(XAudio2Create(&pxa));	// 创建 XAudio 实例
		FAILED_LEAVE(XAMIDICreate(&pxm));	// 创建 XAMIDI 实例
		FAILED_LEAVE(pxa->CreateMasteringVoice(&pxamv, 2, xasp.OutputSampleRate));	// 创建主缓冲区
		pxamsv = new(std::nothrow) CXAMIDISynthVoice<>(pxa, pxm);		// 通过辅助类快速创建 XAudio 与 XAMIDI 交互的对象
		if (!pxamsv) FAILED_LEAVE(E_OUTOFMEMORY);

		// 设置混音缓冲区输出目标为主缓冲区
		XAUDIO2_SEND_DESCRIPTOR xasd[2] = { 0, pxamv, };
		XAUDIO2_VOICE_SENDS xavs = { 1, xasd, };
		FAILED_LEAVE(pxa->CreateSubmixVoice(&pxaMix, xasp.OutputChannels, xasp.OutputSampleRate, 0, 1, &xavs));	// 创建混音缓冲区
		xasd->pOutputVoice = pxaMix;		// 混响声输出到目标混音缓冲区中
		FAILED_LEAVE(pxa->CreateSubmixVoice(&pxaEff, xasp.OutputChannels, xasp.OutputSampleRate, 0, 0, &xavs));	// 创建混响声缓冲区
		FAILED_LEAVE(pxaMix->SetVolume(0.5f));

		// 设置 XAMIDI 输出 XAudio2 的目标为直达声和混响声两个缓冲区
		xasd[1].pOutputVoice = pxaEff;		// 直达声不变继续输出到pxaMix，而混响声输出到pxaEff。
		xavs.SendCount = ARRAYSIZE(xasd);
		FAILED_LEAVE(pxamsv->Initialize(&xasp, 4.f, &xavs));	// 初始化辅助交互对象
		pxaSrc = pxamsv->GetSourceVoice();						// 获取合成器对象（我们需要通过它来实现midi输出）
		pSynth = pxamsv->GetSynthesizer();						// 获取合成器对象（我们需要通过它来实现midi输出）
		FAILED_LEAVE(InitEffectChain(xasp));					// 初始化 XAudio2 的特效链

		// 创建可等待定时器（这种定时器可在SleepEx或WaitFor***Ex的睡眠时间内响应）
		hTimer = CreateWaitableTimer(nullptr, FALSE, nullptr);
		if (!hTimer) return HRESULT_FROM_WIN32(GetLastError());
		for (;;) SleepEx(INFINITE, TRUE);
	}
	__finally {
		// 释放对象操作（注意：delete 语句会自行检测是否空指针，而调用对象释放方法则必须自己手动检测）
		if (hTimer) CloseHandle(hTimer);	// 关闭定时器
		if (pSynth && pSynth->GetEnabled()) pSynth->SetEnabled(FALSE);		// 停止 MIDI 合成
		if (mp.pStm) mp.pStm->Release();	// 释放文件流
		MidiParserShutdown(&mp.Parser);		// 停止 MIDI 文件解析
		delete[] pxamtds;					// 卸载并释放音色（内部会自动循环卸载所有已下载音色）
		delete pxamsv;						// 释放辅助对象（里面会自动释放 XAMIDI 合成器 和 XAudio2 源缓冲区）
		if (pxamtb) pxamtb->DestroyBanks();	// 销毁 XAMIDI 音色库
		if (pxaEff) pxaEff->DestroyVoice();	// 销毁特效子缓冲区（混响、反射、回声）
		if (pxaMix) pxaMix->DestroyVoice();	// 销毁混音子缓冲区（按干湿比混合直达声和混响声）
		if (pxamv) pxamv->DestroyVoice();	// 销毁 XAudio2 主缓冲区
		if (pxm) pxm->Release();			// 释放 XAMIDI 引擎
		if (pxa) pxa->Release();			// 释放 XAudio2 引擎
		CoUninitialize();	// 释放 COM
	}
	return hr;			// 返回错误码
}

// 在子线程中响应控件操作状态
STDMETHODIMP_(void) XAMidiPlayer::OnControlState(int ID, int Value) const
{
	switch (ID)
	{
	case IDC_ChkReverb:
		// 启用或禁用波浪混响特效（此特效特别对钢琴声的表现很好，可以模拟类似琴弦共鸣的效果）
		pSynth->SetEffects((Value & BST_CHECKED) ? XAMIDI_EFFECT_REVERB : 0);
		break;
	case IDC_ChkDiffuse:
		// 启用或禁用房间混响特效（可以模拟房间墙壁反射声波的效果）
		switch (Value)
		{
		case BST_UNCHECKED:
			pxaEff->DisableEffect(xaiFxReverb);
			break;
		case BST_CHECKED:
			pxaEff->EnableEffect(xaiFxReverb);
			break;
		}
		break;
	case IDC_ChkEcho:
		// 启用或禁用回声特效
		switch (Value)
		{
		case BST_UNCHECKED:
			pxaEff->DisableEffect(xaiFxEcho);
			break;
		case BST_CHECKED:
			pxaEff->EnableEffect(xaiFxEcho);
			break;
		}
		break;
	case IDC_SlrPitch:
	case IDC_EdtPitch:
	case IDC_SpnPitch:
	case IDC_RstPitch:
		// 通过调整 XAudio2 的播放速度来实现对 XAMIDI 的全局变调
		pxaSrc->SetFrequencyRatio(exp2f(float(Value) * 1e-3f));
		break;
	case IDC_SlrGain:
	case IDC_EdtGain:
	case IDC_SpnGain:
	case IDC_RstGain:
		// 调整 XAudio2 的比例音量
		pxaSrc->SetVolume(float(Value) * 1e-3f);
		break;
	case IDC_SlrPan:
	case IDC_EdtPan:
	case IDC_SpnPan:
	case IDC_RstPan:
	{	// 调整左右声道平衡
		float vols[2] = { 1.f, 1.f, };
		if (Value > 0)
			vols[0] = float(1000 - Value) * 1e-3f;
		else if (Value < 0)
			vols[1] = float(1000 + Value) * 1e-3f;
		pxaSrc->SetChannelVolumes(ARRAYSIZE(vols), vols);
	}
		break;
	case IDC_SlrMix:
	case IDC_EdtMix:
	case IDC_SpnMix:
	case IDC_RstMix:
	{	// 调整混响干湿比
		float mat[4] = { float(Value) * 2e-3f, };
		pxaEff->SetVolume(*mat);
		mat[3] = mat[0] = 2.f - *mat;
		pxaSrc->SetOutputMatrix(pxaMix, 2, 2, mat);
	}
		break;
	case IDC_SlrDiffuse:
	case IDC_EdtDiffuse:
	case IDC_SpnDiffuse:
	case IDC_RstDiffuse:
		// 调整房间墙壁的粗糙程度（漫反射与镜面反射的比例）
		fxrp.Diffusion = float(Value) * 1e-3f;
		pxaEff->SetEffectParameters(xaiFxReverb, &fxrp, sizeof(fxrp));
		break;
	case IDC_SlrRoom:
	case IDC_EdtRoom:
	case IDC_SpnRoom:
	case IDC_RstRoom:
		// 调整房间大小（影响反射的延迟时间和音量大小）
		fxrp.RoomSize = float(Value) * 1e-3f;
		if (fxrp.RoomSize < FXREVERB_MIN_ROOMSIZE)
			fxrp.RoomSize = FXREVERB_MIN_ROOMSIZE;
		pxaEff->SetEffectParameters(xaiFxReverb, &fxrp, sizeof(fxrp));
		break;
	case IDC_SlrFedback:
	case IDC_EdtFedback:
	case IDC_SpnFedback:
	case IDC_RstFedback:
		// 调整回声的反馈大小
		fxeh.Feedback = float(Value) * 1e-3f;
		pxaEff->SetEffectParameters(xaiFxEcho, &fxeh, sizeof(fxeh));
		break;
	case IDC_SlrDelay:
	case IDC_EdtDelay:
	case IDC_SpnDelay:
	case IDC_RstDelay:
		// 调整回声的延迟时间
		fxeh.Delay = float(Value);
		pxaEff->SetEffectParameters(xaiFxEcho, &fxeh, sizeof(fxeh));
		break;
	case IDC_CmbEqual:
	case IDC_RstEqual:
		// 切换预设的均衡器参数
		for (int i = 0; i < ARRAYSIZE(fxeqps.all); ++i)
		{
			FXEQ_PARAMETER &fxeqp = fxeqps.all[i];
			fxeqp.Gain = XAudio2DecibelsToAmplitudeRatio(EqualizerPresets[Value][i]);
		}
		pxaMix->SetEffectParameters(0, &fxeqps.raw[0], sizeof(*fxeqps.raw));
		pxaMix->SetEffectParameters(1, &fxeqps.raw[1], sizeof(*fxeqps.raw));
		pxaMix->SetEffectParameters(2, &fxeqps.raw[2], sizeof(*fxeqps.raw));
		break;
	case IDC_SlrEq20:
	case IDC_EdtEq20:
	case IDC_SpnEq20:
		// 调整 20Hz 频段的分贝音量
		fxeqps.all[0].Gain = XAudio2DecibelsToAmplitudeRatio(float(Value));
		pxaMix->SetEffectParameters(0, &fxeqps.raw[0], sizeof(*fxeqps.raw));
		break;
	case IDC_SlrEq31:
	case IDC_EdtEq31:
	case IDC_SpnEq31:
		// 调整 31Hz 频段的分贝音量
		fxeqps.all[1].Gain = XAudio2DecibelsToAmplitudeRatio(float(Value));
		pxaMix->SetEffectParameters(0, &fxeqps.raw[0], sizeof(*fxeqps.raw));
		break;
	case IDC_SlrEq62:
	case IDC_EdtEq62:
	case IDC_SpnEq62:
		// 调整 62Hz 频段的分贝音量
		fxeqps.all[2].Gain = XAudio2DecibelsToAmplitudeRatio(float(Value));
		pxaMix->SetEffectParameters(0, &fxeqps.raw[0], sizeof(*fxeqps.raw));
		break;
	case IDC_SlrEq125:
	case IDC_EdtEq125:
	case IDC_SpnEq125:
		// 调整 125Hz 频段的分贝音量
		fxeqps.all[3].Gain = XAudio2DecibelsToAmplitudeRatio(float(Value));
		pxaMix->SetEffectParameters(0, &fxeqps.raw[0], sizeof(*fxeqps.raw));
		break;
	case IDC_SlrEq250:
	case IDC_EdtEq250:
	case IDC_SpnEq250:
		// 调整 250Hz 频段的分贝音量
		fxeqps.all[4].Gain = XAudio2DecibelsToAmplitudeRatio(float(Value));
		pxaMix->SetEffectParameters(1, &fxeqps.raw[1], sizeof(*fxeqps.raw));
		break;
	case IDC_SlrEq500:
	case IDC_EdtEq500:
	case IDC_SpnEq500:
		// 调整 500Hz 频段的分贝音量
		fxeqps.all[5].Gain = XAudio2DecibelsToAmplitudeRatio(float(Value));
		pxaMix->SetEffectParameters(1, &fxeqps.raw[1], sizeof(*fxeqps.raw));
		break;
	case IDC_SlrEq1k:
	case IDC_EdtEq1k:
	case IDC_SpnEq1k:
		// 调整 1000Hz 频段的分贝音量
		fxeqps.all[6].Gain = XAudio2DecibelsToAmplitudeRatio(float(Value));
		pxaMix->SetEffectParameters(1, &fxeqps.raw[1], sizeof(*fxeqps.raw));
		break;
	case IDC_SlrEq2k:
	case IDC_EdtEq2k:
	case IDC_SpnEq2k:
		// 调整 2000Hz 频段的分贝音量
		fxeqps.all[7].Gain = XAudio2DecibelsToAmplitudeRatio(float(Value));
		pxaMix->SetEffectParameters(1, &fxeqps.raw[1], sizeof(*fxeqps.raw));
		break;
	case IDC_SlrEq4k:
	case IDC_EdtEq4k:
	case IDC_SpnEq4k:
		// 调整 4000Hz 频段的分贝音量
		fxeqps.all[8].Gain = XAudio2DecibelsToAmplitudeRatio(float(Value));
		pxaMix->SetEffectParameters(2, &fxeqps.raw[2], sizeof(*fxeqps.raw));
		break;
	case IDC_SlrEq8k:
	case IDC_EdtEq8k:
	case IDC_SpnEq8k:
		// 调整 8000Hz 频段的分贝音量
		fxeqps.all[9].Gain = XAudio2DecibelsToAmplitudeRatio(float(Value));
		pxaMix->SetEffectParameters(2, &fxeqps.raw[2], sizeof(*fxeqps.raw));
		break;
	case IDC_SlrEq16k:
	case IDC_EdtEq16k:
	case IDC_SpnEq16k:
		// 调整 16000Hz 频段的分贝音量
		fxeqps.all[10].Gain = XAudio2DecibelsToAmplitudeRatio(float(Value));
		pxaMix->SetEffectParameters(2, &fxeqps.raw[2], sizeof(*fxeqps.raw));
		break;
	case IDC_SlrEq20k:
	case IDC_EdtEq20k:
	case IDC_SpnEq20k:
		// 调整 20000Hz 频段的分贝音量
		fxeqps.all[11].Gain = XAudio2DecibelsToAmplitudeRatio(float(Value));
		pxaMix->SetEffectParameters(2, &fxeqps.raw[2], sizeof(*fxeqps.raw));
		break;
	case IDC_CmbMatrix:
	case IDC_RstMatrix:
		// 切换预设的音频矩阵参数
		for (int i = 0; i < ARRAYSIZE(fxmats); ++i)
			fxmats[i] = MatrixerPresets[Value][i] * 1e-3f;
		pxaMix->SetOutputMatrix(nullptr, 2, 2, fxmats);
		break;
	case IDC_EdtMat11:
	case IDC_SpnMat11:
		// 调整从源左声道输入并输出到目标左声道的比例音量
		fxmats[0] = float(Value) * 1e-3f;
		pxaMix->SetOutputMatrix(nullptr, 2, 2, fxmats);
		break;
	case IDC_EdtMat12:
	case IDC_SpnMat12:
		// 调整从源右声道输入并输出到目标左声道的比例音量
		fxmats[1] = float(Value) * 1e-3f;
		pxaMix->SetOutputMatrix(nullptr, 2, 2, fxmats);
		break;
	case IDC_EdtMat21:
	case IDC_SpnMat21:
		// 调整从源左声道输入并输出到目标右声道的比例音量
		fxmats[2] = float(Value) * 1e-3f;
		pxaMix->SetOutputMatrix(nullptr, 2, 2, fxmats);
		break;
	case IDC_EdtMat22:
	case IDC_SpnMat22:
		// 调整从源右声道输入并输出到目标右声道的比例音量
		fxmats[3] = float(Value) * 1e-3f;
		pxaMix->SetOutputMatrix(nullptr, 2, 2, fxmats);
		break;
	case IDC_BtnPlay:
		// 播放/停止按钮操作
		switch (Value)
		{
		case BST_UNCHECKED:
			// 停止
			CancelWaitableTimer(hTimer);
			MidiParserShutdown(&mp.Parser);	// 停止 MIDI 文件解析
			for (int i = 0x0; i < 0x10; ++i)
			{	// 先停止所有通道声音
				pSynth->OutShortMsg(0x00007BB0 | i);	// 关闭所有音符
				pSynth->OutShortMsg(0x000078B0 | i);	// 停止所有声音
				pSynth->OutShortMsg(0x000040B0 | i);	// 抬起延音踏板
			}
			if (mp.pStm) mp.pStm = (mp.pStm->Release(), nullptr);
			ChangeBanks();
			SendMessage(GetDlgItem(hWnd, IDC_EdtMidi), EM_SETREADONLY, FALSE, 0);
			SendMessage(GetDlgItem(hWnd, IDC_EdtBank), EM_SETREADONLY, FALSE, 0);
			EnableWindow(GetDlgItem(hWnd, IDC_BtnMidi), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_BtnBank), TRUE);
			SleepEx(0x100, FALSE);
			pxamsv->Stop();
			break;
		case BST_CHECKED:
		{	// 播放
			const INT64 p = -0x55555;
			WCHAR Path[MAX_PATH];
			HWND hMidi = GetDlgItem(hWnd, IDC_EdtMidi);
			HWND hBank = GetDlgItem(hWnd, IDC_EdtBank);
			SendMessage(hMidi, WM_GETTEXT, ARRAYSIZE(Path), LPARAM(Path));
			ChangeMusic(Path);	// 切换音乐文件
			SendMessage(hBank, WM_GETTEXT, ARRAYSIZE(Path), LPARAM(Path));
			ChangeBanks(Path);	// 音色库文件
			SetWaitableTimer(hTimer, PLARGE_INTEGER(&p), p / -10000, any_cast<PTIMERAPCROUTINE>(&XAMidiPlayer::UpdateTimer), LPVOID(this), FALSE);	// 启动定时器
			pxamsv->Start();	// 开始播放
			pSynth->GetReferenceClock()->GetTime(&mp.rtStartTime);	// 获取初始时间戳
			PostMessage(GetDlgItem(hWnd, IDC_LstOutMsg), LVM_DELETEALLITEMS, NULL, NULL);
			for (int i = 0x0; i < 0x10; ++i)
			{	// 再重置所有通道状态
				pSynth->OutShortMsg(0x000000B0 | i);	// 音色库 MSB 重置
				pSynth->OutShortMsg(0x000020B0 | i);	// 音色库 LSB 重置
				pSynth->OutShortMsg(0x000000C0 | i);	// 音色重置（以上两句需在此句执行后才生效）
				//pSynth->OutShortMsg(0x000001B0 | i);	// 颤音 MSB 重置
				//pSynth->OutShortMsg(0x000021B0 | i);	// 颤音 LSB 重置
				//pSynth->OutShortMsg(0x006407B0 | i);	// 音量 MSB 重置
				//pSynth->OutShortMsg(0x000027B0 | i);	// 音量 LSB 重置
				//pSynth->OutShortMsg(0x004008B0 | i);	// 平衡 MSB 重置
				//pSynth->OutShortMsg(0x000028B0 | i);	// 平衡 LSB 重置
				pSynth->OutShortMsg(0x004000E0 | i);	// 滑音重置
				pSynth->OutShortMsg(0x000079B0 | i);	// 所有控制器重置
			}
			MidiParserUpdate(&mp.Parser, PrecacheTime, 0);			// 预先解析一部分MIDI数据
			// 屏蔽不可在播放中操作的控件：
			EnableWindow(GetDlgItem(hWnd, IDC_BtnMidi), FALSE);		// 禁用选音乐文件按钮
			EnableWindow(GetDlgItem(hWnd, IDC_BtnBank), FALSE);		// 禁用选音色库文件按钮
			SendMessage(hMidi, EM_SETREADONLY, TRUE, 0);			// 音乐文件编辑框设为只读
			SendMessage(hBank, EM_SETREADONLY, TRUE, 0);			// 音色库文件编辑框设为只读
		}
			break;
		}
		break;
	}
	__nop();
}

STDMETHODIMP_(void) XAMidiPlayer::UpdateTimer(DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
	REFERENCE_TIME rtCurTime, rtAllTime;
	pSynth->GetReferenceClock()->GetTime(&rtCurTime);	// 获取当前时间戳
	rtCurTime -= mp.rtStartTime;		// 减去初始时间戳（得到已播放的时间）
	rtAllTime = REFERENCE_TIME(mp.Parser.TotalTicks) * REFERENCE_TIME(mp.Parser.Velocity) * 10ll;	// 计算音乐的总时长
	MidiParserUpdate(&mp.Parser, MidiTime_t(rtCurTime) * 0.0001 + PrecacheTime, 0);					// 更新 MIDI 数据解析
	if (mp.Parser.EndOfFile || rtCurTime >= rtAllTime)
	{	// 如果 MIDI 文件解析结束或超过了总时长则重置解析器为初始状态以实现循环播放
		MidiParserReset(&mp.Parser);
		mp.rtStartTime += rtAllTime;
	}
#ifdef _WIN64
	// 64位环境下消息附加参数是两个8字节的，因此正好传递完整的64位时间戳。
	PostMessage(hWnd, ListOutMsg::WM_MidiUpdate, WPARAM(rtCurTime), LPARAM(rtAllTime));
#else // _WIN32
	// 32位环境下消息附加参数是两个4字节的，因此只能牺牲一定精度后再传递。
	rtCurTime /= 10000, rtAllTime /= 10000;
	PostMessage(hWnd, ListOutMsg::WM_MidiUpdate, WPARAM(rtCurTime), LPARAM(rtAllTime));
#endif
}

static HRESULT ChangeMusic(LPCWSTR pPath)
{
	HRESULT hr;
	if (mp.pStm) mp.pStm = (mp.pStm->Release(), nullptr);
	MidiParserShutdown(&mp.Parser);	// 停止 MIDI 文件解析
	FAILED_RETURN(SHCreateStreamOnFileW(pPath, STGM_READ, &mp.pStm));
	mp.Parser.pfnReadFile = A5ReadFile;
	mp.Parser.pfnGetFilePtr = A5GetFilePtr;
	mp.Parser.pfnSetFilePtr = A5SetFilePtr;
	mp.Parser.pfnSendMidiMsg = A5SendMidiMsg;
	mp.Parser.pfnOnError = A5OnError;
	mp.Parser.pUserData = &mp;
	if (!MidiParserStart(&mp.Parser) && (hr = E_FAIL))
		mp.pStm = (mp.pStm->Release(), nullptr);
	return hr;
}

static HRESULT ChangeBanks(LPCWSTR pPath)
{
	HRESULT hr = S_OK;
	IXAMIDITimbreBanks *pxamtb = nullptr;
	AutoPXAMIDITimbreDownloaded *pxamtds = nullptr;

	__try {
		if (pPath)
		{
			FAILED_LEAVE(pxm->LoadTimbreBanks(&pxamtb, *pPath ? XAMIDI_FILEPATH : XAMIDI_DEFAULT, pPath));

			// 获取音色并下载音色到合成器中
			UINT32 count = pxamtb->GetTimbreCount();
			pxamtds = new(std::nothrow)AutoPXAMIDITimbreDownloaded[count];
			if (!pxamtds) return E_OUTOFMEMORY;
			for (UINT32 i = 0; i < count; ++i)
			{
				XAMIDI_TIMBRE_DETAILS td;
				td.Name = nullptr;
				pxamtb->GetTimbreDetails(i, &td);
				FAILED_LEAVE(pSynth->DownloadTimbre(&pxamtds[i].pxamtd, td.Patch, td.hTimbre));
			}
		}
		std::swap(pxamtds, ::pxamtds);
		std::swap(pxamtb, ::pxamtb);
	}
	__finally {
		delete[] pxamtds;	// 卸载并释放音色
		if (pxamtb) pxamtb->DestroyBanks();	// 销毁 XAMIDI 音色库
	}
	return hr;
}
