#pragma once

#ifndef DEV
#define DEV
#endif

#ifdef DEV
	#define DEV_PRINT(...) fmt::print(__VA_ARGS__)
#else
	#define DEV_PRINT(...) (void)0
#endif