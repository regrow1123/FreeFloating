#pragma once

#include <memory>
#include <mutex>
#include <chrono>

class StopWatch
{
public:
	static StopWatch& GetInstance();

	void Start() {
		prev = std::chrono::system_clock::now();
	}

	std::chrono::nanoseconds Hit() {
		std::chrono::system_clock::time_point time =
			std::chrono::system_clock::now();
		std::chrono::nanoseconds duration = time - prev;
		prev = time;
		return duration;
	}

private:
	static std::unique_ptr<StopWatch> instance;
	static std::once_flag flag;

	std::chrono::system_clock::time_point prev;
};
