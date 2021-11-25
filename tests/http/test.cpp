#include <http/server/server.hpp>

int main( int argc, char* argv[] )
{	
	const std::string pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
	const std::string level("info");

	_logger = spdlog::stdout_color_mt("console");

	_logger->set_level(rvision::core::detail::get_logger_level(level));
	_logger->set_pattern(pattern);

	const std::string _host("http:\\\\localhost:8080");
	_http_srv = std::make_unique<rvision::http::server>(_address, _logger);
	
	m_http_server->handle("GET", "ready", std::bind(&app::http_ready_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));		
	m_http_server->start();
	
	return 0;
}

