
// 所需包含的头文件
#define _USE_MATH_DEFINES
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WindowsX.h>
#include <MMSystem.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <XAPOFX.h>
#include <XAMIDIHelper.hpp>
#include <intrin.h>
#include "resource.h"

// 所需链接的库文件
#pragma comment(lib, "WinMM")
#pragma comment(lib, "ComCtl32")
#pragma comment(lib, "Shlwapi")
#pragma comment(lib, "XAPOFX")
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
// 键盘钩子消息处理
LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam);
// 初始化滑块控件的状态
void InitSliderUI(HWND hWnd, int ID, UINT Max, UINT Def = 0);
// 更新编辑控件的状态
bool UpdateEditUI(HWND hWnd, HWND hCtrl, bool Undo = false);
// 响应部分控件的操作状态
HRESULT ControlState(HWND hWnd, int ID, UINT Value);
// 切换 DLS 音色库文件
HRESULT ChangeBanks(HWND hWnd, LPCWSTR pPath = nullptr);
// 切换单个音色到 XAMIDI 中
HRESULT ChangeTimbre(UINT32 Index = 0U);

// 音调映射表（按大调规律）
static const byte Pitchs[] = { 0, 2, 4, 5, 7, 9, 11, };
// 基本音调（定义到中央C）
static byte BasePitch = 4;
// 打击垫起始音调
static const byte HitPad = 0x24;
// 乐器表（Alt+1~8对应的音色）
static const byte Band[] = { 0, 1, 25, 32, 40, 65, 71, 49, };

// 定义一个未公开消息常量（Explorer拖拽文件到窗口上需要）
static const UINT WM_COPYGLOBALDATA = 0x49;

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
static IXAMIDITimbreDownloaded *pxamtd = nullptr;
static FXREVERB_PARAMETERS fxrp = { 0.8f, 0.12f, };
static HHOOK hHook = nullptr;
static HWND hWnd = nullptr;

// 添加在UAC下接收文件拖拽消息的支持
static bool InitMsgFlt()
{
	typedef BOOL(WINAPI *PCWMF)(UINT message, DWORD dwFlag);
	const HMODULE hModule = GetModuleHandle(TEXT("user32")); if (!hModule) return false;
	const PCWMF pcwmf = PCWMF(GetProcAddress(hModule, "ChangeWindowMessageFilter"));
	return pcwmf && pcwmf(WM_COPYGLOBALDATA, MSGFLT_ADD) && pcwmf(WM_DROPFILES, MSGFLT_ADD);
}

// 主函数（程序入口）
int main()
{
	HRESULT hr;

	InitMsgFlt();
	// 初始化COM
	if FAILED(hr = CoInitialize(nullptr)) return hr;
	__try {
		timeBeginPeriod(0U);
		// 合成器初始参数设置
		XAMIDI_SYNTH_PARAMETERS xasp;
		xasp.Flags = 0;
		xasp.Effects = XAMIDI_EFFECT_NONE;					// 不用任何特效
		xasp.OutputChannels = 1;							// 输出到 XAudio 的声道数（1为单声道，2为立体声，1一般用于3D音效生成动态立体声(适用于游戏)，2一般是用于 midi 自身的静态立体声(适用于播放器)）
		xasp.OutputSampleRate = XAMIDI_MAX_SAMPLE_RATE;		// 输出到 XAudio 的采样率（支持 11025Hz(低音质)、22050Hz(中音质)、44100Hz(高音质) 三种，默认为中等音质）
		xasp.VoiceCount = 64;								// 复音数（允许同时播放多少个 MIDI 音符，默认是32个，最大支持1000个）
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

		// 设置 XAudio2 缓冲区的房间混响特效
		XAUDIO2_EFFECT_DESCRIPTOR saed = {};
		const XAUDIO2_EFFECT_CHAIN xaec = { 1, &saed, };
		saed.InitialState = TRUE;
		saed.OutputChannels = xasp.OutputChannels;
		FAILED_LEAVE(CreateFX(__uuidof(FXReverb), &saed.pEffect));			// 创建混响特效
		hr = pxaEff->SetEffectChain(&xaec);
		saed.pEffect->Release();
		if FAILED(hr) __leave;
		FAILED_LEAVE(pxaEff->SetEffectParameters(0, &fxrp, sizeof(fxrp)));

		// 设置 XAMIDI 输出 XAudio2 的目标为直达声和混响声两个缓冲区
		xasd[1].pOutputVoice = pxaEff;		// 直达声不变继续输出到pxaMix，而混响声输出到pxaEff。
		xavs.SendCount = ARRAYSIZE(xasd);
		FAILED_LEAVE(pxamsv->Initialize(&xasp, &xavs));		// 初始化辅助交互对象
		pxaSrc = pxamsv->GetSourceVoice();					// 获取合成器对象（我们需要通过它来实现midi输出）
		pSynth = pxamsv->GetSynthesizer();					// 获取合成器对象（我们需要通过它来实现midi输出）

		InitCommonControls();
		hr = DialogBox(&__ImageBase, MAKEINTRESOURCE(IDD_MainView), HWND_DESKTOP, MainProc);
	}
	__finally {
		// 释放对象操作（注意：delete 语句会自行检测是否空指针，而调用对象释放方法则必须自己手动检测）
		if (hHook) UnhookWindowsHookEx(hHook);
		if (pSynth && pSynth->GetEnabled()) pSynth->SetEnabled(FALSE);		// 停止 MIDI 合成
		if (pxamtd) pxamtd->UnloadTimbre();	// 卸载并释放音色
		delete pxamsv;		// 释放辅助对象（里面会自动释放 XAMIDI 合成器 和 XAudio2 源缓冲区）
		if (pxamtb) pxamtb->DestroyBanks();	// 销毁 XAMIDI 音色库
		if (pxaEff) pxaEff->DestroyVoice();	// 销毁特效子缓冲区（反射）
		if (pxaMix) pxaMix->DestroyVoice();	// 销毁混音子缓冲区（按干湿比混合直达声和混响声）
		if (pxamv) pxamv->DestroyVoice();	// 销毁 XAudio2 主缓冲区
		if (pxm) pxm->Release();			// 释放 XAMIDI 引擎
		if (pxa) pxa->Release();			// 释放 XAudio2 引擎
		timeEndPeriod(0U);
		CoUninitialize();	// 释放 COM
	}
	return hr;			// 返回错误码
}

static INT_PTR CALLBACK MainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HRESULT hr;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{	// 控件初始化处理
		if FAILED(hr = ChangeBanks(hWnd))
			return EndDialog(hWnd, hr);
		SetDlgItemInt(hWnd, IDC_EdtOctave, BasePitch, FALSE);
		Edit_LimitText(GetDlgItem(hWnd, IDC_EdtOctave), 1);
		SNDMSG(GetDlgItem(hWnd, IDC_SpnOctave), UDM_SETRANGE, 0, MAKELPARAM(8, 0));
		InitSliderUI(hWnd, IDC_SlrVibrato, 1000);
		InitSliderUI(hWnd, IDC_SlrPitch, 1000, 500);
		InitSliderUI(hWnd, IDC_SlrGain, 4000, 1000);
		InitSliderUI(hWnd, IDC_SlrPan, 2000, 1000);
		InitSliderUI(hWnd, IDC_SlrMix, 1000, 500);
		InitSliderUI(hWnd, IDC_SlrDiffuse, 1000, 800);
		InitSliderUI(hWnd, IDC_SlrRoom, 1000, 120);
	}
	case WM_EXITMENULOOP:
	case WM_EXITSIZEMOVE:
		// 退出菜单或拖拽窗口后恢复按键响应
		::hWnd = hWnd;
		return FALSE;
	case WM_ENTERMENULOOP:
	case WM_ENTERSIZEMOVE:
		// 进入菜单或拖拽窗口后继续按键响应
		::hWnd = nullptr;
		return FALSE;
	case WM_CLOSE:
		// 点击关闭按钮后结束对话框
		return EndDialog(hWnd, EXIT_SUCCESS);
	case WM_ACTIVATE:
		if (WA_INACTIVE == LOWORD(wParam))
		{	// 窗口失去焦点后停止按键响应
			if (hHook) UnhookWindowsHookEx(hHook);
			hHook = nullptr;
			return SUCCEEDED(pxamsv->Stop());
		}
		else if (!hHook)
		{	// 窗口获得焦点后恢复按键响应
			hHook = SetWindowsHookEx(WH_KEYBOARD_LL, HookProc, &__ImageBase, 0);
			return SUCCEEDED(pxamsv->Start()) && SUCCEEDED(pSynth->SetWriteLatency(40));
		}
		break;
	case WM_DROPFILES:
		// 收到 Explorer 拖拽文件进来时切换音色库
		__try {
			LPDROPFILES pdf = LPDROPFILES(GlobalLock(HGLOBAL(wParam)));
			if (pdf && pdf->fWide)
				FAILED_LEAVE(ChangeBanks(hWnd, LPCWSTR(DWORD_PTR(pdf) + pdf->pFiles)));
			else return FALSE;
		}
		__finally {
			// 解锁并释放拖拽文件产生的临时内存
			GlobalUnlock(HGLOBAL(wParam));
			GlobalFree(HGLOBAL(wParam));
		}
		break;
	case WM_HSCROLL:
		// 滑块控件拖动消息处理
		if (!lParam) return FALSE;
		switch (int id = GetDlgCtrlID(HWND(lParam)))
		{
		case IDC_SlrVibrato:
		case IDC_SlrPitch:
		case IDC_SlrGain:
		case IDC_SlrPan:
		case IDC_SlrMix:
		case IDC_SlrDiffuse:
		case IDC_SlrRoom:
		{
			UINT value = SNDMSG(HWND(lParam), TBM_GETPOS, 0, 0);
			SNDMSG(GetDlgItem(hWnd, id + 2), UDM_SETPOS, 0, value);
			return SUCCEEDED(ControlState(hWnd, id, value));
		}
		default:
			return FALSE;
		}
		break;
	case WM_VSCROLL:
		// UpDown 控件消息处理
		if (!lParam) return FALSE;
		switch (int id = GetDlgCtrlID(HWND(lParam)))
		{
		case IDC_SpnOctave:
		case IDC_SpnVibrato:
		case IDC_SpnPitch:
		case IDC_SpnGain:
		case IDC_SpnPan:
		case IDC_SpnMix:
		case IDC_SpnDiffuse:
		case IDC_SpnRoom:
		{
			UINT value = SNDMSG(HWND(lParam), UDM_GETPOS, 0, 0);
			SNDMSG(GetDlgItem(hWnd, id - 2), TBM_SETPOS, TRUE, value);
			return SUCCEEDED(ControlState(hWnd, id, value));
		}
		default:
			return FALSE;
		}
		break;
	case WM_COMMAND:
		// 按钮点击或编辑框、组合框的事件
		switch (int id = LOWORD(wParam))
		{
		case IDC_CmbTimbre:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				return SUCCEEDED(ChangeTimbre(ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_CmbTimbre))));
			break;
		case IDOK:
		case IDCANCEL:
			UpdateEditUI(hWnd, GetFocus(), IDOK != id);
			break;
		default:
			if (HIWORD(wParam) == EN_KILLFOCUS)
				UpdateEditUI(hWnd, HWND(lParam));
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

static LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam)
{
	const KBDLLHOOKSTRUCT &cur = *LPKBDLLHOOKSTRUCT(lParam);
	DWORD flags = 0;
	if (HC_ACTION == code && hWnd) switch (wParam)
	{
	case WM_KEYDOWN:	//键盘按下事件
		// 屏蔽重复按键事件（保证只首次按下才执行事件）
		if (GetAsyncKeyState(cur.vkCode) < 0) break;
		if (cur.vkCode >= 'A' && cur.vkCode <= 'Z')
		{
			// A~Z 向正常的通道(0)输出消息
			div_t dv = div(int(cur.vkCode - 'A'), ARRAYSIZE(Pitchs));
			pSynth->OutShortMsg(0x007F0000 |
				MAKEWORD(0x90, (BasePitch + dv.quot) * 12 + Pitchs[dv.rem]));
		}
		else switch (cur.vkCode)
		{
		case ' ':
			// 按下空格键时踩下延音踏板
			pSynth->OutShortMsg(0x007F40B0);
			break;
		//case VK_ADD:
		case VK_OEM_PLUS:
			// 按下+键提高一个八度
			if (BasePitch < 8)
				PostMessage(GetDlgItem(hWnd, IDC_SpnOctave), UDM_SETPOS, 0, ++BasePitch);
			break;
		//case VK_SUBTRACT:
		case VK_OEM_MINUS:
			// 按下-键降低一个八度
			if (BasePitch > 0)
				PostMessage(GetDlgItem(hWnd, IDC_SpnOctave), UDM_SETPOS, 0, --BasePitch);
			break;
		default:
			goto Next;
		}
		SetFocus(hWnd);
		break;
	case WM_KEYUP:	// 键盘放开事件
		if (cur.vkCode >= 'A' && cur.vkCode <= 'Z')
		{
			// A~Z 向正常的通道(0)输出消息
			div_t dv = div(int(cur.vkCode - 'A'), ARRAYSIZE(Pitchs));
			pSynth->OutShortMsg(0x00000000 |
				MAKEWORD(0x80, (BasePitch + dv.quot) * 12 + Pitchs[dv.rem]));
		}
		else switch (cur.vkCode)
		{
		case ' ':
			// 放开空格键时弹起延音踏板
			pSynth->OutShortMsg(0x000040B0);
			break;
		default:
			goto Next;
		}
		SetFocus(hWnd);
		break;
	}
Next:
	return CallNextHookEx(hHook, code, wParam, lParam);
}

static void InitSliderUI(HWND hWnd, int ID, UINT Max, UINT Def)
{
	HWND hCtrl = GetDlgItem(hWnd, ID);
	SNDMSG(hCtrl, TBM_SETRANGE, FALSE, MAKELPARAM(0, Max));
	SNDMSG(hCtrl, TBM_SETLINESIZE, 0, Max / 100);
	SNDMSG(hCtrl, TBM_SETPAGESIZE, 0, Max / 10);
	SNDMSG(hCtrl, TBM_SETPOS, TRUE, Def);
	hCtrl = GetDlgItem(hWnd, ID + 1);
	//Edit_LimitText(hCtrl, Max < 10 ? 1 : int(log10f(float(Max))) + 1);
	Edit_LimitText(hCtrl, 4);
	hCtrl = GetDlgItem(hWnd, ID + 2);
	SNDMSG(hCtrl, UDM_SETRANGE, 0, MAKELPARAM(Max, 0));
	SNDMSG(hCtrl, UDM_SETPOS, 0, Def);
}

static bool UpdateEditUI(HWND hWnd, HWND hCtrl, bool Undo)
{
	//if (GetParent(hCtrl) != hWnd) return false;
	int id = GetDlgCtrlID(hCtrl);
	switch (id)
	{
	case IDC_EdtOctave:
	case IDC_EdtVibrato:
	case IDC_EdtPitch:
	case IDC_EdtGain:
	case IDC_EdtPan:
	case IDC_EdtMix:
	case IDC_EdtDiffuse:
	case IDC_EdtRoom:
		if (Undo)
			SetDlgItemInt(hWnd, id, SNDMSG(GetDlgItem(hWnd, id - 1), TBM_GETPOS, 0, 0), FALSE);
		else
		{
			UINT value = GetDlgItemInt(hWnd, id, nullptr, FALSE);
			SNDMSG(GetDlgItem(hWnd, id - 1), TBM_SETPOS, TRUE, value);
			return SUCCEEDED(ControlState(hWnd, id, value));
		}
		break;
	default:
		return false;
	}
	return true;
}

static HRESULT ControlState(HWND hWnd, int ID, UINT Value)
{
	switch (ID)
	{
	case IDC_EdtOctave:
	case IDC_SpnOctave:
		// 八度切换
		BasePitch = Value;
		break;
	case IDC_SlrVibrato:
	case IDC_SpnVibrato:
	case IDC_EdtVibrato:
		// 颤音控制
		Value = MulDiv(Value, 16383, 1000);
		pSynth->OutShortMsg(0x000001B0 | ((Value << 9) & 0x007F0000));
		pSynth->OutShortMsg(0x000021B0 | ((Value << 16) & 0x007F0000));
		break;
	case IDC_SlrPitch:
	case IDC_SpnPitch:
	case IDC_EdtPitch:
		// 滑音控制
		Value = MulDiv(Value, 16383, 1000);
		//pSynth->OutShortMsg(0x000005B0 | ((Value << 9) & 0x007F0000));
		//pSynth->OutShortMsg(0x000025B0 | ((Value << 16) & 0x007F0000));
		pSynth->OutShortMsg(0x000000E0 | ((Value << 9) & 0x007F0000) | ((Value << 8) & 0x00007F00));
		break;
	case IDC_SlrGain:
	case IDC_SpnGain:
	case IDC_EdtGain:
		// 音量控制
		return pxaMix->SetVolume(Value * 1e-3f);
	case IDC_SlrPan:
	case IDC_SpnPan:
	case IDC_EdtPan:
	{	// 平衡控制
		float vols[2] = { 1.f, 1.f, };
		if (Value > 1000)
			vols[0] = (2000 - Value) * 1e-3f;
		else if (Value < 1000)
			vols[1] = (Value) * 1e-3f;
		return pxaMix->SetOutputMatrix(nullptr, 1, ARRAYSIZE(vols), vols);
		//return pxaSrc->SetChannelVolumes(ARRAYSIZE(vols), vols);
	}
		break;
	case IDC_SlrMix:
	case IDC_SpnMix:
	case IDC_EdtMix:
	{	// 干湿比控制
		float vol = Value * 2e-3f;
		pxaSrc->SetOutputMatrix(pxaEff, 1, 1, &vol);
		vol = (1000 - Value) * 2e-3f;
		pxaSrc->SetOutputMatrix(pxaMix, 1, 1, &vol);
	}
		break;
	case IDC_SlrDiffuse:
	case IDC_SpnDiffuse:
	case IDC_EdtDiffuse:
		// 漫反射控制
		fxrp.Diffusion = Value * 1e-3f;
		if (fxrp.Diffusion < FXREVERB_MIN_DIFFUSION)
			fxrp.Diffusion = FXREVERB_MIN_DIFFUSION;
		return pxaEff->SetEffectParameters(0, &fxrp, sizeof(fxrp));
	case IDC_SlrRoom:
	case IDC_SpnRoom:
	case IDC_EdtRoom:
		// 房间大小控制
		fxrp.RoomSize = Value * 1e-3f;
		if (fxrp.RoomSize < FXREVERB_MIN_ROOMSIZE)
			fxrp.RoomSize = FXREVERB_MIN_ROOMSIZE;
		return pxaEff->SetEffectParameters(0, &fxrp, sizeof(fxrp));
	default:
		return E_INVALIDARG;
	}
	return true;
}

static HRESULT ChangeBanks(HWND hWnd, LPCWSTR pPath)
{
	HRESULT hr;
	IXAMIDITimbreBanks *pxamtb = nullptr;
	IXAMIDITimbreDownloaded *pxamtd = nullptr;

	__try {
		FAILED_LEAVE(pxm->LoadTimbreBanks(&pxamtb, pPath ? XAMIDI_FILEPATH : XAMIDI_DEFAULT, pPath));

		// 获取音色并下载音色到合成器中
		if (HXAMIDITimbre hTimbre = pxamtb->IndexOfTimbre(0))
			FAILED_LEAVE(pSynth->DownloadTimbre(&pxamtd, 0, hTimbre));

		std::swap(pxamtd, ::pxamtd);
		std::swap(pxamtb, ::pxamtb);
	}
	__finally {
		if (pxamtd) pxamtd->UnloadTimbre();	// 卸载并释放音色
		if (pxamtb) pxamtb->DestroyBanks();	// 销毁 XAMIDI 音色库
	}
	{
		TCHAR buf[80];
		WCHAR name[0x40];
		HWND hCmb = GetDlgItem(hWnd, IDC_CmbTimbre);
		UINT32 count = ::pxamtb->GetTimbreCount();
		ComboBox_ResetContent(hCmb);
		for (UINT32 i = 0; i < count; ++i)
		{	// 生成音色选择列表
			XAMIDI_TIMBRE_DETAILS td;
			td.Name = name;
			td.NameLen = ARRAYSIZE(name);
			::pxamtb->GetTimbreDetails(i, &td);
			wnsprintf(buf, ARRAYSIZE(buf), TEXT("%c%u.%u.%u %ws"),
				XMIDIIsDrumPatch(td.Patch) ? '+' : '-',
				XMIDIGetPatchMSBank(td.Patch),
				XMIDIGetPatchLSBank(td.Patch),
				XMIDIGetPatchInstrument(td.Patch),
				td.Name);
			ComboBox_AddString(hCmb, buf);
		}
		ComboBox_SetCurSel(hCmb, 0);
	}
	return hr;
}

static HRESULT ChangeTimbre(UINT32 Index)
{
	IXAMIDITimbreDownloaded *pxamtd = nullptr;
	HXAMIDITimbre hTimbre = pxamtb->IndexOfTimbre(Index);		// 获取音色句柄
	if (!hTimbre) return E_INVALIDARG;
	HRESULT hr = pSynth->DownloadTimbre(&pxamtd, 0, hTimbre);	// 下载音色到合成器中
	if SUCCEEDED(hr)
	{	// 如果成功则卸载并释放旧音色
		if (::pxamtd) ::pxamtd->UnloadTimbre();
		::pxamtd = pxamtd;
	}
	return hr;
}
