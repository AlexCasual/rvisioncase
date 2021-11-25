#pragma once
#include <http/server/dispatcher.hpp>

namespace rvision::http
{
	class handler : public Poco::Net::HTTPRequestHandler
	{
		public:
			explicit handler(const rvision::http::dispatcher& dispatcher);
			virtual ~handler();

		public:
			void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override;

		private:
			const rvision::http::dispatcher& m_dispatcher;
	};
}

