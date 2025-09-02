#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
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

void AssetSerializer::compile_asset(const std::string& name_mapping_file, const std::string& sprite_sheet_file, const std::string& level_map_file, const std::string& color_coding_file, const std::string& sprite_group_color_coding_file) {
    std::vector< Tile > all_tile; // all found sprite, sprite contains: tile number, palette table number, and name.
    std::vector< SpritePiece > all_sprite_piece; // all found sprite, sprite contains: tile number, palette table number, and name.
    std::vector< std::vector<glm::u8vec4> > all_palette; // all found sprite, sprite contains: tile number, palette table number, and name.

    // comparison function for vec4
    // reference: https://stackoverflow.com/questions/1380463/how-do-i-sort-a-vector-of-custom-objects
    struct ascend_comparison
    {
        inline bool operator() (const glm::u8vec4& color_a, const glm::u8vec4& color_b)
        {
            if (color_a.x != color_b.x) {
                return (color_a.x < color_b.x);
            }
            else if (color_a.y != color_b.y) {
                return (color_a.y < color_b.y);
                
            }
            else if (color_a.z != color_b.z) {
                return (color_a.z < color_b.z);
                
            }
            else if (color_a.a != color_b.a) {
                return (color_a.a < color_b.a);
            }
            else {
                return false;
            }
        }
    };

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
                SpritePiece curr_sprite_piece;
                Tile curr_tile;
                std::vector< glm::u8vec4 > colors;
                assert(r + 7 < size.y && c + 7 < size.x && "Invalid pixel count of a tile");

                // collect color in the 8*8 grid
                for (uint32_t sr = r; sr < r + 8; sr++) {
                    for (uint32_t sc = c; sc < c + 8; sc++) {
                        glm::u8vec4 *color = &data[sr * size.x + sc];
                        auto ptr = std::find(colors.begin(), colors.end(), *color);
                        if (ptr == colors.end()) {
                            assert(colors.size() < 4 && "tile palette packed.");
                            colors.push_back(*color);
                        }
                    }
                }

                // make sure each palette set has size of 4
                while (colors.size() < 4) {
                    colors.push_back(C_DUMMY_COLOR);
                }

                // sort current palette
                std::sort(colors.begin(), colors.end(), ascend_comparison());
                
                // get correct all_palette index
                uint8_t palette_index = 10;
                for (uint8_t i = 0; i < all_palette.size(); i++) {
                    if (colors == all_palette[i]) { // color exists
                        palette_index = i;
                        break;
                    }
                }

                // new palette
                if (palette_index == 10) {
                    assert( all_palette.size() < 8 && "palette packed.");
                    all_palette.push_back(colors);
                    palette_index = all_palette.size() - 1;
                }
                
                // Build tile map
                for (uint32_t sr = r; sr < r + 8; sr++) {
                    // init tile bitmap
                    uint8_t cur_tile_row_bit0 = 0b00000000;
                    uint8_t cur_tile_row_bit1 = 0b00000000;
                    for (uint32_t sc = c; sc < c + 8; sc++) {
                        glm::u8vec4 *color = &data[sr * size.x + sc];
                        auto ptr = std::find(colors.begin(), colors.end(), *color);
                        assert(ptr != colors.end() && "color should exists in colors.");
                        int idx = ptr - colors.begin(); // palette index of this grid.

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
                // all_palette.push_back(colors);
                all_tile.push_back(curr_tile);
                curr_sprite_piece.tile_index = all_tile.size() - 1;
                curr_sprite_piece.palette_index = palette_index;
                //curr_sprite_piece.name = names[all_sprite_piece.size()];
                all_sprite_piece.push_back(curr_sprite_piece);

                c += 7; // skip following 7 pixels.
            }
        }
    }

    // build color coding map
    std::unordered_map< glm::u8vec4, SpritePiece > color_sprite_map;
    std::cout<< "Build color coding " << std::endl;
    load_png(color_coding_file, &size, &data, OriginLocation::LowerLeftOrigin);
    std::cout<< "load png success, size is " << size.x << ", "<< size.y << std::endl;
    uint16_t index = 0;
    for (int r = 0; r < size.y; r+=8) {
        for (int c = 0; c < size.x; c+=8) {
            glm::u8vec4 color = data[r * size.x + c];
            if (color != C_DISABLED_COLOR) {
                assert(color_sprite_map.find(color) == color_sprite_map.end() && "Duplicate color for different sprite!");
                color_sprite_map[color] = all_sprite_piece[index++];
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
    index = 0;
    load_png(level_map_file, &size, &data, OriginLocation::LowerLeftOrigin);
    std::cout<< "load png success, size is " << size.x << ", "<< size.y << std::endl;
    for (int r = 0; r < size.y; r+=8) {
        for (int c = 0; c < size.x; c+=8) {
            glm::u8vec4 color = data[r * size.x + c];
            assert(color_sprite_map.find(color) != color_sprite_map.end() && "Unknown tile when building background");
            SpritePiece cur_sprite_piece = color_sprite_map[color];
            background.push_back(cur_sprite_piece.palette_index << 8 | cur_sprite_piece.tile_index);
            index++;
        }
    }

    // Build group color coding
    std::cout<< "Build sprite group color coding " << std::endl;
    std::unordered_map< glm::u8vec4, std::vector< uint16_t > > group_map;
    std::unordered_map< glm::u8vec4, std::vector< std::vector< int > > > group_pos_map;
    std::unordered_map< glm::u8vec4, std::vector< std::vector< int > > > group_pos_min_max_map;
    std::unordered_set< glm::u8vec4 > seen;
    std::vector< glm::u8vec4 > ordered_color; // for name mapping
    index = 0;
    load_png(sprite_group_color_coding_file, &size, &data, OriginLocation::LowerLeftOrigin);
    std::cout<< "load png success, size is " << size.x << ", "<< size.y << std::endl;
    for (int r = 0; r < size.y; r+=8) {
        for (int c = 0; c < size.x; c+=8) {
            glm::u8vec4 color = data[r * size.x + c];
            if (color != C_DISABLED_COLOR) { 
                if (!seen.count(color)) {
                    seen.insert(color);
                    ordered_color.push_back(color);

                    group_pos_min_max_map[color].push_back({c, c});
                    group_pos_min_max_map[color].push_back({r, r});
                }
                else {
                    group_pos_min_max_map[color][0][0] = std::min(c, group_pos_min_max_map[color][0][0]);
                    group_pos_min_max_map[color][0][1] = std::max(c, group_pos_min_max_map[color][0][1]);

                    group_pos_min_max_map[color][1][0] = std::min(r, group_pos_min_max_map[color][1][0]);
                    group_pos_min_max_map[color][1][1] = std::max(r, group_pos_min_max_map[color][1][1]);
                }
                group_map[color].push_back(index);
                group_pos_map[color].push_back({c, r});
                index++;
            }
        }
    }

    // Build SpriteInfos
    std::vector< SpriteInfo > all_spriteinfo;
    for (auto& color: ordered_color) { // For name mapping
        SpriteInfo sprite_info;
        sprite_info.name = names[all_spriteinfo.size()];
        for (auto& sprite_piece_index: group_map[color]) {
            SpritePiece piece = all_sprite_piece[sprite_piece_index];
            sprite_info.tile_indexes.push_back(piece.tile_index);
            sprite_info.palette_indexes.push_back(piece.palette_index);
        }

        sprite_info.size = {1 + group_pos_min_max_map[color][0][1] - group_pos_min_max_map[color][0][0], 
                            1 + group_pos_min_max_map[color][1][1] - group_pos_min_max_map[color][1][0]};
        std::vector< int > pivot = {group_pos_map[color][0][0] + sprite_info.size[0] / 2, 
                                                group_pos_map[color][0][1]};
        for (auto& pos: group_pos_map[color]) {
            sprite_info.offsets.push_back({pos[0] - pivot[0], pos[1] - pivot[1]});
        }

        all_spriteinfo.push_back(sprite_info);
    }

    // Save game asset
    // Prepare data
    // flattern palettes
    std::vector< glm::u8vec4 > flat_palette;
    index = 0;
    for (auto& palette_set: all_palette) {
        // std::cout << "palette index: " << index++ << std::endl;
        for (auto& color: palette_set) {
            // std::cout << "color: " << glm::to_string(color) << std::endl;
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
    std::vector< uint16_t > all_tile_index;
    std::vector< uint16_t > all_palette_index;
    std::vector< int16_t > all_offset_x;
    std::vector< int16_t > all_offset_y;

    for (auto& info: all_spriteinfo) {
        SpriteRef ref;
        // Tile index
        ref.tile_index_start = all_tile_index.size();
        for (auto& index: info.tile_indexes) {
            all_tile_index.push_back(index);
        }
        ref.tile_index_end = all_tile_index.size() - 1;

        // Palette index
        ref.palette_index_start = all_palette_index.size();
        for (auto& index: info.palette_indexes) {
            all_palette_index.push_back(index);
        }
        ref.palette_index_end = all_palette_index.size() - 1;

        // name
        ref.name_index_start = all_name.size();
        ref.name_size = info.name.size();
        for (char& c: info.name) {
            all_name.push_back(c);
        }

        // sprite size
        ref.size_x = info.size[0];
        ref.size_y = info.size[1];

        // tile offsets
        ref.offset_index_start = all_offset_x.size();
        for (auto& offset: info.offsets) {
            all_offset_x.push_back(offset[0]);
            all_offset_y.push_back(offset[1]);
        }
        ref.offset_index_end = all_offset_x.size() - 1;

        sprite_refs.push_back(ref);
    }

    std::ofstream output_file(data_path("game.asset"));
    write_chunk("aaaa", flat_palette, &output_file);
    write_chunk("bbbb", flat_tile_bit0, &output_file);
    write_chunk("cccc", flat_tile_bit1, &output_file);
    write_chunk("dddd", all_name, &output_file);
    write_chunk("eeee", sprite_refs, &output_file);
    write_chunk("ffff", background, &output_file);
    write_chunk("eeea", all_tile_index, &output_file);
    write_chunk("eeeb", all_palette_index, &output_file);
    write_chunk("eeec", all_offset_x, &output_file);
    write_chunk("eeed", all_offset_y, &output_file);
    output_file.close();
    std::cout<< "--Success-- Build game asset finished." << std::endl;
}

// void AssetSerializer::build_offset(uint16_t x, uint16_t y, uint16_t offset_x, uint16_t offset_y, std::vector< std::vector< uint16_t > > &offsets, std::vector< glm::u8vec4 >& data, glm::u8vec4 C_DISABLED_COLOR, ) {

// }
