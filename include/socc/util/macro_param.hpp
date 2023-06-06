#ifndef UTIL_MACRO_PARAM_HPP
#define UTIL_MACRO_PARAM_HPP
namespace Util{
	/* macro_param_t<void (T)> = T
	 * Usage:
	 * #define MACRO(TYPE) struct A{ macro_param_t<void TYPE> val;};
	 * MACRO((std::map<int,int>))
	 * */
	template<typename F>struct macro_param{};
	template<typename T>struct macro_param<void(T)>{using type=T;};
	template<typename F>using macro_param_t=typename macro_param<F>::type;
}
#endif //UTIL_MACRO_PARAM_HPP
