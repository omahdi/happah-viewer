// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
//   Hedwig Amberg  - Karlsruhe Institute of Technology - hedwigdorothea@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2017.06 - Hedwig Amberg    - Modified scroll to zoom or walk and activated arrow keys for panning.

#pragma once

#include <happah/Happah.hpp>
#include <happah/math/Space.hpp>
#include <happah/graphics/glad.h>
#include <happah/graphics/Viewport.hpp>
#include <GLFW/glfw3.h>//NOTE: Glad must be included before GLFW.
#include <array>
#include <tuple>
#include <utility>
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
     double m_mousetrans_sensitivity {0.003};
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

public:
     enum class RenderToggle {
          _NIL = 0,
          WIREFRAME,
          CHECKERBOARD,
		  ALPHA_BLENDING,
		  DEPTH_TEST,
          _COUNT
     };
     static_assert(static_cast<int>(RenderToggle::_COUNT) <= 32, "too many RenderToggle items");

     void disable(RenderToggle _what) {
          assert(_what > RenderToggle::_NIL && _what < RenderToggle::_COUNT);
          m_renderToggle &= ~(1 << static_cast<int>(_what));
     }
     void enable(RenderToggle _what) {
          assert(_what > RenderToggle::_NIL && _what < RenderToggle::_COUNT);
          m_renderToggle |= (1 << static_cast<int>(_what));
     }
     void toggle(RenderToggle _what) {
          assert(_what > RenderToggle::_NIL && _what < RenderToggle::_COUNT);
          m_renderToggle ^= (1 << static_cast<int>(_what));
     }
     bool enabled(RenderToggle _what) const {
          assert(_what > RenderToggle::_NIL && _what < RenderToggle::_COUNT);
          return (m_renderToggle & (1 << static_cast<int>(_what))) != 0;
     }
     void clearRenderToggles() {
          m_renderToggle = 0;
     }
     void setRenderToggles() {
          clearRenderToggles();
          m_renderToggle = ~m_renderToggle;
     }
     void setQuitFlag() { m_quitFlag = true; }
     void clearQuitFlag() { m_quitFlag = false; }
     bool quitFlag() const { return m_quitFlag; }
     void setHome() {
          auto eye = m_viewport.getEye();
          m_homeCenter = std::get<0>(eye);
          m_homePosition = std::get<1>(eye);
          m_homeUp = std::get<2>(eye);
     }
     void home() {
          m_viewport.setEye(m_homeCenter, m_homePosition, m_homeUp);
     }
private:
     bool m_quitFlag {false};
     std::uint32_t m_renderToggle {0};
     std::array<std::tuple<Point3D, Point3D, Point3D>, 9> m_eye {};
     Point3D m_homeCenter {0.0, 0.0, 0.0};
     Point3D m_homePosition {0.0, 0.0, 1.0};
     Point3D m_homeUp {0.0, 1.0, 0.0};
public:
     hpreal m_varEdgeWidth {1.0};
     hpreal m_varDepthScale {1.0};
};//Window

}//namespace happah

