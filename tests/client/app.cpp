#include "app.hpp"

namespace rvision
{
	namespace detail
	{
		static const std::string _logger_name("client_rvision");
		static const std::string _logger_file("client_rvision.log");
		static const std::string _rpc_server_host("localhost:10100");
		static const std::string _http_server_host("http://localhost:10101/ready");
	}

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
		waitForTerminationRequest();
		
		m_logger->info("app::exit => receive stop request.");
		
		m_main_thread.request_stop();
		
		m_logger->info("app::exit => stopping main thread...");
		
		m_main_thread.join();
		
		m_logger->info("app::exit => main thread has been stopped.");
	}
	
	void app::main()
	{			
		Poco::URI uri(detail::_http_server_host);
		
		m_logger->info("app::main => http address: {}, host: {}, port: {}", detail::_http_server_host, uri.getHost(), uri.getPort());
		
		Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
		session.setKeepAlive(true);
		session.setKeepAliveTimeout(Poco::Timespan(60, 0));
		session.setTimeout(Poco::Timespan(60, 0));
		
		bool _http_ready = false;
		while(!_http_ready)
		{
			try
			{
				for(auto i = 0; i != 3; ++i)
				{
					Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, uri.getPath(), Poco::Net::HTTPMessage::HTTP_1_1);

					session.sendRequest(request);
				
					Poco::Net::HTTPResponse responce(Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND);		
				
					session.receiveResponse(responce);

					m_logger->debug("lines_poller::poll => responce status: {}", responce.getStatus());
			
					std::this_thread::sleep_for(std::chrono::seconds(1));

					if(responce.getStatus() == Poco::Net::HTTPResponse::HTTPStatus::HTTP_OK)
					{
						_http_ready = true;
						continue;
					}
			
					std::this_thread::sleep_for(std::chrono::seconds(1));
				}
			}
			catch (const std::exception& e)
			{
				m_logger->info("app::main => exception : {}", e.what());
			}
		}
		
		m_logger->info("app::main => rpc_srv_addrr : {}", detail::_rpc_server_host);
		
		m_rpc_client = std::make_shared<rvision::rpc::client>(detail::_rpc_server_host, m_logger, [&](const std::vector<std::pair<std::double_t,std::string>>& lines)
		{
				m_logger->info("app::lines =>");
				for (const auto& l : lines)
				{
					m_logger->info("sport : {} / score {}", l.second, l.first);
				}
				m_logger->info("app::lines <=");
		});
		
		call_rpc(10, 5, {"football"});
		
		call_rpc(10, 7, {"baseball"});
		
		call_rpc(15, 10, {"baseball", "football"});
		
		call_rpc(20, 15, {"baseball", "football", "soccer"});
		
		call_rpc(30, 25, {"baseball"});
		
		std::this_thread::sleep_for(std::chrono::seconds(60));
	}
	
	void app::call_rpc(std::uint64_t _slp, std::uint64_t _prd, const std::vector<std::string>& _sprts)
	{		
		std::string _log;
		for(const auto& s : _sprts)
		{
			_log += s;
			_log += " ";
		}
		
		m_logger->info("app::call_rpc => call for {} with sleep: {}s, period: {}s", _log, _slp, _prd);	

		m_rpc_client->call(_prd, _sprts);
		
		m_logger->info("app::call_rpc => sleep...");	
		
		std::this_thread::sleep_for(std::chrono::seconds(_slp));
		
		m_logger->info("app::call_rpc => wakeup!");	
	}
}