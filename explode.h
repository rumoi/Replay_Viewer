#pragma once

auto explode_view(const std::string_view Input, const char Delim, const size_t Expected_Size = 16) noexcept {

	std::vector<std::string_view> ret;
	ret.reserve(Input.size() ? Expected_Size : 0);

	auto* Chunk{ Input.data() },
		*const Start{ Chunk },
		*const End{ Chunk + Input.size() };

	for (const char* Current{ Chunk }; ; ++Current) {

		const bool at_end{ (Current == End) };

		if (at_end || *Current == Delim) {

			ret.emplace_back(Chunk, Current - Chunk);
			Chunk = Current + 1;

			if (at_end)
				break;
		}

	}

	return ret;
}

template<size_t size, char Delim>
constexpr std::array<std::string_view, size> explode_view_fixed(const std::string_view Input) noexcept {

	static_assert(size > 0, "explode_view_fixed size can not be zero");

	std::array<std::string_view, size> ret{};

	auto* Chunk{ Input.data() },
		*const Start{ Chunk },
		*const End{ Chunk + Input.size() };

	size_t c{};
	for (const char* Current{ Chunk }; ; ++Current) {

		const bool at_end = (Current == End);

		if (at_end || *Current == Delim) {

			ret[c++] = std::string_view{ Chunk, size_t(Current - Chunk) };

			if (at_end || c == size)
				break;
			Chunk = Current + 1;
		}
	}

	return ret;
}