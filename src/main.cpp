// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "Viewer.hpp"

int main(int argc, char* argv[]) {
     // init glfw
     if(!glfwInit()) {
          std::cerr << "Failed to initialize GLFW.\n";
          return 1;
     }

     try {
          auto viewer = happah::Viewer(640, 480, "Happah Viewer");
          viewer.execute(argc, argv);
     } catch(std::exception& e) {
          std::cerr << e.what() << '\n';
          return 1;
     }

     glfwTerminate();
     return 0;
}

