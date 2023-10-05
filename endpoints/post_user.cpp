#include "endpoint_common.h"

static std::shared_ptr<http_response> handle_endpoint_userlogin(const http_request &request)
{
	COMMON_PROLOGUE

	auto name     = ARG("name");
	auto password = ARG("password");

	if (name.empty())
		return make_response<string_response>(json_formulate_failure("Missing name"), 400);

	if (password.empty())
	{
		auto raw_password = ARG("raw_password");
		if (raw_password.empty())
			return make_response<string_response>(json_formulate_failure("Missing password"), 400);

		password = util::sha512(raw_password);
	}

	std::shared_ptr<string_response> error_response;
	std::string token;
	if (!database::exec_steps<std::string>(
	        user::get_database(), "SELECT * FROM users WHERE name=@name", { "@name", name },
	        [&](const SQLite::Statement &statement)
	        {
		        if (statement.getColumn("password").getString() != password)
		        {
			        error_response = make_response<string_response>(json_formulate_failure("Invalid password"), 400);
			        return;
		        }

		        std::string user_id = statement.getColumn("id");
		        if (user_id.empty())
		        {
			        error_response = make_response<string_response>(json_formulate_failure("Invalid user"), 400);
			        return;
		        }

		        if (user::contains_user_attribute(user_id, "suspended"))
		        {
			        error_response =
			            make_response<string_response>(json_formulate_failure("Account is suspended"), 400);
			        return;
		        }

		        const auto &tokens = user::get_user_tokens(user_id);
		        if (tokens.size() > global_options.user_max_active_tokens)
			        user::erase_token(tokens[0]);

		        token = util::generate_random_string();
		        user::set_token_user(token, user_id);
	        }))
		return make_response<string_response>(json_formulate_failure("Name not found"), 400);

	if (error_response)
		return error_response;

	return make_response<string_response>(json_formulate_success().set("token", token));
}

REGISTER_POST_ENDPOINT("/user/login", handle_endpoint_userlogin);

static std::shared_ptr<http_response> handle_endpoint_usertokenvalidation(const http_request &request)
{
	COMMON_PROLOGUE

	auto token = ARG("token");
	if (token.empty())
		return make_response<string_response>(json_formulate_failure("Missing token"), 400);

	return make_response<string_response>(json_formulate_success().set("token_valid", user::does_token_exist(token)));
}

REGISTER_POST_ENDPOINT("/user/token_valid", handle_endpoint_usertokenvalidation);