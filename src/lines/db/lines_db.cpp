#include "lines_db.hpp"

namespace rvision
{	
	static const std::string db_name("rvision.db");
	
	lines_db::connection_pool::accessor::accessor(std::shared_ptr<rvision::core::logger> logger, lines_db::connection_pool& pool, std::uint32_t attemps, const std::chrono::seconds& wait)
	: m_pool(pool), m_logger(logger)
	{
		try
		{
			while (attemps--)
			{
				m_connection = m_pool.pop();
				if (m_connection)
				{
					break;
				}
				if (!m_connection)
				{
					std::this_thread::sleep_for(wait);
				}
			}
			if (!m_connection)
			{
				throw no_resource();
			}
		}
		catch (const draining&)
		{
			m_logger->debug("connection_pool::accessor::accessor => drainig on connection: {}, with total count: {}", static_cast<void*>(m_connection.get()), m_pool.count());
		}
	}

	lines_db::connection_pool::accessor::~accessor()
	{
		if (m_connection)
		{
			m_pool.push(std::move(m_connection));
		}
	}

	lines_db::connection* lines_db::connection_pool::accessor::operator->() const noexcept
	{
		return m_connection.get();
	}

	lines_db::connection::connection(const std::string& path, std::int32_t flags, std::shared_ptr<rvision::core::logger> logger)
	:m_logger(logger)
	{
		sqlite3* pconnection = nullptr;
		int result = sqlite3_open_v2(path.c_str(), &pconnection, flags, nullptr);
		
		if (result != SQLITE_OK)
		{
			m_logger->error("connection::connection => open db: {} error: {}", path, result);
			
			throw rvision::core::exception("lines_db::connection::connection => exception !!!", rvision::core::errc::invalid_argument);
		}

		m_connection = connection_ptr(pconnection);
		
		sqlite3_busy_handler(pconnection, busy_handler, this);

	}

	lines_db::connection::~connection()
	{

	}

	int lines_db::connection::busy_handler(void* ud, int count)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		return 1;
	}

	rvision::core::errc lines_db::connection::retrieve_error(const std::string& sql, int sres, const char* serr)
	{
		std::string error_msg;
		rvision::core::errc errc = rvision::core::errc::success;
		bool executed = false;

		if (sres == SQLITE_OK || sres == SQLITE_DONE || sres == SQLITE_ROW)
		{
			executed = true;
		}

		if (serr != nullptr)
		{
			error_msg = serr;
			sqlite3_free((void*)serr);
		}

		if (!executed)
		{
			std::string msg = "lines_db.execute: sqlite3_exec (";
			msg += sql;
			msg += ") failed! Error: ";
			msg += error_msg;

			m_logger->error("connection::execute => sql_exec error: {}", msg);

			errc = rvision::core::errc::fail;
		}

		return errc;
	}

	rvision::core::errc lines_db::connection::execute(const std::string& sql)
	{
		m_logger->debug("connection::execute => sql: {}", sql);

		char* _error = nullptr;
		auto result = sqlite3_exec(m_connection.get(), sql.c_str(), nullptr, nullptr, &_error);

		auto errc = retrieve_error(sql, result, _error);
		
		return errc;
	}

	rvision::core::errc lines_db::connection::add_line(const std::string& line)
	{
		std::string _sql = std::string("CREATE TABLE IF NOT EXISTS ");
		_sql += line;
		_sql += " (id int AUTO_INCREMENT, score REAL, PRIMARY KEY (id)); CREATE INDEX IF NOT EXISTS score_index ON ";
		_sql += line;
		_sql += " (score ASC);";
	
		auto errc = execute(_sql);
		if (errc != rvision::core::errc::success)
		{
			m_logger->error("connection::add_line => create table: {} error: {}", line, errc);
		}

		return errc;
	}
	
	rvision::core::errc lines_db::connection::rem_line(const std::string& line)
	{
		std::string _sql = std::string("DROP TABLE ");
		_sql += line;
		_sql += ";";
	
		auto errc = execute(_sql);
		if (errc != rvision::core::errc::success)
		{
			m_logger->error("connection::rem_line => remove table: {} error: {}", line, errc);
		}

		return errc;
	}
	
	rvision::core::errc lines_db::connection::update_line(const std::string& line, std::double_t score)
	{
		std::string _sql = std::string("INSERT INTO ");
		_sql += line;
		_sql += " (score) values (";
		_sql += std::to_string(score);
		_sql += ");";

		auto errc = execute(_sql);
		if (errc != rvision::core::errc::success)
		{
			m_logger->error("connection::update_line => update table: {} error: {}", line, errc);
		}

		return errc;
	}
	
	rvision::core::errc lines_db::connection::last_score(const std::string& line, std::double_t& score)
	{
		std::string _sql = std::string("SELECT * FROM ");
		_sql += line;
		_sql += " ORDERED BY rowid DESC LIMIT 1;";
	
		char* error_str = nullptr;
		auto result = sqlite3_exec(m_connection.get(), _sql.c_str(), [](void *data, int argc, char **argv, char **column)
		{ 
			std::double_t* _score = (std::double_t*)data;

			return 1;
		},
		(void*)(&score), &error_str);

		auto errc = retrieve_error(_sql, result, error_str);
		if (errc != rvision::core::errc::success)
		{
			m_logger->error("connection::last_score => fetch table: {} error: {}", line, errc);
		}

		return errc;
	}
	
	rvision::core::errc lines_db::connection::fetch_score(const std::string& line, std::vector<std::double_t>& score)
	{
		int _portion = 100;

		std::string _sql = std::string("SELECT * FROM");
		_sql += line;
		_sql += "ORDERED BY id LIMIT ";
		_sql += _portion;
		_sql += ";";
	
		char* error_str = nullptr;
		auto result = sqlite3_exec(m_connection.get(), _sql.c_str(), [](void *data, int argc, char **argv, char **column)
		{ 
			std::vector<std::double_t>* _score = (std::vector<std::double_t>*)data;

			return 1;
		},
		(void*)(&score), &error_str);

		auto errc = retrieve_error(_sql, result, error_str);
		if (errc != rvision::core::errc::success)
		{
			m_logger->error("connection::fetch_score => fetch table: {} error: {}", line, errc);
		}

		return errc;
	}
		
	lines_db::connection_pool::connection_pool(std::uint32_t size, const pool_connection_factory_t& factory)
		:m_factory(factory), m_drainig(false), m_max(size), m_curr(0)
	{
		while (size--)
		{
			push(m_factory());
		}
	}

	lines_db::connection_pool::~connection_pool()
	{
		m_logger->info("connection_pool::~connection_pool => before pool_max_size: {}, pool_size: {}", m_max.load(), m_curr.load());

		m_drainig.store(true);

		while (m_max != m_curr)
		{
			;
		}

		m_logger->info("connection_pool::~connection_pool => after pool_max_size: {}, pool_size: {}", m_max.load(), m_curr.load());
	}

	lines_db::connection_pool::pool_connection_t lines_db::connection_pool::pop()
	{
		if (m_drainig)
		{
			throw draining();
		}

		std::unique_lock<std::mutex> lock(m_lock);

		if (!m_pool.empty())
		{
			auto conn = std::move(m_pool.front());

			m_pool.pop_front();

			--m_curr;

			return std::move(conn);
		}

		return nullptr;
	}

	void lines_db::connection_pool::push(pool_connection_t&& conn)
	{
		std::unique_lock<std::mutex> lock(m_lock);

		m_pool.emplace_back(std::move(conn));

		++m_curr;
	}

	std::uint32_t lines_db::connection_pool::count() const
	{
		return m_curr.load();
	}

	std::uint32_t lines_db::connection_pool::add()
	{
		std::unique_lock<std::mutex> lock(m_lock);

		m_pool.emplace_back(m_factory());

		return m_curr;
	}

	lines_db::lines_db(const std::string& path, std::shared_ptr<rvision::core::logger> logger)
		: m_logger(logger), m_db_path(path)
	{
		m_db_path += db_name;

		std::uint32_t pool_size = 2 * std::thread::hardware_concurrency();

		m_logger->info("lines_db::lines_db => path : {} db_path : {}, connection pool size : {}",path, m_db_path, pool_size);
		
		init();
				
		m_pool = std::make_unique<connection_pool>(pool_size, [&]()
		{
			std::int32_t _flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX;

			auto _connection = std::make_unique<connection>(m_db_path, _flags, logger);
				
			return _connection;
		});
	}

	lines_db::~lines_db()
	{
		m_logger->debug("lines_db::~lines_db().");
	}	

	void lines_db::init()
	{		
		std::int32_t flags = std::filesystem::exists(m_db_path) ? SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX
			: SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX;

		auto init_connection = std::make_unique<connection>(m_db_path, flags, m_logger);

		init_connection->execute("PRAGMA auto_vacuum = 2;");
		init_connection->execute("PRAGMA incremental_vacuum(10000);");

		init_connection->execute("VACUUM;");		
	}
	
	rvision::core::errc lines_db::add_line(const std::string& line)
	{
		try
		{
			m_logger->debug("lines_db::add_line => try to add line: {}", line);

			connection_pool::accessor accessor(m_logger, *m_pool, 100, std::chrono::seconds(1));

			return accessor->add_line(line);
		}
		catch (const connection_pool::no_resource& )
		{
			m_logger->error("lines_db::add_line: connection_pool::no_resource for line: {}", line);
			
			return rvision::core::errc::insufficient_resources;
		}

		return rvision::core::errc::fail;
	}
		
	rvision::core::errc lines_db::rem_line(const std::string& line)
	{
		try
		{
			m_logger->debug("lines_db::rem_line => try to rem line: {}", line);

			connection_pool::accessor accessor(m_logger, *m_pool, 100, std::chrono::seconds(1));

			return accessor->rem_line(line);
		}
		catch (const connection_pool::no_resource& )
		{
			m_logger->error("lines_db::rem_line: connection_pool::no_resource for line: {}", line);
			
			return rvision::core::errc::insufficient_resources;
		}

		return rvision::core::errc::fail;
	}
	
	rvision::core::errc lines_db::update_line(const std::string& line, std::double_t score)
	{
		try
		{
			m_logger->debug("lines_db::update_line => try to updt line: {}", line);

			connection_pool::accessor accessor(m_logger, *m_pool, 100, std::chrono::seconds(1));

			return accessor->add_line(line);
		}
		catch (const connection_pool::no_resource& )
		{
			m_logger->error("lines_db::update_line: connection_pool::no_resource for line: {}", line);
			
			return rvision::core::errc::insufficient_resources;
		}

		return rvision::core::errc::fail;
	}
	
	rvision::core::errc lines_db::last_score(const std::string& line, std::double_t& score)
	{
		try
		{
			m_logger->debug("lines_db::last_score => try to get last score for line: {}", line);
			
			connection_pool::accessor accessor(m_logger, *m_pool, 100, std::chrono::seconds(1));

			return accessor->last_score(line, score);
		}
		catch (const connection_pool::no_resource& )
		{
			m_logger->error("lines_db::last_score: connection_pool::no_resource for line: {}", line);
			
			return rvision::core::errc::insufficient_resources;
		}

		return rvision::core::errc::fail;
	}
	
	rvision::core::errc lines_db::fetch_score(const std::string& line, std::vector<std::double_t>& score)
	{
		try
		{
			m_logger->debug("lines_db::fetch_score => try to get score for line: {}", line);
			
			connection_pool::accessor accessor(m_logger, *m_pool, 100, std::chrono::seconds(1));

			return accessor->fetch_score(line, score);
		}
		catch (const connection_pool::no_resource& )
		{
			m_logger->error("lines_db::fetch_score: connection_pool::no_resource for line: {}", line);
			
			return rvision::core::errc::insufficient_resources;
		}

		return rvision::core::errc::fail;
	}
}