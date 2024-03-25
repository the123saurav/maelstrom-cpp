#ifndef MAELSTROM_CPP_LOGGING_H
#define MAELSTROM_CPP_LOGGING_H

#include <iostream>
#include <atomic>

// #define LOG(msg) std::cerr << (msg); 

namespace maelstrom {
    namespace core {

        // Thread safe logger
        struct logger {
            friend logger& get_logger();

            inline void log(const std::string& msg) noexcept {
                std::cerr << msg + "\n";
            }
        private:    
            logger();
        };

        /*
            Configures stderr based logging for app.
        */
        logger& get_logger();

    } // core

} // maelstrom

#endif