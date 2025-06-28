
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <optional>     
#include <algorithm>
#include <sodium.h>

#include "brave_web3_service.h"
#include "brave_web3_rpc.h"

namespace Solana_web3 {
    // ===========================================================
    //  ██████  SECTION 1: Base58 decode and encode tool  ██████
    // ===========================================================
    /**
     * function: base58 encode and decode
     */


    /**
     * @brief convert bytes to readable string
     */
    std::string EncodeBase58(const std::vector<uint8_t> &input){
        std::vector<uint8_t> digits;
        digits.push_back(0);

        const std::string BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
        
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

    /**
     * @brief convert string to uint8 bytes
     */
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

    


    // ===========================================================
    //  ██████  SECTION 2: Pubkey class  ██████
    // ===========================================================
    /**
     * function: Pubkey class's method and implementation
     */

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

    std::size_t Pubkey::size() const{
        return this->bytes.size();
    }

    bool Pubkey::is_on_curve() const{
        return is_solana_PDA_valid(bytes.data()) != 0;
    }
    
    /**
     * @brief Get the ipfs cid stored in the legitimate account
     *
     * @return string: if the account being queried is legitimate, return cid
     *                 if the account is invalid, return ""
     */
    std::string Pubkey::get_pubkey_ipfs() const{
        std::optional<json> response = Solana_Rpc::get_account_info(*this);

        if (response.has_value()) {
            std::cout << "Response: " << response.value().dump(4) << std::endl;
            const std::string cid = Solana_Rpc::get_cid_from_json(response);
            std::cout<< "cid:" << cid << std::endl; 

            return cid;
        }

        return "";
    }

    // ===========================================================
    //  ██████  SECTION 3: Universal Functions  ██████
    // ===========================================================
    /**
     * function: the method function in solana_web3_interface
     */
    
    namespace Solana_web3_interface{
        
       /**
        * @brief get the hashed value
        *
        * @param input_data   the data which will be hashed.
        * @param output_hash  pase in an empty std::array to receive the hashed value.
        */
        void sha_256(const std::vector<uint8_t>& input_data, std::array<uint8_t, Pubkey::LENGTH>& output_hash){
            static_assert(Pubkey::LENGTH == crypto_hash_sha256_BYTES, "SHA256 output must be 32 bytes");

            if (crypto_hash_sha256(output_hash.data(), input_data.data(), input_data.size()) != 0) {
                throw std::runtime_error("libsodium SHA-256 hashing failed");
            }
        }


        /**
         * @brief check the legality of PDA
         *
         * @param seeds         combined seeds(include bump in the last place)
         * @param program_id    PDA owner   
         * 
         * @return   Pubkey     A pda admmited by solana program
         *           nullopt    pda check return error
         *          
         */
        std::optional<Pubkey> create_program_address_cxx(
            const std::vector<std::vector<uint8_t>>& seeds,
            const Pubkey& program_id
        ){
            //Determining the seeds' amounts
            if(seeds.size() > MAX_SEEDS){
                return std::nullopt;
            }
            
            //Determining the length of the seed 
            for(const std::vector<uint8_t> &seed: seeds){
                if(seed.size() > MAX_SEED_LEN){
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

            Pubkey publickey(hash_result);

            if (publickey.is_on_curve()) {
                return std::nullopt;
            }

            return publickey;
        }


        /**
         * @brief try to find the solana program pda by related seeds
         *
         * @param seeds         combined seeds(without bump in the last place)
         * @param program_id    PDA owner   
         * 
         * @return   PDA        verfied PDA
         *           empty_PDA  failed_PDA
         *          
         */
        PDA try_find_program_address_cxx(
            const std::vector<std::vector<uint8_t>>& seeds,
            const Pubkey& program_id
        ) {
            if(sodium_init() < 0){
                return empty_PDA;
            }

            for(uint8_t bump = 255; bump >= 0; --bump){
                std::vector<std::vector<uint8_t>> seeds_with_bump = seeds;
                seeds_with_bump.push_back({ static_cast<uint8_t>(bump) });

                const std::optional<Pubkey> create_res = create_program_address_cxx(seeds_with_bump, program_id);

                if(create_res.has_value()){
                    const PDA result = PDA{
                        create_res.value(),
                        bump,
                    };
                    return result;
                }
            }

            return empty_PDA;
        }


        PDA get_cid_from_json_account(const std::string& domain, const Pubkey &root_domain_account){
            const std::string combined_domain = PREFIX + domain;
            const std::vector<uint8_t> combined_domain_bytes(combined_domain.begin(), combined_domain.end());
            
            std::array<uint8_t, Pubkey::LENGTH> hash_domain;
            sha_256(combined_domain_bytes, hash_domain);
            std::vector<std::vector<uint8_t>> domain_account_seeds;

            domain_account_seeds.push_back(std::vector<uint8_t>(hash_domain.begin(), hash_domain.end()));
            domain_account_seeds.push_back(std::vector<uint8_t>(32, 0));
            domain_account_seeds.push_back(std::vector<uint8_t>(root_domain_account.bytes.begin(), root_domain_account.bytes.end()));
            //get domain account key
            auto domain_account_PDA = try_find_program_address_cxx(domain_account_seeds, WEB3_NAME_SERVICE);
        
            //now we can calculate the ipfs account
            std::vector<std::vector<uint8_t>> domain_ipfs_seeds;
            const std::string combined_ipfs = PREFIX + "IPFS";
            const std::vector<uint8_t> ipfs_account_bytes(combined_ipfs.begin(), combined_ipfs.end());

            std::array<uint8_t, Pubkey::LENGTH> hash_ipfs;
            sha_256(ipfs_account_bytes, hash_ipfs);
            std::vector<std::vector<uint8_t>> ipfs_account_seeds;

            ipfs_account_seeds.push_back(std::vector<uint8_t>(hash_ipfs.begin(), hash_ipfs.end()));
            ipfs_account_seeds.push_back(std::vector<uint8_t>(CENTRAL_RECORD_STATE.bytes.begin(), CENTRAL_RECORD_STATE.bytes.end()));
            ipfs_account_seeds.push_back(std::vector<uint8_t>(domain_account_PDA.publickey.bytes.begin(), domain_account_PDA.publickey.bytes.end()));

            auto ipfs_account_PDA = try_find_program_address_cxx(ipfs_account_seeds, WEB3_NAME_SERVICE);

            return ipfs_account_PDA;
            
        }


    }
}