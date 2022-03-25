
// 所需包含的头文件
#include "stdafx.h"
#include <WindowsX.h>
#include <CommCtrl.h>
#include <CommDlg.h>
#include <Uxtheme.h>
#include <ShlObj.h>
#include "main.h"
#include "player.h"
#include "resource.h"

// 所需链接的库文件
#pragma comment(lib, "ComCtl32")
#pragma comment(lib, "Uxtheme")

static WNDPROC DefButtonProc;
// 主窗口的消息处理
static INT_PTR CALLBACK MainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// 对编辑控件子类化处理（实现拖拽和有符号整数编辑处理）
static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
// 对 GroupBox 控件超类化处理
static LRESULT CALLBACK SuperclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// 初始化编辑控件的状态
void InitEditUI(HWND hWnd, int ID, int Min, int Max, int Def = 0);
void InitEditUI(HWND hWnd, int ID, int Min, int Max, int Def, int Line, int Page);
// 更新编辑控件的状态
bool UpdateEditUI(HWND hWnd, int ID, bool Undo = false);

union ProgressHMS
{
	struct { long h1, m1, s1, h2, m2, s2; };
	struct { ldiv_t hm1, __GENSYM($), ms2; };
	struct { long __GENSYM($); ldiv_t ms1, hm2; };
};

// 预设的均衡器参数名称
extern const LPCTSTR EqualizerNames[] = {
	TEXT("原始"),
	TEXT("流行音乐"),
	TEXT("现场"),
	TEXT("俱乐部"),
	TEXT("摇滚乐"),
	TEXT("低音"),
	TEXT("高音"),
	TEXT("人声"),
	TEXT("强劲"),
	TEXT("舞曲"),
	TEXT("轻柔"),
	TEXT("聚会"),
	TEXT("古典乐"),
	TEXT("爵士乐"),
};

// 预设的均衡器参数数据
extern const INT8 EqualizerPresets[][12] = {
	//	20		31		62		125		250		500		1k		2k		4k		8k		16k		20k
	{ +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, }, // 原始
	{ -3, -2, +2, +4, +5, +4, -2, -3, -3, -3, -3, -3, }, // 流行音乐
	{ -4, -3, +0, +4, +5, +5, +5, +2, +2, +1, +0, +0, }, // 现场
	{ +0, +0, +0, +2, +5, +5, +5, +2, +0, +0, +0, +0, }, // 俱乐部
	{ +5, +4, +2, -2, -5, -2, +2, +5, +6, +6, +6, +6, }, // 摇滚乐
	{ 11, 10, +9, +8, +3, +0, -3, -5, -6, -8, -8, -8, }, // 低音
	{ -8, -8, -8, -8, -4, +0, +3, +9, +9, +9, +9, +9, }, // 高音
	{ -3, -3, +5, +5, +5, -3, -3, -3, -3, -3, -3, -3, }, // 人声
	{ +9, +8, +7, +4, -4, -2, +1, +6, +7, +9, +9, +9, }, // 强劲
	{ +9, +7, +4, +1, +0, +0, -3, -5, -5, +0, +0, +0, }, // 舞曲
	{ +3, +2, +0, -1, -3, -1, +1, +4, +5, +6, +7, +8, }, // 轻柔
	{ +5, +5, +5, +0, +0, +0, +0, +0, +0, +5, +5, +5, }, // 聚会
	{ +0, +0, +0, +0, +0, +0, +0, -5, -5, -5, -6, -6, }, // 古典乐
	{ +0, -2, +5, -8, +0, +8, +7, +6, +0, +0, +0, +0, }, // 爵士乐
};

// 预设的音频矩阵名称
extern const LPCTSTR MatrixerNames[] = {
	TEXT("零矩阵—静音"),
	TEXT("单位矩阵—原始"),
	TEXT("反相"),
	TEXT("单声道"),
	TEXT("左声道"),
	TEXT("右声道"),
	TEXT("左右互换"),
};

// 预设的音频矩阵数据
extern const INT16 MatrixerPresets[][4] = {
	{ 0, 0, 0, 0, },
	{ 1000, 0, 0, 1000, },
	{ -1000, 0, 0, -1000, },
	{ 500, 500, 500, 500, },
	{ 1000, 0, 0, 0, },
	{ 0, 0, 0, 1000, },
	{ 0, 1000, 1000, 0, },
};

#ifdef _WIN64
static FORCEINLINE INT64 MulDiv(INT64 nNumber, INT64 nNumerator, INT64 nDenominator)
{
	__m128d n1 = _mm_cvtsi64_sd(n1, nNumber);
	__m128d n2 = _mm_cvtsi64_sd(n2, nNumerator);
	__m128d n3 = _mm_cvtsi64_sd(n3, nDenominator);
	return _mm_cvttsd_si32(_mm_div_sd(_mm_mul_sd(n1, n2), n3));
}
#endif // _WIN64

// 添加在UAC下接收文件拖拽消息的支持
static bool InitMsgFlt()
{
	typedef BOOL (WINAPI *PCWMF)(UINT message, DWORD dwFlag);
	const HMODULE hModule = GetModuleHandle(TEXT("user32")); if (!hModule) return false;
	const PCWMF pcwmf = PCWMF(GetProcAddress(hModule, "ChangeWindowMessageFilter"));
	return pcwmf && pcwmf(WM_COPYGLOBALDATA, MSGFLT_ADD) && pcwmf(WM_DROPFILES, MSGFLT_ADD);
}

// 添加对 GroupBox 控件超类化处理
static bool InitCtrlCls()
{
	WNDCLASS wc;
	GetClassInfo(&__ImageBase, WC_BUTTON, &wc);
	wc.hbrBackground;
	DefButtonProc = wc.lpfnWndProc;
	wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	wc.lpfnWndProc = SuperclassProc;
	return UnregisterClass(wc.lpszClassName, wc.hInstance) && RegisterClass(&wc);
}

int main()
{
	HRESULT hr;
	InitCommonControls();
	InitMsgFlt();
	// 初始化COM
	FAILED_RETURN(CoInitialize(nullptr));
	__try {
		return DialogBoxParam(&__ImageBase, MAKEINTRESOURCE(IDD_MainView), HWND_DESKTOP, MainProc, InitCtrlCls());
	}
	__finally{
		CoUninitialize();	// 释放 COM
	}
	return EXIT_FAILURE;
}

static INT_PTR CALLBACK MainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static TCHAR ProgressFormat[0x40];
	static ITaskbarList3 *pTbl;
	DWORD ec;

	TCHAR buf[MAX_PATH];
	switch (uMsg)
	{
	case WM_INITDIALOG:
		// 如果对 GroupBox 控件超类化成功则开启裁剪绘制提高界面刷新效率（失败就不要开启，否则会变黑）
		if (lParam) SetWindowLong(hWnd, GWL_STYLE, GetWindowStyle(hWnd) | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		{	// 控件初始化处理
			HWND hCtrl;
			SetWindowSubclass(GetDlgItem(hWnd, IDC_EdtMidi), SubclassProc, UINT_PTR(hWnd), NULL);
			SetWindowSubclass(GetDlgItem(hWnd, IDC_EdtBank), SubclassProc, UINT_PTR(hWnd), NULL);
			Button_SetCheck(GetDlgItem(hWnd, IDC_ChkReverb), BST_CHECKED);
			InitEditUI(hWnd, IDC_EdtPitch, -2000, 2000, 0, 25, 250);
			InitEditUI(hWnd, IDC_EdtGain, 0, 2000, 1000);
			InitEditUI(hWnd, IDC_EdtPan, -1000, 1000, 0);
			InitEditUI(hWnd, IDC_EdtMix, 0, 1000, 500);
			InitEditUI(hWnd, IDC_EdtDiffuse, 0, 1000, 800);
			InitEditUI(hWnd, IDC_EdtRoom, 0, 1000, 120);
			InitEditUI(hWnd, IDC_EdtFedback, 0, 1000, 500);
			InitEditUI(hWnd, IDC_EdtDelay, 1, 2000, 500);
			hCtrl = GetDlgItem(hWnd, IDC_CmbEqual);
			for each(LPCTSTR name in EqualizerNames)
				ComboBox_AddString(hCtrl, name);
			ComboBox_SetCurSel(hCtrl, 0);
			for (int id = IDC_EdtEq20; id <= IDC_EdtEq20k; id += IDC_EdtEq31 - IDC_EdtEq20)
				InitEditUI(hWnd, id, -16, 16, 0, 2, 4);
			hCtrl = GetDlgItem(hWnd, IDC_CmbMatrix);
			for each(LPCTSTR name in MatrixerNames)
				ComboBox_AddString(hCtrl, name);
			ComboBox_SetCurSel(hCtrl, 1);
			for (int i = 0; i < ARRAYSIZE(*MatrixerPresets); ++i)
				InitEditUI(hWnd, IDC_EdtMat11 + i * (IDC_EdtMat21 - IDC_EdtMat12), -9999, 9999, MatrixerPresets[1][i]);
			static const LPTSTR header[] = { TEXT("时间点　　"), TEXT("音轨　"), TEXT("命令　"), TEXT("参数1  "), TEXT("参数2  "), };
			hCtrl = GetDlgItem(hWnd, IDC_LstOutMsg);
			LV_COLUMN lvc = { LVCF_FMT | LVCF_TEXT, LVCFMT_CENTER, };
			for (int i = 0; i < ARRAYSIZE(header); ++i)
			{
				lvc.pszText = header[i];
				ListView_InsertColumn(hCtrl, i, &lvc);
			}
			lvc.mask = 0;
			ListView_InsertColumn(hCtrl, ARRAYSIZE(header), &lvc);
			lvc.mask = LVCF_TEXT;
			lvc.pszText = buf;
			for (int i = 0; i < ARRAYSIZE(header); ++i)
			{
				ListView_SetColumnWidth(hCtrl, i, LVSCW_AUTOSIZE_USEHEADER);
				if (StrTrim(lstrcpy(buf, header[i]), TEXT(" \t　"))) ListView_SetColumn(hCtrl, i, &lvc);
			}
			ListView_DeleteColumn(hCtrl, ARRAYSIZE(header));
			static const DWORD LVSX = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER;
			ListView_SetExtendedListViewStyleEx(hCtrl, LVSX, LVSX);
			ListView_SetItemCountEx(hCtrl, ListOutMsg::MaxLines, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
			SetWindowTheme(hCtrl, L"Explorer", nullptr);	// 切换 ListView 控件的主题为资源管理器样式
			hCtrl = GetDlgItem(hWnd, IDC_LabProgress);
			SNDMSG(hCtrl, WM_GETTEXT, ARRAYSIZE(ProgressFormat), LPARAM(ProgressFormat));
			SNDMSG(hCtrl, WM_SETTEXT, 0, LPARAM());
			hCtrl = GetDlgItem(hWnd, IDC_BarProgress);
			SNDMSG(hCtrl, PBM_SETRANGE32, 0, MAXLONG);

			// 创建子线程来处理 A5MIDI 解析和 XAMIDI 合成
			xamp.hThread = CreateThread(nullptr, 0U, any_cast<LPTHREAD_START_ROUTINE>(&XAMidiPlayer::ThreadMain), &xamp, CREATE_SUSPENDED, nullptr);
			xamp.hWnd = hWnd;
			SetThreadPriority(xamp.hThread, THREAD_PRIORITY_TIME_CRITICAL);
			ResumeThread(xamp.hThread);

			// 创建任务栏接口对象（使其在Vista后的任务栏上显示播放进度）
			CoCreateInstance(__uuidof(TaskbarList), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTbl));
		}
		break;
	case WM_DESTROY:
		xamp.Exit(EXIT_SUCCESS);
		if (pTbl) pTbl->Release();
		ec = WaitForSingleObject(xamp.hThread, INFINITE);
		CloseHandle(xamp.hThread);
		break;
	case WM_CLOSE:
		EndDialog(hWnd, EXIT_SUCCESS);
		xamp.Exit(EXIT_SUCCESS);
		break;
	case ListOutMsg::WM_MidiMsg:
		if (HWND hList = GetDlgItem(hWnd, IDC_LstOutMsg))
		{	// 输出当前合成器正常合成的 MIDI 消息
			RListOutMsg lom = RListOutMsg(lParam);
			int lc = ListView_GetItemCount(hList);
			LV_ITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_STATE;
			lvi.state = GetFocus() != hList;
			lvi.iItem = lc;
			lvi.pszText = buf;

			lvi.iSubItem = 0;
			wnsprintf(buf, ARRAYSIZE(buf), TEXT("%lu"), wParam);
			if (ListView_InsertItem(hList, &lvi) < 0)
				break;
			if (lc >= lom.MaxLines - 1 && ListView_DeleteItem(hList, 0))
				--lvi.iItem;

			lvi.iSubItem = 1;
			wnsprintf(buf, ARRAYSIZE(buf), TEXT("%u"), lom.Track);
			ListView_SetItem(hList, &lvi);

			lvi.iSubItem = 2;
			wnsprintf(buf, ARRAYSIZE(buf), TEXT("0x%02X"), lom.Cmd | 0x80);
			ListView_SetItem(hList, &lvi);

			lvi.iSubItem = 3;
			wnsprintf(buf, ARRAYSIZE(buf), TEXT("0x%02X"), lom.Param1);
			ListView_SetItem(hList, &lvi);

			lvi.iSubItem = 4;
			wnsprintf(buf, ARRAYSIZE(buf), TEXT("0x%02X"), lom.Param2);
			ListView_SetItem(hList, &lvi);

			if (!lvi.state) break;
			return SNDMSG(hList, WM_VSCROLL, SB_VERT | SB_BOTTOM, NULL);
		}
		break;
	case ListOutMsg::WM_MidiUpdate:
	{	// 更新播放进度
		ProgressHMS per;
		HWND hCtrl = GetDlgItem(hWnd, IDC_BarProgress);
		if (pTbl) pTbl->SetProgressValue(hWnd, wParam, lParam);
		SNDMSG(hCtrl, PBM_SETPOS, MulDiv(INT_PTR(MAXLONG), wParam, lParam), 0);
#ifdef _WIN64
		wParam /= 100000, lParam /= 100000;
#else // _WIN32
		wParam /= 10, lParam /= 10;
#endif
		hCtrl = GetDlgItem(hWnd, IDC_LabProgress);
		per.ms1 = ldiv(wParam, 100);
		per.hm1 = ldiv(per.m1, 60);
		per.ms2 = ldiv(lParam, 100);
		per.hm2 = ldiv(per.m2, 60);
		wnsprintf(buf, ARRAYSIZE(buf), ProgressFormat, per.h1, per.m1, per.s1, per.h2, per.m2, per.s2);
		return SNDMSG(hCtrl, WM_SETTEXT, NULL, LPARAM(buf));
	}
		break;
	case WM_HSCROLL:
	case WM_VSCROLL:
		// 滑块拖动和 UpDown 控件消息处理
		if (!lParam) return FALSE;
		switch (const int ID = GetDlgCtrlID(HWND(lParam)))
		{
		case IDC_SlrPitch:
		case IDC_SlrGain:
		case IDC_SlrPan:
		case IDC_SlrMix:
		case IDC_SlrDiffuse:
		case IDC_SlrRoom:
		case IDC_SlrFedback:
		case IDC_SlrDelay:
		{
			int value = SNDMSG(HWND(lParam), TBM_GETPOS, 0, 0);
			SNDMSG(GetDlgItem(hWnd, ID + 2), UDM_SETPOS32, 0, value);
			return xamp.ControlState(ID, value);
		}
		case IDC_SlrEq20:
		case IDC_SlrEq31:
		case IDC_SlrEq62:
		case IDC_SlrEq125:
		case IDC_SlrEq250:
		case IDC_SlrEq500:
		case IDC_SlrEq1k:
		case IDC_SlrEq2k:
		case IDC_SlrEq4k:
		case IDC_SlrEq8k:
		case IDC_SlrEq16k:
		case IDC_SlrEq20k:
		{
			int value = -SNDMSG(HWND(lParam), TBM_GETPOS, 0, 0);
			SNDMSG(GetDlgItem(hWnd, ID + 2), UDM_SETPOS32, 0, value);
			return xamp.ControlState(ID, value);
		}
		default:
			return FALSE;
		}
		break;
	case WM_COMMAND:
		// 按钮点击或编辑框、组合框的事件
		switch (const int ID = LOWORD(wParam))
		{
		case IDC_EdtMidi:
			if (EN_CHANGE == HIWORD(wParam)) Button_Enable(GetDlgItem(hWnd, IDC_BtnPlay), SNDMSG(HWND(lParam), WM_GETTEXTLENGTH, NULL, NULL));
			break;
		case IDC_BtnMidi:
		case IDC_BtnBank:
		{
			static OPENFILENAME_NT4 ofn = { sizeof(ofn), hWnd, &__ImageBase, };
			switch (ID)
			{
			case IDC_BtnMidi:
				ofn.lpstrFilter = TEXT("MIDI Files (*.mid;*.midi)\0*.mid;*.midi\0All Files\0*\0");
				break;
			case IDC_BtnBank:
				ofn.lpstrFilter = TEXT("Timbre Banks Files (*.dls)\0*.dls\0All Files\0*\0");
				break;
			default:
				ofn.lpstrFilter = nullptr;
			}
			HWND hCtrl = GetDlgItem(hWnd, ID - 1);
			ofn.Flags = OFN_READONLY | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_SHAREAWARE | OFN_EXPLORER | OFN_LONGNAMES | OFN_ENABLESIZING;
			SNDMSG(hCtrl, WM_GETTEXT, ofn.nMaxFile = ARRAYSIZE(buf), LPARAM(ofn.lpstrFile = buf));
			if (GetOpenFileName(LPOPENFILENAME(&ofn)))
				SNDMSG(hCtrl, WM_SETTEXT, NULL, LPARAM(buf));
			__nop();
		}
			break;
		case IDC_ChkReverb:
		case IDC_ChkDiffuse:
		case IDC_ChkEcho:
		case IDC_BtnPlay:
			return xamp.ControlState(ID, Button_GetCheck(HWND(lParam)));
		case IDC_RstPitch:
		case IDC_RstGain:
		case IDC_RstPan:
		case IDC_RstMix:
		case IDC_RstDiffuse:
		case IDC_RstRoom:
		case IDC_RstFedback:
		case IDC_RstDelay:
			SetDlgItemInt(hWnd, ID - 2, GetWindowLong(HWND(lParam), GWLP_USERDATA), TRUE);
			return UpdateEditUI(hWnd, ID - 2);
		case IDC_RstEqual:
			wParam = MAKEWPARAM(IDC_CmbEqual, CBN_SELCHANGE);
			lParam = LPARAM(GetDlgItem(hWnd, IDC_CmbEqual));
			ComboBox_SetCurSel(HWND(lParam), 0);
		case IDC_CmbEqual:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
			{
				int i = ComboBox_GetCurSel(HWND(lParam));
				for (int j = 0; j < ARRAYSIZE(*EqualizerPresets); ++j)
				{
					const int ID = IDC_SlrEq20 + j * (IDC_SlrEq31 - IDC_SlrEq20);
					SNDMSG(GetDlgItem(hWnd, ID + 0), TBM_SETPOS, TRUE, -EqualizerPresets[i][j]);
					SNDMSG(GetDlgItem(hWnd, ID + 2), UDM_SETPOS32, 0, EqualizerPresets[i][j]);
				}
				return xamp.ControlState(ID, i);
			}
			}
			break;
		case IDC_RstMatrix:
			wParam = MAKEWPARAM(IDC_CmbMatrix, CBN_SELCHANGE);
			lParam = LPARAM(GetDlgItem(hWnd, IDC_CmbMatrix));
			ComboBox_SetCurSel(HWND(lParam), 1);
		case IDC_CmbMatrix:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
			{
				int i = ComboBox_GetCurSel(HWND(lParam));
				for (int j = 0; j < ARRAYSIZE(*MatrixerPresets); ++j)
					SNDMSG(GetDlgItem(hWnd, IDC_SpnMat11 + j * (IDC_SpnMat21 - IDC_SpnMat12)), UDM_SETPOS32, 0, MatrixerPresets[i][j]);
				return xamp.ControlState(ID, i);
			}
			}
			break;
		case IDOK:
		case IDCANCEL:
			return UpdateEditUI(hWnd, GetDlgCtrlID(GetFocus()), IDOK != ID);
		default:
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				return UpdateEditUI(hWnd, ID);
			}
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_CHAR:
		if (wParam > VK_ESCAPE && (GetWindowStyle(hWnd) & ES_NUMBER))
		{	// 允许数字编辑框输入-号
			long sel = DefSubclassProc(hWnd, EM_GETSEL, NULL, NULL);
			if (LOWORD(sel)) break;
			TCHAR buf[2];
			if (!HIWORD(sel) && DefSubclassProc(hWnd, WM_GETTEXT, ARRAYSIZE(buf), LPARAM(buf)) > 0 && TEXT('-') == *buf)
				return 0;
			if (TEXT('-') != wParam) break;
			return DefSubclassProc(hWnd, EM_REPLACESEL, NULL, LPARAM(&wParam));
		}
		break;
	case WM_DROPFILES:
		// 收到 Explorer 拖拽文件进来时切换音色库
		__try {
			LPDROPFILES pdf = LPDROPFILES(GlobalLock(HGLOBAL(wParam)));
			if (GetWindowStyle(hWnd) & ES_READONLY) __leave;
			return DefSubclassProc(hWnd, WM_SETTEXT, 0, DWORD_PTR(pdf) + pdf->pFiles);
		}
		__finally {
			// 解锁并释放拖拽文件产生的临时内存
			GlobalUnlock(HGLOBAL(wParam));
			GlobalFree(HGLOBAL(wParam));
		}
		return 0;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK SuperclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND:
		if (BS_GROUPBOX == (BS_TYPEMASK & GetWindowStyle(hWnd)))
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return DefButtonProc(hWnd, uMsg, wParam, lParam);
}

static FORCEINLINE void InitEditUI(HWND hWnd, int ID, int Min, int Max, int Def)
{
	return InitEditUI(hWnd, ID, Min, Max, Def, (Max - Min) / 100, (Max - Min) / 10);
}

static void InitEditUI(HWND hWnd, int ID, int Min, int Max, int Def, int Line, int Page)
{
	HWND hCtrl = GetDlgItem(hWnd, ID);
	if (Min < 0) SetWindowSubclass(hCtrl, SubclassProc, UINT_PTR(hWnd), NULL);
	if (ID - 1 == GetDlgCtrlID(hCtrl = GetWindow(hCtrl, GW_HWNDPREV)))
	{
		SNDMSG(hCtrl, TBM_SETRANGEMIN, TRUE, Min);
		SNDMSG(hCtrl, TBM_SETRANGEMAX, TRUE, Max);
		SNDMSG(hCtrl, TBM_SETLINESIZE, 0, Line);
		SNDMSG(hCtrl, TBM_SETPAGESIZE, 0, Page);
		SNDMSG(hCtrl, TBM_SETPOS, TRUE, Def);
	}
	hCtrl = GetDlgItem(hWnd, ID + 1);
	SNDMSG(hCtrl, UDM_SETRANGE32, Min, Max);
	SNDMSG(hCtrl, UDM_SETPOS32, 0, Def);
	if (ID + 2 == GetDlgCtrlID(hCtrl = GetWindow(hCtrl, GW_HWNDNEXT)))
		SetWindowLong(hCtrl, GWLP_USERDATA, Def);
}

static bool UpdateEditUI(HWND hWnd, int ID, bool Undo)
{
	switch (ID)
	{
	case IDC_EdtPitch:
	case IDC_EdtGain:
	case IDC_EdtPan:
	case IDC_EdtMix:
	case IDC_EdtDiffuse:
	case IDC_EdtRoom:
	case IDC_EdtFedback:
	case IDC_EdtDelay:
	{
		int value, suc;
		switch (Undo)
		{
		case false:
			value = GetDlgItemInt(hWnd, ID, &suc, TRUE);
			if (suc)
			{
				SNDMSG(GetDlgItem(hWnd, ID - 1), TBM_SETPOS, TRUE, value);
				break;
			}
		case true:
			value = SNDMSG(GetDlgItem(hWnd, ID - 1), TBM_GETPOS, 0, 0);
		}
		SetDlgItemInt(hWnd, ID, value, TRUE);
		return xamp.ControlState(ID, value);
	}
	case IDC_EdtEq20:
	case IDC_EdtEq31:
	case IDC_EdtEq62:
	case IDC_EdtEq125:
	case IDC_EdtEq250:
	case IDC_EdtEq500:
	case IDC_EdtEq1k:
	case IDC_EdtEq2k:
	case IDC_EdtEq4k:
	case IDC_EdtEq8k:
	case IDC_EdtEq16k:
	case IDC_EdtEq20k:
	{
		int value, suc;
		switch (Undo)
		{
		case false:
			value = GetDlgItemInt(hWnd, ID, &suc, TRUE);
			if (suc)
			{
				SNDMSG(GetDlgItem(hWnd, ID - 1), TBM_SETPOS, TRUE, -value);
				break;
			}
		case true:
			value = -SNDMSG(GetDlgItem(hWnd, ID - 1), TBM_GETPOS, 0, 0);
		}
		SetDlgItemInt(hWnd, ID, value, TRUE);
		return xamp.ControlState(ID, value);
	}
	case IDC_EdtMat11:
	case IDC_EdtMat12:
	case IDC_EdtMat21:
	case IDC_EdtMat22:
	{
		int value, suc;
		switch (Undo)
		{
		case false:
			value = GetDlgItemInt(hWnd, ID, &suc, TRUE);
			if (suc) break;
		case true:
			value = SNDMSG(GetDlgItem(hWnd, ID + 1), UDM_GETPOS32, 0, 0);
		}
		SetDlgItemInt(hWnd, ID, value, TRUE);
		return xamp.ControlState(ID, value);
	}
	default:
		return false;
	}
	return true;
}
