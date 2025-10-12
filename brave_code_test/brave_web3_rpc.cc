

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


    /**
     * @brief   build the account data request json
     *
     * @param   pubkey_array    querying publickey
     * @param   commitment      should always be confirmed
     * @param   encoding        default null
     * @param   extra           extral params
     * @return  json            constructed json request params
     */
    json build_common_request_args(
        const std::vector<json>& pubkey_array
    ){
        json args_clone;
        std::cout << "pubkey json size:" << pubkey_array.size() << std::endl;
        if(pubkey_array.size() > 1){
            json pubkey_list = json::array();
            for (const auto& pk : pubkey_array) {
                pubkey_list.push_back(pk);
            }
            args_clone.push_back(pubkey_list);
        }else{
            args_clone = pubkey_array;
        }

        json option_args;

        option_args["encoding"] = "base64";

        option_args["commitment"] = "confirmed";
        
        args_clone.push_back(option_args);
        
        std::cout << "build_common_request_args: " << args_clone << std::endl;
        return args_clone;
    }
    
    /**
     * @brief   return a fixed root domain fliter json
     *
     * @return  json   constructed root domain fliter json
     */
    json build_root_fliters(){
        //get root domains key

        return json::array({
            {
                {"memcmp", {
                    {"offset", 32},
                    {"bytes", CENTRAL_REGISTER_STATE.toBase58()}
                }}
            },
            {
                {"memcmp", {
                    {"offset", 0},
                    {"bytes", "11111111111111111111111111111111"}
                }}
            },
            {
                {"memcmp", {
                    {"offset", 64},
                    {"bytes", "11111111111111111111111111111111"}
                }}
            }
        });
    }

    json build_request_json(
        const std::string& method,
        const json& parms,
        const std::optional<int> request_id,
        const bool fliters
    ){
        if((method == "getAccountInfo") || (method == "getMultipleAccounts")){
            return json{
                {"jsonrpc", "2.0"},
                {"id", request_id},
                {"method", method},
                {"params", parms}
            };
        }else if(method == "getProgramAccounts" && fliters){
            return json{
                {"jsonrpc", "2.0"},
                {"id", request_id.value_or(1)},
                {"method", method},
                {"params", json::array({
                    WEB3_NAME_SERVICE.toBase58(),
                    {
                        {"dataSlice", {
                            {"offset", 0},
                            {"length", 0}
                        }},
                        {"encoding", "base64"},
                        {"filters", parms}
                    }
                })}
            };
        }

        return json{};
    }


    /**
     * @brief   get account's on chain datas
     *
     * @param   publickey    account that needs to get data
     * @return  json         obtained data
     *          nullopt      no data has been retrieved
     */
    std::optional<json> get_account_info(json publickey){

        //commitment -- default: confirm

        //methods: getAccountInfo
        const std::string method = "getAccountInfo";

        //build request params
        json param = build_common_request_args(publickey);
        json request_json = build_request_json(method, param);
        std::cout << "request: " << request_json << std::endl;

        //send request
        SolanaRpcClient new_client = SolanaRpcClient();
        return new_client.send_rpc_request(request_json);
    }

    /**
     * @brief   get accounts' on chain datas
     *
     * @param   publickeys   accounts that needs to get data
     * @return  json         obtained data
     *          nullopt      no data has been retrieved
     */
    std::optional<json> get_multiple_account_info(json publickeys){

        const std::string method = "getMultipleAccounts";

        json params = build_common_request_args(publickeys);
        json request_json = build_request_json(method, params);

        std::cout << "multiple request: " << request_json << std::endl;

        SolanaRpcClient new_client = SolanaRpcClient();
        return new_client.send_rpc_request(request_json);
    }


    int base64_char_value(char c) {
        if ('A' <= c && c <= 'Z') return c - 'A';
        if ('a' <= c && c <= 'z') return c - 'a' + 26;
        if ('0' <= c && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1; 
    }

    int base64_decode(const char* input, std::vector<uint8_t>& output) {
        size_t input_len = strlen(input);
        size_t estimated_len = input_len * 3 / 4;
        output.clear();
        output.reserve(estimated_len);

        int val = 0;
        int valb = -8;

        for (size_t i = 0; i < input_len; ++i) {
            char c = input[i];
            if (c == '=') break;
            int d = base64_char_value(c);
            if (d < 0) continue;
            val = (val << 6) + d;
            valb += 6;
            if (valb >= 0) {
                output.push_back((val >> valb) & 0xFF);
                valb -= 8;
            }
        }

        return 0;
    }

    /**
     * @brief   get account's on chain datas, and extract cid from the datas
     *
     * @param   publickey    accounts that needs to get data
     * @return  json         obtained data
     *          nullopt      no data has been retrieved
     */
    std::string decodeAndStripPubkeys(const std::string& base64_str, const DecodeType type) {
        std::vector<uint8_t> decoded_bytes;
        if (base64_decode(base64_str.c_str(), decoded_bytes) != 0 || decoded_bytes.size() <= 96) {
            return "";
        }

        size_t cut_length = 96;
        switch (type) {
            case DecodeType::Domain:
                cut_length = 100;
                break;
            case DecodeType::Cid:
                cut_length = 104;
                break;
        }

        if (decoded_bytes.size() <= cut_length) {
            return "";
        }

        return std::string(decoded_bytes.begin() + cut_length, decoded_bytes.end());
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

            return decodeAndStripPubkeys(data_base64_string, DecodeType::Cid);
        }

        return "";
    }
    

    std::pair<std::vector<std::string>, std::vector<Solana_web3::Pubkey>> get_all_root_domain(){
        // method -- getProgramAccounts
        const std::string method = "getProgramAccounts";

        const json get_root_params = build_root_fliters();
        
        std::cout << "fliters:" << get_root_params << std::endl;
        const json request_json = build_request_json(method, get_root_params, 1, true);

        SolanaRpcClient new_client = SolanaRpcClient();
        auto response = new_client.send_rpc_request(request_json);

        std::cout << "response: " << response << std::endl;

        std::vector<std::string> root_domains;
        std::vector<Solana_web3::Pubkey> root_pubkeys;

        std::pair<std::vector<std::string>, std::vector<Solana_web3::Pubkey>> result;

        std::vector<std::string> pubkeys;
        if(!response.has_value()){
            return result;
        }

        const json& parse_json = response.value(); 
        std::vector<std::string> pubkey_lists;

        for (const auto& item : parse_json) {
            if (item.contains("pubkey")) {
                pubkeys.push_back(item["pubkey"].get<std::string>());
            }
        }
        

        json root_pubkey_list = json::array();
        for (const auto& pk : pubkeys) {
            std::cout << "Pubkey: " << pk << std::endl;

            root_pubkeys.push_back(Solana_web3::Pubkey(pk));

            const std::string combined_string = Solana_web3::PREFIX + pk ;
            const std::vector<uint8_t> combined_domain_bytes(combined_string.begin(), combined_string.end());

            std::array<uint8_t, Solana_web3::Pubkey::LENGTH> hash_domain;

            Solana_web3::Solana_web3_interface::sha_256(combined_domain_bytes, hash_domain);

            std::vector<std::vector<uint8_t>> domain_account_seeds;
            domain_account_seeds.push_back(std::vector<uint8_t>(hash_domain.begin(), hash_domain.end()));
            
            domain_account_seeds.push_back(std::vector<uint8_t>(Solana_web3::CENTRAL_STATE_AUCTION.bytes.begin(), Solana_web3::CENTRAL_STATE_AUCTION.bytes.end()));
            domain_account_seeds.push_back(std::vector<uint8_t>(32, 0));

            const Solana_web3::PDA root_reverse_key = Solana_web3::Solana_web3_interface::try_find_program_address_cxx(domain_account_seeds, Solana_web3::WEB3_NAME_SERVICE);
            root_pubkey_list.push_back(root_reverse_key.publickey.toBase58());

            std::cout << "reverse key: " << root_reverse_key.publickey.toBase58() << std::endl;

        }
        
        const std::optional<json> root_pubkeys_response = get_multiple_account_info(root_pubkey_list);

        std::cout << "all root domain's qurey response:" << root_pubkeys_response << std::endl;

        
        if(!root_pubkeys_response.has_value()){
            return result;
        }

        const json& root_response = root_pubkeys_response.value();
        if(root_response.contains("value") && !root_response["value"].is_null()){
            const json& root_domains_datas = root_response["value"];
            
            for(const auto& data: root_domains_datas){
                if(data.contains("data") && !data["data"].is_null()){                  
                    root_domains.push_back(decodeAndStripPubkeys(data["data"][0].get<std::string>(), DecodeType::Domain));
                }else{
                    return result;
                }
            }
        }else{
            return result;
        }

        result.first = root_domains;
        result.second = root_pubkeys;

        return result;
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

        std::cout << "request: " << request << std::endl;

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
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);         // Connection timeout 5 seconds
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);               // Total operation timeout 10 seconds

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

    void SolanaRootMap::set_all_pubkey(const std::vector<Solana_web3::Pubkey>& Pubkeys){
        std::lock_guard<std::mutex> lock(mutex_);
        pubkeys_ = Pubkeys;
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

    std::vector<Solana_web3::Pubkey> SolanaRootMap::get_all_Pubkey() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pubkeys_;
    }

}