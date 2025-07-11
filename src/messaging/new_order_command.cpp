#include "messaging/new_order_command.hpp"

NewOrderCommand::NewOrderCommand(uint64_t client_order_id, uint64_t client_id, const std::string& symbol, 
                                 OrderSide side, OrderType type, uint32_t quantity, double price)
    : client_order_id_(client_order_id), 
      client_id_(client_id), 
      symbol_(symbol),
      side_(side), 
      type_(type), 
      quantity_(quantity), 
      price_(price) 
{
}

void NewOrderCommand::execute(MatchingEngine& engine) 
{
    // Implementation of the execute method, which will interact with the MatchingEngine
    // to process the new order command.
    // This is where the logic for adding a new order to the order book would go.
}