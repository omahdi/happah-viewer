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
     
     bool have_disk_mesh {false};

     auto content = format::off::read(argv[1]);
     auto mesh = make_triangle_mesh<VertexP3>(content);
     auto graph = TriangleMesh<VertexP3, Format::DIRECTED_EDGE>(mesh);
     auto edgeColors = std::vector<hpcolor>(3 * size(mesh), blue);
     auto vertexColors = std::vector<hpcolor>(3 * size(mesh), blue);
     auto triangles = make_triangle_array(mesh);
     auto boxes = make_loop_box_spline_mesh(mesh);
     auto quartic = make_spline_surface(TriangleMesh<VertexP3, Format::DIRECTED_EDGE>(mesh));
     //auto mesh = make_triangle_mesh(quartic, 4);
     auto quintic = elevate(quartic);

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
     
     std::cout << "INFO: Making shaders." << std::endl;

     auto hl_fr = make_highlight_lines_fragment_shader();
     auto lb_te = make_tessellation_evaluation_shader("shaders/loop-box-spline.te.glsl");
     auto nm_gm = make_geometry_shader("shaders/normals.g.glsl");
     auto qp_te = make_tessellation_evaluation_shader("shaders/quintic-patch.te.glsl");
     auto si_fr = make_sphere_impostor_fragment_shader();
     auto si_gm = make_sphere_impostor_geometry_shader();
     auto sm_fr = make_simple_fragment_shader();
     auto sm_vx = make_simple_vertex_shader();
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
     auto bc3 = make_buffer(vertexColors);

     std::cout << "INFO: Making vertex arrays." << std::endl;

     auto position = make_attribute(0, 4, DataType::FLOAT);
     auto edgeColor = make_attribute(1, 4, DataType::FLOAT);
     auto vertexColor = make_attribute(2, 4, DataType::FLOAT);

     auto va0 = make_vertex_array();
     auto va1 = make_vertex_array();

     describe(va0, 0, position);
     describe(va1, 0, position);
     describe(va1, 1, edgeColor);
     describe(va1, 2, vertexColor);

     std::cout << "INFO: Making render contexts." << std::endl;

     auto rc0 = make_render_context(va0, bi0, PatchType::TRIANGLE);
     auto rc1 = make_render_context(va0, bi1, PatchType::QUINTIC);
     auto rc2 = make_render_context(va0, bi2, PatchType::LOOP_BOX_SPLINE);
     auto rc30 = make_render_context(va0, PatchType::TRIANGLE);
     auto rc31 = make_render_context(va1, PatchType::TRIANGLE);
     auto rc4 = make_render_context(va0, PatchType::POINT);

     bool have_disk_mesh {false};
     TriangleMesh<VertexP3> cut_mesh, disk_mesh;
     if (argc >= 4) {
          std::cout << "INFO: Reading additional mesh instances (disk topology, embedded mesh)\n";
          cut_mesh = make_triangle_mesh<VertexP3>(format::off::read(argv[2]));
          disk_mesh = make_triangle_mesh<VertexP3>(format::off::read(argv[3]));
          if (cut_mesh.getNumberOfVertices() != disk_mesh.getNumberOfVertices())
               throw std::runtime_error("Expected mesh instances with matching number of vertices");
          have_disk_mesh = true;
     }
     std::cout << "INFO: setting up checkerboard shaders and buffers.\n";
     auto cb_vx = make_checkerboard_vertex_shader();
     auto cb_gm = make_checkerboard_geometry_shader();
     auto cb_fr = make_checkerboard_fragment_shader();
     auto cbp = make_program("checkerboard pattern", cb_vx, cb_gm, cb_fr);
     auto bv_disktopo = make_buffer(cut_mesh.getVertices());
     auto bi_disktopo = make_buffer(cut_mesh.getIndices());
     auto bv_hypcoord = make_buffer(disk_mesh.getVertices());
     auto va_chkb = make_vertex_array();
     auto attr_cb_position = make_attribute(0, 4, DataType::FLOAT);
     auto attr_cb_hyp_coord = make_attribute(1, 3, DataType::FLOAT);
     describe(va_chkb, 0, attr_cb_position);
     describe(va_chkb, 1, attr_cb_hyp_coord);
     auto rc_chkb = make_render_context(va_chkb, bi_disktopo, PatchType::TRIANGLE);

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
          auto tempDirection = viewMatrix * Vector4D(beamDirection, 0.0);
          auto tempOrigin = viewMatrix * Point4D(beamOrigin, 1.0);

          activate(va0);

          activate(qpp, PatchType::QUINTIC);
          activate(bv1, va0, 0);
          //sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(3.5, 0.0, 0.0)));
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(dim.x + spacing, 0.0, 0.0)));
          sm_vx.setProjectionMatrix(projectionMatrix);
          TessellationControlShader::setInnerTessellationLevel(level0);
          TessellationControlShader::setOuterTessellationLevel(level1);
          hl_fr.setBandColor0(red);
          hl_fr.setBandColor1(green);
          hl_fr.setBandWidth(bandWidth);
          hl_fr.setBeam(Point3D(tempOrigin) / tempOrigin.w, glm::normalize(Vector3D(tempDirection)));
          hl_fr.setLight(light);
          render(qpp, rc1);

          activate(tmp);
          activate(bv0, va0, 0);
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(-dim.x - spacing, 0.0, 0.0)));
          sm_vx.setProjectionMatrix(projectionMatrix);
          sm_fr.setLight(light);
          sm_fr.setModelColor(blue);
          render(tmp, rc0);

          activate(bv3, va0, 0);
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(-dim.x - spacing, -dim.y - spacing, 0.0)));
          sm_fr.setModelColor(red);
          render(tmp, rc30, size(triangles));

          activate(lmp, PatchType::LOOP_BOX_SPLINE);
          activate(bv2, va0, 0);
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(0.0, -dim.y - spacing, 0.0)));
          sm_vx.setProjectionMatrix(projectionMatrix);
          sm_fr.setLight(light);
          sm_fr.setModelColor(blue);
          render(lmp, rc2);

          activate(pcp);
          activate(bv0, va0, 0);
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(dim.x + spacing, -dim.y - spacing, 0.0)));
          sm_vx.setProjectionMatrix(projectionMatrix);
          si_gm.setProjectionMatrix(projectionMatrix);
          si_gm.setRadius(radius);
          si_fr.setLight(light);
          si_fr.setModelColor(blue);
          si_fr.setProjectionMatrix(projectionMatrix);
          si_fr.setRadius(radius);
          render(pcp, rc4, mesh.getNumberOfVertices());

          activate(va1);

          activate(wfp);
          activate(bv3, va1, 0);
          activate(be3, va1, 1);
          activate(bc3, va1, 2);
          wf_vx.setModelViewMatrix(viewMatrix);
          wf_vx.setProjectionMatrix(projectionMatrix);
          wf_fr.setEdgeColor(red);
          wf_fr.setEdgeWidth(edgeWidth);
          wf_fr.setLight(light);
          wf_fr.setModelColor(blue);
          render(wfp, rc31, size(triangles));

          if (have_disk_mesh) {
               activate(va_chkb);
               activate(cbp);
               activate(bv_disktopo, va_chkb, 0);
               activate(bv_hypcoord, va_chkb, 1);
               cb_vx.setModelViewMatrix(viewMatrix);
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

