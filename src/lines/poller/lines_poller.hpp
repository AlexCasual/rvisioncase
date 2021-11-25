#pragma once
#include <core/headers.hpp>
#include <core/logger.hpp>

namespace rvision
{
	using poll_callback = std::function<void(const std::string& sport, std::double_t score)>;
	
	class lines_poller
	{
		using time_point_t = std::chrono::steady_clock::time_point;

	public:
		lines_poller(std::shared_ptr<rvision::core::logger> logger, const std::string& address, const std::string& sport, const std::chrono::seconds& period, const poll_callback& cb);
		~lines_poller();

	private:
		void poll();

	private:
		std::jthread m_thread;
		std::mutex m_lock;
		std::condition_variable m_wait;
		poll_callback m_poll_callback;
		std::string m_sport;
		std::string m_address;
		std::chrono::seconds m_period;
		std::shared_ptr<rvision::core::logger> m_logger;
	};
}