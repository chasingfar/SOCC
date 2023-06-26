#include "socc/instrs.hpp"
#include <soasm/soisv1.hpp>
#define SOCC_BP Reg16::HL
#define SOCC_PTR Reg16::FE

namespace SOCC::Instrs{
	using namespace SOASM::SOISv1;

	Code load(Reg16 ptr,ssize_t offset){
		if(offset==0){
			return Load{.from=ptr}();
		}else if(-128<=offset&&offset<=127){
			return LoadNear{.from=ptr}(offset);
		}else{
			return LoadFar{.from=ptr}(offset);
		}
	}
	Code save(Reg16 ptr,ssize_t offset){
		if(offset==0){
			return Save{.to=ptr}();
		}else if(-128<=offset&&offset<=127){
			return SaveNear{.to=ptr}(offset);
		}else{
			return SaveFar{.to=ptr}(offset);
		}
	}

	template<> Code push(Reg reg)           {return Push{.from=reg}();}
	template<> Code pop (Reg reg)           {return Pop{.to=reg}();}
	template<> Code push(Reg16 reg)         {return {push(toH(reg)),push(toL(reg))};}
	template<> Code pop (Reg16 reg)         {return {pop(toL(reg)),pop(toH(reg))};}
	Code imm (uint8_t value)     {return ImmVal{}(value);}
	Code imm (const Lazy& value) {return ImmVal{}(value);}
	Code imm (const may_lazy_t& value) {
		return std::visit([](auto v){return imm(v);},value);
	}
	Code imm (const Label& addr) {
		return LE::u16::to_bytes(addr.lazy())
				|std::views::reverse
		    	|std::views::transform([](const Lazy& v){
			return imm(v);
		});
	}
	data_t to_data(const Label& addr){
		data_t data;
		for(auto v:LE::u16::to_bytes(addr.lazy())){
			data.emplace_back(v);
		}
		return data;
	}


	Code jmp(const Label& addr) {return Jump{}(addr);}
	Code brz(const Label& addr) {return BranchZero{}(addr);}
	Code call(const Label& addr) {return Call{}(addr);}
	Code call_ptr() {return CallPtr{}();}
	Code ret() {return Return{}();}

	Code adj(ssize_t size) {return size==0?Code{}:Adjust{}(size);}
	Code lev() {return Leave{.bp=SOCC_BP}();}
	Code ent() {return Enter{.bp=SOCC_BP}();}
	Code enter(ssize_t size) {return {ent(),adj(-size)};}
	Code leave() {return {lev(),ret()};}

	Code add() {return Calc{.fn=Calc::FN::ADD}();}
	Code adc() {return Calc{.fn=Calc::FN::ADC}();}
	Code sub() {return Calc{.fn=Calc::FN::SUB}();}
	Code suc() {return Calc{.fn=Calc::FN::SUC}();}
	Code shl() {return Calc{.fn=Calc::FN::SHL}();}
	Code shr() {return Calc{.fn=Calc::FN::SHR}();}
	Code rcr() {return Calc{.fn=Calc::FN::RCR}();}
	Code rcl() {return Calc{.fn=Calc::FN::RCL}();}

	Code AND() {return Logic{.fn=Logic::FN::AND}();}
	Code  OR() {return Logic{.fn=Logic::FN:: OR}();}
	Code XOR() {return Logic{.fn=Logic::FN::XOR}();}
	Code NOT() {return Logic{.fn=Logic::FN::NOT}();}

	Code pushCF() {return PushCF{}();}
	Code  popCF() {return PopCF{}();}
	Code pushBP() {return push(SOCC_BP);}
	Code load_local(ssize_t offset) {return load(SOCC_BP,offset);}
	Code save_local(ssize_t offset) {return save(SOCC_BP,offset);}
	Code load_ptr  (ssize_t offset) {return load(SOCC_PTR,offset);}
	Code save_ptr  (ssize_t offset) {return save(SOCC_PTR,offset);}
	Code  pop_ptr() {return pop(SOCC_PTR);}

}