#include "broadcast.h"


Broadcaster::Broadcaster(data::Node& node, core::logger& lg) : node_{node}, lg_{lg} {}

std::vector<std::unique_ptr<data::Message>> Broadcaster::handle_topology(std::shared_ptr<data::Message>& msg) {
    lg_.log("Handling topology message");
    
    if (auto* topology = dynamic_cast<maelstrom::data::Topology*>(msg->body_.get())) { // Branch prediction should not be messed up here
        auto it = topology->topology_.find(node_.get_id());
        if (it == topology->topology_.end()) {
            throw new std::runtime_error{"No self node id in topology message"};
        }
        peers_ = std::move(it->second);
        lg_.log("Logging peers: ");
        for(const auto& peer: peers_) {
            lg_.log(peer + ", ");
        }
        std::vector<std::unique_ptr<data::Message>> response;
        response.emplace_back(std::make_unique<maelstrom::data::Message>(std::move(msg->dest_), 
                    std::move(msg->src_), maelstrom::data::MessageType::TOPOLOGY_OK, 
                        std::make_unique<maelstrom::data::TopologyOk>(topology->msg_id_)));
        return response;
    }
    throw std::runtime_error{"Bad message in topology handler"};
}

std::vector<std::unique_ptr<data::Message>> Broadcaster::handle_broadcast(std::shared_ptr<data::Message>& msg) {
    lg_.log("Handling broadcast message from node: " + msg->src_);
    if (auto* broadcast = dynamic_cast<maelstrom::data::Broadcast*>(msg->body_.get())) { // Branch prediction should not be messed up here
        lg_.log("Received broadcast for message: " + std::to_string(broadcast->message_));
        std::vector<std::unique_ptr<data::Message>> response;
        if (broadcast->msg_id_.has_value()) {
            response.emplace_back(std::make_unique<maelstrom::data::Message>(std::move(msg->dest_), 
                    std::move(msg->src_), maelstrom::data::MessageType::BROADCAST_OK, 
                        std::make_unique<maelstrom::data::BroadcastOk>(broadcast->msg_id_.value())));
        }
        
        // Short circuit the chain.
        if (messages_.find(broadcast->message_) != messages_.end()) {
            lg_.log("Ignoring duplicate broadcast");
        } else {
            messages_.insert(broadcast->message_);
            for(const auto& peer: peers_) {
                if (peer != msg->src_) {
                    response.emplace_back(std::make_unique<maelstrom::data::Message>(node_.get_id(), 
                        peer, maelstrom::data::MessageType::BROADCAST, 
                            std::make_unique<maelstrom::data::Broadcast>(std::optional<unsigned int>{}, broadcast->message_)));
                }
            }
        }
        return response;
    }
    throw std::runtime_error{"Bad message in broadcast handler"};
}

std::vector<std::unique_ptr<data::Message>> Broadcaster::handle_read(std::shared_ptr<data::Message>& msg) {
    lg_.log("Handling read message");
    if (auto* read = dynamic_cast<maelstrom::data::Read*>(msg->body_.get())) { // Branch prediction should not be messed up here
        lg_.log("Received read");
        std::vector<std::unique_ptr<data::Message>> response;
        response.emplace_back(std::make_unique<maelstrom::data::Message>(std::move(msg->dest_), 
                    std::move(msg->src_), maelstrom::data::MessageType::READ_OK, 
                        std::make_unique<maelstrom::data::ReadOk>(read->msg_id_, std::vector(messages_.begin(), messages_.end()))));
        return response;
    }
    throw std::runtime_error{"Bad message in read handler"};
}
