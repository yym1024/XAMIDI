
namespace $XAudio2MIDI
{
    namespace $DSound
    {
        using namespace $DMusic;

        static const DWORD XADSCAPS = DSCAPS_CONTINUOUSRATE | DSCAPS_SECONDARYMONO | DSCAPS_SECONDARYSTEREO | DSCAPS_SECONDARY16BIT;
        static const DWORD XADSBCAPS = DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS | DSBCAPS_TRUEPLAYPOSITION;

        STDMETHODIMP CXAMDirectSound::QueryInterface(REFIID riid, LPVOID * ppvObject)
        {
            if (__uuidof(IUnknown) == riid)
                *ppvObject = static_cast<IUnknown*>(this);
            else if (__uuidof(IDirectSound) == riid)
                *ppvObject = static_cast<IDirectSound*>(this);
            else if (__uuidof(IDirectSound8) == riid)
                *ppvObject = static_cast<IDirectSound8*>(this);
            else
                return E_NOINTERFACE;
            _InterlockedIncrement(&refCount);
            return S_OK;
        }

        STDMETHODIMP_(ULONG) CXAMDirectSound::AddRef(void)
        {
            return _InterlockedIncrement(&refCount);
        }

        STDMETHODIMP_(ULONG) CXAMDirectSound::Release(void)
        {
            ULONG rc = _InterlockedDecrement(&refCount);
            if (!rc)
            {
                XAMIDIFree(this);
            }
            return rc;
        }

        STDMETHODIMP CXAMDirectSound::CreateSoundBuffer(LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER * ppDSBuffer, __null LPUNKNOWN pUnkOuter)
        {
            if (!pcDSBufferDesc || !ppDSBuffer || !pcDSBufferDesc->lpwfxFormat) return DSERR_INVALIDPARAM;
            if (pcDSBufferDesc->dwFlags &~XADSBCAPS) return DSERR_INVALIDPARAM;
            return DSERR_UNSUPPORTED;
        }

        STDMETHODIMP CXAMDirectSound::GetCaps(LPDSCAPS pDSCaps)
        {
            pDSCaps->dwFlags = XADSCAPS;
            pDSCaps->dwMinSecondarySampleRate = DSBFREQUENCY_MIN;
            pDSCaps->dwMaxSecondarySampleRate = DSBFREQUENCY_MAX;
            pDSCaps->dwPrimaryBuffers = 1;
            pDSCaps->dwMaxHwMixingAllBuffers = 1;
            pDSCaps->dwMaxHwMixingStaticBuffers = 1;
            pDSCaps->dwMaxHwMixingStreamingBuffers = 1;
            ZeroMemory(&pDSCaps->dwFreeHwMixingAllBuffers, pDSCaps->dwSize - offsetof(DSCAPS, dwFreeHwMixingAllBuffers));
            return S_OK;
        }

        STDMETHODIMP CXAMDirectSound::DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER * ppDSBufferDuplicate)
        {
            return DSERR_UNSUPPORTED;
        }

        STDMETHODIMP CXAMDirectSound::SetCooperativeLevel(HWND hwnd, DWORD dwLevel)
        {
            return S_FALSE;
        }

        STDMETHODIMP CXAMDirectSound::Compact(void)
        {
            return S_FALSE;
        }

        STDMETHODIMP CXAMDirectSound::GetSpeakerConfig(LPDWORD pdwSpeakerConfig)
        {
            return DSERR_UNSUPPORTED;
        }

        STDMETHODIMP CXAMDirectSound::SetSpeakerConfig(DWORD dwSpeakerConfig)
        {
            return DSERR_UNSUPPORTED;
        }

        STDMETHODIMP CXAMDirectSound::Initialize(LPCGUID pcGuidDevice)
        {
            return DSERR_ALREADYINITIALIZED;
        }

        STDMETHODIMP CXAMDirectSound::VerifyCertification(LPDWORD pdwCertified)
        {
            return DSERR_UNSUPPORTED;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::QueryInterface(REFIID riid, LPVOID * ppvObject)
        {
            if (__uuidof(IUnknown) == riid)
                *ppvObject = static_cast<IUnknown*>(this);
            else if (__uuidof(IDirectSoundBuffer) == riid)
                *ppvObject = static_cast<IDirectSoundBuffer*>(this);
            else if (__uuidof(IDirectSoundBuffer8) == riid)
                *ppvObject = static_cast<IDirectSoundBuffer8*>(this);
            else
                return E_NOINTERFACE;
            _InterlockedIncrement(&refCount);
            return S_OK;
        }

        STDMETHODIMP_(ULONG) CXAMDirectSoundBuffer::AddRef(void)
        {
            return _InterlockedIncrement(&refCount);
        }

        STDMETHODIMP_(ULONG) CXAMDirectSoundBuffer::Release(void)
        {
            ULONG rc = _InterlockedDecrement(&refCount);
            if (!rc)
            {
                if (parent) parent->CXAMDirectSound::Release();
                XAMIDIFree(this);
            }
            return rc;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::GetCaps(LPDSBCAPS pDSBufferCaps)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            if (!pDSBufferCaps || pDSBufferCaps->dwSize < sizeof(*pDSBufferCaps)) return DSERR_INVALIDPARAM;
            pDSBufferCaps->dwFlags = XADSBCAPS;
            pDSBufferCaps->dwBufferBytes = buflen * bufalign;
            pDSBufferCaps->dwUnlockTransferRate = 0;
            pDSBufferCaps->dwPlayCpuOverhead = 0;
            return S_OK;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::GetCurrentPosition(LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            XAMIDI_SYNTH_ONSTATE state;
            pCallback->OnGetState(&state);
            DWORD spos = DWORD((state.SamplesPlayed) % buflen);
            if (pdwCurrentPlayCursor) *pdwCurrentPlayCursor = spos * bufalign;
            if (!pdwCurrentWriteCursor) return S_OK;
            //if ((status & DSBSTATUS_PLAYING) && (spos += stmlen) >= buflen) spos -= buflen;
            *pdwCurrentWriteCursor = spos * bufalign;
            return S_OK;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::GetFormat(LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            if (!pwfxFormat || dwSizeAllocated < sizeof(WAVEFORMAT)) return DSERR_INVALIDPARAM;
            pCallback->OnVoiceDetails(&pwfxFormat->nChannels, PUINT32(&pwfxFormat->nSamplesPerSec));
            pwfxFormat->wFormatTag = WAVE_FORMAT_PCM;
            pwfxFormat->nBlockAlign = pwfxFormat->nChannels << 1;
            pwfxFormat->nAvgBytesPerSec = pwfxFormat->nBlockAlign * pwfxFormat->nSamplesPerSec;
            if (dwSizeAllocated >= sizeof(WAVEFORMATEX))
            {
                pwfxFormat->wBitsPerSample = 16;
                pwfxFormat->cbSize = 0;
                if (pdwSizeWritten) *pdwSizeWritten = sizeof(WAVEFORMATEX);
            }
            else if (dwSizeAllocated >= sizeof(PCMWAVEFORMAT))
            {
                pwfxFormat->wBitsPerSample = 16;
                if (pdwSizeWritten) *pdwSizeWritten = sizeof(PCMWAVEFORMAT);
            }
            else if (pdwSizeWritten) *pdwSizeWritten = sizeof(WAVEFORMAT);
            return S_OK;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::GetVolume(LPLONG plVolume)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            if (!plVolume) return DSERR_INVALIDPARAM;
            *plVolume = DSBVOLUME_MAX;
            return S_OK;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::GetPan(LPLONG plPan)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            if (!plPan) return DSERR_INVALIDPARAM;
            *plPan = DSBPAN_CENTER;
            return S_OK;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::GetFrequency(LPDWORD pdwFrequency)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            if (!pdwFrequency) return DSERR_INVALIDPARAM;
            *pdwFrequency = DSBFREQUENCY_ORIGINAL;
            return S_OK;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::GetStatus(LPDWORD pdwStatus)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            if (!pdwStatus) return DSERR_INVALIDPARAM;
            *pdwStatus = status | DSBSTATUS_LOCSOFTWARE;
            return S_OK;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::Initialize(LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC pcDSBufferDesc)
        {
            if (parent && pCallback) return DSERR_ALREADYINITIALIZED;
            return DSERR_UNSUPPORTED;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::Lock(DWORD dwOffset, DWORD dwBytes, LPVOID * ppvAudioPtr1, LPDWORD pdwAudioBytes1, LPVOID * ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            *ppvAudioPtr1 = LPBYTE(this + 1) + dwOffset;
            DWORD len = (buflen * bufalign) - dwOffset;
            if (dwBytes <= len)
            {
                *pdwAudioBytes1 = dwBytes;
                if (ppvAudioPtr2) *ppvAudioPtr2 = NULL;
                if (pdwAudioBytes2) *pdwAudioBytes2 = 0;
            }
            else
            {
                *pdwAudioBytes1 = len;
                if (ppvAudioPtr2) *ppvAudioPtr2 = this + 1;
                if (pdwAudioBytes2) *pdwAudioBytes2 = dwBytes - len;
            }
            return S_OK;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::Play(DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            HRESULT hr = pCallback->OnVoiceMethod(XAMIDI_VoiceStart);
            if SUCCEEDED(hr) status |= DSBSTATUS_PLAYING | ((dwFlags & DSBPLAY_LOOPING) ? DSBSTATUS_LOOPING : 0);
            return hr;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::SetCurrentPosition(DWORD dwNewPosition)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            return DSERR_UNSUPPORTED;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::SetFormat(LPCWAVEFORMATEX pcfxFormat)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            return DSERR_UNSUPPORTED;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::SetVolume(LONG lVolume)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            return DSERR_UNSUPPORTED;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::SetPan(LONG lPan)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            return DSERR_UNSUPPORTED;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::SetFrequency(DWORD dwFrequency)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            return DSERR_UNSUPPORTED;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::Stop(void)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            HRESULT hr = pCallback->OnVoiceMethod(XAMIDI_VoiceStop);
            if SUCCEEDED(hr) status &= ~DSBSTATUS_PLAYING;
            return hr;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::Unlock(LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            return S_FALSE;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::Restore(void)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            return S_FALSE;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::SetFX(DWORD dwEffectsCount, LPDSEFFECTDESC pDSFXDesc, LPDWORD pdwResultCodes)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            return DSERR_UNSUPPORTED;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::AcquireResources(DWORD dwFlags, DWORD dwEffectsCount, LPDWORD pdwResultCodes)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            return DSERR_UNSUPPORTED;
        }

        STDMETHODIMP CXAMDirectSoundBuffer::GetObjectInPath(REFGUID rguidObject, DWORD dwIndex, REFGUID rguidInterface, LPVOID * ppObject)
        {
            if (!parent || !pCallback) return DSERR_UNINITIALIZED;
            return DSERR_UNSUPPORTED;
        }

        static STDMETHODIMP CXAMCreateDS(CXAMDirectSound **ppXADS)
        {
            CXAMDirectSound* ret;
            ret = (CXAMDirectSound*)XAMIDIAlloc(sizeof(CXAMDirectSound));
            if (!ret) return E_OUTOFMEMORY;

            __try {
                ret->CXAMDirectSound::CXAMDirectSound(*ret);
                *ppXADS = ret;
                ret = NULL;
            }
            __finally {
                if (ret) ret->CXAMDirectSound::Release();
            }
            return S_OK;
        }

        static STDMETHODIMP CXAMCreateDSB(CXAMDirectSoundBuffer ** ppXADSB, CXAMDirectSound * pXADS, IXAMIDISynthCallback *pCallback, LPWAVEFORMATEX Format, DWORD BufferSize)
        {
            HRESULT hr = S_OK;
            CXAMDirectSoundBuffer *ret = (CXAMDirectSoundBuffer*)XAMIDIAlloc(sizeof(CXAMDirectSoundBuffer) + BufferSize);
            if (!ret) return E_OUTOFMEMORY;

            __try {
                ret->CXAMDirectSoundBuffer::CXAMDirectSoundBuffer(*ret);

                ret->pCallback = pCallback;
                ret->parent = pXADS;
                _InterlockedIncrement(&pXADS->refCount);

                ret->buflen = BufferSize / Format->nBlockAlign;
                ret->bufalign = Format->nBlockAlign;
                *ppXADSB = ret;
                ret = NULL;
            }
            __finally {
                if (ret) ret->CXAMDirectSoundBuffer::Release();
            }
            return hr;
        }
    }
}
