#pragma once

class Vertex
{
public:
    glm::vec3 aPos       {};
    glm::vec2 aTexCoords {};
    glm::vec3 aNormal    {};

    bool operator==( const Vertex& other ) const
    {
        return aPos == other.aPos && aTexCoords == other.aTexCoords && aNormal == other.aNormal;
    }

    static constexpr VkVertexInputBindingDescription GetBindingDesc()
    {
        VkVertexInputBindingDescription b{};
        b.binding   = 0;
        b.stride    = sizeof( Vertex );
        b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return b;
    }

    static constexpr std::array< VkVertexInputAttributeDescription, 3 > GetAttributeDesc()
    {
        std::array< VkVertexInputAttributeDescription, 3 > a{};
        a[0].binding   = 0;
        a[0].location  = 0;
        a[0].format    = VK_FORMAT_R32G32B32_SFLOAT;
        a[0].offset    = offsetof( Vertex, aPos );

        a[1].binding   = 0;
        a[1].location  = 1;
        a[1].format    = VK_FORMAT_R32G32_SFLOAT;
        a[1].offset    = offsetof( Vertex, aTexCoords );

        a[2].binding   = 0;
        a[2].location  = 2;
        a[2].format    = VK_FORMAT_R32G32B32_SFLOAT;
        a[2].offset    = offsetof( Vertex, aNormal );

        return a;
    }

    Vertex();
    ~Vertex();
};