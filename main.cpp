#include <iostream>
#include <cmath>
#include <socc/library.hpp>
#include <soasm/asm.hpp>
#include <soasm/soisv1.hpp>

using namespace SOCC;
using Reg=SOASM::SOISv1::Reg;

inline static const u8 Reg_A{RegVar<Reg>::make(Reg::A)};
inline static const u8 Reg_B{RegVar<Reg>::make(Reg::B)};
inline static const u8 Reg_C{RegVar<Reg>::make(Reg::C)};
inline static const u8 Reg_D{RegVar<Reg>::make(Reg::D)};
inline static const u8 Reg_E{RegVar<Reg>::make(Reg::E)};
inline static const u8 Reg_F{RegVar<Reg>::make(Reg::F)};
inline static const u8 Reg_L{RegVar<Reg>::make(Reg::L)};
inline static const u8 Reg_H{RegVar<Reg>::make(Reg::H)};

int main(){
	constexpr size_t mem_size=1<<16;
	std::array<uint8_t,mem_size> mem{};
	Label main;
	Fn<u8(u8)> fib{"fib(i)"};
	Code program{
		fib(6_u8),
		SOASM::SOISv1::Halt{}(),
		fib.impl([&](auto& _,auto i)->Stmt{
			return {
				if_(i <= 1_u8).then({
					_.return_(i),
				}).else_({
					_.return_(fib(i - 1_u8) + fib(i - 2_u8)),
				}),
			};
		}),
	};
	auto data=program.assemble();
	for(auto [addr,bytes,str]:SOASM::disassemble<SOASM::SOISv1::InstrSet>(data)){
		std::string bytes_str;
		for(auto b:bytes){
			bytes_str+=std::bitset<8>(b).to_string()+" ";
		}
		std::cout<<std::format("{0:016b}:{1:27};{0}:{2}\n",addr,bytes_str,str);
	}
	std::ranges::move(data,mem.begin());

	SOASM::SOISv1::Context ctx{mem};
	for(int i=0;i<1000;i++){
		if(!ctx.run()){
			std::cout<<"halt"<<std::endl;
			break;
		}
		using namespace std::views;
		std::cout<<std::format(
			"{}:{:s};{:04x},{:04x};{:s}{:s}\n",
			i,
			ctx.reg.regs|transform([](auto i){
				return std::format(" {:02x}",i);
			})|join,
			ctx.pc,ctx.sp,
			iota((size_t)ctx.sp,mem_size)|take(10)|transform([&](auto i){
				return std::format(" {:02x}",(int)ctx.mem[i]);
			})|join,
			(ctx.sp+10<mem_size?"...":"")
		);
	}
	return 0;
}
