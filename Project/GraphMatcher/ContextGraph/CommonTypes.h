
#pragma once

#include <cstdio>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>
#include <iostream>
#include <algorithm>
#include <memory>
#include <regex>
#include <chrono>
#include <type_traits>
#include <functional>
#include <locale>
#include <tuple>
#include <queue>
#include <sstream>
#include <fstream>
#include <random>

#define HAS_MEM_FUNC(func, name) \
	template <typename Type, \
			  typename = typename std::enable_if<std::is_object<Type>::value>::type> \
	class name \
	{ \
	   class yes { char m;}; \
	   class no { yes m[2];}; \
	   struct BaseMixin \
	   { \
		 void func(){} \
	   }; \
	   struct Base : public Type, public BaseMixin {}; \
	   template <typename T, T t>  class Helper{}; \
	   template <typename U> \
	   static no deduce(U*, Helper<void (BaseMixin::*)(), &U::func>* = nullptr); \
	   static yes deduce(...); \
	public: \
	   static const bool result = sizeof(yes) == sizeof(deduce((Base*)(nullptr))); \
	};

typedef std::pair<std::chrono::system_clock::time_point, std::chrono::system_clock::time_point> Duration;
const Duration PERMANENT_DURATION = Duration(std::chrono::system_clock::time_point::min(), std::chrono::system_clock::time_point::max());
const std::chrono::system_clock::time_point NEVER_EXPIRE(std::chrono::system_clock::time_point::max());

