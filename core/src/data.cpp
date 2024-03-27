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

        void Node::start_and_run() {
            std::string line;
            while (true) { // TODO add atomic bool
                // read from stdin line terminated messages    
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

                std::unique_ptr<MsgBody> resp_body = (it->second)(msg);

                lg_.log("sending response");
                // Create a reply message
                std::string resp_str = prepare_response(msg, std::move(resp_body));
                // TODO: disable buffering
                std::cout << resp_str + "\n";
            }
        }

        void Node::registerHandler(const Handler& handler, std::initializer_list<MessageType> msg_types) {
            std::lock_guard lk{lock_};
            for(auto& msg_type: msg_types) {
                handlers_[msg_type] = handler;
            }
        }

        MessageType Node::get_type(const std::string& type) const noexcept {
            if (type == "init") {
                return MessageType::INIT;
            } else if (type == "init_ok") {
                return MessageType::INIT_OK;
            } else {
                return MessageType::UNKNOWN;
            }
        }

        std::shared_ptr<Init> Node::populate_init(boost::json::object& body_json) const {
            lg_.log("Pupulating init in parse");
            unsigned int msg_id = body_json["msg_id"].as_int64();
            std::string node_id = body_json["node_id"].as_string().c_str(); // can we use move?

            boost::json::array node_ids_arr = body_json["node_ids"].as_array();
            std::vector<std::string> node_ids;
            for (auto& item : node_ids_arr) {
                node_ids.push_back(item.as_string().c_str());
            }

            std::stringstream ss;
            ss << "msg_id: " << msg_id << ", node_id: " << node_id;
            lg_.log(ss.str());
            return std::make_shared<Init>(msg_id, std::move(node_id), std::move(node_ids));        
        }

        std::shared_ptr<InitOk> Node::populate_init_ok(boost::json::object& body_json) const {
            return std::make_shared<InitOk>(body_json["in_reply_to"].as_int64());
        }

        std::string Node::prepare_response(const std::shared_ptr<Message> initial_msg, std::unique_ptr<MsgBody> resp) const {
             using namespace boost::json;

            object json_obj;
            json_obj["src"] = initial_msg->dest_;
            json_obj["dest"] = initial_msg->src_;

            // Check the actual type of MsgBody
            if (typeid(*resp) == typeid(InitOk)) {
                InitOk* init_ok_body = dynamic_cast<InitOk*>(resp.get());
                if (init_ok_body != nullptr) {
                    object body_obj;
                    body_obj["type"] = "init_ok";
                    body_obj["in_reply_to"] = init_ok_body->in_reply_to_;

                    json_obj["body"] = body_obj;
                }
            } else {
                throw std::runtime_error{"Unhandled response"};
            }

            return serialize(json_obj);
        }

        std::unique_ptr<InitOk> Node::handle_init(std::shared_ptr<Message> msg){
            auto init = std::dynamic_pointer_cast<Init>(msg->body_);
            id_ = init->node_id_;
            peers_ = std::move(init->node_ids_);
            lg_.log("Handled init");
            state_ = State::READY;
            return std::make_unique<InitOk>(init->msg_id_);
        }
    }
}