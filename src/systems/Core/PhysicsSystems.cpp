#include "PhysicsSystems.h"

#include "SystemOrdering.h"
#include "components.h"
#include "managers/InputManager.h"

#include <btBulletDynamicsCommon.h>

#include <memory>

#if PHYSICS_DEBUG
#include <sokol_gfx.h>
#include <util/sokol_gl.h>
#include <imgui.h>

#include "systems/Core/ImGuiSystems.h"
#include "systems/Core/TextAndGLDebugSystems.h"

std::unique_ptr<btIDebugDraw> debugDrawer;

struct ImGuiWorldData
{
	Core::EntityID worldEntity;
	bool showDebugDraw{ false };
};

struct ImGuiData
{
	bool showImguiWin{ false };
	std::vector<ImGuiWorldData> physicsWorlds;
	bool showRBAxes{ false };
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
				Render::Debug::DrawLine(ConvertFrombtVector3(_from), ConvertFrombtVector3(_to), ConvertFrombtVector3(_colour));
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

		bool onGround{ false };
		btVector3 groundPoint{};
		
		void AddCharacterControllerSystems()
		{
			ecs::make_system<ecs::opts::group<Sys::GAME>>([](Core::FrameData const& _fd, Core::Physics::CharacterController& _cc, Core::Transform& _t)
			{
				// playerStep
				{
					/* Handle turning */
					float const turnVel = 1.0f;
					float const walkAccel = 250.0f;
					float const maxLinearVel2 = powf(15.0f / 3.6f, 2);

					btVector3 linearVelocity = _cc.m_body->getLinearVelocity();
					float yLinVel = linearVelocity.y();

					fVec3 forward = _cc.m_viewObject.IsValid() ? (ecs::get_component<Core::Transform>(_cc.m_viewObject.GetValue())->T().forward()) : _t.T().forward();
					forward.y = 0;
					forward = glm::normalize(forward);
					fVec3 right = glm::normalize(glm::cross(forward, fVec3(0, 1, 0)));
					fVec3 walkDirection(0.0, 0.0, 0.0);
					float walkSpeed = walkAccel * _fd.dt;

#if PHYSICS_DEBUG
					if (imGuiData.showRBAxes)
					{
						Core::Render::Debug::DrawLine(_t.T().m_origin, _t.T().m_origin + forward);
					}
#endif

					if (Core::Input::Pressed(Core::Input::Action::Forward))
					{
						walkDirection += forward;
					}
					if (Core::Input::Pressed(Core::Input::Action::Backward))
					{
						walkDirection -= forward;
					}
					if (Core::Input::Pressed(Core::Input::Action::Left))
					{
						walkDirection -= right;
					}
					if (Core::Input::Pressed(Core::Input::Action::Right))
					{
						walkDirection += right;
					}


					if (onGround)
					{
						/* Dampen when on the ground and not being moved by the player */
						linearVelocity *= powf(0.1f, _fd.dt);
						_cc.m_body->setLinearVelocity(linearVelocity);
					}

					if (onGround || yLinVel >= 0.0f)
					{
						fVec3 dv = walkDirection * walkSpeed;
						linearVelocity += ConvertTobtVector3(dv);
						btScalar speed2 = powf(linearVelocity.x(), 2) + powf(linearVelocity.z(), 2);
						if (speed2 > maxLinearVel2)
						{
							btScalar correction = sqrt(maxLinearVel2 / speed2);
							linearVelocity[0] *= correction;
							linearVelocity[2] *= correction;
						}

						//_cc.m_body->velo
						_cc.m_body->setLinearVelocity(linearVelocity);
					}

					//btTransform const tForBullet = _t.T().GetBulletTransform();
					//_cc.m_body->setWorldTransform(tForBullet);
					//_cc.m_motionState->setWorldTransform(tForBullet);
				}

				// jump
				if (Core::Input::PressedOnce(Core::Input::Action::Jump) && onGround)
				{
					fVec3 const up = _t.T().up();
					float magnitude = _cc.m_body->getMass() * 8.0f;
					fVec3 const impulse = up * magnitude;
					_cc.m_body->applyCentralImpulse(ConvertTobtVector3(impulse));
					onGround = false;
				}

				if (onGround)
				{
					_cc.m_body->setGravity({ 0, 0, 0 });
				}
				else
				{
					_cc.m_body->setGravity({ 0, -8, 0 });
				}
			});

			ecs::make_system<ecs::opts::group<Sys::PHYSICS_TRANSFORMS_IN>>([](Core::FrameData const& _fd, Core::Physics::CharacterController& _cc, Core::Transform const& _t)
			{
				btTransform xform;
				_cc.m_motionState->getWorldTransform(xform);

				// preStep
				{
					Core::Physics::World const& pw = GetWorld(_cc.m_physicsWorld);

					class FindGround : public btCollisionWorld::ContactResultCallback
					{
					public:
						btScalar addSingleResult(btManifoldPoint& cp,
							const btCollisionObjectWrapper* colObj0, int partId0, int index0,
							const btCollisionObjectWrapper* colObj1, int partId1, int index1)
						{
							if (colObj0->m_collisionObject == mMe && !mHaveGround)
							{
								const btTransform& transform = mMe->getWorldTransform();
								// Orthonormal basis (just rotations) => can just transpose to invert
								btMatrix3x3 invBasis = transform.getBasis().transpose();
								btVector3 localPoint = invBasis * (cp.m_positionWorldOnB - transform.getOrigin());
								localPoint[1] += mShapeHalfHeight;
								float r = localPoint.length();
								float cosTheta = localPoint[1] / r;

								if (fabs(r - mShapeRadius) <= mRadiusThreshold && cosTheta < mMaxCosGround)
								{
									mHaveGround = true;
									mGroundPoint = cp.m_positionWorldOnB;
								}
							}
							return 0;
						}

						btRigidBody* mMe{ nullptr };
						// Assign some values, in some way
						float mShapeRadius{};
						float mShapeHalfHeight{};
						float mRadiusThreshold{};
						float mMaxCosGround{};
						bool mHaveGround{ false };
						btVector3 mGroundPoint{};
					};

					FindGround groundCallback;
					groundCallback.mMe = _cc.m_body;
					groundCallback.mShapeRadius = _cc.m_radius;
					groundCallback.mShapeHalfHeight = _cc.m_halfHeight;
					groundCallback.mRadiusThreshold = 0.01f;
					groundCallback.mMaxCosGround = 1.0f;
					pw.m_dynamicsWorld->contactTest(_cc.m_body, groundCallback);
					if (groundCallback.mHaveGround)
					{
						onGround = true;
						groundPoint = groundCallback.mGroundPoint;
					}
					else
					{
						onGround = false;
					}
				}
			});

			ecs::make_system<ecs::opts::group<Sys::PHYSICS_TRANSFORMS_OUT>>([](Core::Physics::CharacterController const& _cc, Core::Transform& _t)
			{
				btTransform trans;
				_cc.m_body->getMotionState()->getWorldTransform(trans);
				_t.SetLocalTransformFromWorldTransform(fTrans(trans));
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
			ecs::make_system<ecs::opts::group<Sys::PHYSICS_STEP>>([](
#if PHYSICS_DEBUG
			ecs::entity_id _entity,
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
			Core::Render::DImGui::AddMenuItem("Physics", "Physics Worlds", &imGuiData.showImguiWin);

			ecs::make_system<ecs::opts::group<Sys::IMGUI>>([](Core::MT_Only&)
			{
				if (imGuiData.showImguiWin)
				{
					if (ImGui::Begin("Physics Worlds", &imGuiData.showImguiWin, 0))
					{
						ImGui::Checkbox("Show RB axes", &imGuiData.showRBAxes);

						for (ImGuiWorldData& world : imGuiData.physicsWorlds)
						{
							ImGui::PushID((int32)world.worldEntity.GetValue());
							ImGui::Text("World %u", (uint32)world.worldEntity.GetValue());
							ImGui::Checkbox("- Show debug", &world.showDebugDraw);
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

					_t.SetLocalTransformFromWorldTransform(fTrans(trans));

#if PHYSICS_DEBUG
					if(imGuiData.showRBAxes)
					{
						Core::Render::Debug::DrawLine(_t.T().m_origin, _t.T().m_origin + _t.T().forward());
						
						Core::Render::Debug::DrawLine(_t.T().m_origin, _t.T().m_origin + _t.T().up());

						Core::Render::Debug::DrawLine(_t.T().m_origin, _t.T().m_origin + _t.T().right());
					}
#endif
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