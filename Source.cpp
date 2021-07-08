#include <array>
#include <string_view>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iostream>

#include <charconv>
#define from_char_sv(sv, v) {std::from_chars(sv.data(), sv.data() + sv.size(), v);}

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

constexpr float REPLAY_FRAME_TARGET{ 1000.f / 60.f };

constexpr float PLAYFIELD_WIDTH{ 512.f },
				PLAYFIELD_HEIGHT{ 384.f };

std::vector<u8> read_file(const char* file_name) {

	std::ifstream file(file_name, std::ios::binary | std::ios::ate | std::ios::in);

	if (file.is_open() == 0) [[unlikely]]
		return {};

	const size_t file_size{ (size_t)file.tellg() };

	std::vector<u8> ret(file_size);

	if (file_size) [[likely]] {
		file.seekg(0, std::ios::beg);
		file.read((char*)ret.data(), file_size);
	}
	file.close();

	return ret;
}

struct vec2 {

	float x, y;

	float& operator[](const bool v) {
		return v ? y : x;
	}

};

#include "explode.h"

#include "replay.h"
#include "frame_average.h"

#include <filesystem>
#include <string>

void run_test_folder() {

	for (const auto& file_entry : std::filesystem::directory_iterator("test/")) {

		const auto _p{ file_entry.path().native() };

		const auto file_name{ std::string(_p.begin(), _p.end()) };

		if (file_name.find(".osr") == std::string::npos)
			continue;

		const auto replay_data{ load_replay_from_file(file_name.c_str()) };

		(calc_frame_average(replay_data, 0).print());
		printf(" > %s\n", file_name.c_str());

	}

}

int main(const int c, char** d) {

	if (c != 2) {
		run_test_folder();
		return 0;
	}

	const auto replay_data{ load_replay_from_file(d[1])};
	
	const auto frame_time_data{ calc_frame_average(replay_data, 0) };
	
	frame_time_data.print();


	return 0;
}