#include "logging.h"

namespace maelstrom {
    namespace core {

        logger& get_logger() {
            static logger lg{}; // thread safe, onle 1 mem allocated and initialized
            return lg;
        }

        logger::logger() {
            static char buffer[1024];  // Need to be static for duration of program as std err can be used elsewhere too
            std::cerr.rdbuf()->pubsetbuf(buffer, sizeof(buffer));
        }

    } // namespace core
} // namespace maelstrom 

