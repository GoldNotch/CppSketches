// Copyright 2024 JohnCorn
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     https://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <string>
#include <queue>
#include <iostream>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <array>
#include <functional>
#include <optional>


#include <catch2/catch_test_macros.hpp>


template<typename T, size_t capacity>
class LockFreeQueue
{
    struct Node
    {
        std::atomic<Node*> next = nullptr;
        Node() = default;

        template<typename... Args>
        Node(Args &&... args)
            : data(std::in_place_t(), std::forward<Args>(args)...)
        {}

        T && Extract() { return std::move(*data); }
    private:
        std::optional<T> data = std::nullopt;
    };

public:
    LockFreeQueue()
    { head = tail = new Node(); }

    /// @brief push value to back
    void push(const T& val)
    {
        Node* new_node = new Node(val);
        while(true)
        {
            Node* _tail = tail.load();
            Node* _tail_next = _tail->next.load();
            if (_tail->next.compare_exchange_weak(_tail_next, new_node))
            {
                tail.compare_exchange_weak(_tail, new_node);
                return;
            }
            else
                tail.compare_exchange_weak(_tail, _tail_next);
        }
    }

    /// @brief pop value from front
    T pop()
    {        
        Node* _head = head.load();
        // wait while queue is not empty
        while(!_head->next)
        {
            std::this_thread::yield();
            _head = head.load();
        }
        while (!head.compare_exchange_weak(_head, _head->next))
            std::this_thread::yield();
        T result = _head->Extract();
        return result;
    }

    bool is_empty() const {return head == tail;}
private:
    std::atomic<Node*> head;
    std::atomic<Node*> tail;
};

template<size_t capacity, size_t pushers_count, size_t popers_count>
size_t TestQueue()
{
    LockFreeQueue<int, capacity> q;
    const size_t max = (pushers_count+popers_count);
    std::array<std::thread, max> pool;
    auto pusher = [&q, msg_cnt = popers_count](int idx){
        for(size_t i = 0; i < msg_cnt; ++i)
            q.push(idx);
    };

    auto poper = [&q, msg_cnt = 1](int idx){
        for(size_t i = 0; i < msg_cnt; ++i)
            q.pop();
            //std::cout << q.pop() << std::endl;
    };
    for(size_t i = 0; i < pushers_count; ++i)
        pool[i] = std::thread(pusher, i);
    for(size_t i = 0; i < popers_count; ++i)
        pool[i] = std::thread(poper, pushers_count + i);  
    
    for(auto && th : pool)
        th.join();
    
    REQUIRE(q.is_empty());
    return q.is_empty();
};



TEST_CASE("LockFreeQueue", "[LockFreeQueue]")
{
    TestQueue<10, 1, 5>();
}