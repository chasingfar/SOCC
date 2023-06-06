#ifndef SOCC_STATEMENT_HPP
#define SOCC_STATEMENT_HPP
#include "type.hpp"
namespace SOCC {
	struct Stmt{
		using type=std::variant<Label,void_,Stmt>;
		std::vector<type> stmts;
		Stmt(std::initializer_list<type> stmts):stmts{stmts}{}
		[[nodiscard]] Code to_code() const;
		Stmt& add(const type& stmt){
			stmts.push_back(stmt);
			return *this;
		}
	};
	struct Block{
		Label start;
		Stmt body{};
		Label end;
		Block()=default;
		Block(const std::string& name):start{name}{};
		Block(std::initializer_list<Stmt> stmts){
			for(const auto& stmt:stmts){
				body.add(stmt);
			}
		}

		[[nodiscard]] Code to_code() const{
			return {start,body,end};
		}
		[[nodiscard]] Code to_protect() const{
			return {jmp(end),to_code()};
		}
	};
	namespace detail{
		Code stmt_if_to_code(const std::vector<std::pair<bool_,Block>>& conds,const Block& else_block);
	}
	template<typename Ret=Stmt>
	struct IF;
	template<typename Ret=Stmt>
	struct if_{
		std::vector<std::pair<bool_,Block>> conds;
		explicit if_(const bool_& cond):conds{{cond,{}}}{}
		explicit if_(const std::vector<std::pair<bool_,Block>>& conds):conds{conds}{}
		auto then(Ret t_code){
			conds.back().second.body.add(asm_(t_code));
			return IF<Ret>(conds);
		}
	};
	template<typename Ret>
	struct IF{
		std::vector<std::pair<bool_,Block>> conds;
		Block else_block{};
		explicit IF(const std::vector<std::pair<bool_,Block>>& conds):conds{conds}{}
		auto elif(const bool_& cond){
			conds.emplace_back(cond,Block{});
			return if_<Ret>(conds);
		}
		Ret else_(Ret f_code){
			else_block.body.add(asm_(f_code));
			return end();
		}
		Ret end() const{
			if constexpr(std::is_same_v<Ret,Stmt>){
				return Stmt{asm_(detail::stmt_if_to_code(conds,else_block))};
			}else{
				return Ret{expr(detail::stmt_if_to_code(conds,else_block))};
			}
		}
	};

	struct LoopStmt{
		void_ break_,continue_;
	};
	struct while_{
		Label start_;
		bool_ cond;
		Block body{};
		Label end_;
		explicit while_(const bool_& cond):cond(cond){}
		while_& do_(Stmt code){
			body.body=std::move(code);
			return *this;
		}
		template<typename F>requires std::is_invocable_r_v<Stmt , F, LoopStmt>
		while_& do_(F&& fn){
			body.body=fn(LoopStmt{asm_(jmp(end_)),asm_(jmp(start_))});
			return *this;
		}
		[[nodiscard]] void_ end() const;
	};
	struct do_{
		Label start_,cont,end_;
		Block body{};
		bool_ cond;
		explicit do_(Stmt code):body{std::move(code)}{}
		template<typename F>requires std::is_invocable_r_v<Stmt , F, LoopStmt>
		explicit do_(F&& fn):body{fn(LoopStmt{asm_(jmp(end_)),asm_(jmp(cont))})}{}
		void_ while_(const bool_& cond_);
	};
	struct for_{
		Label start_,cont,end_;
		void_ init,iter;
		bool_ cond;
		Block body{};
		explicit for_(const void_& init,const bool_& cond,const void_& iter):init(init),cond(cond),iter(iter){}
		for_& do_(Stmt code){
			body.body=std::move(code);
			return *this;
		}
		template<typename F>requires std::is_invocable_r_v<Stmt , F, LoopStmt>
		for_& do_(F&& fn){
			body.body=fn(LoopStmt{asm_(jmp(end_)),asm_(jmp(cont))});
			return *this;
		}
		[[nodiscard]] void_ end() const;
	};
}
#endif //SOCC_STATEMENT_HPP
