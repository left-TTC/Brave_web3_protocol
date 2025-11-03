
#ifndef BRAVE_WEB3_SERVICER_H_
#define BRAVE_WEB3_SERVICER_H_

#include <iostream>
#include <array>
#include <tuple>
#include <vector>
#include <algorithm>
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "brave_web3_libsodium.h"
#include "base/json/json_reader.h"     
#include "base/json/json_writer.h"    
#include "base/logging.h"             
#include "base/values.h"
#include "base/functional/callback.h"
#include "url/gurl.h"

#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"


namespace Solana_web3{
    GURL RpcUrl();

    const size_t MAX_SEEDS = 16;

    const size_t MAX_SEED_LEN = 32;

    static const std::string PDA_MARKER = "ProgramDerivedAddress";  

    static const std::string PREFIX = "WEB3 Name Service";

    std::string EncodeBase58(const std::vector<uint8_t> &input);

    absl::optional<std::vector<uint8_t>> DecodeBase58(const std::string& input);


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
        }

        bool is_on_curve() const;

        void get_pubkey_ipfs(
            scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
            base::OnceCallback<void(const GURL&, bool is_web3_domain)> restart_callback,
            const std::string maybe_domain,
            const GURL& original_url
        ) const;

    };

    Pubkey return_NAMESERVICE();
    Pubkey return_RECORDSTATE();
    Pubkey return_REGISTERSTATE();

    struct PDA{
        Pubkey publickey;
        uint8_t bump;
    };

    const PDA empty_PDA = PDA{
        Pubkey(),
        uint8_t(-1),   
    };

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

        PDA get_account_from_root(const std::string& domain, const Pubkey &root_domain_account);
    }

    std::string extract_target_domain(const GURL& original_url);

    std::vector<std::string> split_host_by_dots(const std::string& url_host);

    std::tuple<int, bool, std::string> fast_find(const std::string maybe_domain, const std::vector<std::string>& vec);
}




#endif
