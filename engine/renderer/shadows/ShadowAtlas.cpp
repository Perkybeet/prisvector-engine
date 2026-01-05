#include "renderer/shadows/ShadowAtlas.hpp"
#include "core/Logger.hpp"

#include <glad/gl.h>
#include <algorithm>

namespace Engine {

ShadowAtlas::ShadowAtlas(u32 atlasSize)
    : m_AtlasSize(atlasSize) {
    CreateResources();
    InitializeTiles();
}

ShadowAtlas::~ShadowAtlas() {
    DeleteResources();
}

ShadowAtlas::ShadowAtlas(ShadowAtlas&& other) noexcept
    : m_AtlasSize(other.m_AtlasSize)
    , m_CurrentFrame(other.m_CurrentFrame)
    , m_DepthTexture(other.m_DepthTexture)
    , m_Framebuffer(other.m_Framebuffer)
    , m_Tiles(std::move(other.m_Tiles))
    , m_Initialized(other.m_Initialized) {
    other.m_DepthTexture = 0;
    other.m_Framebuffer = 0;
    other.m_Initialized = false;
}

ShadowAtlas& ShadowAtlas::operator=(ShadowAtlas&& other) noexcept {
    if (this != &other) {
        DeleteResources();

        m_AtlasSize = other.m_AtlasSize;
        m_CurrentFrame = other.m_CurrentFrame;
        m_DepthTexture = other.m_DepthTexture;
        m_Framebuffer = other.m_Framebuffer;
        m_Tiles = std::move(other.m_Tiles);
        m_Initialized = other.m_Initialized;

        other.m_DepthTexture = 0;
        other.m_Framebuffer = 0;
        other.m_Initialized = false;
    }
    return *this;
}

void ShadowAtlas::CreateResources() {
    // Create depth texture
    glCreateTextures(GL_TEXTURE_2D, 1, &m_DepthTexture);
    glTextureStorage2D(m_DepthTexture, 1, GL_DEPTH_COMPONENT32F, m_AtlasSize, m_AtlasSize);

    // Texture parameters
    glTextureParameteri(m_DepthTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_DepthTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_DepthTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(m_DepthTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Border color = 1.0 (no shadow outside atlas)
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTextureParameterfv(m_DepthTexture, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Enable hardware depth comparison
    glTextureParameteri(m_DepthTexture, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTextureParameteri(m_DepthTexture, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    // Create framebuffer
    glCreateFramebuffers(1, &m_Framebuffer);
    glNamedFramebufferTexture(m_Framebuffer, GL_DEPTH_ATTACHMENT, m_DepthTexture, 0);
    glNamedFramebufferDrawBuffer(m_Framebuffer, GL_NONE);
    glNamedFramebufferReadBuffer(m_Framebuffer, GL_NONE);

    // Verify framebuffer
    GLenum status = glCheckNamedFramebufferStatus(m_Framebuffer, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_CORE_ERROR("ShadowAtlas framebuffer incomplete: 0x{:X}", status);
    }

    m_Initialized = true;

    LOG_CORE_DEBUG("Created ShadowAtlas {}x{}", m_AtlasSize, m_AtlasSize);
}

void ShadowAtlas::DeleteResources() {
    if (m_Framebuffer) {
        glDeleteFramebuffers(1, &m_Framebuffer);
        m_Framebuffer = 0;
    }
    if (m_DepthTexture) {
        glDeleteTextures(1, &m_DepthTexture);
        m_DepthTexture = 0;
    }
    m_Initialized = false;
}

void ShadowAtlas::InitializeTiles() {
    m_Tiles.clear();

    // Create a grid of 512x512 tiles (8x8 = 64 tiles for 4096 atlas)
    // This gives a good balance between tile count and resolution
    constexpr u32 defaultTileSize = 512;
    u32 tilesPerRow = m_AtlasSize / defaultTileSize;

    for (u32 y = 0; y < tilesPerRow; ++y) {
        for (u32 x = 0; x < tilesPerRow; ++x) {
            ShadowAtlasTile tile;
            tile.X = x * defaultTileSize;
            tile.Y = y * defaultTileSize;
            tile.Size = defaultTileSize;
            tile.LightIndex = -1;
            tile.LastUsedFrame = 0;
            m_Tiles.push_back(tile);
        }
    }

    LOG_CORE_DEBUG("ShadowAtlas initialized with {} tiles ({}x{})",
                   m_Tiles.size(), defaultTileSize, defaultTileSize);
}

u32 ShadowAtlas::QuantizeTileSize(u32 size) {
    // Quantize to power of 2: 256, 512, or 1024
    if (size <= 256) return 256;
    if (size <= 512) return 512;
    return 1024;
}

i32 ShadowAtlas::AllocateTile(u32 requestedSize, i32 lightIndex) {
    u32 size = QuantizeTileSize(requestedSize);

    i32 tileIndex = FindOrEvictTile(size);
    if (tileIndex < 0) {
        LOG_CORE_WARN("ShadowAtlas: Failed to allocate tile for light {}", lightIndex);
        return -1;
    }

    m_Tiles[tileIndex].LightIndex = lightIndex;
    m_Tiles[tileIndex].LastUsedFrame = m_CurrentFrame;

    return tileIndex;
}

void ShadowAtlas::FreeTile(i32 tileIndex) {
    if (tileIndex < 0 || static_cast<size_t>(tileIndex) >= m_Tiles.size()) return;

    m_Tiles[tileIndex].LightIndex = -1;
    m_Tiles[tileIndex].LastUsedFrame = 0;
}

void ShadowAtlas::FreeAllTiles() {
    for (auto& tile : m_Tiles) {
        tile.LightIndex = -1;
        tile.LastUsedFrame = 0;
    }
}

i32 ShadowAtlas::FindOrEvictTile(u32 size) {
    // First try to find a free tile
    i32 freeIndex = FindFreeTile(size);
    if (freeIndex >= 0) return freeIndex;

    // No free tiles, try LRU eviction
    return FindLRUTile(size);
}

i32 ShadowAtlas::FindFreeTile(u32 size) {
    for (size_t i = 0; i < m_Tiles.size(); ++i) {
        if (m_Tiles[i].IsFree() && m_Tiles[i].Size >= size) {
            return static_cast<i32>(i);
        }
    }
    return -1;
}

i32 ShadowAtlas::FindLRUTile(u32 size) {
    i32 lruIndex = -1;
    u32 oldestFrame = m_CurrentFrame;

    for (size_t i = 0; i < m_Tiles.size(); ++i) {
        if (m_Tiles[i].Size >= size && m_Tiles[i].LastUsedFrame < oldestFrame) {
            oldestFrame = m_Tiles[i].LastUsedFrame;
            lruIndex = static_cast<i32>(i);
        }
    }

    return lruIndex;
}

glm::vec4 ShadowAtlas::GetTileScaleOffset(i32 tileIndex) const {
    if (tileIndex < 0 || static_cast<size_t>(tileIndex) >= m_Tiles.size()) {
        return glm::vec4(0.0f);
    }
    return m_Tiles[tileIndex].GetScaleOffset(m_AtlasSize);
}

const ShadowAtlasTile& ShadowAtlas::GetTile(i32 tileIndex) const {
    static ShadowAtlasTile empty;
    if (tileIndex < 0 || static_cast<size_t>(tileIndex) >= m_Tiles.size()) {
        return empty;
    }
    return m_Tiles[tileIndex];
}

void ShadowAtlas::BindForTile(i32 tileIndex) {
    if (tileIndex < 0 || static_cast<size_t>(tileIndex) >= m_Tiles.size()) {
        LOG_CORE_ERROR("ShadowAtlas: Invalid tile index {}", tileIndex);
        return;
    }

    const auto& tile = m_Tiles[tileIndex];

    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

    // Set viewport to tile region
    glViewport(tile.X, tile.Y, tile.Size, tile.Size);

    // Enable scissor test to prevent rendering outside tile
    glEnable(GL_SCISSOR_TEST);
    glScissor(tile.X, tile.Y, tile.Size, tile.Size);

    // Depth settings
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    // Disable color writing
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // Polygon offset to reduce shadow acne
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.1f, 4.0f);

    // Update last used frame
    m_Tiles[tileIndex].LastUsedFrame = m_CurrentFrame;
}

void ShadowAtlas::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Restore state
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void ShadowAtlas::Clear() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glViewport(0, 0, m_AtlasSize, m_AtlasSize);

    f32 clearDepth = 1.0f;
    glClearNamedFramebufferfv(m_Framebuffer, GL_DEPTH, 0, &clearDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowAtlas::ClearTile(i32 tileIndex) {
    if (tileIndex < 0 || static_cast<size_t>(tileIndex) >= m_Tiles.size()) return;

    const auto& tile = m_Tiles[tileIndex];

    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glEnable(GL_SCISSOR_TEST);
    glScissor(tile.X, tile.Y, tile.Size, tile.Size);

    f32 clearDepth = 1.0f;
    glClearNamedFramebufferfv(m_Framebuffer, GL_DEPTH, 0, &clearDepth);

    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowAtlas::BindTexture(u32 slot) const {
    glBindTextureUnit(slot, m_DepthTexture);
}

void ShadowAtlas::BeginFrame(u32 frameNumber) {
    m_CurrentFrame = frameNumber;
}

u32 ShadowAtlas::GetAllocatedTileCount() const {
    u32 count = 0;
    for (const auto& tile : m_Tiles) {
        if (!tile.IsFree()) ++count;
    }
    return count;
}

u32 ShadowAtlas::GetFreeTileCount() const {
    return static_cast<u32>(m_Tiles.size()) - GetAllocatedTileCount();
}

} // namespace Engine
