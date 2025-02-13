using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;

namespace AbsoluteIDRegistration
{
    internal class SecureStorage
    {
        public static byte[] EncryptRegistrationData(string registrationData)
        {
            // Use CurrentUser scope so only this user can decrypt.
            byte[] data = Encoding.UTF8.GetBytes(registrationData);
            return ProtectedData.Protect(data, null, DataProtectionScope.CurrentUser);
        }

        public static string DecryptRegistrationData(byte[] encryptedData)
        {
            byte[] decrypted = ProtectedData.Unprotect(encryptedData, null, DataProtectionScope.CurrentUser);
            return Encoding.UTF8.GetString(decrypted);
        }

        public static void SaveToFile(string filePath, byte[] encryptedData)
        {
            File.WriteAllBytes(filePath, encryptedData);
        }

        public static byte[] LoadFromFile(string filePath)
        {
            return File.ReadAllBytes(filePath);
        }
    }
}
