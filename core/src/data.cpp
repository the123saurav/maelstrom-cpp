#include <iostream>
#include <sstream>
#include "data.h"


namespace maelstrom {
    namespace data {

        
        MsgBody::~MsgBody() {
        }

        Node& Node::get_instance() {
                static Node node;
                return node;
        }

        Node::Node(): state_{State::CREATED}, lg_(maelstrom::core::get_logger()) {
            // Add init handler
            handlers_[MessageType::INIT] = std::bind(&Node::handle_init, this, std::placeholders::_1);
        }

        Node::~Node(){}

        std::string& Node::get_id() {
            return id_;
        }

        void Node::start_and_run() {
            lg_.log("Starting node");

            state_ = State::WAITING_FOR_INIT;

            std::string line;
            while (true) { // TODO add atomic bool
                // read from stdin, newline terminated messages    
                // TODO: buffered IO
                std::getline(std::cin, line);
                lg_.log("Received msg: " + line);
                
                std::shared_ptr<Message> msg = parse_message(line);
                lg_.log("Successfully parsed incoming message");
                auto it = handlers_.find(msg->type_);
                if (it == handlers_.end()) {
                    lg_.log("No handler found");
                    throw std::runtime_error{"Unhandled message type"};
                }
                lg_.log("Calling handler");

                // We want to prevent callers from participating in ownership of message
                // so that we alone would manage the lifetime of message.
                // TODO: this makese our loop blocking on handler, we can make below std::async or TP based.
                std::vector<std::unique_ptr<Message>> responses = (it->second)(msg);

                lg_.log("sending response");
                for(auto& resp: responses) {
                    // Create a reply message
                    std::string resp_str = prepare_response(msg, std::move(resp));
                    // TODO: disable buffering
                    std::cout << resp_str + "\n";
                }
            }
        }

        void Node::registerHandler(const Handler& handler, const std::initializer_list<MessageType>& msg_types) {
            std::lock_guard lk{lock_};
            for(auto const& msg_type: msg_types) {
                handlers_[msg_type] = handler;
            }
        }

        MessageType Node::get_type(const std::string& type) const noexcept {
            if (type == kInitType) {
                return MessageType::INIT;
            } else if (type == kInitOkType) {
                return MessageType::INIT_OK;
            } else if (type == kEchoType) {
                return MessageType::ECHO;
            } else if (type == kTopologyType) {
                return MessageType::TOPOLOGY;
            } else if (type == kBroadcastType) {
                return MessageType::BROADCAST;
            } else if (type == kReadType) {
                return MessageType::READ;
            } else {
                return MessageType::UNKNOWN;
            }
        }

        std::unique_ptr<Init> Node::parse_init(boost::json::object& body_json) const {
            lg_.log("Parsing init");
            
            unsigned int msg_id = body_json[kMsgId].as_int64();
            std::string node_id = body_json[kNodeId].as_string().c_str(); // can we use move?

            boost::json::array node_ids_arr = body_json[kNodeIds].as_array();
            std::vector<std::string> node_ids;
            for (auto& item : node_ids_arr) {
                node_ids.emplace_back(item.as_string().c_str());
            }

            std::stringstream ss;
            ss << "msg_id: " << msg_id << ", node_id: " << node_id;
            lg_.log(ss.str());
            return std::make_unique<Init>(msg_id, std::move(node_id), std::move(node_ids)); // Dont use std::move, compiler will RVO       
        }

        std::unique_ptr<Echo> Node::parse_echo(boost::json::object& body_json) const {
            lg_.log("Parsing echo");
            
            unsigned int msg_id = body_json[kMsgId].as_int64();
            std::string echo = body_json[kEchoField].as_string().c_str(); // can we use move?
        
            std::stringstream ss;
            ss << "msg_id: " << msg_id << ", echo: " << echo;
            lg_.log(ss.str());
            return std::make_unique<Echo>(msg_id, std::move(echo)); // Dont use std::move, compiler will RVO       
        }

        std::unique_ptr<Topology> Node::parse_topology(boost::json::object& body_json) const {
            lg_.log("Parsing topology");
            
            unsigned int msg_id = body_json[kMsgId].as_int64();
            boost::json::object& topology_obj = body_json[kTopologyField].as_object();
            std::unordered_map<std::string, std::vector<std::string>> topology;
            for (const auto& [key, value] : topology_obj) {
                // Check if the value is an array
                if (value.is_array()) {
                    const auto& jsonArray = value.as_array();
                    std::vector<std::string> vec;

                    // Iterate over the array and extract strings
                    for (const auto& item : jsonArray) {
                        vec.push_back(item.as_string().c_str());
                    }

                    // Insert the vector into the map
                    topology.emplace(std::string(key), std::move(vec));
                }
            }
        
            return std::make_unique<Topology>(msg_id, std::move(topology)); // Dont use std::move, compiler will RVO       
        }

        std::unique_ptr<Broadcast> Node::parse_broadcast(boost::json::object& body_json) const {
            lg_.log("Parsing Broadcast");
            std::optional<unsigned int> msg_id;
            if (body_json.contains(kMsgId)) {
                msg_id = body_json[kMsgId].as_int64();
            }
            
            unsigned int msg = body_json[kMessageField].as_int64();
        
            return std::make_unique<Broadcast>(msg_id, msg); // Dont use std::move, compiler will RVO       
        }

        std::unique_ptr<Read> Node::parse_read(boost::json::object& body_json) const {
            lg_.log("Parsing Read");
            unsigned int msg_id = body_json[kMsgId].as_int64();
        
            return std::make_unique<Read>(msg_id); // Dont use std::move, compiler will RVO       
        }

        json_str Node::prepare_response(const std::shared_ptr<Message>& initial_msg, std::unique_ptr<Message> resp) const {
             using namespace boost::json;

            object json_obj;
            json_obj[kSrc] = resp->src_;
            json_obj[kDest] = resp->dest_;

            // Check the actual type of MsgBody
            if (initial_msg->type_ == MessageType::INIT) {
                InitOk* init_ok_body = dynamic_cast<InitOk*>(resp->body_.get());
                if (init_ok_body != nullptr) {
                    object body_obj;
                    body_obj[kType] = kInitOkType;
                    body_obj[kInReplyTo] = init_ok_body->in_reply_to_;

                    json_obj[kBody] = body_obj;
                }
            } else if (initial_msg->type_ == MessageType::ECHO){
                EchoOk* echo_ok_body = dynamic_cast<EchoOk*>(resp->body_.get());
                if (echo_ok_body != nullptr) {
                    object body_obj;
                    body_obj[kType] = kEchoOkType;
                    body_obj[kMsgId] = echo_ok_body->msg_id_;
                    body_obj[kInReplyTo] = echo_ok_body->in_reply_to_;
                    body_obj[kEchoField] = echo_ok_body->echo_;

                    json_obj[kBody] = body_obj;
                }
            } else if (initial_msg->type_ == MessageType::TOPOLOGY){
                TopologyOk* topology_ok_body = dynamic_cast<TopologyOk*>(resp->body_.get());
                if (topology_ok_body != nullptr) {
                    object body_obj;
                    body_obj[kType] = kTopologyOkType;
                    // body_obj[kMsgId] = topology_ok_body->msg_id_;
                    body_obj[kInReplyTo] = topology_ok_body->in_reply_to_;

                    json_obj[kBody] = body_obj;
                }
            } else if (initial_msg->type_ == MessageType::BROADCAST){
                BroadcastOk* broadcast_ok_body = dynamic_cast<BroadcastOk*>(resp->body_.get());
                if (broadcast_ok_body != nullptr) {
                    object body_obj;
                    body_obj[kType] = kBroadcastOkType;
                    // body_obj[kMsgId] = broadcast_ok_body->msg_id_;
                    body_obj[kInReplyTo] = broadcast_ok_body->in_reply_to_.value();

                    json_obj[kBody] = body_obj;
                } else {
                    Broadcast* broadcast_body = dynamic_cast<Broadcast*>(resp->body_.get());
                    if (broadcast_body != nullptr) {
                        object body_obj;
                        body_obj[kType] = kBroadcastType;
                        body_obj[kMessageField] = broadcast_body->message_;
                        // body_obj[kMsgId] = broadcast_ok_body->msg_id_;

                        json_obj[kBody] = body_obj;
                    }
                }
            } else if (initial_msg->type_ == MessageType::READ){
                ReadOk* read_ok_body = dynamic_cast<ReadOk*>(resp->body_.get());
                if (read_ok_body != nullptr) {
                    object body_obj;
                    body_obj[kType] = kReadOkType;
                    // body_obj[kMsgId] = broadcast_ok_body->msg_id_;
                    body_obj[kInReplyTo] = read_ok_body->in_reply_to_;
                    boost::json::array messagesArray;

                    for (const auto& message : read_ok_body->messages_) {
                        messagesArray.push_back(message);
                    }

                    // Assign the array to the Boost.JSON object
                    body_obj[kMessagesField] = std::move(messagesArray);

                    json_obj[kBody] = body_obj;
                }
            } else {
                throw std::runtime_error{"Unhandled response"};
            }

            return serialize(json_obj);
        }

        std::vector<std::unique_ptr<Message>> Node::handle_init(std::shared_ptr<Message> msg){
            if (auto* init = dynamic_cast<Init*>(msg->body_.get())) { // Branch prediction should not be messed up here
                id_ = init->node_id_;
                peers_ = std::move(init->node_ids_);
                lg_.log("Handled init");
                state_ = State::READY;
                std::vector<std::unique_ptr<Message>> response;
                response.emplace_back(std::make_unique<Message>(std::move(msg->dest_), 
                    std::move(msg->src_), MessageType::INIT_OK, std::make_unique<InitOk>(init->msg_id_)));
                return response;    
            } 
            throw std::runtime_error{"Bad body in Init handler"};
        }
    }
}