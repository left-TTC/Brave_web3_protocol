

#ifndef BRAVE_WEB3_RPC_H_
#define BRAVE_WEB3_RPC_H_

#include "json.hpp" 

#include "brave_web3_service.h"

using json = nlohmann::json;

namespace Solana_Rpc{
    class SolanaRpcClient{
    public:
    };

    enum class Commitment {
        Processed,
        Confirmed,
        Finalized,
    };

    class commitment{
    public:
        Commitment confirm_level;

        commitment();
        explicit commitment(const Commitment &commitment);

        std::string commitment_to_string() const;
    };

    const Solana_web3::Pubkey WEB3_AUCTION_SERVICE = Solana_web3::Pubkey("9qQuHLMAJEehtk47nKbY1cMAL1bVD7nQxno4SJRDth7");
    const Solana_web3::Pubkey WEB3_NAME_SERVICE = Solana_web3::Pubkey("8YXaA8pzJ4xVPjYY8b5HkxmPWixwpZu7gVcj8EvHxRDC");
    

    Solana_web3::Pubkey get_all_root_domain();

    std::string get_account_info(Solana_web3::Pubkey publickey);

    json build_request_args(
        const std::vector<json>& pubkey_array,
        const std::optional<commitment>& commitment = std::nullopt,
        const std::optional<std::string>& encoding = std::nullopt,
        const json& extra = {}
    );

    json build_rpc_request(
        const std::string& method,
        const json& params,
        int id = 1
    );

    // const std::string[] 

    

}



#endif
