#include "drivers.h"

using namespace kernel;

std::shared_ptr<DriverManager> DriverManager::_root;
std::shared_ptr<DriverManager> DriverManager::_current;
