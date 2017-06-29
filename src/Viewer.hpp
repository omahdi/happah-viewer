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

     bool render_solid_mesh {false};
     bool render_solid_tris {false};
     bool render_quintic {false};
     bool render_loop_box_spline {false};
     bool render_point_cloud {false};
     bool render_wireframe {true};
     bool render_checkerboard {true};
};//class Viewer

}//namespace happah

