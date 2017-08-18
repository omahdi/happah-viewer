// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
//   Hedwig Amberg  - Karlsruhe Institute of Technology - hedwigdorothea@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2017.06 - Hedwig Amberg    - modified scroll to zoom or walk, arrow keys for strafing.

#include <stdexcept>

#include "Window.hpp"

namespace happah {

Window::Window(hpuint width, hpuint height, const std::string& title)
     : m_handle(glfwCreateWindow(width, height, title.c_str(), 0, 0)), m_viewport(width, height) {
     if(m_handle == 0) throw std::runtime_error("Failed to create window.");
     cache()[m_handle] = this;
     glfwSetCursorPosCallback(m_handle, happah::onCursorPosEvent);
     glfwSetFramebufferSizeCallback(m_handle, happah::onFramebufferSizeEvent);
     glfwSetKeyCallback(m_handle, happah::onKeyEvent);
     glfwSetMouseButtonCallback(m_handle, happah::onMouseButtonEvent);
     glfwSetScrollCallback(m_handle, happah::onScrollEvent);
     glfwSetWindowSizeCallback(m_handle, happah::onWindowSizeEvent);
}

Window::~Window() {
     cache().erase(m_handle);
     glfwDestroyWindow(m_handle);
}

void Window::onCursorPosEvent(double x, double y) {
     y = m_viewport.getHeight() - y;
     if(glfwGetMouseButton(m_handle, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
          m_viewport.rotate(m_x, m_y, x, y);
          m_x = x;
          m_y = y;
     }
}

void Window::onKeyEvent(int key, int code, int action, int mods) {
     switch(key) {
     case GLFW_KEY_LEFT_CONTROL:
     case GLFW_KEY_RIGHT_CONTROL:
          m_ctrlPressed = (action != GLFW_RELEASE);
          break;
     case GLFW_KEY_UP:
          if(action == GLFW_PRESS || action == GLFW_REPEAT) m_viewport.translate(Vector2D(0, m_delta));
          break;
     case GLFW_KEY_DOWN:
          if(action == GLFW_PRESS || action == GLFW_REPEAT) m_viewport.translate(Vector2D(0, -m_delta));
          break;
     case GLFW_KEY_LEFT:
          if(action == GLFW_PRESS || action == GLFW_REPEAT) m_viewport.translate(Vector2D(m_delta, 0));
          break;
     case GLFW_KEY_RIGHT:
          if(action == GLFW_PRESS || action == GLFW_REPEAT) m_viewport.translate(Vector2D(-m_delta, 0));
          break;
     };
}

void Window::onMouseButtonEvent(hpint button, hpint action, hpint mods) {
     if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
          glfwGetCursorPos(m_handle, &m_x, &m_y);
          m_y = m_viewport.getHeight() - m_y;
     }
}

void Window::onScrollEvent(double xoffset, double yoffset) {
     if(m_ctrlPressed) m_viewport.zoom(yoffset * 0.01);
     else {
          float delta = yoffset * 0.1;
          Point3D viewDir = m_viewport.getViewDirection();
          const Vector3D& step = Vector3D(delta * viewDir.x, delta * viewDir.y, delta * viewDir.z);
          m_viewport.translate(step);
     }
}

}//namespace happah
