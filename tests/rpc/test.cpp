#include <gtest/gtest.h>
#include <rpc/server/server.hpp>

namespace rvision::rpc
{
	class client_2
	{
		public:
		client_2(std::shared_ptr<rvision::core::logger> logger, std::shared_ptr<grpc::Channel> channel)
			: _logger(logger), _stub(SportLinesService::NewStub(channel)), _status(grpc::Status::OK)
		{
			_processing_thread.reset(new std::thread(std::bind(&client_2::processing_thread, this)));

			_stream = _stub->AsyncSubscribeOnSportsLines(&_context, &_cq, reinterpret_cast<void*>(call_status::connect));
		}

	   ~client_2() 
	   {
			std::cout << "Shutting down client_2...." << std::endl;
			
			grpc::Status status;
			
			_cq.Shutdown();
			
			_processing_thread->join();
		}
		
		bool AsyncRequest(const std::string& user) 
		{
			if (user == "quit") 
			{
				_stream->WritesDone(reinterpret_cast<void*>(call_status::write_done));
				return true;
			}

			// Data we are sending to the server.
			//_request.set_name(user);

			_stream->Write(_request, reinterpret_cast<void*>(call_status::write));
			return true;
		}

		void AsyncRequestNextMessage() 
		{
			_stream->Read(&_response, reinterpret_cast<void*>(call_status::read));
		}
		
		private:

		void processing_thread()
		{
			while (true) 
			{
				void* _tag;
				bool ok = false;
	   
				if (!_cq.Next(&_tag, &ok))
				{
					std::cerr << "client_2 stream closed. Quitting" << std::endl;
					break;
				}

				if (ok) 
				{
					std::cout << std::endl << "**** Processing completion queue tag " << _tag << std::endl;
					
					switch (static_cast<call_status>(reinterpret_cast<size_t>(_tag))) 
					{
					case call_status::read:
						std::cout << "Read a new message" << std::endl;
						break;
					case call_status::write:
						std::cout << "Sent message" << std::endl;
						AsyncRequestNextMessage();
						break;
					case call_status::connect:
						std::cout << "Server connected." << std::endl;
						break;
					case call_status::write_done:
						std::cout << "writesdone sent,sleeping 5s" << std::endl;
						_stream->Finish(&_status, reinterpret_cast<void*>(call_status::finish));
						break;
					case call_status::finish:
						std::cout << "client_2 finish status:" << _status.error_code() << ", msg:" << _status.error_message() << std::endl;
						//context_.TryCancel();
						_cq.Shutdown();
						break;
					default:
						std::cerr << "Unexpected tag " << _tag << std::endl;
						assert(false);
					}
				}
			}
		}
		
		private:
		std::shared_ptr<rvision::core::logger> _logger;
		grpc::ClientContext _context;
		grpc::CompletionQueue _cq;
		grpc::Status _status;
		std::unique_ptr<SportLinesService::Stub> _stub;
		std::unique_ptr<grpc::ClientAsyncReaderWriter<SportsLinesRequest, SportsLinesResponse>> _stream;
		SportsLinesRequest _request;
		SportsLinesResponse _response;
		std::unique_ptr<std::thread> _processing_thread;
		enum class call_status { read = 1, write = 2, connect = 3, write_done = 4, finish = 5 };
	};
}

class RpcTestEnvironment : public::testing::Environment
{
public:
	RpcTestEnvironment(const std::string& cl_addrr, const std::string& srv_addrr)
	: _cl_addrr(cl_addrr), _srv_addrr(srv_addrr)
	{
	}
	
	void SetUp() override
	{
		const std::string pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
		const std::string level("info");

		_logger = spdlog::stdout_color_mt("console");

		_logger->set_level(rvision::core::detail::get_logger_level(level));
		_logger->set_pattern(pattern);

		_rpc_client_2 = std::make_unique<rvision::rpc::client_2>(_logger, grpc::CreateChannel(_cl_addrr, grpc::InsecureChannelCredentials()));
		_rpc_server = std::make_unique<rvision::rpc::server>(_srv_addrr, 1, _logger, [](const std::vector<std::string>& lines, bool changed) -> std::unordered_map<std::string, std::double_t>
		{
			std::unordered_map<std::string, std::double_t> result;
			
			for(const auto& l : lines)
			{
				result[l] = 1.0;
			}
			
			return result;
		});
	}

	rvision::rpc::client_2* client_2() const
	{
		return _rpc_client_2.get();
	}
	
	rvision::rpc::server* server() const
	{
		return _rpc_server.get();
	}
	
protected:
	std::string _cl_addrr;
	std::string _srv_addrr;
	std::shared_ptr<rvision::core::logger> _logger;
	std::unique_ptr<rvision::rpc::client_2> _rpc_client_2;
	std::unique_ptr<rvision::rpc::server> _rpc_server;
};

RpcTestEnvironment* g_RpcTestEnvironment = nullptr;

class RpcTest : public ::testing::Test
{
protected:
	void SetUp()
	{
		_client_2 = g_RpcTestEnvironment->client_2();
		_server = g_RpcTestEnvironment->server();
	}

	void TearDown()
	{
		_client_2 = nullptr;
		_server = nullptr;
	}

	rvision::rpc::client_2* _client_2;
	rvision::rpc::server* _server;
};

TEST_F( RpcTest, ConnectTest )
{
	//ASSERT_NO_THROW( _client_2->Open( path.string() ) );
	//ASSERT_TRUE( _client_2->IsOpen() );
}

int main( int argc, char* argv[] )
{
	static const std::string _server_address("localhost::100500");
	static const std::string _client_2_address("localhost::100500");
	
	::testing::InitGoogleTest( &argc, argv );

	g_RpcTestEnvironment = dynamic_cast<RpcTestEnvironment*>(::testing::AddGlobalTestEnvironment(new RpcTestEnvironment(_client_2_address, _server_address)));

	return RUN_ALL_TESTS();
}

