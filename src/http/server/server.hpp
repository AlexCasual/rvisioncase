#pragma once
#include <http/server/dispatcher.hpp>
#include <core/logger.hpp>

namespace rvision::http
{
	class server final
	{
	public:
		server(const std::string& addrr, std::shared_ptr<rvision::core::logger> logger);
		~server();

		server(const server&) = delete;
		server& operator = (const server&) = delete;

		void handle(const std::string& method, const std::string& uri, rvision::http::executer exec);

		void start();

	private:
		std::string m_address;
		std::shared_ptr<rvision::core::logger> m_logger;
		rvision::http::dispatcher m_dispatcher;
		std::unique_ptr<Poco::Net::ServerSocket> m_socket;
		std::unique_ptr<Poco::Net::HTTPServer> m_server;

	};
}