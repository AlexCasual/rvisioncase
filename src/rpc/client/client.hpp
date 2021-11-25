#include <core/headers.hpp>
#include <core/logger.hpp>
#include <grpc++/grpc++.h>

#include "rvision.grpc.pb.h"

using rvision_rpc::SportsLinesRequest;
using rvision_rpc::SportsLinesResponse;
using rvision_rpc::SportLinesService;

namespace rvision::rpc
{
	using client_response_callback = std::function<void(const std::vector<std::pair<std::double_t,std::string>>&)>;
	
	class client final
	{
		public:
		client(const std::string& addrr, std::shared_ptr<rvision::core::logger> logger, const client_response_callback& cb);
	   ~client();
	   
	    client(const client&)            = delete;
		client& operator=(const client&) = delete;
		client(client&&)                 = delete;
		client& operator=(client&&)      = delete;

		rvision::core::errc call(std::uint64_t period, const std::vector<std::string>& sports);
		
	    void cancel();

		private:
		void process();
		bool make_request(SportsLinesRequest& req);
		bool on_start(bool ok);
		bool on_read(bool ok);
		bool on_write(bool ok);
		bool on_complete(bool ok);
		bool on_done(bool ok);
		bool on_finish(bool ok);
		
		private:
		std::string 								m_addrr;
		std::thread									m_processor;
		std::mutex 									m_calls_lock;
		std::queue<std::pair<std::uint64_t, std::vector<std::string>>> m_calls;
		std::atomic_bool                            m_started;
		std::atomic_bool                            m_outstand;
		std::atomic_bool                            m_shutdown;
		std::condition_variable 					m_cv;
		std::mutex 									m_cv_lock;
		client_response_callback					m_cb;
		grpc::CompletionQueue                   	m_cq;
		grpc::ClientContext     					m_ctx;
		grpc::Status               					m_status;
		SportsLinesRequest            				m_request;
		SportsLinesResponse            				m_response;
		std::shared_ptr<rvision::core::logger> 		m_logger;
		std::shared_ptr<grpc::Channel>				m_channel;
		std::unique_ptr<SportLinesService::Stub>	m_stub;
		std::unique_ptr<grpc::ClientAsyncReaderWriter<SportsLinesRequest, SportsLinesResponse>> m_rpc;
		enum class call_status { read = 1, write = 2, complete = 3, start = 4, done = 5, finish = 6 };
	};
}