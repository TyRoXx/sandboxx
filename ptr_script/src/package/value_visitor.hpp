#ifndef VALUE_VISITOR_HPP_INCLUDED_HJOJT2RLVND6OVIB
#define VALUE_VISITOR_HPP_INCLUDED_HJOJT2RLVND6OVIB


namespace ptrs
{
	struct local;
	struct element_ptr;
	struct literal;


	struct value_visitor
	{
		virtual ~value_visitor();
		virtual void visit(const local &value);
		virtual void visit(const element_ptr &value);
		virtual void visit(const literal &value);
	};
}


#endif
