// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

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

     void onCursorPosEvent(double x, double y);

     void onFramebufferSizeEvent(hpuint width, hpuint height);

     void onMouseButtonEvent(hpint button, hpint action, hpint mods);

     void onWindowSizeEvent(hpuint width, hpuint height);

     friend void onCursorPosEvent(GLFWwindow* handle, double x, double y);

     friend void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height);

     friend void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods);

     friend void onWindowSizeEvent(GLFWwindow* handle, int width, int height);

};//class Window

void onCursorPosEvent(GLFWwindow* handle, double x, double y);

void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height);

void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods);

void onWindowSizeEvent(GLFWwindow* handle, int width, int height);

}//namespace happah
