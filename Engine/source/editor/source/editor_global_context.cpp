#include "editor_global_context.h"
#include "runtime/engine.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/rhi/vulkan_device.h"
namespace MW {
	EditorGlobalContext editorGlobalContext;

	void EditorGlobalContext::initialize()
	{
		windowSystem = std::make_shared<WindowSystem>();
		WindowCreateInfo windowInfo;
		windowSystem->initialize(windowInfo);
		RenderSystemInitInfo renderInfo;
		renderInfo.window = windowSystem;
		renderSystem = std::make_shared<RenderSystem>(renderInfo);

	}
	void EditorGlobalContext::clear()
	{
		windowSystem.reset();
	}
}