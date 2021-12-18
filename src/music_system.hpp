#pragma once

#include "components.hpp"

#include "soloud_wav.h"

class MusicSystem {
public:
	explicit MusicSystem(std::shared_ptr<SoLoud::Soloud> so_loud);

	void restart_game();

	void step(double elapsed_ms_since_last_update);

	enum class MusicState {
		BlueWorld = 0,
		RedWorld = BlueWorld + 1,
		BossBattle = RedWorld + 1,
		Title = BossBattle + 1,
		YouWon = Title + 1,
		YouDied = YouWon + 1,
		SmallVictory = YouDied + 1,
		Count = SmallVictory + 1,
	};

	void transition_to_state(MusicState transition, MusicState state);

	void set_state(MusicState state, bool situational = false);

	void set_world(MusicState state);

	bool is_curr_situational() const;

private:

	MusicState curr_state = MusicState::Count;

	std::array<SoLoud::Wav, (size_t)MusicState::Count> music_files;

	// Note that 0, the default value, is never a potential channel name, so we can use it to represent no handle
	SoLoud::handle curr_music = SoLoud::SO_NO_ERROR;
	SoLoud::handle other_color_music = SoLoud::SO_NO_ERROR;
	SoLoud::handle curr_situational_music = SoLoud::SO_NO_ERROR;

	MusicState curr_situational_state = MusicState::Count;
	MusicState after_transition_state = MusicState::Count;
	double transition_time_ms = 0;

	std::shared_ptr<SoLoud::Soloud> so_loud;

	static constexpr SoLoud::time fade_time = .35;
	static constexpr SoLoud::time transition_fade_time = .8;

	static constexpr std::array<std::string_view, (size_t)MusicState::Count> music_file_paths = {
		"famous_flower_of_serving_men.wav",
		"henry_martin.wav",
		"a_begging_i_will_go.wav",
		"title.wav",
		"you_won.wav",
		"you_died.wav",
		"small_victory.wav",
	};

	static constexpr std::array<bool, (size_t)MusicState::Count> music_file_loops = {
		true,
		true,
		true,
		true,
		true,
		true,
		false,
	};
};