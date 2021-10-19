#pragma once

#include <concepts>

template< std::integral T = int, T t_shift = T{ 0 } >
consteval T Bit()
{
	static_assert( t_shift >= 0, "Cannot use negative shift value." );
	return T{ 1 } << t_shift;
}

#define BIT(N) Bit< decltype( N ), N >()