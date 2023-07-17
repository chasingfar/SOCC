#ifndef SOCC_LIBRARY_HPP
#define SOCC_LIBRARY_HPP

#include "operator.hpp"
#include <unordered_map>

namespace SOCC{
	struct Lib{
		std::unordered_map<void*,Code> fns{};
		void clear(){
			fns.clear();
		}
		[[nodiscard]] size_t size() const{
			return fns.size();
		}
		void add(void* fn,Code impl){
			fns.try_emplace(fn,impl);
		}
		[[nodiscard]] Code to_code() const{
			Code code{};
			for (const auto&[fn, impl] : fns) {
				code.add(impl);
			}
			return code;
		}
		static Lib lib;
	};

	template<typename FnType>struct LibFn;
	template<typename Ret,typename ...Args>
	struct LibFn<Ret(Args...)>{
		Fn<Ret(Args...)> fn;
		std::function<Stmt(Fn<Ret(Args...)>&, Args...)> impl;
		template<typename F>requires std::is_invocable_r_v<Stmt , F, Fn<Ret(Args...)>&, Args...>
		explicit LibFn(F&& f):impl(f){}
		Ret operator()(const Args&... args_){
			Lib::lib.add(std::addressof(fn),fn.impl(impl).to_code());
			return fn(args_...);
		}
	};

	template<size_t Size,bool Signed>
	inline static LibFn<Int<Size,Signed>(Int<Size,Signed>,Int<Size,Signed>)> mul{
		[](auto& _,const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs)->Stmt{
			u8 rs1{rhs.as_mem_var()->shift(0,1)};
			auto ret_value=_.ret.as_mem_var();
			return {
				([&]<size_t ...I>(std::index_sequence<I...>)->Stmt{
					if constexpr(Signed){
						i8 ret[Size]{i8{ret_value->shift(I, 1)}...};
						return {ret[I].set(0_i8)...};
					}else{
						u8 ret[Size]{u8{ret_value->shift(I, 1)}...};
						return {ret[I].set(0_u8)...};
					}
				})(std::make_index_sequence<Size>{}),
				while_(rhs).do_({
					if_(rs1 & 1_u8).then({
						_.ret += lhs,
					}).end(),
					lhs <<= 1,
					rhs >>= 1,
				}),
				_.return_(),
			};
		}
	};
	template<size_t Size,bool Signed>
	inline auto operator*(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return mul<Size,Signed>(lhs,rhs);
	}
}
#endif //SOCC_LIBRARY_HPP
