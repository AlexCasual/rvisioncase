#pragma once
#include <core/headers.hpp>
#include <http/utils/params.hpp>

namespace rvision::http
{
	class uri final : public Poco::URI
	{
		public:
		explicit uri(const std::string& url);
		~uri();
		
		std::vector<std::string> getSegments();
		rvision::http::params getParams() const;
	};
}