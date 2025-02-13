using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;

namespace AbsoluteIDRegistration
{
    internal class CredentialStorage
    {
        // Registry path for your application data.
        private const string RegistryPath = @"SOFTWARE\Delmock\AbsoluteID";

        // Save the credentials to the registry.
        public static void SaveCredentials(string username, string password)
        {
            string credentialData = $"{username}:{password}";
            byte[] data = Encoding.UTF8.GetBytes(credentialData);

            // Encrypt using LocalMachine so any process on this machine can decrypt.
            byte[] encryptedData = ProtectedData.Protect(data, null, DataProtectionScope.LocalMachine);

            using (RegistryKey key = Registry.LocalMachine.CreateSubKey(RegistryPath))
            {
                if (key == null)
                {
                    throw new Exception("Unable to open registry key for writing.");
                }
                key.SetValue("Credentials", encryptedData, RegistryValueKind.Binary);
            }
            Console.WriteLine("Credentials stored in the registry securely.");
        }

        // Load and decrypt the credentials from the registry.
        public static (string Username, string Password) LoadCredentials()
        {
            byte[] encryptedData;
            using (RegistryKey key = Registry.LocalMachine.OpenSubKey(RegistryPath))
            {
                if (key == null)
                {
                    throw new Exception("Registry key not found.");
                }
                encryptedData = key.GetValue("Credentials") as byte[];
                if (encryptedData == null)
                {
                    throw new Exception("Credentials not found in the registry.");
                }
            }
            byte[] decryptedData = ProtectedData.Unprotect(encryptedData, null, DataProtectionScope.LocalMachine);
            string credentialData = Encoding.UTF8.GetString(decryptedData);
            string[] parts = credentialData.Split(':');
            if (parts.Length != 2)
            {
                throw new FormatException("Invalid credential format.");
            }
            return (parts[0], parts[1]);
        }
    }
}
