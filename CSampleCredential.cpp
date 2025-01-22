//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//

#ifndef WIN32_NO_STATUS
#include <ntstatus.h>
#define WIN32_NO_STATUS
#endif
#include <unknwn.h>
#include "CSampleCredential.h"
#include "CSampleProvider.h"
#include "guid.h"
#include <iostream>
#include <bluetoothapis.h> // Windows Bluetooth API

#pragma comment(lib, "Bthprops.lib") // Link Bluetooth library

// Constructor for CSampleCredential class
CSampleCredential::CSampleCredential():
   // Initialize reference count to 1
   _cRef(1),
   // Initialize credential provider events pointer to nullptr
   _pCredProvCredentialEvents(nullptr),
   // Initialize user SID pointer to nullptr
   _pszUserSid(nullptr),
   // Initialize qualified user name pointer to nullptr
   _pszQualifiedUserName(nullptr),
   // Initialize local user flag to false
   _fIsLocalUser(false)
{
   // Increment DLL reference count
   DllAddRef();

   // Zero out the memory for credential provider field descriptors array
   ZeroMemory(_rgCredProvFieldDescriptors, sizeof(_rgCredProvFieldDescriptors));
   // Zero out the memory for field state pairs array
   ZeroMemory(_rgFieldStatePairs, sizeof(_rgFieldStatePairs));
   // Zero out the memory for field strings array
   ZeroMemory(_rgFieldStrings, sizeof(_rgFieldStrings));
}

CSampleCredential::~CSampleCredential()
{
    // Loop through each field string and free the allocated memory
    for (int i = 0; i < ARRAYSIZE(_rgFieldStrings); i++)
    {
        // Free the memory allocated for the field string
        CoTaskMemFree(_rgFieldStrings[i]);
        // Free the memory allocated for the field descriptor label
        CoTaskMemFree(_rgCredProvFieldDescriptors[i].pszLabel);
    }
    // Free the memory allocated for the user SID
    CoTaskMemFree(_pszUserSid);
    // Free the memory allocated for the qualified user name
    CoTaskMemFree(_pszQualifiedUserName);
    // Decrement the DLL reference count
    DllRelease();
}


// Initializes one credential with the field information passed in.
// Set the value of the SFI_LARGE_TEXT field to pwzUsername.
HRESULT CSampleCredential::Initialize(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
                                      _In_ CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR const *rgcpfd,
                                      _In_ FIELD_STATE_PAIR const *rgfsp,
                                      _In_ ICredentialProviderUser *pcpUser)
{
    // Initialize the HRESULT variable to S_OK, indicating success.
    HRESULT hr = S_OK;
    // Store the usage scenario in the member variable.
    _cpus = cpus;

    // Declare a GUID variable to store the provider ID.
    GUID guidProvider;
    // Get the provider ID from the user and store it in guidProvider.
    pcpUser->GetProviderID(&guidProvider);
    // Check if the provider ID matches the local user provider ID and set the local user flag accordingly.
    _fIsLocalUser = (guidProvider == Identity_LocalUserProvider);

    // Loop through each field descriptor and copy it to the member variable array.
    for (DWORD i = 0; SUCCEEDED(hr) && i < ARRAYSIZE(_rgCredProvFieldDescriptors); i++)
    {
        // Copy the field state pair to the member variable array.
        _rgFieldStatePairs[i] = rgfsp[i];
        // Copy the field descriptor to the member variable array.
        hr = FieldDescriptorCopy(rgcpfd[i], &_rgCredProvFieldDescriptors[i]);
    }

    // Initialize the string value of the label field.
    if (SUCCEEDED(hr))
    {
        hr = SHStrDupW(L"AbsoluteID Credential", &_rgFieldStrings[SFI_LABEL]);
    }
    // Initialize the string value of the large text field.
    if (SUCCEEDED(hr))
    {
        hr = SHStrDupW(L"AbsoluteID Credential Provider", &_rgFieldStrings[SFI_LARGE_TEXT]);
    }
    // Get the qualified user name from the user and store it in the member variable.
    if (SUCCEEDED(hr))
    {
        hr = pcpUser->GetStringValue(PKEY_Identity_QualifiedUserName, &_pszQualifiedUserName);
    }
    // Get the user name from the user and store it in the member variable.
    if (SUCCEEDED(hr))
    {
        // Declare a pointer to store the user name.
        PWSTR pszUserName;
        // Get the user name from the user.
        pcpUser->GetStringValue(PKEY_Identity_UserName, &pszUserName);
        // Check if the user name is not null.
        if (pszUserName != nullptr)
        {
            // Declare a buffer to store the formatted string.
            wchar_t szString[256];
            // Format the user name string.
            StringCchPrintf(szString, ARRAYSIZE(szString), L"User Name: %s", pszUserName);
            // Duplicate the formatted string and store it in the member variable.
            hr = SHStrDupW(szString, &_rgFieldStrings[SFI_FULLNAME_TEXT]);
            // Free the memory allocated for the user name.
            CoTaskMemFree(pszUserName);
        }
        else
        {
            // If the user name is null, set the field string to indicate that.
            hr =  SHStrDupW(L"User Name is NULL", &_rgFieldStrings[SFI_FULLNAME_TEXT]);
        }
    }
    // Get the display name from the user and store it in the member variable.
    if (SUCCEEDED(hr))
    {
        // Declare a pointer to store the display name.
        PWSTR pszDisplayName;
        // Get the display name from the user.
        pcpUser->GetStringValue(PKEY_Identity_DisplayName, &pszDisplayName);
        // Check if the display name is not null.
        if (pszDisplayName != nullptr)
        {
            // Declare a buffer to store the formatted string.
            wchar_t szString[256];
            // Format the display name string.
            StringCchPrintf(szString, ARRAYSIZE(szString), L"Display Name: %s", pszDisplayName);
            // Duplicate the formatted string and store it in the member variable.
            hr = SHStrDupW(szString, &_rgFieldStrings[SFI_DISPLAYNAME_TEXT]);
            // Free the memory allocated for the display name.
            CoTaskMemFree(pszDisplayName);
        }
        else
        {
            // If the display name is null, set the field string to indicate that.
            hr = SHStrDupW(L"Display Name is NULL", &_rgFieldStrings[SFI_DISPLAYNAME_TEXT]);
        }
    }
    // Get the logon status from the user and store it in the member variable.
    if (SUCCEEDED(hr))
    {
        // Declare a pointer to store the logon status.
        PWSTR pszLogonStatus;
        // Get the logon status from the user.
        pcpUser->GetStringValue(PKEY_Identity_LogonStatusString, &pszLogonStatus);
        // Check if the logon status is not null.
        if (pszLogonStatus != nullptr)
        {
            // Declare a buffer to store the formatted string.
            wchar_t szString[256];
            // Format the logon status string.
            StringCchPrintf(szString, ARRAYSIZE(szString), L"Logon Status: %s", pszLogonStatus);
            // Duplicate the formatted string and store it in the member variable.
            hr = SHStrDupW(szString, &_rgFieldStrings[SFI_LOGONSTATUS_TEXT]);
            // Free the memory allocated for the logon status.
            CoTaskMemFree(pszLogonStatus);
        }
        else
        {
            // If the logon status is null, set the field string to indicate that.
            hr = SHStrDupW(L"Logon Status is NULL", &_rgFieldStrings[SFI_LOGONSTATUS_TEXT]);
        }
    }

    // Get the user SID from the user and store it in the member variable.
    if (SUCCEEDED(hr))
    {
        hr = pcpUser->GetSid(&_pszUserSid);
    }

    // Return the HRESULT value indicating success or failure.
    return hr;
}

// LogonUI calls this in order to give us a callback in case we need to notify it of anything.
HRESULT CSampleCredential::Advise(_In_ ICredentialProviderCredentialEvents *pcpce)
{
    // Check if the credential provider events pointer is not null
    if (_pCredProvCredentialEvents != nullptr)
    {
        // Release the current credential provider events
        _pCredProvCredentialEvents->Release();
    }
    // Query the interface for the credential provider events and store the result in _pCredProvCredentialEvents
    return pcpce->QueryInterface(IID_PPV_ARGS(&_pCredProvCredentialEvents));
}

// LogonUI calls this to tell us to release the callback.
HRESULT CSampleCredential::UnAdvise()
{
    // Check if the credential provider events pointer is not null
    if (_pCredProvCredentialEvents)
    {
        // Release the current credential provider events
        _pCredProvCredentialEvents->Release();
    }
    // Set the credential provider events pointer to null
    _pCredProvCredentialEvents = nullptr;
    // Return S_OK to indicate success
    return S_OK;
}

// LogonUI calls this function when our tile is selected (zoomed)
// If you simply want fields to show/hide based on the selected state,
// there's no need to do anything here - you can set that up in the
// field definitions. But if you want to do something
// more complicated, like change the contents of a field when the tile is
// selected, you would do it here.
HRESULT CSampleCredential::SetSelected(_Out_ BOOL *pbAutoLogon)
{
    *pbAutoLogon = FALSE;
    return S_OK;
}

// Similarly to SetSelected, LogonUI calls this when your tile was selected
// and now no longer is. The most common thing to do here (which we do below)
// is to clear out the password field.
HRESULT CSampleCredential::SetDeselected()
{
    return S_OK;
}

// Get info for a particular field of a tile. Called by logonUI to get information
// to display the tile.
HRESULT CSampleCredential::GetFieldState(DWORD dwFieldID,
                                         _Out_ CREDENTIAL_PROVIDER_FIELD_STATE *pcpfs,
                                         _Out_ CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE *pcpfis)
{
    // Declare a variable to hold the result of the function.
    HRESULT hr;

    // Validate our parameters.
    // Check if the field ID is within the range of the array size.
    if ((dwFieldID < ARRAYSIZE(_rgFieldStatePairs)))
    {
        // If valid, set the field state and interactive state from the arrays.
        *pcpfs = _rgFieldStatePairs[dwFieldID].cpfs;
        *pcpfis = _rgFieldStatePairs[dwFieldID].cpfis;
        // Set the result to S_OK indicating success.
        hr = S_OK;
    }
    else
    {
        // If the field ID is not valid, set the result to E_INVALIDARG indicating an invalid argument.
        hr = E_INVALIDARG;
    }
    // Return the result.
    return hr;
}

// Sets ppwsz to the string value of the field at the index dwFieldID
HRESULT CSampleCredential::GetStringValue(DWORD dwFieldID, _Outptr_result_nullonfailure_ PWSTR *ppwsz)
{
    // Declare a variable to hold the result of the function.
    HRESULT hr;
    // Initialize the output pointer to nullptr.
    *ppwsz = nullptr;

    // Check to make sure dwFieldID is a legitimate index.
    if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors))
    {
        // Make a copy of the string and return that. The caller
        // is responsible for freeing it.
        hr = SHStrDupW(_rgFieldStrings[dwFieldID], ppwsz);
    }
    else
    {
        // If the field ID is not valid, set the result to E_INVALIDARG indicating an invalid argument.
        hr = E_INVALIDARG;
    }

    // Return the result.
    return hr;
}

// Get the image to show in the user tile
HRESULT CSampleCredential::GetBitmapValue(DWORD dwFieldID, _Outptr_result_nullonfailure_ HBITMAP *phbmp)
{
    HRESULT hr;
    *phbmp = nullptr;

    if ((SFI_TILEIMAGE == dwFieldID))
    {
        HBITMAP hbmp = LoadBitmap(HINST_THISDLL, MAKEINTRESOURCE(IDB_TILE_IMAGE));
        if (hbmp != nullptr)
        {
            hr = S_OK;
            *phbmp = hbmp;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT CSampleCredential::GetCheckboxValue(DWORD dwFieldID, BOOL* pbChecked, PWSTR* ppwszLabel)
{
    return S_OK;
}

HRESULT CSampleCredential::GetComboBoxValueCount(DWORD dwFieldID, DWORD* pcItems, DWORD* pdwSelectedItem)
{
    return S_OK;
}

HRESULT CSampleCredential::GetComboBoxValueAt(DWORD dwFieldID, DWORD dwItem, PWSTR* ppwszItem)
{
    return S_OK;
}

HRESULT CSampleCredential::GetSubmitButtonValue(DWORD dwFieldID, DWORD* pdwAdjacentTo)
{
    return S_OK;
}

// Sets the value of a field which can accept a string as a value.
// This is called on each keystroke when a user types into an edit field
HRESULT CSampleCredential::SetStringValue(DWORD dwFieldID, _In_ PCWSTR pwz)
{
    return S_OK;
}

// Sets whether the specified checkbox is checked or not.
HRESULT CSampleCredential::SetCheckboxValue(DWORD dwFieldID, BOOL bChecked)
{
    return S_OK;
}

HRESULT CSampleCredential::SetComboBoxSelectedValue(DWORD dwFieldID, DWORD dwSelectedItem)
{
    return S_OK;
}

HRESULT CSampleCredential::CommandLinkClicked(DWORD dwFieldID)
{
    return S_OK;
}

HRESULT CSampleCredential::GetSerialization(_Out_ CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
    _Out_ CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
    _Outptr_result_maybenull_ PWSTR* ppwszOptionalStatusText,
    _Out_ CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon)
{
    // Initialize the HRESULT variable to S_OK, indicating success.
    HRESULT hr = S_OK;
    // Set the serialization response to indicate that the credential is not finished.
    *pcpgsr = CPGSR_NO_CREDENTIAL_NOT_FINISHED;
    // Initialize the optional status text pointer to nullptr.
    *ppwszOptionalStatusText = nullptr;
    // Initialize the optional status icon to none.
    *pcpsiOptionalStatusIcon = CPSI_NONE;
    // Zero out the memory for the credential serialization structure.
    ZeroMemory(pcpcs, sizeof(*pcpcs));

    // Check if the user is a local user.
    if (_fIsLocalUser)
    {
        // Declare pointers to store the domain and username.
        PWSTR pszDomain;
        PWSTR pszUsername;
        // Split the qualified username into domain and username.
        hr = SplitDomainAndUsername(_pszQualifiedUserName, &pszDomain, &pszUsername);
        if (SUCCEEDED(hr))
        {
            // Declare a KERB_INTERACTIVE_UNLOCK_LOGON structure to store the logon information.
            KERB_INTERACTIVE_UNLOCK_LOGON kiul;
            // Initialize the logon structure with the domain, username, and usage scenario.
            hr = KerbInteractiveUnlockLogonInit(pszDomain, pszUsername, L"test123", _cpus, &kiul);
            if (SUCCEEDED(hr))
            {
                // Pack the logon structure into the credential serialization structure.
                hr = KerbInteractiveUnlockLogonPack(kiul, &pcpcs->rgbSerialization, &pcpcs->cbSerialization);
                if (SUCCEEDED(hr))
                {
                    // Retrieve the authentication package.
                    ULONG ulAuthPackage;
                    hr = RetrieveNegotiateAuthPackage(&ulAuthPackage);
                    if (SUCCEEDED(hr))
                    {
                        // Set the authentication package and credential provider CLSID in the serialization structure.
                        pcpcs->ulAuthenticationPackage = ulAuthPackage;
                        pcpcs->clsidCredentialProvider = CLSID_CSample;
                        // Set the serialization response to indicate that the credential is finished.
                        *pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
                    }
                }
            }
            // Free the memory allocated for the domain and username.
            CoTaskMemFree(pszDomain);
            CoTaskMemFree(pszUsername);
        }
    }
    else
    {
        // Declare a variable to store the authentication flags.
        DWORD dwAuthFlags = CRED_PACK_PROTECTED_CREDENTIALS | CRED_PACK_ID_PROVIDER_CREDENTIALS;

        // Get the size of the authentication buffer to allocate.
        if (!CredPackAuthenticationBuffer(dwAuthFlags, _pszQualifiedUserName, L"test123", nullptr, &pcpcs->cbSerialization) &&
            (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
        {
            // Allocate memory for the authentication buffer.
            pcpcs->rgbSerialization = static_cast<byte*>(CoTaskMemAlloc(pcpcs->cbSerialization));
            if (pcpcs->rgbSerialization != nullptr)
            {
                // Initialize the HRESULT variable to S_OK, indicating success.
                hr = S_OK;

                // Retrieve the authentication buffer.
                if (CredPackAuthenticationBuffer(dwAuthFlags, _pszQualifiedUserName, L"test123", pcpcs->rgbSerialization, &pcpcs->cbSerialization))
                {
                    // Retrieve the authentication package.
                    ULONG ulAuthPackage;
                    hr = RetrieveNegotiateAuthPackage(&ulAuthPackage);
                    if (SUCCEEDED(hr))
                    {
                        // Set the authentication package and credential provider CLSID in the serialization structure.
                        pcpcs->ulAuthenticationPackage = ulAuthPackage;
                        pcpcs->clsidCredentialProvider = CLSID_CSample;

                        // Set the serialization response to indicate that the credential is finished.
                        *pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
                    }
                }
                else
                {
                    // If the authentication buffer retrieval fails, set the HRESULT variable to the error code.
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    if (SUCCEEDED(hr))
                    {
                        hr = E_FAIL;
                    }
                }

                // If the HRESULT variable indicates failure, free the memory allocated for the authentication buffer.
                if (FAILED(hr))
                {
                    CoTaskMemFree(pcpcs->rgbSerialization);
                }
            }
            else
            {
                // If memory allocation fails, set the HRESULT variable to E_OUTOFMEMORY.
                hr = E_OUTOFMEMORY;
            }
        }
    }
    // Return the HRESULT value indicating success or failure.
    return hr;
}


struct REPORT_RESULT_STATUS_INFO
{
    NTSTATUS ntsStatus;
    NTSTATUS ntsSubstatus;
    PWSTR     pwzMessage;
    CREDENTIAL_PROVIDER_STATUS_ICON cpsi;
};

static const REPORT_RESULT_STATUS_INFO s_rgLogonStatusInfo[] =
{
    { STATUS_LOGON_FAILURE, STATUS_SUCCESS, L"Incorrect username.", CPSI_ERROR, },
    { STATUS_ACCOUNT_RESTRICTION, STATUS_ACCOUNT_DISABLED, L"The account is disabled.", CPSI_WARNING },
};

// ReportResult is completely optional.  Its purpose is to allow a credential to customize the string
// and the icon displayed in the case of a logon failure.  For example, we have chosen to
// customize the error shown in the case of bad username/password and in the case of the account
// being disabled.
HRESULT CSampleCredential::ReportResult(NTSTATUS ntsStatus,
                                        NTSTATUS ntsSubstatus,
                                        _Outptr_result_maybenull_ PWSTR *ppwszOptionalStatusText,
                                        _Out_ CREDENTIAL_PROVIDER_STATUS_ICON *pcpsiOptionalStatusIcon)
{
    *ppwszOptionalStatusText = nullptr;
    *pcpsiOptionalStatusIcon = CPSI_NONE;

    DWORD dwStatusInfo = (DWORD)-1;

    // Look for a match on status and substatus.
    for (DWORD i = 0; i < ARRAYSIZE(s_rgLogonStatusInfo); i++)
    {
        if (s_rgLogonStatusInfo[i].ntsStatus == ntsStatus && s_rgLogonStatusInfo[i].ntsSubstatus == ntsSubstatus)
        {
            dwStatusInfo = i;
            break;
        }
    }

    if ((DWORD)-1 != dwStatusInfo)
    {
        if (SUCCEEDED(SHStrDupW(s_rgLogonStatusInfo[dwStatusInfo].pwzMessage, ppwszOptionalStatusText)))
        {
            *pcpsiOptionalStatusIcon = s_rgLogonStatusInfo[dwStatusInfo].cpsi;
        }
    }

    // Since nullptr is a valid value for *ppwszOptionalStatusText and *pcpsiOptionalStatusIcon
    // this function can't fail.
    return S_OK;
}

// Gets the SID of the user corresponding to the credential.
HRESULT CSampleCredential::GetUserSid(_Outptr_result_nullonfailure_ PWSTR *ppszSid)
{
    *ppszSid = nullptr;
    HRESULT hr = E_UNEXPECTED;
    if (_pszUserSid != nullptr)
    {
        hr = SHStrDupW(_pszUserSid, ppszSid);
    }
    // Return S_FALSE with a null SID in ppszSid for the
    // credential to be associated with an empty user tile.

    return hr;
}

// GetFieldOptions to enable the password reveal button and touch keyboard auto-invoke in the password field.
HRESULT CSampleCredential::GetFieldOptions(DWORD dwFieldID,
                                           _Out_ CREDENTIAL_PROVIDER_CREDENTIAL_FIELD_OPTIONS *pcpcfo)
{
    *pcpcfo = CPCFO_NONE;

    if (dwFieldID == SFI_TILEIMAGE)
    {
        *pcpcfo = CPCFO_ENABLE_TOUCH_KEYBOARD_AUTO_INVOKE;
    }

    return S_OK;
}

void CSampleCredential::OnProviderStateChange(bool loggedIn)
{
    bool oldLoggedIn = false;
    if (oldLoggedIn != loggedIn) {
        if (loggedIn)
        {
			oldLoggedIn = true;
            // Update the credential state to reflect the logged-in status
            if (_pCredProvCredentialEvents)
            {
                _pCredProvCredentialEvents->SetFieldString(this, SFI_LOGONSTATUS_TEXT, L"User is logged in via Bluetooth proximity.");
            }
        }
        else
        {
			oldLoggedIn = false;
            // Update the credential state to reflect the logged-out status
            if (_pCredProvCredentialEvents)
            {
                _pCredProvCredentialEvents->SetFieldString(this, SFI_LOGONSTATUS_TEXT, L"User is not in Bluetooth proximity.");
            }
        }
    }
}