#include <iostream>
#include "AssetSerializer.hpp"
#include "AssetDeserializer.hpp"

int main(int argc, char ** argv) {
    AssetSerializer::compile_asset(data_path("name_mapping.txt"),data_path("sprite_sheet.png"),data_path("level_map.png"),data_path("color_coding.png"));
    AssetDeserializer::load(data_path("game.asset"));
    return 0;
}