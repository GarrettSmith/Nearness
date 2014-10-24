#ifndef CONVERT_FEATURES
#define CONVERT_FEATURES

#include <string>
#include <fstream>

#include "maximal_clique_basic_includes.hpp"

void convert_matrices(const std::string &in, const std::string &out) {
    std::ifstream in_file(in);
    // TODO check if in file exists
    std::ofstream out_file(out, std::ofstream::trunc);

    std::string line;
    int line_num = 0;

    while (std::getline(in_file, line)) {
        std::istringstream line_stream(line);
        std::stringstream output_stream;

        out_file << line_num << ' ';
        ++line_num;

        bool c;
        int c_num = 0;
        int matches = 0;
        while (line_stream >> c) {
            if (c) {
                ++matches;
                output_stream << ' ' << c_num;
            }
            ++c_num;
        }

        // construct line
        out_file << matches << output_stream.str() << std::endl;
    }

    in_file.close();
    out_file.close();
}

#endif