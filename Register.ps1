# Define the GUID for your Credential Provider
$clsid = "{12345678-1234-1234-1234-1234567890AB}"

# Resolve the absolute path of the DLL
$dllPath = (Resolve-Path .\x64\Debug\BluetoothCredentialProvider.dll).Path

# Define registry keys for the Credential Provider
$credentialProviderKey = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\$clsid"
$clsidKey = "HKLM:\SOFTWARE\Classes\CLSID\$clsid"
$inprocServerKey = "$clsidKey\InprocServer32"

# Register the Credential Provider
Write-Host "Registering Credential Provider..." -ForegroundColor Cyan

# 1. Add the main Credential Provider key
New-Item -Path $credentialProviderKey -Force | Out-Null
Set-ItemProperty -Path $credentialProviderKey -Name "(Default)" -Value "BluetoothCredentialProvider"

# 2. Add the CLSID key
New-Item -Path $clsidKey -Force | Out-Null
Set-ItemProperty -Path $clsidKey -Name "(Default)" -Value "BluetoothCredentialProvider"

# 3. Add the InprocServer32 key with the DLL path and threading model
New-Item -Path $inprocServerKey -Force | Out-Null
Set-ItemProperty -Path $inprocServerKey -Name "(Default)" -Value $dllPath
Set-ItemProperty -Path $inprocServerKey -Name "ThreadingModel" -Value "Apartment"

Write-Host "Credential Provider registered successfully!" -ForegroundColor Green
