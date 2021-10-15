#pragma once

#include <ecs/ecs.h>

// Core
namespace Core
{

// Empty global component, take by ref in any system group that needs to run without parallelisation.
struct MT_Only
{
	ecs_flags( ecs::flag::global );
};

}