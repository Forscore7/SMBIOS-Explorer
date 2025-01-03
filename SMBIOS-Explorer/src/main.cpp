#include "smbios_parser.hpp"
#include <iostream>

int main() {
    try {
        SMBIOSParser parser;
        parser.loadData();

        std::string command;
        while (true) {
            std::cout << "> ";
            std::cin >> command;

            if (command == "cmds") {
                parser.displayCommands();
            }
            else if (command == "table") {
                parser.displaySMBIOSTable();
            }
            else if (command == "all") {
                parser.displayAllStructures();
            }
            else if (command == "quit") {
                break;
            }
            else {
                try {
                    uint16_t id = std::stoi(command);
                    parser.displayStructureByID(id);
                }
                catch (...) {
                    std::cout << "Invalid command or ID.\n";
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}
