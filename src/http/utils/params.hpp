#pragma once
#include <core/headers.hpp>

namespace rvision::http
{
	class params final
	{
		public:
		explicit params() = default;
		~params() = default;
		
		void set(const std::map<std::string, std::string>& params);
		void set(const std::string& name, const std::string& value);

		std::string get(const std::string& name, const std::string& deflt) const;

		private:
		std::map<std::string, std::string> m_params;

	};
}