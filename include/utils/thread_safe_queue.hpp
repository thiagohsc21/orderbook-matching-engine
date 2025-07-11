#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <queue>
#include <mutex>
#include <memory> // Para std::unique_ptr
#include <iostream> 

// Template para que a fila possa guardar qualquer tipo de dado. No nosso caso, será std::unique_ptr<Command> ou std::unique_ptr<Event>
template<typename T>
class ThreadSafeQueue 
{
public:

    void push(T item) 
    {
        // std::lock_guard trava o mutex quando é criado. Quando 'lock' sai de escopo no final da função, o mutex é liberado automaticamente.
        std::lock_guard<std::mutex> lock(mutex_);

        // Usamos std::move para transferir a posse do unique_ptr
        queue_.push(std::move(item)); 
    }

    bool try_pop(T& value) 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) 
        {
            std::cout << "Queue is empty, cannot pop item." << '\n';
            return false;
        }
        
        value = std::move(queue_.front());
        queue_.pop();

        return true;
    }

    bool empty() const 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    std::queue<T> queue_;

    // Mutex precisa ser 'mutable' para que possamos modifica-lo em métodos 'const' como empty().
    mutable std::mutex mutex_; 
};

#endif // THREAD_SAFE_QUEUE_HPP