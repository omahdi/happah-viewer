// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
//   Hedwig Amberg  - Karlsruhe Institute of Technology - hedwigdorothea@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2017.06 - Hedwig Amberg    - modified scroll to zoom or walk, arrow keys for strafing.

#pragma once

#include <happah/Happah.h>
#include <happah/graphics/glad.h>
#include <happah/graphics/Viewport.hpp>
#include <GLFW/glfw3.h>//NOTE: Glad must be included before GLFW.
#include <string>
#include <unordered_map>

namespace happah {

class Window {
public:
     Window(hpuint width, hpuint height, const std::string& title);

     ~Window();

     GLFWwindow* getContext() const;

     Viewport& getViewport();

private:
     static std::unordered_map<GLFWwindow*, Window*> s_windows;

     GLFWwindow* m_handle;
     Viewport m_viewport;
     double m_x;//mouse coordinates
     double m_y;
     
     bool ctrlPressed = false;

     void onCursorPosEvent(double x, double y);

     void onFramebufferSizeEvent(hpuint width, hpuint height);
     
     void onKeyEvent(int key, int scancode, int action, int mods);

     void onMouseButtonEvent(hpint button, hpint action, hpint mods);
     
     void onScrollEvent(double xoffset, double yoffset);

     void onWindowSizeEvent(hpuint width, hpuint height);

     friend void onCursorPosEvent(GLFWwindow* handle, double x, double y);

     friend void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height);
     
     friend void onKeyEvent(GLFWwindow* handle, int key, int scancode, int action, int mods);

     friend void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods);
     
     friend void onScrollEvent(GLFWwindow* handle, double xoffset, double yoffset);

     friend void onWindowSizeEvent(GLFWwindow* handle, int width, int height);

};//class Window

void onCursorPosEvent(GLFWwindow* handle, double x, double y);

void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height);
     
void onKeyEvent(GLFWwindow* handle, int key, int scancode, int action, int mods);

void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods);
     
void onScrollEvent(GLFWwindow* handle, double xoffset, double yoffset);

void onWindowSizeEvent(GLFWwindow* handle, int width, int height);

}//namespace happah
