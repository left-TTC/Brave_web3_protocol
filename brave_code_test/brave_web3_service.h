

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

    static const std::string PREFIX = "WEB3 Name Service";

    std::string EncodeBase58(const std::vector<uint8_t> &input);

    std::vector<uint8_t> DecodeBase58(const std::string& input);

    
    class Pubkey {
    public:
        static const size_t LENGTH = 32;
        
        std::array<uint8_t, LENGTH> bytes;

        Pubkey();
        explicit Pubkey(const std::array<uint8_t, LENGTH>& b);
        explicit Pubkey(const std::string &pubkey);

        std::string toBase58() const;

        std::size_t size() const;

        bool operator==(const Pubkey& other) const {
            return bytes == other.bytes;
        };

        bool is_on_curve() const;

        std::string get_pubkey_ipfs() const;
    };

    struct PDA{
        Pubkey publickey;
        uint8_t bump;
    };

    const PDA empty_PDA = PDA{
        Pubkey(),
        uint8_t(-1),   
    };

    enum class PubkeyError{
        InvalidSeeds,
        MaxSeedLengthExceeded,
    };

    static const Pubkey WEB3_NAME_SERVICE = Pubkey("8YXaA8pzJ4xVPjYY8b5HkxmPWixwpZu7gVcj8EvHxRDC");
    static const Pubkey WEB3_AUCTION_SERVICE = Pubkey("");
    static const Pubkey CENTRAL_STATE_AUCTION = Pubkey("2DnqJcAMA5LPXcQN1Ep1rJNbyXXSofmbXdcweLwyoKq7");
    static const Pubkey CENTRAL_RECORD_STATE = Pubkey("Ha57yHecoA8iepHAX4LY6wG8YWnr47MjcZaPGN3G7XAv");
    namespace Solana_web3_interface{
        //the hash utils to calculate PDA
        
        void sha_256(const std::vector<uint8_t>& input_data, std::array<uint8_t, Pubkey::LENGTH>& output_hash);

        std::optional<Pubkey> create_program_address_cxx(
            const std::vector<std::vector<uint8_t>>& seeds,
            const Pubkey& program_id
        );
        
        PDA try_find_program_address_cxx(
            const std::vector<std::vector<uint8_t>>& seeds,
            const Pubkey& program_id
        );

        PDA get_cid_from_json_account(const std::string& domain, const Pubkey &root_domain_account);
    }
}



#endif

