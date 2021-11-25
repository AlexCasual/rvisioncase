#pragma once

#include <core/headers.hpp>
#include <http/utils/params.hpp>

namespace rvision::http
{
	using executer = std::function <rvision::core::errc(const Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse&, const rvision::http::params& params)>;
}