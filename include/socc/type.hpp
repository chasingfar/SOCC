#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-macro-parentheses"
#ifndef SOCC_TYPE_HPP
#define SOCC_TYPE_HPP

#include "var.hpp"
#include "util/macro_param.hpp"
#include <cstddef>
#include <memory>

#ifndef SOCC_PTR_SIZE
#define SOCC_PTR_SIZE 2
#endif

#define DEF_TYPE0 \
	using Base::Base; \
	auto operator =(const This& rhs) const {return this->set(rhs);}// NOLINT(bugprone-unhandled-self-assignment,misc-unconventional-assign-operator)
#define DEF_TYPE(NAME,THIS,BASE) \
	using This = Util::macro_param_t<void THIS>; \
	using Base = Util::macro_param_t<void BASE>; \
	DEF_TYPE0 \
	template<typename ...ARGs> requires std::is_constructible_v<Base,ARGs...> \
	explicit NAME(ARGs&&... args):Base{args...}{} \
	NAME(const Base& base):Base{base}{}
#define DEF_TYPE2(NAME,...) DEF_TYPE(NAME,(NAME),(__VA_ARGS__))

namespace SOCC {
	template<size_t Size>
	struct Type {
		static constexpr size_t size = Size;
		std::shared_ptr<Value> value{};
		Type():value(nullptr){}
		template<typename T> requires std::is_base_of_v<Value,T>
		explicit Type(std::shared_ptr<T> value):value(value){}
		explicit Type(Allocator& allocator):value(allocator.alloc(size)){}

		template<typename T>
		[[nodiscard]] auto as() const{return std::dynamic_pointer_cast<T>(value);}
		[[nodiscard]] auto as_raw() const{return as<Raw>();}
		[[nodiscard]] auto as_mem_var() const{return as<MemVar>();}
		[[nodiscard]] auto as_local_var() const{return as<LocalVar>();}
		[[nodiscard]] auto as_static_var() const{return as<StaticVar>();}

		[[nodiscard]] Type<0> set(const Type<Size>& rhs) const{
			return Type<0>{expr({rhs.value->load(), as<Var>()->save()})};
		}
		auto operator =(const Type<Size>& rhs) const{ // NOLINT(bugprone-unhandled-self-assignment,misc-unconventional-assign-operator)
			return set(rhs);
		}
		[[nodiscard]] Code to_code() const{
			return value->load();
		}
		[[nodiscard]] Code to_stmt() const{
			if constexpr(Size==0){
				return to_code();
			}
			return {to_code(),adj(size)};

		}
		explicit operator Type<0>(){
			return Type<0>{expr(to_stmt())};
		}
	};
	using void_ = Type<0>;
	inline auto asm_(const Code& code){return void_{expr(code)};}
	namespace Val{
		inline static const void_ none{expr({})};
	}

	template<typename To,typename From>
	struct TypeCaster{};
	template<typename To,typename From>
	struct SimpleCaster{
		static auto to(From from){
			return To{from.value};
		}
	};

	template<typename T,typename ...Ts>
	struct Struct:Type<(T::size+...+Ts::size)>{
		DEF_TYPE(Struct,(Struct<T,Ts...>),(Type<(T::size+...+Ts::size)>)) // NOLINT(google-explicit-constructor)

		static constexpr size_t count=1+sizeof...(Ts);
		template<size_t Index,size_t Offset=0>
		struct SubType:Struct<Ts...>::template SubType<Index-1,Offset+T::size>{};
		template<size_t Offset>
		struct SubType<0,Offset>:T{
			using type = T;
			static constexpr size_t offset=Offset;
		};

		inline static auto make(T val,Ts ...vals){
			if((val.as_raw()&&...&&vals.as_raw())){
				data_t tmp;
				tmp.reserve(Base::size);
				for(const auto& data:{val.as_raw()->data,vals.as_raw()->data...}){
					tmp.insert(tmp.end(),data.begin(),data.end());
				}
				return This{std::make_shared<Raw>(tmp)};
			}
			return This{expr({val, vals...})};
		}
		explicit Struct(T val,Ts ...vals):Base(make(val,vals...)){}

		auto extract(){
			return [&]<size_t... I>(std::index_sequence<I...>){
				return std::tuple{get<I>()...};
			}(std::make_index_sequence<count>{});
		}

		template<size_t I>
		auto get() const{
			using type = typename SubType<I>::type;
			if(auto raw=this->as_raw();raw){
				return type{raw->shift(SubType<I>::offset,type::size)};
			}else if(auto var=this->as_mem_var();var){
				return type{var->shift(SubType<I>::offset,type::size)};
			}else{
				return type{};
			}
		}
	};
	template<typename ...Ts>
	struct Union:Type<std::max({Ts::size...})>{
		DEF_TYPE(Union,(Union<Ts...>),(Type<std::max({Ts::size...})>)) // NOLINT(google-explicit-constructor)

		static constexpr size_t count=sizeof...(Ts);
		template<size_t Index>
		struct SubType:std::tuple_element_t<Index, std::tuple<Ts...>>{
			using type = std::tuple_element_t<Index, std::tuple<Ts...>>;
			static constexpr size_t offset=0;
		};
		template<typename V> requires std::disjunction_v<std::is_same<V, Ts>...>
		inline static auto make(const V& val){
			if(auto v=val.as_raw();v){
				data_t tmp(Base::size,data_t::value_type(static_cast<uint8_t>(0)));
				std::copy(v->data.begin(),v->data.end(),tmp.begin());
				return This{std::make_shared<Raw>(tmp)};
			}
			Code tmp{val};
			for (size_t i=0; i<std::max({Ts::size...})-V::size; ++i) {
				tmp.add(imm(0));
			}
			return This{expr(tmp)};
		}
		template<typename V> requires std::disjunction_v<std::is_same<V, Ts>...>
		explicit Union(const V& v):Base(make(v)){}

		auto extract(){
			return [&]<size_t... I>(std::index_sequence<I...>){
				return std::tuple{get<I>()...};
			}(std::make_index_sequence<count>{});
		}

		template<size_t I>
		auto get() const{
			using type = typename SubType<I>::type;
			if(auto raw=this->as_raw();raw){
				return type{raw->shift(0,type::size)};
			}else if(auto var=this->as_mem_var();var){
				return type{var->shift(0,type::size)};
			}else{
				return type{};
			}
		}
	};

	template<typename T,size_t N>
	struct Array:Type<N*T::size>{
		DEF_TYPE(Array,(Array<T,N>),(Type<N*T::size>)) // NOLINT(google-explicit-constructor)

		template<typename ...Ts>requires ((N==sizeof...(Ts))&&...&&std::is_same_v<T,Ts>)
		inline static auto make(Ts ...vals){
			if((vals.as_raw()&&...)){
				data_t tmp;
				tmp.reserve(Base::size);
				for(const auto& data:{vals.as_raw()->data...}){
					tmp.insert(tmp.end(),data.begin(),data.end());
				}
				return This{std::make_shared<Raw>(tmp)};
			}
			return This{expr({vals...})};
		}
		template<typename ...Ts>requires ((N==sizeof...(Ts))&&...&&std::is_same_v<T,Ts>)
		explicit Array(Ts ...vals):Base(make(vals...)){}

		auto operator[](size_t i){
			return T{this->as_mem_var()->shift(T::size*i,T::size)};
		}
	};

	template<size_t Size,bool Signed=false>
	struct Int:Type<Size>{
		DEF_TYPE(Int,(Int<Size,Signed>),(Type<Size>)) // NOLINT(google-explicit-constructor)

		explicit Int(std::integral auto val):Base(std::make_shared<Raw>(parse_int(val))){}
		static data_t parse_int(std::integral auto val){
			data_t tmp{};
			for (size_t i=0;i<Size;++i){
				tmp.emplace_back(static_cast<uint8_t>((val>>i*8)&0xFF));
			}
			return tmp;
		}
		[[nodiscard]] std::optional<std::conditional_t<Signed,intmax_t,uintmax_t>> to_int() const{
			std::conditional_t<Signed,intmax_t,uintmax_t> v=0;
			if(auto raw=this->as_raw();raw){
				for(auto data:raw->data){
					if(auto byte=std::get_if<uint8_t>(&data);byte){
						v<<=8;
						v|=*byte;
					}else{
						return {};
					}
				}
				return v;
			}
			return {};
		}
		template<size_t ...S> requires(Size==(S+...+0))
		inline static auto make(Int<S,Signed> ...vals){
			return This{expr({vals...})};
		}
	};
	template<typename T>
	using AsInt=Int<sizeof(T),std::is_signed_v<T>>;

	struct bool_:AsInt< uint8_t>{
		DEF_TYPE2(bool_,AsInt< uint8_t>) // NOLINT(google-explicit-constructor)

		template<size_t Size,bool Signed=false>
		explicit bool_(const Int<Size,Signed>& other){
			Code tmp{other};
			for (size_t i = 1; i < Size; ++i) {
				tmp.add(OR());
			}
			this->value=expr(tmp);
		}
	};
	struct   u8 :AsInt< uint8_t>{ DEF_TYPE2(  u8 ,AsInt< uint8_t>) }; // NOLINT(google-explicit-constructor)
	struct   i8 :AsInt<  int8_t>{ DEF_TYPE2(  i8 ,AsInt<  int8_t>) }; // NOLINT(google-explicit-constructor)
	struct   u16:AsInt<uint16_t>{ DEF_TYPE2(  u16,AsInt<uint16_t>) }; // NOLINT(google-explicit-constructor)
	struct   i16:AsInt< int16_t>{ DEF_TYPE2(  i16,AsInt< int16_t>) }; // NOLINT(google-explicit-constructor)
	using usize=u16;
	using isize=i16;

	namespace Val{
		inline static const bool_  true_{expr(imm(1))};
		inline static const bool_ false_{expr(imm(0))};
	}
	inline auto operator""_i8 (unsigned long long val){return i8{val};}
	inline auto operator""_u8 (unsigned long long val){return u8{val};}
	inline auto operator""_i16(unsigned long long val){return i16{val};}
	inline auto operator""_u16(unsigned long long val){return u16{val};}

	template<typename T>
	struct ptr:Int<SOCC_PTR_SIZE,false>{
		DEF_TYPE(ptr,(ptr<T>),(Int<SOCC_PTR_SIZE,false>)) // NOLINT(google-explicit-constructor)

		explicit ptr(const usize& v): Base(v.value){}
		template<typename U>
		explicit ptr(const ptr<U>& v):Base(v.value){}
		explicit ptr(const Label& v):Base(Raw::make(to_data(v))){}

		using type=T;
		auto operator*(){
			return T{PtrVar::make(T::size,value->load(),0)};
		}
	};
	template<typename T>struct UnPtr        {};
	template<typename T>struct UnPtr<ptr<T>>{using type = T;};
	template<typename T>concept IsPtr = requires {typename UnPtr<T>::type;};

	struct char_:AsInt< uint8_t>{ DEF_TYPE2(char_,AsInt< uint8_t>) }; // NOLINT(google-explicit-constructor)
	struct cstring:ptr<char_>{ DEF_TYPE2(cstring,ptr<char_>) }; // NOLINT(google-explicit-constructor)
	inline auto operator""_c  (char val){return char_{val};}
	inline static LabeledVars str_lits;
	inline cstring operator""_s  (const char* str, std::size_t size){
		data_t data(size+1,static_cast<uint8_t>(0));
		std::copy_n(reinterpret_cast<const uint8_t*>(str),size,data.begin());
		str_lits.presets.push_front(data);
		return ptr<char_>{expr(str_lits.alloc(size+1)->get_ref())};
	}
}

#endif //SOCC_TYPE_HPP
#pragma clang diagnostic pop