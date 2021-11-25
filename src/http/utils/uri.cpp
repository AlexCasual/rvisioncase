#include "uri.hpp"

#include <Poco/StringTokenizer.h>

namespace rvision::http
{
	uri::uri(const std::string& url)
	:Poco::URI(url)
	{
	}
	
	uri::~uri()
	{
	}
		
	std::vector<std::string> uri::getSegments()
	{
		std::vector<std::string> segments;
		getPathSegments(segments);

		return segments;
	}

	rvision::http::params uri::getParams() const
	{		
		if (empty())
		{
			return rvision::http::params{};
		}

		std::string query_str(getQuery());
		if ("" == query_str)	
		{
			return rvision::http::params{};
		}

		std::map<std::string, std::string> _queries;
		
		Poco::StringTokenizer restTokenizer(query_str, "&");
		for (Poco::StringTokenizer::Iterator itr = restTokenizer.begin(); itr != restTokenizer.end(); ++itr)
		{
			Poco::StringTokenizer keyValueTokenizer(*itr, "=");
			_queries[keyValueTokenizer[0]] = (1 < keyValueTokenizer.count()) ? keyValueTokenizer[1] : "";
		}
		
		rvision::http::params _params;
		_params.set(_queries);

		return _params;
	}
}