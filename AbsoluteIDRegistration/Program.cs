// See https://aka.ms/new-console-template for more information
using AbsoluteIDRegistration;

Console.WriteLine("Welcome to the AbsoluteID Registration App");
// Simulate received data from the phone app:
var regData = new RegistrationData
{
    PhoneId = "Phone123",
    PublicKeyBlob = new byte[] { 0x52, 0x53, 0x41, 0x50 } // Dummy key blob.
};

// Convert registration data to a string.
string regDataStr = regData.ToString();
Console.WriteLine("Registration Data (Plain): " + regDataStr);

// Encrypt the registration data.
byte[] encryptedData = SecureStorage.EncryptRegistrationData(regDataStr);
SecureStorage.SaveToFile("registration.dat", encryptedData);
Console.WriteLine("Registration data saved securely.");

// Later, you can load and decrypt:
byte[] loadedData = SecureStorage.LoadFromFile("registration.dat");
string decryptedDataStr = SecureStorage.DecryptRegistrationData(loadedData);
RegistrationData loadedRegData = RegistrationData.FromString(decryptedDataStr);
Console.WriteLine("Loaded PhoneId: " + loadedRegData.PhoneId);