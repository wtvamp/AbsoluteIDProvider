#pragma once
#include "pch.h"
#include <windows.h>
#include "CredentialProviderFactory.h"
#include "CredentialProvider.h"

// DLL entry point: Called by the system when the DLL is loaded or unloaded
BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call) // Determine the reason for the function call
    {
    case DLL_PROCESS_ATTACH: // The DLL is being loaded into the virtual address space of the process
        // Perform any necessary initialization here
        break;

    case DLL_THREAD_ATTACH: // A thread is being created within the process
        // This is not typically used for Credential Providers
        break;

    case DLL_THREAD_DETACH: // A thread is exiting cleanly
        // This is also not typically used for Credential Providers
        break;

    case DLL_PROCESS_DETACH: // The DLL is being unloaded from the virtual address space of the process
        // Perform any necessary cleanup here
        break;
    }

    return TRUE; // Indicate successful initialization or cleanup
}

// Exported function to create the class factory
// This function is called by the system to create an instance of the factory
HRESULT __stdcall DllGetClassObject(const CLSID& clsid, const IID& iid, void** ppv)
{
    if (clsid == CLSID_BluetoothCredentialProvider) // Check if the requested CLSID matches your provider
    {
        BluetoothCredentialProviderFactory* factory = new BluetoothCredentialProviderFactory(); // Create a new factory instance
        if (!factory) // Check if memory allocation failed
        {
            return E_OUTOFMEMORY; // Return an out-of-memory error
        }

        // Query the factory for the requested interface
        HRESULT hr = factory->QueryInterface(iid, ppv);
        factory->Release(); // Release the local reference to the factory
        return hr; // Return the result of the QueryInterface call
    }

    return CLASS_E_CLASSNOTAVAILABLE; // Return an error if the CLSID does not match
}

// Exported function to determine whether the DLL can be unloaded
// The system calls this function to check if it is safe to unload the DLL
HRESULT __stdcall DllCanUnloadNow()
{
    // Typically, you would check if there are any outstanding references to your COM objects
    // Here, we assume the DLL can always be unloaded
    return S_OK;
}
