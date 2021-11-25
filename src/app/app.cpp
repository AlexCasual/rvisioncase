#include "app.hpp"

namespace detail
{
	static const std::string _logger_name("rvision");

	static const std::string _logger_file("rvision.log");
	static const std::string _rpc_server_property("grpc_server");
	static const std::string _http_server_property("http_server");
	static const std::string _lines_server_host_property("lines_server.host");
	static const std::string _lines_server_api_property("lines_server.api");
	static const std::string _lines_count_property("lines_count");
	static const std::string _lines_property("lines");
	static const std::string _lines_name_property("sport");
	static const std::string _lines_poll_property("poll");
	static const std::string _logger_level_property("logger_level");
	static const std::string _logger_sink_property("logger_sink");
	static const std::string _rpc_client_mock_property("rpc_client_mock");
	
	using line_sport_property = struct
	{
		std::string _name_property;
		std::string _poll_property;
	};
		
	line_sport_property create_line_sport_property(std::int32_t count)
	{
		std::string lines_property(_lines_property);
		lines_property += "[";
		lines_property += std::to_string(count);
		lines_property += "]";
		
		line_sport_property _lines_sport_property;
		
		_lines_sport_property._name_property = lines_property;
		_lines_sport_property._poll_property = lines_property;
		
		_lines_sport_property._name_property += ".";
		_lines_sport_property._name_property += _lines_name_property;
		
		_lines_sport_property._poll_property += ".";
		_lines_sport_property._poll_property += _lines_poll_property;
		
		return _lines_sport_property;
	}
			
	std::shared_ptr<rvision::http::server> create_http_server(const std::string& addrr, std::shared_ptr<rvision::core::logger> logger)
	{
		return std::make_shared<rvision::http::server>(addrr, logger);
	}
		
	std::shared_ptr<rvision::rpc::server> create_grpc_server(const std::string& addrr, std::shared_ptr<rvision::core::logger> logger, rvision::rpc::rpc_score_callback db_cb)
	{
		return std::make_shared<rvision::rpc::server>(addrr, 1, logger, db_cb);
	}
	
	std::shared_ptr<rvision::lines_provider> create_lines_provider(const std::unordered_map<std::string, std::uint32_t>& pollers, const std::string& host, const std::string& api,
		const std::string& folder, std::shared_ptr<rvision::core::logger> logger)
	{
		return std::make_shared<rvision::lines_provider>(pollers, host, api, folder, logger);
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
		m_config._rpc_srv_addrr = config().getString(detail::_rpc_server_property, "localhost:50502");
		m_config._http_srv_addrr = config().getString(detail::_http_server_property, "localhost:10052"),

		m_config._lines_srv_host = config().getString(detail::_lines_server_host_property, "http://localhost:8000");
		m_config._lines_srv_api = config().getString(detail::_lines_server_api_property, "api/v1/lines");
			
		m_config._data_folder = config().getString("application.dataDir", "");
		
		auto lines_count = config().getInt(detail::_lines_count_property, 0);
		while(lines_count > 0)
		{
			--lines_count;

			auto line_property = detail::create_line_sport_property(lines_count);
			
			auto sport = config().getString(line_property._name_property, "");
			auto poll = config().getInt(line_property._poll_property, 1);
			
			m_config._lines_pollers[sport] = poll;
		}
		
		std::string _logger_path(m_config._data_folder);
		_logger_path += "\\";
		_logger_path += detail::_logger_file;
		
		auto _logger_level = config().getString(detail::_logger_level_property, "debug");
		auto _logger_sink = config().getString(detail::_logger_sink_property, "console");
		
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
		
		//Poco::ThreadPool::defaultPool().addCapacity(std::thread::hardware_conccurency);

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
		m_logger->info("app::main => app_folder : {}", m_config._data_folder);
		m_logger->info("app::main => lines_srv_addrr : {}", m_config._lines_srv_host);
		m_logger->info("app::main => http_srv_addrr : {}", m_config._http_srv_addrr);
		m_logger->info("app::main => rpc_srv_addrr : {}", m_config._rpc_srv_addrr);

		if (!std::filesystem::exists(m_config._data_folder))
		{
			auto _res = std::filesystem::create_directories(m_config._data_folder);
		}

		m_lines_provider = detail::create_lines_provider(m_config._lines_pollers, m_config._lines_srv_host,  m_config._lines_srv_api,  m_config._data_folder, m_logger);

		m_http_server = detail::create_http_server(m_config._http_srv_addrr, m_logger);
						
		m_http_server->handle("GET", "ready", std::bind(&app::http_ready_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));		
		m_http_server->start();

		//m_rpc_server = detail::create_grpc_server(m_config._rpc_srv_addrr, m_logger, std::bind(&app::rpc_get_score, this, std::placeholders::_1, std::placeholders::_2));
		//m_rpc_server->start();

		m_logger->info("app::main => ok");
	}
	
	std::unordered_map<std::string, std::double_t> app::rpc_get_score(const std::vector<std::string>& lines, bool changed)
	{
		return m_lines_provider->fetch_delta(lines, changed);		
	}
	
	rvision::core::errc app::http_ready_callback(const Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const rvision::http::params& params)
	{
		m_logger->info("app::http_ready_callback => lines state {}", m_lines_provider->state());

		if(m_lines_provider->state() == rvision::lines_provider_state::ready)
		{
			{
				std::unique_lock<std::mutex> _lock(m_lock);
				
				if(!m_rpc_server)
				{
					m_rpc_server = detail::create_grpc_server(m_config._rpc_srv_addrr, m_logger, std::bind(&app::rpc_get_score, this, std::placeholders::_1, std::placeholders::_2));
					m_rpc_server->start();
				}
				else
				{
					m_logger->info("app::http_ready_callback => rpc server already exist!");
				}
			}
			
			response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_OK);
			response.send();

			return rvision::core::errc::success;
		}
		
		response.setStatus((Poco::Net::HTTPResponse::HTTPStatus)425);
		response.send();
		
		return rvision::core::errc::not_implement;
	}
}