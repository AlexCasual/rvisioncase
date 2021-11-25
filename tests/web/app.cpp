#include "app.hpp"

namespace detail
{
	static const std::string _logger_name("web_rvision");
	static const std::string _logger_file("web_rvision.log");
	static const std::string _http_host("localhost:9000");
	static const std::string _http_api("api/v1/lines");
	
	static const std::vector<std::string> _lines = {"SOCCER", "FOOTBALL", "BASEBALL"};
	
	std::shared_ptr<rvision::http::server> create_http_server(const std::string& addrr, std::shared_ptr<rvision::core::logger> logger)
	{
		return std::make_shared<rvision::http::server>(addrr, logger);
	}
}

namespace rvision
{
	app::app()
	{
	}
	
	app::~app()
	{
	}
	
	void app::startup()
	{				
		std::string _logger_path(config().getString("application.dataDir", ""));
		_logger_path += "\\";
		_logger_path += detail::_logger_file;
		
		auto _logger_level = "debug";
		auto _logger_sink = "console";
				
		m_logger = rvision::core::create_logger(_logger_level, _logger_sink, detail::_logger_name, _logger_path);
	}

	int app::main(const std::vector<std::string>& /*args*/)
	{
		try
		{	
			startup();
		
			m_main_thread = std::jthread([&] (std::stop_token stoken)
			{
				main();
				
				std::mutex mutex;
				std::unique_lock lock(mutex);
					
				std::condition_variable_any().wait(lock, stoken, [&stoken] { return false; });
					
				if(stoken.stop_requested()) 
				{
					m_logger->info("app::main => exit from main thread by request.");
					return;
				}
			});	
			
			exit();
		}
		catch (const Poco::Exception& ex)
		{
			m_logger->error("app::main => exception [%s]", ex.displayText().c_str());
		}
		catch (const std::exception& ex)
		{
			m_logger->error("app::main => exception! %s", ex.what());
		}
		catch (...)
		{
			m_logger->error("app::main => exception!");
		}

		m_logger->info("app::main => exit.");

		return Application::EXIT_OK;
	}
	
	void app::initialize(Poco::Util::Application& self)
	{
		loadConfiguration();
		ServerApplication::initialize(self);
	}
	
	void app::uninitialize()
	{
		ServerApplication::uninitialize();
	}
	
	void app::defineOptions(Poco::Util::OptionSet& options)
	{
		ServerApplication::defineOptions(options);
	}
	
	void app::handleOption(const std::string& name, const std::string& value)
	{		
		ServerApplication::handleOption(name, value);
	}
	
	void app::exit()
	{
		//todo : add termination logic;
		
		waitForTerminationRequest();
		
		m_logger->info("app::exit => receive stop request.");
		
		m_main_thread.request_stop();
		
		m_logger->info("app::exit => stopping main thread...");
		
		m_main_thread.join();
		
		m_logger->info("app::exit => main thread has been stopped.");
	}
	
	void app::main()
	{		
		m_logger->info("app::main => http_host : {}",detail::_http_host);
		m_logger->info("app::main => http_api : {}", detail::_http_api);
		m_logger->info("app::main => http_lines : ");
		for(const auto& s : detail::_lines)
		{
			m_logger->info("{}", s);
		}

		m_http_server = detail::create_http_server(detail::_http_host, m_logger);
		
		{
			std::string _api(detail::_http_api);
			_api += "/soccer";
			
			m_logger->info("app::get => _api: {}", _api);

			m_http_server->handle("GET", _api , std::bind(&app::http_lines_soccer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		}
		{
			std::string _api(detail::_http_api);
			_api += "/football";
			
			m_logger->info("app::get => _api: {}", _api);

			m_http_server->handle("GET", _api , std::bind(&app::http_lines_football, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		}
		{
			std::string _api(detail::_http_api);
			_api += "/baseball";
			
			m_logger->info("app::get => _api: {}", _api);

			m_http_server->handle("GET", _api , std::bind(&app::http_lines_baseball, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		}
		
		m_http_server->start();
	}
	
	rvision::core::errc app::http_lines_soccer(const Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const rvision::http::params& params)
	{		
		return create_response("SOCCER", request, response, params);
	}
	
	rvision::core::errc app::http_lines_football(const Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const rvision::http::params& params)
	{
		return create_response("FOOTBALL", request, response, params);
	}
	
	rvision::core::errc app::http_lines_baseball(const Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const rvision::http::params& params)
	{
		return create_response("BASEBALL", request, response, params);
	}

	rvision::core::errc app::create_response(const std::string& line, const Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const rvision::http::params& params)
	{
		std::string _lines("lines.");
		_lines += line;
				
		std::srand(std::time(nullptr));
		std::double_t _score = (std::double_t)std::rand();

		m_logger->info("app::create_response => line: {}, score: {}", _lines, _score);
			
		boost::property_tree::ptree _ptree;		
		_ptree.add(_lines, _score);
		
		response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
		response.setContentType("application/json");

		std::ostream& stream = response.send();
		boost::property_tree::write_json(stream, _ptree);
		
		return rvision::core::errc::success;
	}
}