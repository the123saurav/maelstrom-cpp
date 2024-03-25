#include <iostream>
#include "logging.h"

int main() {
    auto lg =maelstrom::core::get_logger();
    lg.log("Hello\n");
    return 0;
}