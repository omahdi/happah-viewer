// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
//   Hedwig Amberg  - Karlsruhe Institute of Technology - hedwigdorothea@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2017.06 - Hedwig Amberg    - modified scroll to zoom or walk, arrow keys for strafing.

#include <stdexcept>

#include <happah/format.hpp>

#include "Window.hpp"

namespace happah {

void onCursorPosEvent(GLFWwindow* handle, double x, double y) { Window::cache()[handle]->onCursorPosEvent(x, y); }

void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height) { Window::cache()[handle]->onFramebufferSizeEvent(width, height); }
     
void onKeyEvent(GLFWwindow* handle, int key, int code, int action, int mods){ Window::cache()[handle]->onKeyEvent(key, code, action, mods); }

void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods) { Window::cache()[handle]->onMouseButtonEvent(button, action, mods); }
     
void onScrollEvent(GLFWwindow* handle, double xoffset, double yoffset){ Window::cache()[handle]->onScrollEvent(xoffset, yoffset); } 

void onWindowSizeEvent(GLFWwindow* handle, int width, int height) { Window::cache()[handle]->onWindowSizeEvent(width, height); }

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
	enable(RenderToggle::WIREFRAME);
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
               m_viewport.translate(Vector2D(x - m_x, m_y - y)*hpreal(m_mousetrans_sensitivity));
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
     using std::get;
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
     if (action == GLFW_PRESS) {
          unsigned i = 0;
          switch (key) {
          case GLFW_KEY_ESCAPE: setQuitFlag(); break;
          case GLFW_KEY_X: clearRenderToggles(); break;
          case GLFW_KEY_W: toggle(RenderToggle::WIREFRAME); break;
          case GLFW_KEY_Q: toggle(RenderToggle::CHECKERBOARD); break;
          case GLFW_KEY_0: home(); break;
          case GLFW_KEY_9: i++;
          case GLFW_KEY_8: i++;
          case GLFW_KEY_7: i++;
          case GLFW_KEY_6: i++;
          case GLFW_KEY_5: i++;
          case GLFW_KEY_4: i++;
          case GLFW_KEY_3: i++;
          case GLFW_KEY_2: i++;
          case GLFW_KEY_1: {
		     if ((mods & GLFW_MOD_SHIFT) != 0) {
		          m_eye[i] = m_viewport.getEye();
		     } else {
                    if (glm::length(get<2>(m_eye[i])) < 1e-6)
                         std::cerr << "Error: Refusing to recall uninitialized view matrix #" << i << ".\n";
                    else
                         m_viewport.setEye(get<0>(m_eye[i]), get<1>(m_eye[i]), get<2>(m_eye[i]));
               }
               break;
          }
          case GLFW_KEY_S: {
               if ((mods & GLFW_MOD_CONTROL) != 0) {
                    std::cout << "Storing camera view records...\n";
                    std::vector<Point3D> buf;
                    buf.reserve(3*m_eye.size());
                    for (const auto& eye : m_eye) {
                         buf.push_back(get<0>(eye));
                         buf.push_back(get<1>(eye));
                         buf.push_back(get<2>(eye));
                    }
                    format::hph::write(buf, p("stored-views.hph"));
               }
               break;
          }
          case GLFW_KEY_L: {
               if ((mods & GLFW_MOD_CONTROL) != 0) {
                    std::cout << "Loading camera view records...\n";
                    auto buf = format::hph::read<std::vector<Point3D>>(p("stored-views.hph"));
                    for (unsigned i = 0; i < m_eye.size(); i++) {
                         get<0>(m_eye[i]) = buf[3*i + 0];
                         get<1>(m_eye[i]) = buf[3*i + 1];
                         get<2>(m_eye[i]) = buf[3*i + 2];
                    }
               }
          }
          case GLFW_KEY_B: {
		     if ((mods & GLFW_MOD_SHIFT) != 0)
                    toggle(RenderToggle::ALPHA_BLENDING);
               break;
          }
          case GLFW_KEY_D: {
		     if ((mods & GLFW_MOD_SHIFT) != 0)
                    toggle(RenderToggle::DEPTH_TEST);
               break;
          }
          case GLFW_KEY_T: {
		     if ((mods & GLFW_MOD_SHIFT) != 0) {
                    enable(RenderToggle::ALPHA_BLENDING);
                    disable(RenderToggle::DEPTH_TEST);
               } else {
                    disable(RenderToggle::ALPHA_BLENDING);
                    enable(RenderToggle::DEPTH_TEST);
               }
               break;
          }
          }
     }
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
