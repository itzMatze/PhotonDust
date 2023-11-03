struct PathTracerPushConstants {
    uint sample_count;
    bool attenuation_view;
    bool emission_view;
    bool normal_view;
    bool tex_view;
    bool path_depth_view;
};

struct CameraData
{
    vec3 pos;
    vec3 u;
    vec3 v;
    vec3 w;
    vec2 sensor_size;
    float focal_length;
    float exposure;
};

struct PixelData {
    vec4 col;
};

struct MeshRenderData {
    int mat_idx;
    uint indices_idx;
    uint idx_count;
};

struct Material {
    vec4 base_color;
    vec4 emission;
    float emission_strength;
    float metallic;
    float roughness;
    int base_texture;
    vec4 B_transmission;
    vec3 C;
    //int emissive_texture;
    //int metallic_roughness_texture;
    //int normal_texture;
    //int occlusion_texture;
};

struct Light {
    vec4 dir_intensity;
    vec4 pos_inner;
    vec4 color_outer;
};

struct AlignedVertex {
    vec4 pos_normal_x;
    vec4 normal_yz_color_rg;
    vec4 color_ba_tex;
};

vec3 get_vertex_pos(in AlignedVertex v) { return v.pos_normal_x.xyz; }
void set_vertex_pos(inout AlignedVertex v, in vec3 pos) { v.pos_normal_x.xyz = pos; }

vec3 get_vertex_normal(in AlignedVertex v) { return vec3(v.pos_normal_x.w, v.normal_yz_color_rg.xy); }
void set_vertex_normal(inout AlignedVertex v, in vec3 normal) {
    v.pos_normal_x.w = normal.x;
    v.normal_yz_color_rg.xy = normal.yz;
}

vec4 get_vertex_color(in AlignedVertex v) { return vec4(v.normal_yz_color_rg.zw, v.color_ba_tex.xy); }
void set_vertex_color(inout AlignedVertex v, in vec4 color) {
    v.normal_yz_color_rg.zw = color.xy;
    v.color_ba_tex.xy = color.zw;
}

vec2 get_vertex_tex(in AlignedVertex v) { return v.color_ba_tex.zw; }
void set_vertex_tex(inout AlignedVertex v, in vec2 tex) { v.color_ba_tex.zw = tex; }

struct Vertex {
    vec3 pos;
    vec3 normal;
    vec4 color;
    vec2 tex;
};

Vertex unpack_vertex(AlignedVertex v)
{
    Vertex vert;
    vert.pos = v.pos_normal_x.xyz;
    vert.normal.x = v.pos_normal_x.w;
    vert.normal.yz = v.normal_yz_color_rg.xy;
    vert.color.rg = v.normal_yz_color_rg.zw;
    vert.color.ba = v.color_ba_tex.xy;
    vert.tex = v.color_ba_tex.zw;
    return vert;
}

AlignedVertex pack_vertex(Vertex v)
{
    AlignedVertex vert;
    vert.pos_normal_x.xyz = v.pos;
    vert.pos_normal_x.w = v.normal.x;
    vert.normal_yz_color_rg.xy = v.normal.yz;
    vert.normal_yz_color_rg.zw = v.color.rg;
    vert.color_ba_tex.xy = v.color.ba;
    vert.color_ba_tex.zw = v.tex;
    return vert;
}
