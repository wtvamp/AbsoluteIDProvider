#pragma once // Ensure this header is included only once during compilation
#include "pch.h" // Include precompiled header for performance optimization
#include <windows.h> // Include Windows-specific definitions
#include <credentialprovider.h> // Include Credential Provider-specific definitions

class BluetoothCredentialProviderCredential : public ICredentialProviderCredential
{
public:
    // Constructor and destructor
    BluetoothCredentialProviderCredential();
    virtual ~BluetoothCredentialProviderCredential();

    // IUnknown
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv);

    // ICredentialProviderCredential methods
    IFACEMETHODIMP Advise(ICredentialProviderCredentialEvents* pcce);
    IFACEMETHODIMP UnAdvise();
    IFACEMETHODIMP SetSelected(BOOL* pbAutoLogon);
    IFACEMETHODIMP SetDeselected();
    IFACEMETHODIMP GetFieldState(DWORD dwFieldID, CREDENTIAL_PROVIDER_FIELD_STATE* pcpfs, CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* pcpfis);
    IFACEMETHODIMP GetStringValue(DWORD dwFieldID, PWSTR* ppwsz);
    IFACEMETHODIMP GetBitmapValue(DWORD dwFieldID, HBITMAP* phbmp);
    IFACEMETHODIMP Submit();
    IFACEMETHODIMP GetSerialization(
        CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
        CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
        PWSTR* ppszOptionalStatusText,
        CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon
    );
    IFACEMETHODIMP ReportResult(NTSTATUS ntsStatus, NTSTATUS ntsSubstatus, PWSTR* ppwszOptionalStatusText, CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon);

    // Checkbox handling
    IFACEMETHODIMP GetCheckboxValue(DWORD dwFieldID, BOOL* pbChecked, PWSTR* ppwszLabel);
    IFACEMETHODIMP SetCheckboxValue(DWORD dwFieldID, BOOL bChecked);

    // Submit button handling
    IFACEMETHODIMP GetSubmitButtonValue(DWORD dwFieldID, DWORD* pdwAdjacentTo);

    // Combo box handling
    IFACEMETHODIMP GetComboBoxCount(DWORD dwFieldID, DWORD* pcItems, DWORD* pdwSelectedItem);
    IFACEMETHODIMP GetComboBoxValueAt(DWORD dwFieldID, DWORD dwItem, PWSTR* ppwszItem);
    IFACEMETHODIMP SetComboBoxSelectedValue(DWORD dwFieldID, DWORD dwSelectedItem);

    // Command link handling
    IFACEMETHODIMP CommandLinkClicked(DWORD dwFieldID);

    // Additional methods required by ICredentialProviderCredential
    IFACEMETHODIMP GetComboBoxValueCount(DWORD dwFieldID, DWORD* pcItems, DWORD* pdwSelectedItem);
    IFACEMETHODIMP SetStringValue(DWORD dwFieldID, LPCWSTR psz);


    // BLE proximity helper
    BOOL CheckBluetoothProximity();

private:
    LONG _cRef; // Reference counter
};
