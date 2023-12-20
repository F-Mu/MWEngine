#include "editorGlobalContext.h"
#include "Runtime/engine.h"
#include "Runtime/function/render/windowSystem.h"
#include "Runtime/function/render/renderSystem.h"
#include "Runtime/function/render/rhi/vulkanDevice.h"
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