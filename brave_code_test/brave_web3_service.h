

#ifndef BRAVE_WEB3_SERVICER_H_
#define BRAVE_WEB3_SERVICER_H_

#include <string>
#include <vector>
#include <array>
#include <optional> 
#include <stdexcept>

namespace Solana_web3 {

    //About generate PDA
    const size_t MAX_SEEDS = 16;

    const size_t MAX_SEED_LEN = 32;

    const std::array<uint8_t, 19> PDA_MARKER = { 
        80, 114, 111, 103, 114, 97, 109, 68, 101, 114, 105, 118, 101, 100, 65, 100, 100, 114, 101 // "ProgramDerivedAddress"
    };

    //About basa58
    const std::string BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

    std::string EncodeBase58(const std::vector<uint8_t> &input);

    std::vector<uint8_t> DecodeBase58(const std::string& input);

    class Pubkey {
    public:
        //Pubkey length
        static const size_t LENGTH = 32;
        
        std::array<uint8_t, LENGTH> bytes;

        //constructor function
        Pubkey();
        explicit Pubkey(const std::array<uint8_t, LENGTH>& b);
        explicit Pubkey(const std::string &pubkey);

        std::string toBase58() const;

        bool operator==(const Pubkey& other) const {
            return bytes == other.bytes;
        };

        bool is_on_curve() const;
    };

    enum class PubkeyError{
        InvalidSeeds,
        MaxSeedLengthExceeded,
    };

    namespace Solana_web3_interface{
        //the hash utils to calculate PDA
        
        /*
        *@name:         sha_256
        *@description:  get the hashed value
        *@input:        input_data: the data which will be hashed
        *               output_hash: pase in an empty std::array
        *@output:       null
        */
        void sha_256(const std::vector<uint8_t>& input_data, std::array<uint8_t, Pubkey::LENGTH>& output_hash);

        //
        std::optional<Pubkey> create_program_address_cxx(
            const std::vector<std::vector<uint8_t>>& seeds,
            const Pubkey& program_id,
            PubkeyError* out_error = nullptr 
        );

        std::optional<std::pair<std::string, uint8_t>> try_find_program_address_cxx(
            const std::vector<std::vector<uint8_t>>& seeds,
            const Pubkey& program_id
        );
    }
}



#endif

