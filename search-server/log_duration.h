#pragma once

#include <chrono>
#include <iostream>
#include <string>
using namespace std;

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profile_guard, __LINE__)
#define LOG_DURATION_STREAM(x, stream) LogDuration UNIQUE_VAR_NAME_PROFILE(x, stream)

class LogDuration {
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = chrono::steady_clock;
    
    LogDuration() : stream_(cerr) {
        
    }
    
    LogDuration(const string& operation, ostream& stream = cerr) : operation_(operation), stream_(stream) {
    }

    ~LogDuration();

private:
    const Clock::time_point start_time_ = Clock::now();
    const string operation_;
    ostream& stream_;
};

LogDuration:: ~LogDuration() {
    using namespace chrono;
    using namespace literals;

    const auto end_time = Clock::now();
    const auto dur = end_time - start_time_;
    stream_ << operation_ << ": "s << duration_cast<milliseconds>(dur).count() << " ms"s << endl;
}

