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
#include "guid.h"


#pragma comment(lib, "Bthprops.lib") // Link Bluetooth library
#pragma comment(lib, "Ws2_32.lib") // Link Winsock library

CSampleProvider::CSampleProvider():
    _cRef(1),
    _pCredential(nullptr),
    _pCredProviderUserArray(nullptr),
	isLoggedIn(false),
	isBluetoothDeviceInProximity(false)
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

    // Ensure the user array is available
    if (_pCredProviderUserArray != nullptr)
    {
        DWORD dwUserCount = 0;
        hr = _pCredProviderUserArray->GetCount(&dwUserCount);

        if (SUCCEEDED(hr) && dwUserCount > 0)
        {
            // Iterate over each user and create a credential
            for (DWORD i = 0; i < dwUserCount; i++)
            {
                ICredentialProviderUser* pCredUser = nullptr;
                hr = _pCredProviderUserArray->GetAt(i, &pCredUser);
                if (SUCCEEDED(hr) && pCredUser != nullptr)
                {
                    // Create a new credential
                    CSampleCredential* pCredential = new (std::nothrow) CSampleCredential();
                    if (pCredential != nullptr)
                    {
                        // Initialize the credential
                        hr = pCredential->Initialize(_cpus, s_rgCredProvFieldDescriptors, s_rgFieldStatePairs, pCredUser);
                        if (SUCCEEDED(hr))
                        {
                            // Register the credential for state notifications
                            RegisterCredential(pCredential);

                            // Store the credential (only storing the first for simplicity here)
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

                    // Release the user reference
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
        return;
    }

    // Start a thread to periodically check Bluetooth proximity
    std::thread([this]() {
        while (true)
        {
            CheckBluetoothProximity();
            std::this_thread::sleep_for(std::chrono::seconds(10)); // Check every 10 seconds
        }
    }).detach();
}

void CSampleProvider::CheckBluetoothProximity()
{
	OutputDebugStringW(L"Scanning for nearby Bluetooth devices...\n");

    HANDLE hRadio = NULL;
    BLUETOOTH_FIND_RADIO_PARAMS btfrp = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
    BLUETOOTH_DEVICE_SEARCH_PARAMS btdsp = { sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS) };
    BLUETOOTH_DEVICE_INFO btdi = { sizeof(BLUETOOTH_DEVICE_INFO) };

    HBLUETOOTH_RADIO_FIND hFindRadio = BluetoothFindFirstRadio(&btfrp, &hRadio);
    if (!hFindRadio)
    {
		OutputDebugStringW(L"No Bluetooth radios found.\n");
        return;
    }

    bool deviceFound = false;

    do
    {
        btdsp.hRadio = hRadio;
        btdsp.fReturnAuthenticated = TRUE;
        btdsp.fReturnRemembered = TRUE;
        btdsp.fReturnUnknown = TRUE;
        btdsp.fReturnConnected = TRUE;
		btdsp.fIssueInquiry = TRUE;
        btdsp.cTimeoutMultiplier = 5; // 1.28 seconds

        HBLUETOOTH_DEVICE_FIND hFindDevice = BluetoothFindFirstDevice(&btdsp, &btdi);
        if (hFindDevice)
        {
            do
            {
                OutputDebugStringW((std::wstring(L"Found Bluetooth device: ") + btdi.szName).c_str());
                if (wcscmp(btdi.szName, L"Warren Thompson’s iPhone") == 0)
                {
                    OutputDebugStringW(L"Target device found!\n");
                    deviceFound = true;
                    break;
                }
            } while (BluetoothFindNextDevice(hFindDevice, &btdi));

            BluetoothFindDeviceClose(hFindDevice);
        }

        CloseHandle(hRadio);
        if (deviceFound)
            break;

    } while (BluetoothFindNextRadio(hFindRadio, &hRadio));

    BluetoothFindRadioClose(hFindRadio);

    if (deviceFound)
    {
		isBluetoothDeviceInProximity = true;
    }
    else
    {
		isBluetoothDeviceInProximity = false;
    }
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

        // Set up socket
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

        // Set the SO_REUSEADDR socket option
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

            std::string request;
            char recvbuf[512];
            int recvbuflen = 512;
            int bytesReceived;

            // Receive the request headers
            // test by sending this in PS:  Invoke-WebRequest -Uri http://192.168.0.253:32808 -Method Post -Body "User logged in"
            while ((bytesReceived = recv(ClientSocket, recvbuf, recvbuflen, 0)) > 0) {
                request.append(recvbuf, bytesReceived);
                if (request.find("\r\n\r\n") != std::string::npos) {
                    break;
                }
            }

            // Check if the request contains a Content-Length header
            size_t contentLengthPos = request.find("Content-Length: ");
            if (contentLengthPos != std::string::npos) {
                size_t contentLengthEnd = request.find("\r\n", contentLengthPos);
                int contentLength = std::stoi(request.substr(contentLengthPos + 16, contentLengthEnd - contentLengthPos - 16));

                // Receive the request body
                std::string body;
                while (body.size() < contentLength && (bytesReceived = recv(ClientSocket, recvbuf, recvbuflen, 0)) > 0) {
                    body.append(recvbuf, bytesReceived);
                }

                OutputDebugStringA(("Received HTTP request body:\n" + body + "\n").c_str());

                // Update state based on the event
                UpdateStateFromEvent(body);
            }

            const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nEvent Received";
            send(ClientSocket, response, (int)strlen(response), 0);

            closesocket(ClientSocket);
        }

        closesocket(ListenSocket);
        WSACleanup();
        }).detach();

    OutputDebugStringW(L"React Native app communication initialized successfully.\n");
}

// Helper function to update state based on events
void CSampleProvider::UpdateStateFromEvent(const std::string& event)
{
    bool oldIsLoggedIn = isLoggedIn;

    if (event.find("User logged in") != std::string::npos) {
        isLoggedIn = true;
    }

    // Notify credentials only if state has changed
    if (isBluetoothDeviceInProximity && isLoggedIn) {
        NotifyCredentials();
    }
}

void CSampleProvider::RegisterCredential(CSampleCredential* pCredential)
{
    _credentials.push_back(pCredential);
}

void CSampleProvider::NotifyCredentials()
{
    for (auto* credential : _credentials)
    {
        if (credential)
        {
            credential->OnProviderStateChange(isLoggedIn); // Update the UI indicating login success
        }
		if (credential == _pCredential) { 
            // Manually trigger serialization for the bluetooth credential
            CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE cpgsr;
            CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION cpcs;
            PWSTR pwszOptionalStatusText;
            CREDENTIAL_PROVIDER_STATUS_ICON cpsiOptionalStatusIcon;

            HRESULT hr = _pCredential->GetSerialization(&cpgsr, &cpcs, &pwszOptionalStatusText, &cpsiOptionalStatusIcon);
            if (SUCCEEDED(hr))
            {
                // Free the allocated memory for serialized data
                CoTaskMemFree(cpcs.rgbSerialization);
            }
        }
    }
}