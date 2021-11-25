#include "server.hpp"

namespace rvision::rpc
{
	server::server(const std::string& addrr, size_t threads, std::shared_ptr<rvision::core::logger> logger, const rpc_score_callback& score_cb)
	:m_addrr(addrr), m_logger(logger), m_score_callback(score_cb), m_shutdown(false), m_stm(grpc::ServerAsyncReaderWriter<SportsLinesResponse, SportsLinesRequest>(&m_ctx))
	{
		 if (threads == 0)
		 {
            throw std::logic_error("server::server: threads number has to be greater than 0");
		 }
			
        m_processors.resize(threads);
	}
	
	server::~server() 
	{
		stop();
	}
	
	void server::start() 
	{
		grpc::ServerBuilder builder;
		builder.AddListeningPort(m_addrr, grpc::InsecureServerCredentials());
		builder.RegisterService(&m_service);
		
		m_cq = builder.AddCompletionQueue();
		m_server = builder.BuildAndStart();
		
		m_ctx.AsyncNotifyWhenDone(reinterpret_cast<void*>(call_status::finish));		
		
		m_writter = std::thread(&server::write, this);
		
		 for (auto& thread : m_processors)
		 {
			std::thread t(&server::process, this);
			thread.swap(t);
		}
	}
	
	void server::stop()
	{
		m_shutdown.store(true, std::memory_order_release);

		m_logger->debug("server::stop() => shutting down server");
		
		m_server->Shutdown();

		m_logger->debug("server::stop() => shutting down notification queue");

		m_cq->Shutdown();
		
		m_logger->debug("server::stop() => waiting for writter thread to shutdown");
		
		m_cv.notify_all();

		m_writter.join();

		m_logger->debug("server::stop() => waiting for worker threads to shutdown");
		
		for (auto& thread : m_processors) 
		{
			thread.join();
		}

		m_logger->debug("server::stop() => draining notification queue");
		
		void* tag = nullptr;
		bool ok = false;
		while (m_cq->Next(&tag, &ok))
		{
			;
		}

		m_logger->debug("server::stop() => exit");
	}
	
	void server::process()
	{
		 try
		 {
			m_service.RequestSubscribeOnSportsLines(&m_ctx, &m_stm, m_cq.get(), m_cq.get(), reinterpret_cast<void*>(call_status::connect));
		
			void* _tag = nullptr;
			bool _ok = false;
			while (!m_shutdown.load(std::memory_order_acquire)) 
			{
				if (m_cq->Next(&_tag, &_ok))
				{
					if (_tag)
					{
						auto _status = static_cast<call_status>(reinterpret_cast<size_t>(_tag));
						switch (_status) 
						{
							case call_status::connect:
								on_connect(_ok);
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
							default:
								m_logger->error("server::process() => unexpected tag {} delivered by notification queue", _status);
								assert(false);
						}
					}
					else 
					{
						m_logger->error("server::process() => invalid tag delivered by notification queue");
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
			m_logger->error("server::process() => caught exception: %s", e.what());
		}
		catch (...) 
		{
			m_logger->error("server::process() => caught unknown exception");
		}

		m_logger->debug("server::process() => worker thread is shutting down");
	}
	
	void server::write()
	{
		while (!m_shutdown.load(std::memory_order_acquire)) 
		{
			{
				std::unique_lock<std::mutex> locker(m_cv_lock);

				m_cv.wait(locker, [this]()
				{

						return m_shutdown.load(std::memory_order_acquire) || m_changed;
				});
			}

			std::vector<std::string> _sports;
			std::uint64_t _period = 0;

			while (!m_shutdown.load(std::memory_order_acquire)) 
			{		
				bool _changed = false;
				bool _expected = true;

				if(m_changed.compare_exchange_strong(_expected, false))
				{
					_sports.clear();

					std::unique_lock<std::shared_mutex> lock(m_written_lock);

					_period = m_last_req.polling_period();
					for (const auto& s : m_last_req.sports())
					{
						_sports.emplace_back(s);
					}

					_changed = true;					
				}

				m_logger->debug("server::on_read => polling_period {}, try to retrive scores for sports {} with changed {}", _period, _sports.size(), _changed);
									
				std::chrono::seconds _poll(_period);
				std::this_thread::sleep_for(_poll);
				
				auto lines = m_score_callback(_sports, _changed);
				
				m_logger->debug("server::on_read => lines count {}",  lines.size());
				
				SportsLinesResponse _response;
				for(const auto& l : lines)
				{
					auto line = _response.add_lines();					
					line->set_sport(l.first);
					line->set_score(l.second);
				}
				

				m_stm.Write(_response, reinterpret_cast<void*>(call_status::write));					
			}		
		}
	}
	
	bool server::on_connect(bool ok)
	{
		if (!ok) 
		{
			grpc::Status _st(grpc::StatusCode::OUT_OF_RANGE, "server::on_connect => error");
			
			m_stm.Finish(_st, reinterpret_cast<void*>(call_status::finish));
			
			m_logger->debug("server::on_connect => ok [failed], cancelled: {}", m_ctx.IsCancelled());
			return false;
		}
		
		m_logger->debug("server::on_connect => connected");
			
		m_stm.Read(&m_request, reinterpret_cast<void*>(call_status::read));

		return true;
	}
	
	bool server::on_done(bool ok)
	{
		m_logger->debug("server::on_done => server done, cancelled: {}", m_ctx.IsCancelled());
			
		return true;
	}
	
	bool server::on_finish(bool ok)
	{
		m_logger->debug("server::on_finish => [done] server done, cancelled: {}", m_ctx.IsCancelled());
			
		return true;
	}
	
	bool server::on_read(bool ok)
	{
		if (!ok)
		{
			if (m_ctx.IsCancelled())
			{
				grpc::Status _st(grpc::StatusCode::CANCELLED, "server::on_read => error");
			
				m_stm.Finish(_st, reinterpret_cast<void*>(call_status::finish));

				m_logger->error("server::on_read => ok [failed], cancelled: {}", m_ctx.IsCancelled());
				return false;
			}

			return true;
		}
		
		m_logger->debug("server::on_read => read a new message");
				
		if (check_request())
		{
			m_changed = true;
			m_cv.notify_one();
		}

		m_stm.Read(&m_request, reinterpret_cast<void*>(call_status::read));
		
		return true;
	}
	
	bool server::on_write(bool ok)
	{
		if (!ok) 
		{
			grpc::Status _st(grpc::StatusCode::OUT_OF_RANGE, "server::on_write => error");
			
			m_stm.Finish(_st, reinterpret_cast<void*>(call_status::finish));

			m_logger->debug("server::on_write => ok [failed], cancelled: {}", m_ctx.IsCancelled());
			return false;
		}
		
		m_logger->debug("server::on_write => write a new message");
			
		//m_stm.Read(&m_request, reinterpret_cast<void*>(call_status::read));

		return true;
	}

	bool server::check_request()
	{
		std::unique_lock<std::shared_mutex> lock(m_written_lock);
		if (m_last_req.polling_period() != m_request.polling_period())
		{
			m_last_req = m_request;
			return true;
		}

		if (m_last_req.sports_size() != m_request.sports_size())
		{
			m_last_req = m_request;
			return true;
		}

		//todo : deep comapre by sport value

		return false;
	}
}
