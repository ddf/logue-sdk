#pragma once

#include <cstdint>
#include <cstring>

// keep track of MIDI Note and Gate On events
// so that when a note goes off, we can return to
// the most recently pressed key that is still pressed.
class Notes
{
  uint8_t notes[256];
  size_t  end;

public:
  static const uint8_t GATE = 128;

  Notes() : end(0)
  {
    memset(notes, 0, sizeof(notes));
  }

  void noteOn(uint8_t note)
  {
    // we use 0 to mean an empty entry
    notes[end++] = note+1;
  }

  // #TODO this might break if sent MIDI from the computer
  // where the same note is twice in a row before getting
  // a note off from the first instance of that note.
  void noteOff(uint8_t note)
  {
    // we use 0 to mean an empty entry
    note += 1;
    int count = end;
    for (int i = 0; i < count; ++i)
    {
      if (notes[i] == note)
      {
        notes[i] = 0;
        --end;
      }

      // shifting the next element down to fill the hole
      if (notes[i] == 0)
      {
        notes[i] = notes[i + 1];
        notes[i + 1] = 0;
      }
    }
  }

  void gateOn()
  {
    noteOn(GATE);
  }

  void gateOff()
  {
    noteOff(GATE);
  }

  size_t size() const { return end; }
  uint8_t last() const { return end > 0 ? notes[end-1] - 1 : 255; }
};