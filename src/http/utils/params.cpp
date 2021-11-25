#include "params.hpp"

namespace rvision::http
{
	void params::set(const std::map<std::string, std::string>& param)
	{
		for (const auto& i : param)
		{
			m_params[i.first] = i.second;
		}
	}
	
	void params::set(const std::string& name, const std::string& value)
	{
		m_params[name] = value;
	}

	std::string params::get(const std::string& name, const std::string& deflt) const
	{
		if(auto p = m_params.find(name); p != m_params.end())
		{
			return p->second;
		}
		
		return deflt;
	}
}