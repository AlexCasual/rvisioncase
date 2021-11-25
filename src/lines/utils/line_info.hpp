#pragma once
#include <core\headers.hpp>

namespace rvision
{	
	struct line_info
	{
		line_info(const std::string& sport, std::double_t score)
		:m_sport(sport), m_score(score)
		{
		}
		
		line_info()
		{
		}
		
		const std::string& sport() const
		{
			return m_sport;
		}
		
		std::double_t score() const
		{
			return m_score;
		}
		
		private:
		std::string m_sport;
		std::double_t m_score;
	}
}