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
    static std::vector< uint16_t > all_tile_index;
    static std::vector< uint16_t > all_palette_index;
    static std::vector< int16_t > all_offset_x;
    static std::vector< int16_t > all_offset_y;
    static void load(const std::string& asset_file);
};