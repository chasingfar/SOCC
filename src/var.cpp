#include <socc/var.hpp>
#include <algorithm>
#include <soasm/soisv1.hpp>

using namespace SOCC;
using namespace SOASM::SOISv1;

Code Raw::load() const{
	return data|std::views::reverse|std::views::transform([](auto v){
		return imm(v);
	});
}

template<> Code RegVar<Reg>::load() const {
	return push(reg);
}

template<> Code RegVar<Reg>::save() const {
	return pop(reg);
}
Code LocalVar::load(ssize_t index, bool is_first) const {
	return load_local(offset+index);
}
Code LocalVar::save(ssize_t index, bool is_first) const {
	return save_local(offset+index);
}

Code LocalVar::get_ref() const {
	return pushBP();
}

Code StaticVar::load(ssize_t index, bool is_first) const {
	if(is_first){
		return {imm(label),pop_ptr(), load_ptr(offset+index)};
	}else{
		return load_ptr(offset+index);
	}
}

Code StaticVar::save(ssize_t index, bool is_first) const {
	if(is_first){
		return {imm(label),pop_ptr(), save_ptr(offset+index)};
	}else{
		return save_ptr(offset+index);
	}
}

Code StaticVar::get_ref() const {
	return imm(label);
}

Code PtrVar::load(ssize_t index, bool is_first) const {
	if(is_first){
		return {ptr,pop_ptr(), load_ptr(offset+index)};
	}else{
		return load_ptr(offset+index);
	}
}

Code PtrVar::save(ssize_t index, bool is_first) const {
	if(is_first){
		return {ptr,pop_ptr(), save_ptr(offset+index)};
	}else{
		return save_ptr(offset+index);
	}
}
