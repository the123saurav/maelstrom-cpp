#include <iostream>
#include "logging.h"
#include "data.h"


std::unique_ptr<maelstrom::data::EchoOk> handle_echo(std::shared_ptr<maelstrom::data::Message> msg){
    if (auto* echo = dynamic_cast<maelstrom::data::Echo*>(msg->body_.get())) { // Branch prediction should not be messed up here
        return std::make_unique<maelstrom::data::EchoOk>(echo->msg_id_, echo->msg_id_, echo->echo_);
    } 
    throw std::runtime_error{"Bad body in Init handler"};
}

int main() {
    auto lg = maelstrom::core::get_logger();
    lg.log("Hello\n");

    auto& node = maelstrom::data::Node::get_instance();
    node.registerHandler(&handle_echo, {maelstrom::data::MessageType::ECHO});
    node.start_and_run();
    return 0;
}