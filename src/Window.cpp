// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
//   Hedwig Amberg  - Karlsruhe Institute of Technology - hedwigdorothea@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2017.06 - Hedwig Amberg    - Added scroll zoom.

#include <stdexcept>

#include "Window.hpp"

namespace happah {

std::unordered_map<GLFWwindow*, Window*> Window::s_windows = std::unordered_map<GLFWwindow*, Window*>();

Window::Window(hpuint width, hpuint height, const std::string& title) 
     : m_handle(glfwCreateWindow(width, height, title.c_str(), 0, 0)), m_viewport(width, height) {
     if(m_handle == 0) throw std::runtime_error("Failed to create window.");
     s_windows[m_handle] = this;
     glfwSetCursorPosCallback(m_handle, happah::onCursorPosEvent);
     glfwSetFramebufferSizeCallback(m_handle, happah::onFramebufferSizeEvent);
     glfwSetMouseButtonCallback(m_handle, happah::onMouseButtonEvent);
     glfwSetScrollCallback(m_handle, happah::onScrollEvent);
     glfwSetWindowSizeCallback(m_handle, happah::onWindowSizeEvent);
}

Window::~Window() {
     s_windows.erase(m_handle);
     glfwDestroyWindow(m_handle);
}

GLFWwindow* Window::getContext() const  { return m_handle; }

Viewport& Window::getViewport() { return m_viewport; }

void Window::onCursorPosEvent(double x, double y) {
     y = m_viewport.getHeight() - y;
     if(glfwGetMouseButton(m_handle, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
          m_viewport.rotate(m_x, m_y, x, y);
          m_x = x;
          m_y = y;
     }
}

void Window::onFramebufferSizeEvent(hpuint width, hpuint height) { glViewport(0, 0, width, height); }

void Window::onMouseButtonEvent(hpint button, hpint action, hpint mods) {
     if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
          glfwGetCursorPos(m_handle, &m_x, &m_y);
          m_y = m_viewport.getHeight() - m_y;
     }
}
     
void Window::onScrollEvent(double xoffset, double yoffset){
     m_viewport.zoom(yoffset * 0.01);
}

void Window::onWindowSizeEvent(hpuint width, hpuint height) { m_viewport.setSize(width, height); }

void onCursorPosEvent(GLFWwindow* handle, double x, double y) { Window::s_windows[handle]->onCursorPosEvent(x, y); }

void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height) { Window::s_windows[handle]->onFramebufferSizeEvent(width, height); }

void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods) { Window::s_windows[handle]->onMouseButtonEvent(button, action, mods); }
     
void onScrollEvent(GLFWwindow* handle, double xoffset, double yoffset){ Window::s_windows[handle]->onScrollEvent(xoffset, yoffset); } 

void onWindowSizeEvent(GLFWwindow* handle, int width, int height) { Window::s_windows[handle]->onWindowSizeEvent(width, height); }

}//namespace happah

