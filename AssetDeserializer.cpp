#include <iostream>
#include <fstream>
#include <vector>
// from https://stackoverflow.com/questions/11515469/how-do-i-print-vector-values-of-type-glmvec3-that-have-been-passed-by-referenc
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#include "DataTypes.hpp"
#include "AssetDeserializer.hpp"
#include "read_write_chunk.hpp"

std::vector< glm::u8vec4 > AssetDeserializer::flat_palette;
std::vector< uint8_t > AssetDeserializer::flat_tile_bit0;
std::vector< uint8_t > AssetDeserializer::flat_tile_bit1;
std::vector< SpriteRef > AssetDeserializer::sprite_refs;
std::vector< char > AssetDeserializer::all_name;
std::vector< uint16_t > AssetDeserializer::background;

void AssetDeserializer::load(const std::string& asset_file) {
    std::ifstream input_file(asset_file);
    if (!input_file.is_open()) {
		throw std::runtime_error("Failed to open file");
    }
    read_chunk(input_file, "aaaa", &flat_palette);
    read_chunk(input_file, "bbbb", &flat_tile_bit0);
    read_chunk(input_file, "cccc", &flat_tile_bit1);
    read_chunk(input_file, "dddd", &all_name);
    read_chunk(input_file, "eeee", &sprite_refs);
    read_chunk(input_file, "ffff", &background);
    input_file.close();
    std::cout<< "--Success-- Deserialize file finished." << std::endl;
}