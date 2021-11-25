#include "lines_poller.hpp"

namespace rvision
{
	lines_poller::lines_poller(std::shared_ptr<rvision::core::logger> logger, const std::string& address, const std::string& sport, const std::chrono::seconds& period, const poll_callback& cb)
	:m_logger(logger), m_address(address), m_sport(sport), m_period(period), m_poll_callback(cb)
	{
		m_logger->info("lines_poller::lines_poller => address: {}", m_address);

		m_thread = std::jthread([&] (std::stop_token stoken)
		{
			while(!stoken.stop_requested())
			{
				std::unique_lock<std::mutex> locker(m_lock);

				auto waiter = m_wait.wait_for(locker, m_period);
				
				if (stoken.stop_requested())
				{
					m_logger->debug("lines_poller::lines_poller::thread => break!");
					break;
				}

				poll();
			}
		});	

	}
	
	lines_poller::~lines_poller()
	{
		m_thread.request_stop();
		
		m_logger->info("lines_poller::~lines_poller => stopping main thread...");
		
		m_thread.join();
		
		m_logger->info("lines_poller::~lines_poller => main thread has been stopped.");

	}
	
	void lines_poller::poll() try
	{
		Poco::URI uri(m_address);
		
		m_logger->debug("lines_poller::poll => address: {}, host: {}, port: {}", m_address, uri.getHost(), uri.getPort());

		Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());

		session.setKeepAlive(true);
		session.setKeepAliveTimeout(Poco::Timespan(60, 0));
		session.setTimeout(Poco::Timespan(60, 0));
		
		Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, uri.getPath(), Poco::Net::HTTPMessage::HTTP_1_1);

		session.sendRequest(request);
				
		Poco::Net::HTTPResponse responce;				
		std::istream& responce_ss = session.receiveResponse(responce);
		
		if (responce.getStatus() == Poco::Net::HTTPResponse::HTTPStatus::HTTP_OK)
		{
			auto _sport = m_sport;
			std::transform(_sport.begin(), _sport.end(), _sport.begin(), [](unsigned char c)
			{
				 return std::toupper(c);
			});
			
			Poco::Util::JSONConfiguration responce_data(responce_ss);

			std::string property("lines.");
			property += _sport;
	
			auto score = responce_data.getDouble(property, 0.0);

			m_logger->debug("lines_poller::poll => property: {}, score: {}", property, score);

			m_poll_callback(m_sport, score);
		}
		else
		{
			m_logger->error("lines_poller::poll => responce status: {}", responce.getStatus());
		}

	}
	catch (const std::exception& e)
	{
		m_logger->error("lines_poller::poll => exception: {}", e.what());
	}
}