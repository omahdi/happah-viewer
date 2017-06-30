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

     auto green = hpcolor(0.0, 1.0, 0.0, 1.0);
     auto red = hpcolor(1.0, 0.0, 0.0, 1.0);
     auto blue = hpcolor(0.0, 0.0, 1.0, 1.0);
     
     auto content = format::off::read(argv[1]);
     auto mesh = make_triangle_mesh<VertexP3>(content);
     auto graph = TriangleMesh<VertexP3, Format::DIRECTED_EDGE>(mesh);
     auto edgeColors = std::vector<hpcolor>(3 * size(mesh), blue);
     auto vertexColors = std::vector<hpcolor>(3 * size(mesh), blue);
     auto triangles = make_triangle_array(mesh);

     bool have_disk_mesh {false};
     TriangleMesh<VertexP3> disk_mesh;
     if (argc >= 3) {
          std::cout << "INFO: Reading additional mesh instance (disk topology)\n";
          disk_mesh = make_triangle_mesh<VertexP3>(format::off::read(argv[2]));
          if (mesh.getNumberOfVertices() != disk_mesh.getNumberOfVertices())
               throw std::runtime_error("Expected mesh instances with matching number of vertices");
          have_disk_mesh = true;
          // Iterate over boundary edges and color their opposites
          for (auto ei = 3*graph.getNumberOfTriangles(), num_edges = graph.getNumberOfEdges(); ei < num_edges; ei++)
               edgeColors[graph.getEdge(ei).opposite] = red;
     } else {
          // Compute reduced cut locus (based on Dijkstra's algorithm on dual graph)
          for(auto e : trim(graph, cut(graph))){
               edgeColors[e] = red;
               visit_spokes(graph.getEdges(), e, [&](auto e) {
                    static constexpr hpuint o[3] = { 1, 2, 0 };
                    
                    auto f = graph.getEdge(e).opposite;
                    auto t = make_triangle_index(f);
                    auto i = make_edge_offset(f);
                    vertexColors[3 * t + o[i]] = red;
                    vertexColors[e] = red;
               });
          }
     }

     std::cout << "INFO: Making shaders." << std::endl;

     auto nm_gm = make_geometry_shader("shaders/normals.g.glsl");
     auto sm_fr = make_simple_fragment_shader();
     auto sm_vx = make_simple_vertex_shader();
     auto wf_fr = make_wireframe_fragment_shader();
     auto wf_gm = make_geometry_shader("shaders/wireframe.g.glsl");
     auto wf_vx = make_wireframe_vertex_shader();

     std::cout << "INFO: Making programs." << std::endl;

     auto tmp = make_program("triangle mesh", sm_vx, nm_gm, sm_fr);
     auto wfp = make_program("wireframe triangle mesh", wf_vx, wf_gm, wf_fr);

     std::cout << "INFO: Making buffers." << std::endl;

     auto bv0 = make_buffer(mesh.getVertices());
     auto bv3 = make_buffer(triangles.getVertices());
     auto bi0 = make_buffer(mesh.getIndices());
     auto be3 = make_buffer(edgeColors);
     auto bc3 = make_buffer(vertexColors);

     std::cout << "INFO: Making vertex arrays." << std::endl;

     auto position = make_attribute(0, 4, DataType::FLOAT);
     auto edgeColor = make_attribute(1, 4, DataType::FLOAT);
     auto vertexColor = make_attribute(2, 4, DataType::FLOAT);

     auto va1 = make_vertex_array();

     describe(va1, 0, position);
     describe(va1, 1, edgeColor);
     describe(va1, 2, vertexColor);

     std::cout << "INFO: Making render contexts." << std::endl;

     auto rc31 = make_render_context(va1, PatchType::TRIANGLE);

     std::cout << "INFO: setting up checkerboard shaders and buffers.\n";
     auto cb_vx = make_checkerboard_vertex_shader();
     auto cb_gm = make_checkerboard_geometry_shader();
     auto cb_fr = make_checkerboard_fragment_shader();
     auto cbp = make_program("checkerboard pattern", cb_vx, cb_gm, cb_fr);
     auto bv_hypcoord = make_buffer(disk_mesh.getVertices());
     auto va_chkb = make_vertex_array();
     auto attr_cb_position = make_attribute(0, 4, DataType::FLOAT);
     auto attr_cb_hyp_coord = make_attribute(1, 3, DataType::FLOAT);
     describe(va_chkb, 0, attr_cb_position);
     describe(va_chkb, 1, attr_cb_hyp_coord);
     auto rc_chkb = make_render_context(va_chkb, bi0, PatchType::TRIANGLE);

     std::cout << "INFO: Setting up scene." << std::endl;

     auto bandWidth = 1.0;
     auto beamDirection = Vector3D(0.0, 0.0, 1.0);
     auto beamOrigin = Point3D(10.0, 0.0, 0.0);
     auto edgeWidth = 0.006; //0.02;
     auto level0 = std::array<hpreal, 2>({ 100, 100 });
     auto level1 = std::array<hpreal, 4>({ 60, 60, 60, 60 });
     auto radius = 0.05;
     Vector3D dim = calcBB(mesh.getVertices());
     float spacing = 1.0;

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

          activate(va1);

          activate(wfp);
          activate(bv3, va1, 0);
          activate(be3, va1, 1);
          activate(bc3, va1, 2);
          wf_vx.setModelViewMatrix(viewMatrix);
          cb_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(-(dim.x/2 + spacing), 0.0, 0.0)));
          wf_vx.setProjectionMatrix(projectionMatrix);
          wf_fr.setEdgeColor(red);
          wf_fr.setEdgeWidth(edgeWidth);
          wf_fr.setLight(light);
          wf_fr.setModelColor(blue);
          render(wfp, rc31, size(triangles));

          if (have_disk_mesh) {
               activate(va_chkb);
               activate(cbp);
               activate(bv0, va_chkb, 0);
               activate(bv_hypcoord, va_chkb, 1);
               cb_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(dim.x/2 + spacing, 0, 0.0)));
               cb_vx.setProjectionMatrix(projectionMatrix);
               cb_fr.setColors(hpcolor(0.0, 0.0, 0.0, 1.0), hpcolor(1.0, 1.0, 1.0, 1.0));
               cb_fr.setPeriod(hpvec2(0.05, 0.05));
               cb_fr.setLight(light);
               render(cbp, rc_chkb);

          }

          glfwSwapBuffers(context);
     }
}
     
}//namespace happah

