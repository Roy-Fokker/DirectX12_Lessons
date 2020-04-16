#pragma once

#include <chrono>

namespace learning_dx12
{
	class game_clock
	{
	public:
		game_clock();
		~game_clock();

		void tick();
		void reset();

		auto get_delta_ns() const -> double;
		auto get_delta_us() const -> double;
		auto get_delta_ms() const -> double;
		auto get_delta_s() const -> double;

		auto get_total_ns() const -> double;
		auto get_total_us() const -> double;
		auto get_total_ms() const -> double;
		auto get_total_s() const -> double;

		private:
		std::chrono::high_resolution_clock::time_point timepoint_prev{};

		std::chrono::high_resolution_clock::duration delta_time{};
		std::chrono::high_resolution_clock::duration total_time{};
	};
}