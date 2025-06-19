

#ifndef BRAVE_WEB3_SERVICER_H_
#define BRAVE_WEB3_SERVICER_H_

#include <string>
#include <vector>
#include <array>
#include <optional> 
#include <stdexcept>
#include <sodium.h>

namespace Solana_web3 {

    const std::string rpc_url = "https://api.devnet.solana.com";

    //About generate PDA
    const size_t MAX_SEEDS = 16;

    const size_t MAX_SEED_LEN = 32;

    static const std::string PDA_MARKER = "ProgramDerivedAddress";  

    //About basa58
    static const std::string BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

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

        std::string get_pubkey_ipfs() const;
    };

    enum class PubkeyError{
        InvalidSeeds,
        MaxSeedLengthExceeded,
    };

    namespace Solana_web3_interface{
        //the hash utils to calculate PDA
        
        void sha_256(const std::vector<uint8_t>& input_data, std::array<uint8_t, Pubkey::LENGTH>& output_hash);

        std::optional<Pubkey> create_program_address_cxx(
            const std::vector<std::vector<uint8_t>>& seeds,
            const Pubkey& program_id,
            PubkeyError* out_error = nullptr 
        );
        
        std::optional<std::pair<Pubkey, uint8_t>> try_find_program_address_cxx(
            const std::vector<std::vector<uint8_t>>& seeds,
            const Pubkey& program_id
        );
    }
}



#endif

