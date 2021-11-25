#include <core/headers.hpp>
#include <core/logger.hpp>
#include <grpc++/grpc++.h>

#include "rvision.grpc.pb.h"

using rvision_rpc::SportsLinesRequest;
using rvision_rpc::SportsLinesResponse;
using rvision_rpc::SportLinesService;

namespace rvision::rpc
{
	using rpc_score_callback = std::function<std::unordered_map<std::string, std::double_t>(const std::vector<std::string>&, bool)>;

	class server final 
	{
	public:
	  server(const std::string& address, size_t threads, std::shared_ptr<rvision::core::logger> logger, const rpc_score_callback& score_cb);
	  ~server();
	  
		void start();
		void stop();

	private:
		void process() ;
		void write();
		bool on_write(bool ok);
		bool on_read(bool ok);
		bool on_finish(bool ok);
		bool on_done(bool ok);
		bool on_connect(bool ok);
		bool check_request();
		
	 private:
		rpc_score_callback m_score_callback;
		std::shared_ptr<rvision::core::logger> m_logger;
		std::unique_ptr<grpc::ServerCompletionQueue>  m_cq;
		grpc::ServerAsyncReaderWriter<SportsLinesResponse, SportsLinesRequest> m_stm;
		SportsLinesRequest m_request;
		SportsLinesRequest m_last_req;
		std::string m_addrr;
		grpc::ServerContext m_ctx;
		SportLinesService::AsyncService m_service;
		std::unique_ptr<grpc::Server> m_server;
		std::vector<std::thread>                     m_processors;
		std::atomic_bool                             m_shutdown;
		std::atomic_bool                             m_changed;
		std::condition_variable m_cv;
		std::mutex m_cv_lock;
		std::shared_mutex m_written_lock;
		std::thread m_writter;
		enum class call_status { read = 1, write = 2, connect = 3, done = 4, finish = 5 };

	};
}