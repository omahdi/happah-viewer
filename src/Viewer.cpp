// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <happah/format.h>
#include <happah/geometries/converters.h>
#include <happah/geometries/LoopBoxSplineMesh.h>
#include <happah/graphics.hpp>
#include <happah/math/Space.h>
#include <GLFW/glfw3.h>//NOTE: Glad must be included before GLFW.
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
     auto boxes = make_loop_box_spline_mesh(mesh);
     auto quartic = make_spline_surface(TriangleMesh<VertexP3, Format::DIRECTED_EDGE>(mesh));
     //auto mesh = make_triangle_mesh(quartic, 4);
     auto quintic = elevate(quartic);

     look_at(viewport, mesh.getVertices());

     std::cout << "INFO: Making shaders." << std::endl;

     auto sm_vx = make_simple_vertex_shader();
     auto nm_gm = make_geometry_shader("shaders/normals.g.glsl");
     auto sm_fr = make_simple_fragment_shader();
     auto hl_fr = make_highlight_lines_fragment_shader();
     auto lb_te = make_tessellation_evaluation_shader("shaders/loop-box-spline.te.glsl");
     auto qp_te = make_tessellation_evaluation_shader("shaders/quintic-patch.te.glsl");
     auto wf_gm = make_geometry_shader("shaders/wireframe.g.glsl");
     auto wf_fr = make_wireframe_fragment_shader();

     std::cout << "INFO: Making programs." << std::endl;

     auto qpp = make_patches_program("quintic spline surface", 21, sm_vx, qp_te, hl_fr);
     auto qpc = make_render_context(quintic);
     auto tmp = make_triangles_program("triangle mesh", sm_vx, nm_gm, sm_fr);
     auto tmc = make_render_context(mesh);
     auto lmp = make_patches_program("loop box spline mesh", 12, sm_vx, lb_te, nm_gm, sm_fr);
     auto lmc = make_render_context(boxes);
     auto wfp = make_triangles_program("wireframe triangle mesh", sm_vx, wf_gm, wf_fr);

     std::cout << "INFO: Making vertex array." << std::endl;

     auto position = make_attribute(0, 4, { GL_FLOAT, sizeof(Glfloat) });
     auto array = make_vertex_array(position);

     std::cout << "INFO: Setting up scene." << std::endl;

     auto beamDirection = Vector3D(0.0, 0.0, 1.0);
     auto beamOrigin = Point3D(10.0, 0.0, 0.0);
     auto edgeColor = hpcolor(1, 0, 0, 1);
     auto edgeWidth = 0.01;
     auto level0 = std::array<hpreal, 2>({ 100, 100 });
     auto level1 = std::array<hpreal, 4>({ 60, 60, 60, 60 });
     auto modelColor = hpcolor(0, 0, 1, 1);

     std::cout << "INFO: Rendering scene." << std::endl;

     glClearColor(1, 1, 1, 1);
     while(!glfwWindowShouldClose(context)) {
          glfwPollEvents();
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
          glEnable(GL_DEPTH_TEST);

          auto& projectionMatrix = viewport.getProjectionMatrix();
          auto& viewMatrix = viewport.getViewMatrix();
          auto light = glm::normalize(Point3D(viewMatrix[0]));
          auto tempDirection = viewMatrix * Vector4D(beamDirection, 0.0);
          auto tempOrigin = viewMatrix * Point4D(beamOrigin, 1.0);

          activate(array);

          /*activate(qpp);
          sm_vx.setModelViewMatrix(viewMatrix);
          sm_vx.setProjectionMatrix(projectionMatrix);
          TessellationControlShader::setInnerTessellationLevel(level0);
          TessellationControlShader::setOuterTessellationLevel(level1);
          hl_fr.setBandColor0({ 1.0, 0.0, 0.0, 1.0 });
          hl_fr.setBandColor1({ 0.0, 1.0, 0.0, 1.0 });
          hl_fr.setBandWidth(1.0);
          hl_fr.setBeam(Point3D(tempOrigin) / tempOrigin.w, glm::normalize(Vector3D(tempDirection)));
          hl_fr.setLight(light);
          render(qpp, array, qpc);*/

          /*activate(tmp);
          sm_vx.setModelViewMatrix(viewMatrix);
          sm_vx.setProjectionMatrix(projectionMatrix);
          sm_fr.setLight(light);
          sm_fr.setModelColor(modelColor);
          render(tmp, array, tmc);*/

          activate(wfp);
          sm_vx.setModelViewMatrix(viewMatrix);
          sm_vx.setProjectionMatrix(projectionMatrix);
          wf_fr.setEdgeColor(edgeColor);
          wf_fr.setEdgeWidth(edgeWidth);
          wf_fr.setLight(light);
          wf_fr.setModelColor(modelColor);
          render(wfp, array, tmc);

          /*activate(lmp);
          sm_vx.setModelViewMatrix(viewMatrix);
          sm_vx.setProjectionMatrix(projectionMatrix);
          sm_fr.setLight(light);
          sm_fr.setModelColor(modelColor);
          render(lmp, array, lmc);*/

          glfwSwapBuffers(context);
     }
}

}//namespace happah

