#include "editor.h"
#include "editorGlobalContext.h"
namespace MW {
	void MWEditor::run()
	{
	}
	void MWEditor::clear()
	{
		editorGlobalContext.clear();
	}
	void MWEditor::initialize()
	{
		editorGlobalContext.initialize();
	}
}
