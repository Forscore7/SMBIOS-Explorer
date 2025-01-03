#ifndef SMBIOS_PARSER_HPP
#define SMBIOS_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>

struct SMBIOSStructure {
    uint8_t type;
    uint8_t length;
    uint16_t handle;
    std::vector<std::string> strings;
    std::map<std::string, std::string> fields;
};

class SMBIOSParser {
public:
    SMBIOSParser();
    ~SMBIOSParser();

    void loadData();
    void displayCommands() const;
    void displaySMBIOSTable() const;
    void displayAllStructures() const;
    void displayStructureByID(uint16_t id) const;
    void saveToJSON(const std::string& filename) const;

private:
    std::vector<SMBIOSStructure> structures;

    void parseSMBIOSData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> fetchSMBIOSData();
    void handleWMIError(const std::string& message) const;
};

#endif
