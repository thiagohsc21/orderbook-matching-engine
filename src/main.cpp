#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include "utils/thread_safe_queue.hpp" 
#include "messaging/inbound_gateway.hpp"
#include <iomanip>
#include "domain/order.hpp"
#include <vector>
#include <random>

void producer(ThreadSafeQueue<std::unique_ptr<int>>& queue) {
    for (int i = 1; i <= 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::unique_ptr item = std::make_unique<int>(i);
        queue.push(std::move(item));
        std::cout << "[Producer] Inseriu " << i << std::endl;
    }
}

void consumer(ThreadSafeQueue<std::unique_ptr<int>>& queue) {
    for (int i = 0; i < 5; ++i) {
        std::unique_ptr<int> item;
        while (!queue.try_pop(item)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "[Consumer] Pegou " << *item << std::endl;
    }
}

int main() {

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

	for (int i = 0; i < 10; ++i) {
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

    // Thread Safe Queue test
    ThreadSafeQueue<std::unique_ptr<int>> queue;

    std::thread t1(producer, std::ref(queue));
    std::thread t2(consumer, std::ref(queue));

    t1.join();
    t2.join();

    // Inbound Gateway test
    std::string wal_file_path = "write_ahead_log.txt";
    ThreadSafeQueue<std::unique_ptr<Command>> command_queue;
    InboundGateway inbound_gateway(command_queue, wal_file_path);
    inbound_gateway.writeAheadLog("Test log message");
    inbound_gateway.parseAndCreateCommand("tag1=value1|tag2=value2|tag3=value3|tag4=value4");


    return 0;
}