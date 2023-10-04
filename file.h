#pragma once

#include <json.h>
#include <sys/stat.h>

#include <string>
#include <string_view>

namespace file
{
	inline std::string get_data_root()
	{
		static auto root = []() -> std::string
		{
			auto root_env = std::getenv("DATA_ROOT");
			if (!root_env)
				return {};

			return std::string(root_env) + "/";
		}();
		return root;
	}

	inline bool does_file_exist(std::string filename)
	{
		if (filename.find(get_data_root()) == filename.npos)
			filename = get_data_root() + filename;

		struct stat temp;
		return stat(filename.c_str(), &temp) == 0;
	}

	inline std::string read_file(std::string filename)
	{
		if (filename.find(get_data_root()) == filename.npos)
			filename = get_data_root() + filename;

		if (!file::does_file_exist(filename))
			return {};

		auto file = fopen(filename.c_str(), "r");
		fseek(file, 0, SEEK_END);
		auto file_len = ftell(file);
		rewind(file);

		std::string buffer;
		buffer.resize(file_len);
		fread(buffer.data(), 1, file_len, file);

		fclose(file);

		return buffer;
	}

	inline nlohmann::basic_json<> read_json_file(const std::string &filename)
	{
		auto file = read_file(filename);
		try
		{
			return nlohmann::json::parse(file);
		}
		catch (nlohmann::json::exception)
		{
		}
		return nlohmann::json();
	}

	inline void write_file(std::string filename, std::string_view content)
	{
		if (filename.find(get_data_root()) == filename.npos)
			filename = get_data_root() + filename;

		auto file = fopen(filename.c_str(), "w");
		fwrite(content.data(), 1, content.size(), file);
		fclose(file);
	}
}