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
				fTrans const worldTrans = _t.CalculateWorldTransform();

				// this part is kind of ridiculous. why is b3 different from bt?
				b3Quaternion rot;
				worldTrans.getBasis().getRotation(rot);
				b3Vector3 const pos = worldTrans.getOrigin();
				btTransform physicsTrans;
				physicsTrans.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
				physicsTrans.setOrigin(btVector3(pos.x, pos.y, pos.z));

				if (_rb.m_body->getMotionState())
				{
					_rb.m_body->getMotionState()->setWorldTransform(physicsTrans);
				}
				else
				{
					_rb.m_body->setWorldTransform(physicsTrans);
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

				btQuaternion rot;
				trans.getBasis().getRotation(rot);
				btVector3 const pos = trans.getOrigin();
				fTrans worldTrans;
				worldTrans.setRotation(fQuat(rot.x(), rot.y(), rot.z(), rot.w()));
				worldTrans.setOrigin(LoadVec3(pos.x(), pos.y(), pos.z()));

				_t.T() = _t.CalculateLocalTransform(worldTrans);
			});
		}

		void Cleanup()
		{

		}
	}
}