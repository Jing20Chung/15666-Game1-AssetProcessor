#pragma once
#include <array>
#include <string>

struct Tile {
    std::array< uint8_t, 8 > bit0; //<-- controls bit 0 of the color index
    std::array< uint8_t, 8 > bit1; //<-- controls bit 1 of the color index
};
static_assert(sizeof(Tile) == 16, "Tile is packed");

struct SpriteRef {
    uint16_t tile_index_start;
    uint16_t tile_index_end;
    uint16_t palette_index_start;
    uint16_t palette_index_end;
    uint16_t name_index_start;
    uint16_t name_size;
    uint16_t size_x;
    uint16_t size_y;
    int16_t offset_index_start;
    int16_t offset_index_end;
};
static_assert(sizeof(SpriteRef) == 20, "SpriteRef is packed");

struct SpritePiece {
    uint16_t tile_index;
    uint8_t palette_index;
};

struct SpriteInfo {
    std::vector< int > size;
    std::vector< std::vector< int > > offsets;
    std::vector< uint16_t > tile_indexes;
    std::vector< uint8_t > palette_indexes;
    std::string name;
};

// From PPU
//Background Layer:
// The PPU's background layer is made of 64x60 tiles (512 x 480 pixels).
// This is twice the size of the screen, to support scrolling.
enum : uint32_t {
    BackgroundWidth = 64,
    BackgroundHeight = 60
};