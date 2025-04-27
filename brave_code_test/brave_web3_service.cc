
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <openssl/sha.h>  // OpenSSL库用于SHA-256哈希
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "brave_web3_service.h"

namespace fanmocheng {
    

    std::string EncodeBase58(const std::vector<uint8_t> &input){
        std::vector<uint8_t> digits;
        digits.push_back(0);
        
        for (size_t i = 0; i < input.size(); i++) {
            uint32_t carry = input[i];
            
            for (size_t j = 0; j < digits.size(); j++) {
                carry += digits[j] << 8;
                digits[j] = carry % 58;
                carry = carry / 58;
            }
            
            while (carry > 0) {
                digits.push_back(carry % 58);
                carry = carry / 58;
            }
        }
        
        std::string result;
        for (size_t i = 0; i < input.size() && input[i] == 0; i++) {
            result.push_back(ALPHABET[0]);
        }
        
        for (auto it = digits.rbegin(); it != digits.rend(); it++) {
            result.push_back(ALPHABET[*it]);
        }
        
        return result;
    }


    std::vector<uint8_t> DecodeBase58(const std::string& input) {
        std::vector<uint8_t> bytes;
        bytes.push_back(0);
        
        for (size_t i = 0; i < input.size(); i++) {
            size_t index = ALPHABET.find(input[i]);
            if (index == std::string::npos) {
                throw std::runtime_error("Invalid Base58 character");
            }
            
            uint32_t carry = static_cast<uint32_t>(index);
            
            for (size_t j = 0; j < bytes.size(); j++) {
                carry += bytes[j] * 58;
                bytes[j] = carry & 0xff;
                carry >>= 8;
            }
            
            while (carry > 0) {
                bytes.push_back(carry & 0xff);
                carry >>= 8;
            }
        }
        
        for (size_t i = 0; i < input.size() && input[i] == ALPHABET[0]; i++) {
            bytes.push_back(0);
        }
        
        std::reverse(bytes.begin(), bytes.end());
        return bytes;
    }

}