
// 所需包含的头文件
#define _USE_MATH_DEFINES
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <X3DAudio.h>
#include <XAudio2fx.h>
#include <XAMIDIHelper.hpp>
#include <intrin.h>
#include <stdio.h>
#include "resource.h"

// 所需链接的库文件
#pragma comment(lib, "ComCtl32")
#pragma comment(lib, "Shlwapi")
#pragma comment(lib, "WinMM")
#pragma comment(lib, "X3DAudio")
#pragma comment(lib, "XAMIDI")

// 系统控件主题支持
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"" \
"type='win32' " \
"name='Microsoft.Windows.Common-Controls' " \
"version='6.0.0.0' processorArchitecture='x86' " \
"publicKeyToken='6595b64144ccf1df' " \
"language='*'" \
"\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"" \
"type='win32' " \
"name='Microsoft.Windows.Common-Controls' " \
"version='6.0.0.0' processorArchitecture='amd64' " \
"publicKeyToken='6595b64144ccf1df' " \
"language='*'" \
"\"")
#elif defined _M_ARM
#pragma comment(linker,"/manifestdependency:\"" \
"type='win32' " \
"name='Microsoft.Windows.Common-Controls' " \
"version='6.0.0.0' processorArchitecture='arm' " \
"publicKeyToken='6595b64144ccf1df' " \
"language='*'" \
"\"")
#else
#pragma comment(linker,"/manifestdependency:\""
"type='win32' "
"name='Microsoft.Windows.Common-Controls' "
"version='6.0.0.0' processorArchitecture='*' "
"publicKeyToken='6595b64144ccf1df' "
"language='*'"
"\"")
#endif

#pragma comment(linker, "/ENTRY:mainCRTStartup")

// COM 错误处理
#define FAILED_LEAVE(_HR_) if FAILED(hr = (_HR_)) __leave

EXTERN_C HINSTANCE__ __ImageBase;
// 主窗口的消息处理
INT_PTR CALLBACK MainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT UpdateXASound3D();

static const struct MainDT
{
	DLGTEMPLATE dt;
	WCHAR menu[2], cls, title;
} MDT = { WS_SYSMENU | WS_MINIMIZEBOX | WS_CAPTION, WS_EX_APPWINDOW | WS_EX_COMPOSITED, 0, MINSHORT, MINSHORT, 200, 128, MAXWORD, IDR_MainMenu, L'\0', L'\0', };

// 预设的环境参数数据
static const XAUDIO2FX_REVERB_I3DL2_PARAMETERS I3DL2Presets[] = {
	XAUDIO2FX_I3DL2_PRESET_DEFAULT,
	XAUDIO2FX_I3DL2_PRESET_GENERIC,
	XAUDIO2FX_I3DL2_PRESET_PADDEDCELL,
	XAUDIO2FX_I3DL2_PRESET_ROOM,
	XAUDIO2FX_I3DL2_PRESET_BATHROOM,
	XAUDIO2FX_I3DL2_PRESET_LIVINGROOM,
	XAUDIO2FX_I3DL2_PRESET_STONEROOM,
	XAUDIO2FX_I3DL2_PRESET_AUDITORIUM,
	XAUDIO2FX_I3DL2_PRESET_CONCERTHALL,
	XAUDIO2FX_I3DL2_PRESET_CAVE,
	XAUDIO2FX_I3DL2_PRESET_ARENA,
	XAUDIO2FX_I3DL2_PRESET_HANGAR,
	XAUDIO2FX_I3DL2_PRESET_CARPETEDHALLWAY,
	XAUDIO2FX_I3DL2_PRESET_HALLWAY,
	XAUDIO2FX_I3DL2_PRESET_STONECORRIDOR,
	XAUDIO2FX_I3DL2_PRESET_ALLEY,
	XAUDIO2FX_I3DL2_PRESET_FOREST,
	XAUDIO2FX_I3DL2_PRESET_CITY,
	XAUDIO2FX_I3DL2_PRESET_MOUNTAINS,
	XAUDIO2FX_I3DL2_PRESET_QUARRY,
	XAUDIO2FX_I3DL2_PRESET_PLAIN,
	XAUDIO2FX_I3DL2_PRESET_PARKINGLOT,
	XAUDIO2FX_I3DL2_PRESET_SEWERPIPE,
	XAUDIO2FX_I3DL2_PRESET_UNDERWATER,
	XAUDIO2FX_I3DL2_PRESET_SMALLROOM,
	XAUDIO2FX_I3DL2_PRESET_MEDIUMROOM,
	XAUDIO2FX_I3DL2_PRESET_LARGEROOM,
	XAUDIO2FX_I3DL2_PRESET_MEDIUMHALL,
	XAUDIO2FX_I3DL2_PRESET_LARGEHALL,
	XAUDIO2FX_I3DL2_PRESET_PLATE,
};

// X3DAudio 控制参数
static const float RadiusX = 12.f, RadiusZ = 8.f;
static X3DAUDIO_HANDLE hX3D;
static float mat[2];
static const X3DAUDIO_CONE listCone = { X3DAUDIO_PI*5.0f / 6.0f, X3DAUDIO_PI*11.0f / 6.0f, 1.0f, 0.75f, 0.0f, 0.25f, 0.708f, 1.0f };
static const X3DAUDIO_DISTANCE_CURVE_POINT emitLfeCurvePts[] = { 0.0f, 1.0f, 0.25f, 0.0f, 1.0f, 0.0f };
static const X3DAUDIO_DISTANCE_CURVE       emitLfeCurve = { LPX3DAUDIO_DISTANCE_CURVE_POINT(emitLfeCurvePts), 3 };
static const X3DAUDIO_DISTANCE_CURVE_POINT emitRevCurvePts[] = { 0.0f, 0.5f, 0.75f, 1.0f, 1.0f, 0.0f };
static const X3DAUDIO_DISTANCE_CURVE       emitRevCurve = { LPX3DAUDIO_DISTANCE_CURVE_POINT(emitRevCurvePts), 3 };
static X3DAUDIO_LISTENER list = { { 0, 0, 1, }, { 0, 1, 0, }, {}, {}, LPX3DAUDIO_CONE(&listCone), };
static X3DAUDIO_EMITTER emit = { nullptr, { 0, 0, 1, }, { 0, 1, 0, }, {}, {}, 2.0f, X3DAUDIO_PI / 4.0f, 1, 1.0f, nullptr, LPX3DAUDIO_DISTANCE_CURVE(&X3DAudioDefault_LinearCurve), LPX3DAUDIO_DISTANCE_CURVE(&emitLfeCurve), nullptr, nullptr, LPX3DAUDIO_DISTANCE_CURVE(&emitRevCurve), 14.0f, 1.0f, };
static X3DAUDIO_DSP_SETTINGS dsps = { mat, nullptr, 1, 2, };
static const UINT32 Flags = X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER | X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_LPF_REVERB | X3DAUDIO_CALCULATE_REVERB;

// 所需要的变量
static IXAudio2 *pxa = nullptr;
static IXAMIDI *pxm = nullptr;
static IXAMIDITimbreBanks *pxamtb = nullptr;
static CXAMIDISynthVoice<> *pxamsv = nullptr;
static IXAudio2MasteringVoice *pxamv = nullptr;
static IXAudio2SubmixVoice *pxaEff = nullptr;
static IXAudio2SourceVoice *pxaSrc = nullptr;
static IXAMIDISynthesizer *pSynth = nullptr;
static IXAMIDITimbreDownloaded *pxamtds[2] = {};
static IReferenceClock *pRefClock = nullptr;
static REFERENCE_TIME irt, ort;

int main()
{
	HRESULT hr;

	// 初始化COM
	if FAILED(hr = CoInitialize(nullptr)) return hr;
	__try {
		X3DAudioInitialize(SPEAKER_STEREO, X3DAUDIO_SPEED_OF_SOUND, hX3D);
		X3DAudioCalculate(hX3D, &list, &emit, Flags, &dsps);
		// 合成器初始参数设置
		XAMIDI_SYNTH_PARAMETERS xasp;
		xasp.Flags = 0;
		xasp.Effects = XAMIDI_EFFECT_NONE;					// 不用任何特效
		xasp.OutputChannels = 1;							// 输出到 XAudio 的声道数（1为单声道，2为立体声，1一般用于3D音效生成动态立体声(适用于游戏)，2一般是用于 midi 自身的静态立体声(适用于播放器)）
		xasp.OutputSampleRate = XAMIDI_MAX_SAMPLE_RATE;		// 输出到 XAudio 的采样率（支持 11025Hz(低音质)、22050Hz(中音质)、44100Hz(高音质) 三种，默认为中等音质）
		xasp.VoiceCount = 4;								// 复音数（允许同时播放多少个 MIDI 音符，默认是32个，最大支持1000个）
		xasp.GroupCount = 1;								// 组数（每个组独立拥有16个 MIDI 通道，默认为2个，最大支持1000个）

		// 初始化（虽然初始化过程比 winmm 的 midiOutOpen 麻烦多了，但是功能要比它强大很多）
		FAILED_LEAVE(XAudio2Create(&pxa));	// 创建 XAudio 实例
		FAILED_LEAVE(XAMIDICreate(&pxm));	// 创建 XAMIDI 实例
		FAILED_LEAVE(pxa->CreateMasteringVoice(&pxamv, 2, xasp.OutputSampleRate));	// 创建主缓冲区
		hr = pxm->LoadTimbreBanks(&pxamtb, XAMIDI_FILEPATH, L"gm.dls");				// 加载自己的音色库文件
		if FAILED(hr) FAILED_LEAVE(pxm->LoadTimbreBanks(&pxamtb, XAMIDI_DEFAULT));	// 失败则加载系统音色库
		pxamsv = new(std::nothrow) CXAMIDISynthVoice<>(pxa, pxm);		// 通过辅助类快速创建 XAudio 与 XAMIDI 交互的对象
		if (!pxamsv) FAILED_LEAVE(E_OUTOFMEMORY);

		// 设置混音缓冲区输出目标为主缓冲区
		XAUDIO2_SEND_DESCRIPTOR xasd[2] = { 0, pxamv, };
		XAUDIO2_VOICE_SENDS xavs = { 1, xasd, };
		FAILED_LEAVE(pxa->CreateSubmixVoice(&pxaEff, xasp.OutputChannels, xasp.OutputSampleRate, 0, 0, &xavs));	// 创建混响声缓冲区

		// 设置 XAudio2 缓冲区的交互式3D混响特效
		XAUDIO2_EFFECT_DESCRIPTOR saed = {};
		const XAUDIO2_EFFECT_CHAIN xaec = { 1, &saed, };
		saed.InitialState = TRUE;
		saed.OutputChannels = xasp.OutputChannels;
		FAILED_LEAVE(XAudio2CreateReverb(&saed.pEffect));			// 创建混响特效
		hr = pxaEff->SetEffectChain(&xaec);
		saed.pEffect->Release();
		if FAILED(hr) __leave;

		// 设置 XAMIDI 输出 XAudio2 的目标为直达声和混响声两个缓冲区
		xasd[0].Flags = XAUDIO2_SEND_USEFILTER;
		xasd[1].Flags = XAUDIO2_SEND_USEFILTER;
		xasd[1].pOutputVoice = pxaEff;		// 直达声不变继续输出到pxamv，而混响声输出到pxaEff。
		xavs.SendCount = ARRAYSIZE(xasd);
		FAILED_LEAVE(pxamsv->Initialize(&xasp, XAUDIO2_DEFAULT_FREQ_RATIO, &xavs));		// 初始化辅助交互对象
		pxaSrc = pxamsv->GetSourceVoice();					// 获取合成器对象（我们需要通过它来实现midi输出）
		pSynth = pxamsv->GetSynthesizer();					// 获取合成器对象（我们需要通过它来实现midi输出）
		FAILED_LEAVE(pxaSrc->SetVolume(1.2f));				// 设置 XAMIDI 合成器的音量大小
		//FAILED_LEAVE(pxaRev->SetVolume(2.0f));			// 设置 XAMIDI 合成器的音量大小

		// 获取音色并下载音色到合成器中
		if (HXAMIDITimbre hTimbre = pxamtb->GetPatchTimbre(XMIDIMakePatch(0, 0, XAMIDI_PATCH_PRESET_Helicopter)))
			FAILED_LEAVE(pSynth->DownloadTimbre(pxamtds + 0, 0, hTimbre));
		if (HXAMIDITimbre hTimbre = pxamtb->GetPatchTimbre(XMIDIMakePatch(0, 0, XAMIDI_PATCH_PRESET_Gunshot)))
			FAILED_LEAVE(pSynth->DownloadTimbre(pxamtds + 0, 1, hTimbre));

		pRefClock = pSynth->GetReferenceClock();	// 获取合成器的时钟对象
		FAILED_LEAVE(pSynth->SetEnabled(TRUE));			// 开始 MIDI 合成
		FAILED_LEAVE(pRefClock->GetTime(&irt));			// 获取当前时间
		ort = irt;
		FAILED_LEAVE(UpdateXASound3D());
		FAILED_LEAVE(pSynth->OutShortMsg(0x0001C1));

		hr = DialogBoxIndirect(&__ImageBase, &MDT.dt, HWND_DESKTOP, MainProc);
		if SUCCEEDED(hr) Sleep(0x200);
	}
	__finally {
		// 释放对象操作（注意：delete 语句会自行检测是否空指针，而调用对象释放方法则必须自己手动检测）
		if (pSynth && pSynth->GetEnabled()) pSynth->SetEnabled(FALSE);	// 停止 MIDI 合成
		for each(IXAMIDITimbreDownloaded *pxamtd in pxamtds)
			if (pxamtd) pxamtd->UnloadTimbre();	// 卸载并释放音色
		delete pxamsv;		// 释放辅助对象（里面会自动释放 XAMIDI 合成器 和 XAudio2 源缓冲区）
		if (pxamtb) pxamtb->DestroyBanks();	// 销毁 XAMIDI 音色库
		if (pxaEff) pxaEff->DestroyVoice();	// 销毁特效子缓冲区（混响）
		if (pxamv) pxamv->DestroyVoice();	// 释放 XAudio2 主缓冲区
		if (pxm) pxm->Release();			// 释放 XAMIDI 引擎
		if (pxa) pxa->Release();			// 释放 XAudio2 引擎
		CoUninitialize();	// 释放 COM
	}
	return hr;			// 返回错误码
}

INT_PTR CALLBACK MainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HRESULT hr;
	static const int IDT_Update = 100;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		DefWindowProc(hWnd, WM_SETTEXT, NULL, LPARAM(TEXT("XAMIDI 3D Audio")));
		//CheckMenuRadioItem(GetMenu(hWnd), IDM_EnvNone, IDM_EnvPlate, IDM_EnvGeneric, MF_BYCOMMAND);
		SetTimer(hWnd, IDT_Update, USER_TIMER_MINIMUM, nullptr);
		(hr = pSynth->OutShortMsg(0x7F3C90));
		break;
	case WM_CLOSE:
		(hr = pSynth->OutShortMsg(0x003C80));
		return EndDialog(hWnd, EXIT_SUCCESS);
	case WM_COMMAND:
		if (LOWORD(wParam) >= IDM_EnvNone && LOWORD(wParam) <= IDM_EnvPlate)
		{
			XAUDIO2FX_REVERB_PARAMETERS fxrp;
			//FAILED_LEAVE(pxaRev->GetEffectParameters(0, &fxrp, sizeof(fxrp)));
			ReverbConvertI3DL2ToNative(I3DL2Presets + LOWORD(wParam) - IDM_EnvNone, &fxrp);
			hr = pxaEff->SetEffectParameters(0, &fxrp, sizeof(fxrp));
			CheckMenuRadioItem(GetMenu(hWnd), IDM_EnvNone, IDM_EnvPlate, LOWORD(wParam), MF_BYCOMMAND);
			return SUCCEEDED(hr);
		}
		return FALSE;
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
		return SUCCEEDED(hr = pSynth->OutShortMsg(0x7F3091));
	case WM_LBUTTONUP:
		//return SUCCEEDED(hr = pSynth->OutShortMsg(0x003081));
	case WM_TIMER:
		if (IDT_Update != wParam) return FALSE;
		return SUCCEEDED(hr = UpdateXASound3D()) && InvalidateRect(hWnd, nullptr, FALSE);
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);
		FillRect(ps.hdc, &ps.rcPaint, GetSysColorBrush(COLOR_3DFACE));
		RECT rc;
		GetClientRect(hWnd, &rc);
		float x = (rc.left + rc.right) * 0.5f;
		float y = (rc.top + rc.bottom) * 0.5f;
		TEXTMETRIC tm;
		GetTextMetrics(ps.hdc, &tm);
		float zoom = 0.8f * tm.tmHeight;
		float cx = zoom * RadiusX;
		float cy = zoom * RadiusZ;
		Ellipse(ps.hdc, lrintf(x - cx), lrintf(y - cy), lrintf(x + cx), lrintf(y + cy));
		MoveToEx(ps.hdc, lrintf(x), lrintf(y), nullptr);
		LineTo(ps.hdc, lrintf(x + zoom * emit.Position.x), lrintf(y - zoom * emit.Position.z));
		EndPaint(hWnd, &ps);
	}
	default:
		return FALSE;
	}
	return TRUE;
}

static HRESULT UpdateXASound3D()
{
	HRESULT hr;
	__try {
		REFERENCE_TIME nrt;
		XAUDIO2_FILTER_PARAMETERS ofp = { LowPassFilter, 0.0f, 1.0f };

		FAILED_LEAVE(pRefClock->GetTime(&nrt));
		float dt = (nrt - irt) * 1e-7f;
		float ct = (nrt - ort) * 1e-7f;
		float x = RadiusX * sinf(dt * 1.2f);
		float z = RadiusZ * cosf(dt * 1.2f);
		if (ct >= FLT_MIN)
		{
			emit.Velocity.x = (x - emit.Position.x) / ct;
			emit.Velocity.z = (z - emit.Position.z) / ct;
		}
		emit.Position.x = x;
		emit.Position.z = z;
		ort = nrt;
		X3DAudioCalculate(hX3D, &list, &emit, Flags, &dsps);

		FAILED_LEAVE(pxaSrc->SetFrequencyRatio(dsps.DopplerFactor));
		FAILED_LEAVE(pxaSrc->SetOutputMatrix(pxamv, 1, 2, dsps.pMatrixCoefficients));
		FAILED_LEAVE(pxaSrc->SetOutputMatrix(pxaEff, 1, 1, &dsps.ReverbLevel));

		ofp.Frequency = sinf(X3DAUDIO_PI / 6.0f * dsps.LPFDirectCoefficient);
		FAILED_LEAVE(pxaSrc->SetOutputFilterParameters(pxamv, &ofp));
		ofp.Frequency = 2.0f * sinf(X3DAUDIO_PI / 6.0f * dsps.LPFReverbCoefficient);
		FAILED_LEAVE(pxaSrc->SetOutputFilterParameters(pxaEff, &ofp));
	}
	__finally {

	}
	return hr;
}
