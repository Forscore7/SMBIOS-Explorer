#include "smbios_parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <Windows.h>
#include <wbemidl.h>
#include "json.hpp" 

#pragma comment(lib, "wbemuuid.lib")

using json = nlohmann::json;

SMBIOSParser::SMBIOSParser() {
    // Initialize 
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        throw std::runtime_error("Failed to initialize COM library.");
    }
}

SMBIOSParser::~SMBIOSParser() {
    CoUninitialize();
}

void SMBIOSParser::loadData() {
    auto smbiosData = fetchSMBIOSData();
    parseSMBIOSData(smbiosData);
}

void SMBIOSParser::parseSMBIOSData(const std::vector<uint8_t>& data) {
    size_t offset = 0;
    while (offset < data.size()) {
        SMBIOSStructure structure;
        const uint8_t* ptr = data.data() + offset;

        structure.type = ptr[0];
        structure.length = ptr[1];
        structure.handle = *(uint16_t*)&ptr[2];


        size_t endOfFormattedArea = offset + structure.length;
        const char* stringsStart = reinterpret_cast<const char*>(data.data() + endOfFormattedArea);

        // Extract strings
        while (*stringsStart != '\0') {
            structure.strings.push_back(stringsStart);
            stringsStart += strlen(stringsStart) + 1;
        }

        offset = endOfFormattedArea + structure.strings.size() + 1; 
        structures.push_back(structure);
    }
}

std::vector<uint8_t> SMBIOSParser::fetchSMBIOSData() {
    // WMI to query SMBIOS data
    IWbemLocator* locator = nullptr;
    IWbemServices* services = nullptr;
    IEnumWbemClassObject* enumerator = nullptr;

    HRESULT hres = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&locator);
    if (FAILED(hres)) {
        handleWMIError("Failed to create IWbemLocator instance.");
    }

    hres = locator->ConnectServer(
        BSTR(L"ROOT\\CIMV2"),
        nullptr, nullptr, nullptr, 0, nullptr, nullptr, &services);
    if (FAILED(hres)) {
        locator->Release();
        handleWMIError("Failed to connect to WMI server.");
    }

    hres = services->ExecQuery(
        BSTR(L"WQL"),
        BSTR(L"SELECT * FROM Win32_BIOS"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr, &enumerator);
    if (FAILED(hres)) {
        services->Release();
        locator->Release();
        handleWMIError("Failed to execute WMI query.");
    }

    std::vector<uint8_t> smbiosData;
    IWbemClassObject* obj = nullptr;
    ULONG returned;

    while (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == WBEM_S_NO_ERROR) {
        VARIANT data;
        hres = obj->Get(L"SMBIOSBIOSVersion", 0, &data, nullptr, nullptr);
        if (SUCCEEDED(hres) && (data.vt == VT_BSTR)) {
            smbiosData.assign(data.bstrVal, data.bstrVal + SysStringLen(data.bstrVal));
        }
        VariantClear(&data);
        obj->Release();
    }

    enumerator->Release();
    services->Release();
    locator->Release();

    return smbiosData;
}

void SMBIOSParser::displayCommands() const {
    std::cout << "Available commands:\n";
    std::cout << "  cmds   - Display all user commands\n";
    std::cout << "  table  - View the SMBIOS structure table\n";
    std::cout << "  all    - View all structures\n";
    std::cout << "  [id]   - View the structure with the given ID\n";
    std::cout << "  quit   - Exit the program\n";
}

void SMBIOSParser::displaySMBIOSTable() const {
    std::cout << "SMBIOS Table:\n";
    for (const auto& structure : structures) {
        std::cout << "Type: " << static_cast<int>(structure.type)
            << ", Length: " << static_cast<int>(structure.length)
            << ", Handle: " << structure.handle << "\n";
    }
}

void SMBIOSParser::displayAllStructures() const {
    for (const auto& structure : structures) {
        std::cout << "Type: " << static_cast<int>(structure.type)
            << ", Handle: " << structure.handle << "\n";
        for (const auto& str : structure.strings) {
            std::cout << "  String: " << str << "\n";
        }
    }
}

void SMBIOSParser::displayStructureByID(uint16_t id) const {
    for (const auto& structure : structures) {
        if (structure.handle == id) {
            std::cout << "Found Structure with ID: " << id << "\n";
            for (const auto& field : structure.fields) {
                std::cout << "  " << field.first << ": " << field.second << "\n";
            }
            return;
        }
    }
    std::cout << "No structure found with ID: " << id << "\n";
}

void SMBIOSParser::saveToJSON(const std::string& filename) const {
    json j;
    for (const auto& structure : structures) {
        json s;
        s["type"] = structure.type;
        s["handle"] = structure.handle;
        s["fields"] = structure.fields;
        j["structures"].push_back(s);
    }

    std::ofstream file(filename);
    file << j.dump(4);
    std::cout << "Data saved to " << filename << "\n";
}

void SMBIOSParser::handleWMIError(const std::string& message) const {
    throw std::runtime_error(message);
}
