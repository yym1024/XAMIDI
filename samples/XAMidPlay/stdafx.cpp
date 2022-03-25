
#include "stdafx.h"

// 所需链接的库文件
#pragma comment(lib, "ntdll")
#pragma comment(lib, "Shlwapi")
#pragma comment(lib, "WinMM")

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
