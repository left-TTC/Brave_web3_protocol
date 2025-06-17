
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <optional> 
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "brave_web3_service.h"

namespace Solana_web3 {
    //---------------------About base58------------------------
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
            result.push_back(BASE58_ALPHABET[0]);
        }
        for (auto it = digits.rbegin(); it != digits.rend(); it++) {
            result.push_back(BASE58_ALPHABET[*it]);
        }
        
        return result;
    }

    std::vector<uint8_t> DecodeBase58(const std::string& input) {
        std::vector<uint8_t> bytes;
        bytes.push_back(0);
        
        for (size_t i = 0; i < input.size(); i++) {
            size_t index = BASE58_ALPHABET.find(input[i]);
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
        
        for (size_t i = 0; i < input.size() && input[i] == BASE58_ALPHABET[0]; i++) {
            bytes.push_back(0);
        }
        
        std::reverse(bytes.begin(), bytes.end());
        return bytes;
    }


    //------------------Pubkey class implementations-------------
    
    //default Pubkey 000...00
    Pubkey::Pubkey() {
        std::fill(bytes.begin(), bytes.end(), 0);
    }

    //construct Pubkey by byte arrays
    Pubkey::Pubkey(const std::array<uint8_t, LENGTH>& b) : bytes(b) {}

    //construct Pubkey from string
    Pubkey::Pubkey(const std::string& pubkey_b58) {
        std::vector<uint8_t> decoded_bytes = DecodeBase58(pubkey_b58);

        if (decoded_bytes.empty()) {
            std::cerr << "Error: Pubkey constructor (string) failed to decode Base58 string: " << pubkey_b58 << std::endl;
            std::fill(bytes.begin(), bytes.end(), 0);
            return;
        }

        if (decoded_bytes.size() != LENGTH) {
            std::cerr << "Error: Pubkey constructor (string) decoded length (" << decoded_bytes.size()
                      << ") does not match expected Pubkey length (" << LENGTH << ") for: " << pubkey_b58 << std::endl;
            std::fill(bytes.begin(), bytes.end(), 0); // Initialize to all zeros on error
            return;
        }

        std::copy(decoded_bytes.begin(), decoded_bytes.end(), bytes.begin());
    }

    static std::vector<uint8_t> array_to_vector(const std::array<uint8_t, Pubkey::LENGTH>& arr) {
        return std::vector<uint8_t>(arr.begin(), arr.end());
    }

    std::string Pubkey::toBase58() const{
        return EncodeBase58(array_to_vector(this->bytes));
    }

    bool Pubkey::is_on_curve() const{

    }


    //--------------------solana web3 interface-------------------
    
    namespace Solana_web3_interface{
        void sha_256(const std::vector<uint8_t>& input_data, std::array<uint8_t, Pubkey::LENGTH>& output_hash){

        }

        // std::optional<Pubkey> create_program_address_cxx(
        //     const std::vector<std::vector<uint8_t>>& seeds,
        //     const Pubkey& program_id,
        //     PubkeyError* out_error 
        // ){

        // }

        // std::optional<std::pair<std::string, uint8_t>> try_find_program_address_cxx(
        //     const std::vector<std::vector<uint8_t>>& seeds,
        //     const Pubkey& program_id
        // ){

        // }
    }
}