#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <memory>

#include "runtime/engine.h"

#include "editor.h"
int main() {
    std::unique_ptr<MW::MWEditor> editor(new MW::MWEditor);
    editor->initialize();
    editor->run();
    editor->clear();
	return 0;
}