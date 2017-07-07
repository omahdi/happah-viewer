// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
//   Hedwig Amberg  - Karlsruhe Institute of Technology - hedwigdorothea@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2017.06 - Hedwig Amberg    - Added scroll zoom.

#include <stdexcept>

#include <happah/format.h>

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
     glfwSetKeyCallback(m_handle, happah::onKeyEvent);
     toggle(RenderToggle::WIREFRAME);
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

void Window::onKeyEvent(int key, int scancode, int action, int mods) {
     using std::get;
     if (action == GLFW_PRESS) {
          unsigned i = 0;
          switch (key) {
               case GLFW_KEY_ESCAPE: setQuitFlag(); break;
               case GLFW_KEY_X: clearRenderToggles(); break;
               case GLFW_KEY_W: toggle(RenderToggle::WIREFRAME); break;
               case GLFW_KEY_Q:
                    if ((mods & GLFW_MOD_SHIFT) != 0) {
                         disable(RenderToggle::CHECKERBOARD_EUC);
                         toggle(RenderToggle::CHECKERBOARD_HYP);
                    } else {
                         disable(RenderToggle::CHECKERBOARD_HYP);
                         toggle(RenderToggle::CHECKERBOARD_EUC);
                    }
                    break;
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
                         format::hph::write(buf, "stored-views.hph");
                    }
                    break;
               }
               case GLFW_KEY_L: {
                    if ((mods & GLFW_MOD_CONTROL) != 0) {
                         std::cout << "Loading camera view records...\n";
                         auto buf = format::hph::read<std::vector<Point3D>>("stored-views.hph");
                         for (unsigned i = 0; i < m_eye.size(); i++) {
                              get<0>(m_eye[i]) = buf[3*i + 0];
                              get<1>(m_eye[i]) = buf[3*i + 1];
                              get<2>(m_eye[i]) = buf[3*i + 2];
                         }
                    }
                    break;
               }
          }
     }
}

void onCursorPosEvent(GLFWwindow* handle, double x, double y) { Window::s_windows[handle]->onCursorPosEvent(x, y); }

void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height) { Window::s_windows[handle]->onFramebufferSizeEvent(width, height); }

void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods) { Window::s_windows[handle]->onMouseButtonEvent(button, action, mods); }

void onScrollEvent(GLFWwindow* handle, double xoffset, double yoffset){ Window::s_windows[handle]->onScrollEvent(xoffset, yoffset); }

void onWindowSizeEvent(GLFWwindow* handle, int width, int height) { Window::s_windows[handle]->onWindowSizeEvent(width, height); }

void onKeyEvent(GLFWwindow* handle, int key, int scancode, int action, int mods) {
     Window::s_windows[handle]->onKeyEvent(key, scancode, action, mods);
}
}//namespace happah

