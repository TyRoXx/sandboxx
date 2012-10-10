#include "compiler.hpp"
#include "compiler_error.hpp"
#include "p0i/save_unit.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
using namespace std;

namespace
{
	void print_help()
	{
		cout << "p0c [source file] [target file]\n";
	}

	std::vector<char> read_file(const std::string &path)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			throw std::runtime_error("Could not open file " + path);
		}
		return std::vector<char>(
			std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>()
			);
	}

	void pretty_print_error(
		std::ostream &out,
		p0::source_range source,
		p0::compiler_error const &error
		)
	{
		auto const pos = error.position();
		size_t const line_index = std::count(
			source.begin(),
			pos.begin(),
			'\n'); //TODO O(n) instead of O(n^2)

		typedef std::reverse_iterator<p0::source_range::iterator> reverse_source_iterator;

		auto const end_of_line = std::find(
			pos.begin(),
			source.end(),
			'\n');

		auto const begin_of_line = std::find(
			reverse_source_iterator(pos.begin()),
			reverse_source_iterator(source.begin()),
			'\n').base();

		auto const line_length = std::distance(begin_of_line, end_of_line);
		auto const half_hint = 74 / 2;
		auto const hint_begin = std::max(pos.begin() - half_hint, begin_of_line);
		auto const hint_end   = std::min(pos.begin() + half_hint, end_of_line);

		std::string hint(
			hint_begin,
			hint_end
			);

		std::for_each(
			begin(hint),
			end(hint),
			[](char &c)
		{
			switch (c)
			{
			case '\r':
			case '\t':
				c = ' ';
				break;

			default:
				break;
			}
		});

		out << (line_index + 1) << ": " << error.what() << '\n';
		out << hint << '\n';

		auto const error_char_index = std::distance(hint_begin, pos.begin());
		out << std::string(error_char_index, ' ') << "^\n";
	}

	bool print_error_return_true(
		std::ostream &out,
		p0::source_range source,
		p0::compiler_error const &error,
		size_t &error_counter
		)
	{
		++error_counter;

		pretty_print_error(out, source, error);
		return true;
	}
}

int main(int argc, char **argv)
{
	if (argc <= 1)
	{
		print_help();
		return 0;
	}

	auto &error_out = std::cerr;
	try
	{
		std::string const source_file_name = argv[1];
		auto const source_file_content = read_file(source_file_name);
		std::string const target_file_name = (argc >= 3) ?
			argv[2] : "out.p0i";

		p0::source_range const source(
			source_file_content.data(),
			source_file_content.data() + source_file_content.size()
			);

		size_t error_counter = 0;
		auto const handle_error = std::bind(print_error_return_true,
			std::ref(error_out),
			source,
			std::placeholders::_1,
			std::ref(error_counter)
			);

		try
		{
			p0::compiler compiler(
				source,
				handle_error
				);

			p0::intermediate::unit const compiled_unit = compiler.compile();

			std::ofstream target_file(
				target_file_name,
				std::ios::binary
				);
			if (!target_file)
			{
				throw std::runtime_error("Could not open target file " + target_file_name);
			}

			p0::intermediate::save_unit(
				target_file,
				compiled_unit
				);
		}
		catch (p0::compiler_error const &e)
		{
			handle_error(
				e
				);
		}

		if (error_counter)
		{
			error_out << error_counter << " error";
			if (error_counter != 1)
			{
				error_out << "s";
			}
			error_out << "\n";
			return 1;
		}
	}
	catch (std::runtime_error const &e)
	{
		error_out << e.what() << '\n';
		return 1;
	}
}
