#include "parser.hpp"
#include "compiler_error.hpp"
#include "scanner.hpp"
#include <cassert>


namespace p0
{
	namespace
	{
		std::string source_range_to_string(source_range source)
		{
			return std::string(
				source.begin(),
				source.end()
				);
		}
	}


	parser::parser(
		scanner &scanner,
		compiler_error_handler error_handler
		)
		: m_scanner(scanner)
		, m_error_handler(std::move(error_handler))
		, m_is_next_token(false)
	{
		assert(error_handler);
	}

	function_tree parser::parse_unit()
	{
		return parse_function();
	}


	function_tree parser::parse_function()
	{
		function_tree::statements body;

		for (;;)
		{
			try
			{
				auto statement = parse_statement();
				if (!statement)
				{
					//end of file
					break;
				}
				body.push_back(std::move(statement));
			}
			catch (compiler_error const &e)
			{
				if (!m_error_handler(e))
				{
					throw;
				}
				
				m_scanner.skip_line();
			}
		}

		return function_tree(std::move(body));
	}

	std::unique_ptr<statement_tree> parser::parse_statement()
	{
		token const first = pop_token();
		switch (first.type)
		{
		case token_type::var:
			{
				token const name_token = pop_token();
				expect_token_type(
					name_token,
					token_type::identifier,
					"Name of declared variable expected");

				token const assign_token = pop_token();
				expect_token_type(
					assign_token,
					token_type::assign,
					"Assignment operator '=' expected");

				auto value = parse_expression();
				return std::unique_ptr<statement_tree>(new declaration_tree(
					source_range_to_string(name_token.content),
					std::move(value)
					));
			}

		case token_type::return_:
			{
				auto value = parse_expression();
				return std::unique_ptr<statement_tree>(new return_tree(
					std::move(value)
					));
			}

		case token_type::end_of_file:
			return std::unique_ptr<statement_tree>();

		default:
			throw compiler_error(
				"Unexpected token",
				first.content
				);
		}
	}

	std::unique_ptr<expression_tree> parser::parse_expression()
	{
		auto const first = pop_token();
		switch (first.type)
		{
		case token_type::identifier:
			return std::unique_ptr<expression_tree>(
				new name_expression_tree(
					first.content
				));

		case token_type::integer_10:
			return std::unique_ptr<expression_tree>(
				new integer_10_expression_tree(
					first.content
				));

		case token_type::parenthesis_left:
			{
				auto value = parse_expression();
				auto const closing_parenthesis = pop_token();
				expect_token_type(closing_parenthesis, token_type::parenthesis_right,
					"Closing parenthesis ')' expected");
				return std::move(value);
			}

		case token_type::call:
			{
				auto function = parse_expression();

				auto const opening_parenthesis = pop_token();
				expect_token_type(opening_parenthesis, token_type::parenthesis_left,
					"Opening parenthesis '(' expected");

				call_expression_tree::expression_vector arguments;

				for (;;)
				{
					if (try_skip_token(token_type::parenthesis_right))
					{
						break;
					}

					auto argument = parse_expression();
					arguments.push_back(std::move(argument));

					if (!try_skip_token(token_type::comma))
					{
						skip_token(
							token_type::parenthesis_right,
							"Comma or closing parenthesis expected"
							);
						break;
					}
				}

				return std::unique_ptr<expression_tree>(new call_expression_tree(
					std::move(function),
					std::move(arguments)
					));
			}

		default:
			throw compiler_error(
				"Expression expected",
				first.content
				);
		}
	}

	void parser::expect_token_type(const token &token, token_type::Enum type, const std::string &message) const
	{
		if (token.type != type)
		{
			throw compiler_error(
				message,
				token.content
				);
		}
	}

	token const &parser::peek_token()
	{
		if (!m_is_next_token)
		{
			m_next_token = m_scanner.next_token();
			m_is_next_token = true;
		}

		return m_next_token;
	}

	token parser::pop_token()
	{
		if (m_is_next_token)
		{
			m_is_next_token = false;
			return m_next_token;
		}
		else
		{
			return m_scanner.next_token();
		}
	}

	bool parser::try_skip_token(token_type::Enum type)
	{
		auto const token = peek_token();
		if (token.type == type)
		{
			pop_token();
			return true;
		}

		return false;
	}

	void parser::skip_token(token_type::Enum type, char const *message)
	{
		if (!try_skip_token(type))
		{
			throw compiler_error(
				message,
				peek_token().content
				);
		}
	}
}
