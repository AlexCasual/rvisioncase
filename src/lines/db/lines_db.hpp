#pragma once
#include <core/headers.hpp>
#include <core/logger.hpp>
#include <sqlite3.h>

namespace rvision
{	
	class lines_db
	{
		struct connection
		{
			enum class statements
			{
				add_line,
				rem_line,
				update_line,
				last_score,
				fetch_score
			};

			struct connection_cleanup
			{
				void operator()(sqlite3* p)
				{
					int res = sqlite3_close_v2(p);

					if (res != SQLITE_OK)
					{
						res = res;
					}
				}
			};

			struct statements_cleanup
			{
				void operator()(sqlite3_stmt* stmt)
				{
					int res = sqlite3_finalize(stmt);

					if (res != SQLITE_OK)
					{
						res = res;
					}
				}
			};

			using exec_callback_t = std::function<int(void *data, int argc, char **argv, char **azColName)>;


			connection(const std::string& path, std::int32_t flags, std::shared_ptr<rvision::core::logger> logger);
			~connection();

			connection(connection&&) = default;
			connection& operator=(connection&&) = default;

			using connection_ptr = std::unique_ptr < sqlite3, connection_cleanup >;
			
			rvision::core::errc execute(const std::string& sql);

			rvision::core::errc add_line(const std::string& line);
			rvision::core::errc rem_line(const std::string& line);
			rvision::core::errc update_line(const std::string& line, std::double_t score);
			rvision::core::errc last_score(const std::string& line, std::double_t& score);
			rvision::core::errc fetch_score(const std::string& line, std::vector<std::double_t>& score);
		
		private:
			static int busy_handler(void* ud, int count);
			rvision::core::errc retrieve_error(const std::string& sql, int sres, const char* serr);

		private:
			std::shared_ptr<rvision::core::logger> m_logger;
			connection_ptr m_connection;
		};

		struct connection_pool
		{
			struct no_resource : public std::exception
			{

			};

			struct draining : public std::exception
			{

			};

			struct accessor
			{
				accessor(std::shared_ptr<rvision::core::logger> logger, connection_pool& pool, std::uint32_t attemps, const std::chrono::seconds& wait);
				~accessor();
				connection* operator->() const noexcept;

			private:
				std::unique_ptr<connection> m_connection;
				std::shared_ptr<rvision::core::logger> m_logger;
				connection_pool& m_pool;
			};
						
			using pool_connection_t = std::unique_ptr<connection>;
			using pool_connection_factory_t = std::function< pool_connection_t () >;

			connection_pool(std::uint32_t size, const pool_connection_factory_t& factory);
			~connection_pool();

			friend accessor;
			
		private:
			std::uint32_t count() const;
			std::uint32_t add();
			pool_connection_t pop();
			void push(pool_connection_t&& conn);

		private:
			std::atomic<std::uint32_t> m_max;
			std::atomic<std::uint32_t> m_curr;
			std::atomic<bool> m_drainig;
			std::mutex m_lock;
			pool_connection_factory_t m_factory;
			std::deque<pool_connection_t> m_pool;
			std::shared_ptr<rvision::core::logger> m_logger;
		};		
		

	public:
		lines_db(const std::string& path, std::shared_ptr<rvision::core::logger> logger);
		~lines_db();

	public:
		rvision::core::errc add_line(const std::string& line);
		rvision::core::errc rem_line(const std::string& line);
		rvision::core::errc update_line(const std::string& line, std::double_t score);
		rvision::core::errc last_score(const std::string& line, std::double_t& score);
		rvision::core::errc fetch_score(const std::string& line, std::vector<std::double_t>& score);

	private:
		void init();
				
	private:
		std::string m_db_path;
		std::shared_ptr<rvision::core::logger> m_logger;
		std::unique_ptr<connection_pool> m_pool;
		std::mutex m_cache_lock;
		std::unordered_map<std::string, std::vector<std::double_t>> m_cache;
		std::atomic<std::uint64_t> m_max;
		std::atomic<std::uint64_t> m_curr;
	};
}