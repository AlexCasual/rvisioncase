#pragma once

#include <http/server/dispatcher.hpp>
#include <Poco/Net/HTTPRequestHandlerFactory.h>

namespace rvision::http
{
	class factory final : public Poco::Net::HTTPRequestHandlerFactory
	{
		public:
		explicit factory(const rvision::http::dispatcher& dispatcher);
		~factory() = default;
		
		private:
		Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request) override;
		
		private:
		rvision::http::dispatcher m_dispatcher;
	};
}