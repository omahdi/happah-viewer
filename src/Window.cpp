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
          if (!m_ctrlPressed)
               m_viewport.rotate(m_x, m_y, x, y);
          else
               m_viewport.translate(Vector2D(x - m_x, y - m_y)*hpreal(m_mousetrans_sensitivity));
          m_x = x;
          m_y = y;
     }
     else if(!m_mousezoom && glfwGetMouseButton(m_handle, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
          double dist = y-m_mousezoom_y;
          m_viewport.setEye(m_mousezoom_eye_center, m_mousezoom_eye_position, m_mousezoom_eye_up);
          m_viewport.translate(Vector3D(0.0, 0.0, dist * m_mousezoom_sensitivity));
          //std::cout << "cursorPosEvent: zoom: dist=" << dist << ", sensitivity=" << m_mousezoom_sensitivity << "\n";
     }
// Note: Test m_mousezoom to ensure m_mousezoom_eye is set up properly in case
// this handler is served before onMouseButtonEvent().
     else if(m_mousezoom && glfwGetMouseButton(m_handle, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
          double dist = y-m_mousezoom_y;
          m_viewport.setEye(m_mousezoom_eye_center, m_mousezoom_eye_position, m_mousezoom_eye_up);
          m_viewport.zoom(std::min(dist * m_mousezoom_sensitivity, 0.99));
          //std::cout << "cursorPosEvent: zoom: dist=" << dist << ", sensitivity=" << m_mousezoom_sensitivity << "\n";
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
     const auto record_cursor_pos = [this] (auto& x, auto& y) {
          glfwGetCursorPos(this->m_handle, &x, &y);
          y = this->m_viewport.getHeight() - y;
     };
     if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
          record_cursor_pos(m_x, m_y);
     } else if(button == GLFW_MOUSE_BUTTON_RIGHT) {
          record_cursor_pos(m_x, m_y);
          if (action == GLFW_PRESS && !m_mousezoom) {
               record_cursor_pos(m_mousezoom_x, m_mousezoom_y);
               m_mousezoom_eye_center = m_viewport.getCenter();
               m_mousezoom_eye_position = m_viewport.getEyePosition();
               m_mousezoom_eye_up = m_viewport.getUp();
               m_mousezoom = true;
          } else if (action == GLFW_RELEASE)
               m_mousezoom = false;
     }
}

void Window::onScrollEvent(double xoffset, double yoffset) {
     if(m_ctrlPressed) m_viewport.zoom(yoffset * m_delta * 0.1);
     else {
          auto delta = hpreal(yoffset) * m_delta;
          auto direction = glm::normalize(make_view_direction(m_viewport));
          m_viewport.translate(delta * direction);
     }
}

}//namespace happah
