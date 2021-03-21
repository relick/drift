#include "TextAndGLDebugSystems.h"

#include "components.h"
#include "SystemOrdering.h"
#include "systems/Core/ImGuiSystems.h"
#include "managers/EntityManager.h"
#include "managers/TextManager.h"

#include <sokol_app.h>
#include <sokol_gfx.h>
#include <util/sokol_gl.h>

#include <imgui.h>

#include <vector>
#include <mutex>
#include <absl/container/flat_hash_map.h>

namespace Core
{
	namespace Render
	{
		struct LineToDraw
		{
			fVec3 m_start{};
			fVec3 m_end{};

			LineToDraw(fVec3 const& _start, fVec3 const& _end) : m_start{ _start }, m_end{ _end } {}
		};
		absl::flat_hash_map<uint32, std::vector<LineToDraw>> linesToDraw{};
		std::mutex linesToDrawLock{};

		void FlushGL()
		{
			sgl_begin_lines();
			for (auto const& [col, lines] : linesToDraw)
			{
				sgl_c1i(col);
				for (auto const& line : lines)
				{
					sgl_v3f(line.m_start.x, line.m_start.y, line.m_start.z);
					sgl_v3f(line.m_end.x, line.m_end.y, line.m_end.z);
				}
			}
			sgl_end();
			linesToDraw.clear();
		}

		namespace TextAndGLDebug
		{
			void Init()
			{
				// gl for debug + text
				sgl_desc_t glDesc{};
				glDesc.color_format = static_cast<sg_pixel_format>(sapp_color_format());
				glDesc.depth_format = static_cast<sg_pixel_format>(sapp_depth_format());
				glDesc.sample_count = sapp_sample_count();
				sgl_setup(&glDesc);

				Core::Render::Text::Init();
			}

			void Setup()
			{
				// Prepare GL matrices.
				Core::MakeSystem<Sys::GL_START>([](Core::MT_Only&, Core::Render::FrameData const& _rfd, Core::Render::MainCamera3D const& _cam, Core::Transform3D const& _camT)
				{
					sgl_defaults();
					sgl_matrix_mode_projection();
					sgl_load_identity();
					sgl_perspective(glm::radians(_cam.m_povY), _rfd.renderArea.f.x / _rfd.renderArea.f.y, 0.01f, 1000.0f);

					sgl_matrix_mode_modelview();
					sgl_load_identity();

					fTrans const cameraTrans = _camT.CalculateWorldTransform();
					fMat4 const cameraMat = glm::lookAt(cameraTrans.m_origin, cameraTrans.m_origin + cameraTrans.forward(), fVec3(0.0f, 1.0f, 0.0f));
					sgl_load_matrix(&cameraMat[0][0]);
				});

				// Prepare text matrices.
				Core::MakeSystem<Sys::TEXT_START>([](Core::MT_Only&, Core::Render::FrameData const& _rfd)
				{
					FlushGL();

					sgl_defaults();

					sgl_matrix_mode_projection();
					sgl_load_identity();
					sgl_ortho(0, _rfd.contextWindow.f.x, _rfd.contextWindow.f.y, 0, -1, 1);

					sgl_matrix_mode_modelview();
					sgl_load_identity();
				});

				Core::Render::Text::Setup();
			}

			void Render()
			{
				// Flush the text before drawing
				Core::Render::Text::Render();
				sgl_draw();
			}

			void Cleanup()
			{
				Core::Render::Text::Cleanup();
				sgl_shutdown();
			}

			void Event(sapp_event const* _event)
			{
				Core::Render::Text::Event(_event);
			}
		}

		namespace Debug
		{
			void DrawLine
			(
				fVec3 const& _start,
				fVec3 const& _end,
				uint32 _col
			)
			{
				std::scoped_lock lock(linesToDrawLock);
				linesToDraw[_col].emplace_back(_start, _end);
			}

			void DrawLine
			(
				fVec3 const& _start,
				fVec3 const& _end,
				fVec3 const& _col
			)
			{
				std::scoped_lock lock(linesToDrawLock);
				uint32 const col = Colour::ConvertRGB(_col);
				linesToDraw[col].emplace_back(_start, _end);
			}
		}
	}
}
