#pragma once

interface IXAudio2;
interface IXAudio2Voice;
interface IXAudio2SourceVoice;

namespace $XAudio2MIDI
{
    namespace $DMusic
    {
        class CXAMEngine;
        class CXAMSynthesizer;
        class CXAMTimbreBanks;
        class CXAMTimbreDownloaded;
    }

    namespace $DSound
    {
        class CXAMDirectSound : public IDirectSound8
        {
            friend class CXAMDirectSoundBuffer;
            friend class $DMusic::CXAMEngine;
            friend STDMETHODIMP CXAMCreateDS(CXAMDirectSound **ppXADS);
            friend STDMETHODIMP CXAMCreateDSB(CXAMDirectSoundBuffer **ppXADSB, CXAMDirectSound *pXADS, IXAMIDISynthCallback *pCallback, LPWAVEFORMATEX Format, DWORD BufferSize);

        public:
            // IUnknown methods
            STDMETHODIMP QueryInterface(THIS_ __in REFIID riid, __deref_out LPVOID*ppvObject) OVERRIDE;
            STDMETHODIMP_(ULONG) AddRef(THIS) OVERRIDE;
            STDMETHODIMP_(ULONG) Release(THIS) OVERRIDE;

            // IDirectSound methods
            STDMETHODIMP CreateSoundBuffer(THIS_ __in LPCDSBUFFERDESC pcDSBufferDesc, __out LPDIRECTSOUNDBUFFER *ppDSBuffer, __null LPUNKNOWN pUnkOuter) OVERRIDE;
            STDMETHODIMP GetCaps(THIS_ __out LPDSCAPS pDSCaps) OVERRIDE;
            STDMETHODIMP DuplicateSoundBuffer(THIS_ __in LPDIRECTSOUNDBUFFER pDSBufferOriginal, __out LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate) OVERRIDE;
            STDMETHODIMP SetCooperativeLevel(THIS_ HWND hwnd, DWORD dwLevel) OVERRIDE;
            STDMETHODIMP Compact(THIS) OVERRIDE;
            STDMETHODIMP GetSpeakerConfig(THIS_ __out LPDWORD pdwSpeakerConfig) OVERRIDE;
            STDMETHODIMP SetSpeakerConfig(THIS_ DWORD dwSpeakerConfig) OVERRIDE;
            STDMETHODIMP Initialize(THIS_ __in_opt LPCGUID pcGuidDevice) OVERRIDE;

            // IDirectSound8 methods
            STDMETHODIMP VerifyCertification(THIS_ __out LPDWORD pdwCertified) OVERRIDE;

        protected:
            CXAMDirectSound(const CXAMDirectSound &) : refCount(1) {}

            volatile long refCount;
        };

        class CXAMDirectSoundBuffer : public IDirectSoundBuffer8
        {
            friend class CXAMDirectSound;
            friend class $DMusic::CXAMSynthesizer;
            friend STDMETHODIMP CXAMCreateDSB(CXAMDirectSoundBuffer **ppXADSB, CXAMDirectSound *pXADS, IXAMIDISynthCallback *pCallback, LPWAVEFORMATEX Format, DWORD BufferSize);

        public:
            // IUnknown methods
            STDMETHODIMP QueryInterface(THIS_ __in REFIID, __deref_out LPVOID*) OVERRIDE;
            STDMETHODIMP_(ULONG) AddRef(THIS) OVERRIDE;
            STDMETHODIMP_(ULONG) Release(THIS) OVERRIDE;

            // IDirectSoundBuffer methods
            STDMETHODIMP GetCaps(THIS_ __out LPDSBCAPS pDSBufferCaps);
            STDMETHODIMP GetCurrentPosition(THIS_ __out_opt LPDWORD pdwCurrentPlayCursor, __out_opt LPDWORD pdwCurrentWriteCursor) OVERRIDE;
            STDMETHODIMP GetFormat(THIS_ __out_bcount_opt(dwSizeAllocated) LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, __out_opt LPDWORD pdwSizeWritten) OVERRIDE;
            STDMETHODIMP GetVolume(THIS_ __out LPLONG plVolume) OVERRIDE;
            STDMETHODIMP GetPan(THIS_ __out LPLONG plPan) OVERRIDE;
            STDMETHODIMP GetFrequency(THIS_ __out LPDWORD pdwFrequency) OVERRIDE;
            STDMETHODIMP GetStatus(THIS_ __out LPDWORD pdwStatus) OVERRIDE;
            STDMETHODIMP Initialize(THIS_ __in LPDIRECTSOUND pDirectSound, __in LPCDSBUFFERDESC pcDSBufferDesc) OVERRIDE;
            STDMETHODIMP Lock(THIS_ DWORD dwOffset, DWORD dwBytes, __deref_out_bcount(*pdwAudioBytes1) LPVOID *ppvAudioPtr1, __out LPDWORD pdwAudioBytes1, __deref_opt_out_bcount(*pdwAudioBytes2) LPVOID *ppvAudioPtr2, __out_opt LPDWORD pdwAudioBytes2, DWORD dwFlags) OVERRIDE;
            STDMETHODIMP Play(THIS_ DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags) OVERRIDE;
            STDMETHODIMP SetCurrentPosition(THIS_ DWORD dwNewPosition) OVERRIDE;
            STDMETHODIMP SetFormat(THIS_ __in LPCWAVEFORMATEX pcfxFormat) OVERRIDE;
            STDMETHODIMP SetVolume(THIS_ LONG lVolume) OVERRIDE;
            STDMETHODIMP SetPan(THIS_ LONG lPan) OVERRIDE;
            STDMETHODIMP SetFrequency(THIS_ DWORD dwFrequency) OVERRIDE;
            STDMETHODIMP Stop(THIS) OVERRIDE;
            STDMETHODIMP Unlock(THIS_ __in_bcount(dwAudioBytes1) LPVOID pvAudioPtr1, DWORD dwAudioBytes1, __in_bcount_opt(dwAudioBytes2) LPVOID pvAudioPtr2, DWORD dwAudioBytes2) OVERRIDE;
            STDMETHODIMP Restore(THIS) OVERRIDE;

            // IDirectSoundBuffer8 methods
            STDMETHODIMP SetFX(THIS_ DWORD dwEffectsCount, __in_ecount_opt(dwEffectsCount) LPDSEFFECTDESC pDSFXDesc, __out_ecount_opt(dwEffectsCount) LPDWORD pdwResultCodes) OVERRIDE;
            STDMETHODIMP AcquireResources(THIS_ DWORD dwFlags, DWORD dwEffectsCount, __out_ecount(dwEffectsCount) LPDWORD pdwResultCodes) OVERRIDE;
            STDMETHODIMP GetObjectInPath(THIS_ __in REFGUID rguidObject, DWORD dwIndex, __in REFGUID rguidInterface, __deref_out LPVOID *ppObject) OVERRIDE;

        protected:
            CXAMDirectSoundBuffer(const CXAMDirectSoundBuffer &) : status(DSBSTATUS_BUFFERLOST), stmlen(1), refCount(1) {}

            CXAMDirectSound *parent;
            IXAMIDISynthCallback *pCallback;
            DWORD status, buflen;
            WORD bufalign, stmlen;
            volatile long refCount;
        };
    }
}
