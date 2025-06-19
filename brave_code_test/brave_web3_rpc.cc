

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <optional>
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

    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
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


    //------------class solanaRpcClient-------------
    std::optional<json> SolanaRpcClient::send_rpc_request(
        const std::string& method,
        const json& params,
        int request_id
    )const {
        CURL* curl = curl_easy_init();

        if (!curl) {
            std::cerr << "Failed to initialize CURL." << std::endl;
            return std::nullopt;
        }

        std::string response_buffer;

        json request_json = {
            {"jsonrpc", "2.0"},
            {"id", request_id},
            {"method", method},
            {"params", params}
        };
        std::string request_body = request_json.dump(); 

        curl_easy_setopt(curl, CURLOPT_URL, rpc_url_.c_str());         // Set the RPC URL
        curl_easy_setopt(curl, CURLOPT_POST, 1L);                       // Set as POST request
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str()); // Set POST data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request_body.length()); // Set POST data length

        curl_easy_setopt(curl, CURLOPT_URL, rpc_url_.c_str());         // Set the RPC URL
        curl_easy_setopt(curl, CURLOPT_POST, 1L);                       // Set as POST request
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str()); // Set POST data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request_body.length()); // Set POST data length

        // Set HTTP headers: Content-Type for JSON
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set callback function to write response data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);

        // Optional: Set timeouts
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // Connection timeout 5 seconds
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);      // Total operation timeout 10 seconds

        // Execute the request
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            curl_slist_free_all(headers); // Clean up headers list
            curl_easy_cleanup(curl);      // Clean up CURL handle
            return std::nullopt;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code); // Get HTTP status code

        curl_slist_free_all(headers); // Clean up headers list
        curl_easy_cleanup(curl);      // Clean up CURL handle

        // 3. Process the HTTP response
        if (http_code == 200) {
            try {
                json response = json::parse(response_buffer); // Parse JSON response

                if (response.count("result")) {
                    return response["result"]; // Return the 'result' part
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

}