#pragma once

interface IDirectMusicLoader;
interface IDirectMusicObject;

namespace $XAudio2MIDI
{
    namespace $DSound
    {
        class CXAMDirectSound;
        class CXAMDirectSoundBuffer;
    }

    namespace $DMusic
    {
        class CXAMEngine : public IXAMIDI
        {
            friend class CXAMSynthesizer;
            friend class CXAMTimbreBanks;
            friend HRESULT STDAPICALLTYPE ::XAMIDICreate(IXAMIDI** ppXAMIDI, UINT32 Flags);

        public:
            // IUnknown methods
            STDMETHODIMP QueryInterface(THIS_ __in REFIID riid, __deref_out void** ppvInterface) OVERRIDE;
            STDMETHODIMP_(ULONG) AddRef(THIS) OVERRIDE;
            STDMETHODIMP_(ULONG) Release(THIS) OVERRIDE;

            // IXAMIDI methods
            STDMETHODIMP GetEngineDetails(THIS_ __out XAMIDI_ENGINE_DETAILS* pEngineDetails) OVERRIDE;
            STDMETHODIMP LoadTimbreBanks(THIS_ __deref_out IXAMIDITimbreBanks** ppTimbreBank, __in XAMIDI_LOAD_OPTION Option, __in_opt const void* Details X2DEFAULT(NULL)) OVERRIDE;
            STDMETHODIMP CreateSynthesizer(THIS_ __deref_out IXAMIDISynthesizer** ppSynthesizer, __inout XAMIDI_SYNTH_PARAMETERS *SynthParams, __in_opt IXAMIDISynthCallback *pCallback) OVERRIDE;
            STDMETHODIMP StartEngine(THIS) OVERRIDE;
            STDMETHODIMP_(void) StopEngine(THIS) OVERRIDE;
            STDMETHODIMP CommitChanges(THIS_ UINT32 OperationSet X2DEFAULT(XAMIDI_COMMIT_ALL)) OVERRIDE;

        protected:
            CXAMEngine(const CXAMEngine &) : refCount(1) {}

            $DSound::CXAMDirectSound *pDSDev;
            IDirectMusic *pDMEng;
            IDirectMusicLoader *pDMLdr;
            IDirectMusicBuffer *pDMBuf;
            IDirectMusicCollection *pDMCol;
            volatile long refCount;
        };

        class CXAMSynthesizer : public IXAMIDISynthesizer
        {
            friend class CXAMEngine;
            friend class CXAMTimbreBanks;
            friend class CXAMTimbreDownloaded;

        public:
            STDMETHODIMP_(void) DestroySynth(THIS) OVERRIDE;
            STDMETHODIMP GetSynthDetails(THIS_ __out XAMIDI_SYNTH_DETAILS* pSynthDetails) OVERRIDE;
            STDMETHOD_(const BYTE*, GetBufferData) (THIS) OVERRIDE;
            STDMETHODIMP_(UINT32) GetGroupCount(THIS) OVERRIDE;
            STDMETHODIMP SetGroupCount(THIS_ __in UINT32 Groups) OVERRIDE;
            STDMETHODIMP DownloadTimbre(THIS_ __deref_out IXAMIDITimbreDownloaded** ppDownloaded, __in UINT32 Patch, __in HXAMIDITimbre hTimbre, __in_opt UINT32 OperationSet X2DEFAULT(XAMIDI_COMMIT_NOW)) OVERRIDE;
            STDMETHODIMP_(BOOL) GetEnabled(THIS) OVERRIDE;
            STDMETHODIMP SetEnabled(THIS_ __in BOOL Active) OVERRIDE;
            STDMETHODIMP_(float) GetVolume(THIS) OVERRIDE;
            STDMETHODIMP SetVolume(THIS_ __in float Volume) OVERRIDE;
            STDMETHODIMP_(UINT32) GetEffects(THIS) OVERRIDE;
            STDMETHODIMP SetEffects(THIS_ __in UINT32 Effects) OVERRIDE;
            STDMETHODIMP_(UINT32) GetWriteLatency(THIS) OVERRIDE;
            STDMETHODIMP SetWriteLatency(THIS_ __in UINT32 Milliseconds) OVERRIDE;
            STDMETHODIMP GetWavesReverb(THIS_ __out XAMIDI_SYNTH_WAVESREVERB *pWavesReverb) OVERRIDE;
            STDMETHODIMP SetWavesReverb(THIS_ __in const XAMIDI_SYNTH_WAVESREVERB *pWavesReverb) OVERRIDE;
            STDMETHODIMP_(void) OnVoiceCallback(THIS_ __in XAMIDI_VOICE_CALLBACK Callback, __in_opt void* pParam X2DEFAULT(NULL)) OVERRIDE;
            STDMETHOD_(IReferenceClock*, GetReferenceClock) (THIS) OVERRIDE;
            STDMETHODIMP OutShortMsg(THIS_ __in UINT32 MsgCmd, __in_opt UINT32 Group X2DEFAULT(XAMIDI_GROUP_ALL), __in_opt REFERENCE_TIME PlayTime X2DEFAULT(XAMIDI_PLAYTIME_NOW), __in_opt UINT32 OperationSet X2DEFAULT(XAMIDI_COMMIT_NOW)) OVERRIDE;
            STDMETHODIMP OutLongMsg(THIS_ __in const void* BytesData, __in UINT32 Length, __in_opt UINT32 Group X2DEFAULT(XAMIDI_GROUP_ALL), __in_opt REFERENCE_TIME PlayTime X2DEFAULT(XAMIDI_PLAYTIME_NOW), __in_opt UINT32 OperationSet X2DEFAULT(XAMIDI_COMMIT_NOW)) OVERRIDE;

        protected:
            CXAMSynthesizer(const CXAMSynthesizer &) : volume(1.f) {}

            CXAMEngine *parent;
            $DSound::CXAMDirectSoundBuffer *pDSBuf;
            IDirectMusicPort *pDMPrt;
            IKsControl *pKSCtl;
            IReferenceClock *pRefClock;
            float volume;
            UINT32 VoiceCount;
            XAMIDI_SYNTH_ONBUFFER onbufs[4];
        };

        class CXAMTimbreBanks : public IXAMIDITimbreBanks
        {
            friend class CXAMEngine;
            friend class CXAMSynthesizer;

        public:
            STDMETHODIMP_(void) DestroyBanks(THIS) OVERRIDE;
            STDMETHODIMP_(UINT32) GetTimbreCount(THIS) OVERRIDE;
            STDMETHODIMP GetTimbreDetails(THIS_ UINT32 Index, __inout XAMIDI_TIMBRE_DETAILS* pTimbreDetails) OVERRIDE;
            STDMETHODIMP_(HXAMIDITimbre) GetPatchTimbre(THIS_ __in UINT32 Patch) OVERRIDE;
            STDMETHODIMP_(HXAMIDITimbre) IndexOfTimbre(THIS_ __in UINT32 Index) OVERRIDE;

        protected:
            CXAMTimbreBanks(const CXAMTimbreBanks &) {}

            CXAMEngine *parent;
            void* pContext;
            XAMIDI_FREE_MEMORY *pFreeFun;
            IDirectMusicCollection *pDMCol;
            UINT32 count;
        };

        class CXAMTimbreDownloaded : public IXAMIDITimbreDownloaded
        {
            friend class CXAMEngine;
            friend class CXAMSynthesizer;

        public:
            STDMETHODIMP_(void) UnloadTimbre(THIS) OVERRIDE;
            STDMETHODIMP_(UINT32) GetPatch(THIS) OVERRIDE;
            STDMETHODIMP_(HXAMIDITimbre) GetTimbre(THIS) OVERRIDE;

        protected:
            CXAMTimbreDownloaded(const CXAMTimbreDownloaded &) {}

            IDirectMusicPort *pDMPrt;
            IDirectMusicInstrument *pDMInstrument;
            IDirectMusicDownloadedInstrument *pDMDownloaded;
            UINT32 patch;
        };
    }
}
