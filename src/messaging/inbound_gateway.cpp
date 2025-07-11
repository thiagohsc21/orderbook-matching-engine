#include "messaging/inbound_gateway.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

// Static member initialization 
std::ofstream InboundGateway::wal_file_;

InboundGateway::InboundGateway(ThreadSafeQueue<std::unique_ptr<Command>>& queue, std::string& wal_file_path)
    : command_queue_(queue), 
      wal_file_path_(wal_file_path) 
{
    ensureWriteAheadLogOpened(wal_file_path);
}

void InboundGateway::processInputFile(const std::string& input_file_path) 
{
    // Implement the logic to read from the input file and process each line
    // This will likely involve reading the file line by line and calling parseAndCreateCommand for each line
}

void InboundGateway::ensureWriteAheadLogOpened(const std::string& wal_file_path) 
{
    // Open the write-ahead log file if it is not already opened
    if (!wal_file_.is_open()) 
    {
        wal_file_.open(wal_file_path, std::ios::app);
        if (!wal_file_.is_open()) 
        {
            std::cerr << "Failed to open WAL file: " << wal_file_path << '\n';
        }
    }
}

void InboundGateway::writeAheadLog(const std::string& log_message) 
{
    if (wal_file_.is_open()) 
    {
        wal_file_ << log_message << '\n';
    } 
    else 
    {
        std::cerr << "Failed to open WAL file: " << wal_file_path_ << "\"'\n'";
    }
}

void InboundGateway::parseAndCreateCommand(const std::string& line) 
{
    writeAheadLog(line);

    std::stringstream oss(line);
    std::string tag;
    std::map<std::string, std::string> fix_fields;
    
    while (std::getline(oss, tag, '|')) 
    {
        std::size_t pos = tag.find('=');
        if (pos != std::string::npos) 
        {
            std::string key = tag.substr(0, pos);
            std::string value = tag.substr(pos + 1);
            fix_fields[key] = value;
        }
    }

    std::map<std::string, std::string>::iterator it = fix_fields.begin();
    while (it != fix_fields.end()) 
    {
        std::cout << it->first << " = " << it->second << '\n';
        ++it;
    }

}