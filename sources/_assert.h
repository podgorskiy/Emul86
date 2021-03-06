#pragma once
#include <stdio.h>
#include <cstdlib>
#include <functional>
#ifdef _WIN32
#define BREAKPOINT() { __debugbreak(); }
#else
#define BREAKPOINT() { __builtin_trap(); }
#endif

namespace Assert
{
	enum result
	{
		result_break,
		result_ignore_always,
		result_ignore_once,
	};

	result message(const char *file, int line, const char *condition, const char *fmt, ...);

	extern std::function<void()> OpPause;
}

#define ASSERT(X, ...) \
{\
	static bool _i=false;\
	if (!_i&&!(X))\
	{\
		printf("%s(%d): %s\n",__FILE__,__LINE__,#X);\
		printf(__VA_ARGS__);\
		printf("\n");\
		Assert::result r=Assert::message(__FILE__,__LINE__,#X,__VA_ARGS__);\
		if(r==Assert::result_ignore_always)\
		{\
			_i=true;\
		}\
		else if(r==Assert::result_break)\
		{\
			BREAKPOINT();\
		}\
		else if(r==Assert::result_ignore_once)\
		{\
			if (Assert::OpPause) Assert::OpPause(); \
		}\
	}\
}

#define WARN(...)\
{\
	static bool _i=false;\
	if(!_i)\
	{\
		printf("%s(%d):",__FILE__,__LINE__);\
		printf(__VA_ARGS__);\
		printf("\n");\
		Assert::result r=Assert::message(__FILE__,__LINE__,nullptr,__VA_ARGS__);\
		if(r==Assert::result_ignore_always)\
		{\
			_i=true;\
		}\
		else if(r==Assert::result_break)\
		{\
			BREAKPOINT();\
		}\
		else if(r==Assert::result_ignore_once)\
		{\
			if (Assert::OpPause != nullptr) Assert::OpPause(); \
		}\
	}\
}

#define FAIL(...)\
{\
	static bool _i=false;\
	if(!_i)\
	{\
		printf("%s(%d):",__FILE__,__LINE__);\
		printf(__VA_ARGS__);\
		printf("\n");\
		Assert::result r=Assert::message(__FILE__,__LINE__,nullptr,__VA_ARGS__);\
		if(r==Assert::result_ignore_always)\
		{\
			_i=true;\
		}\
		else if(r==Assert::result_break)\
		{\
			BREAKPOINT();\
		}\
		std::exit(EXIT_FAILURE);\
	}\
}
