// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "OutputManager.h"
using namespace DirectX;

//
// Constructor NULLs out all pointers & sets appropriate var vals
//
OUTPUTMANAGER::OUTPUTMANAGER() : 
                                 m_Device(nullptr),
                                 m_Factory(nullptr),
                                 m_DeviceContext(nullptr),
                                 m_SharedSurf(nullptr),
                                 m_KeyMutex(nullptr ) 
{
}

//
// Destructor which calls CleanRefs to release all references and memory.
//
OUTPUTMANAGER::~OUTPUTMANAGER()
{
    CleanRefs();
}


//
// Initialize all state
//
DUPL_RETURN OUTPUTMANAGER::InitOutput(INT SingleOutput, _Out_ UINT* OutCount, _Out_ RECT* DeskBounds)
{
    HRESULT hr;
    D3D_FEATURE_LEVEL FeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_1
    };
    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
    D3D_FEATURE_LEVEL FeatureLevel;
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, FeatureLevels, NumFeatureLevels,
        D3D11_SDK_VERSION, &m_Device, &FeatureLevel, &m_DeviceContext);
    if (FAILED(hr))
    {
        return ProcessFailure(m_Device, L"Device creation in OUTPUTMANAGER failed", L"Error", hr, SystemTransitionsExpectedErrors);
    }

    // Get DXGI factory
    IDXGIDevice* DxgiDevice = nullptr;
    hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
    if (FAILED(hr))
    {
        return ProcessFailure(nullptr, L"Failed to QI for DXGI Device", L"Error", hr, nullptr);
    }

    IDXGIAdapter* DxgiAdapter = nullptr;
    hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
    DxgiDevice->Release();
    DxgiDevice = nullptr;
    if (FAILED(hr))
    {
        return ProcessFailure(m_Device, L"Failed to get parent DXGI Adapter", L"Error", hr, SystemTransitionsExpectedErrors);
    }

    hr = DxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&m_Factory));
    DxgiAdapter->Release();
    DxgiAdapter = nullptr;
    if (FAILED(hr))
    {
        return ProcessFailure(m_Device, L"Failed to get parent DXGI Factory", L"Error", hr, SystemTransitionsExpectedErrors);
    }

    // Create shared texture
    DUPL_RETURN Return = CreateSharedSurf(SingleOutput, OutCount, DeskBounds);
    if (Return != DUPL_RETURN_SUCCESS)
    {
        return Return;
    }

    return Return;
}

//
// Recreate shared texture
//
DUPL_RETURN OUTPUTMANAGER::CreateSharedSurf(INT SingleOutput, _Out_ UINT* OutCount, _Out_ RECT* DeskBounds)
{
    HRESULT hr;

    // Get DXGI resources
    IDXGIDevice* DxgiDevice = nullptr;
    hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
    if (FAILED(hr))
    {
        return ProcessFailure(nullptr, L"Failed to QI for DXGI Device", L"Error", hr);
    }

    IDXGIAdapter* DxgiAdapter = nullptr;
    hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
    DxgiDevice->Release();
    DxgiDevice = nullptr;
    if (FAILED(hr))
    {
        return ProcessFailure(m_Device, L"Failed to get parent DXGI Adapter", L"Error", hr, SystemTransitionsExpectedErrors);
    }

    // Set initial values so that we always catch the right coordinates
    DeskBounds->left = INT_MAX;
    DeskBounds->right = INT_MIN;
    DeskBounds->top = INT_MAX;
    DeskBounds->bottom = INT_MIN;

    IDXGIOutput* DxgiOutput = nullptr;

    // Figure out right dimensions for full size desktop texture and # of outputs to duplicate
    UINT OutputCount;
    if (SingleOutput < 0)
    {
        hr = S_OK;
        for (OutputCount = 0; SUCCEEDED(hr); ++OutputCount)
        {
            if (DxgiOutput)
            {
                DxgiOutput->Release();
                DxgiOutput = nullptr;
            }
            hr = DxgiAdapter->EnumOutputs(OutputCount, &DxgiOutput);
            if (DxgiOutput && (hr != DXGI_ERROR_NOT_FOUND))
            {
                DXGI_OUTPUT_DESC DesktopDesc;
                DxgiOutput->GetDesc(&DesktopDesc);

                DeskBounds->left = min(DesktopDesc.DesktopCoordinates.left, DeskBounds->left);
                DeskBounds->top = min(DesktopDesc.DesktopCoordinates.top, DeskBounds->top);
                DeskBounds->right = max(DesktopDesc.DesktopCoordinates.right, DeskBounds->right);
                DeskBounds->bottom = max(DesktopDesc.DesktopCoordinates.bottom, DeskBounds->bottom);
            }
        }

        --OutputCount;
    }
    else
    {
        hr = DxgiAdapter->EnumOutputs(SingleOutput, &DxgiOutput);
        if (FAILED(hr))
        {
            DxgiAdapter->Release();
            DxgiAdapter = nullptr;
            return ProcessFailure(m_Device, L"Output specified to be duplicated does not exist", L"Error", hr);
        }
        DXGI_OUTPUT_DESC DesktopDesc;
        DxgiOutput->GetDesc(&DesktopDesc);
        *DeskBounds = DesktopDesc.DesktopCoordinates;

        DxgiOutput->Release();
        DxgiOutput = nullptr;

        OutputCount = 1;
    }

    DxgiAdapter->Release();
    DxgiAdapter = nullptr;

    // Set passed in output count variable
    *OutCount = OutputCount;

    if (OutputCount == 0)
    {
        // We could not find any outputs, the system must be in a transition so return expected error
        // so we will attempt to recreate
        return DUPL_RETURN_ERROR_EXPECTED;
    }

    height = DeskBounds->bottom - DeskBounds->top;

    // Create shared texture for all duplication threads to draw into
    D3D11_TEXTURE2D_DESC DeskTexD;
    RtlZeroMemory(&DeskTexD, sizeof(D3D11_TEXTURE2D_DESC));
    DeskTexD.Width = DeskBounds->right - DeskBounds->left;
    DeskTexD.Height = DeskBounds->bottom - DeskBounds->top;
    DeskTexD.MipLevels = 1;
    DeskTexD.ArraySize = 1;
    DeskTexD.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    DeskTexD.SampleDesc.Count = 1;
    DeskTexD.Usage = D3D11_USAGE_DEFAULT;
    DeskTexD.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    DeskTexD.CPUAccessFlags = 0;
    DeskTexD.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

    hr = m_Device->CreateTexture2D(&DeskTexD, nullptr, &m_SharedSurf);
    if (FAILED(hr))
    {
        if (OutputCount != 1)
        {
            // If we are duplicating the complete desktop we try to create a single texture to hold the
            // complete desktop image and blit updates from the per output DDA interface.  The GPU can
            // always support a texture size of the maximum resolution of any single output but there is no
            // guarantee that it can support a texture size of the desktop.
            // The sample only use this large texture to display the desktop image in a single window using DX
            // we could revert back to using GDI to update the window in this failure case.
            return ProcessFailure(m_Device, L"Failed to create DirectX shared texture - we are attempting to create a texture the size of the complete desktop and this may be larger than the maximum texture size of your GPU.  Please try again using the -output command line parameter to duplicate only 1 monitor or configure your computer to a single monitor configuration", L"Error", hr, SystemTransitionsExpectedErrors);
        }
        else
        {
            return ProcessFailure(m_Device, L"Failed to create shared texture", L"Error", hr, SystemTransitionsExpectedErrors);
        }
    }

    D3D11_TEXTURE2D_DESC desc2;
    RtlZeroMemory(&desc2, sizeof(D3D11_TEXTURE2D_DESC));
    desc2.Width = DeskBounds->right - DeskBounds->left;
    desc2.Height = DeskBounds->bottom - DeskBounds->top;
    desc2.MipLevels = 1;
    desc2.ArraySize = 1;
    desc2.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc2.SampleDesc.Count = 1;
    desc2.Usage = D3D11_USAGE_STAGING;
    desc2.BindFlags = 0;
    desc2.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc2.MiscFlags = 0;
    hr = m_Device->CreateTexture2D(&desc2, nullptr, &m_StagingSurf);

    if (FAILED(hr))
    {
        if (OutputCount != 1)
        {
            // If we are duplicating the complete desktop we try to create a single texture to hold the
            // complete desktop image and blit updates from the per output DDA interface.  The GPU can
            // always support a texture size of the maximum resolution of any single output but there is no
            // guarantee that it can support a texture size of the desktop.
            // The sample only use this large texture to display the desktop image in a single window using DX
            // we could revert back to using GDI to update the window in this failure case.
            return ProcessFailure(m_Device, L"Failed to create 123 shared texture - we are attempting to create a texture the size of the complete desktop and this may be larger than the maximum texture size of your GPU.  Please try again using the -output command line parameter to duplicate only 1 monitor or configure your computer to a single monitor configuration", L"Error", hr, SystemTransitionsExpectedErrors);
        }
        else
        {
            return ProcessFailure(m_Device, L"Failed to create 321d texture", L"Error", hr, SystemTransitionsExpectedErrors);
        }
    }



    // Get keyed mutex
    hr = m_SharedSurf->QueryInterface(__uuidof(IDXGIKeyedMutex), reinterpret_cast<void**>(&m_KeyMutex));
    if (FAILED(hr))
    {
        return ProcessFailure(m_Device, L"Failed to query for keyed mutex in OUTPUTMANAGER", L"Error", hr);
    }

    return DUPL_RETURN_SUCCESS;
}

//
// Present to the application window
//
DUPL_RETURN OUTPUTMANAGER::UpdateApplicationWindow(LPVOID px)
{
    // In a typical desktop duplication application there would be an application running on one system collecting the desktop images
    // and another application running on a different system that receives the desktop images via a network and display the image. This
    // sample contains both these aspects into a single application.
    // This routine is the part of the sample that displays the desktop image onto the display

    // Try and acquire sync on common display buffer
    HRESULT hr = m_KeyMutex->AcquireSync(1, 100);
    if (hr == static_cast<HRESULT>(WAIT_TIMEOUT))
    {
        // Another thread has the keyed mutex so try again later
        return DUPL_RETURN_SUCCESS;
    }
    else if (FAILED(hr))
    {
        return ProcessFailure(m_Device, L"Failed to acquire Keyed mutex in OUTPUTMANAGER", L"Error", hr, SystemTransitionsExpectedErrors);
    }

    // Got mutex, so draw
    m_DeviceContext->CopyResource(m_StagingSurf, m_SharedSurf);
    ((MPIC*)px)->height = height;
    ((MPIC*)px)->wait = true;
    ((MPIC*)px)->rsc.RowPitch = 0;
    ((MPIC*)px)->rsc.DepthPitch = 0;
    ((MPIC*)px)->rsc.pData = nullptr;
    hr = m_DeviceContext->Map(m_StagingSurf, 0, D3D11_MAP_READ, 0, &((MPIC*)px)->rsc);
    ((MPIC*)px)->startupWait = false;
    while (((MPIC*)px)->wait) {
        Sleep(0);
    }
    m_DeviceContext->Unmap(m_StagingSurf, 0);
    // Release keyed mutex
    hr = m_KeyMutex->ReleaseSync(0);
    if (FAILED(hr))
    {
        return ProcessFailure(m_Device, L"Failed to Release Keyed mutex in OUTPUTMANAGER", L"Error", hr, SystemTransitionsExpectedErrors);
    }


    return DUPL_RETURN_SUCCESS;
}

//
// Returns shared handle
//
HANDLE OUTPUTMANAGER::GetSharedHandle()
{
    HANDLE Hnd = nullptr;

    // QI IDXGIResource interface to synchronized shared surface.
    IDXGIResource* DXGIResource = nullptr;
    HRESULT hr = m_SharedSurf->QueryInterface(__uuidof(IDXGIResource), reinterpret_cast<void**>(&DXGIResource));
    if (SUCCEEDED(hr))
    {
        // Obtain handle to IDXGIResource object.
        DXGIResource->GetSharedHandle(&Hnd);
        DXGIResource->Release();
        DXGIResource = nullptr;
    }

    return Hnd;
}

//
// Releases all references
//
void OUTPUTMANAGER::CleanRefs()
{
    if (m_DeviceContext)
    {
        m_DeviceContext->Release();
        m_DeviceContext = nullptr;
    }

    if (m_Device)
    {
        m_Device->Release();
        m_Device = nullptr;
    }

    if (m_SharedSurf)
    {
        m_SharedSurf->Release();
        m_SharedSurf = nullptr;
    }

    if (m_StagingSurf)
    {
        m_StagingSurf->Release();
        m_StagingSurf = nullptr;
    }

    if (m_KeyMutex)
    {
        m_KeyMutex->Release();
        m_KeyMutex = nullptr;
    }

    if (m_Factory)
    {
        m_Factory->Release();
        m_Factory = nullptr;
    }
}
