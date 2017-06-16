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
     auto triangles = make_triangle_array(mesh);
     auto boxes = make_loop_box_spline_mesh(mesh);
     auto quartic = make_spline_surface(TriangleMesh<VertexP3, Format::DIRECTED_EDGE>(mesh));
     //auto mesh = make_triangle_mesh(quartic, 4);
     auto quintic = elevate(quartic);

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

     auto qpp = make_program("quintic spline surface", sm_vx, qp_te, hl_fr);
     auto tmp = make_program("triangle mesh", sm_vx, nm_gm, sm_fr);
     auto lmp = make_program("loop box spline mesh", sm_vx, lb_te, nm_gm, sm_fr);
     auto wfp = make_program("wireframe triangle mesh", sm_vx, wf_gm, wf_fr);

     std::cout << "INFO: Making buffers." << std::endl;

     auto bv0 = make_buffer(mesh.getVertices());
     auto bv1 = make_buffer(quintic.getControlPoints());
     auto bv2 = make_buffer(boxes.getControlPoints());
     auto bv3 = make_buffer(triangles.getVertices());
     auto bi0 = make_buffer(mesh.getIndices());
     auto bi1 = make_buffer(std::get<1>(quintic.getPatches()));
     auto bi2 = make_buffer(boxes.getIndices());

     std::cout << "INFO: Making vertex arrays." << std::endl;

     auto position = make_attribute(0, 4, DataType::FLOAT);
     auto edgeColor = make_attribute(1, 4, DataType::FLOAT);

     auto va0 = make_vertex_array(position);
     auto va1 = make_vertex_array(position);

     describe(va1, 1, edgeColor);

     std::cout << "INFO: Making render contexts." << std::endl;

     auto rc0 = make_render_context(va0, bi0, PatchType::TRIANGLE);
     auto rc1 = make_render_context(va0, bi1, PatchType::QUINTIC);
     auto rc2 = make_render_context(va0, bi2, PatchType::LOOP_BOX_SPLINE);
     auto rc3 = make_render_context(va0, PatchType::TRIANGLE);

     std::cout << "INFO: Setting up scene." << std::endl;

     auto bandWidth = 1.0;
     auto beamDirection = Vector3D(0.0, 0.0, 1.0);
     auto beamOrigin = Point3D(10.0, 0.0, 0.0);
     auto blue = hpcolor(0.0, 0.0, 1.0, 1.0);
     auto edgeWidth = 0.01;
     auto green = hpcolor(0.0, 1.0, 0.0, 1.0);
     auto level0 = std::array<hpreal, 2>({ 100, 100 });
     auto level1 = std::array<hpreal, 4>({ 60, 60, 60, 60 });
     auto red = hpcolor(1.0, 0.0, 0.0, 1.0);

     std::cout << "INFO: Rendering scene." << std::endl;

     look_at(viewport, mesh.getVertices());
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

          activate(va0);

          activate(qpp, PatchType::QUINTIC);
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(3.5, 0.0, 0.0)));
          sm_vx.setProjectionMatrix(projectionMatrix);
          TessellationControlShader::setInnerTessellationLevel(level0);
          TessellationControlShader::setOuterTessellationLevel(level1);
          hl_fr.setBandColor0(red);
          hl_fr.setBandColor1(green);
          hl_fr.setBandWidth(bandWidth);
          hl_fr.setBeam(Point3D(tempOrigin) / tempOrigin.w, glm::normalize(Vector3D(tempDirection)));
          hl_fr.setLight(light);
          bind(va0, bv1);
          render(qpp, rc1);

          activate(tmp);
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(-3.5, 0.0, 0.0)));
          sm_vx.setProjectionMatrix(projectionMatrix);
          sm_fr.setLight(light);
          sm_fr.setModelColor(blue);
          bind(va0, bv0);
          render(tmp, rc0);

          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(-3.5, -3.5, 0.0)));
          sm_fr.setModelColor(red);
          bind(va0, bv3);
          render(tmp, rc3, size(triangles));

          activate(wfp);
          sm_vx.setModelViewMatrix(viewMatrix);
          sm_vx.setProjectionMatrix(projectionMatrix);
          wf_fr.setEdgeColor(red);
          wf_fr.setEdgeWidth(edgeWidth);
          wf_fr.setLight(light);
          wf_fr.setModelColor(blue);
          bind(va0, bv0);
          render(wfp, rc0);

          activate(lmp, PatchType::LOOP_BOX_SPLINE);
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(0.0, -3.5, 0.0)));
          sm_vx.setProjectionMatrix(projectionMatrix);
          sm_fr.setLight(light);
          sm_fr.setModelColor(blue);
          bind(va0, bv2);
          render(lmp, rc2);

          activate(va1);

          //TODO

          glfwSwapBuffers(context);
     }
}

}//namespace happah

