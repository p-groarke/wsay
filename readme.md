# wsay

Windows equivalent of macOS `say`.

Simple command line text-to-speech with easy file output, voice selection and more.


## Features
- Output to wav file.
- Select different voices (including new Windows 10 voices).
- Supports speech xml.
- Play text file.
- Interactive mode.


## Install Instructions
- Copy `wsay.exe` to a folder you like.
- Add that folder to your `Path` environment variable.
- Type `wsay` in your command prompt.


## Examples

```
wsay "Hello there."

wsay "I can output to a wav file (out.wav)." -o

wsay --list_voices
	1 : Microsoft David Desktop - English (United States)
	2 : Microsoft Hazel Desktop - English (Great Britain)
	3 : Microsoft Zira Desktop - English (United States)
	4 : Microsoft David - English (United States)
	5 : Microsoft James - English (Australia)
	6 : Microsoft Linda - English (Canada)
etc.

wsay "I can use different voices." --voice 6

wsay "You can name the ouput file." -o my_output_file.wav

wsay -i i_can_read_a_text_file.txt

wsay --interactive
	[Enters an interactive mode. Type sentences and press enter to speak them. Type !exit to leave.]

wsay -v 6 -i mix_and_match_options.txt -o output.wav
```


## Help
```
Usage: wsay "sentence" [options]

Arguments:
 "sentence"    Sentence to say. You can use speech xml.

Options:
 -v, --voice <value>           Choose a different voice. Use the voice number printed using --list_voices.
 -o, --output_file <optional>  Outputs to wav file. Uses 'out.wav' if no filename is provided.
 -l, --list_voices             Lists available voices.
 -i, --input_text <value>      Play text from '.txt' file. Supports speech xml.
 -I, --interactive             Enter interactive mode. Type sentences, they will be spoken when you press enter.
                               Use 'ctrl+c' to exit, or type '!exit' (without quotes).
 -h, --help                    Print this help
```

## Build
Install recent conan, cmake and compiler.


### Windows
```
mkdir build && cd build
cmake .. && cmake --build .
```
