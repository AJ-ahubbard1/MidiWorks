#pragma once
#include <string>

namespace MidiInterface
{
	/** Soundset chart from https://dtm.noyu.me/wiki/General_MIDI#Sound_Set
	 *	These values correspond with the Program Change Midi Event
	 *  Example Implementation:
	 *	 MidiOut player;
	 *   MidiMessage slapaDaBass(EventType::PROGRAM_CHANGE, 0, SoundSet::SLAP_BASS_1);
	 *   player.sendMessage(slapaDaBass); */
	enum class SoundSet
	{
		ACOUSTIC_GRAND_PIANO,
		BRIGHT_ACOUSTIC_PIANO,
		ELECTRIC_GRAND_PIANO,
		HONKY_TONK_PIANO,
		ELECTRIC_PIANO_1,
		ELECTRIC_PIANO_2,
		HARPSICHORD,
		CLAVI,
		CELESTA,
		GLOCKENSPIEL,
		MUSIC_BOX,
		VIBRAPHONE,
		MARIMBA,
		XYLOPHONE,
		TUBULAR_BELLS,
		DULCIMER,
		DRAWBAR_ORGAN,
		PERCUSSIVE_ORGAN,
		ROCK_ORGAN,
		CHURCH_ORGAN,
		REED_ORGAN,
		ACCORDION,
		HARMONICA,
		TANGO_ACCORDION,
		ACOUSTIC_GUITAR_NYLON,
		ACOUSTIC_GUITAR_STEEL,
		ELECTRIC_GUITAR_JAZZ,
		ELECTRIC_GUITAR_CLEAN,
		ELECTRIC_GUITAR_MUTED,
		OVERDRIVEN_GUITAR,
		DISTORTION_GUITAR,
		GUITAR_HARMONICS,
		ACOUSTIC_BASS,
		ELECTRIC_BASS_FINGER,
		ELECTRIC_BASS_PICK,
		FRETLESS_BASS,
		SLAP_BASS_1,
		SLAP_BASS_2,
		SYNTH_BASS_1,
		SYNTH_BASS_2,
		VIOLIN,
		VIOLA,
		CELLO,
		CONTRABASS,
		TREMOLO_STRINGS,
		PIZZICATO_STRINGS,
		ORCHESTRAL_HARP,
		TIMPANI,
		STRING_ENSEMBLE_1,
		STRING_ENSEMBLE_2,
		SYNTH_STRINGS_1,
		SYNTH_STRINGS_2,
		CHOIR_AAHS,
		VOICE_OOHS,
		SYNTH_VOICE,
		ORCHESTRA_HIT,
		TRUMPET,
		TROMBONE,
		TUBA,
		MUTED_TRUMPET,
		FRENCH_HORN,
		BRASS_SECTION,
		SYNTH_BRASS_1,
		SYNTH_BRASS_2,
		SOPRANO_SAX,
		ALTO_SAX,
		TENOR_SAX,
		BARITONE_SAX,
		OBOE,
		ENGLISH_HORN,
		BASSOON,
		CLARINET,
		PICCOLO,
		FLUTE,
		RECORDER,
		PAN_FLUTE,
		BLOWN_BOTTLE,
		SHAKUHACHI,
		WHISTLE,
		OCARINA,
		LEAD_1_SQUARE,
		LEAD_2_SAWTOOTH,
		LEAD_3_CALLIOPE,
		LEAD_4_CHIFF,
		LEAD_5_CHARANG,
		LEAD_6_VOICE,
		LEAD_7_FIFTHS,
		LEAD_8_BASS_LEAD,
		PAD_1_NEW_AGE,
		PAD_2_WARM,
		PAD_3_POLYSYNTH,
		PAD_4_CHOIR,
		PAD_5_BOWED,
		PAD_6_METALLIC,
		PAD_7_HALO,
		PAD_8_SWEEP,
		FX_1_RAIN,
		FX_2_SOUNDTRACK,
		FX_3_CRYSTAL,
		FX_4_ATMOSPHERE,
		FX_5_BRIGHTNESS,
		FX_6_GOBLINS,
		FX_7_ECHOES,
		FX_8_SCI_FI,
		SITAR,
		BANJO,
		SHAMISEN,
		KOTO,
		KALIMBA,
		BAG_PIPE,
		FIDDLE,
		SHANAI,
		TINKLE_BELL,
		AGOGO,
		STEEL_DRUMS,
		WOODBLOCK,
		TAIKO_DRUM,
		MELODIC_TOM,
		SYNTH_DRUM,
		REVERSE_CYMBAL,
		GUITAR_FRET_NOISE,
		BREATH_NOISE,
		SEASHORE,
		BIRD_TWEET,
		TELEPHONE_RING,
		HELICOPTER,
		APPLAUSE,
		GUNSHOT,
	};

	const std::string SoundNames[]{
		"ACOUSTIC_GRAND_PIANO",
		"BRIGHT_ACOUSTIC_PIANO",
		"ELECTRIC_GRAND_PIANO",
		"HONKY_TONK_PIANO",
		"ELECTRIC_PIANO_1",
		"ELECTRIC_PIANO_2",
		"HARPSICHORD",
		"CLAVI",
		"CELESTA",
		"GLOCKENSPIEL",
		"MUSIC_BOX",
		"VIBRAPHONE",
		"MARIMBA",
		"XYLOPHONE",
		"TUBULAR_BELLS",
		"DULCIMER",
		"DRAWBAR_ORGAN",
		"PERCUSSIVE_ORGAN",
		"ROCK_ORGAN",
		"CHURCH_ORGAN",
		"REED_ORGAN",
		"ACCORDION",
		"HARMONICA",
		"TANGO_ACCORDION",
		"ACOUSTIC_GUITAR_NYLON",
		"ACOUSTIC_GUITAR_STEEL",
		"ELECTRIC_GUITAR_JAZZ",
		"ELECTRIC_GUITAR_CLEAN",
		"ELECTRIC_GUITAR_MUTED",
		"OVERDRIVEN_GUITAR",
		"DISTORTION_GUITAR",
		"GUITAR_HARMONICS",
		"ACOUSTIC_BASS",
		"ELECTRIC_BASS_FINGER",
		"ELECTRIC_BASS_PICK",
		"FRETLESS_BASS",
		"SLAP_BASS_1",
		"SLAP_BASS_2",
		"SYNTH_BASS_1",
		"SYNTH_BASS_2",
		"VIOLIN",
		"VIOLA",
		"CELLO",
		"CONTRABASS",
		"TREMOLO_STRINGS",
		"PIZZICATO_STRINGS",
		"ORCHESTRAL_HARP",
		"TIMPANI",
		"STRING_ENSEMBLE_1",
		"STRING_ENSEMBLE_2",
		"SYNTH_STRINGS_1",
		"SYNTH_STRINGS_2",
		"CHOIR_AAHS",
		"VOICE_OOHS",
		"SYNTH_VOICE",
		"ORCHESTRA_HIT",
		"TRUMPET",
		"TROMBONE",
		"TUBA",
		"MUTED_TRUMPET",
		"FRENCH_HORN",
		"BRASS_SECTION",
		"SYNTH_BRASS_1",
		"SYNTH_BRASS_2",
		"SOPRANO_SAX",
		"ALTO_SAX",
		"TENOR_SAX",
		"BARITONE_SAX",
		"OBOE",
		"ENGLISH_HORN",
		"BASSOON",
		"CLARINET",
		"PICCOLO",
		"FLUTE",
		"RECORDER",
		"PAN_FLUTE",
		"BLOWN_BOTTLE",
		"SHAKUHACHI",
		"WHISTLE",
		"OCARINA",
		"LEAD_1_SQUARE",
		"LEAD_2_SAWTOOTH",
		"LEAD_3_CALLIOPE",
		"LEAD_4_CHIFF",
		"LEAD_5_CHARANG",
		"LEAD_6_VOICE",
		"LEAD_7_FIFTHS",
		"LEAD_8_BASS_LEAD",
		"PAD_1_NEW_AGE",
		"PAD_2_WARM",
		"PAD_3_POLYSYNTH",
		"PAD_4_CHOIR",
		"PAD_5_BOWED",
		"PAD_6_METALLIC",
		"PAD_7_HALO",
		"PAD_8_SWEEP",
		"FX_1_RAIN",
		"FX_2_SOUNDTRACK",
		"FX_3_CRYSTAL",
		"FX_4_ATMOSPHERE",
		"FX_5_BRIGHTNESS",
		"FX_6_GOBLINS",
		"FX_7_ECHOES",
		"FX_8_SCI_FI",
		"SITAR",
		"BANJO",
		"SHAMISEN",
		"KOTO",
		"KALIMBA",
		"BAG_PIPE",
		"FIDDLE",
		"SHANAI",
		"TINKLE_BELL",
		"AGOGO",
		"STEEL_DRUMS",
		"WOODBLOCK",
		"TAIKO_DRUM",
		"MELODIC_TOM",
		"SYNTH_DRUM",
		"REVERSE_CYMBAL",
		"GUITAR_FRET_NOISE",
		"BREATH_NOISE",
		"SEASHORE",
		"BIRD_TWEET",
		"TELEPHONE_RING",
		"HELICOPTER",
		"APPLAUSE",
		"GUNSHOT"
	};


	/** GM2 Drum Kit Names (for Channel 10)
	 *  Program changes on channel 10 select different drum kits in GM2-compatible devices
	 *  Standard GM1 devices typically only support kit 0 (Standard Kit)
	 */
	const std::string DrumKitNames[]{
		"Standard Kit",        // 0
		"Standard Kit 2",      // 1
		"Standard Kit 3",      // 2
		"Standard Kit 4",      // 3
		"Standard Kit 5",      // 4
		"Standard Kit 6",      // 5
		"Standard Kit 7",      // 6
		"Standard Kit 8",      // 7
		"Room Kit",            // 8
		"Power Kit",           // 16 (but we'll fill gaps)
		"Power Kit 2",         // 9
		"Power Kit 3",         // 10
		"Power Kit 4",         // 11
		"Power Kit 5",         // 12
		"Power Kit 6",         // 13
		"Power Kit 7",         // 14
		"Power Kit 8",         // 15
		"Electronic Kit",      // 16
		"TR-808 Kit",          // 17
		"TR-808 Kit 2",        // 18
		"TR-808 Kit 3",        // 19
		"TR-808 Kit 4",        // 20
		"TR-808 Kit 5",        // 21
		"TR-808 Kit 6",        // 22
		"TR-808 Kit 7",        // 23
		"Jazz Kit",            // 24
		"Jazz Kit 2",          // 25
		"Jazz Kit 3",          // 26
		"Jazz Kit 4",          // 27
		"Jazz Kit 5",          // 28
		"Jazz Kit 6",          // 29
		"Jazz Kit 7",          // 30
		"Jazz Kit 8",          // 31
		"Brush Kit",           // 32
		"Brush Kit 2",         // 33
		"Brush Kit 3",         // 34
		"Brush Kit 4",         // 35
		"Brush Kit 5",         // 36
		"Brush Kit 6",         // 37
		"Brush Kit 7",         // 38
		"Brush Kit 8",         // 39
		"Orchestra Kit",       // 40
		"Orchestra Kit 2",     // 41
		"Orchestra Kit 3",     // 42
		"Orchestra Kit 4",     // 43
		"Orchestra Kit 5",     // 44
		"Orchestra Kit 6",     // 45
		"Orchestra Kit 7",     // 46
		"Orchestra Kit 8",     // 47
		"SFX Kit",             // 48
		"SFX Kit 2",           // 49
		"SFX Kit 3",           // 50
		"SFX Kit 4",           // 51
		"SFX Kit 5",           // 52
		"SFX Kit 6",           // 53
		"SFX Kit 7",           // 54
		"SFX Kit 8",           // 55
		// Fill remaining slots with generic names
		"Drum Kit 56", "Drum Kit 57", "Drum Kit 58", "Drum Kit 59",
		"Drum Kit 60", "Drum Kit 61", "Drum Kit 62", "Drum Kit 63",
		"Drum Kit 64", "Drum Kit 65", "Drum Kit 66", "Drum Kit 67",
		"Drum Kit 68", "Drum Kit 69", "Drum Kit 70", "Drum Kit 71",
		"Drum Kit 72", "Drum Kit 73", "Drum Kit 74", "Drum Kit 75",
		"Drum Kit 76", "Drum Kit 77", "Drum Kit 78", "Drum Kit 79",
		"Drum Kit 80", "Drum Kit 81", "Drum Kit 82", "Drum Kit 83",
		"Drum Kit 84", "Drum Kit 85", "Drum Kit 86", "Drum Kit 87",
		"Drum Kit 88", "Drum Kit 89", "Drum Kit 90", "Drum Kit 91",
		"Drum Kit 92", "Drum Kit 93", "Drum Kit 94", "Drum Kit 95",
		"Drum Kit 96", "Drum Kit 97", "Drum Kit 98", "Drum Kit 99",
		"Drum Kit 100", "Drum Kit 101", "Drum Kit 102", "Drum Kit 103",
		"Drum Kit 104", "Drum Kit 105", "Drum Kit 106", "Drum Kit 107",
		"Drum Kit 108", "Drum Kit 109", "Drum Kit 110", "Drum Kit 111",
		"Drum Kit 112", "Drum Kit 113", "Drum Kit 114", "Drum Kit 115",
		"Drum Kit 116", "Drum Kit 117", "Drum Kit 118", "Drum Kit 119",
		"Drum Kit 120", "Drum Kit 121", "Drum Kit 122", "Drum Kit 123",
		"Drum Kit 124", "Drum Kit 125", "Drum Kit 126", "Drum Kit 127"
	};

	/** PercussionMap from https://dtm.noyu.me/wiki/General_MIDI#Percussion_Map
	 *  Sounds found on 10th channel (ubyte 9)
	 *	These values correspond to the Midi note number
	 *  ACOUSTIC_BASS_DRUM = 35, Note B1
	*/
	enum class PercussionMap
	{
		ACOUSTIC_BASS_DRUM = 35,
		BASS_DRUM_1,
		SIDE_STICK,
		ACOUSTIC_SNARE,
		HAND_CLAP,
		ELECTRIC_SNARE,
		LOW_FLOOR_TOM,
		CLOSED_HI_HAT,
		HIGH_FLOOR_TOM,
		PEDAL_HI_HAT,
		LOW_TOM,
		OPEN_HI_HAT,
		LOW_MID_TOM,
		HI_MID_TOM,
		CRASH_CYMBAL_1,
		HIGH_TOM,
		RIDE_CYMBAL_1,
		CHINESE_CYMBAL,
		RIDE_BELL,
		TAMBOURINE,
		SPLASH_CYMBAL,
		COWBELL,
		CRASH_CYMBAL_2,
		VIBRASLAP,
		RIDE_CYMBAL_2,
		HI_BONGO,
		LOW_BONGO,
		MUTE_HI_CONGA,
		OPEN_HI_CONGA,
		LOW_CONGA,
		HIGH_TIMBALE,
		LOW_TIMBALE,
		HIGH_AGOGO,
		LOW_AGOGO,
		CABASA,
		MARACAS,
		SHORT_WHISTLE,
		LONG_WHISTLE,
		SHORT_GUIRO,
		LONG_GUIRO,
		CLAVES,
		HI_WOOD_BLOCK,
		LOW_WOOD_BLOCK,
		MUTE_CUICA,
		OPEN_CUICA,
		MUTE_TRIANGLE,
		OPEN_TRIANGLE,
	};
}

