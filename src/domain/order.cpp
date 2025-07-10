#include "types/order_params.hpp"
#include "domain/order.hpp"
#include <chrono>
#include <string>

uint64_t Order::next_order_id_ = 1; // Initialize static member

Order::Order(uint64_t client_id, uint64_t client_order_id,
             const std::string& symbol, double price, uint32_t quantity, 
             OrderSide side, OrderType type,
             OrderTimeInForce time_in_force, OrderCapacity capacity)
    : order_id_(next_order_id_++), // Assign and increment the static order ID
      client_id_(client_id),
      client_order_id_(client_order_id),
      symbol_(symbol),
      price_(price),
      quantity_(quantity),
      filled_quantity_(0), // Initially, no quantity is filled
      remaining_quantity_(quantity), // Remaining quantity is the total quantity at creation
      side_(side),
      type_(type),
      status_(OrderStatus::New), // New order status at creation
      time_in_force_(time_in_force),
      capacity_(capacity),
      received_timestamp_(std::chrono::system_clock::now()) // Set the current time as received timestamp
{}

