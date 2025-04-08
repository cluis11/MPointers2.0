#include "Server.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 7) {
        std::cerr << "Uso: " << argv[0] << " --port LISTEN_PORT --memsize SIZE_MB --dumpFolder DUMP_FOLDER" << std::endl;
        return 1;
    }

    std::string listenPort;
    std::size_t memSize = 0;
    std::string dumpFolder;
    std::string folder;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            listenPort = argv[++i];
        }
        else if (arg == "--memsize" && i + 1 < argc) {
            memSize = std::stoul(argv[++i]);
        }
        else if (arg == "--dumpFolder" && i + 1 < argc) {
            dumpFolder = argv[++i];
        }
        else {
            std::cerr << "Argumento desconocido o valor faltante: " << arg << std::endl;
            return 1;
        }
    }

    if (listenPort.empty() || memSize == 0 || dumpFolder.empty()) {
        std::cerr << "Faltan parámetros requeridos." << std::endl;
        return 1;
    }

    folder = "." + dumpFolder;
    MemoryServer::Run(listenPort, memSize, folder);
    return 0;
}