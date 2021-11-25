#include <gtest/gtest.h>
#include <lines/provider/lines_provider.hpp>

namespace rvision::rpc
{
	
}

class LinesTestEnvironment : public::testing::Environment
{
public:
	LinesTestEnvironment(const std::unordered_map<std::string, std::uint32_t>& sports, const std::string& host, const std::string& api, const std::filesystem::path& db)
	:_sports(sports), _host(host), _api(api), _db(db)
	{
	}
	
	void SetUp() override
	{
		const std::string pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
		const std::string level("info");

		_logger = spdlog::stdout_color_mt("console");

		_logger->set_level(rvision::core::detail::get_logger_level(level));
		_logger->set_pattern(pattern);

		_lines_provider = std::make_unique<rvision::lines_provider>(_sports, _host, _api, _db.generic_string(), _logger);
	}

	rvision::lines_provider* lines_provider() const
	{
		return _lines_provider.get();
	}

	
protected:
	std::shared_ptr<rvision::core::logger> _logger;
	std::unordered_map<std::string, std::uint32_t> _sports;
	std::unique_ptr<rvision::lines_provider> _lines_provider;
	std::filesystem::path _db;
	std::string _host;
	std::string _api;
};

LinesTestEnvironment* g_LinesTestEnvironment = nullptr;

class LinesTest : public ::testing::Test
{
protected:
	void SetUp()
	{
		_lines_provider = g_LinesTestEnvironment->lines_provider();
	}

	void TearDown()
	{
		_lines_provider = nullptr;
	}

	rvision::lines_provider* _lines_provider;
};

TEST_F( LinesTest, AddTest )
{
	std::chrono::seconds polling(1);
	_lines_provider->add("baseball", polling);
}

int main( int argc, char* argv[] )
{
	std::unordered_map<std::string, std::uint32_t> _sports;
	std::string _host("http:\\\\localhost:1024");
	std::string _api("api\\v1\\lines");
	std::string _db("sql_db.db");
	
	auto _db_dir = std::filesystem::temp_directory_path();
	_db_dir /= _db;

	::testing::InitGoogleTest( &argc, argv );

	g_LinesTestEnvironment = dynamic_cast<LinesTestEnvironment*>(::testing::AddGlobalTestEnvironment(new LinesTestEnvironment(_sports, _host, _api, _db_dir)));

	return RUN_ALL_TESTS();
}

