#ifndef SOCC_OPERATOR_HPP
#define SOCC_OPERATOR_HPP
#include "function.hpp"

namespace SOCC {
	template<size_t Size,bool Signed,auto fn,auto fnc=fn>
	inline Int<Size,Signed> calc_(const Int<Size,Signed>& lhs) {
		Code tmp{lhs};
		if constexpr (Size > 1) {
			tmp.add([]<size_t ...I>(std::index_sequence<I...>){
				return InplaceFn<Int<Size,Signed>(Array<Int<1,Signed>,Size>)>{
					[](auto& _,Array<Int<1,Signed>,Size> ls)->Stmt{
						return {
							ls[I].set(calc_<1,Signed,I==0?fn:fnc,fnc>(ls[I]))...
						};
					}
				}.to_code();
			}(std::make_index_sequence<Size>{}));
		} else if constexpr (Size == 1) {
			tmp.add(fn());
		}
		return Int<Size,Signed>{expr(tmp)};
	}

	template<size_t Size,bool Signed,auto fn,auto fnc=fn>
	inline Int<Size,Signed> calc_(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {
		Code tmp{lhs};
		tmp.add(rhs.to_code());
		if constexpr (Size > 1) {
			tmp.add([]<size_t ...I>(std::index_sequence<I...>){
				return InplaceFn<Int<Size,Signed>(Array<Int<1,Signed>,Size>,Array<Int<1,Signed>,Size>)>{
					[](auto& _,Array<Int<1,Signed>,Size> ls,Array<Int<1,Signed>,Size> rs)->Stmt{
						return {
							ls[I].set(calc_<1,Signed,I==0?fn:fnc,fnc>(ls[I],rs[I]))...
						};
					}
				}.to_code();
			}(std::make_index_sequence<Size>{}));
		} else if constexpr (Size == 1) {
			tmp.add(fn());
		}
		return Int<Size,Signed>{expr(tmp)};
	}

	template<size_t Size,bool Signed> inline auto shl(const Int<Size,Signed>& lhs)                             {return calc_<Size,Signed,Instrs::shl,Instrs::rcl>(lhs);}
	template<size_t Size,bool Signed> inline auto rcl(const Int<Size,Signed>& lhs)                             {return calc_<Size,Signed,Instrs::rcl,Instrs::rcl>(lhs);}
	template<size_t Size,bool Signed> inline auto shr(const Int<Size,Signed>& lhs)                             {return calc_<Size,Signed,Instrs::shr,Instrs::rcr>(lhs);}
	template<size_t Size,bool Signed> inline auto rcr(const Int<Size,Signed>& lhs)                             {return calc_<Size,Signed,Instrs::rcr,Instrs::rcr>(lhs);}
	template<size_t Size,bool Signed> inline auto add(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Instrs::add,Instrs::adc>(lhs,rhs);}
	template<size_t Size,bool Signed> inline auto adc(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Instrs::adc,Instrs::adc>(lhs,rhs);}
	template<size_t Size,bool Signed> inline auto sub(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Instrs::sub,Instrs::suc>(lhs,rhs);}
	template<size_t Size,bool Signed> inline auto suc(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Instrs::suc,Instrs::suc>(lhs,rhs);}

	template<size_t Size,bool Signed> inline auto NOT(const Int<Size,Signed>& lhs)                             {return calc_<Size,Signed,Instrs::NOT,Instrs::NOT>(lhs);}
	template<size_t Size,bool Signed> inline auto AND(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Instrs::AND,Instrs::AND>(lhs,rhs);}
	template<size_t Size,bool Signed> inline auto  OR(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Instrs:: OR,Instrs:: OR>(lhs,rhs);}
	template<size_t Size,bool Signed> inline auto XOR(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Instrs::XOR,Instrs::XOR>(lhs,rhs);}

	template<typename To,typename From>
	concept CanCastTo = requires(From from) {TypeCaster<To,From>::to(from);};
	template<typename To,typename From> requires CanCastTo<To, From>
	inline static auto to(From from){return TypeCaster<To,From>::to(from);}

	template<size_t Size,bool Signed>
	inline auto operator!(const Int<Size,Signed>& lhs){
		return if_<bool_>(lhs).then(Val::false_).else_(Val::true_);
	}
	template<size_t Size,bool Signed>
	inline auto operator!=(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return ((bool_)lhs - rhs);
	}
	template<size_t Size,bool Signed>
	inline auto operator==(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return !(lhs!=rhs);
	}

	template<size_t Size,bool Signed>
	inline auto operator>=(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return bool_{expr({(void_)(lhs - rhs),pushCF()})};
	}
	template<size_t Size,bool Signed>
	inline auto operator<(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return !(lhs>=rhs);
	}
	template<size_t Size,bool Signed>
	inline auto operator<=(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return rhs>=lhs;
	}
	template<size_t Size,bool Signed>
	inline auto operator>(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return rhs<lhs;
	}

	template<size_t Size,bool Signed>
	inline auto operator+(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return add(lhs,rhs);
	}
	template<size_t Size,bool Signed>
	inline auto operator-(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return sub(lhs,rhs);
	}
	template<size_t Size,bool Signed>
	inline auto operator-(const Int<Size,Signed>& lhs){
		if(auto l=lhs.to_int();l){
			return Int<Size,Signed>(-(*l));
		}
		if constexpr(Signed){
			return sub(Int<Size,Signed>(0ll),lhs);
		}else{
			return sub(Int<Size,Signed>(0ull),lhs);
		}
	}
	template<size_t Size,bool Signed>
	inline auto operator<<(const Int<Size,Signed>& lhs,size_t n){
		Int<Size,Signed> tmp{lhs.value};
		for (size_t i = 0; i < n; ++i) {
			tmp.value=shl(tmp).value;
		}
		return tmp;
	}
	template<size_t Size,bool Signed>
	inline auto operator>>(const Int<Size,Signed>& lhs,size_t n){
		Int<Size,Signed> tmp{lhs.value};
		for (size_t i = 0; i < n; ++i) {
			tmp.value=shr(tmp).value;
		}
		return tmp;
	}
	template<size_t Size,bool Signed>
	inline auto operator&(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return AND(lhs,rhs);
	}
	template<size_t Size,bool Signed>
	inline auto operator|(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return OR(lhs,rhs);
	}
	template<size_t Size,bool Signed>
	inline auto operator^(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return XOR(lhs,rhs);
	}
	template<size_t Size,bool Signed>
	inline auto operator~(const Int<Size,Signed>& lhs){
		return NOT(lhs);
	}

	template<typename T,typename U> requires std::is_convertible_v<U,T>
	inline auto operator+=(const T& var,const U&value){
		return var.set(var+value);
	}
	template<typename T,typename U> requires std::is_convertible_v<U,T>
	inline auto operator-=(const T& var,const U&value){
		return var.set(var-value);
	}
	template<typename T>
	inline auto operator<<=(const T& var,size_t value){
		return var.set(var<<value);
	}
	template<typename T>
	inline auto operator>>=(const T& var,size_t value){
		return var.set(var>>value);
	}
	template<typename T,typename U> requires std::is_convertible_v<U,T>
	inline auto operator&=(const T& var,const U&value){
		return var.set(var&value);
	}
	template<typename T,typename U> requires std::is_convertible_v<U,T>
	inline auto operator|=(const T& var,const U&value){
		return var.set(var|value);
	}
	template<typename T,typename U> requires std::is_convertible_v<U,T>
	inline auto operator^=(const T& var,const U&value){
		return var.set(var^value);
	}

	template<size_t Size,bool Signed>
	inline auto operator*(const Int<Size,Signed>& lhs,size_t rhs){
		std::vector<Int<Size,Signed>> res;
		size_t i=0;
		while(rhs!=0){
			if(rhs&1u){
				res.push_back(lhs<<i);
			}
			++i;
			rhs>>=1;
		}
		if(res.empty()){
			return Int<Size,Signed>{0ull};
		}
		auto sum=res[0];
		for(auto it=res.begin()+1; it!=res.end(); ++it) {
			sum.value=(sum + *it).value;
		}
		return sum;
	}
	template<typename T>
	inline ptr<T> operator+(const ptr<T>& p,const Int<SOCC_PTR_SIZE,false>& v){
		return add(p,v*T::size);
	}
	template<typename T> requires std::is_base_of_v<Type<T::size>,T>
	inline auto operator&(const T& v){
		auto var=v.as_mem_var();
		u16 ref{expr(var->get_ref())},off{var->offset};
		return ptr<T>(ref+off);
	}
}

#endif //SOCC_OPERATOR_HPP
