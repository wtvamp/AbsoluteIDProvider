#pragma once // Ensure this header is included only once during compilation
#include "pch.h" // Include precompiled header for performance optimization
#include <windows.h> // Include Windows-specific definitions
#include <credentialprovider.h> // Include Credential Provider-specific definitions
#include "BluetoothCredentialProviderCredential.h" // Include the header for individual credentials

// Declare the CLSID for the Credential Provider, which uniquely identifies it
extern const CLSID CLSID_BluetoothCredentialProvider;

// Define the BluetoothCredentialProvider class, which implements ICredentialProvider
class BluetoothCredentialProvider : public ICredentialProvider
{
public:
    // Constructor: Initializes a new instance of the Credential Provider
    BluetoothCredentialProvider();

    // Destructor: Cleans up resources when the object is destroyed
    virtual ~BluetoothCredentialProvider();

    // IUnknown methods (required for all COM objects for memory management and interface querying)
    IFACEMETHODIMP_(ULONG) AddRef(); // Increment the reference count
    IFACEMETHODIMP_(ULONG) Release(); // Decrement the reference count and delete the object if the count reaches zero
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv); // Provide pointers to supported interfaces

    // ICredentialProvider methods (define how the provider interacts with the Windows logon UI)
    IFACEMETHODIMP SetUsageScenario( // Specify when the Credential Provider is used (e.g., logon, unlock)
        CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, // The usage scenario (logon, unlock, etc.)
        DWORD dwFlags); // Additional flags for the scenario
    IFACEMETHODIMP SetSerialization( // Pre-fill credentials if provided by the system
        const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs);
    IFACEMETHODIMP Advise( // Hook into Credential Provider-related events
        ICredentialProviderEvents* pcpe, // The events interface for the Credential Provider
        UINT_PTR upAdviseContext); // Context for event notifications
    IFACEMETHODIMP UnAdvise(); // Unhook from Credential Provider-related events
    IFACEMETHODIMP GetFieldDescriptorCount( // Get the number of UI fields
        DWORD* pdwCount); // Output: Number of fields
    IFACEMETHODIMP GetFieldDescriptorAt( // Get the definition for a specific field
        DWORD dwIndex, // The index of the field
        CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd); // Output: Field descriptor
    IFACEMETHODIMP GetCredentialCount( // Get the number of credentials this provider offers
        DWORD* pdwCount, // Output: Number of credentials
        DWORD* pdwDefault, // Output: Default credential index
        BOOL* pbAutoLogonWithDefault); // Output: Whether auto-logon is enabled
    IFACEMETHODIMP GetCredentialAt( // Retrieve a specific credential object
        DWORD dwIndex, // The index of the credential
        ICredentialProviderCredential** ppcpc); // Output: Pointer to the credential object

private:
    LONG _cRef; // Reference counter for COM memory management
    CREDENTIAL_PROVIDER_USAGE_SCENARIO _cpus; // The current usage scenario (logon, unlock, etc.)
    BluetoothCredentialProviderCredential* _credential; // Single credential instance representing a BLE-based login tile
    bool _advised; // Tracks whether the provider is hooked into event notifications
};
