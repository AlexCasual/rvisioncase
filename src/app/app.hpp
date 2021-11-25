#pragma once 
#include <core/logger.hpp>
#include <http/server/server.hpp>
#include <lines/provider/lines_provider.hpp>
#include <rpc/server/server.hpp>

namespace rvision
{
	class app final : public Poco::Util::ServerApplication
	{		
		using app_config = struct
		{
			std::string _rpc_srv_addrr;
			std::string _http_srv_addrr;
			std::string _lines_srv_host;
			std::string _lines_srv_api;
			std::string _data_folder;
			
			std::unordered_map<std::string, std::uint32_t> _lines_pollers;
		};
		
	public:
		explicit app();
		~app();

		app(const app&) = delete;
		app& operator = (const app&) = delete;

	private:
		void startup();
		void main();
		void exit();
		int main(const std::vector<std::string>& args) override final;
		void initialize(Poco::Util::Application& self) override;
		void uninitialize() override;
		void defineOptions(Poco::Util::OptionSet& options) override;
		void handleOption(const std::string& name, const std::string& value) override;
				
	private:
	std::unordered_map<std::string, std::double_t> rpc_get_score(const std::vector<std::string>& lines, bool changed);
	rvision::core::errc http_ready_callback(const Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const rvision::http::params& params);

	private:
		app_config m_config;
		std::shared_ptr<rvision::core::logger> m_logger;
		std::shared_ptr<rvision::http::server> m_http_server;
		std::shared_ptr<rvision::rpc::server> m_rpc_server;
		std::shared_ptr<rvision::lines_provider> m_lines_provider;
		std::jthread m_main_thread;
		std::mutex m_lock;
	};
}