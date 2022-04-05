#include "bankID.h"

int main() {
    bankID::init();
    bankID::auth("202203072380");
    bankID::collect(" ");
    std::cout << bankID::credentials;
    return 0;
}

