#include "server.hpp"
#include "handler.hpp"
#include "factory.hpp"

#include <Poco/Net/ServerSocketImpl.h>

namespace rvision::http
{
	namespace detail
	{
		std::unique_ptr<Poco::Net::ServerSocket> create_socket(const std::string & address)
		{
			const Poco::Net::SocketAddress socket_address(address);

			auto _socket = std::make_unique<Poco::Net::ServerSocket>();

			_socket->init(socket_address.af());
			_socket->setReuseAddress(true);
			_socket->setReusePort(false);
			_socket->bind(socket_address, false);
				
			_socket->listen();

			return _socket;

		}
	}

	server::server(const std::string& address, std::shared_ptr<rvision::core::logger> logger)
	: m_logger(logger), m_address(address)
	{
		
	}
	
	server::~server()
	{
		if(m_server)
		{
			m_server->stopAll();
		}
	}
	
	void server::handle(const std::string& method, const std::string& uri, rvision::http::executer exec)
	{
		m_dispatcher.add(method, uri, exec);
	}
	
		
	void server::start()
	{
		Poco::Net::HTTPServerParams::Ptr parameters = new Poco::Net::HTTPServerParams();

		parameters->setMaxQueued(32);
		parameters->setMaxThreads(2*std::thread::hardware_concurrency());		
		parameters->setKeepAlive(true);
		parameters->setKeepAliveTimeout(Poco::Timespan(5, 0));

		m_socket = detail::create_socket(m_address);
		
		m_server = std::make_unique<Poco::Net::HTTPServer>(new rvision::http::factory(m_dispatcher), *m_socket, parameters);
		
		m_server->start();
	}
}