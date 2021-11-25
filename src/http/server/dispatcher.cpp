#include "dispatcher.hpp"
#include "../utils/uri.hpp"

namespace rvision::http
{
	void dispatcher::add(const std::string& method, const std::string& uri, const rvision::http::executer& exec)
	{
		rvision::http::uri _uri(uri);
		auto segments = _uri.getSegments();

		m_executers[method].emplace_back(std::make_pair(segments, exec));

		//m_executers.emplace(std::piecewise_construct, std::forward_as_tuple(method), std::forward_as_tuple(segments, exec));
	}
	
	rvision::core::errc dispatcher::execute(const Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) const
	{
		std::string method = request.getMethod();
		std::string uri = request.getURI();
				
		rvision::http::uri _uri(uri);
		auto params = _uri.getParams();
		
		rvision::http::executer _executor = nullptr;
		if(const auto& p = m_executers.find(method); p != m_executers.end())
		{
			auto segments = _uri.getSegments();
					
			for (const auto& ex : p->second)
			{
				if (ex.first.size() == segments.size())
				{
					auto n = ex.first.begin();
					auto k = segments.begin();
					
					for(; n != ex.first.end() || k != segments.end(); ++n, ++k)
					{
						if(*n != *k && (*n)[0] != ':')
						{
							_executor = nullptr;
							break;
						}
						
						if ((*n)[0] == ':')
						{
							params.set(&(*n)[1], *k);
						}
						
						_executor = ex.second;
					}
				}

				if (_executor != nullptr)
				{
					break;
				}
			}
			
			if (_executor == nullptr)
			{
				for (const auto& ex : p->second)
				{
					if (ex.first.size() == 0)
					{
						_executor = ex.second;
						break;
					}
				}
			}
		}
		
		return _executor ? _executor(request, response, params) : rvision::core::errc::not_found;
	}
}