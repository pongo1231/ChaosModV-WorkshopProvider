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
	std::ostringstream log_builder;

	log_builder << "-----------------------------------------\n";
	auto time      = std::time(nullptr);
	auto localtime = *std::localtime(&time);
	log_builder << "Time: " << CYAN << std::put_time(&localtime, "%d-%m-%Y %H:%M:%S") << RESET << "\n";

	log_builder << "Received " << RED << request.get_method() << " " << RESET << request.get_path() << " ("
	            << request.get_requestor() << ")\n";

	std::ostringstream headers_str_builder;
	for (const auto &pair : request.get_headers())
		headers_str_builder << YELLOW << pair.first << ": " << GREEN << pair.second << RESET << " | ";
	auto headers_str = headers_str_builder.str();
	if (!headers_str.empty())
	{
		headers_str = headers_str.substr(0, headers_str.find_last_not_of(" | "));
		log_builder << "Headers: " << headers_str << RESET << "\n";
	}

	std::ostringstream args_str_builder;
	for (const auto &pair : request.get_args_flat())
		headers_str_builder << MAGENTA << pair.first << ": " << GREEN << pair.second << RESET << " | ";
	auto args_str = args_str_builder.str();
	if (!args_str.empty())
	{
		args_str = args_str.substr(0, args_str.find_last_not_of(" | "));
		log_builder << "Args: " << args_str << RESET << "\n";
	}

	if (!request.get_querystring().empty())
	{
		log_builder << "Query: " << BLUE << request.get_querystring() << RESET << "\n";
	}

	if (!request.get_content().empty())
	{
		log_builder << "Content: " << YELLOW << request.get_content() << RESET << "\n";
	}

	LOG(log_builder.str());
}

class chaosworkshop : public http_resource
{
  public:
	chaosworkshop()
	{
	}

	std::shared_ptr<http_response> render_GET(const http_request &request) override
	{
		log_request(request);

		const auto &endpoint = request.get_path();

		if (endpoint::get_endpoint_handlers.contains(endpoint))
			return endpoint::get_endpoint_handlers.at(endpoint)(request);

		return make_response<http_response>(404, http::http_utils::text_plain);
	}

	std::shared_ptr<http_response> render_POST(const http_request &request) override
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

	auto server_builder = create_webserver(global_options.port).connection_timeout(global_options.connection_timeout);

	if (global_options.use_tls)
	{
		if (!file::does_file_exist("cert.pem") || !file::does_file_exist("key.pem"))
		{
			LOG(RED
			    << "No cert.pem or key.pem files found in DATA_ROOT path, aborting (set use_tls to false to disable)\n"
			    << WHITE);
			exit(EXIT_FAILURE);
		}

		server_builder = server_builder.use_ssl()
		                     .https_mem_cert(file::get_data_root() + "cert.pem")
		                     .https_mem_key(file::get_data_root() + "key.pem");
	}

	LOG("\n\n" << GREEN << "Starting http server on port " << global_options.port << "\n" << WHITE);

	webserver server = server_builder;

	chaosworkshop workshop;
	server.register_resource("/", &workshop, true);
	server.start(true);
}