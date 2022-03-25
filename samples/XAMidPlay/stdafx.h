#pragma once

// 所需包含的头文件
#define WIN32_LEAN_AND_MEAN
#define _USE_MATH_DEFINES
#include <Windows.h>
#include <Shlwapi.h>
#include <intrin.h>
#include <math.h>

#ifndef _NTDEF_
typedef __success(return >= 0) LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;
#endif

typedef VOID(NTAPI *PIO_APC_ROUTINE)(IN OUT LPVOID Argument0, IN OUT OPTIONAL LPVOID Argument1, IN OUT OPTIONAL LPVOID Argument2);
EXTERN_C NTSYSAPI NTSTATUS NTAPI NtQueueApcThread(IN HANDLE ThreadHandle, IN PIO_APC_ROUTINE ApcRoutine, IN OUT LPVOID Argument0, IN OUT OPTIONAL LPVOID Argument1, IN OUT OPTIONAL LPVOID Argument2);

// COM 错误处理
#define FAILED_RETURN(_HR_) if FAILED(hr = (_HR_)) return hr
#define FAILED_LEAVE(_HR_) if FAILED(hr = (_HR_)) __leave

const UINT32 WM_COPYGLOBALDATA = 0x49;
EXTERN_C HINSTANCE__ __ImageBase;

#ifndef ___MKID
#define ___MKID(x, y) x ## y
#endif
#ifndef __MKID
#define __MKID(x, y) ___MKID(x, y)
#endif
#ifndef __GENSYM
// Synthesize a unique symbol.
#define __GENSYM(x) __MKID(x, __COUNTER__)
#endif
