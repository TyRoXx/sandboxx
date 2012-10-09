#include "code_generator.hpp"
#include "function_tree.hpp"


namespace p0
{
	code_generator::code_generator(
		function_tree const &tree
		)
		: m_tree(tree)
	{
	}

	intermediate::unit code_generator::generate_unit()
	{
		intermediate::unit::function_vector functions;

		return intermediate::unit(
			std::move(functions)
			);
	}
}
