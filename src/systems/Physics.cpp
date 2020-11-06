#include "Physics.h"

#include "SystemOrdering.h"
#include "components.h"

#include <btBulletDynamicsCommon.h>

#include <memory>

#define PHYSICS_DEBUG DEBUG_TOOLS

#if PHYSICS_DEBUG
#include <sokol_gfx.h>
#include <util/sokol_gl.h>
#define BT_LINE_BATCH_SIZE 512

std::unique_ptr<btIDebugDraw> debugDrawer;
#endif

namespace Core
{
	namespace Physics
	{
#if PHYSICS_DEBUG
		class DebugDrawer : public btIDebugDraw
		{
			int m_debugMode{ btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb | btIDebugDraw::DBG_DrawContactPoints };

			std::vector<fVec3> m_linePoints;

			fVec3 m_currentLineColor{ -1, -1, -1 };
			DefaultColors m_ourColors;

		public:
			DefaultColors getDefaultColors() const override
			{
				return m_ourColors;
			}

			void setDefaultColors(DefaultColors const& _colours) override
			{
				m_ourColors = _colours;
			}

			void drawLine(fVec3 const& _from, fVec3 const& _to, fVec3 const& _colour) override
			{
				if (m_currentLineColor != _colour || m_linePoints.size() >= BT_LINE_BATCH_SIZE)
				{
					flushLines();
					m_currentLineColor = _colour;
				}

				m_linePoints.push_back(_from);
				m_linePoints.push_back(_to);
			}

			void drawContactPoint(fVec3 const& _pointOnB, fVec3 const& _normalOnB, btScalar _distance, int _lifeTime, fVec3 const& _colour) override
			{
				drawLine(_pointOnB, _pointOnB + _normalOnB * _distance, _colour);
				btVector3 ncolor(0, 0, 0);
				drawLine(_pointOnB, _pointOnB + _normalOnB * 0.01, ncolor);
			}

			void reportErrorWarning(char const* _warningString) override
			{}

			void draw3dText(fVec3 const& _location, char const* _textString) override
			{}

			void setDebugMode(int _debugMode) override
			{
				m_debugMode = _debugMode;
			}

			int getDebugMode() const override
			{
				return m_debugMode;
			}

			void clearLines() override
			{

			}

			void flushLines() override
			{
				int sz = m_linePoints.size();
				if (sz)
				{
					sgl_begin_lines();
					sgl_c3b(m_currentLineColor.x(), m_currentLineColor.y(), m_currentLineColor.z());
					for (fVec3 const& lineP : m_linePoints)
					{
						sgl_v3f(lineP.x(), lineP.y(), lineP.z());
					}
					sgl_end();

					m_linePoints.clear();
				}
			}
		};

		btIDebugDraw* GetDebugDrawer()
		{
			return debugDrawer.get();
		}
#endif

		void Init()
		{
#if PHYSICS_DEBUG
			debugDrawer = std::make_unique<DebugDrawer>();
#endif
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
#if PHYSICS_DEBUG
				_pw.m_dynamicsWorld->debugDrawWorld();
#endif
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
#if PHYSICS_DEBUG
			debugDrawer.reset();
#endif
		}
	}
}