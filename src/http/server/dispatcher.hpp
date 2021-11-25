#pragma once

#include <core/headers.hpp>
#include <http/utils/executor.hpp>
#include <http/utils/params.hpp>

namespace rvision::http
{
	class dispatcher
	{
	public:
		explicit dispatcher() = default;
		~dispatcher() = default;

		void add(const std::string& method, const std::string& uri, const rvision::http::executer& exec);
		rvision::core::errc execute(const Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) const;

	private:
		std::unordered_map<std::string, std::vector<std::pair<std::vector<std::string>, executer>>> m_base_executer;
		std::unordered_map<std::string, std::vector<std::pair<std::vector<std::string>, executer>>> m_executers;
	};
}