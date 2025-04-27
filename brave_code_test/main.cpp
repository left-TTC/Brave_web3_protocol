
#include <iostream>
#include "brave_web3_service.h"

using namespace std;

int main(){ 
    cout << "[1] test start" << endl;

    cout << "[1] test encode" <<endl;

    const string input1 = "abc";

    std::vector<uint8_t> decode = fanmocheng::DecodeBase58(input1);
    std::cout << "Decoded vector:" << std::endl;
    for (size_t i = 0; i < decode.size(); ++i) {
        std::cout << "Byte " << i << ": " << std::hex << (int)decode[i] << std::dec << std::endl;
    }
    string abc = fanmocheng::EncodeBase58(decode);
    cout << "output1 is " << abc << endl;

}