#include <iostream>
#include "logging.h"
#include "data.h"
#include "broadcast.h"

static Broadcaster* b = nullptr;

int main() {
    auto lg = maelstrom::core::get_logger();
    lg.log("Starting broadcast server\n");


    auto& node = maelstrom::data::Node::get_instance();
    b = new Broadcaster{node, lg};

    node.registerHandler(std::bind(&Broadcaster::handle_topology, b, std::placeholders::_1), {maelstrom::data::MessageType::TOPOLOGY});
    node.registerHandler(std::bind(&Broadcaster::handle_broadcast, b, std::placeholders::_1), {maelstrom::data::MessageType::BROADCAST});
    node.registerHandler(std::bind(&Broadcaster::handle_read, b, std::placeholders::_1), {maelstrom::data::MessageType::READ});
    
    node.start_and_run(); // Single threaded as of now
    return 0;
}