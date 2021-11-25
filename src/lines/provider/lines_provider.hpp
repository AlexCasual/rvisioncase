#pragma once
#include <lines/db/lines_db.hpp>
#include <lines/poller/lines_poller.hpp>

namespace rvision
{
	enum class lines_provider_state
	{
		not_inited = 0,
		ready = 1
	};

	class lines_provider
	{
		struct limes_delta_cache_item
		{
			bool inited = false;
			std::double_t last_recv = 0.0;
			std::double_t last_send = 0.0;
		};

	public:
		lines_provider(const std::unordered_map<std::string, std::uint32_t>& sports, const std::string& host, const std::string& api, const std::string& db, std::shared_ptr<rvision::core::logger> logger);
		~lines_provider();

		lines_provider_state state();

		void add(const std::string& sport, std::chrono::seconds& period);
		void rem(const std::string& sport);

		std::unordered_map<std::string, std::double_t> fetch_delta(const std::vector<std::string>& lines, bool changed);

		std::unordered_map<std::string, std::vector<std::double_t>> fetch(const std::vector<std::string>& sports);
		std::vector<std::double_t> fetch(const std::string& sport);
		std::unordered_map<std::string, std::double_t> fetch_last(const std::vector<std::string>& sports);
		std::double_t fetch_last(const std::string& sport);

	private:
		void update_cache(const std::string& sport, std::double_t score);

	private:
		std::shared_ptr<rvision::core::logger> m_logger;
		rvision::lines_db m_db;
		std::string m_host;
		std::string m_api;
		std::string m_address;
		std::mutex m_pollers_lock;
		std::map<std::string, lines_poller> m_pollers;
		std::shared_mutex m_lines_cache_lock;
		std::unordered_map<std::string, limes_delta_cache_item> m_lines_cache;
	};
}