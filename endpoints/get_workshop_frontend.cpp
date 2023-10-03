#include "endpoint_common.h"

static std::shared_ptr<http_response> handle_endpoint_view(const http_request &request)
{
	COMMON_PROLOGUE

	auto token = ARG("token");
	if (token.empty())
		token = request.get_cookie("user_token");

	if (token.empty() || !user::does_token_exist(token))
		return std::make_shared<string_response>(html_file::read_file("page/workshop/login.html"), 200, "text/html");

	if (args.contains("submission_id"))
	{
		auto submission_id = ARG("submission_id");
		if (!submission_id.empty())
		{
			if (!submission::submission_id_sanitycheck(submission_id))
				return std::make_shared<string_response>(html_file::read_file("page/workshop/root.html"), 200,
				                                         "text/html");

			if (!database::exec_steps<std::string, std::string>(
			        submission::get_database(), "SELECT * FROM submissions WHERE id=@submission_id AND author=@author",
			        { "@submission_id", submission_id },
			        { "@author", user::get_user_name(user::get_token_user(token)) }))
				return std::make_shared<string_response>(html_file::read_file("page/workshop/root.html"), 200,
				                                         "text/html");
		}

		return std::make_shared<string_response>(
		    html_file::read_file("page/workshop/edit.html",
		                         { { "$$submission_id$$", "const submission_id = \"" + submission_id + "\";" } }),
		    200, "text/html");
	}

	return std::make_shared<string_response>(html_file::read_file("page/workshop/root.html"), 200, "text/html");
}

REGISTER_GET_ENDPOINT("/workshop/view", handle_endpoint_view);