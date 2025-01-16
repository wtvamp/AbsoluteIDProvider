#pragma once
#include "pch.h"
#include <credentialprovider.h>
#include "BluetoothCredentialProviderCredential.h"

class BluetoothCredentialProviderFactory : public IClassFactory
{
public:
    BluetoothCredentialProviderFactory();
    virtual ~BluetoothCredentialProviderFactory();

    // IUnknown
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv);

    // IClassFactory
    IFACEMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv);
    IFACEMETHODIMP LockServer(BOOL fLock);

private:
    LONG _cRef;
};
