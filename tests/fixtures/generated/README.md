# Generated codec fixtures

These files contain a synthetic 440 Hz sine wave, 0.25 seconds, mono 44.1 kHz. No recorded music or third-party sample was used; these generated fixtures are distributed under the repository license. Generated with FFmpeg 9.0.2 as follows:

```sh
ffmpeg -f lavfi -i 'sine=frequency=440:sample_rate=44100:duration=0.25' -ac 1 -c:a pcm_s16le -map_metadata -1 tone.wav
ffmpeg -f lavfi -i 'sine=frequency=440:sample_rate=44100:duration=0.25' -ac 1 -c:a flac -map_metadata -1 tone.flac
ffmpeg -f lavfi -i 'sine=frequency=440:sample_rate=44100:duration=0.25' -ac 1 -c:a libmp3lame -b:a 128k -write_xing 0 -id3v2_version 0 -map_metadata -1 tone.mp3
ffmpeg -f lavfi -i 'sine=frequency=440:sample_rate=44100:duration=0.25' -ac 1 -c:a pcm_s16be -map_metadata -1 tone.aiff
```

They verify basic decoding and source preservation only, not musical accuracy, tagging, or every codec variant.
