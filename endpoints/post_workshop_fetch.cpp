#include "endpoint_common.h"

static std::shared_ptr<http_response> handle_endpoint_fetchusersubmissions(const http_request &request)
{
	COMMON_PROLOGUE

	USER_TOKEN_CHECK(std::string());
	auto token    = ARG("token");
	auto username = user::get_user_name(token::get_token_user(token));

	json submissions_json;
	database::exec_steps<std::string>(
	    submission::get_database(), "SELECT * FROM submissions WHERE author=@username", { "@username", username },
	    [&](const SQLite::Statement &statement)
	    {
		    std::string submission_id = statement.getColumn(0);
		    for (int i = 1; i < statement.getColumnCount(); i++)
			    submissions_json[submission_id][statement.getColumnName(i)] = statement.getColumn(i);
	    });

	return make_response<string_response>(json_formulate_success().set("submissions", submissions_json));
}

REGISTER_POST_ENDPOINT("/workshop/fetch_my_submissions", handle_endpoint_fetchusersubmissions);