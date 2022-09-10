# wsay

Windows equivalent of macOS `say`.

Simple command line text-to-speech with easy file output, voice selection and more.


## Features
- Output to wav file.
- Select different voices (including new Windows 10 voices).
- Supports speech xml (text file mode only).
- Play text file.
- Interactive mode.
- Supports selecting playback device.
- Supports multiple simultaneous playback devices and file output.
- Full unicode (utf8) support (supports asian languages, Slovak, etc).
  - Note : Window's legacy command prompt has some of it's own issues with utf8.
  - If you need full unicode support, best to use the new [Windows Terminal](https://aka.ms/terminal).
- Supports utf8, utf16le and utf16be text files.
- Volume and speed options.


## Install Instructions
- Copy `wsay.exe` to a folder you like.
- Add that folder to your `Path` environment variable.
- Type `wsay` in your command prompt.


## Examples

```bash
# Say something.
wsay "Hello there."

# Ouput to a wav file. If no filename is entered, outputs to 'out.wav'.
wsay "I can output to a wav file." -o

# List supported voices. Install new Windows voices for more choices.
wsay --list_voices
	1 : Microsoft David Desktop - English (United States)
	2 : Microsoft Hazel Desktop - English (Great Britain)
	3 : Microsoft Zira Desktop - English (United States)
	4 : Microsoft David - English (United States)
	5 : Microsoft James - English (Australia)
	6 : Microsoft Linda - English (Canada)
	# etc.

# Use the number provided by '--list_voices' to select a different voice.
wsay "I can use different voices." --voice 6

# Provide an output filename.
wsay "You can name the ouput file." -o my_output_file.wav

# Read text from a text file instead.
wsay -i i_can_read_a_text_file.txt

# In interactive mode, type sentences and press enter for them to be read.
# Use !exit to quit.
# Use !stop to stop speaking.
wsay --interactive
[Info] Type sentences, press enter to speak them.
[Command] '!exit' : Exit interactive mode.
[Command] '!stop' : Interrupt speaking.

# List supported playback devices.
wsay --list_devices
	1 : BenQ GW2765 (NVIDIA High Definition Audio)
	2 : Speakers (AudioQuest DragonFly Black v1.5)
	3 : Digital Audio (S/PDIF) (High Definition Audio Device)
	# etc.

	Use 'all' to select every device.

# Speak using a non-default playback device.
wsay "I am speaking on another playback device." --playback_device 2

# You can play on multiple devices and save to a file simultaneously.
# Seperate the device numbers with spaces.
# WARNING : The "sentence" must come before the --playback_device (-p) option if it is used!
wsay "Output EVERYWHERE \o/" -p 1 2 -o

# Use the 'all' shortcut to playback on every device simultaneously.
wsay "I conquer all devices" -p all

# You can set the voice volume, from 0 to 100.
wsay "Softly speaking" --volume 25

# You can set the voice speed, from 0 to 100. 50 is the default speed.
wsay "Quickly speaking" --speed 75

# Here, we are using voice 6, reading text from a file and outputting to 'output.wav'.
wsay -v 6 -i mix_and_match_options.txt -o output.wav

# Ouput to multiple devices using interactive mode with voice 5.
wsay -v 5 -I -p 1 2

# Speak slowly and quietly, on all devices, using voice 7 and save to wav file.
wsay "Multiple options example." -v 7 -p all -s 25 -V 25 -o
```


## Help
```
Usage: wsay "sentence" [options]

Arguments:
 "sentence"    Sentence to say. You can use speech xml.

Options:
 -i, --input_text <value>          Play text from '.txt' file. Supports speech xml.
 -I, --interactive                 Enter interactive mode. Type sentences, they will be spoken when you press enter.
                                   Use 'ctrl+c' or type '!exit' to quit.
 -d, --list_devices                List detected playback devices.
 -l, --list_voices                 Lists available voices.
 -o, --output_file <optional>      Outputs to wav file. Uses 'out.wav' if no filename is provided.
 -p, --playback_device <multiple>  Specify a playback device. Use the number provided by --list_devices.
                                   You can provide more than 1 playback device, seperate the numbers with spaces. You
                                   can also mix output to file + playback.
                                   Use 'all' to select all devices.
 -s, --speed <value>               Sets the voice speed, from 0 to 100. 50 is the default speed.
 -v, --voice <value>               Choose a different voice. Use the voice number printed using --list_voices.
 -V, --volume <value>              Sets the voice volume, from 0 to 100.
 -h, --help                        Print this help
```

## Build
Install recent conan, cmake and compiler.


### Windows
```
mkdir build && cd build
cmake .. && cmake --build .
```
