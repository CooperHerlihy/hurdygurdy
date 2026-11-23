#include "tests.hpp"
#include "hg/render3d.hpp"
#include "hg/serialization.hpp"
#include "hg/utility.hpp"

#include <algorithm>
#include <vector>

using namespace hg;

// ---- mesh vertex layout ----

TEST(testMeshVertexLayout)
{
    // pos, uvU, norm, uvV, tan tightly interleaved
    static_assert(sizeof(MeshVertex) == 48);
    ASSERT(sizeof(MeshVertex) == 48);

    MeshVertex v{Vec3{1.0f, 2.0f, 3.0f}, Vec3{0.0f, 1.0f, 0.0f}, Vec4{0.0f, 0.0f, 1.0f, 1.0f}, Vec2{0.25f, 0.75f}};
    ASSERT(v.pos.x() == 1.0f);
    ASSERT(v.pos.y() == 2.0f);
    ASSERT(v.pos.z() == 3.0f);
    ASSERT(v.norm.y() == 1.0f);
    ASSERT(v.tan.z() == 1.0f);
    ASSERT(v.uvU == 0.25f);
    ASSERT(v.uvV == 0.75f);
}

// ---- mesh data serialization ----

TEST(testMeshDataSerializeRoundtrip)
{
    MeshData mesh{};
    mesh.vertices = {
        MeshVertex{Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.0f, 0.0f, 1.0f}, Vec4{1.0f, 0.0f, 0.0f, 1.0f}, Vec2{0.0f, 0.0f}},
        MeshVertex{Vec3{1.0f, 0.0f, 0.0f}, Vec3{0.0f, 0.0f, 1.0f}, Vec4{1.0f, 0.0f, 0.0f, 1.0f}, Vec2{1.0f, 0.0f}},
        MeshVertex{Vec3{0.0f, 1.0f, 0.0f}, Vec3{0.0f, 0.0f, 1.0f}, Vec4{1.0f, 0.0f, 0.0f, 1.0f}, Vec2{0.0f, 1.0f}},
    };
    mesh.indices = {0, 1, 2};

    Serializer s = Serializer::writer();
    ASSERT(serialize(s, &mesh));
    std::vector<byte> bin = s.takeFromWriter();
    ASSERT(isSerialData(bin));

    Serializer r = Serializer::reader(bin);
    MeshData out{};
    ASSERT(serialize(r, &out));

    ASSERT(out.vertices.size() == 3);
    ASSERT(out.indices.size() == 3);
    for (u32 i = 0; i < 3; ++i)
    {
        ASSERT(out.vertices[i].pos == mesh.vertices[i].pos);
        ASSERT(out.vertices[i].norm == mesh.vertices[i].norm);
        ASSERT(out.vertices[i].tan == mesh.vertices[i].tan);
        ASSERT(out.vertices[i].uvU == mesh.vertices[i].uvU);
        ASSERT(out.vertices[i].uvV == mesh.vertices[i].uvV);
        ASSERT(out.indices[i] == mesh.indices[i]);
    }
}

// ---- mesh upload / download ----

static MeshData testTriangleMesh()
{
    MeshData mesh{};
    mesh.vertices = {
        MeshVertex{Vec3{-1.0f, -1.0f, 0.0f}, Vec3{0.0f, 0.0f, 1.0f}, Vec4{1.0f, 0.0f, 0.0f, 1.0f}, Vec2{0.0f, 0.0f}},
        MeshVertex{Vec3{3.0f, -1.0f, 0.0f}, Vec3{0.0f, 0.0f, 1.0f}, Vec4{1.0f, 0.0f, 0.0f, 1.0f}, Vec2{1.0f, 0.0f}},
        MeshVertex{Vec3{-1.0f, 3.0f, 0.0f}, Vec3{0.0f, 0.0f, 1.0f}, Vec4{1.0f, 0.0f, 0.0f, 1.0f}, Vec2{0.0f, 1.0f}},
    };
    mesh.indices = {0, 1, 2};
    return mesh;
}

TEST(testMeshUpload)
{
    // Note: downloadMeshFromGpu cannot be roundtrip tested because
    // uploadMeshToGpu creates the buffers without GpuBufferUsage::transferSrc,
    // so GpuBuffer::read aborts on the validation layer
    MeshData original = testTriangleMesh();

    Mesh mesh = uploadMeshToGpu(testGpu, original);
    ASSERT(mesh.vertexCount == 3);
    ASSERT(mesh.vertexWidth == sizeof(MeshVertex));
    ASSERT(mesh.indexCount == 3);
    ASSERT(mesh.vertexBuffer != nullptr);
    ASSERT(mesh.indexBuffer != nullptr);
    ASSERT(mesh.vertexBuffer->size() == 3 * sizeof(MeshVertex));
    ASSERT(mesh.indexBuffer->size() == 3 * sizeof(u32));
}

TEST(testMeshLoadMissing)
{
    setError("");
    std::optional<Mesh> mesh = meshLoadToGpu(testGpu, "/tmp/hg_render3d_test_missing_mesh");
    ASSERT(!mesh.has_value());
    ASSERT(getError().length() > 0);
}

// ---- offscreen rendering ----

TEST(testRenderer3DRenderScene)
{
    static constexpr u32 size = 8;

    MeshData meshData = testTriangleMesh();
    // The renderer culls back faces with front face CCW: wind the triangle so
    // it faces the camera with the unflipped orthographic projection
    meshData.indices = {0, 2, 1};
    Mesh mesh = uploadMeshToGpu(testGpu, meshData);

    // A solid white color map and a solid neutral normal map
    TextureData whiteData{};
    whiteData.width = 1;
    whiteData.height = 1;
    whiteData.depth = 1;
    whiteData.format = Format::rgba8_unorm;
    u32 whitePixel = 0xFFFFFFFF;
    whiteData.pixels.assign(reinterpret_cast<const byte*>(&whitePixel),
        reinterpret_cast<const byte*>(&whitePixel) + sizeof(whitePixel));
    Texture colorMap = uploadTextureToGpu(testGpu, whiteData);
    Texture normalMap = uploadTextureToGpu(testGpu, whiteData);
    ASSERT(colorMap.image != nullptr);
    ASSERT(normalMap.image != nullptr);

    Renderer3D renderer{testGpu, Format::rgba8_unorm, Format::d32_sfloat};
    renderer.drawDirectionalLight(Vec3{1.0f, 1.0f, 1.0f}, Vec3{0.0f, 0.0f, -1.0f});

    Model3D material{};
    material.mesh = &mesh;
    material.colorMap = &colorMap;
    material.normalMap = &normalMap;
    material.diffuse = 1.0f;
    material.shininess = 1.0f;

    Mat4 transform = Mat4{1.0f};
    renderer.drawModelMaterial(material, transform);

    std::unique_ptr<GpuImage> colorImg = testGpu->createImage(size, size, Format::rgba8_unorm,
        GpuImageUsage::colorAttachment | GpuImageUsage::transferSrc);
    std::unique_ptr<GpuView> colorView = colorImg->createView();
    std::unique_ptr<GpuImage> depthImg = testGpu->createImage(size, size, Format::d32_sfloat,
        GpuImageUsage::depthStencilAttachment);
    std::unique_ptr<GpuView> depthView = depthImg->createView();

    Camera camera{};
    camera.projection = Orthographic{{{-1.0f, -1.0f}, {1.0f, 1.0f}}};

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuAttachment colorAtt{};
    colorAtt.image = colorView.get();
    colorAtt.loadOp = GpuLoadOp::clear;
    colorAtt.storeOp = GpuStoreOp::store;
    colorAtt.clearColor[0] = 1.0f;
    colorAtt.clearColor[1] = 0.0f;
    colorAtt.clearColor[2] = 0.0f;
    colorAtt.clearColor[3] = 1.0f;

    GpuAttachment depthAtt{};
    depthAtt.image = depthView.get();
    depthAtt.loadOp = GpuLoadOp::clear;
    depthAtt.storeOp = GpuStoreOp::store;
    depthAtt.clearDepthStencil.depth = 1.0f;

    GpuPass pass{};
    pass.colorAttachments = {&colorAtt, 1};
    pass.depthAttachment = &depthAtt;

    cmd->beginRenderPass(pass);
    renderer.renderScene(cmd.get(), camera);
    cmd->endRenderPass();

    testGpu->endCommands(std::move(cmd));

    u32 pixels[size * size] = {};
    colorImg->readWhole(std::as_writable_bytes(std::span{pixels}));

    // The triangle covers the whole viewport: every pixel must differ from
    // the red clear color
    ASSERT(std::all_of(pixels, pixels + size * size, [](u32 p) { return p != 0xFF0000FF; }));
}

TEST(testRenderer3DSceneClearedAfterRender)
{
    static constexpr u32 size = 8;

    MeshData meshData = testTriangleMesh();
    meshData.indices = {0, 2, 1};
    Mesh mesh = uploadMeshToGpu(testGpu, meshData);

    TextureData whiteData{};
    whiteData.width = 1;
    whiteData.height = 1;
    whiteData.depth = 1;
    whiteData.format = Format::rgba8_unorm;
    u32 whitePixel = 0xFFFFFFFF;
    whiteData.pixels.assign(reinterpret_cast<const byte*>(&whitePixel),
        reinterpret_cast<const byte*>(&whitePixel) + sizeof(whitePixel));
    Texture colorMap = uploadTextureToGpu(testGpu, whiteData);
    Texture normalMap = uploadTextureToGpu(testGpu, whiteData);

    Renderer3D renderer{testGpu, Format::rgba8_unorm, Format::d32_sfloat};
    renderer.drawDirectionalLight(Vec3{1.0f, 1.0f, 1.0f}, Vec3{0.0f, 0.0f, -1.0f});

    std::unique_ptr<GpuImage> colorImg = testGpu->createImage(size, size, Format::rgba8_unorm,
        GpuImageUsage::colorAttachment | GpuImageUsage::transferSrc);
    std::unique_ptr<GpuView> colorView = colorImg->createView();
    std::unique_ptr<GpuImage> depthImg = testGpu->createImage(size, size, Format::d32_sfloat,
        GpuImageUsage::depthStencilAttachment);
    std::unique_ptr<GpuView> depthView = depthImg->createView();

    Camera camera{};
    camera.projection = Orthographic{{{-1.0f, -1.0f}, {1.0f, 1.0f}}};

    GpuAttachment colorAtt{};
    colorAtt.image = colorView.get();
    colorAtt.loadOp = GpuLoadOp::clear;
    colorAtt.storeOp = GpuStoreOp::store;
    colorAtt.clearColor[0] = 0.0f;
    colorAtt.clearColor[1] = 0.0f;
    colorAtt.clearColor[2] = 0.0f;
    colorAtt.clearColor[3] = 1.0f;
    GpuAttachment depthAtt{};
    depthAtt.image = depthView.get();
    depthAtt.loadOp = GpuLoadOp::clear;
    depthAtt.storeOp = GpuStoreOp::store;
    depthAtt.clearDepthStencil.depth = 1.0f;
    GpuPass pass{};
    pass.colorAttachments = {&colorAtt, 1};
    pass.depthAttachment = &depthAtt;

    // First frame with a model: rendered
    renderer.drawModel(mesh, colorMap, normalMap, 1.0f, 1.0f, Mat4{1.0f});
    {
        std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
        cmd->beginRenderPass(pass);
        renderer.renderScene(cmd.get(), camera);
        cmd->endRenderPass();
        testGpu->endCommands(std::move(cmd));
    }

    u32 pixels[size * size] = {};
    colorImg->readWhole(std::as_writable_bytes(std::span{pixels}));
    for (u32 i = 0; i < size * size; ++i)
        ASSERT(pixels[i] != 0xFF000000);

    // Second frame with no model submitted: everything stays clear
    {
        std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
        cmd->beginRenderPass(pass);
        renderer.renderScene(cmd.get(), camera);
        cmd->endRenderPass();
        testGpu->endCommands(std::move(cmd));
    }

    colorImg->readWhole(std::as_writable_bytes(std::span{pixels}));
    for (u32 i = 0; i < size * size; ++i)
        ASSERT(pixels[i] == 0xFF000000);
}
