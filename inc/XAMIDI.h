/**************************************************************************
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * File:    xamidi.h
 * Content: Declarations for the XAudio2 game midi API.
 *
 **************************************************************************/

#ifndef __XAMIDI_INCLUDED__
#define __XAMIDI_INCLUDED__

#if _MSC_VER > 1000
#pragma once
#endif

//--------------<D-E-F-I-N-I-T-I-O-N-S>-------------------------------------//
#include <windef.h>         // general windows types
#include <sal.h>            // Markers for documenting API semantics


/**************************************************************************
 *
 *  WAVEFORMATEXTENSIBLE: Extended version of WAVEFORMATEX that should be
 *  used as a basis for all new audio formats.  The format tag is replaced
 *  with a GUID, allowing new formats to be defined without registering a
 *  format tag with Microsoft.  There are also new fields that can be used
 *  to specify the spatial positions for each channel and the bit packing
 *  used for wide samples (e.g. 24-bit PCM samples in 32-bit containers).
 *
 ***************************************************************************/

#ifndef _WAVEFORMATEXTENSIBLE_

    #define _WAVEFORMATEXTENSIBLE_
    typedef struct WAVEFORMATEXTENSIBLE
    {
        WAVEFORMATEX Format;          // Base WAVEFORMATEX data
        union
        {
            WORD wValidBitsPerSample; // Valid bits in each sample container
            WORD wSamplesPerBlock;    // Samples per block of audio data; valid
                                      // if wBitsPerSample=0 (but rarely used).
            WORD wReserved;           // Zero if neither case above applies.
        } Samples;
        DWORD dwChannelMask;          // Positions of the audio channels
        GUID SubFormat;               // Format identifier GUID
    } WAVEFORMATEXTENSIBLE;

#endif

#ifndef WAVE_FORMAT_EXTENSIBLE
    #define WAVE_FORMAT_EXTENSIBLE      0xFFFE // All WAVEFORMATEXTENSIBLE formats
#endif


/**************************************************************************
 *
 * XAMIDI COM object class and interface IDs.
 *
 **************************************************************************/

DEFINE_GUID(IID_IXAMIDI, 0x78C390D9, 0x9571, 0x42AA, 0x84, 0x70, 0xAC, 0x91, 0x76, 0x6F, 0xC4, 0xAC);
DEFINE_GUID(IID_IReferenceClock, 0x56a86897, 0x0ad4, 0x11ce, 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

#ifndef X2DEFAULT
// Use default arguments if compiling as C++
#ifdef __cplusplus
    #define X2DEFAULT(x) =x
#else
    #define X2DEFAULT(x)
#endif
#endif // !X2DEFAULT


// Ignore the rest of this header if only the GUID definitions were requested
#ifndef GUID_DEFS_ONLY

#ifdef _XBOX
    #include <xobjbase.h>   // Xbox COM declarations (IUnknown, etc)
#else
    #include <objbase.h>    // Windows COM declarations
#endif


/**************************************************************************
 *
 * Forward declarations for the XAMIDI interfaces.
 *
 **************************************************************************/

#ifndef FWD_DECLARE
#ifdef __cplusplus
    #define FWD_DECLARE(x) interface x
#else
    #define FWD_DECLARE(x) typedef interface x x
#endif
#endif // !FWD_DECLARE

FWD_DECLARE(IXAMIDI);
FWD_DECLARE(IXAMIDISynthesizer);
FWD_DECLARE(IXAMIDITimbreBanks);
FWD_DECLARE(IXAMIDITimbreDownloaded);
FWD_DECLARE(IXAMIDISynthCallback);
FWD_DECLARE(IReferenceClock);

typedef INT64 REFERENCE_TIME;
typedef double REFTIME;
typedef UINT_PTR HXAMIDITimbre;
typedef void STDAPICALLTYPE XAMIDI_FREE_MEMORY(void *pContext);


/**************************************************************************
 *
 * XAMIDI constants, flags and error codes.
 *
 **************************************************************************/

// Numeric boundary values
static const UINT32 XAMIDI_MAX_AUDIO_CHANNELS = 2;      // Maximum channels in an audio stream
static const UINT32 XAMIDI_MIN_SAMPLE_RATE = 11025;     // Minimum audio sample rate supported
static const UINT32 XAMIDI_MAX_SAMPLE_RATE = 44100;     // Maximum audio sample rate supported
static const UINT32 XAMIDI_MAX_GROUPS = 1000;           // Maximum audio sample rate supported
static const UINT32 XAMIDI_MAX_VOICES = 1000;           // Maximum audio sample rate supported
static const FLOAT XAMIDI_MAX_VOLUME_LEVEL = 10.0f;     // Maximum acceptable volume level (10^1)

// Numeric values with special meanings
static const UINT32 XAMIDI_COMMIT_NOW = 0;              // Used as an OperationSet argument
static const UINT32 XAMIDI_COMMIT_ALL = 0;              // Used in IXAMIDI::CommitChanges
static const UINT32 XAMIDI_INVALID_OPSET = -1;          // Not allowed for OperationSet arguments
static const UINT32 XAMIDI_DEFAULT_CHANNELS = 0;        // Used in CreateSynthesizer
static const UINT32 XAMIDI_DEFAULT_SAMPLERATE = 0;      // Used in CreateSynthesizer
static const UINT32 XAMIDI_DEFAULT_GROUPS = 0;          // Used in CreateSynthesizer
static const UINT32 XAMIDI_DEFAULT_VOICES = 0;          // Used in CreateSynthesizer
static const UINT32 XAMIDI_GROUP_ALL = 0;               // Used in SendShortMsg
static const REFERENCE_TIME XAMIDI_PLAYTIME_NOW = 0;    // Used in SendShortMsg

// XAudio2 error codes
static const UINT16 FACILITY_XAMIDI = 0x896;
static const HRESULT XAMIDI_E_INVALID_CALL = 0x88960001;// An API call or one of its arguments was illegal


/**************************************************************************
 *
 * XAMIDI structures and enumerations.
 *
 **************************************************************************/

// Standard MIDI instrument's presets
typedef enum XAMIDI_INSTRUMENTS_PATCH_PRESET
{
    XAMIDI_PATCH_PRESET_AcousticPiano,
    XAMIDI_PATCH_PRESET_BrightPiano,
    XAMIDI_PATCH_PRESET_ElectricPiano,
    XAMIDI_PATCH_PRESET_HonkytonkPiano,
    XAMIDI_PATCH_PRESET_RhodesEPiano,
    XAMIDI_PATCH_PRESET_ChorusedEPiano,
    XAMIDI_PATCH_PRESET_Harpsichord,
    XAMIDI_PATCH_PRESET_Clavichord,
    XAMIDI_PATCH_PRESET_Celesta,
    XAMIDI_PATCH_PRESET_Glockenspiel,
    XAMIDI_PATCH_PRESET_MusicBox,
    XAMIDI_PATCH_PRESET_Vibraphone,
    XAMIDI_PATCH_PRESET_Marimba,
    XAMIDI_PATCH_PRESET_Xylophone,
    XAMIDI_PATCH_PRESET_TubularBell,
    XAMIDI_PATCH_PRESET_Dulcimer,
    XAMIDI_PATCH_PRESET_HammondOrgan,
    XAMIDI_PATCH_PRESET_PercussiveOrgan,
    XAMIDI_PATCH_PRESET_RockOrgan,
    XAMIDI_PATCH_PRESET_ChurchOrgan,
    XAMIDI_PATCH_PRESET_ReedOrgan,
    XAMIDI_PATCH_PRESET_Accordion,
    XAMIDI_PATCH_PRESET_Harmonica,
    XAMIDI_PATCH_PRESET_Bandoneon,
    XAMIDI_PATCH_PRESET_NylonGuitar,
    XAMIDI_PATCH_PRESET_SteelGuitar,
    XAMIDI_PATCH_PRESET_JazzGuitar,
    XAMIDI_PATCH_PRESET_CleanGuitar,
    XAMIDI_PATCH_PRESET_MutedGuitar,
    XAMIDI_PATCH_PRESET_OverdriveGuitar,
    XAMIDI_PATCH_PRESET_DistortionGuitar,
    XAMIDI_PATCH_PRESET_GuitarHarmonics,
    XAMIDI_PATCH_PRESET_AcousticBass,
    XAMIDI_PATCH_PRESET_FingeredBass,
    XAMIDI_PATCH_PRESET_PickedBass,
    XAMIDI_PATCH_PRESET_FretlessBass,
    XAMIDI_PATCH_PRESET_SlapBass1,
    XAMIDI_PATCH_PRESET_SlapBass2,
    XAMIDI_PATCH_PRESET_SynthBass1,
    XAMIDI_PATCH_PRESET_SynthBass2,
    XAMIDI_PATCH_PRESET_Violin,
    XAMIDI_PATCH_PRESET_Viola,
    XAMIDI_PATCH_PRESET_Cello,
    XAMIDI_PATCH_PRESET_Contrabass,
    XAMIDI_PATCH_PRESET_TremoloStrings,
    XAMIDI_PATCH_PRESET_PizzicatoStrings,
    XAMIDI_PATCH_PRESET_OrchestralHarp,
    XAMIDI_PATCH_PRESET_Timpani,
    XAMIDI_PATCH_PRESET_StringsEnsemble,
    XAMIDI_PATCH_PRESET_SlowStrings,
    XAMIDI_PATCH_PRESET_SynthStrings1,
    XAMIDI_PATCH_PRESET_SynthStrings2,
    XAMIDI_PATCH_PRESET_ChoirAahs,
    XAMIDI_PATCH_PRESET_VoiceOohs,
    XAMIDI_PATCH_PRESET_SynthVoice,
    XAMIDI_PATCH_PRESET_OrchestraHit,
    XAMIDI_PATCH_PRESET_Trumpet,
    XAMIDI_PATCH_PRESET_Trombone,
    XAMIDI_PATCH_PRESET_Tuba,
    XAMIDI_PATCH_PRESET_MutedTrumpet,
    XAMIDI_PATCH_PRESET_FrenchHorns,
    XAMIDI_PATCH_PRESET_BrassSection,
    XAMIDI_PATCH_PRESET_SynthBrass1,
    XAMIDI_PATCH_PRESET_SynthBrass2,
    XAMIDI_PATCH_PRESET_SopranoSax,
    XAMIDI_PATCH_PRESET_AltoSax,
    XAMIDI_PATCH_PRESET_TenorSax,
    XAMIDI_PATCH_PRESET_BaritoneSax,
    XAMIDI_PATCH_PRESET_Oboe,
    XAMIDI_PATCH_PRESET_EnglishHorn,
    XAMIDI_PATCH_PRESET_Bassoon,
    XAMIDI_PATCH_PRESET_Clarinet,
    XAMIDI_PATCH_PRESET_Piccolo,
    XAMIDI_PATCH_PRESET_Flute,
    XAMIDI_PATCH_PRESET_Recorder,
    XAMIDI_PATCH_PRESET_PanFlute,
    XAMIDI_PATCH_PRESET_BottleBlow,
    XAMIDI_PATCH_PRESET_Shakuhachi,
    XAMIDI_PATCH_PRESET_Whistle,
    XAMIDI_PATCH_PRESET_Ocarina,
    XAMIDI_PATCH_PRESET_SquareWave,
    XAMIDI_PATCH_PRESET_SawtoothWave,
    XAMIDI_PATCH_PRESET_Calliope,
    XAMIDI_PATCH_PRESET_ChifferLead,
    XAMIDI_PATCH_PRESET_Charang,
    XAMIDI_PATCH_PRESET_SoloVoice,
    XAMIDI_PATCH_PRESET_5thSawWave,
    XAMIDI_PATCH_PRESET_BassLead,
    XAMIDI_PATCH_PRESET_Fantasia,
    XAMIDI_PATCH_PRESET_WarmPad,
    XAMIDI_PATCH_PRESET_Polysynth,
    XAMIDI_PATCH_PRESET_SpaceVoice,
    XAMIDI_PATCH_PRESET_BowedGlass,
    XAMIDI_PATCH_PRESET_MetalPad,
    XAMIDI_PATCH_PRESET_HaloPad,
    XAMIDI_PATCH_PRESET_SweepPad,
    XAMIDI_PATCH_PRESET_IceRain,
    XAMIDI_PATCH_PRESET_Soundtrack,
    XAMIDI_PATCH_PRESET_Crystal,
    XAMIDI_PATCH_PRESET_Atmosphere,
    XAMIDI_PATCH_PRESET_Brightness,
    XAMIDI_PATCH_PRESET_Goblin,
    XAMIDI_PATCH_PRESET_EchoDrops,
    XAMIDI_PATCH_PRESET_StarTheme,
    XAMIDI_PATCH_PRESET_Sitar,
    XAMIDI_PATCH_PRESET_Banjo,
    XAMIDI_PATCH_PRESET_Shamisen,
    XAMIDI_PATCH_PRESET_Koto,
    XAMIDI_PATCH_PRESET_Kalimba,
    XAMIDI_PATCH_PRESET_Bagpipe,
    XAMIDI_PATCH_PRESET_Fiddle,
    XAMIDI_PATCH_PRESET_Shanai,
    XAMIDI_PATCH_PRESET_TinkleBell,
    XAMIDI_PATCH_PRESET_Agogo,
    XAMIDI_PATCH_PRESET_SteelDrums,
    XAMIDI_PATCH_PRESET_Woodblock,
    XAMIDI_PATCH_PRESET_TaikoDrums,
    XAMIDI_PATCH_PRESET_MelodicTom,
    XAMIDI_PATCH_PRESET_SynthDrum,
    XAMIDI_PATCH_PRESET_ReverseCymbal,
    XAMIDI_PATCH_PRESET_GuitarFretNoise,
    XAMIDI_PATCH_PRESET_BreathNoise,
    XAMIDI_PATCH_PRESET_Seashore,
    XAMIDI_PATCH_PRESET_BirdTweet,
    XAMIDI_PATCH_PRESET_TelephoneRing,
    XAMIDI_PATCH_PRESET_Helicopter,
    XAMIDI_PATCH_PRESET_Applause,
    XAMIDI_PATCH_PRESET_Gunshot,
} XAMIDI_INSTRUMENTS_PATCH_PRESET;

typedef enum XAMIDI_LOAD_OPTION
{
    XAMIDI_DEFAULT,                     // Details as NULL: System GM sound set is built in
    XAMIDI_FILEPATH,                    // Details as LPCWSTR: Local disk file path
    XAMIDI_URLPATH,                     // Details as LPCWSTR: Network URL file path
    XAMIDI_MEMORY,                      // Details as XAMIDI_LOAD_MEMORY: Memory data
    XAMIDI_STREAM,                      // Details as IStream: Stream with data
} XAMIDI_LOAD_OPTION;

// XAMIDI_SYNTH_EFFECTS are used in the Effects fields of both XAMIDI_SYNTH_PARAMETERS and SetEffects.
typedef enum XAMIDI_SYNTH_EFFECTS
{
    XAMIDI_EFFECT_NONE = 0x00000000,
    XAMIDI_EFFECT_REVERB = 0x00000001,
} XAMIDI_SYNTH_EFFECTS;

typedef enum XAMIDI_VOICE_CALLBACK
{
    XAMIDI_OnVoiceProcessingPassStart,  // From IXAudio2VoiceCallback::OnVoiceProcessingPassStart
    XAMIDI_OnVoiceProcessingPassEnd,    // From IXAudio2VoiceCallback::OnVoiceProcessingPassEnd
    XAMIDI_OnStreamEnd,                 // From IXAudio2VoiceCallback::OnStreamEnd
    XAMIDI_OnBufferStart,               // From IXAudio2VoiceCallback::OnBufferStart
    XAMIDI_OnBufferEnd,                 // From IXAudio2VoiceCallback::OnBufferEnd
    XAMIDI_OnLoopEnd,                   // From IXAudio2VoiceCallback::OnLoopEnd
} XAMIDI_VOICE_CALLBACK;

typedef enum XAMIDI_VOICE_METHOD
{
    XAMIDI_VoiceStart,                  // To IXAudio2SourceVoice::Start
    XAMIDI_VoiceStop,                   // To IXAudio2SourceVoice::Stop
    XAMIDI_VoiceFlush,                  // To IXAudio2SourceVoice::FlushSourceBuffers
} XAMIDI_VOICE_METHOD;

typedef struct XAMIDI_ENGINE_DETAILS
{
    UINT32 MemorySize;                  // Amount of memory available to store DLS instruments.
    UINT32 MaxOutputChannels;           // Maximum number of audio channels that can be rendered by the synthesizer.
    UINT32 MaxVoiceCount;               // Maximum number of voices that can be allocated when this synthesizer is opened.
    UINT32 MaxGroupCount;               // Maximum number of audio groups supported by this synthesizer.
} XAMIDI_ENGINE_DETAILS;

typedef struct XAMIDI_LOAD_MEMORY
{
    const void* pMemData;               // Memory pointer for data.
    SIZE_T MemBytes;                    // Size of Memory data.
    void* pContext;                     // Context value to be passed back in callbacks.
    XAMIDI_FREE_MEMORY *pFreeFun;       // Callback function for releasing memory.
} XAMIDI_LOAD_MEMORY;

typedef struct XAMIDI_SYNTH_PARAMETERS
{
    UINT32 Flags;                       // XAMIDI_SYNTH flags specifying the midi synthesizer's behavior.
    UINT32 Effects;                     // Initial enabled special effects.
    UINT32 OutputChannels;              // Desired number of output audio channels.
    UINT32 OutputSampleRate;            // Desired output sample rate, in hertz.
    UINT32 VoiceCount;                  // Number of voices required on this synthesizer.
    UINT32 GroupCount;                  // Number of audio groups to be allocated on this synthesizer
} XAMIDI_SYNTH_PARAMETERS;

typedef struct XAMIDI_SYNTH_DETAILS
{
    UINT32 VoiceCount;                  // Number of voices required on this synthesizer.
    UINT32 BufferSize;                  // Amount of memory available to submit source buffers.
    WAVEFORMATEXTENSIBLE OutputFormat;  // The device's native PCM audio output format.
} XAMIDI_SYNTH_DETAILS;

typedef struct XAMIDI_SYNTH_WAVESREVERB
{
    float InGain;                       // Input gain in dB (to avoid output overflows)
    float ReverbMix;                    // Reverb mix in dB. 0dB means 100% wet reverb (no direct signal) Negative values gives less wet signal.
    float ReverbTime;                   // The reverb decay time, in milliseconds.
    float HighFreqRTRatio;              // The ratio of the high frequencies to the global reverb time.
} XAMIDI_SYNTH_WAVESREVERB;

// Used in IXAudio2SourceVoice::SubmitSourceBuffer
typedef struct XAMIDI_SYNTH_ONBUFFER
{
    UINT32 SamplesOffset;               // The samples offset number of voice blocks in the current audio stream.
    UINT32 AudioBytes;                  // Size of the audio data buffer in bytes.
} XAMIDI_SYNTH_ONBUFFER;

// Returned by IXAudio2SourceVoice::GetState
typedef struct XAMIDI_SYNTH_ONSTATE
{
    const XAMIDI_SYNTH_ONBUFFER* pCurrentBuffer;        // The pointer provided on the XAMIDI_SYNTH_ONBUFFER that is currently being processed, or NULL if there are no buffers in the queue.
    UINT64 SamplesPlayed;               // The number of samples played in the current audio stream.
} XAMIDI_SYNTH_ONSTATE;

typedef struct XAMIDI_TIMBRE_DETAILS
{
    HXAMIDITimbre hTimbre;              // That the timbre's object handle.
    UINT32 Patch;                       // That the timbre's patch number.
    UINT32 NameLen;                     // That the timbre's name length.
    LPWSTR Name;                        // That the timbre's instrument name.
} XAMIDI_TIMBRE_DETAILS;

/**************************************************************************
 *
 * IXAMIDI: Top-level XAMIDI COM interface.
 *
 **************************************************************************/

#undef INTERFACE
#define INTERFACE IXAMIDI
DECLARE_INTERFACE_IID_(IXAMIDI, IUnknown, "78C390D9-9571-42AA-8470-AC91766FC4AC")
{
    // IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ __in REFIID riid, __deref_out void** ppvInterface) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // IXAMIDI methods
    STDMETHOD(GetEngineDetails) (THIS_ __out XAMIDI_ENGINE_DETAILS* pEngineDetails) PURE;
    STDMETHOD(LoadTimbreBanks) (THIS_ __deref_out IXAMIDITimbreBanks** ppTimbreBank, __in_opt XAMIDI_LOAD_OPTION Option X2DEFAULT(XAMIDI_DEFAULT), __in_opt const void* Details X2DEFAULT(NULL)) PURE;
    STDMETHOD(CreateSynthesizer) (THIS_ __deref_out IXAMIDISynthesizer** ppSynthesizer, __inout XAMIDI_SYNTH_PARAMETERS *SynthParams, __in IXAMIDISynthCallback *pCallback) PURE;
    STDMETHOD(StartEngine) (THIS) PURE;
    STDMETHOD_(void, StopEngine) (THIS) PURE;
    STDMETHOD(CommitChanges) (THIS_ UINT32 OperationSet X2DEFAULT(XAMIDI_COMMIT_ALL)) PURE;
};


/**************************************************************************
 *
 * IXAMIDISynthesizer: XAMIDI Synthesizer management interface.
 *
 **************************************************************************/

#undef INTERFACE
#define INTERFACE IXAMIDISynthesizer
DECLARE_INTERFACE(IXAMIDISynthesizer)
{
    STDMETHOD_(void, DestroySynth) (THIS) PURE;
    STDMETHOD(GetSynthDetails) (THIS_ __out XAMIDI_SYNTH_DETAILS* pSynthDetails) PURE;
    STDMETHOD_(const BYTE *, GetBufferData) (THIS) PURE;
    STDMETHOD_(UINT32, GetGroupCount) (THIS) PURE;
    STDMETHOD(SetGroupCount) (THIS_ __in UINT32 Groups) PURE;
    STDMETHOD(DownloadTimbre) (THIS_ __deref_out IXAMIDITimbreDownloaded** ppDownloaded, __in UINT32 Patch, __in HXAMIDITimbre hTimbre, __in_opt UINT32 OperationSet X2DEFAULT(XAMIDI_COMMIT_NOW)) PURE;
    STDMETHOD_(BOOL, GetEnabled) (THIS) PURE;
    STDMETHOD(SetEnabled) (THIS_ __in BOOL Active) PURE;
    STDMETHOD_(float, GetVolume) (THIS) PURE;
    STDMETHOD(SetVolume) (THIS_ __in float Volume) PURE;
    STDMETHOD_(UINT32, GetEffects) (THIS) PURE;
    STDMETHOD(SetEffects) (THIS_ __in UINT32 Effects) PURE;
    STDMETHOD_(UINT32, GetWriteLatency) (THIS) PURE;
    STDMETHOD(SetWriteLatency) (THIS_ __in UINT32 Milliseconds) PURE;
    STDMETHOD(GetWavesReverb) (THIS_ __out XAMIDI_SYNTH_WAVESREVERB *pWavesReverb) PURE;
    STDMETHOD(SetWavesReverb) (THIS_ __in const XAMIDI_SYNTH_WAVESREVERB *pWavesReverb) PURE;
    STDMETHOD_(void, OnVoiceCallback) (THIS_ __in XAMIDI_VOICE_CALLBACK Callback, __in_opt void* pParam X2DEFAULT(NULL)) PURE;
    STDMETHOD_(IReferenceClock*, GetReferenceClock) (THIS) PURE;
    STDMETHOD(OutShortMsg) (THIS_ __in UINT32 MsgCmd, __in_opt UINT32 Group X2DEFAULT(XAMIDI_GROUP_ALL), __in_opt REFERENCE_TIME PlayTime X2DEFAULT(XAMIDI_PLAYTIME_NOW), __in_opt UINT32 OperationSet X2DEFAULT(XAMIDI_COMMIT_NOW)) PURE;
    STDMETHOD(OutLongMsg) (THIS_ __in const void* BytesData, __in UINT32 Length, __in_opt UINT32 Group X2DEFAULT(XAMIDI_GROUP_ALL), __in_opt REFERENCE_TIME PlayTime X2DEFAULT(XAMIDI_PLAYTIME_NOW), __in_opt UINT32 OperationSet X2DEFAULT(XAMIDI_COMMIT_NOW)) PURE;
};


/**************************************************************************
 *
 * IXAMIDITimbreBanks: XAMIDI TimbreBanks management interface.
 *
 **************************************************************************/

#undef INTERFACE
#define INTERFACE IXAMIDITimbreBanks
DECLARE_INTERFACE(IXAMIDITimbreBanks)
{
    STDMETHOD_(void, DestroyBanks) (THIS) PURE;
    STDMETHOD_(UINT32, GetTimbreCount) (THIS) PURE;
    STDMETHOD(GetTimbreDetails) (THIS_ UINT32 Index, __inout XAMIDI_TIMBRE_DETAILS* pTimbreDetails) PURE;
    STDMETHOD_(HXAMIDITimbre, GetPatchTimbre) (THIS_ __in UINT32 Patch) PURE;
    STDMETHOD_(HXAMIDITimbre, IndexOfTimbre) (THIS_ __in UINT32 Index) PURE;
};


/**************************************************************************
 *
 * IXAMIDITimbreDownloaded: XAMIDI TimbreDownloaded management interface.
 *
 **************************************************************************/

#undef INTERFACE
#define INTERFACE IXAMIDITimbreDownloaded
DECLARE_INTERFACE(IXAMIDITimbreDownloaded)
{
    STDMETHOD_(void, UnloadTimbre) (THIS) PURE;
    STDMETHOD_(UINT32, GetPatch) (THIS) PURE;
    STDMETHOD_(HXAMIDITimbre, GetTimbre) (THIS) PURE;
};


/**************************************************************************
 *
 * IXAMIDISynthCallback: Client notification interface for voice events.
 *
 * REMARKS: Contains methods to notify the client when certain events happen
 *          in an XAudio2 voice.  This interface should be implemented by the
 *          client.  XAudio2 will call these methods via an interface pointer
 *          provided by the client in the IXAudio2::CreateSourceVoice call.
 *
 **************************************************************************/

#undef INTERFACE
#define INTERFACE IXAMIDISynthCallback
DECLARE_INTERFACE(IXAMIDISynthCallback)
{
    STDMETHOD_(void, OnSynthError) (THIS_ void* pReserved, HRESULT Error) PURE;
    STDMETHOD_(void, OnVoiceDetails) (THIS_ __out UINT16* pInputChannels, __out UINT32 *pInputSampleRate) PURE;
    STDMETHOD(OnVoiceMethod) (THIS_ __in XAMIDI_VOICE_METHOD Method) PURE;
    STDMETHOD(OnSubmitBuffer) (THIS_ __in const XAMIDI_SYNTH_ONBUFFER* pSynthBuffer) PURE;
    STDMETHOD_(void, OnGetState) (THIS_ __out XAMIDI_SYNTH_ONSTATE* pSynthState) PURE;
};

#ifndef __IReferenceClock_INTERFACE_DEFINED__
#define __IReferenceClock_INTERFACE_DEFINED__

#undef INTERFACE
#define INTERFACE IReferenceClock
DECLARE_INTERFACE_IID_(IReferenceClock, IUnknown, "56a86897-0ad4-11ce-b03a-0020af0ba770")
{
    // IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ __in REFIID riid, __deref_out void** ppvInterface) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // IReferenceClock methods
    STDMETHOD(GetTime) (THIS_ __out REFERENCE_TIME *pTime) PURE;
    STDMETHOD(AdviseTime) (THIS_ REFERENCE_TIME rtBaseTime, REFERENCE_TIME rtStreamTime, HANDLE hEvent, __out LPDWORD pdwAdviseCookie) PURE;
    STDMETHOD(AdvisePeriodic) (THIS_ REFERENCE_TIME rtStartTime, REFERENCE_TIME rtPeriodTime, HANDLE hSemaphore, __out LPDWORD pdwAdviseCookie) PURE;
    STDMETHOD(Unadvise) (THIS_ DWORD dwAdviseCookie) PURE;
};

#endif // __IReferenceClock_INTERFACE_DEFINED__


/**************************************************************************
 *
 * These are only defined if the client #defines XAMIDI_HELPER_FUNCTIONS
 * prior to #including xmidi.h.
 *
 **************************************************************************/

#ifdef XAMIDI_HELPER_FUNCTIONS

// Make pathces code
FORCEINLINE UINT32 XMIDIMakePatch(BYTE MSBank, BYTE LSBank, BYTE Instrument)
{
    return (0x7FFF0000U & (MSBank << 16U)) | (0x0000FF00U & (LSBank << 8U)) | (Instrument & 0x000000FFU);
}

// Make drum's pathces code
FORCEINLINE UINT32 XMIDIMakeDrumPatch(BYTE MSBank, BYTE LSBank, BYTE Instrument)
{
    return (0x80000000U | (MSBank << 16U)) | (0x0000FF00U & (LSBank << 8U)) | (Instrument & 0x000000FFU);
}

// Is drum's patches code
FORCEINLINE BOOL XMIDIIsDrumPatch(UINT32 Patch)
{
    return (BOOL)(Patch >> 31U);
}

// Get patches MSBank code
FORCEINLINE BYTE XMIDIGetPatchMSBank(UINT32 Patch)
{
    return (BYTE)(Patch >> 16U);
}

// Is drum's patches code
FORCEINLINE BYTE XMIDIGetPatchLSBank(UINT32 Patch)
{
    return (BYTE)(Patch >> 8U);
}

// Is drum's patches code
FORCEINLINE BYTE XMIDIGetPatchInstrument(UINT32 Patch)
{
    return (BYTE)(0x000000FFU & Patch);
}

// Load resource to memory details
FORCEINLINE HRESULT XMIDILoadResource(XAMIDI_LOAD_MEMORY *LoadDetails, HMODULE hModule, HRSRC hResInfo)
{
	if (!LoadDetails || !hResInfo) return XAMIDI_E_INVALID_CALL;
	if (!(LoadDetails->MemBytes = SizeofResource(hModule, hResInfo))) goto Err;
	if (!(LoadDetails->pContext = LoadResource(hModule, hResInfo))) goto Err;
	LoadDetails->pFreeFun = reinterpret_cast<XAMIDI_FREE_MEMORY*>(FreeResource);
	LoadDetails->pMemData = LockResource(LoadDetails->pContext);
	return S_OK;
Err:
	return HRESULT_FROM_WIN32(GetLastError());
}

#endif // #ifdef XMIDI_HELPER_FUNCTIONS


/**************************************************************************
 *
 * XAMIDICreate: Top-level function that creates an XAMIDI instance.
 *
 * ARGUMENTS:
 *
 *  Flags - Flags specifying the XAudio2 object's behavior. Currently unused, must be set to 0.
 *
 **************************************************************************/

STDAPI XAMIDICreate(__deref_out IXAMIDI** ppXAMIDI, __in_opt UINT32 Flags X2DEFAULT(0));

#endif // #ifndef GUID_DEFS_ONLY
#endif // #ifndef __XAMIDI_INCLUDED__
