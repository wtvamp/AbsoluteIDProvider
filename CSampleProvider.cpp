//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// CSampleProvider implements ICredentialProvider, which is the main
// interface that logonUI uses to decide which tiles to display.
// In this sample, we will display one tile that uses each of the nine
// available UI controls.
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define _WINSOCKAPI_        // Prevent inclusion of winsock.h

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bluetoothapis.h> // Windows Bluetooth API
#include <iostream>
#include <thread>
#include <initguid.h>
#include "CSampleProvider.h"
#include "CSampleCredential.h"
#include "guid.h"


#pragma comment(lib, "Bthprops.lib") // Link Bluetooth library
#pragma comment(lib, "Ws2_32.lib") // Link Winsock library

CSampleProvider::CSampleProvider():
    _cRef(1),
    _pCredential(nullptr),
    _pCredProviderUserArray(nullptr)
{
    DllAddRef();
}

CSampleProvider::~CSampleProvider()
{
    if (_pCredential != nullptr)
    {
        _pCredential->Release();
        _pCredential = nullptr;
    }
    if (_pCredProviderUserArray != nullptr)
    {
        _pCredProviderUserArray->Release();
        _pCredProviderUserArray = nullptr;
    }

    DllRelease();
}

// SetUsageScenario is the provider's cue that it's going to be asked for tiles
// in a subsequent call.
HRESULT CSampleProvider::SetUsageScenario(
    CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
    DWORD /*dwFlags*/)
{
    HRESULT hr;

    // Decide which scenarios to support here. Returning E_NOTIMPL simply tells the caller
    // that we're not designed for that scenario.
    switch (cpus)
    {
    case CPUS_LOGON:
    case CPUS_UNLOCK_WORKSTATION:
        // The reason why we need _fRecreateEnumeratedCredentials is because ICredentialProviderSetUserArray::SetUserArray() is called after ICredentialProvider::SetUsageScenario(),
        // while we need the ICredentialProviderUserArray during enumeration in ICredentialProvider::GetCredentialCount()
        _cpus = cpus;
        _fRecreateEnumeratedCredentials = true;
        hr = S_OK;
        break;

    case CPUS_CHANGE_PASSWORD:
    case CPUS_CREDUI:
        hr = E_NOTIMPL;
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}

// SetSerialization takes the kind of buffer that you would normally return to LogonUI for
// an authentication attempt.  It's the opposite of ICredentialProviderCredential::GetSerialization.
// GetSerialization is implement by a credential and serializes that credential.  Instead,
// SetSerialization takes the serialization and uses it to create a tile.
//
// SetSerialization is called for two main scenarios.  The first scenario is in the credui case
// where it is prepopulating a tile with credentials that the user chose to store in the OS.
// The second situation is in a remote logon case where the remote client may wish to
// prepopulate a tile with a username, or in some cases, completely populate the tile and
// use it to logon without showing any UI.
//
// If you wish to see an example of SetSerialization, please see either the SampleCredentialProvider
// sample or the SampleCredUICredentialProvider sample.  [The logonUI team says, "The original sample that
// this was built on top of didn't have SetSerialization.  And when we decided SetSerialization was
// important enough to have in the sample, it ended up being a non-trivial amount of work to integrate
// it into the main sample.  We felt it was more important to get these samples out to you quickly than to
// hold them in order to do the work to integrate the SetSerialization changes from SampleCredentialProvider
// into this sample.]
HRESULT CSampleProvider::SetSerialization(
    _In_ CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION const * /*pcpcs*/)
{
    return E_NOTIMPL;
}

// Called by LogonUI to give you a callback.  Providers often use the callback if they
// some event would cause them to need to change the set of tiles that they enumerated.
HRESULT CSampleProvider::Advise(
    _In_ ICredentialProviderEvents * /*pcpe*/,
    _In_ UINT_PTR /*upAdviseContext*/)
{
    return E_NOTIMPL;
}

// Called by LogonUI when the ICredentialProviderEvents callback is no longer valid.
HRESULT CSampleProvider::UnAdvise()
{
    return E_NOTIMPL;
}

// Called by LogonUI to determine the number of fields in your tiles.  This
// does mean that all your tiles must have the same number of fields.
// This number must include both visible and invisible fields. If you want a tile
// to have different fields from the other tiles you enumerate for a given usage
// scenario you must include them all in this count and then hide/show them as desired
// using the field descriptors.
HRESULT CSampleProvider::GetFieldDescriptorCount(
    _Out_ DWORD *pdwCount)
{
    *pdwCount = SFI_NUM_FIELDS;
    return S_OK;
}

// Gets the field descriptor for a particular field.
HRESULT CSampleProvider::GetFieldDescriptorAt(
    DWORD dwIndex,
    _Outptr_result_nullonfailure_ CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR **ppcpfd)
{
    HRESULT hr;
    *ppcpfd = nullptr;

    // Verify dwIndex is a valid field.
    if ((dwIndex < SFI_NUM_FIELDS) && ppcpfd)
    {
        hr = FieldDescriptorCoAllocCopy(s_rgCredProvFieldDescriptors[dwIndex], ppcpfd);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

// Sets pdwCount to the number of tiles that we wish to show at this time.
// Sets pdwDefault to the index of the tile which should be used as the default.
// The default tile is the tile which will be shown in the zoomed view by default. If
// more than one provider specifies a default the last used cred prov gets to pick
// the default. If *pbAutoLogonWithDefault is TRUE, LogonUI will immediately call
// GetSerialization on the credential you've specified as the default and will submit
// that credential for authentication without showing any further UI.
HRESULT CSampleProvider::GetCredentialCount(
    _Out_ DWORD *pdwCount,
    _Out_ DWORD *pdwDefault,
    _Out_ BOOL *pbAutoLogonWithDefault)
{
    *pdwDefault = CREDENTIAL_PROVIDER_NO_DEFAULT;
    *pbAutoLogonWithDefault = FALSE;

    if (_fRecreateEnumeratedCredentials)
    {
        _fRecreateEnumeratedCredentials = false;
        _ReleaseEnumeratedCredentials();
        _CreateEnumeratedCredentials();
    }

    *pdwCount = 1;

    return S_OK;
}

// Returns the credential at the index specified by dwIndex. This function is called by logonUI to enumerate
// the tiles.
HRESULT CSampleProvider::GetCredentialAt(
    DWORD dwIndex,
    _Outptr_result_nullonfailure_ ICredentialProviderCredential **ppcpc)
{
    HRESULT hr = E_INVALIDARG;
    *ppcpc = nullptr;

    if ((dwIndex == 0) && ppcpc)
    {
        hr = _pCredential->QueryInterface(IID_PPV_ARGS(ppcpc));
    }
    return hr;
}

// This function will be called by LogonUI after SetUsageScenario succeeds.
// Sets the User Array with the list of users to be enumerated on the logon screen.
HRESULT CSampleProvider::SetUserArray(_In_ ICredentialProviderUserArray *users)
{
    if (_pCredProviderUserArray)
    {
        _pCredProviderUserArray->Release();
    }
    _pCredProviderUserArray = users;
    _pCredProviderUserArray->AddRef();
    return S_OK;
}

// Update _CreateEnumeratedCredentials to include initialization of new features
void CSampleProvider::_CreateEnumeratedCredentials()
{
    InitializeBluetoothProximityCheck();
    InitializeReactNativeAppCommunication();

    switch (_cpus)
    {
    case CPUS_LOGON:
    case CPUS_UNLOCK_WORKSTATION:
    {
        _EnumerateCredentials();
        break;
    }
    default:
        break;
    }
}

void CSampleProvider::_ReleaseEnumeratedCredentials()
{
    if (_pCredential != nullptr)
    {
        _pCredential->Release();
        _pCredential = nullptr;
    }
}

HRESULT CSampleProvider::_EnumerateCredentials()
{
    HRESULT hr = E_UNEXPECTED;
    if (_pCredProviderUserArray != nullptr)
    {
        DWORD dwUserCount;
        _pCredProviderUserArray->GetCount(&dwUserCount);
        if (dwUserCount > 0)
        {
            ICredentialProviderUser *pCredUser;
            hr = _pCredProviderUserArray->GetAt(0, &pCredUser);
            if (SUCCEEDED(hr))
            {
                _pCredential = new(std::nothrow) CSampleCredential();
                if (_pCredential != nullptr)
                {
                    hr = _pCredential->Initialize(_cpus, s_rgCredProvFieldDescriptors, s_rgFieldStatePairs, pCredUser);
                    if (FAILED(hr))
                    {
                        _pCredential->Release();
                        _pCredential = nullptr;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
                pCredUser->Release();
            }
        }
    }
    return hr;
}

// Boilerplate code to create our provider.
HRESULT CSample_CreateInstance(_In_ REFIID riid, _Outptr_ void **ppv)
{
    HRESULT hr;
    CSampleProvider *pProvider = new(std::nothrow) CSampleProvider();
    if (pProvider)
    {
        hr = pProvider->QueryInterface(riid, ppv);
        pProvider->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}


// Placeholder function to initialize Bluetooth Proximity Check
void CSampleProvider::InitializeBluetoothProximityCheck()
{
    std::wcout << L"Initializing Bluetooth Proximity Check..." << std::endl;

    // Initialize Bluetooth APIs
    HANDLE hRadio = NULL;
    BLUETOOTH_FIND_RADIO_PARAMS btfrp = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };

    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&btfrp, &hRadio);
    if (hFind)
    {
        std::wcout << L"Bluetooth radio found and initialized successfully." << std::endl;
        BluetoothFindRadioClose(hFind);
        CloseHandle(hRadio);
    }
    else
    {
        std::wcerr << L"Failed to initialize Bluetooth radio. Ensure Bluetooth is enabled." << std::endl;
    }
}

// Placeholder function to initialize React Native App Communication
void CSampleProvider::InitializeReactNativeAppCommunication()
{
    std::wcout << L"Initializing communication with the React Native app..." << std::endl;

    std::thread([]() {
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            std::wcerr << L"WSAStartup failed: " << iResult << std::endl;
            return;
        }

        struct addrinfo* result = NULL, hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        iResult = getaddrinfo(NULL, "8080", &hints, &result);
        if (iResult != 0) {
            std::wcerr << L"getaddrinfo failed: " << iResult << std::endl;
            WSACleanup();
            return;
        }

        SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
            std::wcerr << L"Socket failed: " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            WSACleanup();
            return;
        }

        iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            std::wcerr << L"Bind failed: " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            return;
        }

        freeaddrinfo(result);

        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            std::wcerr << L"Listen failed: " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            WSACleanup();
            return;
        }

        std::wcout << L"Listening for events on port 8080..." << std::endl;

        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            std::wcerr << L"Accept failed: " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            WSACleanup();
            return;
        }

        char recvbuf[512];
        int recvbuflen = 512;
        int bytesReceived = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (bytesReceived > 0) {
            std::wcout << L"Received event: " << recvbuf << std::endl;
        }
        else {
            std::wcerr << L"Receive failed: " << WSAGetLastError() << std::endl;
        }

        closesocket(ClientSocket);
        closesocket(ListenSocket);
        WSACleanup();
        }).detach();

    std::wcout << L"React Native app communication initialized successfully." << std::endl;

}