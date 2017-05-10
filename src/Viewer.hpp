// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <happah/Happah.h>
#include <string>

#include "Window.hpp"

namespace happah {

class Viewer {
public:
     Viewer(hpuint width, hpuint height, const std::string& title);

     void execute(int argc, char* argv[]);

private:
     Window m_window;

};//class Viewer

}//namespace happah

