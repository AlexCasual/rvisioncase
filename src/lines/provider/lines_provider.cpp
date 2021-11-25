#include "lines_provider.hpp"

namespace rvision
{	
	lines_provider::lines_provider(const std::unordered_map<std::string, std::uint32_t>& sports, const std::string& host, const std::string& api, const std::string& db, std::shared_ptr<rvision::core::logger> logger)
	: m_host(host), m_api(api), m_logger(logger), m_db(db, logger)
	{
		m_address += m_host;
		m_address += "/";
		m_address += m_api;

		m_logger->info("lines_provider::lines_provider => address {}", m_address);

		for(const auto& s : sports)
		{
			std::chrono::seconds period(s.second);
			add(s.first, period);
		}
	}
	
	lines_provider::~lines_provider()
	{
	}
	
	lines_provider_state lines_provider::state()
	{
		size_t inited_lines = 0;

		std::shared_lock<std::shared_mutex> lock(m_lines_cache_lock);
						
		for(const auto p: m_lines_cache)
		{
			if(p.second.inited == true)
			{
				++inited_lines;
			}
		}
		
		return (inited_lines!= 0 && inited_lines == m_lines_cache.size()) ? lines_provider_state::ready : lines_provider_state::not_inited;
	}
	
	void lines_provider::add(const std::string& sport, std::chrono::seconds& period)
	{
		std::string address(m_address);
		address += "/";
		address += sport;
		
		m_db.add_line(sport);
		
		update_cache(sport, 0.0);
		
		std::unique_lock<std::mutex> lock(m_pollers_lock);
		if(auto p = m_pollers.find(address); p == m_pollers.end())
		{
			m_pollers.emplace(std::piecewise_construct, std::forward_as_tuple(address), std::forward_as_tuple(m_logger, address, sport, period,
			[&](const std::string& sport, std::double_t score)
			{
				m_db.update_line(sport, score);
				
				update_cache(sport, score);

				m_logger->debug("lines_provider::update => line : {} with score : {}", sport, score);
				
			}));
		}
	}
	
	void lines_provider::rem(const std::string& sport)
	{
		std::string address(m_address);
		address += "//";
		address += sport;
				
		std::unique_lock<std::mutex> lock(m_pollers_lock);
		if(auto p = m_pollers.find(address); p != m_pollers.end())
		{
			m_pollers.erase(p);
		}
		
		m_db.rem_line(sport);
	}
	
	std::unordered_map<std::string, std::vector<std::double_t>> lines_provider::fetch(const std::vector<std::string>& sports)
	{
		std::unordered_map<std::string, std::vector<std::double_t>> lines;
		
		for(const auto& s : sports)
		{
			std::vector<std::double_t> score;
			m_db.fetch_score(s, score);
			
			lines[s] = score;
		}
		
		return lines;
	}
	
	std::vector<std::double_t> lines_provider::fetch(const std::string& sport)
	{
		std::vector<std::double_t> score;
		m_db.fetch_score(sport, score);
		
		return score;
	}
	
	std::unordered_map<std::string, std::double_t> lines_provider::fetch_last(const std::vector<std::string>& sports)
	{		
		std::unordered_map<std::string, std::double_t> lines;
		
		for(const auto& s : sports)
		{
			std::double_t score;
			m_db.last_score(s, score);
			
			lines[s] = score;
		}
		
		return lines;
	}
	
	void lines_provider::update_cache(const std::string& sport, std::double_t score)
	{
		std::unique_lock<std::shared_mutex> lock(m_lines_cache_lock);
		
		if(auto p = m_lines_cache.find(sport); p == m_lines_cache.end())
		{
			limes_delta_cache_item item;
			item.inited = false;
			item.last_recv = score;
			item.last_send = 0.0;
			
			m_lines_cache[sport] = item;
		}
		else
		{
			p->second.inited = true;
			p->second.last_recv = score;
		}
	}
	
	std::unordered_map<std::string, std::double_t> lines_provider::fetch_delta(const std::vector<std::string>& lines, bool changed)
	{
		std::unordered_map<std::string, std::double_t> deltas;
		
		//if(lines.size() == m_lines_cache.size())
		{
			for(const auto& l : lines)
			{
				std::unique_lock<std::shared_mutex> lock(m_lines_cache_lock);
				
				if (auto p = m_lines_cache.find(l); p != m_lines_cache.end())
				{
					//if (lines.size() == m_lines_cache.size())
					if(!changed)
					{
						m_logger->debug("lines_provider::fetch_delta => delta for {}", l);

						deltas[l] = p->second.last_recv - p->second.last_send;

						p->second.last_send = p->second.last_recv;
					}
					else
					{
						m_logger->debug("lines_provider::fetch_delta => curr for {}", l);

						deltas[l] = p->second.last_recv;
					}
				}				
			}
		}
		
		return deltas;
	}
}
