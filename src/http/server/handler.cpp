#include "handler.hpp"

namespace rvision::http
{
	handler::handler(const rvision::http::dispatcher& dispatcher)
	:m_dispatcher(dispatcher)
	{
	}
	
	handler::~handler()
	{
	}

	void handler::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
	{
		auto errc = m_dispatcher.execute(request, response);
		
		if (response.getKeepAlive())
		{
			Poco::NullOutputStream nullStream;
			Poco::StreamCopier::copyStream(request.stream(), nullStream);
		}
		
		if(errc != rvision::core::errc::success)
		{
			response.setContentLength(0);
			response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_IMPLEMENTED);
			response.send();
		}
	}
}

