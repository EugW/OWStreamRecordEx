#include "MDXGI.h"

#include <limits.h>
#include "DXGI/DisplayManager.h"
#include "DXGI/DuplicationManager.h"
#include "DXGI/OutputManager.h"
#include "DXGI/ThreadManager.h"

#include <leptonica/allheaders.h>

OUTPUTMANAGER OutMgr;

// Below are lists of errors expect from Dxgi API calls when a transition event like mode change, PnpStop, PnpStart
// desktop switch, TDR or session disconnect/reconnect. In all these cases we want the application to clean up the threads that process
// the desktop updates and attempt to recreate them.
// If we get an error that is not on the appropriate list then we exit the application

// These are the errors we expect from general Dxgi API due to a transition
HRESULT SystemTransitionsExpectedErrors[] = {
                                                DXGI_ERROR_DEVICE_REMOVED,
                                                DXGI_ERROR_ACCESS_LOST,
                                                static_cast<HRESULT>(WAIT_ABANDONED),
                                                S_OK                                    // Terminate list with zero valued HRESULT
};

// These are the errors we expect from IDXGIOutput1::DuplicateOutput due to a transition
HRESULT CreateDuplicationExpectedErrors[] = {
                                                DXGI_ERROR_DEVICE_REMOVED,
                                                static_cast<HRESULT>(E_ACCESSDENIED),
                                                DXGI_ERROR_UNSUPPORTED,
                                                DXGI_ERROR_SESSION_DISCONNECTED,
                                                S_OK                                    // Terminate list with zero valued HRESULT
};

// These are the errors we expect from IDXGIOutputDuplication methods due to a transition
HRESULT FrameInfoExpectedErrors[] = {
                                        DXGI_ERROR_DEVICE_REMOVED,
                                        DXGI_ERROR_ACCESS_LOST,
                                        S_OK                                    // Terminate list with zero valued HRESULT
};

// These are the errors we expect from IDXGIAdapter::EnumOutputs methods due to outputs becoming stale during a transition
HRESULT EnumOutputsExpectedErrors[] = {
                                          DXGI_ERROR_NOT_FOUND,
                                          S_OK                                    // Terminate list with zero valued HRESULT
};


//
// Forward Declarations
//
DWORD WINAPI DDProc(_In_ void* Param);

//
// Class for progressive waits
//
typedef struct
{
    UINT    WaitTime;
    UINT    WaitCount;
}WAIT_BAND;

#define WAIT_BAND_COUNT 3
#define WAIT_BAND_STOP 0

class DYNAMIC_WAIT
{
public:
    DYNAMIC_WAIT();
    ~DYNAMIC_WAIT();

    void Wait();

private:

    static const WAIT_BAND   m_WaitBands[WAIT_BAND_COUNT];

    // Period in seconds that a new wait call is considered part of the same wait sequence
    static const UINT       m_WaitSequenceTimeInSeconds = 2;

    UINT                    m_CurrentWaitBandIdx;
    UINT                    m_WaitCountInCurrentBand;
    LARGE_INTEGER           m_QPCFrequency;
    LARGE_INTEGER           m_LastWakeUpTime;
    BOOL                    m_QPCValid;
};
const WAIT_BAND DYNAMIC_WAIT::m_WaitBands[WAIT_BAND_COUNT] = {
                                                                 {250, 20},
                                                                 {2000, 60},
                                                                 {5000, WAIT_BAND_STOP}   // Never move past this band
};

DYNAMIC_WAIT::DYNAMIC_WAIT() {
    m_QPCValid = QueryPerformanceFrequency(&m_QPCFrequency);
    m_LastWakeUpTime.QuadPart = 0L;
}

DYNAMIC_WAIT::~DYNAMIC_WAIT() {
}

void DYNAMIC_WAIT::Wait() {
    LARGE_INTEGER CurrentQPC = { 0 };

    // Is this wait being called with the period that we consider it to be part of the same wait sequence
    QueryPerformanceCounter(&CurrentQPC);
    if (m_QPCValid && (CurrentQPC.QuadPart <= (m_LastWakeUpTime.QuadPart + (m_QPCFrequency.QuadPart * m_WaitSequenceTimeInSeconds))))
    {
        // We are still in the same wait sequence, lets check if we should move to the next band
        if ((m_WaitBands[m_CurrentWaitBandIdx].WaitCount != WAIT_BAND_STOP) && (m_WaitCountInCurrentBand > m_WaitBands[m_CurrentWaitBandIdx].WaitCount))
        {
            m_CurrentWaitBandIdx++;
            m_WaitCountInCurrentBand = 0;
        }
    }
    else
    {
        // Either we could not get the current time or we are starting a new wait sequence
        m_WaitCountInCurrentBand = 0;
        m_CurrentWaitBandIdx = 0;
    }

    // Sleep for the required period of time
    Sleep(m_WaitBands[m_CurrentWaitBandIdx].WaitTime);

    // Record the time we woke up so we can detect wait sequences
    QueryPerformanceCounter(&m_LastWakeUpTime);
    m_WaitCountInCurrentBand++;
}


DWORD WINAPI startDup(LPVOID lpParam) {
    INT SingleOutput = -1;

    // Synchronization
    HANDLE UnexpectedErrorEvent = nullptr;
    HANDLE ExpectedErrorEvent = nullptr;
    HANDLE TerminateThreadsEvent = nullptr;

    // Event used by the threads to signal an unexpected error and we want to quit the app
    UnexpectedErrorEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!UnexpectedErrorEvent)
    {
        ProcessFailure(nullptr, L"UnexpectedErrorEvent creation failed", L"Error", E_UNEXPECTED);
        return 0;
    }

    // Event for when a thread encounters an expected error
    ExpectedErrorEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!ExpectedErrorEvent)
    {
        ProcessFailure(nullptr, L"ExpectedErrorEvent creation failed", L"Error", E_UNEXPECTED);
        return 0;
    }

    // Event to tell spawned threads to quit
    TerminateThreadsEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!TerminateThreadsEvent)
    {
        ProcessFailure(nullptr, L"TerminateThreadsEvent creation failed", L"Error", E_UNEXPECTED);
        return 0;
    }

   
    THREADMANAGER ThreadMgr;
    RECT DeskBounds;
    UINT OutputCount;

    // Message loop (attempts to update screen when no other messages to process)
    MSG msg = { 0 };
    bool FirstTime = true;
    DYNAMIC_WAIT DynamicWait;

    while (true)
    {
        if (WaitForSingleObjectEx(UnexpectedErrorEvent, 0, FALSE) == WAIT_OBJECT_0)
        {
            // Unexpected error occurred so exit the application
            break;
        }
        else if (FirstTime || WaitForSingleObjectEx(ExpectedErrorEvent, 0, FALSE) == WAIT_OBJECT_0)
        {
            if (!FirstTime)
            {
                // Terminate other threads
                SetEvent(TerminateThreadsEvent);
                ThreadMgr.WaitForThreadTermination();
                ResetEvent(TerminateThreadsEvent);
                ResetEvent(ExpectedErrorEvent);

                // Clean up
                ThreadMgr.Clean();
                OutMgr.CleanRefs();

                // As we have encountered an error due to a system transition we wait before trying again, using this dynamic wait
                // the wait periods will get progressively long to avoid wasting too much system resource if this state lasts a long time
                DynamicWait.Wait();
            }
            else
            {
                // First time through the loop so nothing to clean up
                FirstTime = false;
            }

            // Re-initialize
            HRESULT Ret = OutMgr.InitOutput(SingleOutput, &OutputCount, &DeskBounds);
            if (Ret == DUPL_RETURN_SUCCESS)
            {
                HANDLE SharedHandle = OutMgr.GetSharedHandle();
                if (SharedHandle)
                {
                    Ret = ThreadMgr.Initialize(SingleOutput, OutputCount, UnexpectedErrorEvent, ExpectedErrorEvent, TerminateThreadsEvent, SharedHandle, &DeskBounds);
                }
                else
                {
                    DisplayMsg(L"Failed to get handle of shared surface", L"Error", S_OK);
                    Ret = DUPL_RETURN_ERROR_UNEXPECTED;
                }
            }

        }
        else
        {
            OutMgr.UpdateApplicationWindow(lpParam);
        }
    }

    // Make sure all other threads have exited
    if (SetEvent(TerminateThreadsEvent))
    {
        ThreadMgr.WaitForThreadTermination();
    }

    // Clean up
    CloseHandle(UnexpectedErrorEvent);
    CloseHandle(ExpectedErrorEvent);
    CloseHandle(TerminateThreadsEvent);

    return 0;
}


DWORD WINAPI DDProc(_In_ void* Param) {
    // Classes
    DISPLAYMANAGER DispMgr;
    DUPLICATIONMANAGER DuplMgr;

    // D3D objects
    ID3D11Texture2D* SharedSurf = nullptr;
    IDXGIKeyedMutex* KeyMutex = nullptr;

    // Data passed in from thread creation
    THREAD_DATA* TData = reinterpret_cast<THREAD_DATA*>(Param);

    // Get desktop
    DUPL_RETURN Ret;
    HDESK CurrentDesktop = nullptr;
    CurrentDesktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
    if (!CurrentDesktop)
    {
        // We do not have access to the desktop so request a retry
        SetEvent(TData->ExpectedErrorEvent);
        Ret = DUPL_RETURN_ERROR_EXPECTED;

        if (Ret != DUPL_RETURN_SUCCESS)
        {
            if (Ret == DUPL_RETURN_ERROR_EXPECTED)
            {
                // The system is in a transition state so request the duplication be restarted
                SetEvent(TData->ExpectedErrorEvent);
            }
            else
            {
                // Unexpected error so exit the application
                SetEvent(TData->UnexpectedErrorEvent);
            }
        }

        if (SharedSurf)
        {
            SharedSurf->Release();
            SharedSurf = nullptr;
        }

        if (KeyMutex)
        {
            KeyMutex->Release();
            KeyMutex = nullptr;
        }

        return 0;
    }

    // Attach desktop to this thread
    bool DesktopAttached = SetThreadDesktop(CurrentDesktop) != 0;
    CloseDesktop(CurrentDesktop);
    CurrentDesktop = nullptr;
    if (!DesktopAttached)
    {
        // We do not have access to the desktop so request a retry
        Ret = DUPL_RETURN_ERROR_EXPECTED;

        if (Ret != DUPL_RETURN_SUCCESS)
        {
            if (Ret == DUPL_RETURN_ERROR_EXPECTED)
            {
                // The system is in a transition state so request the duplication be restarted
                SetEvent(TData->ExpectedErrorEvent);
            }
            else
            {
                // Unexpected error so exit the application
                SetEvent(TData->UnexpectedErrorEvent);
            }
        }

        if (SharedSurf)
        {
            SharedSurf->Release();
            SharedSurf = nullptr;
        }

        if (KeyMutex)
        {
            KeyMutex->Release();
            KeyMutex = nullptr;
        }

        return 0;
    }

    // New display manager
    DispMgr.InitD3D(&TData->DxRes);

    // Obtain handle to sync shared Surface
    HRESULT hr = TData->DxRes.Device->OpenSharedResource(TData->TexSharedHandle, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&SharedSurf));
    if (FAILED(hr))
    {
        Ret = ProcessFailure(TData->DxRes.Device, L"Opening shared texture failed", L"Error", hr, SystemTransitionsExpectedErrors);

        if (Ret != DUPL_RETURN_SUCCESS)
        {
            if (Ret == DUPL_RETURN_ERROR_EXPECTED)
            {
                // The system is in a transition state so request the duplication be restarted
                SetEvent(TData->ExpectedErrorEvent);
            }
            else
            {
                // Unexpected error so exit the application
                SetEvent(TData->UnexpectedErrorEvent);
            }
        }

        if (SharedSurf)
        {
            SharedSurf->Release();
            SharedSurf = nullptr;
        }

        if (KeyMutex)
        {
            KeyMutex->Release();
            KeyMutex = nullptr;
        }

        return 0;
    }

    hr = SharedSurf->QueryInterface(__uuidof(IDXGIKeyedMutex), reinterpret_cast<void**>(&KeyMutex));
    if (FAILED(hr))
    {
        Ret = ProcessFailure(nullptr, L"Failed to get keyed mutex interface in spawned thread", L"Error", hr);

        if (Ret != DUPL_RETURN_SUCCESS)
        {
            if (Ret == DUPL_RETURN_ERROR_EXPECTED)
            {
                // The system is in a transition state so request the duplication be restarted
                SetEvent(TData->ExpectedErrorEvent);
            }
            else
            {
                // Unexpected error so exit the application
                SetEvent(TData->UnexpectedErrorEvent);
            }
        }

        if (SharedSurf)
        {
            SharedSurf->Release();
            SharedSurf = nullptr;
        }

        if (KeyMutex)
        {
            KeyMutex->Release();
            KeyMutex = nullptr;
        }

        return 0;
    }

    // Make duplication manager
    Ret = DuplMgr.InitDupl(TData->DxRes.Device, TData->Output);
    if (Ret != DUPL_RETURN_SUCCESS)
    {

        if (Ret != DUPL_RETURN_SUCCESS)
        {
            if (Ret == DUPL_RETURN_ERROR_EXPECTED)
            {
                // The system is in a transition state so request the duplication be restarted
                SetEvent(TData->ExpectedErrorEvent);
            }
            else
            {
                // Unexpected error so exit the application
                SetEvent(TData->UnexpectedErrorEvent);
            }
        }

        if (SharedSurf)
        {
            SharedSurf->Release();
            SharedSurf = nullptr;
        }

        if (KeyMutex)
        {
            KeyMutex->Release();
            KeyMutex = nullptr;
        }

        return 0;
    }

    // Get output description
    DXGI_OUTPUT_DESC DesktopDesc;
    RtlZeroMemory(&DesktopDesc, sizeof(DXGI_OUTPUT_DESC));
    DuplMgr.GetOutputDesc(&DesktopDesc);

    // Main duplication loop
    bool WaitToProcessCurrentFrame = false;
    FRAME_DATA CurrentData;

    while ((WaitForSingleObjectEx(TData->TerminateThreadsEvent, 0, FALSE) == WAIT_TIMEOUT))
    {
        if (!WaitToProcessCurrentFrame)
        {
            // Get new frame from desktop duplication
            bool TimeOut;
            Ret = DuplMgr.GetFrame(&CurrentData, &TimeOut);
            if (Ret != DUPL_RETURN_SUCCESS)
            {
                // An error occurred getting the next frame drop out of loop which
                // will check if it was expected or not
                break;
            }

            // Check for timeout
            if (TimeOut)
            {
                // No new frame at the moment
                continue;
            }
        }

        // We have a new frame so try and process it
        // Try to acquire keyed mutex in order to access shared surface
        hr = KeyMutex->AcquireSync(0, 1000);
        if (hr == static_cast<HRESULT>(WAIT_TIMEOUT))
        {
            // Can't use shared surface right now, try again later
            WaitToProcessCurrentFrame = true;
            continue;
        }
        else if (FAILED(hr))
        {
            // Generic unknown failure
            Ret = ProcessFailure(TData->DxRes.Device, L"Unexpected error acquiring KeyMutex", L"Error", hr, SystemTransitionsExpectedErrors);
            DuplMgr.DoneWithFrame();
            break;
        }

        // We can now process the current frame
        WaitToProcessCurrentFrame = false;

        // Process new frame
        Ret = DispMgr.ProcessFrame(&CurrentData, SharedSurf, TData->OffsetX, TData->OffsetY, &DesktopDesc);
        if (Ret != DUPL_RETURN_SUCCESS)
        {
            DuplMgr.DoneWithFrame();
            KeyMutex->ReleaseSync(1);
            break;
        }

        // Release acquired keyed mutex
        hr = KeyMutex->ReleaseSync(1);
        if (FAILED(hr))
        {
            Ret = ProcessFailure(TData->DxRes.Device, L"Unexpected error releasing the keyed mutex", L"Error", hr, SystemTransitionsExpectedErrors);
            DuplMgr.DoneWithFrame();
            break;
        }

        // Release frame back to desktop duplication
        Ret = DuplMgr.DoneWithFrame();
        if (Ret != DUPL_RETURN_SUCCESS)
        {
            break;
        }
    }

    if (Ret != DUPL_RETURN_SUCCESS)
    {
        if (Ret == DUPL_RETURN_ERROR_EXPECTED)
        {
            // The system is in a transition state so request the duplication be restarted
            SetEvent(TData->ExpectedErrorEvent);
        }
        else
        {
            // Unexpected error so exit the application
            SetEvent(TData->UnexpectedErrorEvent);
        }
    }

    if (SharedSurf)
    {
        SharedSurf->Release();
        SharedSurf = nullptr;
    }

    if (KeyMutex)
    {
        KeyMutex->Release();
        KeyMutex = nullptr;
    }

    return 0;
}

_Post_satisfies_(return != DUPL_RETURN_SUCCESS)
DUPL_RETURN ProcessFailure(_In_opt_ ID3D11Device * Device, _In_ LPCWSTR Str, _In_ LPCWSTR Title, HRESULT hr, _In_opt_z_ HRESULT * ExpectedErrors) {
    HRESULT TranslatedHr;

    // On an error check if the DX device is lost
    if (Device)
    {
        HRESULT DeviceRemovedReason = Device->GetDeviceRemovedReason();

        switch (DeviceRemovedReason)
        {
        case DXGI_ERROR_DEVICE_REMOVED:
        case DXGI_ERROR_DEVICE_RESET:
        case static_cast<HRESULT>(E_OUTOFMEMORY):
        {
            // Our device has been stopped due to an external event on the GPU so map them all to
            // device removed and continue processing the condition
            TranslatedHr = DXGI_ERROR_DEVICE_REMOVED;
            break;
        }

        case S_OK:
        {
            // Device is not removed so use original error
            TranslatedHr = hr;
            break;
        }

        default:
        {
            // Device is removed but not a error we want to remap
            TranslatedHr = DeviceRemovedReason;
        }
        }
    }
    else
    {
        TranslatedHr = hr;
    }

    // Check if this error was expected or not
    if (ExpectedErrors)
    {
        HRESULT* CurrentResult = ExpectedErrors;

        while (*CurrentResult != S_OK)
        {
            if (*(CurrentResult++) == TranslatedHr)
            {
                return DUPL_RETURN_ERROR_EXPECTED;
            }
        }
    }

    // Error was not expected so display the message box
    DisplayMsg(Str, Title, TranslatedHr);

    return DUPL_RETURN_ERROR_UNEXPECTED;
}

//
// Displays a message
//
void DisplayMsg(_In_ LPCWSTR Str, _In_ LPCWSTR Title, HRESULT hr) {
    if (SUCCEEDED(hr))
    {
        MessageBoxW(nullptr, Str, Title, MB_OK);
        return;
    }

    const UINT StringLen = (UINT)(wcslen(Str) + sizeof(" with HRESULT 0x########."));
    wchar_t* OutStr = new wchar_t[StringLen];
    if (!OutStr)
    {
        return;
    }

    INT LenWritten = swprintf_s(OutStr, StringLen, L"%s with 0x%X.", Str, hr);
    if (LenWritten != -1)
    {
        MessageBoxW(nullptr, OutStr, Title, MB_OK);
    }

    delete[] OutStr;
}

