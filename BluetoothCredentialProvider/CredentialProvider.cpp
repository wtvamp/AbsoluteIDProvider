#pragma once
#include "pch.h"
#include "CredentialProvider.h"
#include <initguid.h>
#include "BluetoothCredentialProviderCredential.h"
#include <Shlwapi.h>

// Define the GUID for the Credential Provider
DEFINE_GUID(CLSID_BluetoothCredentialProvider,
    0x12345678, 0x1234, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x90, 0xAB); // Replace with your GUID

// Constructor: Initializes reference count and members
BluetoothCredentialProvider::BluetoothCredentialProvider() :
    _cRef(1), _cpus(CPUS_INVALID), _credential(nullptr), _advised(false)
{
}

// Destructor: Cleans up resources
BluetoothCredentialProvider::~BluetoothCredentialProvider()
{
    if (_credential) // If the credential exists
    {
        _credential->Release(); // Release it
    }
}

// IUnknown methods (required by COM)

// AddRef: Increment the reference count
IFACEMETHODIMP_(ULONG) BluetoothCredentialProvider::AddRef()
{
    return InterlockedIncrement(&_cRef); // Atomically increment the reference count
}

// Release: Decrement the reference count and delete the object if count reaches zero
IFACEMETHODIMP_(ULONG) BluetoothCredentialProvider::Release()
{
    LONG cRef = InterlockedDecrement(&_cRef); // Atomically decrement the reference count
    if (!cRef) // If no references remain
    {
        delete this; // Free the object
    }
    return cRef; // Return the updated reference count
}

// QueryInterface: Provide pointers to supported interfaces
IFACEMETHODIMP BluetoothCredentialProvider::QueryInterface(REFIID riid, void** ppv)
{
    if (ppv == nullptr) // Check if the pointer is valid
    {
        return E_POINTER; // Return an error if null
    }
    // If the client requests IUnknown or ICredentialProvider, return this object
    if (riid == IID_IUnknown || riid == IID_ICredentialProvider)
    {
        *ppv = static_cast<ICredentialProvider*>(this); // Provide the requested interface
        AddRef(); // Increment the reference count
        return S_OK; // Indicate success
    }
    *ppv = nullptr; // If the requested interface is not supported, set output to null
    return E_NOINTERFACE; // Indicate the interface is not available
}

// ICredentialProvider methods

// SetUsageScenario: Called to specify the current usage scenario (e.g., logon, unlock)
IFACEMETHODIMP BluetoothCredentialProvider::SetUsageScenario(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, DWORD dwFlags)
{
    // Store the usage scenario
    _cpus = cpus;

    // Only handle supported scenarios
    if (cpus == CPUS_LOGON || cpus == CPUS_UNLOCK_WORKSTATION)
    {
        // Create a new credential instance if it doesn't already exist
        if (!_credential)
        {
            _credential = new BluetoothCredentialProviderCredential(); // Create BLE credential
            if (!_credential)
            {
                return E_OUTOFMEMORY; // Handle memory allocation failure
            }
        }
        return S_OK; // Successfully initialized for the usage scenario
    }

    // If the usage scenario is unsupported, release existing credential and return error
    if (_credential)
    {
        _credential->Release();
        _credential = nullptr;
    }

    return E_NOTIMPL; // Indicate that the usage scenario is not implemented
}


// SetSerialization: Optionally pre-fill credentials (not used in this example)
IFACEMETHODIMP BluetoothCredentialProvider::SetSerialization(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs)
{
    return E_NOTIMPL; // Not implemented
}

// Advise: Hook into event notifications
IFACEMETHODIMP BluetoothCredentialProvider::Advise(ICredentialProviderEvents* pcpe, UINT_PTR upAdviseContext)
{
    if (_advised) // If already advised
    {
        return S_OK; // Avoid re-advising
    }
    _advised = true; // Mark as advised
    return S_OK; // Indicate success
}

// UnAdvise: Unhook from event notifications
IFACEMETHODIMP BluetoothCredentialProvider::UnAdvise()
{
    if (!_advised) // If not advised, no action needed
    {
        return S_OK;
    }
    _advised = false; // Mark as unadvised
    return S_OK; // Indicate success
}

// GetFieldDescriptorCount: Return the number of UI fields
IFACEMETHODIMP BluetoothCredentialProvider::GetFieldDescriptorCount(DWORD* pdwCount)
{
    if (!pdwCount) // Check if the pointer is valid
    {
        return E_POINTER; // Return an error if null
    }
    *pdwCount = 1; // This example has one field (e.g., BLE status)
    return S_OK; // Indicate success
}

// GetFieldDescriptorAt: Provide the definition for a specific UI field
IFACEMETHODIMP BluetoothCredentialProvider::GetFieldDescriptorAt(DWORD dwIndex, CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd)
{
    if (!ppcpfd) // Check if the pointer is valid
    {
        return E_POINTER; // Return an error if null
    }

    if (dwIndex != 0) // Check if the field index is valid (only one field in this example)
    {
        return E_INVALIDARG; // Return an error for invalid indices
    }

    // Provide a static field descriptor
    static CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR descriptor = { 0 };
    descriptor.dwFieldID = 0; // Field ID
    descriptor.cpft = CPFT_TILE_IMAGE; // Field type (small text label)

    // Assign a label to the descriptor
    HRESULT hr = SHStrDupW(L"Proximity Detection", &descriptor.pszLabel); // Correctly assign the label
    if (FAILED(hr)) // Check if string duplication succeeded
    {
        return hr; // Return the error
    }

    *ppcpfd = &descriptor; // Return the descriptor
    return S_OK; // Indicate success
}


// GetCredentialCount: Return the number of credentials available
IFACEMETHODIMP BluetoothCredentialProvider::GetCredentialCount(DWORD* pdwCount, DWORD* pdwDefault, BOOL* pbAutoLogonWithDefault)
{
    if (!pdwCount || !pdwDefault || !pbAutoLogonWithDefault) // Check if pointers are valid
    {
        return E_POINTER; // Return an error if any are null
    }
    *pdwCount = 1; // This example provides one credential
    *pdwDefault = 0; // The default credential is the first one
    *pbAutoLogonWithDefault = FALSE; // Auto-logon is not enabled
    return S_OK; // Indicate success
}

// GetCredentialAt: Retrieve a specific credential object
IFACEMETHODIMP BluetoothCredentialProvider::GetCredentialAt(DWORD dwIndex, ICredentialProviderCredential** ppcpc)
{
    if (!ppcpc || dwIndex != 0 || !_credential) // Check if pointers are valid and the index is correct
    {
        return E_INVALIDARG; // Return an error if invalid
    }
    *ppcpc = _credential; // Return the credential
    _credential->AddRef(); // Increment the reference count
    return S_OK; // Indicate success
}
