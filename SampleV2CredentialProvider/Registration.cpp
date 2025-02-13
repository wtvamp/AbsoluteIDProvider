#include <windows.h>
#include <wincrypt.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Generate a pairing code (for example, a 6-digit code).
std::string GeneratePairingCode() {
    // For demonstration, using a fixed code.
    // In production, use a secure random generator.
    return "123456";
}

// Securely store the phone's registration data using DPAPI.
// This example writes the encrypted blob to a file named after the phone ID.
bool StorePhoneRegistration(const std::string& phoneId, const std::vector<BYTE>& publicKeyBlob) {
    // Combine phoneId and publicKeyBlob into a single byte vector.
    std::vector<BYTE> data;
    data.insert(data.end(), phoneId.begin(), phoneId.end());
    data.insert(data.end(), publicKeyBlob.begin(), publicKeyBlob.end());

    DATA_BLOB input;
    input.pbData = data.data();
    input.cbData = (DWORD)data.size();

    DATA_BLOB output;
    if (!CryptProtectData(&input, L"PhoneRegistration", nullptr, nullptr, nullptr, 0, &output)) {
        std::cerr << "CryptProtectData failed." << std::endl;
        return false;
    }

    std::string filename = phoneId + "_registration.dat";
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        LocalFree(output.pbData);
        return false;
    }
    file.write(reinterpret_cast<char*>(output.pbData), output.cbData);
    file.close();
    LocalFree(output.pbData);
    return true;
}

// Process the registration by comparing the provided pairing code with the expected one.
// If they match, store the phone's public key for later unlocks.
void ProcessRegistration(const std::string& receivedPairingCode,
    const std::string& expectedPairingCode,
    const std::string& phoneId,
    const std::vector<BYTE>& phonePublicKeyBlob)
{
    if (receivedPairingCode != expectedPairingCode) {
        std::cerr << "Pairing code mismatch. Registration failed." << std::endl;
        return;
    }

    if (StorePhoneRegistration(phoneId, phonePublicKeyBlob)) {
        std::cout << "Registration successful for phone: " << phoneId << std::endl;
    }
    else {
        std::cerr << "Registration failed while storing phone data." << std::endl;
    }
}

int main() {
    // Simulate PC displaying a pairing code.
    std::string pairingCode = GeneratePairingCode();
    std::cout << "Display this pairing code on the PC: " << pairingCode << std::endl;

    // In production, the phone app will capture this code and send it along with its data.
    // For demonstration, we use hardcoded values:
    std::string receivedPairingCode = "123456"; // Sent from phone app.
    std::string phoneId = "Phone123";

    // Simulated RSA public key blob (must be in BCRYPT_RSAPUBLIC_BLOB format).
    std::vector<BYTE> phonePublicKeyBlob = {
        0x52, 0x53, 0x41, 0x50, // "RSA1" magic value.
        0x00, 0x00, 0x00, 0x80, // Bit length (e.g., 2048 bits).
        0x00, 0x00, 0x00, 0x01, // Public exponent (example value, typically 0x10001).
        // Modulus bytes would follow here.
    };

    ProcessRegistration(receivedPairingCode, pairingCode, phoneId, phonePublicKeyBlob);
    return 0;
}
