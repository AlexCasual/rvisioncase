#include "error.hpp"

namespace 
{ // anonymous namespace
  
	struct core_error_category : std::error_category
	{
	  const char* name() const noexcept override;
	  std::string message(int ev) const override;
	};
	  
	const char* core_error_category::name() const noexcept
	{
	  return "core";
	}
	  
	std::string core_error_category::message(int ev) const
	{
	  switch (static_cast<rvision::core::errc>(ev))
	  {
	  case rvision::core::errc::fail:
		return "common core error";
	   
	  case rvision::core::errc::success:
		return "common core success";
	  	  
	  default:
		return "(unrecognized error)";
	  }
}
  
const core_error_category the_core_error_category {};
  
} // anonymous namespace

namespace rvision::core
{	
	std::error_code make_error_code(rvision::core::errc e)
	{
		 return {static_cast<int>(e), the_core_error_category};
	}
	
	exception::exception(char const* const what, const rvision::core::errc& ec)
	: m_what("rvision::core::exception => "), m_errc(ec)
	{
		m_what += "[";
		m_what += what;
		m_what += "]";
		m_what += " (";
		//m_what += std::to_string(reinterpret_cast<std::int32_t>(ec));
		m_what += ")";
	}
		
	const char* exception::what() const noexcept
	{
		return m_what.c_str();
	}
		
	const rvision::core::errc& exception::code() const
	{
		return m_errc;
	}
}