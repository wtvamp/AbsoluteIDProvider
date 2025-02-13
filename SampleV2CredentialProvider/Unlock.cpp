#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")

// Generate a random challenge string (hex encoded).
std::string GenerateChallenge(size_t length) {
    std::vector<BYTE> buffer(length);
    NTSTATUS status = BCryptGenRandom(nullptr, buffer.data(), (ULONG)length, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (!BCRYPT_SUCCESS(status)) {
        std::cerr << "BCryptGenRandom failed: 0x" << std::hex << status << std::endl;
        return "";
    }
    std::stringstream ss;
    for (BYTE b : buffer) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    }
    return ss.str();
}

// Compute the SHA-256 hash of the input data.
std::vector<BYTE> ComputeSHA256(const std::string& data) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status)) {
        std::cerr << "BCryptOpenAlgorithmProvider failed: 0x" << std::hex << status << std::endl;
        return {};
    }

    DWORD hashObjectSize = 0, result = 0;
    status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hashObjectSize, sizeof(DWORD), &result, 0);
    if (!BCRYPT_SUCCESS(status)) {
        std::cerr << "BCryptGetProperty (object length) failed: 0x" << std::hex << status << std::endl;
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return {};
    }

    std::vector<BYTE> hashObject(hashObjectSize);
    DWORD hashLength = 0;
    status = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PUCHAR)&hashLength, sizeof(DWORD), &result, 0);
    if (!BCRYPT_SUCCESS(status)) {
        std::cerr << "BCryptGetProperty (hash length) failed: 0x" << std::hex << status << std::endl;
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return {};
    }

    std::vector<BYTE> hash(hashLength);
    BCRYPT_HASH_HANDLE hHash = nullptr;
    status = BCryptCreateHash(hAlg, &hHash, hashObject.data(), hashObjectSize, nullptr, 0, 0);
    if (!BCRYPT_SUCCESS(status)) {
        std::cerr << "BCryptCreateHash failed: 0x" << std::hex << status << std::endl;
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return {};
    }

    status = BCryptHashData(hHash, (PUCHAR)data.data(), (ULONG)data.size(), 0);
    if (!BCRYPT_SUCCESS(status)) {
        std::cerr << "BCryptHashData failed: 0x" << std::hex << status << std::endl;
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return {};
    }

    status = BCryptFinishHash(hHash, hash.data(), hashLength, 0);
    if (!BCRYPT_SUCCESS(status)) {
        std::cerr << "BCryptFinishHash failed: 0x" << std::hex << status << std::endl;
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return {};
    }

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return hash;
}

// Import an RSA public key from a key blob.
BCRYPT_KEY_HANDLE ImportRSAPublicKey(const std::vector<BYTE>& keyBlob) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_KEY_HANDLE hKey = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RSA_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status)) {
        std::cerr << "BCryptOpenAlgorithmProvider for RSA failed: 0x" << std::hex << status << std::endl;
        return nullptr;
    }
    status = BCryptImportKeyPair(hAlg, nullptr, BCRYPT_RSAPUBLIC_BLOB, &hKey, (PUCHAR)keyBlob.data(), (ULONG)keyBlob.size(), 0);
    if (!BCRYPT_SUCCESS(status)) {
        std::cerr << "BCryptImportKeyPair failed: 0x" << std::hex << status << std::endl;
        hKey = nullptr;
    }
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return hKey;
}

// Verify the phone's signature using the stored public key.
// The phone is expected to sign the SHA-256 hash of the challenge.
bool VerifySignature(const std::string& challenge, const std::vector<BYTE>& signature, BCRYPT_KEY_HANDLE hPublicKey) {
    auto hash = ComputeSHA256(challenge);
    if (hash.empty()) {
        return false;
    }
    NTSTATUS status = BCryptVerifySignature(
        hPublicKey,
        nullptr, // For RSA PKCS#1 v1.5, adjust padding info if necessary.
        hash.data(),
        (ULONG)hash.size(),
        (PUCHAR)signature.data(),
        (ULONG)signature.size(),
        0
    );
    if (!BCRYPT_SUCCESS(status)) {
        std::cerr << "BCryptVerifySignature failed: 0x" << std::hex << status << std::endl;
        return false;
    }
    return true;
}

// Load the stored registration data for the given phone.
// For this example, we assume the file format is the same as written during registration.
bool LoadPhoneRegistration(const std::string& phoneId, std::vector<BYTE>& publicKeyBlob) {
    std::string filename = phoneId + "_registration.dat";
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open registration file for phone: " << phoneId << std::endl;
        return false;
    }
    std::vector<BYTE> encryptedData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    DATA_BLOB input, output;
    input.pbData = encryptedData.data();
    input.cbData = (DWORD)encryptedData.size();

    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, 0, &output)) {
        std::cerr << "CryptUnprotectData failed." << std::endl;
        return false;
    }
    // In this simple example, assume the phone ID is stored at the beginning.
    // We skip the phone ID bytes and retrieve the public key blob.
    size_t phoneIdLength = phoneId.size();
    publicKeyBlob.assign(((BYTE*)output.pbData) + phoneIdLength, ((BYTE*)output.pbData) + output.cbData);
    LocalFree(output.pbData);
    return true;
}

//int main() {
//    // For the unlock process, assume the PC receives:
//    // 1. The phone's identifier.
//    // 2. A signature over a challenge.
//    std::string phoneId = "Phone123";
//
//    // Simulated signature received from the phone.
//    // In a real scenario, this signature comes from the phone app over Wi‑Fi or Bluetooth.
//    std::vector<BYTE> receivedSignature = {
//        // Replace these bytes with the actual signature data.
//        0x00, 0x01, 0x02, 0x03
//    };
//
//    // Generate a challenge string.
//    std::string challenge = GenerateChallenge(32); // 32 bytes.
//    std::cout << "Generated challenge: " << challenge << std::endl;
//
//    // The challenge would typically be sent to the phone which then returns a signature.
//    // For this demonstration, we assume that process has already taken place.
//
//    // Retrieve the stored public key from the registration data.
//    std::vector<BYTE> publicKeyBlob;
//    if (!LoadPhoneRegistration(phoneId, publicKeyBlob)) {
//        std::cerr << "Failed to load phone registration data." << std::endl;
//        return 1;
//    }
//
//    BCRYPT_KEY_HANDLE hPublicKey = ImportRSAPublicKey(publicKeyBlob);
//    if (hPublicKey == nullptr) {
//        std::cerr << "Failed to import RSA public key for phone: " << phoneId << std::endl;
//        return 1;
//    }
//
//    // Verify the phone's signature.
//    if (VerifySignature(challenge, receivedSignature, hPublicKey)) {
//        std::cout << "Unlock successful. PC is unlocked." << std::endl;
//        // Code to unlock the PC goes here.
//    }
//    else {
//        std::cerr << "Unlock failed. Signature verification unsuccessful." << std::endl;
//    }
//
//    if (hPublicKey) {
//        BCryptDestroyKey(hPublicKey);
//    }
//    return 0;
//}
