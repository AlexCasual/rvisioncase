#pragma once 
#include <core/logger.hpp>
#include <http/server/server.hpp>

namespace rvision
{
	class app final : public Poco::Util::ServerApplication
	{				
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
		rvision::core::errc create_response(const std::string& line, const Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const rvision::http::params& params);

	private:
	rvision::core::errc http_lines_soccer(const Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const rvision::http::params& params);
	rvision::core::errc http_lines_football(const Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const rvision::http::params& params);
	rvision::core::errc http_lines_baseball(const Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const rvision::http::params& params);
	
	private:
		std::shared_ptr<rvision::core::logger> m_logger;
		std::shared_ptr<rvision::http::server> m_http_server;
		std::jthread m_main_thread;

	};
}