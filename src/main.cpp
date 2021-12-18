#include <cstdlib>
#include <fstream>
#include <glog/logging.h>
#include "test.hpp"
#include <fstream>

using ::std::unique_ptr;
using namespace std;

int main(int argc, char** argv) {
    (void)argc;
    ::google::InitGoogleLogging(argv[0]);

    unique_ptr<WindowManager> window_manager(WindowManager::Create());
    if (!window_manager) {
        LOG(ERROR) << "Failed to initialize window manager.";
        return EXIT_FAILURE;
    }

    window_manager->Run();

    return EXIT_SUCCESS;
}
