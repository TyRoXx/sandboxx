#pragma once
#ifndef P0C_LVALUE_GENERATOR_HPP
#define P0C_LVALUE_GENERATOR_HPP


#include "expression_tree.hpp"
#include "reference.hpp"
#include "p0i/emitter.hpp"


namespace p0
{
	struct local_frame;
	struct code_generator;


	struct lvalue_generator : expression_tree_visitor
	{
		explicit lvalue_generator(
			code_generator &function_generator,
			intermediate::emitter &emitter,
			local_frame &frame
			);
		reference address() const;

	private:

		code_generator &m_function_generator;
		intermediate::emitter &m_emitter;
		local_frame &m_frame;
		reference m_address;


		virtual void visit(name_expression_tree const &expression) override;
		virtual void visit(integer_10_expression_tree const &expression) override;
		virtual void visit(call_expression_tree const &expression) override;
		virtual void visit(function_tree const &expression) override;
		virtual void visit(null_expression_tree const &expression) override;
		virtual void visit(table_expression const &expression) override;
	};
}


#endif
