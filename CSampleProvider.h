//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

#include "helpers.h"
#include <windows.h>
#include <strsafe.h>
#include <new>

#include "CSampleCredential.h"
#include <vector>
#include <string>


class CSampleProvider : public ICredentialProvider,
                        public ICredentialProviderSetUserArray
{
  public:
    // IUnknown
    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return ++_cRef;
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        long cRef = --_cRef;
        if (!cRef)
        {
            delete this;
        }
        return cRef;
    }

    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _COM_Outptr_ void **ppv)
    {
        static const QITAB qit[] =
        {
            QITABENT(CSampleProvider, ICredentialProvider), // IID_ICredentialProvider
            QITABENT(CSampleProvider, ICredentialProviderSetUserArray), // IID_ICredentialProviderSetUserArray
            {0},
        };
        return QISearch(this, qit, riid, ppv);
    }

  public:
    IFACEMETHODIMP SetUsageScenario(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, DWORD dwFlags);
    IFACEMETHODIMP SetSerialization(_In_ CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION const *pcpcs);

    IFACEMETHODIMP Advise(_In_ ICredentialProviderEvents *pcpe, _In_ UINT_PTR upAdviseContext);
    IFACEMETHODIMP UnAdvise();

    IFACEMETHODIMP GetFieldDescriptorCount(_Out_ DWORD *pdwCount);
    IFACEMETHODIMP GetFieldDescriptorAt(DWORD dwIndex,  _Outptr_result_nullonfailure_ CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR **ppcpfd);

    IFACEMETHODIMP GetCredentialCount(_Out_ DWORD *pdwCount,
                                      _Out_ DWORD *pdwDefault,
                                      _Out_ BOOL *pbAutoLogonWithDefault);
    IFACEMETHODIMP GetCredentialAt(DWORD dwIndex,
                                   _Outptr_result_nullonfailure_ ICredentialProviderCredential **ppcpc);

    IFACEMETHODIMP SetUserArray(_In_ ICredentialProviderUserArray *users);

    friend HRESULT CSample_CreateInstance(_In_ REFIID riid, _Outptr_ void** ppv);

    // Register credentials for event notifications
    void RegisterCredential(CSampleCredential* pCredential);

  protected:
    CSampleProvider();
    __override ~CSampleProvider();

  private:
    void _ReleaseEnumeratedCredentials();
    void _CreateEnumeratedCredentials();
    HRESULT _EnumerateEmpty();
    HRESULT _EnumerateCredentials();
    HRESULT _EnumerateEmptyTileCredential();
private:
    long                                    _cRef;            // Used for reference counting.
    CSampleCredential                       *_pCredential;    // SampleV2Credential
    bool                                    _fRecreateEnumeratedCredentials;
    CREDENTIAL_PROVIDER_USAGE_SCENARIO      _cpus;
    ICredentialProviderUserArray            *_pCredProviderUserArray;
    // New function declarations
    void InitializeBluetoothProximityCheck();
    void InitializeReactNativeAppCommunication();
    // State management for app events
    bool isLoggedIn = false;        // Tracks whether the user is logged in
    bool isButtonClicked = false;  // Tracks whether the button was clicked
    void UpdateStateFromEvent(const std::string& event); // Helper for updating state

    void NotifyCredentials(); // Notify all registered credentials of state changes

    std::vector<CSampleCredential*> _credentials; // List of registered credentials
};
