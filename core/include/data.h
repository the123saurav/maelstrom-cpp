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

        const std::string kInitType = "init";
        const std::string kInitOkType = "init_ok";

        const std::string kEchoType = "echo";
        const std::string kEchoOkType = "echo_ok";

        const std::string kSrc = "src";
        const std::string kDest = "dest";
        const std::string kType = "type";
        const std::string kBody = "body";
        const std::string kMsgId = "msg_id";
        const std::string kInReplyTo = "in_reply_to";
        const std::string kNodeId = "node_id";
        const std::string kNodeIds = "node_ids";
        const std::string kEchoField = "echo";

        using json_str = std::string;

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

            Init(unsigned int msg_id, std::string node_id, std::vector<std::string> node_ids): 
                msg_id_{msg_id}, node_id_{node_id}, node_ids_{std::move(node_ids)} {}
        };

        struct InitOk: public MsgBody {
            unsigned int in_reply_to_;

            InitOk(unsigned int msg_id): in_reply_to_{msg_id} {}
        };

        struct Echo: public MsgBody {
            unsigned int msg_id_;
            std::string echo_;

            Echo(unsigned int msg_id, std::string echo):
                msg_id_{msg_id}, echo_{std::move(echo)} {}
        };

        struct EchoOk: public MsgBody {
            unsigned int msg_id_;
            unsigned int in_reply_to_;
            std::string echo_;

            EchoOk(unsigned int msg_id, unsigned int in_reply_to, std::string echo):
                msg_id_{msg_id}, in_reply_to_{in_reply_to}, echo_{std::move(echo)} {}
        };


        // TODO: think about templatizing it to need to cast and vtable pointer mem cost
        // template<typename T>
        struct Message {
            std::string src_;
            std::string dest_;
            MessageType type_;

            std::unique_ptr<MsgBody> body_; 

            Message(std::string src, std::string dest, MessageType type): 
                src_{std::move(src)}, dest_{std::move(dest)}, type_{type}, body_{nullptr} {}
        };

        /*
            We want handler to support stateless like lambda as well as stateful
            handlers.
        */
        using Handler = std::function<std::unique_ptr<MsgBody>(std::shared_ptr<Message>&)>;

        class Node {
        public:    
            // Meyer's singleton, safe to return by ref as lifetime is static
            static Node& get_instance();
            
            // Creates or replaces a handler for message;
            void registerHandler(const Handler& handler, const std::initializer_list<MessageType>& msg_types);

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

            std::atomic_uint msg_id = 1; // TODO: its assumed to be local to node
                 
            Node();

            Node(const Node& other) = delete;  
            Node& operator=(const Node& other) = delete;

            Node(Node&& other) = delete;  
            Node& operator=(Node&& other) = delete;

            ~Node();

            MessageType get_type(const std::string& type) const noexcept;

            std::unique_ptr<Init> parse_init(boost::json::object& body_json) const;

            std::unique_ptr<InitOk> parse_init_ok(boost::json::object& body_json) const;

            std::unique_ptr<Echo> parse_echo(boost::json::object& body_json) const;

            // std::unique_ptr<EchoOk> parse_echo_ok(boost::json::object& body_json) const;

            // Passing by ref is okay for shared_ptr as we are not increasing lifetime here.
            json_str prepare_response(const std::shared_ptr<Message>& initial_msg, std::unique_ptr<MsgBody> resp) const;

            std::shared_ptr<Message> parse_message(const std::string& json_str) {
                boost::json::value jv = boost::json::parse(json_str);
                boost::json::object& obj = jv.as_object(); // Treat it as object
                std::string src{obj[kSrc].as_string().c_str()};
                std::string dest{obj[kDest].as_string().c_str()};

                boost::json::object& bodyObj = obj[kBody].as_object();
                std::string type{bodyObj[kType].as_string().c_str()};
                lg_.log("received type: " + type);
                
                std::shared_ptr<Message> msg = nullptr;
                switch (get_type(type))
                {
                case MessageType::INIT:
                    msg = std::make_shared<Message>(std::move(src), std::move(dest), MessageType::INIT);
                    msg->body_ = parse_init(bodyObj);
                    break;
                // case MessageType::INIT_OK:
                //     msg = std::make_shared<Message>(std::move(src), std::move(dest), MessageType::INIT_OK);
                //     msg->body_ = parse_init_ok(bodyObj);
                //     break;
                case MessageType::ECHO:
                    msg = std::make_shared<Message>(std::move(src), std::move(dest), MessageType::ECHO);
                    msg->body_ = parse_echo(bodyObj);
                    break;
                // case MessageType::ECHO_OK:
                //     msg = std::make_shared<Message>(std::move(src), std::move(dest), MessageType::ECHO_OK);
                //     msg->body_ = parse_init_ok(bodyObj);
                //     break;        
                default:
                    // TODO
                    throw std::runtime_error{"Unexpected message"};
                }
                return msg;
            }

            std::unique_ptr<InitOk> handle_init(std::shared_ptr<Message> msg);
        };
    }
}

#endif