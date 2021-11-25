#include "client.hpp"

namespace rvision::rpc
{
	client::client(const std::string& addrr, std::shared_ptr<rvision::core::logger> logger, const client_response_callback& cb)
	: m_shutdown(false), m_started(false), m_outstand(false), m_logger(logger), m_addrr(addrr), m_cb(cb)
	{
		m_channel = grpc::CreateChannel(m_addrr, grpc::InsecureChannelCredentials());
		m_stub = SportLinesService::NewStub(m_channel);
		m_rpc = m_stub->PrepareAsyncSubscribeOnSportsLines(&m_ctx, &m_cq);
		m_processor = std::thread(&client::process, this);
	}
	
	client::~client()
	{
		m_shutdown.store(true, std::memory_order_release);
		
		m_cv.notify_all();

		m_processor.join();
			
		cancel();

		m_cq.Shutdown();

		void* tag = nullptr;
		bool ok = false;
		while (m_cq.Next(&tag, &ok))
		{
			;
		}
	}
	
	rvision::core::errc client::call(std::uint64_t period, const std::vector<std::string>& sports)
	{
		{
			std::unique_lock<std::mutex> _lock(m_calls_lock);
			m_calls.emplace(period, sports);
		}

		bool _expected = false;
		if(m_started.compare_exchange_strong(_expected,true))
		{
			m_rpc->StartCall(reinterpret_cast<void*>(call_status::start));
		}

		return rvision::core::errc::success;
	}
				
	void client::cancel()
	{
		 m_ctx.TryCancel();
	}

	void client::process()
	{
		try
		{
			void* _tag = nullptr;
			bool _ok = false;
			while (!m_shutdown.load(std::memory_order_acquire)) 
			{
				if (m_cq.Next(&_tag, &_ok))
				{
					if (_tag)
					{
						auto _status = static_cast<call_status>(reinterpret_cast<size_t>(_tag));
						switch (_status) 
						{
							case call_status::start:
								on_start(_ok);
								break;
							case call_status::read:
								on_read(_ok);
								break;
							case call_status::write:
								on_write(_ok);
								break;							
							case call_status::done:
								on_done(_ok);
								break;
							case call_status::finish:
								on_finish(_ok);
								break;
							case call_status::complete:
								on_complete(_ok);
								break;
							default:
								m_logger->error("client::process() => unexpected tag {} delivered by notification queue", _status);
								assert(false);
						}
					}
					else 
					{
						m_logger->error("client::process() => invalid tag delivered by notification queue");
					}
				}
				else 
				{
					break;
				}
			}    
		}
		catch (std::exception& e)
		{
			m_logger->error("client::process() => caught exception: %s", e.what());
		}
		catch (...) 
		{
			m_logger->error("client::process() => caught unknown exception");
		}

		m_logger->debug("client::process() => worker thread is shutting down");
	}
	
	bool client::on_start(bool ok)
	{
		if (!ok) 
		{
			m_rpc->Finish(&m_status, reinterpret_cast<void*>(call_status::complete));

			m_logger->debug("client::on_start => ok [failed], completed");
			return false;
		}

		m_logger->debug("client::on_start => start {}", ok);
		
		if(make_request(m_request))
		{
			m_rpc->Write(m_request, reinterpret_cast<void*>(call_status::write));
		}

		return true;
	}
	
	bool client::on_read(bool ok)
	{
		if (!ok) 
		{
			m_rpc->Finish(&m_status, reinterpret_cast<void*>(call_status::complete));

			m_logger->debug("client::on_read => ok [failed], completed");
			return false;
		}
		
		m_logger->debug("client::on_read => read a new message");
				
		std::vector<std::pair<std::double_t,std::string>> __lines;

		auto _lines = m_response.lines();
		for (const auto& l : _lines)
		{
			__lines.emplace_back(l.score(), l.sport());
		}

		m_cb(__lines);
		
		m_response.Clear();
		
		if(make_request(m_request))
		{
			m_rpc->Write(m_request, reinterpret_cast<void*>(call_status::write));
		}
		else
		{
			m_rpc->Read(&m_response, reinterpret_cast<void*>(call_status::read));
		}

		return true;
	}
	
	bool client::on_write(bool ok)
	{
		if (!ok) 
		{
			m_rpc->Finish(&m_status, reinterpret_cast<void*>(call_status::complete));

			m_logger->debug("client::on_write => ok [failed], completed: {}");
			return false;
		}
		
		m_logger->debug("client::on_write => read a new message");
		
		m_outstand = false;

		m_rpc->Read(&m_response, reinterpret_cast<void*>(call_status::read));

		return true;
	}

	bool client::on_complete(bool ok)
	{
		switch (m_status.error_code()) 
		{
			case grpc::OK:
				m_logger->debug("client::on_complete => client call completed");
				break;

			case grpc::CANCELLED:
				m_logger->debug("client::on_complete => client call cancelled");
				break;

			default:
				m_logger->error("client::on_complete => client call failed {}", m_status.error_message());
				break;
		}

		return true;
	}
	
	bool client::on_done(bool ok)
	{
		m_logger->debug("client::on_done => client done, ok: {}", ok);
			
		return true;
	}
		
	bool client::on_finish(bool ok)
	{
		m_logger->debug("client::on_finish => [done] client done, ok: {}", ok);
			
		return true;
	}

	bool client::make_request(SportsLinesRequest& req)
	{
		if (m_outstand)
		{
			return false;
		}

		std::unique_lock<std::mutex> _lock(m_calls_lock);
		if(!m_calls.empty())
		{
			auto&& _call = m_calls.front();

			req.Clear();

			req.set_polling_period(_call.first);
			for (const auto& s : _call.second)
			{
				req.add_sports(s);
			}		

			m_calls.pop();

			m_outstand = true;

			return true;
		}

		return false;
	}
}