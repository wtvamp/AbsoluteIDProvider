//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define _WINSOCKAPI_        // Prevent inclusion of winsock.h

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bluetoothapis.h> // Windows Bluetooth API
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <initguid.h>
#include "CSampleProvider.h"
#include "CSampleCredential.h"
#include "guid.h"

#pragma comment(lib, "Bthprops.lib") // Link Bluetooth library
#pragma comment(lib, "Ws2_32.lib")     // Link Winsock library

//
// CSampleProvider Implementation
//

// Constructor for CSampleProvider.
CSampleProvider::CSampleProvider() :
    _cRef(1),
    _pCredential(nullptr),
    _pCredProviderUserArray(nullptr),
    _pCredProviderEvents(nullptr),
    isLoggedIn(false),
    isBluetoothDeviceInProximity(false),
    _fRecreateEnumeratedCredentials(true)
{
    DllAddRef();
}

// Destructor for CSampleProvider.
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
    if (_pCredProviderEvents)
    {
        _pCredProviderEvents->Release();
        _pCredProviderEvents = nullptr;
    }
    DllRelease();
}

// SetUsageScenario tells us which scenario (logon or unlock) we are in.
HRESULT CSampleProvider::SetUsageScenario(
    CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
    DWORD /*dwFlags*/)
{
    HRESULT hr;

    switch (cpus)
    {
    case CPUS_LOGON:
    case CPUS_UNLOCK_WORKSTATION:
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

// SetSerialization is not implemented in this sample.
HRESULT CSampleProvider::SetSerialization(
    _In_ CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION const* /*pcpcs*/)
{
    return E_NOTIMPL;
}

// Advise: LogonUI calls this to provide a callback for when our credentials change.
HRESULT CSampleProvider::Advise(
    _In_ ICredentialProviderEvents *pcpe,
    _In_ UINT_PTR upAdviseContext)
{
    if (pcpe)
    {
        _pCredProviderEvents = pcpe;
        _pCredProviderEvents->AddRef();
        _upAdviseContext = upAdviseContext; // Store the context
    }
    return S_OK;
}


// UnAdvise: LogonUI calls this to indicate that the ICredentialProviderEvents callback is no longer valid.
HRESULT CSampleProvider::UnAdvise()
{
    if (_pCredProviderEvents)
    {
        _pCredProviderEvents->Release();
        _pCredProviderEvents = nullptr;
    }
    return S_OK;
}

// GetFieldDescriptorCount: Returns the number of fields in our tile.
HRESULT CSampleProvider::GetFieldDescriptorCount(
    _Out_ DWORD* pdwCount)
{
    *pdwCount = SFI_NUM_FIELDS;
    return S_OK;
}

// GetFieldDescriptorAt: Returns the field descriptor for a specific field.
HRESULT CSampleProvider::GetFieldDescriptorAt(
    DWORD dwIndex,
    _Outptr_result_nullonfailure_ CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd)
{
    HRESULT hr;
    *ppcpfd = nullptr;

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

// GetCredentialCount: Returns the number of tiles to show.
HRESULT CSampleProvider::GetCredentialCount(
    _Out_ DWORD* pdwCount,
    _Out_ DWORD* pdwDefault,
    _Out_ BOOL* pbAutoLogonWithDefault)
{
    // If our enumeration needs to be refreshed, do so.
    if (_fRecreateEnumeratedCredentials)
    {
        _fRecreateEnumeratedCredentials = false;
        _ReleaseEnumeratedCredentials();
        _CreateEnumeratedCredentials();
    }

    // We are only showing one tile in this provider.
    *pdwCount = 1;

    // If the phone is in proximity and we have received the "User logged in" event,
    // then we want to auto logon.
    if (isLoggedIn && isBluetoothDeviceInProximity)
    {
        *pdwDefault = 0;               // Use our only tile as the default.
        *pbAutoLogonWithDefault = TRUE;  // Trigger auto logon.
    }
    else
    {
        *pdwDefault = CREDENTIAL_PROVIDER_NO_DEFAULT;
        *pbAutoLogonWithDefault = FALSE;
    }

    return S_OK;
}


// GetCredentialAt: Returns the credential at the specified index.
HRESULT CSampleProvider::GetCredentialAt(
    DWORD dwIndex,
    _Outptr_result_nullonfailure_ ICredentialProviderCredential** ppcpc)
{
    HRESULT hr = E_INVALIDARG;
    *ppcpc = nullptr;

    if ((dwIndex == 0) && ppcpc)
    {
        hr = _pCredential->QueryInterface(IID_PPV_ARGS(ppcpc));
    }
    return hr;
}

// SetUserArray: Called by LogonUI to pass in the array of users.
HRESULT CSampleProvider::SetUserArray(_In_ ICredentialProviderUserArray* users)
{
    if (_pCredProviderUserArray)
    {
        _pCredProviderUserArray->Release();
    }
    _pCredProviderUserArray = users;
    _pCredProviderUserArray->AddRef();
    return S_OK;
}

// _CreateEnumeratedCredentials: Creates the credential tiles.
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

// _ReleaseEnumeratedCredentials: Releases any enumerated credentials.
void CSampleProvider::_ReleaseEnumeratedCredentials()
{
    if (_pCredential != nullptr)
    {
        _pCredential->Release();
        _pCredential = nullptr;
    }
}

// _EnumerateCredentials: Enumerates users and creates a credential for each.
HRESULT CSampleProvider::_EnumerateCredentials()
{
    HRESULT hr = E_UNEXPECTED;

    if (_pCredProviderUserArray != nullptr)
    {
        DWORD dwUserCount = 0;
        hr = _pCredProviderUserArray->GetCount(&dwUserCount);

        if (SUCCEEDED(hr) && dwUserCount > 0)
        {
            for (DWORD i = 0; i < dwUserCount; i++)
            {
                ICredentialProviderUser* pCredUser = nullptr;
                hr = _pCredProviderUserArray->GetAt(i, &pCredUser);
                if (SUCCEEDED(hr) && pCredUser != nullptr)
                {
                    CSampleCredential* pCredential = new (std::nothrow) CSampleCredential();
                    if (pCredential != nullptr)
                    {
                        hr = pCredential->Initialize(_cpus, s_rgCredProvFieldDescriptors, s_rgFieldStatePairs, pCredUser);
                        if (SUCCEEDED(hr))
                        {
                            RegisterCredential(pCredential);
                            _pCredential = pCredential;
                        }
                        else
                        {
                            pCredential->Release();
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
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

// RegisterCredential: Stores a credential pointer for later notifications.
void CSampleProvider::RegisterCredential(CSampleCredential* pCredential)
{
    _credentials.push_back(pCredential);
}

// NotifyCredentials: Notifies all stored credentials of a state change.
void CSampleProvider::NotifyCredentials()
{
    for (auto* credential : _credentials)
    {
        if (credential)
        {
            // Pass the updated state (isLoggedIn) to the credential so it can update its UI.
            credential->OnProviderStateChange(isLoggedIn);
        }
        if (credential == _pCredential)
        {
            // Optionally, trigger serialization for this credential.
            CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE cpgsr;
            CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION cpcs;
            PWSTR pwszOptionalStatusText = nullptr;
            CREDENTIAL_PROVIDER_STATUS_ICON cpsiOptionalStatusIcon;

            HRESULT hr = _pCredential->GetSerialization(&cpgsr, &cpcs, &pwszOptionalStatusText, &cpsiOptionalStatusIcon);
            if (SUCCEEDED(hr))
            {
                CoTaskMemFree(cpcs.rgbSerialization);
                if (pwszOptionalStatusText)
                {
                    CoTaskMemFree(pwszOptionalStatusText);
                }
            }
        }
    }
}

// UpdateStateFromEvent: Called when an HTTP event is received from the React Native app.
void CSampleProvider::UpdateStateFromEvent(const std::string& event)
{
    if (event.find("User logged in") != std::string::npos)
    {
        isLoggedIn = true;
        // Notify LogonUI that our credentials have changed, using the stored context.
        if (_pCredProviderEvents)
        {
            _pCredProviderEvents->CredentialsChanged(_upAdviseContext);
        }
    }
}

// ---------------------------------------------------
// Bluetooth Proximity Check and React Native App Communication
// ---------------------------------------------------

void CSampleProvider::InitializeBluetoothProximityCheck()
{
    isBluetoothDeviceInProximity = false;
    OutputDebugStringW(L"Initializing Bluetooth Proximity Check...\n");

    // Initialize Bluetooth APIs
    HANDLE hRadio = NULL;
    BLUETOOTH_FIND_RADIO_PARAMS btfrp = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };

    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&btfrp, &hRadio);
    if (hFind)
    {
        OutputDebugStringW(L"Bluetooth radio found and initialized successfully.\n");
        BluetoothFindRadioClose(hFind);
    }
    else
    {
        OutputDebugStringW(L"Failed to initialize Bluetooth radio. Ensure Bluetooth is enabled.\n");
        return;
    }

    // Start a thread to periodically check Bluetooth proximity.
    std::thread([this, hRadio]() {
        OutputDebugStringW(L"Checking Bluetooth proximity...\n");
        while (!isBluetoothDeviceInProximity)
        {
            OutputDebugStringW(L"Scanning for nearby Bluetooth devices...\n");

            BLUETOOTH_DEVICE_SEARCH_PARAMS btdsp = { sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS) };
            BLUETOOTH_DEVICE_INFO btdi = { sizeof(BLUETOOTH_DEVICE_INFO) };
            bool deviceFound = false;

            btdsp.hRadio = hRadio;
            btdsp.fReturnAuthenticated = TRUE;
            btdsp.fReturnRemembered = TRUE;
            btdsp.fReturnUnknown = TRUE;
            btdsp.fReturnConnected = TRUE;
            btdsp.fIssueInquiry = TRUE;
            btdsp.cTimeoutMultiplier = 5; // About 6.4 seconds

            HBLUETOOTH_DEVICE_FIND hFindDevice = BluetoothFindFirstDevice(&btdsp, &btdi);
            if (hFindDevice)
            {
                do
                {
                    std::wstring debugMsg = L"Found Bluetooth device: ";
                    debugMsg += btdi.szName;
                    OutputDebugStringW(debugMsg.c_str());
                    if (wcscmp(btdi.szName, L"Warren Thompson’s iPhone") == 0)
                    {
                        OutputDebugStringW(L"Target device found!\n");
                        deviceFound = true;
                        break;
                    }
                } while (BluetoothFindNextDevice(hFindDevice, &btdi));

                BluetoothFindDeviceClose(hFindDevice);
            }

            isBluetoothDeviceInProximity = deviceFound;
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }

        CloseHandle(hRadio);
        }).detach();
}

void LogWSAError(const wchar_t* msg)
{
    wchar_t* s = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&s, 0, nullptr);
    std::wstring fullMsg = std::wstring(msg) + L": " + s;
    OutputDebugStringW(fullMsg.c_str());
    LocalFree(s);
}

void CSampleProvider::InitializeReactNativeAppCommunication()
{
    OutputDebugStringW(L"Starting HTTP server to listen for React Native app events...\n");

    std::thread([this]() {
        WSADATA wsaData;
        int iResult;

        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            LogWSAError(L"WSAStartup failed");
            return;
        }

        struct addrinfo* result = NULL, hints;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        iResult = getaddrinfo(NULL, "32808", &hints, &result);
        if (iResult != 0) {
            LogWSAError(L"getaddrinfo failed");
            WSACleanup();
            return;
        }

        SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
            LogWSAError(L"Socket creation failed");
            freeaddrinfo(result);
            WSACleanup();
            return;
        }

        int optval = 1;
        iResult = setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
        if (iResult == SOCKET_ERROR) {
            LogWSAError(L"setsockopt failed");
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            return;
        }

        iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            LogWSAError(L"Bind failed");
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            return;
        }

        freeaddrinfo(result);

        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            LogWSAError(L"Listen failed");
            closesocket(ListenSocket);
            WSACleanup();
            return;
        }

        OutputDebugStringW(L"HTTP server listening on port 32808...\n");

        while (true) {
            SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
            if (ClientSocket == INVALID_SOCKET) {
                LogWSAError(L"Accept failed");
                continue;
            }

            char recvbuf[512];
            int recvbuflen = 512;
            int bytesReceived = recv(ClientSocket, recvbuf, recvbuflen, 0);
            if (bytesReceived > 0) {
                recvbuf[bytesReceived] = '\0';
                std::string request(recvbuf);
                OutputDebugStringA(("Received HTTP request:\n" + request + "\n").c_str());

                // Update state based on the event.
                UpdateStateFromEvent(request);
                // Optionally, notify credentials immediately.
                NotifyCredentials();

                const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nEvent Received";
                send(ClientSocket, response, (int)strlen(response), 0);
            }

            closesocket(ClientSocket);
        }

        closesocket(ListenSocket);
        WSACleanup();
        }).detach();

    OutputDebugStringW(L"React Native app communication initialized successfully.\n");
}

// Boilerplate code to create our provider.
HRESULT CSample_CreateInstance(_In_ REFIID riid, _Outptr_ void** ppv)
{
    HRESULT hr;
    CSampleProvider* pProvider = new(std::nothrow) CSampleProvider();
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
