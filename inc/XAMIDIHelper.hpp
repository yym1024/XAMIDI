/**************************************************************************
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * File:    xmidihelper.inl
 * Content: Declarations for the XAudio2 game midi API.
 *
 **************************************************************************/

#ifndef __XAMIDI_HELPER_INCLUDED__
#define __XAMIDI_HELPER_INCLUDED__

#if _MSC_VER > 1000
#pragma once
#endif

//--------------<D-E-F-I-N-I-T-I-O-N-S>-------------------------------------//
#ifndef __XAUDIO2_INCLUDED__
#define XAUDIO2_HELPER_FUNCTIONS
#include "XAudio2.h"
#endif
#ifndef __XAMIDI_INCLUDED__
#define XAMIDI_HELPER_FUNCTIONS
#include "XAMIDI.h"
#endif
#ifdef __cplusplus
#include <algorithm>
#endif


/**************************************************************************
 *
 * prior to #including xmidihelper.inl.
 *
 **************************************************************************/

#ifdef __cplusplus

template <class _base = void>
class CXAMIDISynthVoice : public _base, public CXAMIDISynthVoice<void>
{
    friend typename _base;

public:
    CXAMIDISynthVoice(__in IXAudio2 *pXAudio2, __in IXAMIDI *pXAMIDI) throw() : CXAMIDISynthVoice<void>(pXAudio2, pXAMIDI) {}
};

template <>
class CXAMIDISynthVoice<void> : protected IXAudio2VoiceCallback, protected IXAMIDISynthCallback
{
public: // CXAMIDISynthVoice methods

    FORCEINLINE IXAudio2 *GetXAudio2(void) const throw()
    {
        return _pXAudio;
    }

    FORCEINLINE IXAMIDI *GetXAMIDI(void) const throw()
    {
        return _pXAMIDI;
    }
    FORCEINLINE IXAudio2SourceVoice *GetSourceVoice(void) const throw()
    {
        return _pSrcVoice;
    }

    FORCEINLINE IXAMIDISynthesizer *GetSynthesizer(void) const throw()
    {
        return _pMidSynth;
    }

    FORCEINLINE HRESULT Start(void) throw()
    {
        return _pMidSynth->SetEnabled(TRUE);
    }

    FORCEINLINE HRESULT Stop(void) throw()
    {
        return _pMidSynth->SetEnabled(FALSE);
    }

    HRESULT Initialize(__inout XAMIDI_SYNTH_PARAMETERS *pSynthParams, __in_opt const XAUDIO2_VOICE_SENDS* pSendList X2DEFAULT(NULL), __in_opt const XAUDIO2_EFFECT_CHAIN* pEffectChain X2DEFAULT(NULL)) throw()
    {
        HRESULT hr = S_OK;
        IXAMIDISynthesizer* pMidSynth = NULL;
        IXAudio2SourceVoice* pSrcVoice = NULL;

        if (!pSynthParams) return XAUDIO2_E_INVALID_CALL;
        __try {
            XAMIDI_SYNTH_DETAILS synthDetails;

            hr = _pXAMIDI->CreateSynthesizer(&pMidSynth, pSynthParams, this);
            if (FAILED(hr)) return hr;

            hr = pMidSynth->GetSynthDetails(&synthDetails);
            if (FAILED(hr)) return hr;

            hr = _pXAudio->CreateSourceVoice(&pSrcVoice, &synthDetails.OutputFormat.Format, XAUDIO2_VOICE_NOPITCH, 1.f, this, pSendList, pEffectChain);
            if (FAILED(hr)) return hr;

            std::swap(const_cast<IXAMIDISynthesizer*&>(_pMidSynth), pMidSynth);
            std::swap(const_cast<IXAudio2SourceVoice*&>(_pSrcVoice), pSrcVoice);
        }
        __finally {
            if (pSrcVoice) pSrcVoice->DestroyVoice();
            if (pMidSynth) pMidSynth->DestroySynth();
        }
        return hr;
    }

    HRESULT Initialize(__inout XAMIDI_SYNTH_PARAMETERS *pSynthParams, __in float MaxFrequencyRatio, __in_opt const XAUDIO2_VOICE_SENDS* pSendList X2DEFAULT(NULL), __in_opt const XAUDIO2_EFFECT_CHAIN* pEffectChain X2DEFAULT(NULL)) throw()
    {
        HRESULT hr = S_OK;
        IXAMIDISynthesizer* pMidSynth = NULL;
        IXAudio2SourceVoice* pSrcVoice = NULL;

        if (!pSynthParams) return XAUDIO2_E_INVALID_CALL;
        __try {
            XAMIDI_SYNTH_DETAILS synthDetails;

            hr = _pXAMIDI->CreateSynthesizer(&pMidSynth, pSynthParams, this);
            if (FAILED(hr)) return hr;

            hr = pMidSynth->GetSynthDetails(&synthDetails);
            if (FAILED(hr)) return hr;

            hr = _pXAudio->CreateSourceVoice(&pSrcVoice, &synthDetails.OutputFormat.Format, 0, MaxFrequencyRatio, this, pSendList, pEffectChain);
            if (FAILED(hr)) return hr;

            std::swap(const_cast<IXAMIDISynthesizer*&>(_pMidSynth), pMidSynth);
            std::swap(const_cast<IXAudio2SourceVoice*&>(_pSrcVoice), pSrcVoice);
        }
        __finally {
            if (pMidSynth) pMidSynth->DestroySynth();
            if (pSrcVoice) pSrcVoice->DestroyVoice();
        }
        return hr;
    }

    CXAMIDISynthVoice(__in IXAudio2 *pXAudio2, __in IXAMIDI *pXAMIDI) throw() : _pXAudio(pXAudio2), _pXAMIDI(pXAMIDI), _pSrcVoice(), _pMidSynth()
    {
        if (_pXAudio) _pXAudio->AddRef();
        if (_pXAMIDI) _pXAMIDI->AddRef();
    }

    ~CXAMIDISynthVoice() throw()
    {
        IXAMIDISynthesizer* pMidSynth = _pMidSynth;
        IXAudio2SourceVoice* pSrcVoice = _pSrcVoice;
        const_cast<IXAMIDISynthesizer*&>(_pMidSynth) = NULL;
        const_cast<IXAudio2SourceVoice*&>(_pSrcVoice) = NULL;
        if (pMidSynth) pMidSynth->DestroySynth();
        if (pSrcVoice) pSrcVoice->DestroyVoice();
        if (_pXAMIDI) _pXAMIDI->Release();
        if (_pXAudio) _pXAudio->Release();
    }

private:

    CXAMIDISynthVoice(const CXAMIDISynthVoice &)
        : _pXAudio()
        , _pXAMIDI()
        , _pSrcVoice()
        , _pMidSynth()
    {
        throw "Duplicate copy is not supported";
    }

    CXAMIDISynthVoice &operator=(const CXAMIDISynthVoice &)
    {
        throw "Duplicate copy is not supported";
    }

protected: // IXAudio2VoiceCallback methods

    STDMETHODIMP_(void) OnVoiceProcessingPassStart(UINT32 BytesRequired) throw()
    {
        if (_pMidSynth) return _pMidSynth->OnVoiceCallback(XAMIDI_OnVoiceProcessingPassStart, &BytesRequired);
    }

    STDMETHODIMP_(void) OnVoiceProcessingPassEnd(void) throw()
    {
        if (_pMidSynth) return _pMidSynth->OnVoiceCallback(XAMIDI_OnVoiceProcessingPassEnd);
    }

    STDMETHODIMP_(void) OnStreamEnd(void) throw()
    {
        if (_pMidSynth) return _pMidSynth->OnVoiceCallback(XAMIDI_OnStreamEnd);
    }

    STDMETHODIMP_(void) OnBufferStart(void* pBufferContext) throw()
    {
        if (_pMidSynth) return _pMidSynth->OnVoiceCallback(XAMIDI_OnBufferStart, pBufferContext);
    }

    STDMETHODIMP_(void) OnBufferEnd(void* pBufferContext) throw()
    {
        if (_pMidSynth) return _pMidSynth->OnVoiceCallback(XAMIDI_OnBufferEnd, pBufferContext);
    }

    STDMETHODIMP_(void) OnLoopEnd(void* pBufferContext) throw()
    {
        if (_pMidSynth) return _pMidSynth->OnVoiceCallback(XAMIDI_OnLoopEnd, pBufferContext);
    }

    STDMETHODIMP_(void) OnVoiceError(void* pBufferContext, HRESULT Error)
    {
        throw "Voice Error", pBufferContext, Error;
    }

protected: // IXAMIDISynthCallback methods

    STDMETHODIMP_(void) OnSynthError(void* pReserved, HRESULT Error)
    {
        throw "Synth Error", pReserved, Error;
    }

    STDMETHODIMP_(void) OnVoiceDetails(THIS_ __out UINT16* pInputChannels, __out UINT32 *pInputSampleRate) throw()
    {
        if (_pSrcVoice)
        {
            XAUDIO2_VOICE_DETAILS Details;
            _pSrcVoice->GetVoiceDetails(&Details);
            *pInputChannels = Details.InputChannels;
            *pInputSampleRate = Details.InputSampleRate;
        }
        else
        {
            *pInputChannels = XAUDIO2_DEFAULT_CHANNELS;
            *pInputSampleRate = XAUDIO2_DEFAULT_SAMPLERATE;
        }
    }

    STDMETHODIMP OnVoiceMethod(__in XAMIDI_VOICE_METHOD Method) throw()
    {
        if (_pSrcVoice) switch (Method)
        {
        case XAMIDI_VoiceStart:
            return _pSrcVoice->Start();
        case XAMIDI_VoiceStop:
            return _pSrcVoice->Stop();
        case XAMIDI_VoiceFlush:
            return _pSrcVoice->FlushSourceBuffers();
        }
        return XAUDIO2_E_INVALID_CALL;
    }

    STDMETHODIMP OnSubmitBuffer(__in const XAMIDI_SYNTH_ONBUFFER* pSynthBuffer) throw()
    {
        if (!_pSrcVoice) return XAUDIO2_E_INVALID_CALL;
        XAUDIO2_BUFFER VoiceBuffer = { 0U, pSynthBuffer->AudioBytes, _pMidSynth->GetBufferData() + pSynthBuffer->SamplesOffset, };
        VoiceBuffer.pContext = const_cast<XAMIDI_SYNTH_ONBUFFER*>(pSynthBuffer);
        return _pSrcVoice->SubmitSourceBuffer(&VoiceBuffer);
    }

    STDMETHODIMP_(void) OnGetState(__out XAMIDI_SYNTH_ONSTATE* pSynthState) throw()
    {
        if (_pSrcVoice)
        {
            XAUDIO2_VOICE_STATE VoiceState;
            _pSrcVoice->GetState(&VoiceState);
            pSynthState->pCurrentBuffer = static_cast<XAMIDI_SYNTH_ONBUFFER*>(VoiceState.pCurrentBufferContext);
            pSynthState->SamplesPlayed = VoiceState.SamplesPlayed;
        }
        else
        {
            pSynthState->pCurrentBuffer = NULL;
            pSynthState->SamplesPlayed = 0;
        }
    }

protected:

    IXAudio2* const _pXAudio;
    IXAMIDI* const _pXAMIDI;
    IXAudio2SourceVoice* const _pSrcVoice;
    IXAMIDISynthesizer* const _pMidSynth;
};

#endif // #ifdef __cplusplus
#endif // #ifndef __XAMIDI_HELPER_INCLUDED__
