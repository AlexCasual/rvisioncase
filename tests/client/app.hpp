#pragma once 
#include <core/logger.hpp>
#include <rpc/client/client.hpp>

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

	private:
	void call_rpc(std::uint64_t _slp, std::uint64_t _prd, const std::vector<std::string>& _sprts);
	
	private:
		std::shared_ptr<rvision::core::logger> m_logger;
		std::shared_ptr<rvision::rpc::client> m_rpc_client;
		std::jthread m_main_thread;

	};
}