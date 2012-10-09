#include "function_tree.hpp"


namespace p0
{
	expression_tree_visitor::~expression_tree_visitor()
	{
	}


	expression_tree::~expression_tree()
	{
	}


	name_expression_tree::name_expression_tree(
		source_range name
		)
		: m_name(name)
	{
	}

	source_range const &name_expression_tree::name() const
	{
		return m_name;
	}

	void name_expression_tree::accept(expression_tree_visitor &visitor) const
	{
		visitor.visit(*this);
	}


	integer_10_expression_tree::integer_10_expression_tree(
		source_range value
		)
		: m_value(value)
	{
	}

	source_range const &integer_10_expression_tree::value() const
	{
		return m_value;
	}

	void integer_10_expression_tree::accept(expression_tree_visitor &visitor) const
	{
		visitor.visit(*this);
	}


	call_expression_tree::call_expression_tree(
		std::unique_ptr<expression_tree> function,
		expression_vector arguments
		)
		: m_function(std::move(function))
		, m_arguments(std::move(arguments))
	{
	}

	void call_expression_tree::accept(expression_tree_visitor &visitor) const
	{
		visitor.visit(*this);
	}

	expression_tree const &call_expression_tree::function() const
	{
		return *m_function;
	}

	call_expression_tree::expression_vector const &call_expression_tree::arguments() const
	{
		return m_arguments;
	}


	statement_tree::~statement_tree()
	{
	}


	declaration_tree::declaration_tree(
		std::string name,
		std::unique_ptr<expression_tree> value
		)
		: m_name(std::move(name))
		, m_value(std::move(value))
	{
	}

	void declaration_tree::accept(statement_tree_visitor &visitor) const
	{
		visitor.visit(*this);
	}

	std::string const &declaration_tree::name() const
	{
		return m_name;
	}

	expression_tree const &declaration_tree::value() const
	{
		return *m_value;
	}


	return_tree::return_tree(
		std::unique_ptr<expression_tree> value
		)
		: m_value(std::move(value))
	{
	}

	void return_tree::accept(statement_tree_visitor &visitor) const
	{
		visitor.visit(*this);
	}

	expression_tree const &return_tree::value() const
	{
		return *m_value;
	}


	function_tree::function_tree(
		statements body
		)
		: m_body(std::move(body))
	{
	}

	function_tree::function_tree(function_tree &&other)
		: m_body(std::move(other.m_body))
	{
	}

	function_tree &function_tree::operator = (function_tree &&other)
	{
		function_tree(std::move(other)).swap(*this);
		return *this;
	}

	void function_tree::swap(function_tree &other)
	{
		m_body.swap(other.m_body);
	}

	function_tree::statements const &function_tree::body() const
	{
		return m_body;
	}
}
