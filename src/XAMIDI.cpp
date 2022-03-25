#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement

#include <Windows.h>
#undef GetObject
#include <Ks.h>
#include <KsProxy.h>
#include <dmusicc.h>
#include "XAMIDI.h"
#include <winnt.h>
#include <xmmintrin.h>
#include <stddef.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>
#define lrintf(_X) long(_mm_cvt_ss2si(_mm_set_ss(_X)))
#if _MSC_VER >= 1500
#define OVERRIDE override
#else
#define OVERRIDE
#endif // _MSC_VER >= 1500
#include "XADSound.h"
#include "XADMusic.h"

#ifndef _DMUSICI_
#define _DMUSICI_

#define DMUS_MAX_NAME           64         /* Maximum object name length. */
#define DMUS_MAX_CATEGORY       64         /* Maximum object category name length. */
#define DMUS_MAX_FILENAME       MAX_PATH

typedef struct _DMUS_VERSION {
    DWORD    dwVersionMS;
    DWORD    dwVersionLS;
}DMUS_VERSION, FAR *LPDMUS_VERSION;

typedef struct _DMUS_OBJECTDESC
{
    DWORD          dwSize;                 /* Size of this structure. */
    DWORD          dwValidData;            /* Flags indicating which fields below are valid. */
    GUID           guidObject;             /* Unique ID for this object. */
    GUID           guidClass;              /* GUID for the class of object. */
    FILETIME       ftDate;                 /* Last edited date of object. */
    DMUS_VERSION   vVersion;               /* Version. */
    WCHAR          wszName[DMUS_MAX_NAME]; /* Name of object. */
    WCHAR          wszCategory[DMUS_MAX_CATEGORY]; /* Category for object (optional). */
    WCHAR          wszFileName[DMUS_MAX_FILENAME]; /* File path. */
    LONGLONG       llMemLength;            /* Size of Memory data. */
    LPBYTE         pbMemData;              /* Memory pointer for data. */
    IStream *      pStream;                /* Stream with data. */
} DMUS_OBJECTDESC, *LPDMUS_OBJECTDESC;

#define DMUS_OBJ_OBJECT         (1 << 0)     /* Object GUID is valid. */
#define DMUS_OBJ_CLASS          (1 << 1)     /* Class GUID is valid. */
#define DMUS_OBJ_NAME           (1 << 2)     /* Name is valid. */
#define DMUS_OBJ_CATEGORY       (1 << 3)     /* Category is valid. */
#define DMUS_OBJ_FILENAME       (1 << 4)     /* File path is valid. */
#define DMUS_OBJ_FULLPATH       (1 << 5)     /* Path is full path. */
#define DMUS_OBJ_URL            (1 << 6)     /* Path is URL. */
#define DMUS_OBJ_VERSION        (1 << 7)     /* Version is valid. */
#define DMUS_OBJ_DATE           (1 << 8)     /* Date is valid. */
#define DMUS_OBJ_LOADED         (1 << 9)     /* Object is currently loaded in memory. */
#define DMUS_OBJ_MEMORY         (1 << 10)    /* Object is pointed to by pbMemData. */
#define DMUS_OBJ_STREAM         (1 << 11)    /* Object is stored in pStream. */

DECLARE_INTERFACE_IID_(IDirectMusicLoader, IUnknown, "2ffaaca2-5dca-11d2-afa6-00aa0024d8b6")
{
    /* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG, AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG, Release)       (THIS) PURE;

    /* IDirectMusicLoader */
    STDMETHOD(GetObject)            (THIS_ LPDMUS_OBJECTDESC pDesc,
        REFIID riid,
        LPVOID FAR *ppv) PURE;
    STDMETHOD(SetObject)            (THIS_ LPDMUS_OBJECTDESC pDesc) PURE;
    STDMETHOD(SetSearchDirectory)   (THIS_ REFGUID rguidClass,
        WCHAR *pwzPath,
        BOOL fClear) PURE;
    STDMETHOD(ScanDirectory)        (THIS_ REFGUID rguidClass,
        WCHAR *pwzFileExtension,
        WCHAR *pwzScanFileName) PURE;
    STDMETHOD(CacheObject)          (THIS_ IDirectMusicObject * pObject) PURE;
    STDMETHOD(ReleaseObject)        (THIS_ IDirectMusicObject * pObject) PURE;
    STDMETHOD(ClearCache)           (THIS_ REFGUID rguidClass) PURE;
    STDMETHOD(EnableCache)          (THIS_ REFGUID rguidClass,
        BOOL fEnable) PURE;
    STDMETHOD(EnumObject)           (THIS_ REFGUID rguidClass,
        DWORD dwIndex,
        LPDMUS_OBJECTDESC pDesc) PURE;
};

#endif /* #ifndef _DMUSICI_ */

interface DECLSPEC_UUID("279AFA83-4981-11CE-A521-0020AF0BE560") IDirectSound;
interface DECLSPEC_UUID("C50A7E93-F395-4834-9EF6-7FA99DE50966") IDirectSound8;
interface DECLSPEC_UUID("279AFA85-4981-11CE-A521-0020AF0BE560") IDirectSoundBuffer;
interface DECLSPEC_UUID("6825a449-7524-4d82-920f-50e36ab3ab1e") IDirectSoundBuffer8;

interface DECLSPEC_UUID("6536115a-7b2d-11d2-ba18-0000f875ac12") IDirectMusic;
interface DECLSPEC_UUID("d2ac2878-b39b-11d1-8704-00600893b1bd") IDirectMusicBuffer;
interface DECLSPEC_UUID("08f2d8c9-37c2-11d2-b9f9-0000f875ac12") IDirectMusicPort;
interface DECLSPEC_UUID("ced153e7-3606-11d2-b9f9-0000f875ac12") IDirectMusicThru;
interface DECLSPEC_UUID("d2ac287a-b39b-11d1-8704-00600893b1bd") IDirectMusicPortDownload;
interface DECLSPEC_UUID("d2ac287b-b39b-11d1-8704-00600893b1bd") IDirectMusicDownload;
interface DECLSPEC_UUID("d2ac287c-b39b-11d1-8704-00600893b1bd") IDirectMusicCollection;
interface DECLSPEC_UUID("d2ac287d-b39b-11d1-8704-00600893b1bd") IDirectMusicInstrument;
interface DECLSPEC_UUID("d2ac287e-b39b-11d1-8704-00600893b1bd") IDirectMusicDownloadedInstrument;
interface DECLSPEC_UUID("6fc2cae1-bc78-11d2-afa6-00aa0024d8b6") IDirectMusic2;
interface DECLSPEC_UUID("2d3629f7-813d-4939-8508-f05c6b75fd97") IDirectMusic8;
interface DECLSPEC_UUID("d2ac28b5-b39b-11d1-8704-00600893b1bd") IDirectMusicObject;

class DECLSPEC_UUID("636b9f10-0c7d-11d1-95b2-0020afdc7421") CDirectMusic : IDirectMusic {};
class DECLSPEC_UUID("480ff4b0-28b2-11d1-bef7-00c04fbf8fef") CDirectMusicCollection : IDirectMusicCollection {};
class DECLSPEC_UUID("58C2B4D0-46E7-11D1-89AC-00A0C9054129") CDirectMusicSynth;
class DECLSPEC_UUID("d2ac2892-b39b-11d1-8704-00600893b1bd") CDirectMusicLoader : IDirectMusicLoader {};

EXTERN_C const IID DECLSPEC_SELECTANY IID_IXAMIDI = { 0x78C390D9, 0x9571, 0x42AA, 0x84, 0x70, 0xAC, 0x91, 0x76, 0x6F, 0xC4, 0xAC, };
EXTERN_C const GUID DECLSPEC_SELECTANY GUID_DirectMusicAllTypes = {0xd2ac2893, 0xb39b, 0x11d1, 0x87, 0x4, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd};
EXTERN_C const GUID DECLSPEC_SELECTANY GUID_DMUS_PROP_Volume = { 0xfedfae25L, 0xe46e, 0x11d1, 0xaa, 0xce, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12, };
EXTERN_C const GUID DECLSPEC_SELECTANY GUID_DMUS_PROP_Effects = { 0xcda8d611, 0x684a, 0x11d2, 0x87, 0x1e, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd, };
EXTERN_C const GUID DECLSPEC_SELECTANY GUID_DMUS_PROP_WriteLatency = { 0x268a0fa0, 0x60f2, 0x11d2, 0xaf, 0xa6, 0x0, 0xaa, 0x0, 0x24, 0xd8, 0xb6, };
EXTERN_C const GUID DECLSPEC_SELECTANY GUID_DMUS_PROP_WavesReverb = { 0x4cb5622, 0x32e5, 0x11d2, 0xaf, 0xa6, 0x0, 0xaa, 0x0, 0x24, 0xd8, 0xb6, };

#ifdef _DEBUG
#define XAMIDIAlloc(size) calloc(size, 1)
#define XAMIDIRealloc(block, size) _recalloc(block, size, 1)
#define XAMIDIFree(block) free(block)
#else
#define XAMIDIAlloc(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size)
#define XAMIDIRealloc(block, size) HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, block, size)
#define XAMIDIFree(block) HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, block)
#endif // _DEBUG

#include "XADSound.hpp"
#include "XADMusic.hpp"
