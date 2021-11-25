#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <core/headers.hpp>

namespace rvision::core
{
	namespace detail
	{
		static spdlog::level::level_enum get_logger_level(const std::string& level)
		{
			static const std::map<std::string, spdlog::level::level_enum> _spdlog_levels =
			{
				{"info", spdlog::level::level_enum::info},
				{"debug", spdlog::level::level_enum::debug},
				{"error", spdlog::level::level_enum::err}
			};

			if (const auto& p = _spdlog_levels.find(level); p != _spdlog_levels.end())
			{
				return p->second;
			}

			return spdlog::level::level_enum::info;
		};
	}

	using logger = spdlog::logger;

	static std::shared_ptr<logger> create_logger(const std::string& level, const std::string& sink, const std::string& name, const std::string& path)
	{
		const std::string pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");

		if (sink == "console")
		{
			auto _impl = spdlog::default_logger();

			_impl->set_level(detail::get_logger_level(level));
			_impl->set_pattern(pattern);

			return _impl;
		}

		if (sink == "file")
		{
			auto _impl = spdlog::daily_logger_mt(name, path, 23, 59);

			_impl->flush_on(detail::get_logger_level(level));
			_impl->set_level(detail::get_logger_level(level));
			_impl->set_pattern(pattern);			

			return _impl;
		}

		return nullptr;
	}
}