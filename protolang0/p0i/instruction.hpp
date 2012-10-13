#pragma once
#ifndef P0I_INSTRUCTION_HPP
#define P0I_INSTRUCTION_HPP


#include <array>
#include <cstdint>


namespace p0
{
	namespace intermediate
	{
		namespace instruction_type
		{
			enum Enum
			{
				nothing,
				return_,
				allocate,
				deallocate,
				set_from_constant,
				set_null,
				set_function,
				copy,
				add,
				mul,
				div,
				negate,
				not_,
				and_,
				or_,
				xor_,
				equal,
				less,
				greater,
				call,

				count_,
			};
		};

		typedef std::uint64_t instruction_argument;


		struct instruction_info
		{
			char const *name;
			size_t argument_count;
		};

		instruction_info const &get_instruction_info(instruction_type::Enum instruction);


		struct instruction
		{
			enum
			{
				max_arguments = 2,
			};

			typedef std::array<instruction_argument, max_arguments> argument_array;


			instruction();
			explicit instruction(
				instruction_type::Enum type,
				argument_array const &arguments = argument_array()
				);
			explicit instruction(
				instruction_type::Enum type,
				instruction_argument argument0
				);
			explicit instruction(
				instruction_type::Enum type,
				instruction_argument argument0,
				instruction_argument argument1
				);
			instruction_type::Enum type() const;
			argument_array const &arguments() const;

		private:

			instruction_type::Enum m_type;
			argument_array m_arguments;
		};
	}
}


#endif
