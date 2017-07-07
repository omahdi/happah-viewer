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

Vector3D calcBB(std::vector<VertexP3> vertices){
     Vector3D min = Vector3D(FLT_MAX);
     Vector3D max = Vector3D(FLT_MIN);
     for(auto v : vertices){
          if (v.position.x < min.x) min.x = v.position.x;
          if (v.position.y < min.y) min.y = v.position.y;
          if (v.position.z < min.z) min.z = v.position.z;

          if (v.position.x > max.x) max.x = v.position.x;
          if (v.position.y > max.y) max.y = v.position.y;
          if (v.position.z > max.z) max.z = v.position.z;
     }
     return max - min;
}

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

     const auto green  = hpcolor(0.0, 1.0, 0.0, 1.0);
     const auto red    = hpcolor(1.0, 0.0, 0.0, 1.0);
     const auto blue   = hpcolor(0.0, 0.0, 1.0, 1.0);
     const auto grey40 = hpcolor(0.4, 0.4, 0.4, 1.0);
     const auto white  = hpcolor(1.0, 1.0, 1.0, 1.0);
     const auto black  = hpcolor(1.0, 1.0, 1.0, 1.0);

     auto content = format::off::read(argv[1]);
     auto mesh = make_triangle_mesh<VertexP3>(content);
     auto graph = TriangleMesh<VertexP3, Format::DIRECTED_EDGE>(mesh);
     auto edgeColors = std::vector<hpcolor>(3 * size(mesh), blue);
     //auto edgeFlags = std::vector<GLint>(3 * size(mesh), 0);
     auto vertexColors = std::vector<hpcolor>(3 * size(mesh), blue);
     // Iterate over boundary edges and color their opposites
     for (auto ei = 3*graph.getNumberOfTriangles(), num_edges = graph.getNumberOfEdges(); ei < num_edges; ei++) {
          edgeColors[graph.getEdge(ei).opposite] = red;
          //edgeFlags[graph.getEdge(ei).opposite] = 1;
          //std::cout << "- setting edge flag for opposite of " << ei << " = " << graph.getEdge(ei).opposite << "\n";
     }
     auto triangles = make_triangle_array(mesh);

     std::vector<Point2D> proj_coords, hyp_coords;
     bool have_proj_coords {false}, have_hyp_coords {false};
     if (argc >= 3) {
          std::cout << "INFO: Reading additional coordinates (projective disk)\n";
          proj_coords = format::xyz::read<std::vector<Point2D>>(argv[2]);
          if (mesh.getNumberOfVertices() != proj_coords.size())
               throw std::runtime_error("Expected coordinates and mesh instance with matching number of vertices.");
          have_proj_coords = true;
     }
     if (argc >= 4) {
          std::cout << "INFO: Reading additional coordinates (conformal disk)\n";
          hyp_coords = format::xyz::read<std::vector<Point2D>>(argv[3]);
          if (mesh.getNumberOfVertices() != hyp_coords.size())
               throw std::runtime_error("Expected coordinates and mesh instance with matching number of vertices.");
          have_hyp_coords = true;
     }

     std::cout << "INFO: Making shaders." << std::endl;

     //auto nm_gm = make_geometry_shader("shaders/normals.g.glsl");
     //auto sm_fr = make_simple_fragment_shader();
     //auto sm_vx = make_simple_vertex_shader();
     auto wf_fr = make_wireframe_fragment_shader();
     auto wf_gm = make_geometry_shader("shaders/wireframe.g.glsl");
     auto wf_vx = make_wireframe_vertex_shader();

     std::cout << "INFO: Making programs." << std::endl;

     //auto tmp = make_program("triangle mesh", sm_vx, nm_gm, sm_fr);
     auto wfp = make_program("wireframe triangle mesh", wf_vx, wf_gm, wf_fr);

     std::cout << "INFO: Making buffers." << std::endl;

     auto bv0 = make_buffer(mesh.getVertices());
     auto bi0 = make_buffer(mesh.getIndices());
     auto be3 = make_buffer(edgeColors);
     //auto bc3 = make_buffer(vertexColors);
     auto bv3 = make_buffer(triangles.getVertices());

     std::cout << "INFO: Making vertex arrays." << std::endl;

     auto position = make_attribute(0, 4, DataType::FLOAT);
     //auto edgeFlag = make_attribute(1, 1, DataType::INT);
     auto edgeColor = make_attribute(1, 4, DataType::FLOAT);
     //auto vertexColor = make_attribute(2, 4, DataType::FLOAT);

     auto va1 = make_vertex_array();
     describe(va1, 0, position);
     describe(va1, 1, edgeColor);
     //describe(va1, 2, vertexColor);

     std::cout << "INFO: Making render contexts." << std::endl;

     auto rc31 = make_render_context(va1, PatchType::TRIANGLE);

     std::cout << "INFO: setting up checkerboard shaders and buffers.\n";
     auto cb_euc_vx = make_euclidean_checkerboard_vertex_shader();
     auto cb_euc_gm = make_euclidean_checkerboard_geometry_shader();
     auto cb_euc_fr = make_euclidean_checkerboard_fragment_shader();
     auto cb_euc_p = make_program("Euclidean checkerboard pattern", cb_euc_vx, cb_euc_gm, cb_euc_fr);
     auto cb_hyp_vx = make_hyperbolic_checkerboard_vertex_shader();
     auto cb_hyp_gm = make_hyperbolic_checkerboard_geometry_shader();
     auto cb_hyp_fr = make_hyperbolic_checkerboard_fragment_shader();
     auto cb_hyp_p = make_program("Hyperbolic checkerboard pattern", cb_hyp_vx, cb_hyp_gm, cb_hyp_fr);
     auto bv_projcoord = make_buffer(proj_coords);
     auto bv_hypcoord = make_buffer(hyp_coords);
     auto va_chkb = make_vertex_array();
     auto attr_cb_position = make_attribute(0, 4, DataType::FLOAT);
     auto attr_cb_uv_coord = make_attribute(1, 2, DataType::FLOAT);
     describe(va_chkb, 0, attr_cb_position);
     describe(va_chkb, 1, attr_cb_uv_coord);
     auto rc_chkb = make_render_context(va_chkb, bi0, PatchType::TRIANGLE);

     std::cout << "INFO: Setting up scene." << std::endl;

     auto bandWidth = 1.0;
     auto beamDirection = Vector3D(0.0, 0.0, 1.0);
     auto beamOrigin = Point3D(10.0, 0.0, 0.0);
     auto edgeWidth = 0.005; //0.02;
     auto level0 = std::array<hpreal, 2>({ 100, 100 });
     auto level1 = std::array<hpreal, 4>({ 60, 60, 60, 60 });
     auto radius = 0.01;
     Vector3D dim = calcBB(mesh.getVertices());
     float spacing = 1.0;

     std::cout << "INFO: Rendering scene." << std::endl;

     glfwSwapInterval(1);
     look_at(viewport, mesh.getVertices());
     m_window.setHome();
     glClearColor(1, 1, 1, 1);
     while(!glfwWindowShouldClose(context) && !m_window.quitFlag()) {
          glfwPollEvents();
          if (m_window.quitFlag())
               break;
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
          glEnable(GL_DEPTH_TEST);

          auto& projectionMatrix = viewport.getProjectionMatrix();
          auto& viewMatrix = viewport.getViewMatrix();
          auto light = glm::normalize(Point3D(viewMatrix[0]));

          activate(va1);

          if (m_window.enabled(Window::RenderToggle::WIREFRAME)) {
               activate(wfp);
               activate(bv3, va1, 0);
               activate(be3, va1, 1);
               //activate(bc3, va1, 2);
               wf_vx.setModelViewMatrix(viewMatrix);
               wf_vx.setProjectionMatrix(projectionMatrix);
               wf_fr.setEdgeColor(red);
               wf_fr.setEdgeWidth(edgeWidth);
               wf_fr.setLight(light);
               wf_fr.setModelColor(blue);
               render(wfp, rc31, size(triangles));
          }

          if (have_proj_coords && m_window.enabled(Window::RenderToggle::CHECKERBOARD_EUC)) {
               activate(va_chkb);
               activate(cb_euc_p);
               activate(bv0, va_chkb, 0);
               activate(bv_projcoord, va_chkb, 1);
               cb_euc_vx.setModelViewMatrix(viewMatrix);
               cb_euc_vx.setProjectionMatrix(projectionMatrix);
               cb_euc_fr.setColors(hpcolor(0.0, 0.0, 0.0, 1.0), hpcolor(1.0, 1.0, 1.0, 1.0));
               cb_euc_fr.setPeriod(hpvec2(0.05, 0.05));
               cb_euc_fr.setLight(light);
               render(cb_euc_p, rc_chkb);
          }
          if (have_hyp_coords && m_window.enabled(Window::RenderToggle::CHECKERBOARD_HYP)) {
               activate(va_chkb);
               activate(cb_hyp_p);
               activate(bv0, va_chkb, 0);
               activate(bv_hypcoord, va_chkb, 1);
               cb_hyp_vx.setModelViewMatrix(viewMatrix);
               cb_hyp_vx.setProjectionMatrix(projectionMatrix);
               cb_hyp_fr.setColors(hpcolor(0.0, 0.0, 0.0, 1.0), hpcolor(1.0, 1.0, 1.0, 1.0));
               cb_hyp_fr.setPeriod(hpvec2(0.05, 0.05));
               cb_hyp_fr.setLight(light);
               render(cb_hyp_p, rc_chkb);
          }

          glfwSwapBuffers(context);
          glFinish();
     }
}

}//namespace happah

