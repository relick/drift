#include "Physics.h"

#include "SystemOrdering.h"
#include "components.h"

#include <btBulletDynamicsCommon.h>

#include <memory>

#if PHYSICS_DEBUG
#include <sokol_gfx.h>
#include <util/sokol_gl.h>
#define BT_LINE_BATCH_SIZE 512
#include "ImGui.h"
#include <imgui.h>

#include "managers/Input.h"
#include "systems/Core/TextAndGLDebug.h"

std::unique_ptr<btIDebugDraw> debugDrawer;

struct ImGuiWorldData
{
	Core::EntityID worldEntity;
	bool showDebugDraw{ false };
};

struct ImGuiData
{
	bool showImguiWin = false;
	std::vector<ImGuiWorldData> physicsWorlds;
} imGuiData;

#endif
namespace Core
{
	namespace Physics
	{
#if PHYSICS_DEBUG
		class DebugDrawer : public btIDebugDraw
		{
			int m_debugMode{ btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb | btIDebugDraw::DBG_DrawContactPoints };

			std::vector<btVector3> m_linePoints;

			btVector3 m_currentLineColor{ -1, -1, -1 };
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

			void drawLine(btVector3 const& _from, btVector3 const& _to, btVector3 const& _colour) override
			{
				if (m_currentLineColor != _colour || m_linePoints.size() >= BT_LINE_BATCH_SIZE)
				{
					flushLines();
					m_currentLineColor = _colour;
				}

				m_linePoints.push_back(_from);
				m_linePoints.push_back(_to);
			}

			void drawContactPoint(btVector3 const& _pointOnB, btVector3 const& _normalOnB, btScalar _distance, int _lifeTime, btVector3 const& _colour) override
			{
				drawLine(_pointOnB, _pointOnB + _normalOnB * _distance, _colour);
				btVector3 ncolor(0, 0, 0);
				drawLine(_pointOnB, _pointOnB + _normalOnB * 0.01f, ncolor);
			}

			void reportErrorWarning(char const* _warningString) override
			{}

			void draw3dText(btVector3 const& _location, char const* _textString) override
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
				if (!m_linePoints.empty())
				{
					sgl_begin_lines();
					sgl_c3f(m_currentLineColor.x(), m_currentLineColor.y(), m_currentLineColor.z());
					for (btVector3 const& lineP : m_linePoints)
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

		void AddPhysicsWorld(Core::EntityID _entity)
		{
			imGuiData.physicsWorlds.push_back(ImGuiWorldData{ _entity });
		}

		void RemovePhysicsWorld(Core::EntityID _entity)
		{
			for (auto worldI = imGuiData.physicsWorlds.begin(); worldI != imGuiData.physicsWorlds.end(); ++worldI)
			{
				if (worldI->worldEntity == _entity)
				{
					imGuiData.physicsWorlds.erase(worldI);
					return;
				}
			}
		}
#endif

		void Init()
		{
#if PHYSICS_DEBUG

			btIDebugDraw::DefaultColors colours;
			colours.m_aabb = btVector3(1.0f, 0.0f, 0.0f);
			colours.m_contactPoint = btVector3(0.0f, 1.0f, 0.0f);
			colours.m_activeObject = btVector3(0.0f, 0.0f, 1.0f);

			debugDrawer = std::make_unique<DebugDrawer>();
			debugDrawer->setDefaultColors(colours);
#endif
		}

		float rayLambda[2]{};
		
		void AddCharacterControllerSystems()
		{
			ecs::make_system<ecs::opts::group<Sys::GAME>>([](Core::FrameData const& _fd, Core::Physics::CharacterController& _cc, Core::Transform& _t)
			{
				auto fnOnGround = []() -> bool
				{
					return rayLambda[0] < 1.0f;
				};

				// playerStep
				{
					/* Handle turning */
					static float turnAngle = 0.0f;
					float const turnVel = 1.0f;
					float const walkVel = 8.0f;
					float const maxLinearVel = 10.0f;
					if (Core::Input::Pressed(Core::Input::Action::Left))
					{
						turnAngle += _fd.dt * turnVel;
					}
					if (Core::Input::Pressed(Core::Input::Action::Right))
					{
						turnAngle -= _fd.dt * turnVel;
					}

					_t.T().m_basis = fMat3(glm::rotate(fQuatIdentity(), turnAngle, fVec3(0.0, 1.0, 0.0)));

					btVector3 linearVelocity = _cc.m_body->getLinearVelocity();
					float yLinVel = linearVelocity.y();
					linearVelocity.setY(0.0f);
					float speed = _cc.m_body->getLinearVelocity().length();

					fVec3 const forwardDir = _t.T().forward();
					fVec3 walkDirection(0.0, 0.0, 0.0);
					float walkSpeed = walkVel * _fd.dt;

					Core::Render::Debug::DrawLine(_t.T().m_origin, _t.T().m_origin + forwardDir);

					if (Core::Input::Pressed(Core::Input::Action::Forward))
					{
						walkDirection += forwardDir;
					}
					if (Core::Input::Pressed(Core::Input::Action::Backward))
					{
						walkDirection -= forwardDir;
					}


					if (!Core::Input::Pressed(Core::Input::Action::Forward) && !Core::Input::Pressed(Core::Input::Action::Backward) && fnOnGround())
					{
						/* Dampen when on the ground and not being moved by the player */
						linearVelocity *= powf(0.2f, _fd.dt);
						linearVelocity.setY(yLinVel);
						_cc.m_body->setLinearVelocity(linearVelocity);
					}
					else
					{
						if (speed < maxLinearVel)
						{
							linearVelocity.setY(yLinVel);
							fVec3 walkF = walkDirection * walkSpeed;
							btVector3 walk(walkF.x, walkF.y, walkF.z);
							btVector3 velocity = linearVelocity + walk;
							_cc.m_body->setLinearVelocity(velocity);
						}
					}

					btTransform const tForBullet = _t.T().GetBulletTransform();
					_cc.m_body->setWorldTransform(tForBullet);
					_cc.m_motionState->setWorldTransform(tForBullet);
				}

				// jump
				if (Core::Input::PressedOnce(Core::Input::Action::Jump) && fnOnGround())
				{
					fVec3 const up = _t.T().up();
					float magnitude = _cc.m_body->getMass() * 8.0f;
					fVec3 const impulse = up * magnitude;
					_cc.m_body->applyCentralImpulse(ConvertTobtVector3(impulse));
				}
			});

			ecs::make_system<ecs::opts::group<Sys::PHYSICS_TRANSFORMS_IN>>([](Core::FrameData const& _fd, Core::Physics::CharacterController& _cc, Core::Transform const& _t)
			{
				btTransform xform;
				_cc.m_motionState->getWorldTransform(xform);

				// preStep
				{
					Core::Physics::World const& pw = GetWorld(_cc.m_physicsWorld);

					fVec3 down = -_t.T().up();
					fVec3 forward = _t.T().forward();

					btVector3 raySource[2]{};
					btVector3 rayTarget[2]{};
					raySource[0] = xform.getOrigin();
					raySource[1] = xform.getOrigin();

					rayTarget[0] = raySource[0] + ConvertTobtVector3(down * _cc.m_halfHeight * 1.2f);
					rayTarget[1] = raySource[1] + ConvertTobtVector3(forward * _cc.m_halfHeight * 1.2f);

					class ClosestNotMe : public btCollisionWorld::ClosestRayResultCallback
					{
					public:
						ClosestNotMe(btRigidBody* me) : btCollisionWorld::ClosestRayResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
						{
							m_me = me;
						}

						virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
						{
							if (rayResult.m_collisionObject == m_me)
								return 1.0;

							return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
						}
					protected:
						btRigidBody* m_me;
					};

					ClosestNotMe rayCallback(_cc.m_body);

					int i = 0;
					for (i = 0; i < 2; i++)
					{
						rayCallback.m_closestHitFraction = 1.0f;
						pw.m_dynamicsWorld->rayTest(raySource[i], rayTarget[i], rayCallback);
						if (rayCallback.hasHit())
						{
							rayLambda[i] = rayCallback.m_closestHitFraction;
						}
						else
						{
							rayLambda[i] = 1.0f;
						}
					}
				}
			});

			ecs::make_system<ecs::opts::group<Sys::PHYSICS_TRANSFORMS_OUT>>([](Core::Physics::CharacterController const& _cc, Core::Transform& _t)
			{
				btTransform trans;
				_cc.m_body->getMotionState()->getWorldTransform(trans);
				_t.T() = _t.CalculateLocalTransform(fTrans(trans));
			});
		}

		void Setup()
		{
			AddCharacterControllerSystems();


			// Object transforms being pushed into physics
			ecs::make_system<ecs::opts::group<Sys::PHYSICS_TRANSFORMS_IN>>([](Core::Physics::RigidBody& _rb, Core::Transform const& _t)
			{
				// Can't set transforms for non-kinematic bodies.
				if (_rb.m_body->isKinematicObject())
				{
					fTrans const worldTrans = _t.CalculateWorldTransform();
					_rb.m_motionState->setWorldTransform(worldTrans.GetBulletTransform());
				}
			});

			// Physics update step
			// Multiple worlds can run I guess
			ecs::make_system<ecs::opts::group<Sys::PHYSICS_STEP>
#if PHYSICS_DEBUG
			, ecs::opts::not_parallel>([](ecs::entity_id _entity,
#else
			>([](
#endif 
				Core::FrameData const& _fd, Core::Physics::World& _pw)
			{
				_pw.m_dynamicsWorld->stepSimulation(_fd.dt, 10);
#if PHYSICS_DEBUG
				for (ImGuiWorldData const& world : imGuiData.physicsWorlds)
				{
					if (world.worldEntity == _entity && world.showDebugDraw)
					{
						_pw.m_dynamicsWorld->debugDrawWorld();
					}
				}
#endif
			});

#if PHYSICS_DEBUG
			Core::Render::DImGui::AddMenuItem("Physics", "Physics worlds", &imGuiData.showImguiWin);

			ecs::make_system<ecs::opts::group<Sys::IMGUI>>([](Core::MT_Only&, Core::GlobalWorkaround_Tag)
			{
				if (imGuiData.showImguiWin)
				{
					if (ImGui::Begin("Physics worlds", &imGuiData.showImguiWin, 0))
					{
						for (ImGuiWorldData& world : imGuiData.physicsWorlds)
						{
							ImGui::PushID((int32)world.worldEntity.GetValue());
							ImGui::Text("World %u", (uint32)world.worldEntity.GetValue());
							ImGui::Checkbox("- Show Debug", &world.showDebugDraw);
							ImGui::PopID();
						}
					}
					ImGui::End();
				}
			});
#endif

			// Physics propogating transforms
			// Just reading and throwing into the transform components so should be parallel.
			ecs::make_system<ecs::opts::group<Sys::PHYSICS_TRANSFORMS_OUT>>([](Core::Physics::RigidBody const& _rb, Core::Transform& _t)
			{
				if (_rb.m_body->isActive())
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

					_t.T() = _t.CalculateLocalTransform(fTrans(trans));

					{
						Core::Render::Debug::DrawLine(_t.T().m_origin, _t.T().m_origin + _t.T().forward());
						
						Core::Render::Debug::DrawLine(_t.T().m_origin, _t.T().m_origin + _t.T().up());

						Core::Render::Debug::DrawLine(_t.T().m_origin, _t.T().m_origin + _t.T().right());
					}
					{
						fVec3 btForwardDir = ConvertFrombtVector3(trans.getBasis().getColumn(2));
						fVec3 btOrigin = ConvertFrombtVector3(trans.getOrigin());
						Core::Render::Debug::DrawLine(btOrigin, btOrigin + btForwardDir, Colour::green);

						fVec3 btUpDir = ConvertFrombtVector3(trans.getBasis().getColumn(1));
						Core::Render::Debug::DrawLine(btOrigin, btOrigin + btUpDir, Colour::green);

						fVec3 btRightDir = ConvertFrombtVector3(trans.getBasis().getColumn(0));
						Core::Render::Debug::DrawLine(btOrigin, btOrigin + btRightDir, Colour::green);

					}
				}
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