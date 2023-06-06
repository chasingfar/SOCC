#ifndef SOCC_LIBRARY_HPP
#define SOCC_LIBRARY_HPP

#include "operator.hpp"
#include <unordered_map>

namespace SOCC::Library{
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
			Label end;
			Code code{};
			for (const auto&[fn, impl] : fns) {
				code.add(impl);
			}
			return {jmp(end),code,end};
		}
	};
	static Lib stdlib;

	template<size_t Size,bool Signed>
	inline Int<Size,Signed> mul(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		using int_t=Int<Size,Signed>;
		static Fn<int_t(int_t,int_t)> fn{
			[](auto& _,const int_t& ls,const int_t& rs)->Stmt{
				u8 rs1{rs.as_mem_var()->shift(0,1)};
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
					while_(rs).do_({
						if_(rs1 & 1_u8).then({
							_.ret += ls,
						}).end(),
						ls <<= 1,
						rs >>= 1,
					}).end(),
					_.return_(),
				};
			}
		};
		stdlib.add(std::addressof(fn),fn.to_code());
		return fn(lhs,rhs);
	}
	template<size_t Size,bool Signed>
	inline auto operator*(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return mul(lhs,rhs);
	}
}
#endif //SOCC_LIBRARY_HPP
