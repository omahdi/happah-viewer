// Copyright 2017
//   Pawel Herman   - Karlsruhe Institute of Technology - pherman@ira.uka.de
//   Hedwig Amberg  - Karlsruhe Institute of Technology - hedwigdorothea@gmail.com
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2017.07 - Hedwig Amberg    - Added new shader for coloring triangles individually.
// 2017.08 - Hedwig Amberg    - Added new shader for coloring both triangles and edges.

#include <happah/format.hpp>
#include <happah/geometry/LoopBoxSplineMesh.hpp>
#include <happah/geometry/BezierTriangleMesh.hpp>
#include <happah/geometry/TriangleArray.hpp>
#include <happah/geometry/DiskEmbedding.hpp>
#include <happah/graphics.hpp>
#include <happah/math/Space.hpp>
#include <GLFW/glfw3.h>//NOTE: Glad must be included before GLFW.
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <queue>
#include <string>

#include "Viewer.hpp"

namespace happah {

Viewer::Viewer(hpuint width, hpuint height, const std::string& title)
     : m_window(width, height, title) {
     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);//TODO: find current version
     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
     glfwWindowHint(GLFW_SAMPLES, 8);
     glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
     glfwMakeContextCurrent(m_window.getContext());
     if(!gladLoadGL()) throw std::runtime_error("Failed to initialize Glad.");
}

/// Variant of cut() that picks next edge by hop distance.
/// Note: not fit for meshes with border!
template<class _Vertex>
Indices geocut(const TriangleGraph<_Vertex>& graph, hpindex t0 = 0) {
     const auto& edges = graph.getEdges();
     const auto& vertices = graph.getVertices();
     struct edge_info {
          hpindex ei;
          hpreal dist;
          // Note: invert meaning to turn max-heap into min-heap
          bool operator<(const edge_info& other) const { return dist > other.dist; }
     };
     auto cache = boost::dynamic_bitset<>(edges.size(), false);
     std::priority_queue<edge_info> pq;
     assert(t0 < edges.size() / 3);
     for (hpindex ei = 3*t0, last_ei = 3*t0+2; ei <= last_ei; ei++) {
          pq.push(edge_info{ei, 0});
          cache[ei] = true;
     }
     return basic_cut(edges, t0, [&](auto& neighbors) {
          //for(auto e : boost::irange(0u, hpindex(mesh.getEdges().size())))
          //     if(neighbors[e << 1] != std::numeric_limits<hpindex>::max() && neighbors[mesh.getEdge(e).opposite << 1] == std::numeric_limits<hpindex>::max()) return e;
          edge_info info;
          hpindex e;
          while (!pq.empty()) {
               info = pq.top();
               e = info.ei;
               if (cache[e])
                    break;
               pq.pop();
          }
          if (pq.empty())
               return std::numeric_limits<hpindex>::max();
          pq.pop();
          auto& edge = edges[edges[e].opposite];
          auto e0 = edges[edge.previous].opposite;
          auto e1 = edges[edge.next].opposite;
          auto b0 = neighbors[e0 << 1] == std::numeric_limits<hpindex>::max();
          auto b1 = neighbors[e1 << 1] == std::numeric_limits<hpindex>::max();
          if (b0) {
               const Point3D vu0 = vertices[edge.vertex].position;
               const Point3D vu1 = vertices[edges[edge.next].vertex].position;
               const Point3D vu2 = vertices[edges[edge.previous].vertex].position;
               const Point3D vt0 = vertices[edges[edges[e].next].vertex].position;
               const Point3D vu = (vu0+vu1+vu2) / hpreal(3), vt = (vt0+vu0+vu2) / hpreal(3);
               pq.push(edge_info{edge.previous, info.dist+glm::distance(vt, vu)});
               cache[edge.previous] = true;
          } else
               cache[e0] = false;
          if (b1) {
               pq.push(edge_info{edge.next, info.dist+1});
               cache[edge.next] = true;
          } else
               cache[e1] = false;
          cache[e] = false;
          return hpindex(e);
     });
}

/// Variant of cut() that picks next edge by hop distance.
/// Note: not fit for meshes with border!
Indices hopdist_cut(const std::vector<Edge>& edges, hpindex t0 = 0) {
     struct edge_info {
          hpindex ei, dist;
          // Note: invert meaning to turn max-heap into min-heap
          bool operator<(const edge_info& other) const { return dist > other.dist; }
     };
     auto cache = boost::dynamic_bitset<>(edges.size(), false);
     std::priority_queue<edge_info> pq;
     assert(t0 < edges.size() / 3);
     for (hpindex ei = 3*t0, last_ei = 3*t0+2; ei <= last_ei; ei++) {
          pq.push(edge_info{ei, 0});
          cache[ei] = true;
     }
     return basic_cut(edges, t0, [&](auto& neighbors) {
          //for(auto e : boost::irange(0u, hpindex(mesh.getEdges().size())))
          //     if(neighbors[e << 1] != std::numeric_limits<hpindex>::max() && neighbors[mesh.getEdge(e).opposite << 1] == std::numeric_limits<hpindex>::max()) return e;
          edge_info info;
          hpindex e;
          while (!pq.empty()) {
               info = pq.top();
               e = info.ei;
               if (cache[e])
                    break;
               pq.pop();
          }
          if (pq.empty())
               return std::numeric_limits<hpindex>::max();
          pq.pop();
          auto& edge = edges[edges[e].opposite];
          auto e0 = edges[edge.previous].opposite;
          auto e1 = edges[edge.next].opposite;
          auto b0 = neighbors[e0 << 1] == std::numeric_limits<hpindex>::max();
          auto b1 = neighbors[e1 << 1] == std::numeric_limits<hpindex>::max();
          if (b0) {
               pq.push(edge_info{edge.previous, info.dist+1});
               cache[edge.previous] = true;
          } else
               cache[e0] = false;
          if (b1) {
               pq.push(edge_info{edge.next, info.dist+1});
               cache[edge.next] = true;
          } else
               cache[e1] = false;
          cache[e] = false;
          return hpindex(e);
     });
}

void Viewer::execute(int argc, char* argv[]) {
     using namespace std::string_literals;
     auto context = m_window.getContext();
     auto& viewport = m_window.getViewport();

     const auto blue = hpcolor(0.0, 0.0, 1.0, 1.0);
     const auto green = hpcolor(0.0, 1.0, 0.0, 1.0);
     const auto red = hpcolor(1.0, 0.0, 0.0, 1.0);
     const auto white = hpcolor(1.0, 1.0, 1.0, 1.0);
     const auto grey05 = hpcolor(0.05, 0.05, 0.05, 1.0);
     const auto grey25 = hpcolor(0.25, 0.25, 0.25, 1.0);
     const auto grey50 = hpcolor(0.50, 0.50, 0.50, 1.0);
     const auto grey75 = hpcolor(0.75, 0.75, 0.75, 1.0);
     //const auto regular_edge_color = hpcolor(0.25, 0.25, 0.25, 0.0);
     //const auto cut_edge_color = hpcolor(1.0, 0.0, 1.0, 1.0);
     //const auto new_edge_color = hpcolor(0.0, 0.9, 0.4, 1.0);
     //const auto old_edge_color = hpcolor(0.5, 0.0, 0.8, 1.0);
     const auto regular_edge_color = hpcolor(0.55, 0.55, 0.55, 0.0);
     const auto cut_edge_color = hpcolor(0.1, 0.1, 0.2, 1.0);
     const auto new_edge_color = hpcolor(0.1, 0.6, 0.0, 1.0);
     const auto old_edge_color = hpcolor(0.5, 0.0, 0.0, 1.0);

     if (argc <= 1) {
          std::cout << "Usage: " << argv[0] << " <mesh.off> [initial-cut.hph]\n";
          return;
     }
     std::cout << "INFO: Importing " << argv[1] << '.' << std::endl;

     auto content = format::off::read(argv[1]);
     auto mesh = make_triangle_mesh<VertexP3>(content);
     auto graph = make_triangle_graph(mesh);
     auto edgeColors = std::vector<hpcolor>(3 * size(mesh), regular_edge_color); //std::vector<hpcolor>(3 * size(mesh), blue);
     auto triangleColors = std::vector<hpcolor>(3 * size(mesh), blue);
     auto vertexColors = std::vector<hpcolor>(3 * size(mesh), red); //std::vector<hpcolor>(3 * size(mesh), blue);
     auto triangles = make_triangle_array(mesh);

     // Don't name it ``cut'' or ``cut_edges'' to avoid shadowing functions of
     // that name: cut() is defined in "TriangleGraph.hpp", cut_edges() in
     // "DiskEmbedding.hpp".
#if 0
     const auto the_cut = trim(graph, cut(graph));
#else
     Indices the_cut;
     if (argc >= 3) {
          const std::string arg {argv[2]};
          if (arg == "random"s)
               the_cut = trim(graph, cut(graph.getEdges()));
          else if (arg == "geo"s)
               the_cut = trim(graph, geocut(graph));
          else
               the_cut = format::hph::read<Indices>(p(arg));
     } else
          the_cut = trim(graph, hopdist_cut(graph.getEdges()));
#endif
     for(auto e : the_cut){
          edgeColors[e] = cut_edge_color;
          edgeColors[graph.getEdge(e).opposite] = cut_edge_color;
          visit_spokes(make_spokes_enumerator(graph.getEdges(), e), [&](auto e) {
               static constexpr hpuint o[3] = { 1, 2, 0 };

               auto f = graph.getEdge(e).opposite;
               auto t = make_triangle_index(f);
               auto i = make_edge_offset(f);
               vertexColors[3 * t + o[i]] = cut_edge_color;
               vertexColors[e] = cut_edge_color;
          });
     }
     try {
          auto cut_graph {cut_graph_from_edges(graph, the_cut)};
          remove_chords(cut_graph, graph);
          {
               using std::begin;
               using std::end;
               auto sorted_cut = the_cut;
               std::sort(begin(sorted_cut), end(sorted_cut));
               auto reduced_cut = cut_edges(cut_graph);
               std::sort(begin(reduced_cut), end(reduced_cut));
               std::vector<hpindex> sym_diff;
               sym_diff.reserve((sorted_cut.size()-reduced_cut.size())*2);
               std::set_symmetric_difference(
                    begin(sorted_cut), end(sorted_cut),
                    begin(reduced_cut), end(reduced_cut),
                    std::back_inserter(sym_diff));
               for (auto ei : sym_diff) {
                    const auto is_new = std::binary_search(begin(reduced_cut), end(reduced_cut), ei);
                    const auto opp = graph.getEdge(ei).opposite;
                    edgeColors[ei] = is_new ? new_edge_color : old_edge_color;
                    edgeColors[opp] = is_new ? new_edge_color : old_edge_color;
               }
          }
     } catch (const std::exception& ecp) {
          std::cout << "WARN: Exception while building cut graph: " << ecp.what() << "\n";
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
     load("/happah/geometry.h.glsl", p("shaders/geometry.h.glsl"));

#if 1
     auto ed_fr = make_highlight_edge_fragment_shader();
     auto ed_gm = make_geometry_shader(p("shaders/hiedge.g.glsl"));
     auto ed_vx = make_highlight_edge_vertex_shader();
     compile(ed_fr);
     compile(ed_gm);
     compile(ed_vx);
     auto cb_euc_vx = make_euclidean_checkerboard_vertex_shader();
     //auto cb_euc_gm = make_euclidean_checkerboard_geometry_shader();
     auto cb_euc_gm = make_geometry_shader(p("shaders/euclidean-checkerboard.g.glsl"));
     auto cb_euc_fr = make_euclidean_checkerboard_fragment_shader();
     compile(cb_euc_fr);
     compile(cb_euc_gm);
     compile(cb_euc_vx);
#else
     auto ed_fr = make_edge_fragment_shader();
     auto ed_gm = make_geometry_shader(p("shaders/edge.g.glsl"));
     auto ed_vx = make_edge_vertex_shader();
#endif

     //auto nm_gm = make_geometry_shader(p("shaders/normals.g.glsl"));
     //auto wf_fr = make_wireframe_fragment_shader();
     //auto wf_gm = make_geometry_shader(p("shaders/wireframe.g.glsl"));
     //compile(nm_gm);
     //compile(wf_fr);
     //compile(wf_gm);

     std::cout << "INFO: Making programs." << std::endl;

     //auto wfp = make_program("wireframe triangle mesh", sm_vx, wf_gm, wf_fr);
     auto edp = make_program("edges triangle mesh", ed_vx, ed_gm, ed_fr);
     auto cb_euc_p = make_program("Euclidean checkerboard pattern", cb_euc_vx, cb_euc_gm, cb_euc_fr);

     std::cout << "INFO: Making buffers." << std::endl;

     auto bv0 = make_buffer(mesh.getVertices());
     auto bv3 = make_buffer(triangles.getVertices());
     auto bi0 = make_buffer(mesh.getIndices());
     auto be3 = make_buffer(edgeColors);

     std::cout << "INFO: Making vertex arrays." << std::endl;

     auto position = make_attribute(0, 4, DataType::FLOAT);
     auto edgeColor = make_attribute(1, 4, DataType::FLOAT);
     //auto vertexColor = make_attribute(2, 4, DataType::FLOAT);

     auto va0 = make_vertex_array();

     describe(va0, 0, position);
     describe(va0, 1, edgeColor);
     //describe(va0, 2, vertexColor);

     auto cb_euc_coords = make_buffer(proj_coords);
     auto attr_cb_position = make_attribute(0, 4, DataType::FLOAT);
     auto attr_cb_uv_coord = make_attribute(1, 2, DataType::FLOAT);
     auto va_chkb = make_vertex_array();
     describe(va_chkb, 0, attr_cb_position);
     describe(va_chkb, 1, attr_cb_uv_coord);
     auto rc_chkb = make_render_context(va_chkb, bi0, PatchType::TRIANGLE);

     std::cout << "INFO: Making render contexts." << std::endl;

     auto rc31 = make_render_context(va0, PatchType::TRIANGLE);

     std::cout << "INFO: Setting up scene." << std::endl;

     const auto box = make_axis_aligned_bounding_box(mesh);
     const auto edgeWidth = 3.0;
     const auto lengths = std::get<1>(box) - std::get<0>(box);
     //auto padding = hpreal(0.1) * lengths;

     std::cout << "INFO: Rendering scene." << std::endl;

     look_at(viewport, mesh.getVertices());
     glClearColor(1, 1, 1, 1);
     //glEnable(GL_BLEND);
     //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
     //glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
     glEnable(GL_DEPTH_TEST);
     while(!glfwWindowShouldClose(context)) {
          glfwPollEvents();
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

          auto projectionMatrix = make_projection_matrix(viewport);
          auto viewMatrix = make_view_matrix(viewport);
          auto light = 0.6f*glm::normalize(Point3D(viewMatrix[0]));

#if 0
          activate(va0);
          activate(wfp);
          activate(bv0, va0, 0);
          sm_vx.setModelViewMatrix(glm::translate(viewMatrix, Vector3D(-lengths.x - padding.x, lengths.y + padding.y, 0.0)));
          sm_vx.setProjectionMatrix(projectionMatrix);
          wf_fr.setEdgeWidth(edgeWidth);
          wf_fr.setEdgeColor(red);
          wf_fr.setLight(light);
          wf_fr.setModelColor(blue);
          render(wfp, rc0);
#endif

#if 1
		activate(va_chkb);
          activate(edp);
          activate(bv0, va_chkb, 0);
          activate(cb_euc_coords, va_chkb, 1);
          cb_euc_vx.setModelViewMatrix(viewMatrix);
          cb_euc_vx.setProjectionMatrix(projectionMatrix);
          //ed_fr.setEdgeWidth(3.0);
          ed_fr.setEdgeWidth(1.0);
          cb_fr.setColors(hpcolor(0.0, 0.0, 0.0, 1.0), hpcolor(1.0, 1.0, 1.0, 1.0));
          cb_fr.setPeriod(hpvec2(0.05, 0.05));
          cb_euc_fr.setLight(light);
          render(cb_euc_p, rc_chkb);
#endif

#if 0
          activate(va0);
          activate(edp);
          activate(bv3, va0, 0);
          activate(be3, va0, 1);
          ed_vx.setModelViewMatrix(viewMatrix);
          ed_vx.setProjectionMatrix(projectionMatrix);
          //ed_fr.setEdgeWidth(3.0);
          ed_fr.setEdgeWidth(1.0);
          ed_fr.setLight(light);
          ed_fr.setModelColor(hpcolor(1.0, 1.0, 1.0, 1.0)); //white);
#if 1
          ed_fr.setSqueezeScale(0.45);
          ed_fr.setSqueezeMin(0.35);
#endif
          render(edp, rc31, size(triangles));
#endif

          glfwSwapBuffers(context);
     }
}

}//namespace happah

