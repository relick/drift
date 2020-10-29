#include "common.h"

#include "components.h"
#include "systems.h"

#include <iostream>

#include <ecs/ecs.h>

#include <chrono>
#include <thread>
using namespace std::chrono_literals;

int main()
{
    // Setup entity manager

    // Setup entity initialisers

    // Setup systems
    ecs::add_component(0, Core::Transform());

    ecs::make_system<ecs::opts::group<0>>([](Core::Transform& _trans)
    {
        _trans.m_transform.getOrigin() += LoadVec3(0.0f, 1.0f, 0.0f);
    });

    ecs::make_system<ecs::opts::group<1>>([](Core::Transform& _trans)
    {
        fVec3Data pos;
        _trans.m_transform.getOrigin().serialize(pos);
        std::cout << "x: " << pos.m_floats[0] << ", y: " << pos.m_floats[1] << ", z: " << pos.m_floats[2] << '\n';

        std::this_thread::sleep_for(1000ms);
    });

    ecs::commit_changes();

    // Run
    while (true)
    {
        ecs::update();
    }
}