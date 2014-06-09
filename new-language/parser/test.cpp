#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "parser.hpp"
#include "analyze.hpp"
#include <unordered_map>
#include <boost/lexical_cast.hpp>

BOOST_AUTO_TEST_CASE(scan_token_end_of_file)
{
	Si::memory_source<char> empty;
	boost::optional<nl::token> scanned = nl::scan_token(empty);
	BOOST_REQUIRE(scanned);
	BOOST_CHECK(nl::token_type::end_of_file == scanned->type);
	BOOST_CHECK_EQUAL("", scanned->content);
}

BOOST_AUTO_TEST_CASE(scan_token_sequence)
{
	std::string const input = ".,()= \t\n\"string\"identifier 123 return";
	auto const tokens =
	{
		nl::token_type::dot,
		nl::token_type::comma,
		nl::token_type::left_parenthesis,
		nl::token_type::right_parenthesis,
		nl::token_type::assignment,
		nl::token_type::space,
		nl::token_type::tab,
		nl::token_type::newline,
		nl::token_type::string,
		nl::token_type::identifier,
		nl::token_type::space,
		nl::token_type::integer,
		nl::token_type::space,
		nl::token_type::return_,
		nl::token_type::end_of_file
	};
	Si::memory_source<char> source(boost::make_iterator_range(input.data(), input.data() + input.size()));
	for (auto const expected : tokens)
	{
		boost::optional<nl::token> const scanned = nl::scan_token(source);
		BOOST_REQUIRE(scanned);
		BOOST_CHECK_EQUAL(static_cast<int>(expected), static_cast<int>(scanned->type));
	}
	BOOST_CHECK(!Si::get(source));
}

BOOST_AUTO_TEST_CASE(scan_token_string)
{
	std::string const input = "\"abc123\\\"\\\\\"";
	Si::memory_source<char> source(boost::make_iterator_range(input.data(), input.data() + input.size()));
	boost::optional<nl::token> scanned = nl::scan_token(source);
	BOOST_REQUIRE(scanned);
	BOOST_CHECK(nl::token_type::string == scanned->type);
	BOOST_CHECK_EQUAL("abc123\"\\", scanned->content);

	boost::optional<nl::token> eof = nl::scan_token(source);
	BOOST_REQUIRE(eof);
	BOOST_CHECK(nl::token_type::end_of_file == eof->type);
	BOOST_CHECK_EQUAL("", eof->content);
}

template <class TokenizerHandler>
void with_tokenizer(std::string const &code, TokenizerHandler const &handle)
{
	Si::memory_source<char> source(boost::make_iterator_range(code.data(), code.data() + code.size()));
	auto lexer = Si::make_generator_source<nl::token>([&source]
	{
		auto token = nl::scan_token(source);
		if (!token)
		{
			throw std::runtime_error("lexer failure");
		}
		return token;
	});
	Si::buffering_source<nl::token> buffer(lexer, 1);
	handle(buffer);
}

BOOST_AUTO_TEST_CASE(ast_parse_parameters)
{
	std::string const input = "uint32 a, unicode.code_point c)";
	with_tokenizer(input, [](Si::source<nl::token> &tokens)
	{
		auto parsed = nl::ast::parse_parameters(tokens, 0);
		BOOST_REQUIRE_EQUAL(2, parsed.size());
	});
}

BOOST_AUTO_TEST_CASE(ast_lambda)
{
	std::string const input =
			"(uint32 b, string s)\n"
			"\ta = 2\n"
			"\treturn 1\n"
			;
	Si::memory_source<char> source(boost::make_iterator_range(input.data(), input.data() + input.size()));
	auto lexer = Si::make_generator_source<nl::token>([&source]
	{
		auto token = nl::scan_token(source);
		if (!token)
		{
			throw std::runtime_error("lexer failure");
		}
		return token;
	});
	Si::buffering_source<nl::token> buffer(lexer, 1);
	auto parsed = nl::ast::parse_expression(buffer, 0);
	nl::ast::lambda const * const lambda = boost::get<nl::ast::lambda>(&parsed);
	BOOST_REQUIRE(lambda);

	nl::ast::lambda expected;
	expected.parameters.emplace_back(nl::ast::parameter{nl::ast::identifier{nl::token{nl::token_type::identifier, "uint32"}}, nl::token{nl::token_type::identifier, "b"}});
	expected.parameters.emplace_back(nl::ast::parameter{nl::ast::identifier{nl::token{nl::token_type::identifier, "string"}}, nl::token{nl::token_type::identifier, "s"}});
	expected.body.elements.emplace_back(nl::ast::definition{nl::token{nl::token_type::identifier, "a"}, nl::ast::integer{nl::token{nl::token_type::integer, "2"}}});
	expected.body.result = nl::ast::integer{nl::token{nl::token_type::integer, "1"}};
	BOOST_CHECK_EQUAL(expected, *lambda);

	std::string back_to_str = boost::lexical_cast<std::string>(*lambda);
	BOOST_CHECK_EQUAL(input, back_to_str);
}

BOOST_AUTO_TEST_CASE(analyzer_lambda)
{
	nl::il::external uint32{"uint32"};
	nl::il::name_space context;
	context.next = nullptr;
	context.definitions.insert(std::make_pair("uint32", nl::il::name_space_entry{nl::il::null(), nl::il::type(uint32)}));
	nl::ast::lambda lambda;
	lambda.body.result = nl::ast::identifier{nl::token{nl::token_type::integer, "a"}};
	lambda.parameters.emplace_back(nl::ast::parameter{nl::ast::identifier{nl::token{nl::token_type::identifier, "uint32"}}, nl::token{nl::token_type::identifier, "a"}});
	auto analyzed = nl::il::analyze(lambda, context);
	nl::il::make_closure expected;
	expected.parameters.emplace_back(nl::il::parameter{uint32, "a"});
	expected.body.result = nl::il::definition_expression{"a", uint32, boost::none, 0};
	BOOST_CHECK_EQUAL(nl::il::expression(expected), analyzed);
}

BOOST_AUTO_TEST_CASE(analyzer_argument_type_mismatch)
{
	nl::il::external uint32{"uint32"};
	nl::il::external uint64{"uint64"};
	nl::il::external f{"f"};
	nl::il::name_space context;
	context.next = nullptr;
	context.definitions.insert(std::make_pair("uint32", nl::il::name_space_entry{nl::il::null(), nl::il::type(uint32)}));
	context.definitions.insert(std::make_pair("f", nl::il::name_space_entry{nl::il::type(nl::il::signature{uint32, {nl::il::type(uint64)}}), nl::il::value(f)}));
	nl::ast::lambda lambda;
	lambda.body.result = nl::ast::call{nl::ast::identifier{nl::token{nl::token_type::integer, "f"}}, {nl::ast::identifier{nl::token{nl::token_type::integer, "a"}}}};
	lambda.parameters.emplace_back(nl::ast::parameter{nl::ast::identifier{nl::token{nl::token_type::identifier, "uint32"}}, nl::token{nl::token_type::identifier, "a"}});
	BOOST_CHECK_EXCEPTION(nl::il::analyze(lambda, context), std::runtime_error, [](std::runtime_error const &ex)
	{
		return ex.what() == std::string("Argument type mismatch"); //TODO
	});
}

namespace
{
	nl::ast::block parse(std::string const &code)
	{
		nl::ast::block result;
		with_tokenizer(code, [&result](Si::source<nl::token> &tokens)
		{
			result = nl::ast::parse_block(tokens, 0);
		});
		return result;
	}
}

BOOST_AUTO_TEST_CASE(analyzer_chaining)
{
	std::string const code = "return f(g())\n";
	auto const parsed = parse(code);

	nl::il::type const uint32{nl::il::external{"uint32"}};
	nl::il::value const f{nl::il::external{"f"}};
	nl::il::value const g{nl::il::external{"g"}};

	nl::il::name_space globals;
	globals.next = nullptr;
	globals.definitions =
	{
		{"uint32", {nl::il::null(), uint32}},
		{"f", {nl::il::signature{uint32, {uint32}}, f}},
		{"g", {nl::il::signature{uint32, {}}, g}}
	};

	nl::il::block const analyzed = nl::il::analyze_block(parsed, globals);
	nl::il::block expected;
	auto g_call = nl::il::call{nl::il::definition_expression{"g", nl::il::signature{uint32, {}}, g, 0}, {}};
	auto f_call = nl::il::call{nl::il::definition_expression{"f", nl::il::signature{uint32, {uint32}}, f, 0}, {g_call}};
	expected.result = f_call;
	BOOST_CHECK_EQUAL(expected, analyzed);
}

namespace
{
	struct closure;

	typedef boost::variant<nl::il::value, boost::recursive_wrapper<closure>> value;

	struct closure
	{
		nl::il::make_closure const *original;
		boost::unordered_map<std::string, value> bound;
	};

	struct expression_evaluator : boost::static_visitor<value>
	{
		explicit expression_evaluator(boost::unordered_map<std::string, value> const &locals)
			: m_locals(locals)
		{
		}

		value operator()(nl::il::constant_expression const &expr) const
		{
			return expr.constant;
		}

		value operator()(nl::il::make_closure const &expr) const
		{

		}

		value operator()(nl::il::subscript const &expr) const
		{

		}

		value operator()(nl::il::call const &expr) const
		{

		}

		value operator()(nl::il::definition_expression const &expr) const
		{

		}

	private:

		boost::unordered_map<std::string, value> const &m_locals;
	};

	value evaluate(nl::il::expression const &expr, boost::unordered_map<std::string, value> const &locals)
	{
		return boost::apply_visitor(expression_evaluator{locals}, expr);
	}

	value evaluate(nl::il::block const &program, boost::unordered_map<std::string, value> locals)
	{
		for (nl::il::definition const &definition : program.definitions)
		{
			locals[definition.name] = evaluate(definition.value, locals);
		}
		return evaluate(program.result, locals);
	}
}

BOOST_AUTO_TEST_CASE(il_interpretation)
{
	std::string const code =
			"print_hello = ()\n"
			"	return print_line(\"Hello, world!\")\n"
			"\n"
			"return print_hello()\n";
	auto const parsed = parse(code);

	nl::il::value const print_line{nl::il::external{"print_line"}};
	nl::il::value const print_operation{nl::il::external{"print_operation"}};

	nl::il::name_space globals;
	globals.next = nullptr;
	globals.definitions =
	{
		{"print_line", {nl::il::signature{print_operation, {nl::il::null{}}}}}
	};

	nl::il::block const analyzed = nl::il::analyze_block(parsed, globals);

}
