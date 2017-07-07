// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <happah/format.h>
#include <happah/geometries/converters.h>
#include <happah/geometries/LoopBoxSplineMesh.h>
#include <happah/geometries/TriangleArray.h>
#include <happah/graphics.hpp>
#include <happah/math/Space.h>
#include <GLFW/glfw3.h>//NOTE: Glad must be included before GLFW.
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>

#include "Viewer.hpp"

namespace happah {

Viewer::Viewer(hpuint width, hpuint height, const std::string& title)
     : m_window(width, height, title) {
     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);//TODO: find current version
     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
     glfwMakeContextCurrent(m_window.getContext());
     if(!gladLoadGL()) throw std::runtime_error("Failed to initialize Glad.");
}

void Viewer::execute(int argc, char* argv[]) {
     auto context = m_window.getContext();
     auto& viewport = m_window.getViewport();

     std::cout << "INFO: Importing " << argv[1] << '.' << std::endl;

     auto content = format::off::read(argv[1]);
     auto mesh = make_triangle_mesh<VertexP3>(content);
     auto graph = TriangleMesh<VertexP3, Format::DIRECTED_EDGE>(mesh);
     auto edgeColors = std::vector<hpcolor>(3 * size(mesh), hpcolor(0.0, 0.0, 1.0, 1.0));

     std::cout << "INFO: Making shaders." << std::endl;

     auto wf_fr = make_wireframe_fragment_shader();
     auto wf_gm = make_geometry_shader("shaders/wireframe.g.glsl");
     auto wf_vx = make_wireframe_vertex_shader();

     std::cout << "INFO: Making programs." << std::endl;

     auto lmp = make_program("loop box spline mesh", sm_vx, lb_te, nm_gm, sm_fr);
     auto qpp = make_program("quintic spline surface", sm_vx, qp_te, hl_fr);
     auto pcp = make_program("point cloud", sm_vx, si_gm, si_fr);
     auto tmp = make_program("triangle mesh", sm_vx, nm_gm, sm_fr);
     auto wfp = make_program("wireframe triangle mesh", wf_vx, wf_gm, wf_fr);

     std::cout << "INFO: Making buffers." << std::endl;

     auto bv0 = make_buffer(mesh.getVertices());
     auto bv1 = make_buffer(quintic.getControlPoints());
     auto bv2 = make_buffer(boxes.getControlPoints());
     auto bv3 = make_buffer(triangles.getVertices());
     auto bi0 = make_buffer(mesh.getIndices());
     auto bi1 = make_buffer(std::get<1>(quintic.getPatches()));
     auto bi2 = make_buffer(boxes.getIndices());
     auto be3 = make_buffer(edgeColors);

     std::cout << "INFO: Making vertex arrays." << std::endl;

     auto position = make_attribute(0, 4, DataType::FLOAT);
     auto edgeColor = make_attribute(1, 4, DataType::FLOAT);

     auto va0 = make_vertex_array();
     auto va1 = make_vertex_array();

     describe(va0, 0, position);
     describe(va1, 0, position);
     describe(va1, 1, edgeColor);

     std::cout << "INFO: Making render contexts." << std::endl;

     auto rc0 = make_render_context(va0, bi0, PatchType::TRIANGLE);
     auto rc1 = make_render_context(va0, bi1, PatchType::QUINTIC);
     auto rc2 = make_render_context(va0, bi2, PatchType::LOOP_BOX_SPLINE);
     auto rc30 = make_render_context(va0, PatchType::TRIANGLE);
     auto rc31 = make_render_context(va1, PatchType::TRIANGLE);
     auto rc4 = make_render_context(va0, PatchType::POINT);

     std::cout << "INFO: Setting up scene." << std::endl;

     auto blue = hpcolor(0.0, 0.0, 1.0, 1.0);
     auto edgeWidth = 0.02;
     auto green = hpcolor(0.0, 1.0, 0.0, 1.0);
     auto level0 = std::array<hpreal, 2>({ 100, 100 });
     auto level1 = std::array<hpreal, 4>({ 60, 60, 60, 60 });
     auto radius = 0.05;
     auto red = hpcolor(1.0, 0.0, 0.0, 1.0);

     std::cout << "INFO: Rendering scene." << std::endl;

     glfwSwapInterval(1);
     look_at(viewport, mesh.getVertices());
     glClearColor(1, 1, 1, 1);
     while(!glfwWindowShouldClose(context) && !m_window.quit_flag()) {
          glfwPollEvents();
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
          glEnable(GL_DEPTH_TEST);

          auto& projectionMatrix = viewport.getProjectionMatrix();
          auto& viewMatrix = viewport.getViewMatrix();
          auto light = glm::normalize(Point3D(viewMatrix[0]));
          auto tempDirection = viewMatrix * Vector4D(beamDirection, 0.0);
          auto tempOrigin = viewMatrix * Point4D(beamOrigin, 1.0);

          activate(va0);

          if (m_window.enabled(Window::RenderToggle::WIREFRAME)) {
               activate(va1);
               activate(wfp);
               activate(bv3, va1, 0);
               activate(be3, va1, 1);
               wf_vx.setModelViewMatrix(viewMatrix);
               wf_vx.setProjectionMatrix(projectionMatrix);
               wf_fr.setEdgeColor(red);
               wf_fr.setEdgeWidth(edgeWidth);
               wf_fr.setLight(light);
               wf_fr.setModelColor(blue);
               render(wfp, rc31, size(triangles));
          }

          if (m_window.enabled(Window::RenderToggle::WIREFRAME)) {
          }

          glfwSwapBuffers(context);
          glFinish();
     }
}

}//namespace happah

