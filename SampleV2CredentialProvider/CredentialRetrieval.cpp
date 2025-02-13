#include <windows.h>
#include <wincrypt.h>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")

// Reads the encrypted credentials from the registry.
std::vector<BYTE> ReadRegistryCredentials()
{
    HKEY hKey;
    const wchar_t* subKey = L"SOFTWARE\Delmock\AbsoluteID";
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS)
    {
        std::wcerr << L"Failed to open registry key." << std::endl;
        return {};
    }

    DWORD dataSize = 0;
    result = RegQueryValueExW(hKey, L"Credentials", nullptr, nullptr, nullptr, &dataSize);
    if (result != ERROR_SUCCESS)
    {
        std::wcerr << L"Failed to query registry value size." << std::endl;
        RegCloseKey(hKey);
        return {};
    }

    std::vector<BYTE> encryptedData(dataSize);
    result = RegQueryValueExW(hKey, L"Credentials", nullptr, nullptr, encryptedData.data(), &dataSize);
    if (result != ERROR_SUCCESS)
    {
        std::wcerr << L"Failed to read registry value." << std::endl;
        RegCloseKey(hKey);
        return {};
    }
    RegCloseKey(hKey);
    return encryptedData;
}

// Decrypts the encrypted credentials using DPAPI.
std::string DecryptCredentials(const std::vector<BYTE>& encryptedData)
{
    DATA_BLOB input, output;
    input.pbData = const_cast<BYTE*>(encryptedData.data());
    input.cbData = static_cast<DWORD>(encryptedData.size());

    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, 0, &output))
    {
        std::cerr << "CryptUnprotectData failed: " << GetLastError() << std::endl;
        return "";
    }

    std::string credentials(reinterpret_cast<char*>(output.pbData), output.cbData);
    LocalFree(output.pbData);
    return credentials;
}

//int main()
//{
//    auto encryptedData = ReadRegistryCredentials();
//    if (encryptedData.empty())
//    {
//        std::cerr << "No encrypted data read from the registry." << std::endl;
//        return 1;
//    }
//
//    std::string credentials = DecryptCredentials(encryptedData);
//    if (credentials.empty())
//    {
//        std::cerr << "Failed to decrypt credentials." << std::endl;
//        return 1;
//    }
//
//    std::cout << "Decrypted Credentials: " << credentials << std::endl;
//
//    // Parse the credentials in the format "username:password".
//    size_t colonPos = credentials.find(':');
//    if (colonPos != std::string::npos)
//    {
//        std::string username = credentials.substr(0, colonPos);
//        std::string password = credentials.substr(colonPos + 1);
//        std::cout << "Username: " << username << "\nPassword: " << password << std::endl;
//
//        // At this point, you can use these credentials with LogonUser or pass them along to the Windows logon process.
//    }
//    else
//    {
//        std::cerr << "Invalid credentials format." << std::endl;
//    }
//    return 0;
//}
