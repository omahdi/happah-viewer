// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
//   Hedwig Amberg  - Karlsruhe Institute of Technology - hedwigdorothea@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2017.06 - Hedwig Amberg    - modified scroll to zoom or walk, arrow keys for strafing.

#pragma once

#include <happah/Happah.hpp>
#include <happah/graphics/glad.h>
#include <happah/graphics/Viewport.hpp>
#include <GLFW/glfw3.h>//NOTE: Glad must be included before GLFW.
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace happah {

//DECLARATIONS

class Window;

inline void onCursorPosEvent(GLFWwindow* handle, double x, double y);

inline void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height);
     
inline void onKeyEvent(GLFWwindow* handle, int key, int scancode, int action, int mods);

inline void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods);
     
inline void onScrollEvent(GLFWwindow* handle, double xoffset, double yoffset);

inline void onWindowSizeEvent(GLFWwindow* handle, int width, int height);

//DEFINITIONS

class Window {
public:
     Window(hpuint width, hpuint height, const std::string& title)
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

     ~Window() {
          cache().erase(m_handle);
          glfwDestroyWindow(m_handle);
     }

     GLFWwindow* getContext() const { return m_handle; }

     Viewport& getViewport() { return m_viewport; }

private:
     static std::unordered_map<GLFWwindow*, Window*>& cache() {
          static std::unordered_map<GLFWwindow*, Window*> s_windows;
          return s_windows;
     }

     GLFWwindow* m_handle;
     Viewport m_viewport;
     double m_x;//mouse coordinates
     double m_y;
     
     bool ctrlPressed = false;

     void onCursorPosEvent(double x, double y) {
          y = m_viewport.getHeight() - y;
          if(glfwGetMouseButton(m_handle, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
               m_viewport.rotate(m_x, m_y, x, y);
               m_x = x;
               m_y = y;
          }
     }

     void onFramebufferSizeEvent(hpuint width, hpuint height) { glViewport(0, 0, width, height); }
     
     void onKeyEvent(int key, int scancode, int action, int mods) {
          if( (key == GLFW_KEY_LEFT_CONTROL) || (key == GLFW_KEY_RIGHT_CONTROL) ){
               ctrlPressed = (action != GLFW_RELEASE);
          }
     }

     void onMouseButtonEvent(hpint button, hpint action, hpint mods) {
          if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
               glfwGetCursorPos(m_handle, &m_x, &m_y);
               m_y = m_viewport.getHeight() - m_y;
          }
     }
     
     void onScrollEvent(double xoffset, double yoffset) {
          if(ctrlPressed) m_viewport.zoom(yoffset * 0.01);
          else {
               float delta = yoffset * 0.1;
               Point3D viewDir = m_viewport.getViewDirection();
               const Vector3D& step = Vector3D(delta * viewDir.x, delta * viewDir.y, delta * viewDir.z);
               m_viewport.translate(step);
          }
     }

     void onWindowSizeEvent(hpuint width, hpuint height) { m_viewport.setSize(width, height); }

     friend void onCursorPosEvent(GLFWwindow* handle, double x, double y);

     friend void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height);
     
     friend void onKeyEvent(GLFWwindow* handle, int key, int scancode, int action, int mods);

     friend void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods);
     
     friend void onScrollEvent(GLFWwindow* handle, double xoffset, double yoffset);

     friend void onWindowSizeEvent(GLFWwindow* handle, int width, int height);

};//Window

inline void onCursorPosEvent(GLFWwindow* handle, double x, double y) { Window::cache()[handle]->onCursorPosEvent(x, y); }

inline void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height) { Window::cache()[handle]->onFramebufferSizeEvent(width, height); }
     
inline void onKeyEvent(GLFWwindow* handle, int key, int scancode, int action, int mods){ Window::cache()[handle]->onKeyEvent(key, scancode, action, mods); }

inline void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods) { Window::cache()[handle]->onMouseButtonEvent(button, action, mods); }
     
inline void onScrollEvent(GLFWwindow* handle, double xoffset, double yoffset){ Window::cache()[handle]->onScrollEvent(xoffset, yoffset); } 

inline void onWindowSizeEvent(GLFWwindow* handle, int width, int height) { Window::cache()[handle]->onWindowSizeEvent(width, height); }

}//namespace happah

