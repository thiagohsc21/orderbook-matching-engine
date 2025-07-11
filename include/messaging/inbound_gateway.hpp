#ifndef INBOUND_GATEWAY_HPP
#define INBOUND_GATEWAY_HPP

#include "messaging/command.hpp"
#include "utils/thread_safe_queue.hpp" 
#include <string>
#include <memory>

class InboundGateway 
{
public:
   
    explicit InboundGateway(ThreadSafeQueue<std::unique_ptr<Command>>& queue, std::string& wal_file_path);

    void processInputFile(const std::string& input_file_path);

    // Garante que o WAL será aberto apenas uma vez, mesmo com múltiplos gateways
    static void ensureWriteAheadLogOpened(const std::string& wal_file_path);
    void writeAheadLog(const std::string& log_message);

    void parseAndCreateCommand(const std::string& line);

private:
    
    ThreadSafeQueue<std::unique_ptr<Command>>& command_queue_;
    std::string& wal_file_path_;
    static std::ofstream wal_file_;
};

#endif // INBOUND_GATEWAY_HPP