
#include <iostream>
#include "brave_web3_service.h"
#include "brave_web3_rpc.h"
#include <iomanip> 

#include "json.hpp"

using json = nlohmann::json;


using namespace std;

void test_function1(){
    cout << "This is the frist test" << endl;

    cout << "[1] test start" << endl;

    cout << "[1] test decode" <<endl;

    const std::array<uint8_t, 32> known_pubkey_bytes_arr = {
        0x4d, 0xd9, 0x13, 0x8a, 0xea, 0x98, 0x42, 0xc2,
        0xd4, 0x37, 0xad, 0x12, 0x5a, 0x6f, 0xb2, 0x5a,
        0x7b, 0x1e, 0xd3, 0x63, 0x29, 0xc6, 0x66, 0xe9,
        0x3d, 0x4c, 0xdc, 0x66, 0x4e, 0xc9, 0x54, 0x0a
    };

    std::vector<uint8_t> pubkey_vector(known_pubkey_bytes_arr.begin(), known_pubkey_bytes_arr.end());

    const string test_pubkey = "6EtNpLmx3n133UhLvNSkXyCXkkRCZqNrp37kNmw41urq";
    cout << "output should be " << test_pubkey << endl;

    const Solana_web3::Pubkey test_Pubkey = Solana_web3::Pubkey(test_pubkey); 
    cout << "test pubkey's base58:" << test_Pubkey.toBase58() << endl;
    
    cout << "now dncode" <<endl;
    const vector<uint8_t> pubkey_byte = Solana_web3::DecodeBase58(test_Pubkey.toBase58());
    const string encode = Solana_web3::EncodeBase58(pubkey_vector);
    cout << "result:" << (pubkey_vector == pubkey_byte) << endl;
    cout << "result:" << encode << endl;
}


void print_seeds_json_like(const std::vector<std::vector<uint8_t>>& seeds) {
    std::cout << "[";
    for (size_t i = 0; i < seeds.size(); ++i) {
        std::cout << "[";
        for (size_t j = 0; j < seeds[i].size(); ++j) {
            std::cout << (int)seeds[i][j];
            if (j != seeds[i].size() - 1) std::cout << ", ";
        }
        std::cout << "]";
        if (i != seeds.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
}


void test_function2(){
    cout << "this is the second test -- test curve and sha256" << endl;

    string test_domain = "test";
    const string Pre = "WEB3 Name Service";
    const std::string combined = Pre + test_domain;
    std::vector<uint8_t> bytes(combined.begin(), combined.end());
    std::array<uint8_t, Solana_web3::Pubkey::LENGTH> hash_result;
    Solana_web3::Solana_web3_interface::sha_256(bytes, hash_result);

    std::cout << "[";
    for (size_t i = 0; i < hash_result.size(); ++i) {
        std::cout << static_cast<int>(hash_result[i]);
        if (i != hash_result.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;

    Solana_web3::Pubkey test_program_id = Solana_web3::Pubkey("8YXaA8pzJ4xVPjYY8b5HkxmPWixwpZu7gVcj8EvHxRDC");

    std::vector<std::vector<uint8_t>> seeds;
    for(size_t i = 0; i < hash_result.size(); i += 32) {
        size_t end = std::min(i + 32, hash_result.size());
        seeds.push_back(std::vector<uint8_t>(hash_result.begin() + i, hash_result.begin() + end));
    }

    //[106, 242, 169, 57, 75, 100, 138, 26, 118, 8, 20, 106, 171, 69, 13, 171, 169, 12, 255, 215, 88, 30, 151, 173, 172, 58, 4, 203, 162, 69, 55, 83]
    //[106, 242, 169, 57, 75, 100, 138, 26, 118, 8, 20, 106, 171, 69, 13, 171, 169, 12, 255, 215, 88, 30, 151, 173, 172, 58, 4, 203, 162, 69, 55, 83]
    print_seeds_json_like(seeds);

    auto result = Solana_web3::Solana_web3_interface::try_find_program_address_cxx(seeds, test_program_id);

    Solana_web3::Pubkey pda = result.publickey;
    uint8_t bump = result.bump;
    std::cout << "PDA: " << pda.toBase58() << ", bump: " << static_cast<int>(bump) << std::endl;
    //F6XgnHw3HVffBJLhhLnuu4XGQtKYz2TxYP2oJUCuxBfQ
    //F6XgnHw3HVffBJLhhLnuu4XGQtKYz2TxYP2oJUCuxBfQ
}

void test_function4(){
    //8SCKeL3FDsTLNLEtLTVrnmNaGBTi8meNJHkGk4yBHKkm   aaa
    //8ZdaNzfVYUdFpJH8sCkWy59SxN1UXP5JrBJZwxtWQTRB   QmPu4ZT2zPfyVY8CA2YBzqo9HfAV79nDuuf177tMrQK1py
    Solana_web3::Pubkey test_publickey = Solana_web3::Pubkey("8SCKeL3FDsTLNLEtLTVrnmNaGBTi8meNJHkGk4yBHKkm");

    Solana_web3::Pubkey root_domain_key = Solana_web3::Pubkey("D1Ee8US7XM5jCs978iuAqBTLPvK6969ZNdi3yqDeZnH4");
    const Solana_web3::PDA ipfs_account = Solana_web3::Solana_web3_interface::get_account_from_root("ajja", root_domain_key);

    cout << "ipfs key:" << ipfs_account.publickey.toBase58() << endl;

    string a = ipfs_account.publickey.get_pubkey_ipfs();
    cout << "cid:" << a << endl;
}

#include <fstream>

void test_function6(){
    ofstream outfile("ipfs_account_generate_test.txt");
    for(uint32_t i = 0; i < 100000; i++ ){
        string str_i = to_string(i);

        Solana_web3::Pubkey root_domain_key = Solana_web3::Pubkey("D1Ee8US7XM5jCs978iuAqBTLPvK6969ZNdi3yqDeZnH4");
        Solana_web3::PDA result = Solana_web3::Solana_web3_interface::get_account_from_root(str_i, root_domain_key);

        outfile << "Pubkey:" << " " << result.publickey.toBase58() << " " << "bump:" << " " << static_cast<int>(result.bump) << "\n";
    }

    outfile.close();
    std::cout << "输出完成" << std::endl;
}

size_t count_different_lines(const std::string& file1, const std::string& file2) {
    std::ifstream f1(file1, std::ios::binary);
    std::ifstream f2(file2, std::ios::binary);
    size_t diff_count = 0;

    if (!f1.is_open() || !f2.is_open()) {
        std::cerr << "无法打开文件" << std::endl;
        return static_cast<size_t>(-1); // 返回最大值表示错误
    }

    std::string line1, line2;
    while (std::getline(f1, line1) && std::getline(f2, line2)) {
        // 统一处理换行符（移除Windows的\r）
        if (!line1.empty() && line1.back() == '\r') line1.pop_back();
        if (!line2.empty() && line2.back() == '\r') line2.pop_back();
        
        if (line1 != line2) {
            ++diff_count;
        }
    }

    // 检查是否有剩余行（文件长度不同）
    while (std::getline(f1, line1)) ++diff_count;
    while (std::getline(f2, line2)) ++diff_count;

    return diff_count;
}

void test_function8(){
    std::pair<std::vector<std::string>, std::vector<Solana_web3::Pubkey>> a = Solana_Rpc::get_all_root_domain();
}

#include "brave_web3_task.h"

void test_function9(){
    Brave_Web3_Solana_task::update_root_domains();

    const std::vector<std::string> roots = Brave_Web3_Solana_task::get_root_domains();

    for(const auto& root: roots){
        std::cout << "root:" << root << std::endl;
    }
}

void test_function10(){
    const Brave_Web3_Solana_task::test_class a = Brave_Web3_Solana_task::test_class("ajja", "web3");

    Brave_Web3_Solana_task::replace_domain_tocid(a);
}


int main(){ 
    // const string file1 = "./ipfs_account_generate_test.txt";
    // const string file2 = "../test_txt/ipfs_account_generate_test_js.txt";

    // size_t i = count_different_lines(file1, file2);
    // cout << "line:" << i << endl;

    test_function4();


    return 0;
}

