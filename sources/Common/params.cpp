#include "params.h"

std::unique_ptr<Params> Params::instance;
std::once_flag Params::flag;

Params& Params::GetInstance()
{
	std::call_once(flag, [] {instance.reset(new Params); });
	return *instance;
}