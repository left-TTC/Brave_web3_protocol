

#include <stdexcept>
#include <unordered_map>
#include "brave_web3_rpc.h"
#include <curl/curl.h> 

#include "json.hpp"


namespace Solana_Rpc{
    //---------------Solana RPC client-------------









    //---------------Solana RPC utils--------------
    /*
    *@name:         buildArgs
    *@description:  cosntruct   an entire json array(based on args)
    *@input:        args:       main args, such as [Publickey]
    *               commitment: confirm level
    *               encoding:   return value's code format
    *               extra:      other parameters
    *@output:       json:       constructed json        
    */
    json build_request_args(
        const std::vector<json>& pubkey_array,
        const std::optional<commitment>& commitment,
        const std::optional<std::string>& encoding,
        const json& extra
    ){
        json args_clone = pubkey_array;

        if(encoding || commitment || !extra.empty()){
            json option_args;

            if (encoding) {
                option_args["encoding"] = *encoding;
            }
            
            if (commitment) {
                option_args["commitment"] = commitment->commitment_to_string();
            }
            
            if (!extra.empty()) {
                option_args.update(extra); 
            }
            
            args_clone.push_back(option_args);
        }
        return args_clone;
    }


    /*
    *@name:         build_rpc_request
    *@description:  cosntruct an entire rpc json
    *@input:        params:     the return value of build_request_args
    *               id:         request id(default 1)
    *               method:     rpc request method(always be getAccountInfo in our browser)
    *@output:       json:       constructed rpc json        
    */
    json build_rpc_request(
        const std::string& method,
        const json& params,
        int id
    ){
        json rpc_requet{
            {"jsonrpc", "2.0"},
            {"id", id},
            {"method", method},
            {"params", params}
        };
        return rpc_requet;
    }



    //----------------Main function----------------

    std::string get_account_info(){

    }


    Solana_web3::Pubkey get_all_root_domain(){

    }


    //---------------class commitment--------------
    std::string commitment::commitment_to_string() const {
        static const std::unordered_map<Commitment, std::string> map = {
            {Commitment::Processed, "processed"},
            {Commitment::Confirmed, "confirmed"},
            {Commitment::Finalized, "finalized"},
        };

        return map.at(confirm_level);
    }

    commitment::commitment(){
        this->confirm_level = Commitment::Confirmed;
    }

    commitment::commitment(const Commitment &commitment){
        this->confirm_level = commitment;
    }

}