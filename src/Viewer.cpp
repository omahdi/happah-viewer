// Copyright 2017
//   Pawel Herman   - Karlsruhe Institute of Technology - pherman@ira.uka.de
//   Hedwig Amberg  - Karlsruhe Institute of Technology - hedwigdorothea@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2017.07 - Hedwig Amberg    - Added new shader for coloring triangles individually.
// 2017.08 - Hedwig Ammber    - add new shader for coloring triangles and edges

#include <happah/format.hpp>
#include <happah/geometries/LoopBoxSplineMesh.hpp>
#include <happah/geometries/SurfaceSplineBEZ.hpp>
#include <happah/geometries/TriangleArray.hpp>
#include <happah/graphics.hpp>
#include <happah/math/Space.hpp>
#include <GLFW/glfw3.h>//NOTE: Glad must be included before GLFW.
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>
#include <experimental/filesystem>

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

     auto blue = hpcolor(0.0, 0.0, 1.0, 1.0);
     auto green = hpcolor(0.0, 1.0, 0.0, 1.0);
     auto red = hpcolor(1.0, 0.0, 0.0, 1.0);
     
     std::cout << "INFO: Importing " << argv[1] << '.' << std::endl;

     auto content = format::off::read(argv[1]);
     auto mesh = make_triangle_mesh<VertexP3>(content);
     auto graph = make_triangle_graph(mesh);
     auto edgeColors = std::vector<hpcolor>(3 * size(mesh), green); //std::vector<hpcolor>(3 * size(mesh), blue);
     auto triangleColors = std::vector<hpcolor>(3 * size(mesh), blue);
     auto vertexColors = std::vector<hpcolor>(3 * size(mesh), red); //std::vector<hpcolor>(3 * size(mesh), blue);
     auto triangles = make_triangle_array(mesh);
     auto boxes = make_loop_box_spline_mesh(mesh);
     auto quartic = make_spline_surface(graph);
     //auto mesh = make_triangle_mesh(quartic, 4);
     auto quintic = elevate(quartic);

     for(auto e : trim(graph, cut(graph))){
          edgeColors[e] = red;
          visit_spokes(make_spokes_enumerator(graph.getEdges(), e), [&](auto e) {
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

     load("/happah/illumination.h.glsl", p("shaders/illumination.h.glsl"));
     load("/happah/paint.h.glsl", p("shaders/paint.h.glsl"));
     load("/happah/geometry.h.glsl", p("shaders/geometry.h.glsl"));

     auto ed_fr = make_edge_fragment_shader();
     auto ed_gm = make_geometry_shader(p("shaders/edge.g.glsl"));
     auto ed_vx = make_edge_vertex_shader();
     auto hl_fr = make_highlight_lines_fragment_shader();
     auto lb_te = make_tessellation_evaluation_shader(p("shaders/loop-box-spline.te.glsl"));
     auto nm_gm = make_geometry_shader(p("shaders/normals.g.glsl"));
     auto pt_vx = make_patches_vertex_shader();
     auto pt_gm = make_geometry_shader(p("shaders/patches.g.glsl"));
     auto pt_fr = make_patches_fragment_shader();
     auto qp_te = make_tessellation_evaluation_shader(p("shaders/quintic-patch.te.glsl"));
     auto si_fr = make_sphere_impostor_fragment_shader();
     auto si_gm = make_sphere_impostor_geometry_shader();
     auto sm_fr = make_simple_fragment_shader();
     auto sm_vx = make_simple_vertex_shader();
     auto tr_fr = make_triangles_fragment_shader();
     auto tr_gm = make_geometry_shader(p("shaders/triangles.g.glsl"));
     auto tr_vx = make_triangles_vertex_shader();
     auto wf_fr = make_wireframe_fragment_shader();
     auto wf_gm = make_geometry_shader(p("shaders/wireframe.g.glsl"));

     compile(ed_fr);
     compile(ed_gm);
     compile(ed_vx);
     compile(hl_fr);
     compile(lb_te);
     compile(nm_gm);
     compile(pt_vx);
     compile(pt_gm);
     compile(pt_fr);
     compile(qp_te);
     compile(si_fr);
     compile(si_gm);
     compile(sm_fr);
     compile(sm_vx);
     compile(tr_fr);
     compile(tr_gm);
     compile(tr_vx);
     compile(wf_fr);
     compile(wf_gm);

     std::cout << "INFO: Making programs." << std::endl;

     auto lmp = make_program("loop box spline mesh", sm_vx, lb_te, nm_gm, sm_fr);
     auto ptc = make_program("colored patches mesh", pt_vx, pt_gm, pt_fr);
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

     auto edgeColor = make_attribute(2, 4, DataType::FLOAT);
     auto position = make_attribute(0, 4, DataType::FLOAT);
     auto vertexColor = make_attribute(1, 4, DataType::FLOAT);

     auto va0 = make_vertex_array();

     describe(va0, 0, position);
     describe(va0, 1, vertexColor);
     describe(va0, 2, edgeColor);

     std::cout << "INFO: Making render contexts." << std::endl;

     auto rc0 = make_render_context(va0, bi0, PatchType::TRIANGLE);
     auto rc1 = make_render_context(va0, bi1, PatchType::QUINTIC);
     auto rc2 = make_render_context(va0, bi2, PatchType::LOOP_BOX_SPLINE);
     auto rc30 = make_render_context(va0, PatchType::TRIANGLE);
     auto rc31 = make_render_context(va0, PatchType::TRIANGLE);
     auto rc4 = make_render_context(va0, PatchType::POINT);

     std::cout << "INFO: Setting up scene." << std::endl;

     auto bandWidth = 1.0;
     auto beamDirection = Vector3D(0.0, 0.0, 1.0);
     auto beamOrigin = Point3D(10.0, 0.0, 0.0);
     auto box = make_axis_aligned_bounding_box(mesh);
     auto edgeWidth = 0.006; //0.02;//TODO: 0.1 * average height?
     auto lengths = std::get<1>(box) - std::get<0>(box);
     auto level0 = std::array<float, 2>({ 50, 50 });
     auto level1 = std::array<float, 4>({ 30, 30, 30, 30 });
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
          
          activate(wfp);
          activate(bv0, va0, 0);
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(-lengths.x - padding.x, lengths.y + padding.y, 0.0)));
          sm_vx.setProjectionMatrix(projectionMatrix);
          wf_fr.setEdgeWidth(edgeWidth);
          wf_fr.setEdgeColor(red);
          wf_fr.setLight(light);
          wf_fr.setModelColor(blue);
          render(wfp, rc0);
          
          activate(trp);
          activate(bv3, va0, 0);
          activate(bt3, va0, 1);
          tr_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(0.0, lengths.y + padding.y, 0.0)));
          tr_vx.setProjectionMatrix(projectionMatrix);
          tr_fr.setLight(light);
          render(trp, rc31, size(triangles));
          
          activate(edp);
          activate(bv3, va0, 0);
          activate(bc3, va0, 1);
          activate(be3, va0, 2);
          ed_vx.setModelViewMatrix(viewMatrix);
          ed_vx.setProjectionMatrix(projectionMatrix);
          ed_fr.setEdgeWidth(edgeWidth);
          ed_fr.setLight(light);
          ed_fr.setModelColor(blue);
          render(edp, rc31, size(triangles));
          
          activate(ptc);
          activate(bv3, va0, 0); //position
          activate(bt3, va0, 1); //triangle color
          activate(be3, va0, 2); //edge color
          activate(bc3, va0, 3); //vertex color
          pt_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(lengths.x + padding.x, lengths.y + padding.y, 0.0)));
          pt_vx.setProjectionMatrix(projectionMatrix);
          pt_fr.setEdgeWidth(edgeWidth);
          pt_fr.setLight(light);
          render(ptc, rc31, size(triangles));
          
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

