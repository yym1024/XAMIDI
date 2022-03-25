#pragma once

typedef struct FXEQ_PARAMETER
{
	float FrequencyCenter; // center frequency in Hz, band
	float Gain;            // boost/cut
	float Bandwidth;       // bandwidth, region of EQ is center frequency +/- bandwidth/2
} FXEQ_PARAMETER;

typedef union FXEQ_PARAMETERSEX
{
#if defined(FXEQ_MIN_FRAMERATE) && defined(FXEQ_MAX_FRAMERATE)
	FXEQ_PARAMETERS raw[3];
#endif
	FXEQ_PARAMETER ext[3][4];
	FXEQ_PARAMETER all[12];
} FXEQ_PARAMETERSEX;

template <typename T, typename U>
FORCEINLINE T &any_cast(const U &u)
{
	return (T&)u;
}

DECLSPEC_SELECTANY struct XAMidiPlayer
{
	jmp_buf ljb;
	HWND hWnd;
	HANDLE hThread, hTimer;

	STDMETHODIMP_(DWORD) ThreadMain(void);
	STDMETHODIMP_(void) OnControlState(int ID, int Value) const;
	STDMETHODIMP_(void) UpdateTimer(DWORD dwTimerLowValue, DWORD dwTimerHighValue);


	FORCEINLINE bool Exit(int code) const
	{
		return SUCCEEDED(NtQueueApcThread(hThread, PIO_APC_ROUTINE(&longjmp), LPVOID(&ljb), LPVOID(code), LPVOID()));
	}

	FORCEINLINE bool ControlState(int ID, int Value) const
	{
		return SUCCEEDED(NtQueueApcThread(hThread, any_cast<PIO_APC_ROUTINE>(&XAMidiPlayer::OnControlState), LPVOID(this), LPVOID(ID), LPVOID(Value)));
	}
} xamp;
