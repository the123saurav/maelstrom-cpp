#ifndef MAELSTROM_CPP_BROADCAST_H
#define MAELSTROM_CPP_BROADCAST_H

#include <unordered_set>
#include "logging.h"
#include "data.h"

using namespace maelstrom;

// Not thread safe
class Broadcaster {
private:
    data::Node& node_;
    core::logger& lg_;
    std::vector<std::string> peers_;
    std::unordered_set<unsigned int> messages_;

public:
    Broadcaster(data::Node& node, core::logger& lg);

    std::vector<std::unique_ptr<data::Message>> handle_topology(std::shared_ptr<data::Message>& msg);
    std::vector<std::unique_ptr<data::Message>> handle_broadcast(std::shared_ptr<data::Message>& msg);
    std::vector<std::unique_ptr<data::Message>> handle_read(std::shared_ptr<data::Message>& msg);
};


#endif