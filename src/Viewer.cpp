// Copyright 2017
//   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
//   Hedwig Amberg  - Karlsruhe Institute of Technology - hedwigdorothea@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2017.07 - Hedwig Amberg    - add new shader for coloring triangles individualy.

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

//TODO: move next two methods to TriangleMesh.h
template<class Vertex>
std::tuple<Point3D, Point3D> make_axis_aligned_bounding_box(const std::vector<Vertex>& vertices){
     auto min = Point3D(std::numeric_limits<hpreal>::min());
     auto max = Point3D(std::numeric_limits<hpreal>::min());

     for(auto& vertex : vertices) {
          if(vertex.position.x < min.x) min.x = vertex.position.x;
          if(vertex.position.y < min.y) min.y = vertex.position.y;
          if(vertex.position.z < min.z) min.z = vertex.position.z;
          
          if(vertex.position.x > max.x) max.x = vertex.position.x;
          if(vertex.position.y > max.y) max.y = vertex.position.y;
          if(vertex.position.z > max.z) max.z = vertex.position.z;
     }

     return std::make_tuple(min, max);
}

template<class Vertex, Format format>
std::tuple<Point3D, Point3D> make_axis_aligned_bounding_box(const TriangleMesh<Vertex, format>& mesh) { return make_axis_aligned_bounding_box(mesh.getVertices()); }
     
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
     auto triangleColors = std::vector<hpcolor>(3 * size(mesh), blue);
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
     
     //TODO: only for testing !!!
     for(int i = 0; i < size(triangleColors); i += 3){
          triangleColors[i] = red;
          triangleColors[i+1] = blue;
          triangleColors[i+2] = green;
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
     auto ed_fr = make_edge_fragment_shader();
     auto ed_gm = make_geometry_shader("shaders/edge.g.glsl");
     auto ed_vx = make_edge_vertex_shader();
     auto tr_vx = make_triangles_vertex_shader();
     auto tr_gm = make_geometry_shader("shaders/triangles.g.glsl");
     auto tr_fr = make_triangles_fragment_shader();

     std::cout << "INFO: Making programs." << std::endl;

     auto lmp = make_program("loop box spline mesh", sm_vx, lb_te, nm_gm, sm_fr);
     auto qpp = make_program("quintic spline surface", sm_vx, qp_te, hl_fr);
     auto pcp = make_program("point cloud", sm_vx, si_gm, si_fr);
     auto tmp = make_program("triangle mesh", sm_vx, nm_gm, sm_fr);
     auto wfp = make_program("wireframe triangle mesh", sm_vx, wf_gm, wf_fr);
     auto edp = make_program("edges triangle mesh", ed_vx, ed_gm, ed_fr);
     auto trp = make_program("triangle colors mesh", tr_vx, tr_gm, tr_fr);

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
     auto bt3 = make_buffer(triangleColors);

     std::cout << "INFO: Making vertex arrays." << std::endl;

     auto position = make_attribute(0, 4, DataType::FLOAT);
     auto edgeColor = make_attribute(1, 4, DataType::FLOAT);
     auto vertexColor = make_attribute(2, 4, DataType::FLOAT);
     auto triangleColor = make_attribute(3, 4, DataType::FLOAT);

     auto va0 = make_vertex_array();
     auto va1 = make_vertex_array();

     describe(va0, 0, position);
     describe(va1, 0, position);
     describe(va1, 1, edgeColor);
     describe(va1, 2, vertexColor);
     describe(va1, 3, triangleColor);

     std::cout << "INFO: Making render contexts." << std::endl;

     auto rc0 = make_render_context(va0, bi0, PatchType::TRIANGLE);
     auto rc1 = make_render_context(va0, bi1, PatchType::QUINTIC);
     auto rc2 = make_render_context(va0, bi2, PatchType::LOOP_BOX_SPLINE);
     auto rc30 = make_render_context(va0, PatchType::TRIANGLE);
     auto rc31 = make_render_context(va1, PatchType::TRIANGLE);
     auto rc4 = make_render_context(va0, PatchType::POINT);

     std::cout << "INFO: Setting up scene." << std::endl;

     auto bandWidth = 1.0;
     auto beamDirection = Vector3D(0.0, 0.0, 1.0);
     auto beamOrigin = Point3D(10.0, 0.0, 0.0);
     auto box = make_axis_aligned_bounding_box(mesh);
     auto edgeWidth = 0.006; //0.02;//TODO: 0.1 * average height?
     auto lengths = std::get<1>(box) - std::get<0>(box);
     auto level0 = std::array<hpreal, 2>({ 100, 100 });
     auto level1 = std::array<hpreal, 4>({ 60, 60, 60, 60 });
     auto padding = hpreal(0.1) * lengths;
     auto radius = 0.05;

     std::cout << "INFO: Rendering scene." << std::endl;

     look_at(viewport, mesh.getVertices());
     glClearColor(1, 1, 1, 1);
     while(!glfwWindowShouldClose(context)) {
          
          movement(viewport);
          
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
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(lengths.x + padding.x, 0.0, 0.0)));
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
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(-lengths.x - padding.x, 0.0, 0.0)));
          sm_vx.setProjectionMatrix(projectionMatrix);
          sm_fr.setLight(light);
          sm_fr.setModelColor(blue);
          render(tmp, rc0);

          activate(bv3, va0, 0);
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(-lengths.x - padding.x, -lengths.y - padding.y, 0.0)));
          sm_fr.setModelColor(red);
          render(tmp, rc30, size(triangles));

          activate(lmp, PatchType::LOOP_BOX_SPLINE);
          activate(bv2, va0, 0);
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(0.0, -lengths.y - padding.y, 0.0)));
          sm_vx.setProjectionMatrix(projectionMatrix);
          sm_fr.setLight(light);
          sm_fr.setModelColor(blue);
          render(lmp, rc2);

          activate(pcp);
          activate(bv0, va0, 0);
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(lengths.x + padding.x, -lengths.y - padding.y, 0.0)));
          sm_vx.setProjectionMatrix(projectionMatrix);
          si_gm.setProjectionMatrix(projectionMatrix);
          si_gm.setRadius(radius);
          si_fr.setLight(light);
          si_fr.setModelColor(blue);
          si_fr.setProjectionMatrix(projectionMatrix);
          si_fr.setRadius(radius);
          render(pcp, rc4, mesh.getNumberOfVertices());
          /*
          activate(wfp);
          activate(bv0, va0, 0);
          sm_vx.setModelViewMatrix(viewMatrix);
          sm_vx.setProjectionMatrix(projectionMatrix);
          wf_fr.setEdgeWidth(edgeWidth);
          wf_fr.setEdgeColor(red);
          wf_fr.setLight(light);
          wf_fr.setModelColor(blue);
          render(wfp, rc0);
          */
          activate(va1);
          /*
          activate(trp);
          activate(bv3, va1, 0);
          activate(bt3, va1, 1);
          tr_vx.setModelViewMatrix(viewMatrix);
          tr_vx.setProjectionMatrix(projectionMatrix);
          tr_fr.setLight(light);
          render(trp, rc31, size(triangles));
          */
          activate(edp);
          activate(bv3, va1, 0);
          activate(be3, va1, 1);
          activate(bc3, va1, 2);
          ed_vx.setModelViewMatrix(viewMatrix);
          ed_vx.setProjectionMatrix(projectionMatrix);
          ed_fr.setEdgeWidth(edgeWidth);
          ed_fr.setLight(light);
          ed_fr.setModelColor(blue);
          render(edp, rc31, size(triangles));
          
          glfwSwapBuffers(context);
     }
}
     
void Viewer::movement(Viewport& viewport){
     float speed = 0.1;
     auto window = m_window.getContext();
     if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS){
          const Vector2D& step = Vector2D(0, speed);
          viewport.translate(step);
     }
     if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS){
          const Vector2D& step = Vector2D(0, - speed);
          viewport.translate(step);
     }
     if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS){
          const Vector2D& step = Vector2D(speed, 0);
          viewport.translate(step);
     }
     if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS){
          const Vector2D& step = Vector2D(- speed, 0);
          viewport.translate(step);
     }
}
     
}//namespace happah

