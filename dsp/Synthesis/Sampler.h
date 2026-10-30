#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <libraries/AudioFile/AudioFile.h>
#include "../Utility.h"
#include "../Other/Adsr.h"

namespace dsp
{
/** Plays back a recorded sound - from a WAV file, or recorded live
 *  from the audio input - at any speed, forwards or backwards, once
 *  or in a loop. Speed changes the pitch too, like a tape or record:
 *  2 = twice as fast and an octave up, 0.5 = half as fast and an
 *  octave down.
 *
 *  SetStart() and SetEnd() pick which part of the sound plays (0 =
 *  the very beginning, 1 = the very end), so you can play just one
 *  drum hit of a loop, or loop a tiny slice of a voice.
 *
 *  Load() reads a whole file into memory, so it belongs in setup() -
 *  never in render(). The path is relative to the project folder,
 *  e.g. "samples/drum_loop.wav". Recording into memory is real-time
 *  safe: the space for it is set aside in Init().
 *
 *  It has the same shape as the synth voices (SetFreq, NoteOn, ...),
 *  so dsp::VoiceManager can play several notes of a sample at once:
 *  SetRootFreq() is the pitch the sound has at normal speed. Load the
 *  file into every voice: voices.Voice(v).Load(...).
 *
 *  Usage, a file:
 *      Sampler sampler;
 *      sampler.Init(sample_rate);
 *      sampler.Load("samples/drum_loop.wav"); // in setup()
 *      ...
 *      sampler.NoteOn();                     // play it from the start
 *      sampler.SetSpeed(1.5f);               // faster and higher
 *      float sample = sampler.Process();
 *
 *  Usage, recording from the audio input:
 *      sampler.StartRecording();             // e.g. when a button is pressed
 *      sampler.Record(audioRead(context, n, 0)); // every sample, in the loop
 *      sampler.StopRecording();              // then NoteOn() plays it back
 */
class Sampler
{
  public:
    /** max_record_seconds: how long a recording can be. The memory for
     *  it is set aside here, so recording never has to wait for it. */
    void Init(float sample_rate, float max_record_seconds = 10.f)
    {
        sample_rate_  = sample_rate;
        rec_capacity_ = (size_t)(max_record_seconds * sample_rate);
        data_[0].assign(rec_capacity_, 0.f);
        data_[1].clear();
        length_    = 0;
        channels_  = 1;
        file_rate_ = sample_rate;
        fade_frames_ = 0.002f * sample_rate; // 2 ms

        env_.Init(sample_rate);
        env_.SetAttack(0.002f);
        env_.SetDecay(0.001f);
        env_.SetSustain(1.f);
        env_.SetRelease(0.05f);

        SetSpeed(1.f);
        SetRootFreq(NoteToFreq("C4"));
        SetStart(0.f);
        SetEnd(1.f);
        SetLoop(false);
        SetReverse(false);
        playing_   = false;
        recording_ = false;
    }

    /** Loads a WAV (or AIFF, FLAC, ...) file. Mono or stereo; a file
     *  with more channels uses its first two. Only in setup(). Returns
     *  false, and prints why, if the file can't be read. */
    bool Load(const std::string& filename)
    {
        // The file's own sample rate, so it plays at the right speed
        // even if it isn't the board's
        SF_INFO  info = {};
        SNDFILE* file = sf_open(filename.c_str(), SFM_READ, &info);
        if(!file)
        {
            printf("Sampler: can't open %s - is it in the project folder?\n", filename.c_str());
            return false;
        }
        sf_close(file);

        std::vector<std::vector<float> > channels = AudioFileUtilities::load(filename);
        if(channels.empty() || channels[0].empty())
        {
            printf("Sampler: %s has no sound in it\n", filename.c_str());
            return false;
        }

        playing_   = false;
        recording_ = false;
        channels_  = channels.size() > 1 ? 2 : 1;
        for(int c = 0; c < channels_; c++)
            data_[c] = channels[c];
        length_    = data_[0].size();
        file_rate_ = info.samplerate;

        // Keep room for a recording after the file, too
        if(data_[0].size() < rec_capacity_)
            data_[0].resize(rec_capacity_, 0.f);

        printf("Sampler: loaded %s (%.2f s, %s)\n",
               filename.c_str(),
               GetLength(),
               channels_ == 2 ? "stereo" : "mono");
        return true;
    }

    /** Playback speed. 1 = as recorded, 2 = twice as fast (an octave
     *  up), 0.5 = half as fast (an octave down). */
    void SetSpeed(float speed) { speed_ = speed > 0.f ? speed : 0.f; }

    /** Sets the speed from a pitch, for playing the sample like a
     *  synth voice: SetFreq(root freq) plays it at normal speed. */
    void SetFreq(float freq_hz) { SetSpeed(freq_hz / root_freq_); }

    /** The pitch of the sound at normal speed (default C4). Only used
     *  by SetFreq(). */
    void SetRootFreq(float freq_hz) { root_freq_ = freq_hz > 1.f ? freq_hz : 1.f; }

    /** Where playback starts, 0 (beginning) - 1 (end of the sound). */
    void SetStart(float position) { start_ = Clamp(position, 0.f, 1.f); }

    /** Where playback ends, 0 - 1. Below SetStart(), it plays just a
     *  tiny slice. */
    void SetEnd(float position) { end_ = Clamp(position, 0.f, 1.f); }

    /** true = when it reaches the end, it starts again until NoteOff().
     *  false = plays once and stops. */
    void SetLoop(bool loop) { loop_ = loop; }

    /** true = plays backwards, from SetEnd() to SetStart(). */
    void SetReverse(bool reverse) { reverse_ = reverse; }

    /** Fade-in at every note, in seconds (default 2 ms - just enough
     *  to avoid a click). */
    void SetAmpAttack(float time_s) { env_.SetAttack(time_s); }

    /** Fade-out after NoteOff(), in seconds (default 50 ms). */
    void SetAmpRelease(float time_s) { env_.SetRelease(time_s); }

    /** Plays the sound from its start (or from its end, in reverse).
     *  Does nothing while recording, or before anything is loaded. */
    void NoteOn()
    {
        if(recording_ || length_ == 0)
            return;
        if(!playing_)
            env_.Reset(); // fade in from silence
        env_.NoteOn();
        double first, last;
        Region(first, last);
        pos_     = reverse_ ? last : first;
        playing_ = true;
    }

    /** Fades the sound out (see SetAmpRelease). A sound that isn't
     *  looping also stops by itself at its end. */
    void NoteOff() { env_.NoteOff(); }

    /** Starts recording from the beginning of the memory. Feed it the
     *  input with Record(), then call StopRecording(). */
    void StartRecording()
    {
        if(rec_capacity_ == 0)
            return;
        playing_   = false;
        recording_ = true;
        rec_pos_   = 0;
    }

    /** Ends the recording. From now on NoteOn() plays it. If nothing
     *  was recorded, the old sound stays. */
    void StopRecording()
    {
        if(!recording_)
            return;
        recording_ = false;
        if(rec_pos_ > 0)
        {
            length_    = rec_pos_;
            channels_  = 1;
            file_rate_ = sample_rate_;
        }
    }

    /** Call once per sample with the input, e.g.
     *  Record(audioRead(context, n, 0)). Only records between
     *  StartRecording() and StopRecording(), and stops by itself when
     *  the memory is full. */
    void Record(float in)
    {
        if(!recording_)
            return;
        data_[0][rec_pos_++] = in;
        if(rec_pos_ >= rec_capacity_)
            StopRecording();
    }

    /** Returns the next sample, mono (a stereo sound is mixed down). */
    float Process()
    {
        float left, right;
        Process(left, right);
        return channels_ == 2 ? 0.5f * (left + right) : left;
    }

    /** Stereo version: writes the next left and right samples (both
     *  the same for a mono sound). */
    void Process(float& out_l, float& out_r)
    {
        out_l = out_r = 0.f;
        if(!playing_)
            return;

        float env = env_.Process();
        if(!env_.IsRunning())
        {
            playing_ = false; // the fade-out after NoteOff() is over
            return;
        }

        double first, last;
        Region(first, last);

        // How far to move on each sample. file_rate_ / sample_rate_
        // corrects for a file recorded at another sample rate.
        double step = speed_ * file_rate_ / sample_rate_;

        // When it plays once, fade out over the last 2 ms instead of
        // stopping with a click
        float gain = env;
        if(!loop_ && step > 0.0)
        {
            double samples_left = (reverse_ ? pos_ - first : last - pos_) / step;
            if(samples_left < fade_frames_)
                gain *= (float)(samples_left / fade_frames_);
        }

        out_l = gain * Read(0, pos_);
        out_r = channels_ == 2 ? gain * Read(1, pos_) : out_l;

        pos_ += reverse_ ? -step : step;

        // Past the end (or, backwards, the start)? Loop or stop.
        double length = last - first;
        if(pos_ > last)
        {
            if(reverse_)
                pos_ = last; // SetEnd() moved below where we are
            else if(loop_)
                pos_ = pos_ - length > first ? pos_ - length : first;
            else
                playing_ = false;
        }
        else if(pos_ < first)
        {
            if(!reverse_)
                pos_ = first; // SetStart() moved past where we are
            else if(loop_)
                pos_ = pos_ + length < last ? pos_ + length : last;
            else
                playing_ = false;
        }
    }

    /** True while a note is sounding. */
    bool IsPlaying() const { return playing_; }

    /** True between StartRecording() and StopRecording(). */
    bool IsRecording() const { return recording_; }

    /** Length of the whole sound, in seconds. */
    float GetLength() const { return file_rate_ > 0.f ? length_ / file_rate_ : 0.f; }

  private:
    /** The part of the sound that plays, in frames: SetStart() to
     *  SetEnd(), at least 1 ms long. */
    void Region(double& first, double& last) const
    {
        double frames = length_ > 1 ? length_ - 1 : 0;
        first = start_ * frames;
        last  = end_ * frames;
        double shortest = 0.001 * file_rate_;
        if(last < first + shortest)
            last = first + shortest < frames ? first + shortest : frames;
        if(last <= first)
            first = last > shortest ? last - shortest : 0.0;
    }

    /** The sound at a position between two frames, smoothly
     *  interpolated from the four frames around it (cubic Hermite), so
     *  slowed-down sounds stay clean. */
    float Read(int channel, double pos) const
    {
        const std::vector<float>& d = data_[channel];
        long  i = (long)pos;
        float t = (float)(pos - i);
        float xm1 = d[Index(i - 1)];
        float x0  = d[Index(i)];
        float x1  = d[Index(i + 1)];
        float x2  = d[Index(i + 2)];

        float c = (x1 - xm1) * 0.5f;
        float v = x0 - x1;
        float w = c + v;
        float a = w + v + (x2 - x0) * 0.5f;
        float b = w + a;
        return ((a * t - b) * t + c) * t + x0;
    }

    size_t Index(long i) const
    {
        if(i < 0)
            return 0;
        if((size_t)i >= length_)
            return length_ - 1;
        return (size_t)i;
    }

    std::vector<float> data_[2]; // one per channel; [0] also holds recordings
    size_t             length_       = 0; // frames of sound in data_
    size_t             rec_capacity_ = 0;
    size_t             rec_pos_      = 0;
    int                channels_     = 1;
    float              sample_rate_  = 44100.f;
    float              file_rate_    = 44100.f;
    float              fade_frames_  = 88.f;

    double pos_       = 0.0; // double: float loses the fraction in long sounds
    float  speed_     = 1.f;
    float  root_freq_ = 261.63f;
    float  start_     = 0.f;
    float  end_       = 1.f;
    bool   loop_      = false;
    bool   reverse_   = false;
    bool   playing_   = false;
    bool   recording_ = false;

    Adsr env_;
};

} // namespace dsp
