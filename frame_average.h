#pragma once

constexpr int FREQ_RANGE{ 6 };//

auto calc_frame_average(const _replay& replay, const void* beatmap){

	const auto is_invalid_frame = [&replay](const size_t i)->bool{

		#define KEY_UPDATE(x)(\
					replay.frame[i x].keys != replay.frame[i x].p_keys ||\
					replay.frame[i x].smoke_held != replay.frame[i x].p_smoke_held\
				)

		// The smoke check is a bit overkill, could be occluded.

		if (KEY_UPDATE(-1u) || KEY_UPDATE() || KEY_UPDATE(+1u))
			return 1;

		#undef KEY_UPDATE

		return 0;
	};

	struct _frame_average {

		u32 counted_frames;
		float model_average;// The 'ideal' average
		float average;

		void print() const noexcept{
			printf("%fms - %f | frame pairs: %i",
				average, average / model_average, counted_frames
			);
		}

	} ret{};

	ret.model_average = REPLAY_FRAME_TARGET * replay.get_play_factor();

	if (replay.frame.size() < 50)
		return ret;

	struct _frame_check {
		u32 frame_id;
		int delay;
	};

	std::vector<_frame_check> frame_check; frame_check.reserve(replay.frame.size());

	for (size_t i{ 2 }, frame_count{ replay.frame.size() - 2 }; i < frame_count; ++i) {

		if (is_invalid_frame(i))// Ignore any frames surrounding a key update
			continue;

		// In the original implementation slider tick frames were ignored 
		// It does not affect the results much but it could be done here.

		constexpr float edge_buffer{ 50.f };

		// Removes all frames out of the playable area in an attempt to fix certain alt tab frames.
		if (const auto pos{ replay.frame[i].pos };
				pos.x < -edge_buffer || pos.x > PLAYFIELD_WIDTH + edge_buffer ||
				pos.y < -edge_buffer || pos.y > PLAYFIELD_HEIGHT + edge_buffer
			)
			continue;

		const int delay{ replay.frame[i].time - replay.frame[i - 1u].time };

		if (delay > 0)
			frame_check.push_back(_frame_check{ static_cast<u32>(i), delay });
	}

	if (frame_check.size() <= 2)
		return ret;

	std::vector<u16> delay_buffer; delay_buffer.reserve(frame_check.size() >> 1);

	u16 frame_freq[128]{};// Could be a map, but this is probably good enough

	for (size_t i{ 1 }, frame_count{ frame_check.size() - 2 }; i < frame_count; ++i) {
		const _frame_check last_frame{ frame_check[i - 1u] },
						   this_frame{ frame_check[i] };

		// Ignore all frames that are not pairs.
		if (last_frame.frame_id + 1 != this_frame.frame_id)
			continue;

		const u16 delay{ static_cast<u16>(last_frame.delay + this_frame.delay) };

		++frame_freq[delay & 0x7fu];
		delay_buffer.push_back(delay);

		++i;
		// I have not done extensive testing on this; but I assume the rolling average of triplets
		// would produce less desirable results.		
	}
	
	const u16 most_average_ms{
		static_cast<u16>(std::max_element(frame_freq,frame_freq + 128) - frame_freq)
	};

	{
		double final_sum{};

		for (const u16 d : delay_buffer) {

			// Remove the high and low freq data

			if (
				d < most_average_ms - FREQ_RANGE ||
				d > most_average_ms + FREQ_RANGE
				) [[unlikely]]
				continue;

			final_sum += double(d) * 0.5f;
			++ret.counted_frames;

		}

		ret.average = ret.counted_frames ? final_sum / double(ret.counted_frames) : 0.f;

	}

	return ret;
}


