#include "targetver.h"

#define GLEW_STATIC
#include "oglplus/gl.hpp"
#include "oglplus/all.hpp"

#include "glm/glm.hpp"
#include "sfml/Window.hpp"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <set>


#define MinLogLevel LogBase::Debug

