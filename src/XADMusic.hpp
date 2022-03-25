
namespace $XAudio2MIDI
{
    namespace $DMusic
    {
        using namespace $DSound;

        STDMETHODIMP CXAMEngine::QueryInterface(REFIID riid, void ** ppvInterface)
        {
            if (__uuidof(IUnknown) == riid)
                *ppvInterface = static_cast<IUnknown*>(this);
            else if (IID_IXAMIDI == riid)
                *ppvInterface = static_cast<IXAMIDI*>(this);
            else
                return E_NOINTERFACE;
            _InterlockedIncrement(&refCount);
            return S_OK;
        }

        STDMETHODIMP_(ULONG) CXAMEngine::AddRef(void)
        {
            return _InterlockedIncrement(&refCount);
        }

        STDMETHODIMP_(ULONG) CXAMEngine::Release(void)
        {
            ULONG rc = _InterlockedDecrement(&refCount);
            if (!rc)
            {
                if (pDMCol) pDMCol->Release();
                if (pDMBuf) pDMBuf->Release();
                if (pDMLdr) pDMLdr->Release();
                if (pDMEng) pDMEng->Release();
                if (pDSDev) pDSDev->CXAMDirectSound::Release();
                XAMIDIFree(this);
            }
            return rc;
        }

        STDMETHODIMP CXAMEngine::GetEngineDetails(XAMIDI_ENGINE_DETAILS * pEngineDetails)
        {
            HRESULT hr;
            if (!pEngineDetails) return XAMIDI_E_INVALID_CALL;
            DWORD i = 0;
            DMUS_PORTCAPS caps;
            caps.dwSize = sizeof(caps);
            while (S_OK == (hr = pDMEng->EnumPort(i++, &caps)))
            {
                if (__uuidof(CDirectMusicSynth) != caps.guidPort)
                    continue;
                pEngineDetails->MemorySize = caps.dwMemorySize;
                pEngineDetails->MaxOutputChannels = caps.dwMaxAudioChannels;
                pEngineDetails->MaxVoiceCount = caps.dwMaxVoices;
                pEngineDetails->MaxGroupCount = caps.dwMaxChannelGroups;
            }
            return hr > S_OK ? E_NOT_SET : hr;
        }

        STDMETHODIMP CXAMEngine::LoadTimbreBanks(IXAMIDITimbreBanks ** ppTimbreBank, XAMIDI_LOAD_OPTION Option, const void *Details)
        {
            typedef IDirectMusicInstrument *PDMI, **PPDMIS;
            if (!ppTimbreBank || (Option && !Details)) return XAMIDI_E_INVALID_CALL;
            DWORD patch, i = 0x1000U, min = 0U, max = 0x400000U, size = 1 << 8;
            HRESULT hr = S_OK;
            IDirectMusicCollection *pDMCol = NULL;
            CXAMTimbreBanks* ret = NULL;
            void* pContext = NULL;
            XAMIDI_FREE_MEMORY *pFreeFun = NULL;
            DMUS_OBJECTDESC objdesc = { sizeof(objdesc), DMUS_OBJ_CLASS, {}, __uuidof(CDirectMusicCollection), };

            __try {

                switch (Option)
                {
                case XAMIDI_DEFAULT:
                    (pDMCol = this->pDMCol)->AddRef();
                    goto Loaded;
                case XAMIDI_FILEPATH:
                    objdesc.dwValidData |= DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH;
                    GetFullPathNameW(LPCWSTR(Details), DMUS_MAX_FILENAME, objdesc.wszFileName, NULL);
                    break;
                case XAMIDI_URLPATH:
                    objdesc.dwValidData |= DMUS_OBJ_URL;
                    lstrcpynW(objdesc.wszFileName, LPCWSTR(Details), DMUS_MAX_FILENAME);
                    break;
                case XAMIDI_MEMORY:
                {
                    const XAMIDI_LOAD_MEMORY*const pLoadMem = static_cast<const XAMIDI_LOAD_MEMORY*>(Details);
                    objdesc.dwValidData |= DMUS_OBJ_MEMORY;
                    objdesc.pbMemData = LPBYTE(pLoadMem->pMemData);
                    objdesc.llMemLength = pLoadMem->MemBytes;
                    pContext = pLoadMem->pContext;
                    pFreeFun = pLoadMem->pFreeFun;
                    break;
                }
                case XAMIDI_STREAM:
                    objdesc.dwValidData |= DMUS_OBJ_STREAM;
                    objdesc.pStream = static_cast<IStream*>(const_cast<void*>(Details));
                    break;
                default:
                    break;
                }

                hr = pDMLdr->GetObject(&objdesc, IID_PPV_ARGS(&pDMCol));
                if FAILED(hr) __leave;
Loaded:
                for (;;) {
                    hr = pDMCol->EnumInstrument(i, &patch, NULL, 0);
                    if (S_OK == hr)
                    {
                        if (i++ >= max) break;
                        min = i;
                    }
                    else if (S_OK < hr)
                    {
                        if (i <= min) break;
                        max = --i;
                    }
                    else __leave;
                    i = min + ((max - min) >> 1);
                }

                ret = (CXAMTimbreBanks*)XAMIDIAlloc(sizeof(CXAMTimbreBanks) + sizeof(PDMI) * i);
                if (!ret) return E_OUTOFMEMORY;
                ret->CXAMTimbreBanks::CXAMTimbreBanks(*ret);

                PPDMIS ppDMItms = PPDMIS(ret + 1);
                while (S_OK == (hr = pDMCol->EnumInstrument(ret->count, &patch, NULL, 0)))
                {
                    hr = pDMCol->GetInstrument(patch, ppDMItms);
                    if FAILED(hr) __leave;

                    ++ret->count;
                    ++ppDMItms;
                }
                if FAILED(hr) __leave;

                hr = S_OK;
                ret->pDMCol = pDMCol;
                ret->pContext = pContext;
                ret->pFreeFun = pFreeFun;
                pDMCol = NULL;
                pContext = NULL;
                pFreeFun = NULL;
                ret->parent = this;
                _InterlockedIncrement(&refCount);
                *ppTimbreBank = ret;
                ret = NULL;
            }
            __finally {
                if (ret) ret->CXAMTimbreBanks::DestroyBanks();
                if (pDMCol) pDMCol->Release();
                if (pFreeFun) pFreeFun(pContext);
            }
            return hr;
        }

        STDMETHODIMP CXAMEngine::CreateSynthesizer(IXAMIDISynthesizer ** ppSynthesizer, XAMIDI_SYNTH_PARAMETERS * pSynthParams, IXAMIDISynthCallback * pCallback)
        {
            HRESULT hr;
            DWORD wfxsize, buflen;
            DMUS_PORTPARAMS dmpp;
            WAVEFORMATEX wfx;

            if (!ppSynthesizer || !pSynthParams) return XAMIDI_E_INVALID_CALL;
            CXAMSynthesizer* ret = (CXAMSynthesizer*)XAMIDIAlloc(sizeof(CXAMSynthesizer));
            if (!ret) return E_OUTOFMEMORY;

            __try {
                ret->CXAMSynthesizer::CXAMSynthesizer(*ret);

                dmpp.dwSize = sizeof(dmpp);
                dmpp.dwValidParams = DMUS_PORTPARAMS_EFFECTS;
                dmpp.dwEffectFlags = pSynthParams->Effects;

                if (pSynthParams->OutputChannels)
                {
                    dmpp.dwValidParams |= DMUS_PORTPARAMS_AUDIOCHANNELS;
                    dmpp.dwAudioChannels = pSynthParams->OutputChannels;
                }

                if (pSynthParams->OutputSampleRate)
                {
                    dmpp.dwValidParams |= DMUS_PORTPARAMS_SAMPLERATE;
                    dmpp.dwSampleRate = pSynthParams->OutputSampleRate;
                }

                if (pSynthParams->VoiceCount)
                {
                    dmpp.dwValidParams |= DMUS_PORTPARAMS_VOICES;
                    dmpp.dwVoices = pSynthParams->VoiceCount;
                }

                if (pSynthParams->GroupCount)
                {
                    dmpp.dwValidParams |= DMUS_PORTPARAMS_CHANNELGROUPS;
                    dmpp.dwChannelGroups = pSynthParams->GroupCount;
                }

                hr = pDMEng->CreatePort(__uuidof(CDirectMusicSynth), &dmpp, &ret->pDMPrt, NULL);
                if FAILED(hr) __leave;

                hr = ret->pDMPrt->QueryInterface(IID_PPV_ARGS(&ret->pKSCtl));
                if FAILED(hr) __leave;

                hr = ret->pDMPrt->GetLatencyClock(&ret->pRefClock);
                if FAILED(hr) __leave;

                wfxsize = sizeof(wfx);
                hr = ret->pDMPrt->GetFormat(&wfx, &wfxsize, &buflen);
                if FAILED(hr) __leave;

                hr = CXAMCreateDSB(&ret->pDSBuf, pDSDev, pCallback, &wfx, buflen);
                if FAILED(hr) __leave;

                hr = ret->pDMPrt->SetDirectSound(pDSDev, ret->pDSBuf);
                if FAILED(hr) __leave;

                pSynthParams->OutputChannels = dmpp.dwAudioChannels;
                pSynthParams->OutputSampleRate = dmpp.dwSampleRate;
                pSynthParams->VoiceCount = dmpp.dwVoices;
                pSynthParams->GroupCount = dmpp.dwChannelGroups;

                ret->VoiceCount = dmpp.dwVoices;
                ret->parent = this;
                _InterlockedIncrement(&refCount);

                *ppSynthesizer = ret;
                ret = NULL;
            }
            __finally {
                if (ret) ret->CXAMSynthesizer::DestroySynth();
            }
            return hr;
        }

        STDMETHODIMP CXAMEngine::StartEngine(void)
        {
            return pDMEng ? pDMEng->Activate(TRUE) : XAMIDI_E_INVALID_CALL;
        }
        STDMETHODIMP_(void) CXAMEngine::StopEngine(void)
        {
            if (pDMEng) pDMEng->Activate(FALSE);
        }

        STDMETHODIMP CXAMEngine::CommitChanges(UINT32 OperationSet)
        {
            if (XAMIDI_INVALID_OPSET == OperationSet) return XAMIDI_E_INVALID_CALL;
            return S_FALSE;
        }

        STDMETHODIMP_(void) CXAMSynthesizer::DestroySynth(void)
        {
            if (pRefClock) pRefClock->Release();
            if (pKSCtl) pKSCtl->Release();
            if (pDMPrt)
            {
                if (pDSBuf)
                {
                    if (pDSBuf->status & DSBSTATUS_PLAYING)
                        pDMPrt->Activate(FALSE);
                    if (!(pDSBuf->status & DSBSTATUS_BUFFERLOST))
                        pDSBuf->pCallback->OnVoiceMethod(XAMIDI_VoiceFlush);
                    pDSBuf->status |= DSBSTATUS_BUFFERLOST;
                }
                pDMPrt->Release();
            }
            if (pDSBuf) pDSBuf->CXAMDirectSoundBuffer::Release();
            if (parent) parent->CXAMEngine::Release();
            XAMIDIFree(this);
        }

        STDMETHODIMP CXAMSynthesizer::GetSynthDetails(XAMIDI_SYNTH_DETAILS * pSynthDetails)
        {
            DWORD size = sizeof(WAVEFORMATEXTENSIBLE);
            HRESULT hr = pDMPrt->GetFormat(&pSynthDetails->OutputFormat.Format, &size, LPDWORD(&pSynthDetails->BufferSize));
            if SUCCEEDED(hr)
            {
                pSynthDetails->VoiceCount = VoiceCount;
                ZeroMemory(LPBYTE(&pSynthDetails->OutputFormat) + size, sizeof(WAVEFORMATEXTENSIBLE) - size);
            }
            return hr;
        }

        STDMETHODIMP_(const BYTE *) CXAMSynthesizer::GetBufferData(void)
        {
            return LPCBYTE(pDSBuf + 1);
        }

        STDMETHODIMP_(UINT32) CXAMSynthesizer::GetGroupCount(void)
        {
            DWORD Groups = 0;
            if (pDMPrt) pDMPrt->GetNumChannelGroups(&Groups);
            return Groups;
        }

        STDMETHODIMP CXAMSynthesizer::SetGroupCount(UINT32 Groups)
        {
            if (Groups > XAMIDI_MAX_GROUPS) return XAMIDI_E_INVALID_CALL;
            return pDMPrt->SetNumChannelGroups(Groups);
        }

        STDMETHODIMP CXAMSynthesizer::DownloadTimbre(IXAMIDITimbreDownloaded** ppDownloaded, UINT32 Patch, HXAMIDITimbre hTimbre, UINT32 OperationSet)
        {
            HRESULT hr;

            CXAMTimbreDownloaded* ret = (CXAMTimbreDownloaded*)XAMIDIAlloc(sizeof(CXAMTimbreDownloaded));
            if (!ret) return E_OUTOFMEMORY;

            __try {
                ret->CXAMTimbreDownloaded::CXAMTimbreDownloaded(*ret);

                hr = LPUNKNOWN(hTimbre)->QueryInterface(&ret->pDMInstrument);
                if FAILED(hr) __leave;

                hr = ret->pDMInstrument->SetPatch(Patch);
                if FAILED(hr) __leave;

                hr = pDMPrt->DownloadInstrument(ret->pDMInstrument, &ret->pDMDownloaded, NULL, 0);
                if FAILED(hr) __leave;

                ret->pDMPrt = pDMPrt;
                pDMPrt->AddRef();

                ret->patch = Patch;
                *ppDownloaded = ret;
                ret = NULL;
            }
            __finally {
                if (ret) ret->CXAMTimbreDownloaded::UnloadTimbre();
            }
            return hr;
        }

        STDMETHODIMP_(BOOL) CXAMSynthesizer::GetEnabled(void)
        {
            return (pDSBuf->status & DSBSTATUS_PLAYING);
        }

        STDMETHODIMP CXAMSynthesizer::SetEnabled(BOOL Active)
        {
            HRESULT hr;
            if (Active && (pDSBuf->status & DSBSTATUS_BUFFERLOST))
            {
                DWORD offset = 0;
                for (int i = 0; i < ARRAYSIZE(onbufs); ++i)
                {
                    XAMIDI_SYNTH_ONBUFFER &cur = onbufs[i];
                    cur.SamplesOffset = offset;
                    cur.AudioBytes = pDSBuf->buflen / ARRAYSIZE(onbufs) * pDSBuf->bufalign;
                    offset += cur.AudioBytes;
                    hr = pDSBuf->pCallback->OnSubmitBuffer(&cur);
                    if SUCCEEDED(hr) continue;
                    pDSBuf->pCallback->OnVoiceMethod(XAMIDI_VoiceFlush);
                    return hr;
                }
                pDSBuf->status &= ~DSBSTATUS_BUFFERLOST;
            }
            return pDMPrt->Activate(Active);
        }

        STDMETHODIMP_(float) CXAMSynthesizer::GetVolume(void)
        {
            return volume;
        }

        STDMETHODIMP CXAMSynthesizer::SetVolume(float Volume)
        {
            ULONG retlen;
            KSPROPERTY ksp;
            ksp.Id = 0;
            ksp.Set = GUID_DMUS_PROP_Volume;
            ksp.Flags = KSPROPERTY_TYPE_SET;
            LONG lvolmb = lrintf(2000.f * log10f(Volume));
            HRESULT hr = pKSCtl->KsProperty(&ksp, sizeof(ksp), &lvolmb, sizeof(lvolmb), &retlen);
            if SUCCEEDED(hr) volume = Volume;
            return hr;
        }

        STDMETHODIMP_(UINT32) CXAMSynthesizer::GetEffects(void)
        {
            ULONG retlen;
            UINT32 Effects;
            KSPROPERTY ksp;
            ksp.Id = 0;
            ksp.Set = GUID_DMUS_PROP_Effects;
            ksp.Flags = KSPROPERTY_TYPE_GET;
            HRESULT hr = pKSCtl->KsProperty(&ksp, sizeof(ksp), &Effects, sizeof(Effects), &retlen);
            return Effects;
        }

        STDMETHODIMP CXAMSynthesizer::SetEffects(UINT32 Effects)
        {
            ULONG retlen;
            KSPROPERTY ksp;
            ksp.Id = 0;
            ksp.Set = GUID_DMUS_PROP_Effects;
            ksp.Flags = KSPROPERTY_TYPE_SET;
            return pKSCtl->KsProperty(&ksp, sizeof(ksp), &Effects, sizeof(Effects), &retlen);
        }

        STDMETHODIMP_(UINT32) CXAMSynthesizer::GetWriteLatency(void)
        {
            ULONG retlen;
            UINT32 Milliseconds;
            KSPROPERTY ksp;
            ksp.Id = 0;
            ksp.Set = GUID_DMUS_PROP_WriteLatency;
            ksp.Flags = KSPROPERTY_TYPE_GET;
            HRESULT hr = pKSCtl->KsProperty(&ksp, sizeof(ksp), &Milliseconds, sizeof(Milliseconds), &retlen);
            return Milliseconds;
        }

        STDMETHODIMP CXAMSynthesizer::SetWriteLatency(UINT32 Milliseconds)
        {
            ULONG retlen;
            KSPROPERTY ksp;
            ksp.Id = 0;
            ksp.Set = GUID_DMUS_PROP_WriteLatency;
            ksp.Flags = KSPROPERTY_TYPE_SET;
            return pKSCtl->KsProperty(&ksp, sizeof(ksp), &Milliseconds, sizeof(Milliseconds), &retlen);
        }

        STDMETHODIMP CXAMSynthesizer::GetWavesReverb(XAMIDI_SYNTH_WAVESREVERB * pWavesReverb)
        {
            ULONG retlen;
            KSPROPERTY ksp;
            ksp.Id = 0;
            ksp.Set = GUID_DMUS_PROP_WavesReverb;
            ksp.Flags = KSPROPERTY_TYPE_GET;
            return pKSCtl->KsProperty(&ksp, sizeof(ksp), pWavesReverb, sizeof(*pWavesReverb), &retlen);
        }

        STDMETHODIMP CXAMSynthesizer::SetWavesReverb(const XAMIDI_SYNTH_WAVESREVERB * pWavesReverb)
        {
            ULONG retlen;
            KSPROPERTY ksp;
            ksp.Id = 0;
            ksp.Set = GUID_DMUS_PROP_WavesReverb;
            ksp.Flags = KSPROPERTY_TYPE_SET;
            return pKSCtl->KsProperty(&ksp, sizeof(ksp), LPVOID(pWavesReverb), sizeof(*pWavesReverb), &retlen);
        }

        STDMETHODIMP_(void) CXAMSynthesizer::OnVoiceCallback(XAMIDI_VOICE_CALLBACK Callback, void *pParam)
        {
            switch (Callback)
            {
            case XAMIDI_OnVoiceProcessingPassStart:
                if (pParam && *PUINT32(pParam))
                    pDSBuf->stmlen = *PUINT32(pParam);
                break;
            case XAMIDI_OnVoiceProcessingPassEnd:
                break;
            case XAMIDI_OnStreamEnd:
                break;
            case XAMIDI_OnBufferStart:
                break;
            case XAMIDI_OnBufferEnd:
                pDSBuf->pCallback->OnSubmitBuffer(static_cast<const XAMIDI_SYNTH_ONBUFFER*>(pParam));
                break;
            case XAMIDI_OnLoopEnd:
                break;
            }
        }

        STDMETHODIMP_(IReferenceClock *) CXAMSynthesizer::GetReferenceClock(void)
        {
            return pRefClock;
        }

        STDMETHODIMP CXAMSynthesizer::OutShortMsg(UINT32 MsgCmd, UINT32 Group, REFERENCE_TIME PlayTime, UINT32 OperationSet)
        {
            if (MsgCmd > 0x00FFFFFFU || XAMIDI_INVALID_OPSET == OperationSet) return XAMIDI_E_INVALID_CALL;

            HRESULT hr = parent->pDMBuf->Flush();
            if FAILED(hr) return hr;

            hr = parent->pDMBuf->PackStructured(PlayTime, Group, MsgCmd);
            if FAILED(hr) return hr;

            hr = pDMPrt->PlayBuffer(parent->pDMBuf);
            return hr;
        }

        STDMETHODIMP CXAMSynthesizer::OutLongMsg(const void* BytesData, UINT32 Length, UINT32 Group, REFERENCE_TIME PlayTime, UINT32 OperationSet)
        {
            if (!BytesData || XAMIDI_INVALID_OPSET == OperationSet) return XAMIDI_E_INVALID_CALL;

            HRESULT hr = parent->pDMBuf->Flush();
            if FAILED(hr) return hr;

            hr = parent->pDMBuf->PackUnstructured(PlayTime, Group, Length, LPBYTE(BytesData));
            if FAILED(hr) return hr;

            hr = pDMPrt->PlayBuffer(parent->pDMBuf);
            return hr;
        }

        STDMETHODIMP_(void) CXAMTimbreBanks::DestroyBanks(void)
        {
            typedef IDirectMusicInstrument *PDMI, **PPDMIS;
            PPDMIS ppDMItms = PPDMIS(this + 1);
            while (count)
            {
                PDMI pDMItm = ppDMItms[--count];
                if (pDMItm) pDMItm->Release();
            }
            if (pDMCol) pDMCol->Release();
            if (parent) parent->CXAMEngine::Release();
            XAMIDIFree(this);
        }

        STDMETHODIMP_(UINT32) CXAMTimbreBanks::GetTimbreCount(void)
        {
            return count;
        }

        STDMETHODIMP CXAMTimbreBanks::GetTimbreDetails(UINT32 Index, XAMIDI_TIMBRE_DETAILS * pTimbreDetails)
        {
            if (!pTimbreDetails || Index >= count) return XAMIDI_E_INVALID_CALL;

            LPWSTR Name = pTimbreDetails->Name;
            UINT32 &len = pTimbreDetails->NameLen;
            if (!Name) len = 0U;
            HRESULT hr = pDMCol->EnumInstrument(Index, PDWORD(&pTimbreDetails->Patch), Name, len);
            if FAILED(hr) return hr;
            if (len) len = lstrlenW(Name);
            pTimbreDetails->hTimbre = reinterpret_cast<HXAMIDITimbre*>(this + 1)[Index];
            return hr;
        }

        STDMETHODIMP_(HXAMIDITimbre) CXAMTimbreBanks::GetPatchTimbre(UINT32 Patch)
        {
            IDirectMusicInstrument *pDMItm = NULL;
            HRESULT hr = pDMCol->GetInstrument(Patch, &pDMItm);
            if (SUCCEEDED(hr) && pDMItm) pDMItm->Release();
            return HXAMIDITimbre(pDMItm);
        }

        STDMETHODIMP_(HXAMIDITimbre) CXAMTimbreBanks::IndexOfTimbre(UINT32 Index)
        {
            if (Index >= count) return NULL;
            return reinterpret_cast<HXAMIDITimbre*>(this + 1)[Index];
        }

        STDMETHODIMP_(void) CXAMTimbreDownloaded::UnloadTimbre(void)
        {
            if (pDMDownloaded)
            {
                if (pDMPrt) pDMPrt->UnloadInstrument(pDMDownloaded);
                pDMDownloaded->Release();
            }
            if (pDMInstrument) pDMInstrument->Release();
            if (pDMPrt) pDMPrt->Release();
            XAMIDIFree(this);
        }

        STDMETHODIMP_(UINT32) CXAMTimbreDownloaded::GetPatch(void)
        {
            return patch;
        }

        STDMETHODIMP_(HXAMIDITimbre) CXAMTimbreDownloaded::GetTimbre(void)
        {
            return HXAMIDITimbre(pDMInstrument);
        }
    }
}

STDAPI XAMIDICreate(__deref_out IXAMIDI** ppXAMIDI, __in UINT32 Flags)
{
    using namespace $XAudio2MIDI;
    using namespace $DSound;
    using namespace $DMusic;

    HRESULT hr = S_OK;
    CXAMEngine *ret = (CXAMEngine*)XAMIDIAlloc(sizeof(CXAMEngine));
    if (!ret) return E_OUTOFMEMORY;

    __try {
        ret->CXAMEngine::CXAMEngine(*ret);

        hr = CXAMCreateDS(&ret->pDSDev);
        if (FAILED(hr)) __leave;

        hr = CoCreateInstance(__uuidof(CDirectMusic), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ret->pDMEng));
        if (FAILED(hr)) __leave;

        hr = CoCreateInstance(__uuidof(CDirectMusicLoader), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ret->pDMLdr));
        if (FAILED(hr)) __leave;

        hr = ret->pDMEng->SetDirectSound(ret->pDSDev, NULL);
        if (FAILED(hr)) __leave;

        DMUS_BUFFERDESC dmbd = { sizeof(dmbd), 0,{}, 0x100U, };
        hr = ret->pDMEng->CreateMusicBuffer(&dmbd, &ret->pDMBuf, NULL);
        if (FAILED(hr)) __leave;

        hr = ret->pDMLdr->EnableCache(GUID_DirectMusicAllTypes, FALSE);
        if (FAILED(hr)) __leave;
        DMUS_OBJECTDESC objdesc; objdesc.dwSize = sizeof(objdesc);
        hr = ret->pDMLdr->EnumObject(__uuidof(CDirectMusicCollection), 0, &objdesc);
        if FAILED(hr) __leave;
        hr = ret->pDMLdr->GetObject(&objdesc, IID_PPV_ARGS(&ret->pDMCol));
        if FAILED(hr) __leave;

        *ppXAMIDI = ret;
        ret = NULL;
    }
    __finally {
        if (ret) ret->CXAMEngine::Release();
    }
    return hr;
}
