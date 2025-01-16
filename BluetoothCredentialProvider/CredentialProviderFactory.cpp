#pragma once                                // Include the header file only once
#include "pch.h"                           // Precompiled header for faster compilation
#include "CredentialProviderFactory.h"     // Header file for the CredentialProviderFactory class
#include "CredentialProvider.h"            // Header file for the CredentialProvider class

// Constructor: Initializes the factory object with a reference count of 1
BluetoothCredentialProviderFactory::BluetoothCredentialProviderFactory() : _cRef(1)
{
}

// Destructor: Cleans up the factory object when it is no longer in use
BluetoothCredentialProviderFactory::~BluetoothCredentialProviderFactory()
{
}

// AddRef: Increments the reference count for the factory object
// This ensures that the factory stays alive while it is being used
IFACEMETHODIMP_(ULONG) BluetoothCredentialProviderFactory::AddRef()
{
    return InterlockedIncrement(&_cRef); // Atomically increment the reference count
}

// Release: Decrements the reference count for the factory object
// If the count reaches zero, the object is deleted
IFACEMETHODIMP_(ULONG) BluetoothCredentialProviderFactory::Release()
{
    LONG cRef = InterlockedDecrement(&_cRef); // Atomically decrement the reference count
    if (!cRef)                                // If the reference count is zero
    {
        delete this;                          // Delete the object
    }
    return cRef;                              // Return the updated reference count
}

// QueryInterface: Provides access to supported interfaces of the factory object
IFACEMETHODIMP BluetoothCredentialProviderFactory::QueryInterface(REFIID riid, void** ppv)
{
    if (ppv == nullptr)                       // Check if the output pointer is null
    {
        return E_POINTER;                     // Return an error if the pointer is invalid
    }

    // Check if the requested interface is IUnknown or IClassFactory
    if (riid == IID_IUnknown || riid == IID_IClassFactory)
    {
        *ppv = static_cast<IClassFactory*>(this); // Provide the requested interface
        AddRef();                                 // Increment the reference count
        return S_OK;                              // Indicate success
    }

    *ppv = nullptr;                              // Set the output pointer to null if the interface is not supported
    return E_NOINTERFACE;                        // Indicate that the interface is not available
}

// CreateInstance: Creates a new instance of the CredentialProvider class
IFACEMETHODIMP BluetoothCredentialProviderFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv)
{
    if (pUnkOuter != nullptr)                   // COM aggregation is not supported
    {
        return CLASS_E_NOAGGREGATION;           // Return an error if aggregation is requested
    }

    // Allocate a new instance of the CredentialProvider class
    BluetoothCredentialProvider* provider = new BluetoothCredentialProvider();
    if (!provider)                              // Check if the allocation failed
    {
        return E_OUTOFMEMORY;                   // Return an out-of-memory error
    }

    // Query the new CredentialProvider instance for the requested interface
    HRESULT hr = provider->QueryInterface(riid, ppv);
    provider->Release();                        // Release the local reference to the CredentialProvider

    return hr;                                  // Return the result of the QueryInterface call
}

// LockServer: Locks or unlocks the server. Not implemented in this example.
IFACEMETHODIMP BluetoothCredentialProviderFactory::LockServer(BOOL fLock)
{
    return S_OK;                                // No locking functionality required
}
