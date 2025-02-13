//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// CSampleCredential is our implementation of ICredentialProviderCredential.
// ICredentialProviderCredential is what LogonUI uses to let a credential
// provider specify what a user tile looks like and then tell it what the
// user has entered into the tile.  ICredentialProviderCredential is also
// responsible for packaging up the users credentials into a buffer that
// LogonUI then sends on to LSA.

//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// CSampleCredential is our implementation of ICredentialProviderCredential.
// ICredentialProviderCredential is what LogonUI uses to let a credential
// provider specify what a user tile looks like and then tell it what the
// user has entered into the tile.  ICredentialProviderCredential is also
// responsible for packaging up the users credentials into a buffer that
// LogonUI then sends on to LSA.

#pragma once // Ensures the header file is included only once during compilation

#include <windows.h> // Includes Windows API functions
#include <strsafe.h> // Includes safe string functions
#include <shlguid.h> // Includes GUID definitions
#include <propkey.h> // Includes property key definitions
#include "common.h" // Includes common definitions
#include "dll.h" // Includes DLL definitions
#include "resource.h" // Includes resource definitions

// CSampleCredential class definition
class CSampleCredential : public ICredentialProviderCredential2, ICredentialProviderCredentialWithFieldOptions
{
public:
    // IUnknown interface methods
    IFACEMETHODIMP_(ULONG) AddRef() // Increments the reference count
    {
        return ++_cRef; // Returns the incremented reference count
    }

    IFACEMETHODIMP_(ULONG) Release() // Decrements the reference count
    {
        long cRef = --_cRef; // Decrements the reference count and stores it in cRef
        if (!cRef) // If the reference count is zero
        {
            delete this; // Deletes the object
        }
        return cRef; // Returns the decremented reference count
    }

    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _COM_Outptr_ void **ppv) // Queries for a pointer to the specified interface
    {
        static const QITAB qit[] = // Array of interface ID and offset pairs
        {
            QITABENT(CSampleCredential, ICredentialProviderCredential), // IID_ICredentialProviderCredential
            QITABENT(CSampleCredential, ICredentialProviderCredential2), // IID_ICredentialProviderCredential2
            QITABENT(CSampleCredential, ICredentialProviderCredentialWithFieldOptions), //IID_ICredentialProviderCredentialWithFieldOptions
            {0}, // End of array
        };
        return QISearch(this, qit, riid, ppv); // Searches for the requested interface
    }
public:
    // ICredentialProviderCredential interface methods
    IFACEMETHODIMP Advise(_In_ ICredentialProviderCredentialEvents *pcpce); // Advises the credential provider of events
    IFACEMETHODIMP UnAdvise(); // Unadvises the credential provider of events

    IFACEMETHODIMP SetSelected(_Out_ BOOL *pbAutoLogon); // Sets the credential as selected
    IFACEMETHODIMP SetDeselected(); // Sets the credential as deselected

    IFACEMETHODIMP GetFieldState(DWORD dwFieldID,
                                 _Out_ CREDENTIAL_PROVIDER_FIELD_STATE *pcpfs,
                                 _Out_ CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE *pcpfis); // Gets the state of a field

    IFACEMETHODIMP GetStringValue(DWORD dwFieldID, _Outptr_result_nullonfailure_ PWSTR *ppwsz); // Gets the string value of a field
    IFACEMETHODIMP GetBitmapValue(DWORD dwFieldID, _Outptr_result_nullonfailure_ HBITMAP *phbmp); // Gets the bitmap value of a field
    IFACEMETHODIMP GetCheckboxValue(DWORD dwFieldID, _Out_ BOOL *pbChecked, _Outptr_result_nullonfailure_ PWSTR *ppwszLabel); // Gets the checkbox value of a field
    IFACEMETHODIMP GetComboBoxValueCount(DWORD dwFieldID, _Out_ DWORD *pcItems, _Deref_out_range_(<, *pcItems) _Out_ DWORD *pdwSelectedItem); // Gets the count of combo box items
    IFACEMETHODIMP GetComboBoxValueAt(DWORD dwFieldID, DWORD dwItem, _Outptr_result_nullonfailure_ PWSTR *ppwszItem); // Gets the value of a combo box item
    IFACEMETHODIMP GetSubmitButtonValue(DWORD dwFieldID, _Out_ DWORD *pdwAdjacentTo); // Gets the value of a submit button

    IFACEMETHODIMP SetStringValue(DWORD dwFieldID, _In_ PCWSTR pwz); // Sets the string value of a field
    IFACEMETHODIMP SetCheckboxValue(DWORD dwFieldID, BOOL bChecked); // Sets the checkbox value of a field
    IFACEMETHODIMP SetComboBoxSelectedValue(DWORD dwFieldID, DWORD dwSelectedItem); // Sets the selected value of a combo box
    IFACEMETHODIMP CommandLinkClicked(DWORD dwFieldID); // Handles command link click events

    IFACEMETHODIMP GetSerialization(_Out_ CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE *pcpgsr,
                                    _Out_ CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *pcpcs,
                                    _Outptr_result_maybenull_ PWSTR *ppwszOptionalStatusText,
                                    _Out_ CREDENTIAL_PROVIDER_STATUS_ICON *pcpsiOptionalStatusIcon); // Gets the serialization of the credential
    IFACEMETHODIMP ReportResult(NTSTATUS ntsStatus,
                                NTSTATUS ntsSubstatus,
                                _Outptr_result_maybenull_ PWSTR *ppwszOptionalStatusText,
                                _Out_ CREDENTIAL_PROVIDER_STATUS_ICON *pcpsiOptionalStatusIcon); // Reports the result of the credential

    // ICredentialProviderCredential2 interface method
    IFACEMETHODIMP GetUserSid(_Outptr_result_nullonfailure_ PWSTR *ppszSid); // Gets the user SID

    // ICredentialProviderCredentialWithFieldOptions interface method
    IFACEMETHODIMP GetFieldOptions(DWORD dwFieldID,
                                   _Out_ CREDENTIAL_PROVIDER_CREDENTIAL_FIELD_OPTIONS *pcpcfo); // Gets the field options

public:
    HRESULT Initialize(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
                       _In_ CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR const *rgcpfd,
                       _In_ FIELD_STATE_PAIR const *rgfsp,
                       _In_ ICredentialProviderUser *pcpUser); // Initializes the credential
    void OnProviderStateChange(bool loggedIn); // Handles provider state changes
    CSampleCredential(); // Constructor

private:
    virtual ~CSampleCredential(); // Destructor
    long                                    _cRef; // Reference count
    CREDENTIAL_PROVIDER_USAGE_SCENARIO      _cpus; // The usage scenario for which we were enumerated
    CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR    _rgCredProvFieldDescriptors[SFI_NUM_FIELDS]; // An array holding the type and name of each field in the tile
    FIELD_STATE_PAIR                        _rgFieldStatePairs[SFI_NUM_FIELDS]; // An array holding the state of each field in the tile
    PWSTR                                   _rgFieldStrings[SFI_NUM_FIELDS]; // An array holding the string value of each field
    PWSTR                                   _pszUserSid; // User SID
    PWSTR                                   _pszQualifiedUserName; // The user name that's used to pack the authentication buffer
    ICredentialProviderCredentialEvents2*   _pCredProvCredentialEvents; // Used to update fields
    bool                                    _fIsLocalUser; // If the cred prov is associating with a local user tile
};
