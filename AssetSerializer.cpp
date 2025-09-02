#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <algorithm>
// from https://stackoverflow.com/questions/11515469/how-do-i-print-vector-values-of-type-glmvec3-that-have-been-passed-by-referenc
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/hash.hpp"

#include "DataTypes.hpp"
#include "AssetSerializer.hpp"
#include "load_save_png.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"

void AssetSerializer::compile_asset(const std::string& name_mapping_file, const std::string& sprite_sheet_file, const std::string& level_map_file, const std::string& color_coding_file) {
    std::vector< Tile > all_tile; // all found sprite, sprite contains: tile number, palette table number, and name.
    std::vector< Sprite > all_sprite; // all found sprite, sprite contains: tile number, palette table number, and name.
    std::vector< std::vector<glm::u8vec4> > all_palette; // all found sprite, sprite contains: tile number, palette table number, and name.

    // read name file
    std::vector< std::string > names;
    std::ifstream name_file(name_mapping_file);
    std::string str;
    while (std::getline(name_file, str)) {
        names.emplace_back(str);
    }
    name_file.close();

    // build tile, palette, sprite
    // load from png
    std::vector< glm::u8vec4 > data;
    glm::uvec2 size;
    load_png(sprite_sheet_file, &size, &data, OriginLocation::LowerLeftOrigin);

    std::cout<< "Build sprite sheet " << std::endl;
    std::cout<< "load png success, size is " << size.x << ", "<< size.y << std::endl;
    // get upper right corner's color, it is a marker that marks disabled color.
    const glm::u8vec4 C_DISABLED_COLOR = data[data.size() - 1];
    const glm::u8vec4 C_DUMMY_COLOR = glm::u8vec4(-1, -1, -1, -1);

    for (uint32_t r = 0; r < size.y; r++) {
        for (uint32_t c = 0; c < size.x; c++) {
            if (data[r * size.x + c] != C_DISABLED_COLOR) { // check if this block is a valid one
                // std::cout << "data[" << r << "][" << c << "] = " << glm::to_string(data[r * size.x + c]) << std::endl;
                // std::cout << "new sprite found, r = " << r << ", c = " << c << std::endl;
                Sprite curr_sprite;
                Tile curr_tile;
                std::vector< glm::u8vec4 > colors;
                assert(r + 7 < size.y && c + 7 < size.x && "Invalid pixel count of a tile");

                // collect color in the 8*8 grid
                for (uint32_t sr = r; sr < r + 8; sr++) {
                    // init tile bitmap
                    uint8_t cur_tile_row_bit0 = 0b00000000;
                    uint8_t cur_tile_row_bit1 = 0b00000000;
                    for (uint32_t sc = c; sc < c + 8; sc++) {
                        glm::u8vec4 *color = &data[sr * size.x + sc];
                        auto ptr = std::find(colors.begin(), colors.end(), *color);
                        int idx = 0; // palette index of this grid.
                        if (ptr == colors.end()) { // new color in a 8*8 grid
                            assert(colors.size() < 4 && "tile palette packed.");
                            colors.push_back(*color);
                            idx = colors.size() - 1;
                            // std::cout << "new color added, color = " << glm::to_string(*color) << std::endl;
                        }
                        else { // existing color in current grid
                            idx = ptr - colors.begin();
                        }

                        // build tile bitmap of this row
                        cur_tile_row_bit0 <<= 1;
                        cur_tile_row_bit0 |= idx % 2;
                        cur_tile_row_bit1 <<= 1;
                        cur_tile_row_bit1 |= idx / 2;
                        data[sr * size.x + sc] = C_DISABLED_COLOR; // mark as seen using disabled color
                    }
                    // assign tile bit map
                    curr_tile.bit0[sr - r] = cur_tile_row_bit0;
                    curr_tile.bit1[sr - r] = cur_tile_row_bit1;
                }

                // make sure each palette set has size of 4
                while (colors.size() < 4) {
                    colors.push_back(C_DUMMY_COLOR);
                }
                all_palette.push_back(colors);
                all_tile.push_back(curr_tile);
                curr_sprite.tile_index = all_tile.size() - 1;
                curr_sprite.palette_index = all_palette.size() - 1;
                curr_sprite.name = names[all_sprite.size()];
                all_sprite.push_back(curr_sprite);

                c += 7; // skip following 7 pixels.
            }
        }
    }

    // build color coding map
    std::unordered_map< glm::u8vec4, Sprite > color_sprite_map;
    std::cout<< "Build color coding " << std::endl;
    load_png(color_coding_file, &size, &data, OriginLocation::LowerLeftOrigin);
    std::cout<< "load png success, size is " << size.x << ", "<< size.y << std::endl;
    uint16_t index = 0;
    for (int r = 0; r < size.y; r+=8) {
        for (int c = 0; c < size.x; c+=8) {
            glm::u8vec4 color = data[r * size.x + c];
            if (color != C_DISABLED_COLOR) {
                assert(color_sprite_map.find(color) == color_sprite_map.end() && "Duplicate color for different sprite!");
                color_sprite_map[color] = all_sprite[index++];
            }
        }
    }

    // Build background data
    // From PPU
    // The background is stored as a row-major grid of 16-bit values:
    //  the origin of the grid (tile (0,0)) is the bottom left of the grid
    //  each value in the grid gives:
    //    - bits 0-7: tile table index
    //    - bits 8-10: palette table index
    //    - bits 11-15: unused, should be 0
    //
    //  bits:  F E D C B A 9 8 7 6 5 4 3 2 1 0
    //        |---------|-----|---------------|
    //            ^        ^        ^-- tile index
    //            |        '----------- palette index
    //            '-------------------- unused (set to zero)
    std::cout<< "Build background " << std::endl;
    std::vector< uint16_t > background;
    std::cout<< "load png success, size is " << size.x << ", "<< size.y << std::endl;
    index = 0;
    load_png(level_map_file, &size, &data, OriginLocation::LowerLeftOrigin);
    for (int r = 0; r < size.y; r+=8) {
        for (int c = 0; c < size.x; c+=8) {
            glm::u8vec4 color = data[r * size.x + c];
            assert(color_sprite_map.find(color) != color_sprite_map.end() && "Unknown tile when building background");
            Sprite curSprite = color_sprite_map[color];
            background.push_back(curSprite.palette_index << 8 | curSprite.tile_index);
            index++;
        }
    }   

    // Save game asset
    // Prepare data
    // flattern palettes
    std::vector< glm::u8vec4 > flat_palette;
    for (auto& palette_set: all_palette) {
        for (auto& color: palette_set) {
            flat_palette.push_back(color);
        }
    }

    // flattern tiles
    std::vector< uint8_t > flat_tile_bit0;
    std::vector< uint8_t > flat_tile_bit1;
    for (auto& tile_set: all_tile) {
        for (auto& bit0: tile_set.bit0) {
            flat_tile_bit0.push_back(bit0);
        }
        for (auto& bit1: tile_set.bit1) {
            flat_tile_bit1.push_back(bit1);
        }
    }

    // Build sprite refs
    std::vector< SpriteRef > sprite_refs;
    std::vector< char > all_name;
    for (auto& sprite: all_sprite) {
        SpriteRef ref;
        ref.tile_index = sprite.tile_index;
        ref.palette_index = sprite.palette_index;
        ref.name_index_start = all_name.size();
        ref.name_size = sprite.name.size();
        for (char& c: sprite.name) {
            all_name.push_back(c);
        }
        sprite_refs.push_back(ref);
    }

    std::ofstream output_file(data_path("game.asset"));
    write_chunk("aaaa", flat_palette, &output_file);
    write_chunk("bbbb", flat_tile_bit0, &output_file);
    write_chunk("cccc", flat_tile_bit1, &output_file);
    write_chunk("dddd", all_name, &output_file);
    write_chunk("eeee", sprite_refs, &output_file);
    write_chunk("ffff", background, &output_file);
    output_file.close();
    std::cout<< "--Success-- Build game asset finished." << std::endl;
}
