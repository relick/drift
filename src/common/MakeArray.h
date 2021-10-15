#pragma once

#include <array>
#include <concepts>

namespace detail
{
// Make it possible to forward types that match but cast those that don't
template< typename From, typename To > requires std::convertible_to< From, To >
struct ConvertOrForwardHelper { using Type = To; };
template< typename From, typename To > requires std::same_as< std::remove_reference_t< From >, std::remove_reference_t< To > >
struct ConvertOrForwardHelper< From, To > { using Type = To&&; };

template< typename From, typename To >
using ConvertOrForwardType = typename ConvertOrForwardHelper< From, To >::Type;
}

template< typename T, typename ...U >
constexpr auto MakeArray( U&&... args )
	requires ( std::convertible_to< U, T > && ... )
{
	return std::array< T, sizeof...( args ) > {
		static_cast< detail::ConvertOrForwardType< U, T > >( std::forward< U >( args ) )...
	};
}
