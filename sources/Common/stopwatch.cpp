#include "stopwatch.h"

std::unique_ptr<StopWatch> StopWatch::instance;
std::once_flag StopWatch::flag;

StopWatch& StopWatch::GetInstance()
{
	std::call_once(flag, [] {instance.reset(new StopWatch); });
	return *instance;
}