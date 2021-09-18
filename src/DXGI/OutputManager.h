// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#ifndef _OUTPUTMANAGER_H_
#define _OUTPUTMANAGER_H_

#include <stdio.h>

#include "CommonTypes.h"
#include "warning.h"

//
// Handles the task of drawing into a window.
// Has the functionality to draw the mouse given a mouse shape buffer and position
//
class OUTPUTMANAGER
{
    public:
        OUTPUTMANAGER();
        ~OUTPUTMANAGER();
        DUPL_RETURN InitOutput(INT SingleOutput, _Out_ UINT* OutCount, _Out_ RECT* DeskBounds);
        DUPL_RETURN UpdateApplicationWindow(LPVOID px);
        void CleanRefs();
        HANDLE GetSharedHandle();

    private:
    // Methods
        DUPL_RETURN CreateSharedSurf(INT SingleOutput, _Out_ UINT* OutCount, _Out_ RECT* DeskBounds);

    // Vars
        ID3D11Device* m_Device;
        IDXGIFactory2* m_Factory;
        ID3D11DeviceContext* m_DeviceContext;
        ID3D11Texture2D* m_SharedSurf;
        ID3D11Texture2D* m_StagingSurf;
        IDXGIKeyedMutex* m_KeyMutex;
        int height;
        MPIC pic;
};

#endif
