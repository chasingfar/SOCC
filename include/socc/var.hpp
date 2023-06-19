
#ifndef SOCC_VAR_HPP
#define SOCC_VAR_HPP

#include "instrs.hpp"
#include <cstddef>
#include <forward_list>
#include <memory>

namespace SOCC {
	struct Value{
		[[nodiscard]] virtual Code load() const=0;
		virtual ~Value()=default;
	};
	struct Raw:Value{
		data_t data{};
		Raw() = default;
		explicit Raw(data_t data):data(std::move(data)){}
		static auto make(const data_t& data){ return std::make_shared<Raw>(data);}
		[[nodiscard]] Code load() const override;
		[[nodiscard]] std::shared_ptr<Raw> shift(ssize_t shift_offset,size_t new_size) const{
			data_t tmp(new_size);
			std::copy_n(data.begin()+shift_offset,new_size,tmp.begin());
			return make(tmp);
		}
	};
	template<typename T>
	concept CanAsRaw = requires(T x) {
		{ x.as_raw() }->std::same_as<std::shared_ptr<Raw>>;
	};
	struct Expr:Value{
		Code code{};
		Expr() = default;
		explicit Expr(Code  value):code(std::move(value)){}
		explicit Expr(const Value& value):code(value.load()){}
		[[nodiscard]] Code load() const override{
			return code;
		}
	};
	inline auto expr(const Code& code){return std::make_shared<Expr>(code);}
	struct Var:Value{
		[[nodiscard]] virtual Code save() const=0;
	};
	template<typename Reg>
	struct RegVar:Var{
		Reg reg;
		explicit RegVar(Reg reg):reg(reg){}
		static auto make(Reg reg){ return std::make_shared<RegVar>(reg);}
		[[nodiscard]] Code load() const override;
		[[nodiscard]] Code save() const override;
	};

	struct MemVar:Var{
		size_t size;
		ssize_t offset;
		explicit MemVar(size_t size,ssize_t offset):size(size),offset(offset){}
		[[nodiscard]] virtual Code load(ssize_t index,bool is_first) const=0;
		[[nodiscard]] virtual Code save(ssize_t index,bool is_first) const=0;
		[[nodiscard]] virtual Code load(ssize_t index) const { return load(index,true); }
		[[nodiscard]] virtual Code save(ssize_t index) const { return save(index,true); }
		[[nodiscard]] virtual Code get_ref() const=0;
		[[nodiscard]] virtual std::shared_ptr<MemVar> shift(ssize_t shift_offset,size_t new_size) const=0;
		[[nodiscard]] Code load() const override{
			Code tmp{};
			for (size_t i=0; i<size; ++i) {
				tmp.add(load(size-1-i,i==0));
			}
			return tmp;
		}
		[[nodiscard]] Code save() const override{
			Code tmp{};
			for (size_t i=0; i<size; ++i) {
				tmp.add(save(i,i==0));
			}
			return tmp;
		}
	};
	struct LocalVar:MemVar{
		explicit LocalVar(size_t size,ssize_t offset):MemVar(size,offset){}
		static auto make(size_t size,ssize_t offset){ return std::make_shared<LocalVar>(size,offset);}
		[[nodiscard]] Code load(ssize_t index,bool is_first) const override;
		[[nodiscard]] Code save(ssize_t index,bool is_first) const override;
		[[nodiscard]] Code get_ref() const override;
		[[nodiscard]] std::shared_ptr<MemVar> shift(ssize_t shift_offset,size_t new_size) const override{
			return make(new_size,offset + shift_offset);
		}
	};
	struct StaticVar:MemVar{
		Label label;
		StaticVar(size_t size,Label label,ssize_t offset):MemVar(size,offset),label(std::move(label)){}
		static auto make(size_t size,const Label& label,ssize_t offset){ return std::make_shared<StaticVar>(size,label,offset);}
		[[nodiscard]] Code load(ssize_t index,bool is_first) const override;
		[[nodiscard]] Code save(ssize_t index,bool is_first) const override;
		[[nodiscard]] Code get_ref() const override;
		[[nodiscard]] std::shared_ptr<MemVar> shift(ssize_t shift_offset,size_t new_size) const override{
			return make(new_size,label,offset + shift_offset);
		}
	};
	struct PtrVar:MemVar{
		Code ptr;
		explicit PtrVar(size_t size,Code ptr,ssize_t offset=0):MemVar(size,offset),ptr(std::move(ptr)){}
		static auto make(size_t size,const Code& ptr,ssize_t offset=0){ return std::make_shared<PtrVar>(size,ptr,offset);}
		[[nodiscard]] Code load(ssize_t index,bool is_first) const override;
		[[nodiscard]] Code save(ssize_t index,bool is_first) const override;
		[[nodiscard]] Code get_ref() const override{
			return ptr;
		}
		[[nodiscard]] std::shared_ptr<MemVar> shift(ssize_t shift_offset,size_t new_size) const override{
			return make(new_size,ptr,offset + shift_offset);
		}
	};
	struct Allocator{
		template<typename Type,typename ...Rest>
		auto _vars() {
			Type var{alloc(Type::size)};
			if constexpr (sizeof...(Rest)==0){
				return std::tuple{var};
			}else{
				return std::tuple_cat(std::tuple{var}, _vars<Rest...>());
			}
		}
		template<typename ...Types>
		std::tuple<Types...> vars() {
			if constexpr (sizeof...(Types)==0){
				return std::tuple{};
			}else{
				return _vars<Types...>();
			}
		}
		virtual std::shared_ptr<MemVar> alloc(size_t size)=0;
	};
	struct StaticVars:DataBlock,Allocator{
		CodeBlock init;
		std::shared_ptr<MemVar> alloc(size_t size) override {
			auto var=StaticVar::make(size, start, static_cast<ssize_t>(body.size()));
			body.resize(body.size()+size,static_cast<uint8_t>(0));
			return var;
		}
		template<typename T>
		auto operator=(const T& val){
			T var{alloc(T::size)};
			init.body.add(var.set(val));
			return var;
		}
	};
	struct ReadOnlyVars:DataBlock,Allocator{
		std::forward_list<data_t> presets{};
		std::shared_ptr<MemVar> alloc(size_t size) override {
			auto var=StaticVar::make(size, start, static_cast<ssize_t>(body.size()));
			if(presets.empty()){
				body.resize(body.size()+size,static_cast<uint8_t>(0));
			}else{
				auto data=presets.front();
				body.insert(body.end(),data.begin(),data.end());
				presets.pop_front();
			}
			return var;
		}
		template<typename T>
		auto operator=(const T& val){
			presets.push_front(val.as_raw()->data);
			return T{alloc(T::size)};
		}
	};
}
#endif //SOCC_VAR_HPP
