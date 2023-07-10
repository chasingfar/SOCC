#include "socc/statement.hpp"
using namespace SOCC;

Code Stmt::to_code() const {
	Code code{};
	for(const auto& stmt:stmts){
		std::visit([&](auto v){
			code.add(v);
		},stmt);
	}
	return code;
}

Code detail::stmt_if_to_code(const std::vector<std::pair<bool_, Block>> &conds, const Block &else_block) {
	Label end;
	Code code{};
	if(!conds.empty()){
		for(auto it=conds.begin();it!=conds.end()-1;++it){
			auto [cond,block]=*it;
			Label next;
			code.add(Code{
					cond,
					brz(next),
					block,
					jmp(end),
					next,
			});
		}
		auto [cond,block]=conds.back();
		if(else_block.body.stmts.empty()){
			return code.add(Code{
					cond,
					brz(end),
					block,
					end,
			});
		}else{
			return code.add(Code{
					cond,
					brz(else_block.start),
					block,
					jmp(end),
					else_block,
					end,
			});
		}
	}
	return {};
}

void_ while_::end() const {
	return asm_({
		start_,
		cond,
		brz(end_),
		body,
		jmp(start_),
		end_,
	});
}

void_ do_::while_(const bool_ &cond_) {
	cond.value=cond_.value;
	return asm_({
		start_,
		body,
		cont,
		cond,
		brz(end_),
		jmp(start_),
		end_,
	});
}

void_ for_::end() const {
	return asm_({
		init,
		start_,
		cond,
		brz(end_),
		body,
		cont,
		iter,
		jmp(start_),
		end_,
	});
}

void_ SOCC::loop_(const Stmt &stmt, const Label &start, const Label &end) {
	return asm_({start,stmt,jmp(start),end});
}