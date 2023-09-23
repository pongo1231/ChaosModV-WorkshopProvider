#include "json_response.h"
#include "logging.h"
#include "options.h"

#include "endpoints/endpoint_common.h"

#include <httpserver.hpp>

#include <csignal>
#include <memory>

using namespace httpserver;

static void log_request(const http_request &request)
{
	LOG("-----------------------------------------");
	auto time      = std::time(nullptr);
	auto localtime = *std::localtime(&time);
	LOG("Time: " << CYAN << std::put_time(&localtime, "%d-%m-%Y %H:%M:%S") << RESET);

	LOG("Received " << RED << request.get_method() << " " << RESET << request.get_path() << " ("
	                << request.get_requestor() << ")");

	std::ostringstream headers_str_builder;
	for (const auto &pair : request.get_headers())
		headers_str_builder << YELLOW << pair.first << ": " << GREEN << pair.second << RESET << " | ";
	auto headers_str = headers_str_builder.str();
	if (!headers_str.empty())
	{
		headers_str = headers_str.substr(0, headers_str.find_last_not_of(" | "));
		LOG("Headers: " << headers_str << RESET);
	}

	std::ostringstream args_str_builder;
	for (const auto &pair : request.get_args())
		headers_str_builder << MAGENTA << pair.first << ": " << GREEN << pair.second << RESET << " | ";
	auto args_str = args_str_builder.str();
	if (!args_str.empty())
	{
		args_str = args_str.substr(0, args_str.find_last_not_of(" | "));
		LOG("Args: " << args_str << RESET);
	}

	if (!request.get_querystring().empty())
	{
		LOG("Query: " << BLUE << request.get_querystring() << RESET);
	}

	if (!request.get_content().empty())
	{
		LOG("Content: " << YELLOW << request.get_content() << RESET);
	}
}

class chaosworkshop : public http_resource
{
  public:
	chaosworkshop()
	{
	}

	const std::shared_ptr<http_response> render_GET(const http_request &request) override
	{
		log_request(request);

		const auto &endpoint = request.get_path();

		if (endpoint::get_endpoint_handlers.contains(endpoint))
			return endpoint::get_endpoint_handlers.at(endpoint)(request);

		return make_response<http_response>(404, http::http_utils::text_plain);
	}

	const std::shared_ptr<http_response> render_POST(const http_request &request) override
	{
		log_request(request);

		const auto &endpoint = request.get_path();

		if (endpoint::post_endpoint_handlers.contains(endpoint))
			return endpoint::post_endpoint_handlers.at(endpoint)(request);

		return make_response<http_response>(404, http::http_utils::text_plain);
	}
};

int main()
{
	// reap children automatically
	signal(SIGCHLD, SIG_IGN);

	global_options.read();

	webserver server = create_webserver(global_options.port)
	                       .use_ssl()
	                       .https_mem_cert("cert.pem")
	                       .https_mem_key("key.pem")
	                       .connection_timeout(global_options.connection_timeout);

	LOG("\n\n" << GREEN << "Starting http server on port " << global_options.port << "\n");

	chaosworkshop workshop;
	server.register_resource("/", &workshop, true);
	server.start(true);
}