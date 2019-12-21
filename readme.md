# wsay

Windows equivalent of macOS `say`.

Simple command line text-to-speech with easy file output and voice selection.

## Install Instructions
- Copy `wsay.exe` to a folder you like.
- Add that folder to your `Path` environment variable.
- Type `wsay` in your command prompt.

## Examples

```
wsay "Hello there."

wsay "I can output to a wav file." -o

wsay --list_voices

wsay "I can use different voices." --voice 10
```

## Help
```
Usage: wsay "sentence" [options]

Arguments:
 "sentence"    Sentence to say. You can use speech xml.

Options:
 -v, --voice <value>  Choose a different voice. Use the voice number printed using --list_voices.
 -o, --output_file    Outputs to out.wav
 -l, --list_voices    Lists available voices.
 -h, --help           Print this help
```

## Build
Install recent conan, cmake and compiler.

### Windows
```
mkdir build && cd build
cmake .. && cmake --build .
```
