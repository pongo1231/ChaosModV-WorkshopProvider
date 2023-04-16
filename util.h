#pragma once

#include <openssl/evp.h>
#include <unicode/uchar.h>

#include <algorithm>
#include <array>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace util
{
	inline std::string string_trim(std::string string)
	{
		if (string.empty())
			return {};

		int first_non_whitespace_pos = -1;
		for (int i = 0; i < string.length(); i++)
		{
			if (!u_isspace(string[i]) && !u_isblank(string[i]))
			{
				first_non_whitespace_pos = i;
				break;
			}
		}

		if (first_non_whitespace_pos == -1)
			return {};

		string                      = string.substr(first_non_whitespace_pos);

		int last_non_whitespace_pos = -1;
		for (int i = first_non_whitespace_pos; i < string.length(); i++)
		{
			if (!u_isspace(string[i]) && !u_isblank(string[i]))
				last_non_whitespace_pos = i;
		}

		return string.substr(first_non_whitespace_pos, last_non_whitespace_pos - first_non_whitespace_pos + 1);
	}

	inline std::string string_sanitize(std::string string)
	{
		for (int i = 0; i < string.length(); i++)
		{
			if (!u_isdefined(string[i]))
				string.erase(i);
		}

		size_t pos = string.find("\u2800");
		while (pos != string.npos)
		{
			string.erase(pos, 2);
			pos = string.find("\u2800");
		}

		return string;
	}

	inline std::vector<std::string> string_split(std::string string, std::string_view delimiter)
	{
		if (string.empty())
			return {};

		std::vector<std::string> parts;
		size_t index_start = 0, index_head = 0;
		while ((index_start = string.find_first_not_of(delimiter, index_head)) != string.npos)
		{
			index_head = string.find(delimiter, index_start);
			parts.push_back(
			    string.substr(index_start, index_head == string.npos ? string.npos : index_head - index_start));
		}

		return parts;
	}

	inline std::string hash(const std::string &data, const EVP_MD *alg)
	{
		auto evp_context = EVP_MD_CTX_new();
		EVP_DigestInit(evp_context, alg);
		EVP_DigestUpdate(evp_context, data.c_str(), data.size());
		unsigned char hash[EVP_MAX_MD_SIZE];
		unsigned int hash_len = 0;
		EVP_DigestFinal(evp_context, hash, &hash_len);
		EVP_MD_CTX_free(evp_context);

		std::string hash_str;
		{
			std::ostringstream oss;
			for (unsigned int i = 0; i < hash_len; i++)
				oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];

			hash_str = oss.str();
		}

		return hash_str;
	}

	inline std::string sha256(const std::string &data)
	{
		return hash(data, EVP_sha256());
	}

	inline std::string sha512(const std::string &data)
	{
		return hash(data, EVP_sha512());
	}

	inline std::string generate_random_string()
	{
		std::random_device rd;
		static const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyz";
		std::uniform_int_distribution dist(0, static_cast<int>(sizeof(charset) - 2 /* including null terminator */));

		std::string str;
		str.resize(16);
		for (int i = 0; i < 16; i++)
		{
			sprintf(str.data() + i, "%c", charset[dist(rd)]);
		}

		return str;
	}
}