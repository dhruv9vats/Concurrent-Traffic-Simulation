#include "TrafficLight.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive() {
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics.
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> lck(_mutex);

    _cond.wait(lck, [this]() {
        return !_queue.empty();
    });

    T msg = std::move(_queue.back());
    _queue.clear();

    return msg;  // Will be optimized using RVO (Return Value Optimization)
}

template <typename T>
void MessageQueue<T>::send(T &&msg) {
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lck(_mutex);

    _queue.push_back(std::move(msg));
    _cond.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight() {
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen() {
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue.
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (_msg_queue.receive() == TrafficLightPhase::green) return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase() {
    return _currentPhase;
}

void TrafficLight::simulate() {
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    TrafficObject::threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread

double getRandomWaitTime() {
    /*
    Return a random number between and including 4000-6000 ms
    http://www.cplusplus.com/reference/random/uniform_int_distribution/
    */
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(4, 6);
    return distribution(generator) * 1000;
}

void TrafficLight::cycleThroughPhases() {
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green and sends an update method
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    auto previous_time = std::chrono::system_clock::now();
    double duration = getRandomWaitTime();
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - previous_time).count();

        if (diff > duration) {
            previous_time = std::chrono::system_clock::now();
            duration = getRandomWaitTime();
            _currentPhase = _currentPhase == TrafficLightPhase::red ? TrafficLightPhase::green : TrafficLightPhase::red;

            _msg_queue.send(std::move(_currentPhase));
        }
    }
}