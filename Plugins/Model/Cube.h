#ifndef GEK_STATIC_MODEL
#define GEK_STATIC_MODEL
struct StaticModel
{
    std::string material;
    std::vector<uint16_t> indices;
    std::vector<Math::Float3> positions;
    std::vector<Math::Float2> texCoords;
    std::vector<Math::Float4> tangents;
    std::vector<Math::Float3> normals;

    StaticModel(std::string &&material, 
        std::vector<Math::Float3> &&positions,
        std::vector<Math::Float2> &&texCoords,
        std::vector<Math::Float4> &&tangents,
        std::vector<Math::Float3> &&normals)
        : material(std::move(material))
        , indices(std::move(indices))
        , positions(std::move(positions))
        , texCoords(std::move(texCoords))
        , tangents(std::move(tangents))
        , normals(std::move(normals))
    {
    }
};
#endif
static const StaticModel cube_models[] = {
   StaticModel("debug_x.json",
       std::vector<Math::Float3>({ Math::Float3(-0.50000f, -0.50000f, -0.50000f), Math::Float3(-0.50000f, -0.50000f, 0.50000f), Math::Float3(-0.50000f, 0.50000f, -0.50000f), Math::Float3(0.50000f, -0.50000f, 0.50000f), Math::Float3(0.50000f, -0.50000f, -0.50000f), Math::Float3(0.50000f, 0.50000f, 0.50000f), Math::Float3(-0.50000f, -0.50000f, 0.50000f), Math::Float3(-0.50000f, 0.50000f, 0.50000f), Math::Float3(-0.50000f, 0.50000f, -0.50000f), Math::Float3(0.50000f, -0.50000f, -0.50000f), Math::Float3(0.50000f, 0.50000f, -0.50000f), Math::Float3(0.50000f, 0.50000f, 0.50000f), }),
       std::vector<Math::Float2>({ Math::Float2(1.00000f, 1.00000f), Math::Float2(0.00000f, 1.00000f), Math::Float2(1.00000f, 0.00000f), Math::Float2(1.00000f, 1.00000f), Math::Float2(0.00000f, 1.00000f), Math::Float2(1.00000f, 0.00000f), Math::Float2(0.00000f, 1.00000f), Math::Float2(0.00000f, 0.00000f), Math::Float2(1.00000f, 0.00000f), Math::Float2(0.00000f, 1.00000f), Math::Float2(0.00000f, 0.00000f), Math::Float2(1.00000f, 0.00000f), }),
       std::vector<Math::Float4>({ Math::Float4(0.00000f, 0.00000f, -1.00000f, 1.00000f), Math::Float4(0.00000f, 0.00000f, -1.00000f, 1.00000f), Math::Float4(0.00000f, 0.00000f, -1.00000f, 1.00000f), Math::Float4(0.00000f, -0.00000f, 1.00000f, 1.00000f), Math::Float4(0.00000f, -0.00000f, 1.00000f, 1.00000f), Math::Float4(0.00000f, -0.00000f, 1.00000f, 1.00000f), Math::Float4(0.00000f, 0.00000f, -1.00000f, 1.00000f), Math::Float4(0.00000f, 0.00000f, -1.00000f, 1.00000f), Math::Float4(0.00000f, 0.00000f, -1.00000f, 1.00000f), Math::Float4(0.00000f, -0.00000f, 1.00000f, 1.00000f), Math::Float4(0.00000f, -0.00000f, 1.00000f, 1.00000f), Math::Float4(0.00000f, -0.00000f, 1.00000f, 1.00000f), }),
       std::vector<Math::Float3>({ Math::Float3(-100.00000f, 0.00000f, 0.00000f), Math::Float3(-100.00000f, 0.00000f, 0.00000f), Math::Float3(-100.00000f, 0.00000f, 0.00000f), Math::Float3(100.00000f, 0.00000f, 0.00000f), Math::Float3(100.00000f, 0.00000f, 0.00000f), Math::Float3(100.00000f, 0.00000f, 0.00000f), Math::Float3(-100.00000f, 0.00000f, 0.00000f), Math::Float3(-100.00000f, 0.00000f, 0.00000f), Math::Float3(-100.00000f, 0.00000f, 0.00000f), Math::Float3(100.00000f, 0.00000f, 0.00000f), Math::Float3(100.00000f, 0.00000f, 0.00000f), Math::Float3(100.00000f, 0.00000f, 0.00000f), })),
   StaticModel("debug_y.json",
       std::vector<Math::Float3>({ Math::Float3(0.50000f, 0.50000f, 0.50000f), Math::Float3(0.50000f, 0.50000f, -0.50000f), Math::Float3(-0.50000f, 0.50000f, 0.50000f), Math::Float3(-0.50000f, -0.50000f, 0.50000f), Math::Float3(-0.50000f, -0.50000f, -0.50000f), Math::Float3(0.50000f, -0.50000f, 0.50000f), Math::Float3(0.50000f, 0.50000f, -0.50000f), Math::Float3(-0.50000f, 0.50000f, -0.50000f), Math::Float3(-0.50000f, 0.50000f, 0.50000f), Math::Float3(-0.50000f, -0.50000f, -0.50000f), Math::Float3(0.50000f, -0.50000f, -0.50000f), Math::Float3(0.50000f, -0.50000f, 0.50000f), }),
       std::vector<Math::Float2>({ Math::Float2(1.00000f, 0.00000f), Math::Float2(1.00000f, 1.00000f), Math::Float2(0.00000f, 0.00000f), Math::Float2(0.00000f, 1.00000f), Math::Float2(0.00000f, 0.00000f), Math::Float2(1.00000f, 1.00000f), Math::Float2(1.00000f, 1.00000f), Math::Float2(0.00000f, 1.00000f), Math::Float2(0.00000f, 0.00000f), Math::Float2(0.00000f, 0.00000f), Math::Float2(1.00000f, 0.00000f), Math::Float2(1.00000f, 1.00000f), }),
       std::vector<Math::Float4>({ Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), }),
       std::vector<Math::Float3>({ Math::Float3(0.00000f, 100.00000f, 0.00003f), Math::Float3(0.00000f, 100.00000f, 0.00003f), Math::Float3(0.00000f, 100.00000f, 0.00003f), Math::Float3(0.00000f, -100.00000f, -0.00003f), Math::Float3(0.00000f, -100.00000f, -0.00003f), Math::Float3(0.00000f, -100.00000f, -0.00003f), Math::Float3(0.00000f, 100.00000f, 0.00003f), Math::Float3(0.00000f, 100.00000f, 0.00003f), Math::Float3(0.00000f, 100.00000f, 0.00003f), Math::Float3(0.00000f, -100.00000f, -0.00003f), Math::Float3(0.00000f, -100.00000f, -0.00003f), Math::Float3(0.00000f, -100.00000f, -0.00003f), })),
   StaticModel("debug_z.json",
       std::vector<Math::Float3>({ Math::Float3(0.50000f, -0.50000f, -0.50000f), Math::Float3(-0.50000f, -0.50000f, -0.50000f), Math::Float3(0.50000f, 0.50000f, -0.50000f), Math::Float3(-0.50000f, -0.50000f, 0.50000f), Math::Float3(0.50000f, -0.50000f, 0.50000f), Math::Float3(-0.50000f, 0.50000f, 0.50000f), Math::Float3(-0.50000f, -0.50000f, -0.50000f), Math::Float3(-0.50000f, 0.50000f, -0.50000f), Math::Float3(0.50000f, 0.50000f, -0.50000f), Math::Float3(0.50000f, -0.50000f, 0.50000f), Math::Float3(0.50000f, 0.50000f, 0.50000f), Math::Float3(-0.50000f, 0.50000f, 0.50000f), }),
       std::vector<Math::Float2>({ Math::Float2(1.00000f, 1.00000f), Math::Float2(0.00000f, 1.00000f), Math::Float2(1.00000f, 0.00000f), Math::Float2(1.00000f, 1.00000f), Math::Float2(0.00000f, 1.00000f), Math::Float2(1.00000f, 0.00000f), Math::Float2(0.00000f, 1.00000f), Math::Float2(0.00000f, 0.00000f), Math::Float2(1.00000f, 0.00000f), Math::Float2(0.00000f, 1.00000f), Math::Float2(0.00000f, 0.00000f), Math::Float2(1.00000f, 0.00000f), }),
       std::vector<Math::Float4>({ Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(-1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(-1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(-1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(-1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(-1.00000f, 0.00000f, 0.00000f, 1.00000f), Math::Float4(-1.00000f, 0.00000f, 0.00000f, 1.00000f), }),
       std::vector<Math::Float3>({ Math::Float3(0.00000f, 0.00003f, -100.00000f), Math::Float3(0.00000f, 0.00003f, -100.00000f), Math::Float3(0.00000f, 0.00003f, -100.00000f), Math::Float3(0.00000f, -0.00003f, 100.00000f), Math::Float3(0.00000f, -0.00003f, 100.00000f), Math::Float3(0.00000f, -0.00003f, 100.00000f), Math::Float3(0.00000f, 0.00003f, -100.00000f), Math::Float3(0.00000f, 0.00003f, -100.00000f), Math::Float3(0.00000f, 0.00003f, -100.00000f), Math::Float3(0.00000f, -0.00003f, 100.00000f), Math::Float3(0.00000f, -0.00003f, 100.00000f), Math::Float3(0.00000f, -0.00003f, 100.00000f), })),

};