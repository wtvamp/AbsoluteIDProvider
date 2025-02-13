using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AbsoluteIDRegistration
{
    internal class RegistrationData
    {
        public string PhoneId { get; set; }
        public byte[] PublicKeyBlob { get; set; }

        public override string ToString()
        {
            // Serialize the data as "PhoneId:Base64EncodedKey"
            return $"{PhoneId}:{Convert.ToBase64String(PublicKeyBlob)}";
        }

        public static RegistrationData FromString(string data)
        {
            var parts = data.Split(':');
            if (parts.Length != 2)
                throw new FormatException("Invalid registration data format.");
            return new RegistrationData
            {
                PhoneId = parts[0],
                PublicKeyBlob = Convert.FromBase64String(parts[1])
            };
        }
    }
}
