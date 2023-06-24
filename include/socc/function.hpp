#ifndef SOCC_FUNCTION_HPP
#define SOCC_FUNCTION_HPP

#include "statement.hpp"

namespace SOCC{
/*
int16 sub_function(int8 arg1, int16 arg2, int8 arg3);

|    ....       | high address
+---------------+
|     high      |    new BP + 9
+ return value  +
|     low       |    new BP + 8
+---------------+
| arg1          |    new BP + 7
+---------------+
|     high      |    new BP + 6
+ arg2          +
|     low       |    new BP + 5
+---------------+
| arg3          |    new BP + 4
+---------------+
|     high      |    new BP + 3
+return address +
|     low       |    new BP + 2
+---------------+
|     high      |    new BP + 1
+ old BP        +
|     low       | <- new BP
+---------------+
| local var 1   |    new BP - 1
+---------------+
| local var 2   |    new BP - 2
+---------------+
|    ....       |  low address

*/
	template<typename FnType> struct FnBase;
	template<typename Ret,typename ...Args>
	struct FnBase<Ret(Args...)>:Block,Allocator{
		static constexpr ssize_t ret_size=Ret::size;
		static constexpr ssize_t arg_size=(0+...+Args::size);
		ssize_t local_size;
		Ret ret;
		std::tuple<Args...> args;
		explicit FnBase(ssize_t ret_start,ssize_t arg_start):
				local_size(-arg_start),
				ret{LocalVar::make(Ret::size,ret_start)},
				args{vars<Args...>()}
		{local_size=0;}
		explicit FnBase(const std::string& name,ssize_t ret_start,ssize_t arg_start):
				FnBase(ret_start,arg_start)
		{Label::tbl[name]=start;}
		std::shared_ptr<MemVar> alloc(size_t size) override {
			local_size+=size;
			return LocalVar::make(size,-local_size);
		}
	};

	template<typename FnType> struct Fn;
	template<typename Ret,typename ...Args>
	struct Fn<Ret(Args...)>:FnBase<Ret(Args...)>{
		using This = Fn<Ret(Args...)>;
		using Base = FnBase<Ret(Args...)>;
		static constexpr ssize_t ret_start=Base::arg_size+4;
		static constexpr ssize_t arg_start=Base::arg_size+4;
		explicit Fn(const std::string& name=""):Base{name,ret_start,arg_start}{}

		Ret operator()(const Args&... args_) const{
			Code code{};
			code.add(adj(-Base::ret_size));
			if constexpr (sizeof...(Args)>0){
				(code.add(args_),...);
			}
			code.add(call(this->start))
			    .add(adj(Base::arg_size));
			return Ret{expr(code)};
		}
		This& impl(const Stmt& stmt){
			this->body.add(asm_({enter(this->local_size), stmt}));
			return *this;
		}

		template<typename F>requires std::is_invocable_r_v<Stmt , F, This&, Args...>
		Block impl(F&& fn){
			auto body=std::apply(fn,std::tuple_cat(std::forward_as_tuple(*this),this->args));
			return impl(body);
		}
		inline void_ return_(Ret value){return asm_({this->ret.set(value), leave()});}
		inline void_ return_(){return asm_(leave());}

		explicit Fn(const Stmt& stmt){impl(stmt);}
		template<typename F>requires std::is_invocable_r_v<Stmt , F, This&, Args...>
		explicit Fn(F&& fn):Base{ret_start,arg_start}{impl(fn);}
		auto operator&(){
			return ptr<This>(expr(push(this->start)));
		}
	};

	template<typename Ret,typename ...Args>
	struct ptr<Fn<Ret(Args...)>>: Int<SOCC_PTR_SIZE,false>{
		DEF_TYPE(ptr,(ptr<Fn<Ret(Args...)>>),(Int<SOCC_PTR_SIZE,false>)) // NOLINT(google-explicit-constructor)

		explicit ptr(const usize& v): Base(v.value){}
		template<typename U>
		explicit ptr(const ptr<U>& v):Base(v.value){}
		explicit ptr(const Label& v):Base(Raw::make(to_data(v))){}

		using type=Fn<Ret(Args...)>;
		Ret operator()(const Args&... args_) const{
			Code code{};
			code.add(adj(-type::ret_size));
			if constexpr (sizeof...(Args)>0){
				(code.add(args_),...);
			}
			code.add(value->load())
			    .add(call_ptr())
			    .add(adj(type::arg_size));
			return Ret{expr(code)};
		}
	};
/*

int16 sub_function(int8 arg1, int16 arg2, int8 arg3);

|    ....       | high address
+---------------+---------------+
| arg1          |     high      |    new BP + 5
+---------------+ return value  +
|     high      |     low       |    new BP + 4
+ arg2          +---------------+
|     low       |    new BP + 3
+---------------+
| arg3          |    new BP + 2
+---------------+
|     high      |    new BP + 1
+ old BP        +
|     low       | <- new BP
+---------------+
| local var 1   |    new BP - 1
+---------------+
| local var 2   |    new BP - 2
+---------------+
|    ....       |  low address

*/
	template<typename FnType> struct InplaceFn;
	template<typename Ret,typename ...Args>
	struct InplaceFn<Ret(Args...)>:FnBase<Ret(Args...)>{
		using This = InplaceFn<Ret(Args...)>;
		using Base = FnBase<Ret(Args...)>;
		static constexpr ssize_t ret_start=Base::arg_size+2-Base::ret_size;
		static constexpr ssize_t arg_start=Base::arg_size+2;
		Label end_;

		template<typename F>requires std::is_invocable_r_v<Stmt , F, This&, Args...>
		This& impl(F&& fn){
			auto stmt=std::apply(fn,std::tuple_cat(std::forward_as_tuple(*this),this->args));
			this->body.add(asm_({
				enter(this->local_size),
				stmt,
				end_,
				lev(),
				adj(Base::arg_size-Base::ret_size),
			}));
			return *this;
		}
		inline void_ return_(Ret value){return asm_({this->ret.set(value), jmp(end_)});}
		inline void_ return_(){return asm_(jmp(end_));}

		template<typename F>requires std::is_invocable_r_v<Stmt , F, This&, Args...>
		explicit InplaceFn(F&& fn):Base{ret_start,arg_start}{impl(fn);}
	};
}
#endif //SOCC_FUNCTION_HPP
