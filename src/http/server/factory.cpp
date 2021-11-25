#include "factory.hpp"
#include "handler.hpp"
#include <Poco/Net/HTTPServerRequest.h>

namespace rvision::http
{
	factory::factory(const rvision::http::dispatcher& dispatcher)
	:m_dispatcher(dispatcher)
	{
	}
	
	Poco::Net::HTTPRequestHandler* factory::createRequestHandler(const Poco::Net::HTTPServerRequest& request)
	{
		return new rvision::http::handler(m_dispatcher);
	}
}