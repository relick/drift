#pragma once

#include <array>
#include <concepts>

namespace detail
{
// Make it possible to forward types that match but cast those that don't
template< typename From, typename To > requires std::convertible_to< From, To >
struct ConvertOrForwardHelper { using Type = To; };
template< typename From, typename To > requires std::same_as< From, To >
struct ConvertOrForwardHelper< From, To > { using Type = To&&; };

template< typename From, typename To >
using ConvertOrForwardType = typename ConvertOrForwardHelper< From, To >::Type;
}

template< typename T, typename F, typename ...U >
constexpr auto MakeArray( F&& arg1, U&&... args )
	requires ( std::convertible_to< F, T > && ( std::convertible_to< U, T > && ... ) )
{
	return std::array< T, 1 + sizeof...( args ) > {
		static_cast< detail::ConvertOrForwardType< F, T > >( std::forward< F >( arg1 ) ),
		static_cast< detail::ConvertOrForwardType< U, T > >( std::forward< U >( args ) )...
	};
}