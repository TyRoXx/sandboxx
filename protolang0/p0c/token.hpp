#pragma once
#ifndef P0C_TOKEN_HPP
#define P0C_TOKEN_HPP


#include "source_range.hpp"


namespace p0
{
	namespace token_type
	{
		enum Enum
		{
			identifier,
			var,
			function,
			return_,
			null,
			if_,
			else_,
			while_,
			break_,
			continue_,
			integer_10,
			parenthesis_left,
			parenthesis_right,
			brace_left,
			brace_right,
			bracket_left,
			bracket_right,
			assign,
			comma,
			plus,
			minus,
			star,
			slash,
			dot,
			exclamation_mark,
			tilde,
			end_of_file,
		};
	};


	struct token
	{
		token_type::Enum type;
		source_range content;


		token();
		explicit token(token_type::Enum type, source_range content);
	};
}


#endif
