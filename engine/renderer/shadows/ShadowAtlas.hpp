#pragma once

#include "core/Types.hpp"
#include "renderer/shadows/ShadowTypes.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace Engine {

class ShadowAtlas {
public:
    explicit ShadowAtlas(u32 atlasSize = DEFAULT_SPOT_SHADOW_ATLAS_SIZE);
    ~ShadowAtlas();

    // Non-copyable, movable
    ShadowAtlas(const ShadowAtlas&) = delete;
    ShadowAtlas& operator=(const ShadowAtlas&) = delete;
    ShadowAtlas(ShadowAtlas&& other) noexcept;
    ShadowAtlas& operator=(ShadowAtlas&& other) noexcept;

    // Tile allocation
    // Returns tile index, or -1 if allocation failed
    i32 AllocateTile(u32 requestedSize, i32 lightIndex);
    void FreeTile(i32 tileIndex);
    void FreeAllTiles();

    // Get UV scale/offset for shader (xy=scale, zw=offset)
    glm::vec4 GetTileScaleOffset(i32 tileIndex) const;
    const ShadowAtlasTile& GetTile(i32 tileIndex) const;

    // Rendering
    void BindForTile(i32 tileIndex);
    void Unbind();
    void Clear();
    void ClearTile(i32 tileIndex);

    // Texture binding for sampling
    void BindTexture(u32 slot) const;
    u32 GetTextureID() const { return m_DepthTexture; }
    u32 GetFramebufferID() const { return m_Framebuffer; }

    // Frame management for LRU eviction
    void BeginFrame(u32 frameNumber);
    u32 GetCurrentFrame() const { return m_CurrentFrame; }

    // Info
    u32 GetAtlasSize() const { return m_AtlasSize; }
    u32 GetAllocatedTileCount() const;
    u32 GetFreeTileCount() const;

private:
    void CreateResources();
    void DeleteResources();
    void InitializeTiles();

    // Find a free tile of the given size, or evict LRU if needed
    i32 FindOrEvictTile(u32 size);
    i32 FindFreeTile(u32 size);
    i32 FindLRUTile(u32 size);

    // Quantize size to power of 2 (256, 512, 1024)
    static u32 QuantizeTileSize(u32 size);

private:
    u32 m_AtlasSize;
    u32 m_CurrentFrame = 0;

    // OpenGL resources
    u32 m_DepthTexture = 0;
    u32 m_Framebuffer = 0;

    // Tile management
    // Pre-allocated grid: 4096 / 256 = 16x16 = 256 potential 256x256 tiles
    // Or 8x8 = 64 potential 512x512 tiles
    // Or 4x4 = 16 potential 1024x1024 tiles
    Vector<ShadowAtlasTile> m_Tiles;

    bool m_Initialized = false;
};

} // namespace Engine
