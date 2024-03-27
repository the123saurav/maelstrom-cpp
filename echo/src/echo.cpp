#include <iostream>
#include "logging.h"
#include "data.h"

int main() {
    auto lg = maelstrom::core::get_logger();
    lg.log("Hello\n");

    auto& node = maelstrom::data::Node::get_instance();
    node.start_and_run();
    return 0;
}