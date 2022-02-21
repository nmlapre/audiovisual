# audiovisual
A software synthesizer library and Windows GUI implementation

![image](https://user-images.githubusercontent.com/5578914/153744733-dde43b70-d917-4a9b-9d24-6cf7ef2ddcaa.png)

### Description
This project provides an oscillator library and a Windows ImGui implementation that uses the library. The GUI allows the user to customize a collection of oscillators, render the output to an audio device, and see the wave output in a live graph. Each oscillator may be a sine, square, saw, or triangle wave whose volume, pan, and frequency are adjustable in real time. The ultimate goal of this project is to connect this oscillator collection to visual input for generative music. It could also work as a general software synthesizer.

### Threading Model
Musicians want synthesizer output to update as responsively as possible - like a real instrument - for the best experience. To achieve minimum possible latency, the computer needs to generate 44,100 samples, per channel (left and right), per second, in the smallest chunks possible. When the audio output device requests a set of samples, the program must take no longer than `(chunk_size / 44100) seconds` to write the samples, or risk a jarring audio discontinuity. To make configuration easy, the program should display a responsive GUI that updates the rendered audio as quickly as possible.

To address these needs, this program has two threads: the GUI (non-realtime) thread, and the realtime thread. The realtime thread, managed by the `portaudio` library, runs with highest possible priority. That thread calls the audio callback, whose responsibility it is to write the next set of samples to the audio buffer passed to the audio device, at some (possibly variable) interval.

Since the GUI must change settings of the oscillators, and the realtime thread must read the settings of the oscillators, a goal of the architecture is to avoid data races. To this end, the threads communicate via events passed via lock-free queues. The GUI thread enqueues requests onto a single-producer single-consumer queue provided by the `farbot` library. The realtime thread reacts to those requests and enqueues responses on a separate queue that returns to the GUI thread. In this way, the GUI stays updated with the latest settings of the oscillators, without needing to read or write their settings directly.

In optimized builds, with a sufficiently low audio callback interval, the "event lag" between the GUI thread enqueueing an event and the realtime thread reacting to it is very low so as to be unnoticeable. On my Windows 10 laptop with ASIO4All drivers installed, I am able to get the audio sample chunk size down to 64 - around 1.5ms. With this threading model, the program achieves responsiveness that's perceived as immediate or "realtime."

### Extra Features
The program provides the ability to log and save the audio session to a file. Though this requires some allocation on the realtime thread (a bad idea), it is useful when debugging, and does not cause discontinuities even on my relatively underpowered laptop (i5-8250U @ 1.6GHz). Each oscillator automatically fades between changes of volume, pan, and frequency so no discontinuities arise while modifying settings. The program has the ability to graph the output live by logging the samples for both L and R channels with `implot`.
