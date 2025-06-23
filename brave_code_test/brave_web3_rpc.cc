

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include "brave_web3_rpc.h"
#include <curl/curl.h> 
#include <cppcodec/base64_rfc4648.hpp>

#include "json.hpp"

using base64 = cppcodec::base64_rfc4648;

namespace Solana_Rpc{

    /**
     * function: the method function in solana_web3_interface
     *
     */


    /*
    *@description:  cosntruct   an entire json array(based on args)
    *@input:        args:       main args, such as [Publickey]
    *               commitment: confirm level
    *               encoding:   return value's code format
    *               extra:      other parameters
    *@output:       json:       constructed json        
    */
    json build_common_request_args(
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

            option_args["encoding"] = "base64";

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


    json build_request_json(
        const std::string& method,
        const json& parms,
        const std::optional<int> request_id,
        const bool fliters
    ){
        if(method == "getAccountInfo"){
            json request_json = {
                {"jsonrpc", "2.0"},
                {"id", request_id},
                {"method", method},
                {"params", parms}
            };

            return request_json;
        }else if(method == "getProgramAccounts" && fliters){
            const Solana_web3::Pubkey central = Solana_web3::Pubkey("ERCnYDoLmfPCvmSWB52mubuDgPb7CwgUmfourQJ3sK7d");
            //get root domains key
            return json::array({
                {
                    {"memcmp", {
                        {"offset", 32},
                        {"bytes", central.toBase58()}
                    }}
                },
                {
                    {"memcmp", {
                        {"offset", 0},
                        {"bytes", Solana_web3::Pubkey().toBase58()}
                    }}
                },
                {
                    {"memcmp", {
                        {"offset", 64},
                        {"bytes", Solana_web3::Pubkey().toBase58()}
                    }}
                }
            });
        }
    }


    /**
     * @brief   get account's on chain datas
     *
     * @param   publickey    accounts that needs to get data
     * @return  json         obtained data
     *          nullopt      no data has been retrieved
     */
    std::optional<json> get_account_info(Solana_web3::Pubkey publickey){
        //publickey's str
        const std::string publcikey_str = publickey.toBase58();
        json pubkey_list = json::array();
        pubkey_list.push_back(publcikey_str);

        //commitment -- default: confirm
        const std::optional<commitment> confirm_level = commitment();
        //methods: getAccountInfo
        const std::string method = "getAccountInfo";
        //it must be base64 when we want to get all datas
        const std::optional<std::string> base_64_encode = "base64";
        //build request params
        json param = build_common_request_args(pubkey_list, confirm_level, base_64_encode);
        json request_json = build_request_json(method, param);
        //send request
        SolanaRpcClient new_client = SolanaRpcClient();
        return new_client.send_rpc_request(request_json);
    }

    /**
     * @brief   get account's on chain datas, and extract cid from the datas
     *
     * @param   publickey    accounts that needs to get data
     * @return  json         obtained data
     *          nullopt      no data has been retrieved
     */
    std::string decodeAndStripPubkeys(const std::string& base64_str) {
        std::vector<uint8_t> decoded_bytes = cppcodec::base64_rfc4648::decode(base64_str);

        if (decoded_bytes.size() <= 96) {
            return "";
        }

        std::vector<uint8_t> remaining_bytes(decoded_bytes.begin() + 96, decoded_bytes.end());

        std::string remaining_str(remaining_bytes.begin(), remaining_bytes.end());

        return remaining_str;
    }


    /**
     * @brief   get account's on chain datas
     *
     * @param   json_str    acquired account json data
     * @return  string      the cid stored in domain's ipfs account data
     */
    std::string get_cid_from_json(std::optional<json> json_str){
        if (!json_str.has_value()) {
            return "";  
        }
            
        const json& could_parse_json = *json_str;

        if(could_parse_json.contains("value") && !could_parse_json["value"].is_null()){
            json json_value = could_parse_json["value"];

            std::string data_base64_string = "";
            if (json_value.contains("data") && json_value["data"].is_array() && json_value["data"].size() >= 1) {
                if (json_value["data"][0].is_string()) {
                    data_base64_string = json_value["data"][0].get<std::string>();
                }
            }else{
                return "";
            }

            return decodeAndStripPubkeys(data_base64_string);
        }

        return "";
    }

    

    Solana_web3::Pubkey get_all_root_domain(){

    }

    /**
     * @brief libcurl's write callback function.
     *
     * @param contents  A pointer to the data chunk received by libcurl.
     * @param size      The size in bytes of each data element. This is typically 1.
     * @param nmemb     The number of data elements. Thus, the actual number of bytes received is size * nmemb.
     * @param userp     A user-defined pointer, typically pointing to a std::string object
     * 
     * @return The number of bytes successfully processed. If the returned value is
     * not equal to size * nmemb, libcurl will consider the write operation
     * to have failed and abort the transfer.
     */
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    //convert type Commitment to string
    std::string commitment::commitment_to_string() const {
        static const std::unordered_map<Commitment, std::string> map = {
            {Commitment::Processed, "processed"},
            {Commitment::Confirmed, "confirmed"},
            {Commitment::Finalized, "finalized"},
        };

        return map.at(confirm_level);
    }

    //default construct function: level Confirmed
    commitment::commitment(){
        this->confirm_level = Commitment::Confirmed;
    }

    commitment::commitment(const Commitment &commitment){
        this->confirm_level = commitment;
    }


    /**
     * @brief send rpc request and receive data
     *
     * @param method        solana rpc method
     * @param params        solana rpc params
     * @param request_id    always be 1
     * 
     * @return    json      received json's result
     *            nullopt   error   
     */
    std::optional<json> SolanaRpcClient::send_rpc_request(
        const json& request
    )const {
        CURL* curl = curl_easy_init();

        if (!curl) {
            return std::nullopt;
        }

        if(request == ""){
            return std::nullopt;
        }

        std::string response_buffer;

        std::string request_body = request.dump(); 

        curl_easy_setopt(curl, CURLOPT_URL, rpc_url_.c_str());                  // Set the RPC URL
        curl_easy_setopt(curl, CURLOPT_POST, 1L);                               // Set as POST request
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());       // Set POST data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request_body.length());   // Set POST data length

        // Set HTTP headers: Content-Type for JSON
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set callback function to write response data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);

        // Optional: Set timeouts
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);         // Connection timeout 5 seconds
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);               // Total operation timeout 10 seconds

        // Execute the request
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            curl_slist_free_all(headers);                           // Clean up headers list
            curl_easy_cleanup(curl);                                // Clean up CURL handle
            return std::nullopt;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);// Get HTTP status code

        curl_slist_free_all(headers);                               // Clean up headers list
        curl_easy_cleanup(curl);                                    // Clean up CURL handle

        // 3. Process the HTTP response
        if (http_code == 200) {
            try {
                json response = json::parse(response_buffer); // Parse JSON response

                if (response.count("result")) {
                    return response["result"];                // Return the 'result' part
                } else if (response.count("error")) {
                    std::cerr << "RPC Error: " << response["error"].dump(4) << std::endl;
                    return std::nullopt;
                } else {
                    std::cerr << "Unexpected RPC response format: " << response.dump(4) << std::endl;
                    return std::nullopt;
                }
            } catch (const json::parse_error& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "An unexpected error occurred during JSON processing: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "HTTP Error: " << http_code << " - " << response_buffer << std::endl;
        }
        return std::nullopt;
    }



    /**
     * @brief   create a unique exaplem of the whole run
     * 
     * @return  use instance to get a ref to use it  
     */
    SolanaRootMap& SolanaRootMap::instance() {
        static SolanaRootMap instance;
        return instance;
    }

    /**
     * @brief   update all the root domain in the instance
     * 
     */
    void SolanaRootMap::set_all(const std::vector<std::string>& values) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_ = values;
    }

    /**
     * @brief   get all the root domain stored in the instance
     * 
     * @return  vector<string>  root domains  
     */
    std::vector<std::string> SolanaRootMap::get_all() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_; 
    }

}