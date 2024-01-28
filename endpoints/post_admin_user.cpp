#include "endpoint_common.h"

#include "webhook.h"

#define COMMON_ADMIN                                                                           \
	do                                                                                         \
	{                                                                                          \
		USER_TOKEN_CHECK(std::string());                                                       \
		auto token = ARG("token");                                                             \
		if (!user::is_user_admin(user::get_token_user(token)))                                 \
			return make_response<string_response>(json_formulate_failure("Not allowed"), 400); \
	} while (0);

static std::shared_ptr<http_response> handle_endpoint_admingetusers(const http_request &request)
{
	COMMON_PROLOGUE
	COMMON_ADMIN

	nlohmann::json users_json;
	database::exec_steps(user::get_database(), "SELECT name,id FROM users",
	                     [&](const SQLite::Statement &statement)
	                     {
		                     nlohmann::json user_json;
		                     for (int i = 0; i < statement.getColumnCount(); i++)
			                     user_json[statement.getColumnName(i)] = statement.getColumn(i);

		                     users_json.push_back(user_json);
	                     });

	return make_response<string_response>(json_formulate_success().set("users", users_json), 400);
}

REGISTER_POST_ENDPOINT("/admin/user/get_users", handle_endpoint_admingetusers);

static std::shared_ptr<http_response> handle_endpoint_admingetuserid(const http_request &request)
{
	COMMON_PROLOGUE
	COMMON_ADMIN

	auto user_name = ARG("name");
	if (user_name.empty())
		return make_response<string_response>(json_formulate_failure("Missing name"), 400);

	if (!user::does_user_name_exist(user_name))
		return make_response<string_response>(json_formulate_failure("User doesn't exist"), 400);

	return make_response<string_response>(json_formulate_success().set("user_id", user::get_user_id(user_name)), 400);
}

REGISTER_POST_ENDPOINT("/admin/user/get_id", handle_endpoint_admingetuserid);

static std::shared_ptr<http_response> handle_endpoint_adminsetuserpassword(const http_request &request)
{
	COMMON_PROLOGUE
	COMMON_ADMIN

	auto user_id = ARG("user_id");
	if (user_id.empty())
		return make_response<string_response>(json_formulate_failure("Missing user_id"), 400);

	if (!user::does_user_id_exist(user_id))
		return make_response<string_response>(json_formulate_failure("User doesn't exist"), 400);

	// Same semantics as registration where the password is hashed with sha512 both on client and server
	auto password = ARG("password");
	if (!password.empty())
		password = util::sha512(password);
	else
	{
		password = ARG("raw_password");
		if (password.empty())
			return make_response<string_response>(json_formulate_failure("Missing password"), 400);
	}

	password = util::sha512(password);

	if (!database::exec<std::string, std::string>(user::get_database(),
	                                              "UPDATE users SET password = @password WHERE id = @user_id",
	                                              { "@password", password }, { "@user_id", user_id }))
		return make_response<string_response>(json_formulate_failure("Statement failed"), 400);

	return make_response<string_response>(json_formulate_success(), 400);
}

REGISTER_POST_ENDPOINT("/admin/user/set_password", handle_endpoint_adminsetuserpassword);

static std::shared_ptr<http_response> handle_endpoint_adminsetuserattribute(const http_request &request)
{
	COMMON_PROLOGUE
	COMMON_ADMIN

	auto user_id = ARG("user_id");
	if (user_id.empty())
		return make_response<string_response>(json_formulate_failure("Missing user_id"), 400);

	if (!user::does_user_id_exist(user_id))
		return make_response<string_response>(json_formulate_failure("User doesn't exist"), 400);

	auto attribute_name = ARG("attribute");
	if (attribute_name.empty())
		return make_response<string_response>(json_formulate_failure("Missing attribute"), 400);

	auto attribute_value = ARG("value");
	if (attribute_value.empty())
		return make_response<string_response>(json_formulate_failure("Missing value"), 400);

	std::filesystem::create_directories(file::get_data_root() + USER_DIR_FRAGMENT + user_id);

	user::set_user_attribute(user_id, attribute_name, attribute_value);

	if (attribute_name == "suspended")
	{
		for (const auto &token : user::get_user_tokens(user_id))
			user::erase_token(token);
	}

	return make_response<string_response>(json_formulate_success(), 400);
}

REGISTER_POST_ENDPOINT("/admin/user/set_attribute", handle_endpoint_adminsetuserattribute);

static std::shared_ptr<http_response> handle_endpoint_adminclearuserattribute(const http_request &request)
{
	COMMON_PROLOGUE
	COMMON_ADMIN

	auto user_id = ARG("user_id");
	if (user_id.empty())
		return make_response<string_response>(json_formulate_failure("Missing user_id"), 400);

	if (!user::does_user_id_exist(user_id))
		return make_response<string_response>(json_formulate_failure("User doesn't exist"), 400);

	auto attribute_name = ARG("attribute");
	if (attribute_name.empty())
		return make_response<string_response>(json_formulate_failure("Missing attribute"), 400);

	std::filesystem::create_directories(file::get_data_root() + USER_DIR_FRAGMENT + user_id);

	user::erase_user_attribute(user_id, attribute_name);

	return make_response<string_response>(json_formulate_success(), 400);
}

REGISTER_POST_ENDPOINT("/admin/user/clear_attribute", handle_endpoint_adminclearuserattribute);

static std::shared_ptr<http_response> handle_endpoint_admingetuserattributes(const http_request &request)
{
	COMMON_PROLOGUE
	COMMON_ADMIN

	auto user_id = ARG("user_id");
	if (user_id.empty())
		return make_response<string_response>(json_formulate_failure("Missing user_id"), 400);

	if (!user::does_user_id_exist(user_id))
		return make_response<string_response>(json_formulate_failure("User doesn't exist"), 400);

	auto response = json_formulate_success();

	return make_response<string_response>(json_formulate_success().set("attributes", user::get_user_json(user_id)),
	                                      400);
}

REGISTER_POST_ENDPOINT("/admin/user/get_attributes", handle_endpoint_admingetuserattributes);

static std::shared_ptr<http_response> handle_endpoint_adminpostwebhook(const http_request &request)
{
	COMMON_PROLOGUE
	COMMON_ADMIN

	auto description = ARG("description");
	auto fields      = ARG("fields");
	std::vector<std::pair<std::string, std::string>> fields_list;

	if (!fields.empty())
	{
		try
		{
			auto fields_json = nlohmann::json::parse(fields);
			for (const auto &field : fields_json)
			{
				if (!field.contains("name") || !field.contains("text"))
					continue;

				fields_list.emplace_back(field.at("name"), field.at("text"));
			}
		}
		catch (nlohmann::json::exception)
		{
			return make_response<string_response>(
			    json_formulate_failure("Argument 'fields' doesn't specify a json object"), 400);
		}
	}

	if (description.empty() && fields_list.empty())
		return make_response<string_response>(json_formulate_failure("Can't send empty message to webhook!"), 400);

	webhook::send({
	    .author      = "Admin",
	    .fields      = fields_list,
	    .description = description,
	});

	return make_response<string_response>(json_formulate_success(), 400);
}

REGISTER_POST_ENDPOINT("/admin/post_webhook", handle_endpoint_adminpostwebhook);