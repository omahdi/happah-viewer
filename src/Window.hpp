// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
//   Hedwig Amberg  - Karlsruhe Institute of Technology - hedwigdorothea@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2017.06 - Hedwig Amberg    - Added scroll zoom.

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
     enum class RenderToggle {
          _NIL = 0,
          QUINTIC,
          LOOP_BOX_SPLINE,
          SOLID_MESH,
          SOLID_TRIS,
          POINT_CLOUD,
          WIREFRAME,
          CHECKERBOARD,
          _COUNT
     };
     static_assert(static_cast<int>(RenderToggle::_COUNT) <= 32);

     void toggle(RenderToggle _what) {
          assert(_what > RenderToggle::_NIL && _what < RenderToggle::_COUNT);
          m_render_toggle ^= (1 << static_cast<int>(_what));
     }
     bool enabled(RenderToggle _what) const {
          assert(_what > RenderToggle::_NIL && _what < RenderToggle::_COUNT);
          return (m_render_toggle & (1 << static_cast<int>(_what))) != 0;
     }
     void clear_render_toggles() {
          m_render_toggle = 0;
     }
     void set_render_toggles() {
          clear_render_toggles();
          m_render_toggle = ~m_render_toggle;
     }
     void set_quit_flag() { m_quit_flag = true; }
     void clear_quit_flag() { m_quit_flag = false; }
     bool quit_flag() const { return m_quit_flag; }

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
     bool m_quit_flag {false};
     std::uint32_t m_render_toggle {0};

     void onCursorPosEvent(double x, double y);

     void onFramebufferSizeEvent(hpuint width, hpuint height);

     void onMouseButtonEvent(hpint button, hpint action, hpint mods);
     
     void onScrollEvent(double xoffset, double yoffset);

     void onWindowSizeEvent(hpuint width, hpuint height);

     void onKeyEvent(int key, int scancode, int action, int mods);

     friend void onCursorPosEvent(GLFWwindow* handle, double x, double y);

     friend void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height);

     friend void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods);
     
     friend void onScrollEvent(GLFWwindow* handle, double xoffset, double yoffset);

     friend void onWindowSizeEvent(GLFWwindow* handle, int width, int height);

     friend void onKeyEvent(GLFWwindow* handle, int key, int scancode, int action, int mods);
};//class Window

void onCursorPosEvent(GLFWwindow* handle, double x, double y);

void onFramebufferSizeEvent(GLFWwindow* handle, int width, int height);

void onMouseButtonEvent(GLFWwindow* handle, int button, int action, int mods);
     
void onScrollEvent(GLFWwindow* handle, double xoffset, double yoffset);

void onWindowSizeEvent(GLFWwindow* handle, int width, int height);

void onKeyEvent(GLFWwindow* handle, int key, int scancode, int action, int mods);
}//namespace happah
