#ifndef AB1318AF_B0AF_4B57_A68C_6419C2B3294A
#define AB1318AF_B0AF_4B57_A68C_6419C2B3294A

#include <future>

namespace util {

// Binding an instance function to a non-weak pointer.
template<typename TClass,\
				 typename TPointerType,
				 typename... TArgs,
				 typename... TExtraArgs>
std::packaged_task<void(TExtraArgs...)> Bind(
		void (TClass::*func)(TArgs... intl_args),
		TPointerType ptr,
		TArgs... args) {
	return [func, ptr, args...](TExtraArgs... extra_args) {
		(ptr->*func)(args..., extra_args...);
	}
}

// Binding an instance function to a smart pointer.
template<typename TClass, template<typename> class TPtrWrapper, typename... TArgs, typename... TExtraArgs>
std::packaged_task<void(TExtraArgs...)> Bind<TArgs...>(
		void (TClass::*func)(TArgs... intl_args),
		TPtrWrapper<TClass>&& ptr,
		TExtraArgs... args) {
	return [func, p = std::forward(ptr), args...](TExtraArgs... extra_args) {
		(!p) {
			return;
		}

		(p->*func)(args..., extra_args...);
	}
}

// Binding a non-instance function.
template<typename... TArgs, typename... TExtraArgs>
std::packaged_task<void(TExtraArgs...)> Bind(
		std::packaged_task<TArgs..., TExtraArgs...> func,
		TArgs... args) {
	return [func, args...](TExtraArgs... extra_args) {
		func(extra_args...);
	}
}

}  // namespace util

#endif /* AB1318AF_B0AF_4B57_A68C_6419C2B3294A */
