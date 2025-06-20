
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <optional>     
#include <algorithm>
#include <sodium.h>
#include <limits>

#include "brave_web3_service.h"
#include "brave_web3_rpc.h"

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
        
        const std::string BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
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
            return;
        }

        if (decoded_bytes.size() != LENGTH) {
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
        return crypto_core_ed25519_is_valid_point(bytes.data()) == 1;
    }

    std::string Pubkey::get_pubkey_ipfs() const{
        const std::string base58_pubkey = this->toBase58();
        json pubkey_list = json::array();
        pubkey_list.push_back(base58_pubkey); 

        const std::optional<Solana_Rpc::commitment> confirm_level = Solana_Rpc::commitment();

        Solana_Rpc::SolanaRpcClient new_rpc_client = Solana_Rpc::SolanaRpcClient();

        const std::string method = "getAccountInfo";
        json params = Solana_Rpc::build_request_args(pubkey_list, confirm_level);

        std::optional<json> response = new_rpc_client.send_rpc_request(method, params);

        if (response.has_value()) {
            std::cout << "Response: " << response.value().dump(4) << std::endl;
        }else{
            std::cout << "noResponse" << std::endl;
        }

        return "1";
    }


    //--------------------solana web3 interface-------------------
    
    namespace Solana_web3_interface{
        /*
        *@name:         sha_256
        *@description:  get the hashed value
        *@input:        input_data: the data which will be hashed
        *               output_hash: pase in an empty std::array
        *@output:       null
        */
        void sha_256(const std::vector<uint8_t>& input_data, std::array<uint8_t, Pubkey::LENGTH>& output_hash){
            static_assert(Pubkey::LENGTH == crypto_hash_sha256_BYTES, "SHA256 output must be 32 bytes");

            if (crypto_hash_sha256(output_hash.data(), input_data.data(), input_data.size()) != 0) {
                throw std::runtime_error("libsodium SHA-256 hashing failed");
            }
        }


        /*
        *@name:         create_program_address_cxx
        *@description:  get the hashed value
        *@input:        seeds: the seed to calculate PDA
        *               program_id: the Pubkey of the program that generate the PDA
        *               out_error: output error happends in the run time
        *@output:       sucess: Pubkey
        *               fail: PubkeyError
        */
        std::optional<Pubkey> create_program_address_cxx(
            const std::vector<std::vector<uint8_t>>& seeds,
            const Pubkey& program_id,
            PubkeyError* out_error 
        ){
            //Determining the seeds' amounts
            if(seeds.size() > MAX_SEEDS){
                if(out_error){
                    *out_error = PubkeyError::MaxSeedLengthExceeded;
                }
                return std::nullopt;
            }
            
            //Determining the length of the seed 
            for(const std::vector<uint8_t> &seed: seeds){
                if(seed.size() > MAX_SEED_LEN){
                    if(out_error){
                        *out_error = PubkeyError::MaxSeedLengthExceeded;
                    }
                    return std::nullopt;
                }
            }

            //Combine all the seeds to one data
            std::vector<uint8_t> combined_data;
            size_t total_seeds_len = 0;
            for (const auto& seed: seeds){
                total_seeds_len += seed.size();
            }
            //Allocate capacity for data
            combined_data.reserve(total_seeds_len + Pubkey::LENGTH + PDA_MARKER.size());
            for (const auto& seed : seeds) {
                combined_data.insert(combined_data.end(), seed.begin(), seed.end());
            }

            // Append program_id bytes
            combined_data.insert(combined_data.end(), program_id.bytes.begin(), program_id.bytes.end());
            // Append the magic PDA marker
            combined_data.insert(combined_data.end(), PDA_MARKER.begin(), PDA_MARKER.end());

            std::array<uint8_t, Pubkey::LENGTH> hash_result;
            sha_256(combined_data, hash_result);

            Pubkey potential_pda(hash_result);

            if (potential_pda.is_on_curve()) {
                if (out_error) *out_error = PubkeyError::InvalidSeeds;
                return std::nullopt; 
            }

            return potential_pda;
        }


        /*
        *@name:         try_find_program_address_cxx
        *@description:  calculate the solana program PDA
        *@input:        seeds
        *               program_id
        *@output:       <Pubkey, uint8_t>
        *               Pubkey:     PDA key
        *               uint8_t:    bumps
        */
        std::optional<std::pair<Pubkey, uint8_t>> try_find_program_address_cxx(
            const std::vector<std::vector<uint8_t>>& seeds,
            const Pubkey& program_id
        ) {
            for (int bump = 255; bump >= 0; --bump) {
                std::vector<std::vector<uint8_t>> seeds_with_bump = seeds;
                seeds_with_bump.push_back({ static_cast<uint8_t>(bump) });

                PubkeyError error_code;
                std::optional<Pubkey> result_pubkey = create_program_address_cxx(seeds_with_bump, program_id, &error_code);

                if (result_pubkey) {
                    return std::make_optional(std::make_pair(*result_pubkey, static_cast<uint8_t>(bump)));
                } else if (error_code == PubkeyError::InvalidSeeds) {
                    // continue
                } else {
                    std::cerr << "Error in create_program_address_cxx (non-curve related): "
                            << static_cast<int>(error_code) << std::endl;
                    break;
                }
            }
            return std::nullopt;
        }


    }
}