#ifndef NEW_LANG_ANALYZE_HPP
#define NEW_LANG_ANALYZE_HPP

#include "ast.hpp"
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/lexical_cast.hpp>

namespace nl
{
	namespace il
	{
		struct map;
		struct signature;

		struct external
		{
			void const *payload;
		};

		inline bool operator == (external const &left, external const &right)
		{
			return (left.payload == right.payload);
		}

		inline std::size_t hash_value(external const &value)
		{
			return boost::hash_value(value.payload);
		}

		struct integer
		{
			std::string value;
		};

		inline bool operator == (integer const &left, integer const &right)
		{
			return (left.value == right.value);
		}

		inline std::size_t hash_value(integer const &value)
		{
			return boost::hash_value(value.value);
		}

		struct string
		{
			std::string value;
		};

		inline bool operator == (string const &left, string const &right)
		{
			return (left.value == right.value);
		}

		inline std::size_t hash_value(string const &value)
		{
			return boost::hash_value(value.value);
		}

		struct null
		{
		};

		inline bool operator == (null, null)
		{
			return true;
		}

		inline std::size_t hash_value(null const &)
		{
			return boost::hash_value(true);
		}

		typedef boost::variant<null, boost::recursive_wrapper<map>, boost::recursive_wrapper<signature>, external, integer, string> value;
		typedef value type;

		struct map
		{
			boost::unordered_map<value, value> elements;
		};

		inline bool operator == (map const &left, map const &right)
		{
			return (left.elements == right.elements);
		}

		inline std::size_t hash_value(map const &value)
		{
			std::size_t digest = 0;
			for (auto &entry : value.elements)
			{
				boost::hash_combine(digest, entry);
			}
			return digest;
		}

		struct signature
		{
			type result;
			std::vector<type> parameters;
		};

		inline bool operator == (signature const &left, signature const &right)
		{
			return
					(left.result == right.result) &&
					(left.parameters == right.parameters);
		}

		inline std::size_t hash_value(signature const &value)
		{
			std::size_t digest = 0;
			boost::hash_combine(digest, value.result);
			boost::hash_combine(digest, value.parameters);
			return digest;
		}

		struct constant_expression
		{
			value constant;
		};

		struct make_closure;
		struct subscript;
		struct call;
		struct definition_expression;

		typedef boost::variant<
			constant_expression,
			boost::recursive_wrapper<make_closure>,
			boost::recursive_wrapper<subscript>,
			boost::recursive_wrapper<call>,
			boost::recursive_wrapper<definition_expression>
		> expression;

		struct definition_expression
		{
			std::string name;
			il::type type;
			boost::optional<value> const_value;
			std::size_t hops;
		};

		bool operator == (expression const &left, expression const &right);

		struct parameter
		{
			il::type type;
			std::string name;
		};

		struct definition
		{
			std::string name;
			expression value;
		};

		struct block
		{
			std::vector<definition> definitions;
			expression result;
		};

		struct make_closure
		{
			std::vector<parameter> parameters;
			block body;
		};

		struct subscript
		{
			expression left;
			std::string element;
		};

		struct call
		{
			expression function;
			std::vector<expression> arguments;
		};

		struct name_space_entry
		{
			il::type type;
			boost::optional<value> const_value;
		};

		struct name_space
		{
			name_space const *next;
			boost::unordered_map<std::string, name_space_entry> definitions;
		};

		inline boost::optional<definition_expression> resolve_name(name_space const &leaf, std::string const &name)
		{
			name_space const *n = &leaf;
			std::size_t hops = 0;
			while (n)
			{
				auto definition = n->definitions.find(name);
				if (definition != end(n->definitions))
				{
					return definition_expression{name, definition->second.type, definition->second.const_value, hops};
				}
				n = n->next;
				++hops;
			}
			return boost::none;
		}

		type type_of_expression(expression const &expr);

		struct subscription_visitor : boost::static_visitor<boost::optional<type>>
		{
			explicit subscription_visitor(std::string element)
				: m_element(std::move(element))
			{
			}

			boost::optional<type> operator()(map const &m) const
			{
				auto e = m.elements.find(string{m_element});
				if  (e == end(m.elements))
				{
					return boost::none;
				}
				return e->second;
			}

			template <class Rest>
			boost::optional<type> operator()(Rest const &) const
			{
				return boost::none;
			}

		private:

			std::string m_element;
		};

		struct expression_type_visitor : boost::static_visitor<type>
		{
			type operator()(constant_expression const &) const
			{
				return null();
			}

			type operator()(make_closure const &closure) const
			{
				signature sig;
				sig.result = type_of_expression(closure.body.result);
				for (parameter const &param : closure.parameters)
				{
					sig.parameters.emplace_back(param.type);
				}
				return std::move(sig);
			}

			type operator()(subscript const &expr) const
			{
				auto left = type_of_expression(expr.left);
				auto element_type = boost::apply_visitor(subscription_visitor{expr.element}, left);
				if (!element_type)
				{
					throw std::runtime_error("Cannot deduce type because element does not exist: " + expr.element);
				}
				return std::move(*element_type);
			}

			type operator()(call const &expr) const
			{
				throw std::logic_error("not implemented");
			}

			type operator()(definition_expression const &expr) const
			{
				return expr.type;
			}
		};

		inline type type_of_expression(expression const &expr)
		{
			return boost::apply_visitor(expression_type_visitor{}, expr);
		}

		inline bool is_callable(expression const &function, std::vector<expression> const &arguments)
		{
			type function_type = type_of_expression(function);
			signature const * const sig = boost::get<signature>(&function_type);
			if (!sig)
			{
				throw std::runtime_error("The expression does not evaluate to something callable");
			}
			std::vector<type> argument_types;
			std::transform(begin(arguments), end(arguments), std::back_inserter(argument_types), type_of_expression);
			return (sig->parameters == argument_types);
		}

		expression analyze(ast::expression const &syntax, name_space const &names);

		value evaluate_const(expression const &expr);

		struct const_expression_evaluator : boost::static_visitor<value>
		{
			value operator()(constant_expression const &expr) const
			{
				return expr.constant;
			}

			value operator()(make_closure const &) const
			{
				throw std::runtime_error("A closure is not a compile-time value");
			}

			value operator()(subscript const &expr) const
			{
				auto left = evaluate_const(expr.left);
				throw std::logic_error("not implemented");
			}

			value operator()(call const &) const
			{
				throw std::runtime_error("Functions cannot be evaluated at compile-time");
			}

			value operator()(definition_expression const &expr) const
			{
				if (!expr.const_value)
				{
					throw std::runtime_error("This definition cannot be evaluated at compile-time: " + expr.name);
				}
				return *expr.const_value;
			}
		};

		inline value evaluate_const(expression const &expr)
		{
			return boost::apply_visitor(const_expression_evaluator{}, expr);
		}

		struct expression_analyzer : boost::static_visitor<expression>
		{
			explicit expression_analyzer(name_space const &names)
				: m_names(names)
			{
			}

			expression operator()(ast::integer const &syntax) const
			{
				return constant_expression{integer{syntax.position.content}};
			}

			expression operator()(ast::string const &syntax) const
			{
				return constant_expression{string{syntax.position.content}};
			}

			expression operator()(ast::identifier const &syntax) const
			{
				auto expr = resolve_name(m_names, syntax.position.content);
				if (!expr)
				{
					throw std::runtime_error("Unknown identifier " + syntax.position.content);
				}
				return std::move(*expr);
			}

			expression operator()(ast::lambda const &syntax) const
			{
				name_space locals{&m_names, {}};
				std::vector<parameter> parameters;
				for (ast::parameter const &parameter_syntax : syntax.parameters)
				{
					auto type_expr = analyze(parameter_syntax.type, locals);
					auto type = evaluate_const(type_expr);
					parameters.emplace_back(parameter{type, parameter_syntax.name.content});
					name_space_entry entry{type, boost::none};
					if (!locals.definitions.insert(std::make_pair(parameter_syntax.name.content, entry)).second)
					{
						throw std::runtime_error("Cannot redefine " + parameter_syntax.name.content);
					}
				}
				block body;
				for (ast::definition const &definition_syntax : syntax.body.elements)
				{
					auto value = analyze(definition_syntax.value, m_names);
					body.definitions.emplace_back(definition{definition_syntax.name.content, value});
					name_space_entry entry{type_of_expression(value), boost::none};
					try
					{
						entry.const_value = evaluate_const(value);
					}
					catch (std::runtime_error const &) //TODO
					{
					}
					if (!locals.definitions.insert(std::make_pair(definition_syntax.name.content, entry)).second)
					{
						throw std::runtime_error("Cannot redefine " + definition_syntax.name.content);
					}
				}
				body.result = analyze(syntax.body.result, locals);
				return make_closure{std::move(parameters), std::move(body)};
			}

			expression operator()(ast::subscript const &syntax) const
			{
				auto left = analyze(syntax.left, m_names);
				return subscript{std::move(left), syntax.element.content};
			}

			expression operator()(ast::call const &syntax) const
			{
				auto function = analyze(syntax.function, m_names);
				std::vector<expression> arguments;
				for (auto &argument : syntax.arguments)
				{
					arguments.emplace_back(analyze(argument, m_names));
				}
				if (!is_callable(function, arguments))
				{
					throw std::runtime_error("Argument type mismatch");
				}
				return call{std::move(function), std::move(arguments)};
			}

		private:

			name_space const &m_names;
		};

		inline expression analyze(ast::expression const &syntax, name_space const &names)
		{
			return boost::apply_visitor(expression_analyzer{names}, syntax);
		}

		void print(Si::sink<char> &sink, value const &v);
		void print(Si::sink<char> &sink, expression const &expr);

		struct value_printer : boost::static_visitor<>
		{
			explicit value_printer(Si::sink<char> &out)
				: m_out(out)
			{
			}

			void operator()(null) const
			{
				Si::append(m_out, "-null-");
			}

			void operator()(map const &value) const
			{
				Si::append(m_out, "-map-[\n");
				for (auto &elem : value.elements)
				{
					print(m_out, elem.first);
					Si::append(m_out, ": ");
					print(m_out, elem.second);
					Si::append(m_out, "\n");
				}
				Si::append(m_out, "]");
			}

			void operator()(signature const &value) const
			{
				Si::append(m_out, "-signature-(");
				for (auto &param : value.parameters)
				{
					print(m_out, param);
					Si::append(m_out, ", ");
				}
				Si::append(m_out, ")->");
				print(m_out, value.result);
			}

			void operator()(external const &value) const
			{
				Si::append(m_out, boost::lexical_cast<std::string>(value.payload));
			}

			void operator()(integer const &value) const
			{
				Si::append(m_out, value.value);
			}

			void operator()(string const &value) const
			{
				Si::append(m_out, value.value);
			}

		private:

			Si::sink<char> &m_out;
		};

		inline void print(Si::sink<char> &sink, value const &v)
		{
			return boost::apply_visitor(value_printer{sink}, v);
		}

		struct expression_printer : boost::static_visitor<>
		{
			explicit expression_printer(Si::sink<char> &out)
				: m_out(out)
			{
			}

			void operator()(constant_expression const &expr) const
			{
				print(m_out, expr.constant);
			}

			void operator()(make_closure const &expr) const
			{
				Si::append(m_out, "(");
				for (parameter const &param : expr.parameters)
				{
					print(m_out, param.type);
					Si::append(m_out, " ");
					Si::append(m_out, param.name);
					Si::append(m_out, ", ");
				}
				Si::append(m_out, ")\n");
				for (definition const &def : expr.body.definitions)
				{
					Si::append(m_out, def.name);
					Si::append(m_out, " = ");
					print(m_out, def.value);
					Si::append(m_out, "\n");
				}
				print(m_out, expr.body.result);
				Si::append(m_out, "\n");
			}

			void operator()(subscript const &expr) const
			{
				print(m_out, expr.left);
				Si::append(m_out, ".");
				Si::append(m_out, expr.element);
			}

			void operator()(call const &expr) const
			{
				print(m_out, expr.function);
				Si::append(m_out, "(...)");
			}

			void operator()(definition_expression const &expr) const
			{
				Si::append(m_out, expr.name);
				Si::append(m_out, "(");
				Si::append(m_out, boost::lexical_cast<std::string>(expr.hops));
				Si::append(m_out, ")");
			}

		private:

			Si::sink<char> &m_out;
		};

		inline void print(Si::sink<char> &sink, expression const &expr)
		{
			return boost::apply_visitor(expression_printer{sink}, expr);
		}

		inline bool operator == (expression const &left, expression const &right)
		{
			std::string left_str, right_str;
			auto left_sink = Si::make_container_sink(left_str);
			auto right_sink = Si::make_container_sink(right_str);
			print(left_sink, left);
			print(right_sink, right);
			return (left_str == right_str);
		}

		inline std::ostream &operator << (std::ostream &out, expression const &expr)
		{
			Si::ostream_ref_sink sink(out);
			print(sink, expr);
			return out;
		}
	}
}

#endif
