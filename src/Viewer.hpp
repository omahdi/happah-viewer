// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
//   Hedwig Amberg  - Karlsruhe Institute of Technology - hedwigdorothea@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2017.07 - Hedwig Amberg    - added arrow key movement.

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
     
     float lastFrame;
     
     void movement(Viewport& viewport);

};//class Viewer

}//namespace happah

