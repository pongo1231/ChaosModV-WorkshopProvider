#pragma once

#include <json.h>
#include <sys/stat.h>

#include <string>
#include <string_view>

namespace file
{
	inline bool does_file_exist(std::string_view file_name)
	{
		struct stat temp;
		return stat(file_name.data(), &temp) == 0;
	}

	inline std::string read_file(std::string_view filename)
	{
		if (!file::does_file_exist(filename))
			return {};

		auto file = fopen(filename.data(), "r");
		fseek(file, 0, SEEK_END);
		auto file_len = ftell(file);
		rewind(file);

		std::string buffer;
		buffer.resize(file_len);
		fread(buffer.data(), 1, file_len, file);

		fclose(file);

		return buffer;
	}

	inline nlohmann::basic_json<> read_json_file(std::string_view filename)
	{
		auto file = file::read_file(filename);
		try
		{
			return nlohmann::json::parse(file);
		}
		catch (nlohmann::json::exception)
		{
		}
		return nlohmann::json();
	}

	inline void write_file(std::string_view filename, std::string_view content)
	{
		auto file = fopen(filename.data(), "w");
		fwrite(content.data(), 1, content.size(), file);
		fclose(file);
	}
}