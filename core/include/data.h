#ifndef MAELSTROM_CORE_MESSAGE_H
#define MAELSTROM_CORE_MESSAGE_H

#include <string>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <boost/json.hpp>
#include "logging.h"


namespace maelstrom {
    namespace data {

        enum class MessageType {
            INIT, INIT_OK,
            ECHO, ECHO_OK,
            UNKNOWN
        };

        struct MsgBody {
            virtual ~MsgBody() = 0;
        };

        struct Init: public MsgBody {
            unsigned int msg_id_;
            std::string node_id_;
            std::vector<std::string> node_ids_;

            Init(unsigned int msg_id, std::string node_id, std::vector<std::string> node_ids): msg_id_{msg_id}, node_id_{node_id}, node_ids_{node_ids} {}
        };

        struct InitOk: public MsgBody {
            unsigned int in_reply_to_;

            InitOk(unsigned int msg_id): in_reply_to_{msg_id} {}
        };


        // template<typename T>
        struct Message {
            std::string src_;
            std::string dest_;
            MessageType type_;

            std::shared_ptr<MsgBody> body_; 

            Message(std::string src, std::string dest, MessageType type): 
                src_{std::move(src)}, dest_{std::move(dest)}, type_{type}, body_{nullptr} {}
        };

        /*
            We want handler to support stateless like lambda as well as stateful
            handlers.
        */
        using Handler = std::function<std::unique_ptr<MsgBody>(std::shared_ptr<Message>)>;

        class Node {
        public:    
            // Meyer's singleton, safe to return by ref as lifetime is static
            static Node& get_instance();
            
            // Creates or replaces a handler for message;
            void registerHandler(const Handler& handler, std::initializer_list<MessageType> msg_types);

            // Should be called in a new thread as this starts the node engine which is a big loop.
            // We do not want to start a thread inside this and let user decide on execution runtime for it.
            void start_and_run();

        private:    
            enum class State {
                CREATED,
                WAITING_FOR_INIT,
                READY    
            };

            std::mutex lock_;
            std::unordered_map<MessageType, Handler> handlers_;
            State state_;
            maelstrom::core::logger& lg_;
            std::string id_;
            std::vector<std::string> peers_;
                 
            Node();

            Node(const Node& other) = delete;  
            Node& operator=(const Node& other) = delete;

            Node(Node&& other) = delete;  
            Node& operator=(Node&& other) = delete;

            ~Node();

            MessageType get_type(const std::string& type) const noexcept;

            std::shared_ptr<Init> populate_init(boost::json::object& body_json) const;

            std::shared_ptr<InitOk> populate_init_ok(boost::json::object& body_json) const;

            std::string prepare_response(const std::shared_ptr<Message> initial_msg, std::unique_ptr<MsgBody> resp) const;

            std::shared_ptr<Message> parse_message(const std::string& json_str) {
                boost::json::value jv = boost::json::parse(json_str);
                boost::json::object& obj = jv.as_object();
                std::string src = obj["src"].as_string().c_str();
                std::string dest = obj["dest"].as_string().c_str();

                boost::json::object& bodyObj = obj["body"].as_object();
                std::string type = bodyObj["type"].as_string().c_str();

                lg_.log("received type: " + type);
                
                std::shared_ptr<Message> msg = nullptr;

                switch (get_type(type))
                {
                case MessageType::INIT:
                    msg = std::make_shared<Message>(std::move(src), std::move(dest), MessageType::INIT);
                    msg->body_ = populate_init(bodyObj);
                    break;
                case MessageType::INIT_OK:
                    msg = std::make_shared<Message>(std::move(src), std::move(dest), MessageType::INIT_OK);
                    msg->body_ = populate_init_ok(bodyObj);
                    break;    
                default:
                    // TODO
                    break;
                }
                return msg;
            }

            std::unique_ptr<InitOk> handle_init(std::shared_ptr<Message> msg);
        };
    }
}

#endif