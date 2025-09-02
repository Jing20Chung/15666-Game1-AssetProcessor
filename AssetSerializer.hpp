#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "data_path.hpp"
#include "DataTypes.hpp"

struct AssetSerializer {
    static void compile_asset(const std::string& name_mapping_file, const std::string& sprite_sheet_file, const std::string& level_map_file, const std::string& color_coding_file);
};
