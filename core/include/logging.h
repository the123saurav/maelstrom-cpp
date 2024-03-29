#ifndef MAELSTROM_CPP_LOGGING_H
#define MAELSTROM_CPP_LOGGING_H

#include <iostream>
#include <atomic>

// #define LOG(msg) std::cerr << (msg); 

namespace maelstrom {
    namespace core {

        // TODO: Make Thread safe logger
        struct logger {
            friend logger& get_logger();

            void log(const std::string& msg) const noexcept {
                #ifdef DEBUG
                    std::cerr << msg + "\n";
                #endif
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