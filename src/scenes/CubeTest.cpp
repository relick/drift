#include "CubeTest.h"

#include "managers/EntityManager.h"
#include "components.h"
#include "systems.h"


struct CubeTest
{
	bool isLightCube;
	Vec1 rx{ 0.0f };
	Vec1 ry{ 0.0f };
};

static void CubeTestEntities()
{
	Core::EntityID ground = Core::CreateEntity();
	Trans const groundTrans{ Identity<Quat>(), Vec3(0.0f, -2.0f, 0.0f) };
	Core::AddComponent(ground, Core::Transform3D(groundTrans));
	{
		Core::Render::ModelDesc modelDesc{};
		modelDesc.m_filePath = "assets/models/cube/groundcube.obj";
		Core::AddComponent(ground, modelDesc);
	}
	{
		Core::Physics::RigidBodyDesc rbDesc{};
		rbDesc.m_shapeType = Core::Physics::ShapeType::Box;
		rbDesc.m_boxHalfDimensions = Vec3(50.0f, 1.0f, 50.0f);
		rbDesc.m_mass = 0.0f;
		rbDesc.m_startTransform = groundTrans;
		rbDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

		Core::AddComponent(ground, rbDesc);
	}

	Core::EntityID wall = Core::CreateEntity();
	Trans const wallTrans{ Mat3(glm::yawPitchRoll(0.0f, glm::radians(90.0f), 0.0f)), Vec3(0.0f, 0.0f, 2.0f) };
	Core::AddComponent(wall, Core::Transform3D(wallTrans));
	{
		Core::Render::ModelDesc modelDesc{};
		modelDesc.m_filePath = "assets/models/cube/groundcube.obj";
		Core::AddComponent(wall, modelDesc);
	}
	{
		Core::Physics::RigidBodyDesc rbDesc{};
		rbDesc.m_shapeType = Core::Physics::ShapeType::Box;
		rbDesc.m_boxHalfDimensions = Vec3(50.0f, 1.0f, 50.0f);
		rbDesc.m_mass = 0.0f;
		rbDesc.m_startTransform = wallTrans;
		rbDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

		Core::AddComponent(wall, rbDesc);
	}

	Core::EntityID cube = Core::CreateEntity();
	Trans const cubeTrans{ Identity<Quat>(), Vec3(0.0f, 0.0f, 0.0f) };
	Core::AddComponent(cube, Core::Transform3D(cubeTrans));
	Core::AddComponent(cube, CubeTest{ false, 0.0f, 0.0f });
	{
		Core::Render::ModelDesc modelDesc{};
		modelDesc.m_filePath = "assets/models/cube/bluecube.obj";
		Core::AddComponent(cube, modelDesc);
	}
	{
		Core::Physics::RigidBodyDesc rbDesc{};
		rbDesc.m_shapeType = Core::Physics::ShapeType::Box;
		rbDesc.m_boxHalfDimensions = Vec3(0.5f, 0.5f, 0.5f);
		rbDesc.m_mass = 1000.0f;
		rbDesc.m_isKinematic = true;
		rbDesc.m_startTransform = cubeTrans;
		rbDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

		Core::AddComponent(cube, rbDesc);
	}
	constexpr bool enableBGM = false;
	if constexpr (enableBGM)
	{
		Core::AddComponent(cube, Core::Sound::BGMDesc{ .m_filePath = "assets/encrypted/bgm/rom.mp3", .m_initVolume = 1.0f, });
		Core::AddComponent(cube, Core::Sound::FadeChangeBGM{ .m_timeToFade = 5.0f, .m_targetVolume = 1.0f, .m_nextBGMFilePath = "assets/encrypted/bgm/ztd.mp3", });
	}

	Core::EntityID cube2 = Core::CreateEntity();
	Trans const cube2Trans{ Identity<Quat>(), Vec3(-0.5f, 1.5f, 0.0f) };
	Core::AddComponent(cube2, Core::Transform3D(cube2Trans));
	{
		Core::Render::ModelDesc modelDesc{};
		modelDesc.m_filePath = "assets/models/cube/bluecube.obj";
		Core::AddComponent(cube2, modelDesc);
	}
	{
		Core::Physics::RigidBodyDesc rbDesc{};
		rbDesc.m_shapeType = Core::Physics::ShapeType::Box;
		rbDesc.m_boxHalfDimensions = Vec3(0.5f, 0.5f, 0.5f);
		rbDesc.m_mass = 160.0f;
		rbDesc.m_startTransform = cube2Trans;
		rbDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

		Core::AddComponent(cube2, rbDesc);
	}

	constexpr bool enableBackpack = false;
	if constexpr (enableBackpack)
	{
		Core::EntityID backpack = Core::CreateEntity();
		Core::AddComponent(backpack, Core::Transform3D(Identity<Quat>(), Vec3(0.0f, 1.0f, -2.0f)));
		{
			Core::Render::ModelDesc modelDesc{};
			modelDesc.m_filePath = "assets/models/backpack/backpack.obj";
			Core::AddComponent(backpack, modelDesc);
			Core::AddComponent(backpack, CubeTest{ true, 0.0f, 0.0f });
		}
	}

	Core::EntityID lightCube = Core::CreateEntity();
	Core::AddComponent(lightCube, Core::Transform3D(RotationFromForward(Vec3(-1.0f, -1.0f, 1.0f)), Vec3(1.2f, 1.0f, 2.0f)));
	{
		Core::Render::Light lightComponent{};
		lightComponent.m_colour = Vec3(1.0f, 1.0f, 1.0f);
		lightComponent.m_intensity = 1.0f;
		lightComponent.m_type = Core::Render::Light::Type::Directional;
		Core::AddComponent(lightCube, lightComponent);
	}
	Core::AddComponent(lightCube, Core::Render::SkyboxDesc{ .m_cubemapPath = "assets/encrypted/skybox/starrysky/starrysky.cubemap" });

	Core::EntityID lightCube2 = Core::CreateEntity();
	Core::AddComponent(lightCube2, CubeTest{ true, 0.0f, 0.0f });
	Core::AddComponent(lightCube2, Core::Transform3D(RotationFromForward(Vec3(0.0f, 0.0f, -1.0f)), Vec3(1.2f, 1.0f, 2.0f)));
	{
		Core::Render::Light lightComponent{};
		lightComponent.m_colour = Vec3(1.0f, 1.0f, 1.0f);
		lightComponent.m_intensity = 5.0f;
		lightComponent.m_attenuation = Vec3(1.0f, 0.07f, 0.18f);
		lightComponent.m_type = Core::Render::Light::Type::Spotlight;
		Core::AddComponent(lightCube2, lightComponent);
	}

	Core::EntityID lightCube3 = Core::CreateEntity();
	Core::AddComponent(lightCube3, Core::Transform3D(lightCube2));
	{
		Core::Render::Light lightComponent{};
		lightComponent.m_colour = Vec3(1.0f, 1.0f, 1.0f);
		lightComponent.m_intensity = 0.1f;
		lightComponent.m_type = Core::Render::Light::Type::Ambient;
		Core::AddComponent(lightCube3, lightComponent);
	}

}

void CubeTestSystems()
{
	Core::MakeSystem<Sys::GAME>([](Core::FrameData const& _fd, CubeTest& _cubeTest, Core::Transform3D& _t)
	{
		if (_cubeTest.isLightCube)
		{
			_cubeTest.rx += _fd.dt;
			_cubeTest.ry = sin(_cubeTest.rx);
			_t.T().m_origin.x = 0.0f;
			_t.T().m_basis = glm::yawPitchRoll(_cubeTest.rx, 0.0f, 0.0f);
		}
		else
		{
			_cubeTest.rx -= 1.0f * _fd.dt;
			_cubeTest.ry -= 2.0f * _fd.dt;
			_t.T().m_basis = glm::yawPitchRoll(0.0f, _cubeTest.rx, _cubeTest.ry);
		}
	});

	Core::MakeSystem<Sys::GAME>([](Core::FrameData const& _fd, Core::Render::Light& _light, Core::Transform3D& _t)
	{
		static Vec1 n = 0.0f;
		n += _fd.dt * 0.5f;
		if (_light.m_type == Core::Render::Light::Type::Directional)
		{
			_t.T().m_basis = RotationFromForward(Vec3(cos(n), sin(n), 1.0f));
		}
	});
}

namespace Game::Scene
{
	void CubeTestScene::Setup()
	{
		Core::EntityID const camera = Core::CreateEntity();
		Core::EntityID const character = Core::CreateEntity();
		Trans const characterTrans{ Identity<Quat>(), Vec3(2.0f, 0.0f, 0.0f) };
		Core::AddComponent(character, Core::Transform3D(characterTrans));
		{
			Core::Physics::CharacterControllerDesc ccDesc{};
			ccDesc.m_viewObject = camera;
			ccDesc.m_halfHeight = 0.9f;
			ccDesc.m_radius = 0.5f;
			ccDesc.m_mass = 80.0f;
			ccDesc.m_startTransform = characterTrans;
			ccDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

			Core::AddComponent(character, ccDesc);
		}

		Core::AddComponent(camera, Core::Transform3D(Identity<Quat>(), Vec3(0.0f, 0.8f, 0.0f), character));
		Core::AddComponent(camera, Core::Render::MainCamera3D());
		Core::AddComponent(camera, Core::Render::DebugCameraControl());
		Core::AddComponent(camera, Game::Player::MouseLook());

		CubeTestEntities();
	}
}