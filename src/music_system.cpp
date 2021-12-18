#include "music_system.hpp"

MusicSystem::MusicSystem(std::shared_ptr<SoLoud::Soloud> so_loud)
	: so_loud(std::move(so_loud))
{
	for (size_t state = 0; state < (size_t)MusicState::Count; state++) {
		music_files.at(state).load(music_path(music_file_paths.at(state)).c_str());
		music_files.at(state).setLooping(music_file_loops.at(state));
	}
}

void MusicSystem::restart_game()
{
	// Disable situations, color cache, transitions
	so_loud->stopAll();
	curr_music = SoLoud::SO_NO_ERROR;
	other_color_music = SoLoud::SO_NO_ERROR;
	curr_situational_music = SoLoud::SO_NO_ERROR;
	curr_state = MusicState::Count;
	curr_situational_state = MusicState::Count;
	transition_time_ms = 0;
	after_transition_state = MusicState::Count;

	// Start the title music
	set_state(MusicSystem::MusicState::Title);
}

void MusicSystem::step(double elapsed_ms_since_last_update)
{
	if (after_transition_state != MusicState::Count) {
		transition_time_ms -= elapsed_ms_since_last_update;
		if (transition_time_ms <= 0) {
			// Disable current situation
			curr_situational_music = SoLoud::SO_NO_ERROR;
			curr_situational_state = MusicState::Count;

			// Start the next track
			SoLoud::handle prev = curr_music;
			set_state(after_transition_state);

			// Fade twice as slow
			so_loud->fadeVolume(curr_music, -1, transition_fade_time);
			so_loud->fadeVolume(prev, 0, transition_fade_time);

			// Reset tracking
			after_transition_state = MusicState::Count;
			transition_time_ms = 0;
		}
	}
}

void MusicSystem::transition_to_state(MusicState transition, MusicState state) {
	auto transition_value = (size_t)transition;
	set_state(transition, true);
	SoLoud::time length = music_files.at(transition_value).getLength() - transition_fade_time;
	after_transition_state = state;
	transition_time_ms = length * 1000.0;
}

void MusicSystem::set_state(MusicState state, bool situational)
{
	if (curr_state == state) {
		return;
	}

	auto state_value = (size_t)state;

	SoLoud::handle handle = SoLoud::SO_NO_ERROR;
	// Red-Blue Switch
	bool color_switch = state_value < 2;
	if (color_switch) {
		so_loud->stop(curr_situational_music);
		curr_situational_state = MusicState::Count;
		curr_situational_music = SoLoud::SO_NO_ERROR;

		if (other_color_music != SoLoud::SO_NO_ERROR) {
			// Already ready to go, swap them
			handle = other_color_music;
			other_color_music = curr_music;
		} else {
			// Load both color songs
			handle = so_loud->play(music_files.at(state_value), 0.f);
			other_color_music = so_loud->play(music_files.at((state_value + 1) % 2), 0.f);
		}
	} else if (situational) {
		if (curr_situational_music != SoLoud::SO_NO_ERROR && curr_situational_state == state) {
			handle = curr_situational_music;
			so_loud->setPause(handle, false);
		} else {
			curr_situational_state = state;
			handle = so_loud->play(music_files.at(state_value), 0.f);
		}
		curr_situational_music = handle;
	} else {
		handle = so_loud->play(music_files.at(state_value), 0.f);
	}

	so_loud->fadeVolume(handle, -1, fade_time);
	so_loud->fadeVolume(curr_music, 0, fade_time);

	if (!color_switch) {
		so_loud->stop(other_color_music);
		other_color_music = SoLoud::SO_NO_ERROR;

		if (curr_situational_music != SoLoud::SO_NO_ERROR && !situational) {
			so_loud->schedulePause(curr_music, fade_time);
		} else {
			so_loud->scheduleStop(curr_music, fade_time);
		}
	}

	curr_music = handle;
	curr_state = state;
}

void MusicSystem::set_world(MusicState state)
{
	// Only colors allowed here
	if ((size_t)state >= 2 || curr_state == curr_situational_state) {
		return;
	}
	if (is_curr_situational()) {
		so_loud->fadeVolume(curr_music, 0, fade_time);
		so_loud->fadeVolume(curr_situational_music, -1, fade_time);
		so_loud->setPause(curr_situational_music, false);
		so_loud->scheduleStop(curr_music, fade_time);
		curr_music = curr_situational_music;
		return;
	}
	set_state(state);
}

bool MusicSystem::is_curr_situational() const { return curr_situational_music != SoLoud::SO_NO_ERROR; }
