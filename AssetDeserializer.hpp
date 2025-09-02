#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "DataTypes.hpp"
struct AssetDeserializer {
    static std::vector< glm::u8vec4 > flat_palette;
    static std::vector< uint8_t > flat_tile_bit0;
    static std::vector< uint8_t > flat_tile_bit1;
    static std::vector< SpriteRef > sprite_refs;
    static std::vector< char > all_name;
    static std::vector< uint16_t > background;
    static void load(const std::string& asset_file);
};