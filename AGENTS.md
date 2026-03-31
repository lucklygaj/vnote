# VNote Agent Development Guide

## Setup

After cloning the repository, run the init script to set up your development environment:
* **Linux/macOS**: `bash scripts/init.sh`
* **Windows**: `scripts\init.cmd`

## Build/Lint/Test Commands

* **Build**: `mkdir build && cd build && cmake .. && cmake --build . --config Release`
* **Debug Build**: `cmake .. -DCMAKE_BUILD_TYPE=Debug && cmake --build . --config Debug`
* **Run Tests**: Uncomment `add_subdirectory(tests)` in root CMakeLists.txt, rebuild, then `ctest`
* **Run Single Test**: `ctest -R test_task` (use test name pattern matching)
* **Format Code**: Use clang-format with provided `.clang-format` (100 char line limit, 2-space indent)

## Build Artifact Rules (IRON LAW)

All build artifacts MUST be placed under the build directory (`${CMAKE_BINARY_DIR}`),
NEVER in the source tree. Violations must be fixed before any commit.

### Output Directory Structure
- Executables  -> `${CMAKE_BINARY_DIR}/bin/[CONFIG]/`
- Shared libs  -> `${CMAKE_BINARY_DIR}/lib/[CONFIG]/`
- Static libs  -> `${CMAKE_BINARY_DIR}/lib/[CONFIG]/`
- Intermediate -> `${CMAKE_BINARY_DIR}/` only

### Prohibited in Source Tree
- No `.dll`, `.exe`, `.lib`, `.obj`, `.o`, `.a`, `.so`, `.dylib` files
- No `CMakeFiles/`, `cmake_install.cmake`, `*.vcxproj`, `*.sln`
- No `*_autogen/`, `.moc/`, `.uic/`, `.rcc` directories
- No Qt plugin directories (`platforms/`, `iconengines/`, etc.)
- No `build`, `build.*`, `build*` directories at any level

### libs/ Directory Policy
- `libs/` is a source dependency directory (git submodules), NOT a build output directory.
- Third-party libraries under `libs/` must also output their build artifacts to `${CMAKE_BINARY_DIR}/`.
- `target_link_directories()` must NOT point to source directories for build artifacts.

### Pre-build Checklist
Before each build, verify:
1. `src/Release/` does not exist (or is empty)
2. `bin/` does not exist at project root (or is empty)
3. No `.dll`/`.lib`/`.exe` files in `libs/` subdirectories
4. `.gitignore` covers all potential build artifacts


## C++ Code Style Guidelines

* **Standards**: C++14, Qt 5/6 framework with CMAKE_AUTOMOC/AUTOUIC/AUTORCC enabled
* **Includes**: Order: system includes, Qt includes, local includes (blank line between groups)
* **Namespaces**: Use `vnotex` namespace for all core classes; `using namespace vnotex;` in .cpp files
* **Headers**: Use header guards (`#ifndef CLASSNAME_H`), forward declarations preferred in headers
* **Classes**: CamelCase, private inheritance for utility classes (Noncopyable pattern)
* **Methods**: camelCase with getter prefixes (`getInst()`, `getThemeMgr()`), parameters prefixed with `p_`
* **Members**: Prefix private members with `m_` (e.g., `m_jobj`, `m_themeMgr`)
* **Singletons**: Use static instance pattern: `static Type &getInst() { static Type inst; return inst; }`
* **Qt Integration**: Use signals/slots, QObject inheritance, Q_OBJECT macro for MOC
* **Memory**: Use Qt smart pointers (QScopedPointer, QSharedPointer), not raw new/delete
* **Config**: Access via ConfigMgr::getInst(), JSON-based hierarchical config system
* **Architecture**: Layered structure - core/ for logic, widgets/ for UI, utils/ for helpers
* **Error Handling**: Use custom Exception class with Exception::throwOne(), qCritical/qWarning/qInfo for logging
* **Formatting**: 2-space indent, 100 char line limit, pointer alignment right (e.g., `int *ptr`)

## Architecture Notes

* Core singleton VNoteX coordinates all managers (ThemeMgr, TaskMgr, NotebookMgr, BufferMgr)
* ConfigMgr provides MainConfig, SessionConfig, CoreConfig, EditorConfig, WidgetConfig
* Test framework uses Qt Test (QObject-based tests with initTestCase() and test methods)