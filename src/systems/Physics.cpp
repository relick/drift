#include "Physics.h"

#include "SystemOrdering.h"
#include "components.h"

namespace Core
{
	namespace Physics
	{
		void Init()
		{

		}
		void Setup()
		{
			// Object transforms being pushed into physics
			// Serial to prevent (unconfirmed?) issues
			ecs::make_system<ecs::opts::group<Sys::PHYSICS_TRANSFORMS_IN>, ecs::opts::not_parallel>([](Core::Physics::RigidBody& _rb, Core::Transform const& _t)
			{
				// Can't set transforms for active non-kinematic bodies.
				if (!_rb.m_body->isActive() || _rb.m_body->isKinematicObject())
				{
					fTrans const worldTrans = _t.CalculateWorldTransform();
					_rb.m_motionState->setWorldTransform(worldTrans);
				}
			});

			// Physics update step
			// Multiple worlds can run I guess
			ecs::make_system<ecs::opts::group<Sys::PHYSICS_STEP>>([](Core::FrameData const& _fd, Core::Physics::World& _pw)
			{
				_pw.m_dynamicsWorld->stepSimulation(_fd.dt, 10);
			});

			// Physics propogating transforms
			// Just reading and throwing into the transform components so should be parallel.
			ecs::make_system<ecs::opts::group<Sys::PHYSICS_TRANSFORMS_OUT>>([](Core::Physics::RigidBody const& _rb, Core::Transform& _t)
			{
				btTransform trans;
				if (_rb.m_body->getMotionState())
				{
					_rb.m_body->getMotionState()->getWorldTransform(trans);
				}
				else
				{
					trans = _rb.m_body->getWorldTransform();
				}

				_t.T() = _t.CalculateLocalTransform(trans);
			});
		}

		void Cleanup()
		{

		}
	}
}