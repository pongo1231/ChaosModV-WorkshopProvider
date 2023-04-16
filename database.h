#pragma once

#include <SQLiteCpp/Database.h>

#include <functional>

namespace database
{
	template <typename t> struct query_binding
	{
		std::string bind;
		t value;

		query_binding(std::string bind, t value) : bind(bind), value(value)
		{
		}
	};

	template <typename... t>
	int exec(const SQLite::Database &database, const std::string &query, const query_binding<t> &...bindings)
	{
		SQLite::Statement statement(database, query);
		(
		    [&]
		    {
			    statement.bind(bindings.bind, bindings.value);
		    }(),
		    ...);

		return statement.exec();
	}

	template <typename... t>
	int exec_steps(const SQLite::Database &database, const std::string &query, const query_binding<t> &...bindings,
	               std::function<void(const SQLite::Statement &)> callback = nullptr)
	{
		SQLite::Statement statement(database, query);
		(
		    [&]
		    {
			    statement.bind(bindings.bind, bindings.value);
		    }(),
		    ...);

		int i = 0;
		while (statement.executeStep())
		{
			i++;
			if (callback)
				callback(statement);
		}

		return i;
	}
}