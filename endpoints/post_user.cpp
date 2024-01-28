#include "endpoint_common.h"

static std::unordered_map<std::string, std::uint64_t> requestor_last_registration;

static std::shared_ptr<http_response> handle_endpoint_userlogin(const http_request &request)
{
	COMMON_PROLOGUE

	auto name = ARG("name");
	if (name.empty())
		return make_response<string_response>(json_formulate_failure("Missing name"), 400);

	auto password = ARG("raw_password");
	if (password.empty())
	{
		password = ARG("password");
		if (password.empty())
			return make_response<string_response>(json_formulate_failure("Missing password"), 400);

		password = util::sha512(password);
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

static std::shared_ptr<http_response> handle_endpoint_userregister(const http_request &request)
{
	COMMON_PROLOGUE

	auto requestor = std::string(request.get_requestor());
	if (!global_options.requestor_substitute_header.empty())
		requestor = request.get_header(global_options.requestor_substitute_header);

	auto time = std::time(nullptr);
	if (requestor_last_registration.contains(requestor)
	    && time - requestor_last_registration.at(requestor) < global_options.user_time_between_registrations)
		return make_response<string_response>(json_formulate_failure("Can not register at this time, try again later"),
		                                      400);

	auto name = ARG("name");
	if (name.empty())
		return make_response<string_response>(json_formulate_failure("Missing name"), 400);

	if (name.length() < global_options.user_min_name_length)
		return make_response<string_response>(
		    json_formulate_failure("Name too short (min " + std::to_string(global_options.user_min_name_length)
		                           + " characters)"),
		    400);

	if (name.length() > global_options.user_max_name_length)
		return make_response<string_response>(
		    json_formulate_failure("Name too long (max " + std::to_string(global_options.user_max_name_length)
		                           + " characters)"),
		    400);

	if (std::find_if(name.begin(), name.end(), [](unsigned char c) { return c > 127 || c == ' '; }) != name.end())
		return make_response<string_response>(json_formulate_failure("Name contains invalid characters"), 400);

	if (user::does_user_name_exist(name))
		return make_response<string_response>(json_formulate_failure("Name already in use"), 400);

	auto password = ARG("password");
	if (password.empty())
		return make_response<string_response>(json_formulate_failure("Missing password"), 400);

	if (password.length() > global_options.user_max_password_length)
		return make_response<string_response>(
		    json_formulate_failure("Password too long (max " + std::to_string(global_options.user_max_password_length)
		                           + " characters)"),
		    400);

	password                               = util::sha512(password);

	requestor_last_registration[requestor] = time;

	std::string user_id;
	while (user_id.empty())
	{
		user_id = util::generate_random_string();
		if (user::does_user_id_exist(user_id))
			user_id.clear();
	}

	if (!database::exec<std::string, std::string, std::string>(
	        user::get_database(), "INSERT INTO users (name, id, password) VALUES (@name, @user_id, @password)",
	        { "@name", name }, { "@user_id", user_id }, { "@password", password }))
		return make_response<string_response>(json_formulate_failure("Statement failed"), 400);

	std::filesystem::create_directories(file::get_data_root() + USER_DIR_FRAGMENT + user_id);

	auto token = util::generate_random_string();
	user::set_token_user(token, user_id);

	return make_response<string_response>(json_formulate_success().set("user_id", user_id).set("token", token), 400);
}

REGISTER_POST_ENDPOINT("/user/register", handle_endpoint_userregister);

static std::shared_ptr<http_response> handle_endpoint_usertokenvalidation(const http_request &request)
{
	COMMON_PROLOGUE

	auto token = ARG("token");
	if (token.empty())
		return make_response<string_response>(json_formulate_failure("Missing token"), 400);

	return make_response<string_response>(json_formulate_success().set("token_valid", user::does_token_exist(token)));
}

REGISTER_POST_ENDPOINT("/user/token_valid", handle_endpoint_usertokenvalidation);