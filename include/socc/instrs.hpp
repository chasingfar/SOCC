
#ifndef SOCC_INSTRS_HPP
#define SOCC_INSTRS_HPP

#include <soasm/asm.hpp>
namespace SOCC{
	using SOASM::Lazy;
	using SOASM::Label;
	using SOASM::may_lazy_t;
	using SOASM::data_t;
	using SOASM::Code;
	using SOASM::CodeBlock;
	using SOASM::DataBlock;
	namespace Instrs{
		template<typename T>
		Code push(T);
		template<typename T>
		Code pop(T);
		Code imm(uint8_t);
		Code imm(const Lazy&);
		Code imm(const may_lazy_t&);
		Code imm(const Label&);

		Code load_local(ssize_t);
		Code save_local(ssize_t);
		Code load_ptr(ssize_t);
		Code save_ptr(ssize_t);
		Code pop_ptr();
		Code pushBP();

		Code adj(ssize_t);

		Code jmp(const Label&);
		Code brz(const Label&);
		Code call(const Label&);
		Code ret();
		Code lev();
		Code enter(ssize_t);
		Code leave();
		Code call_ptr();

		Code add();
		Code adc();
		Code sub();
		Code suc();
		Code shl();
		Code shr();
		Code rcr();
		Code rcl();

		Code AND();
		Code OR ();
		Code XOR();
		Code NOT();
		Code pushCF();
		Code popCF();
		data_t to_data(const Label&);
	}
	using namespace Instrs;
}

#endif //SOCC_INSTRS_HPP
