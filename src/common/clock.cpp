#include "clock.h"

#include <ratio>

using namespace learning_dx12;

using hrc = std::chrono::high_resolution_clock;
using namespace std::chrono;

game_clock::game_clock()
{
	timepoint_prev = hrc::now();
}

game_clock::~game_clock() = default;

void game_clock::tick()
{
	auto timepoint_now = hrc::now();
	delta_time = timepoint_now - timepoint_prev;
	total_time += delta_time;
	timepoint_prev = timepoint_now;
}

void game_clock::reset()
{
	timepoint_prev = hrc::now();
	delta_time = hrc::duration{};
	total_time = hrc::duration{};
}

auto game_clock::get_delta_ns() const -> double
{
	using ns = duration<double, std::nano>;
	return duration_cast<ns>(delta_time).count();
}

auto game_clock::get_delta_us() const -> double
{
	using us = duration<double, std::micro>;
	return duration_cast<us>(delta_time).count();
}

auto game_clock::get_delta_ms() const -> double
{
	using ms = duration<double, std::milli>;
	return duration_cast<ms>(delta_time).count();
}

auto game_clock::get_delta_s() const -> double
{
	using s = duration<double, std::ratio<1>>;
	return duration_cast<s>(delta_time).count();
}

auto game_clock::get_total_ns() const -> double
{
	using ns = duration<double, std::nano>;
	return duration_cast<ns>(total_time).count();
}

auto game_clock::get_total_us() const -> double
{
	using us = duration<double, std::micro>;
	return duration_cast<us>(total_time).count();
}

auto game_clock::get_total_ms() const -> double
{
	using ms = duration<double, std::milli>;
	return duration_cast<ms>(total_time).count();
}

auto game_clock::get_total_s() const -> double
{
	using s = duration<double, std::ratio<1>>;
	return duration_cast<s>(total_time).count();
}
