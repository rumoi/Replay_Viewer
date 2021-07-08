#pragma once

struct replay_frame {

	int time;
	vec2 pos;

	u32 keys : 4, p_keys : 4, key_frame : 1, smoke_held : 1, p_smoke_held:1, _padding0 : 21;

};

#define rframe_count_estimate(char_count) (char_count >> 3ul)

void parse_replay(const std::string_view replay_text, std::vector<replay_frame>& output) {

	const auto& frames{ explode_view(replay_text, ',', rframe_count_estimate(replay_text.size())) };

	output.clear();
	output.reserve(frames.size());

	int current_time{};

	for (const std::string_view line : frames) {

		const auto [_delta, _x, _y, _keys] { explode_view_fixed<4, '|'>(line) };

		replay_frame rf{};

		{
			int delta;
			from_char_sv(_delta, delta);

			rf.time = (current_time += delta);
		}

		from_char_sv(_x, rf.pos.x);
		from_char_sv(_y, rf.pos.y);

		{
			u32 keys;
			from_char_sv(_keys, keys);

			rf.keys = (keys & 0b1111ul);
			rf.smoke_held = ((keys & 0b10000ul) != 0ul);

		}

		if (output.size()) [[likely]] {

			const auto & b{output.back()};
			rf.p_keys = b.keys;
			rf.p_smoke_held = b.smoke_held;			
			//rf.key_frame = (rf.keys & 0b11ul & (rf.keys ^ rf.p_keys)) != 0ul; unused

		}

		output.push_back(rf);
	}

}

_inline size_t get_uleb_size(const u8*& p, const u8*__restrict const end) {

	if (p + 2 > end || *p == 0u)
		return 0;

	u32 s{ *++p & 0x7fu }, l_s{};

	while ((*p & 0x80u) && p + 1 < end) [[unlikely]]
		s |= (*++p & 0x7fu) << (l_s += 7);

	return ++p + s <= end ? s : 0;
}

struct _replay_header {

	u32 game_mode : 2, game_version: 25, is_perfect : 1, _padding0 : 4;
	u32 total_score, mods;
	u16 hit_300, hit_miss, highest_combo;

	union { u16 hit_100, hit_150/*taiko*/; };
	union { u16 hit_50, hit_tick/*ctb*/; };
	union { u16 hit_geki, hit_max/*mania*/; };
	union { u16 hit_katu, hit_200/*mania*/; };

};

#include "LzmaDec.h"

void* __lzma_alloc(ISzAllocPtr, size_t size) { return new u8[size];}
void __lzma_free(ISzAllocPtr, void* p) { if (p) delete[] (u8*)p;}

std::vector<u8> lzma_decompress(const uint8_t* input, uint32_t input_size) {

	constexpr auto p_size{ LZMA_PROPS_SIZE + 8 };

	if (input_size < p_size)
		return {};

	size_t size{};

	for (int i = 0; i < 8; ++i)
		size |= (input[LZMA_PROPS_SIZE + i] << (i * 8));

	if (size <= (u32(-1) >> 1)) {

		std::vector<u8> ret(size);

		ELzmaStatus lzma_status{};
		size_t decomp_size{ size }, comp_size{ input_size - p_size };

		constexpr ISzAlloc _lzma_alloc{ __lzma_alloc, __lzma_free };

		if (
			LzmaDecode(ret.data(), &decomp_size, input + p_size, &comp_size, input,
				LZMA_PROPS_SIZE, LZMA_FINISH_END, &lzma_status, &_lzma_alloc) == SZ_OK && decomp_size == size)

			return ret;
	}

	return {};
}

struct _replay {
	_replay_header header;
	std::vector<replay_frame> frame;

	float get_play_factor()const noexcept{
		return
			(header.mods & 64u) > 0 ? 1.5f
				: ((header.mods & 256u) > 0 ? 0.75f : 1.f);
	}

};

_replay load_replay_from_file(const char* file_name) {

	#define padv(type) (ptr += sizeof(type), ptr <= end ? *(type*)(ptr - sizeof(type)) : type{})

	const auto& raw_data{ read_file(file_name) };

	const u8* ptr{ (const u8*)raw_data.data() }, *const end{ ptr + raw_data.size() };

	_replay ret{};
	auto& rh{ret.header};

	rh.game_mode = padv(u8);
	rh.game_version = padv(u32);

	{//beatmap_hash
		const auto str_size{ get_uleb_size(ptr, end) };

		ptr += str_size;
	}

	{//player_name
		const auto str_size{ get_uleb_size(ptr, end) };

		ptr += str_size;
	}

	{//replay_hash
		const auto str_size{ get_uleb_size(ptr, end) };

		ptr += str_size;
	}

	rh.hit_300 = padv(u16);
	rh.hit_100 = padv(u16);
	rh.hit_50 = padv(u16);
	rh.hit_geki = padv(u16);
	rh.hit_katu = padv(u16);
	rh.hit_miss = padv(u16);
	rh.total_score = padv(u32);
	rh.highest_combo = padv(u16);
	rh.is_perfect = (padv(u8) != 0);
	rh.mods = padv(u32);

	{//life_bar
		const auto str_size{ get_uleb_size(ptr, end) };

		ptr += str_size;
	}

	const auto time_stamp{ padv(u64) };

	{
		const auto& raw_replay_string{ lzma_decompress(ptr, padv(u32)) };

		parse_replay(std::string_view((const char*)raw_replay_string.data(), raw_replay_string.size()), ret.frame);
	}

	#undef padv

	return ret;
}
