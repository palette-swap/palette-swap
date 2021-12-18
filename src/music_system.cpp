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
	curr_state = MusicState::Count;

	// Start the title music
	set_state(MusicSystem::MusicState::Title);
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
		if (other_color_music != SoLoud::SO_NO_ERROR) {
			// Already ready to go, swap them
			handle = other_color_music;
			other_color_music = curr_music;
		} else {
			// Load both color songs
			handle = so_loud->play(music_files.at(state_value), 0.f);
			other_color_music = so_loud->play(music_files.at((state_value + 1) % 2), 0.f);
		}
	} else {
		handle = so_loud->play(music_files.at(state_value), 0.f);
	}

	so_loud->fadeVolume(handle, -1, fade_time);
	so_loud->fadeVolume(curr_music, 0, fade_time);

	if (!color_switch) {
		so_loud->stop(other_color_music);
		other_color_music = SoLoud::SO_NO_ERROR;

			so_loud->scheduleStop(curr_music, fade_time);
	}

	curr_music = handle;
	curr_state = state;
}

void MusicSystem::set_world(MusicState state)
{
	// Only colors allowed here
	set_state(state);
}

bool MusicSystem::is_curr_situational() const { return curr_situational_music != SoLoud::SO_NO_ERROR; }
