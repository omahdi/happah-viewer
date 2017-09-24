// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
//   Hedwig Amberg  - Karlsruhe Institute of Technology - hedwigdorothea@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2017.06 - Hedwig Amberg    - Modified scroll to zoom or walk and activated arrow keys for panning.

#pragma once

#include <happah/Happah.hpp>
#include <happah/graphics/glad.h>
#include <happah/graphics/Viewport.hpp>
#include <GLFW/glfw3.h>//NOTE: Glad must be included before GLFW.
#include <string>
#include <unordered_map>

namespace happah {

//DECLARATIONS

class Window;

inline void onCursorPosEvent(GLFWwindow* handle, double x, double y);

inline void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height);
     
inline void onKeyEvent(GLFWwindow* handle, int key, int code, int action, int mods);

inline void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods);
     
inline void onScrollEvent(GLFWwindow* handle, double xoffset, double yoffset);

inline void onWindowSizeEvent(GLFWwindow* handle, int width, int height);

//DEFINITIONS

class Window {
public:
     Window(hpuint width, hpuint height, const std::string& title);

     ~Window();

     GLFWwindow* getContext() const { return m_handle; }

     Viewport& getViewport() { return m_viewport; }

private:
     static std::unordered_map<GLFWwindow*, Window*>& cache() {
          static std::unordered_map<GLFWwindow*, Window*> s_windows;
          return s_windows;
     }

     bool m_ctrlPressed = false;
     GLFWwindow* m_handle;
     hpreal m_delta = hpreal(0.1);
     Viewport m_viewport;
     double m_x;//mouse coordinates
     double m_y;
     double m_mousetrans_sensitivity {0.01};
     bool m_mousezoom {false};
     Point3D m_mousezoom_eye_center, m_mousezoom_eye_position;
     Vector3D m_mousezoom_eye_up;
     double m_mousezoom_sensitivity {0.01};
     double m_mousezoom_x, m_mousezoom_y;
     
     void onCursorPosEvent(double x, double y);

     void onFramebufferSizeEvent(hpuint width, hpuint height) { glViewport(0, 0, width, height); }
     
     void onKeyEvent(int key, int code, int action, int mods);

     void onMouseButtonEvent(hpint button, hpint action, hpint mods);
     
     void onScrollEvent(double xoffset, double yoffset);

     void onWindowSizeEvent(hpuint width, hpuint height) { m_viewport.setSize(width, height); }

     friend void onCursorPosEvent(GLFWwindow* handle, double x, double y);

     friend void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height);
     
     friend void onKeyEvent(GLFWwindow* handle, int key, int code, int action, int mods);

     friend void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods);
     
     friend void onScrollEvent(GLFWwindow* handle, double xoffset, double yoffset);

     friend void onWindowSizeEvent(GLFWwindow* handle, int width, int height);

};//Window

inline void onCursorPosEvent(GLFWwindow* handle, double x, double y) { Window::cache()[handle]->onCursorPosEvent(x, y); }

inline void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height) { Window::cache()[handle]->onFramebufferSizeEvent(width, height); }
     
inline void onKeyEvent(GLFWwindow* handle, int key, int code, int action, int mods){ Window::cache()[handle]->onKeyEvent(key, code, action, mods); }

inline void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods) { Window::cache()[handle]->onMouseButtonEvent(button, action, mods); }
     
inline void onScrollEvent(GLFWwindow* handle, double xoffset, double yoffset){ Window::cache()[handle]->onScrollEvent(xoffset, yoffset); } 

inline void onWindowSizeEvent(GLFWwindow* handle, int width, int height) { Window::cache()[handle]->onWindowSizeEvent(width, height); }

}//namespace happah

