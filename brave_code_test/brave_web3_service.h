

#ifndef BRAVE_WEB3_SERVICER_H_
#define BRAVE_WEB3_SERVICER_H_


// #include "net/url_request/url_request_interceptor.h" 
// #include "url/gurl.h"

#include <string>
#include <vector>
#include <cstdint>
#include <ed25519.h>

namespace fanmocheng {

    const std::string ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

    std::string EncodeBase58(const std::vector<uint8_t> &input);

    std::vector<uint8_t> DecodeBase58(const std::string& input);

    namespace Web3Service  {
        std::string FindProgramAddressSync(const std::vector<unsigned char>& publickey, const std::vector<unsigned char>& programid);



    };

    class PublicKey {
    public:
        static const size_t LENGTH = 32;
        //store the publickey's byte data
        std::vector<uint8_t> bytes;

        //The default constructor initializes a public key object
        //PublicKey and initializes the bytes array to 32 zero bytes. 
        //In this way, if the user does not pass the public key data, 
        //it will get a default, invalid public key
        PublicKey() : bytes(LENGTH, 0) {}

        explicit PublicKey(const std::vector<uint8_t>& bytes){
            if(bytes.size() != LENGTH){
                throw std::runtime_error("Invalid public key length");
            }
            this->bytes = bytes;
        }

        explicit PublicKey(const std::string &base58Key){
            this->bytes = fanmocheng::DecodeBase58(base58Key);
            if(bytes.size() != LENGTH){
                throw std::runtime_error("Invalid public key length");
            }
        }

        std::string toBase58() const {
            return fanmocheng::EncodeBase58(bytes);
        }

        bool isOnCurve() const {
            return ed25519
        }
    };
}



#endif
