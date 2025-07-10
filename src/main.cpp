#include <iostream>
#include <chrono>
#include <iomanip>

#include "domain/order.hpp"
#include <vector>
#include <random>

int main() 
{

	std::vector<Order> orders;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint64_t> client_id_dist(10000, 99999);
	std::uniform_int_distribution<uint64_t> client_order_id_dist(100000, 999999);
	std::vector<std::string> symbols = {"AAPL", "GOOG", "MSFT", "AMZN", "TSLA"};
	std::uniform_int_distribution<int> symbol_dist(0, symbols.size() - 1);
	std::uniform_int_distribution<int> side_dist(0, 1);
	std::uniform_int_distribution<int> type_dist(0, 1);
	std::uniform_int_distribution<int> tif_dist(0, 2);
	std::uniform_int_distribution<int> capacity_dist(0, 1);
	std::uniform_real_distribution<double> price_dist(100.0, 200.0);
	std::uniform_int_distribution<uint32_t> quantity_dist(10, 1000);

	for (int i = 0; i < 20; ++i) {
		uint64_t client_id = client_id_dist(gen);
		uint64_t client_order_id = client_order_id_dist(gen);
		std::string symbol = symbols[symbol_dist(gen)];
		OrderSide side = static_cast<OrderSide>(side_dist(gen));
		OrderType type = static_cast<OrderType>(type_dist(gen));
		OrderTimeInForce tif = static_cast<OrderTimeInForce>(tif_dist(gen));
		OrderCapacity capacity = static_cast<OrderCapacity>(capacity_dist(gen));
		double price = price_dist(gen);
		uint32_t quantity = quantity_dist(gen);
		orders.emplace_back(client_id, client_order_id, symbol, price, quantity, 
		                    side, type, tif, capacity);
		std::cout << "------ Order created ------\n";
		auto ts = orders.back().getReceivedTimestamp();
		auto t = std::chrono::system_clock::to_time_t(ts);
		auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(ts.time_since_epoch()) % 1000000000;

		std::cout << "Order ID: " << orders.back().getOrderId() << "\n"
				  << "Client ID: " << orders.back().getClientId() << "\n"
				  << "Client Order ID: " << orders.back().getClientOrderId() << "\n"
				  << "Symbol: " << orders.back().getSymbol() << "\n"
				  << "Side: " << (orders.back().getSide() == OrderSide::Buy ? "Buy" : "Sell") << "\n"
				  << "Type: " << static_cast<int>(orders.back().getType()) << "\n"
				  << "Time In Force: " << static_cast<int>(orders.back().getTimeInForce()) << "\n"
				  << "Capacity: " << static_cast<int>(orders.back().getCapacity()) << "\n"
				  << std::fixed << std::setprecision(2)
				  << "Price: $" << orders.back().getPrice() << "\n"
				  << "Quantity: " << orders.back().getQuantity() << "\n"
				  << "Filled Quantity: " << orders.back().getFilledQuantity() << "\n"
				  << "Remaining Quantity: " << orders.back().getRemainingQuantity() << "\n"
				  << "Received Timestamp: " << std::put_time(std::localtime(&t), "%F %T")
				  << "." << std::setfill('0') << std::setw(9) << ns.count()
				  << "\n\n";
	}
	
    return 0;
}
