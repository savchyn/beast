/* StandardGusPatchEnvelope - Standard GUS Patch Envelope
 * Copyright (C) 2004-2005 Stefan Westerfeld
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * based on code from Timidity++:
 *
 * TiMidity++ -- MIDI to WAVE converter and player
 * Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>
 * Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>
 *
 * Code to load and unload GUS-compatible instrument patches.
 */

#include <bse/bsecxxplugin.hh>
#include <bse/bsewave.h>
#include "standardguspatchenvelope.gen-idl.h"
#include <bse/gslwavechunk.h>
#include <bse/bsemathsignal.h>

using namespace std;
using namespace Sfi;

namespace Bse {
namespace Standard {

class GusPatchEnvelope : public GusPatchEnvelopeBase {
  /* properties (used to pass "global" envelope data into the modules) */
  struct Properties : public GusPatchEnvelopeProperties {
    BseWaveIndex *wave_index;

    Properties (GusPatchEnvelope *envelope)
      : GusPatchEnvelopeProperties (envelope),
        wave_index (envelope->wave_index)
    {
    }
  };
  /* actual computation */
  class Module : public SynthesisModule {
  private:
    gfloat envelope_value;
    BseWaveIndex *wave_index;
    GslWaveChunk *wave_chunk;
    bool retrigger;
    bool in_attack_or_sustain_phase;

    vector<float> envelope_rates;
    vector<float> envelope_offsets;
    bool envelope_valid;
    int envelope_phase;

  public:
    void
    config (Properties *properties)
    {
      wave_index = properties->wave_index;
    }
    void
    reset()
    {
      envelope_valid = false;
      retrigger = true;
      wave_chunk = 0;
    }

    enum EnvelopeConversion { CONVERT_RATE, CONVERT_OFFSET };

    float
    convert_envelope_value (EnvelopeConversion convert, guint8 byte)
    {
      if (convert == CONVERT_RATE)
	{
	  gint32 r;

	  r = 3 - ((byte >> 6) & 0x3);
	  r *= 3;
	  r = (gint32)(byte & 0x3f) << r; /* 6.9 fixed point */

	  return r * 44100 / mix_freq() / 512.0 / 1024.0;
	}
      return byte / 256.0;
    }

    void
    parse_envelope_floats (vector<float>& values, const gchar *key, EnvelopeConversion convert)
    {
      values.clear();

      const char *parse_me = bse_xinfos_get_value (wave_chunk->dcache->dhandle->setup.xinfos, key);
      if (parse_me)
	{
	  string val_string;

	  for (char c; (c = *parse_me); parse_me++)
	    {
	      if ((c >= '0' && c <= '9') || c == '.')
		{
		  val_string += c;
		}
	      else if (c == ',')
		{
		  values.push_back (convert_envelope_value (convert, atoi (val_string.c_str())));
		  val_string.clear();
		}
	    }
	  values.push_back (convert_envelope_value (convert, atoi (val_string.c_str())));
	}
    }
    void
    update_envelope (gfloat frequency)
    {
      envelope_valid = false;
      envelope_phase = 0;
      in_attack_or_sustain_phase = true;

      wave_chunk = bse_wave_index_lookup_best (wave_index, frequency);
      if (wave_chunk)
	{
	  parse_envelope_floats (envelope_rates, "gus-patch-envelope-rates", CONVERT_RATE);
	  parse_envelope_floats (envelope_offsets, "gus-patch-envelope-offsets", CONVERT_OFFSET);

	  if (envelope_rates.size() == 6 && envelope_offsets.size() == 6)
	    {
	      envelope_valid = true;

	      for (int i = 1; i < 6; i++)
		{
		  if (envelope_offsets[i-1] > envelope_offsets[i]) /* rate needs to be negative if envelope offset gets smaller */
		    envelope_rates[i] *= -1;
		}

	      /*
	      printf ("envelope:\n");
	      printf ("  wave-format=%s\n", bse_xinfos_get_value (wave_chunk->dcache->dhandle->setup.xinfos, "gus-patch-wave-format"));
	      for (int i = 0; i < 6; i++)
		printf ("  rate=%f, offset=%f\n", envelope_rates[i], envelope_offsets[i]);
	      */
	    }
	}
    }
    void
    process (unsigned int n_values)
    {
      if (retrigger)
	{
	  const gfloat *frequency = istream (ICHANNEL_FREQUENCY).values;
	  update_envelope (frequency[0]);
	  retrigger = false;
	}

      /* optimize me: we need 4 cases */
      if (ostream (OCHANNEL_AUDIO_OUT1).connected || ostream (OCHANNEL_AUDIO_OUT2).connected)
        {
          if (istream (ICHANNEL_AUDIO_IN).connected)
            {
	      const gfloat *gate = istream (ICHANNEL_GATE_IN).values;
              const gfloat *in = istream (ICHANNEL_AUDIO_IN).values;
	      gfloat *out1 = ostream (OCHANNEL_AUDIO_OUT1).values, *bound = out1 + n_values;
	      gfloat *out2 = ostream (OCHANNEL_AUDIO_OUT2).values;
	      gfloat *done = ostream (OCHANNEL_DONE_OUT).values;


	      while (out1 < bound)
		{
		  bool gate_active = (*gate++ > 0.5);
		  gdouble output;

		  if (envelope_valid)
		    {
		      if (gate_active)
			{
			  gdouble new_value = envelope_value + envelope_rates[envelope_phase];

			  if ((new_value > envelope_offsets[envelope_phase]) ^ (envelope_rates[envelope_phase] < 0))
			    {
			      envelope_value = envelope_offsets[envelope_phase];

			      if (envelope_phase < 2)
				envelope_phase++;
			    }
			  else
			    {
			      envelope_value = new_value;
			    }
			  output = envelope_value; /* attack is linear */
			}
		      else /* !gate_active */
			{
			  /* attack -> decay transition: here we switch from linear to exponential scale */
			  if (in_attack_or_sustain_phase)
			    {
			      /*
			      printf ("jumping from linear scale: %f\n", envelope_value);
			      */
			      envelope_value = log (envelope_value*64) / log (2) / 6;
			      /*
			      printf ("to exponential scale: %f\n", envelope_value);
			      printf ("to exponential scale which is with exp: %f\n", exp (envelope_value * log (2) * 6) / 64.0);
			      printf ("to exponential scale which is with approx: %f\n", bse_approx3_exp2 (envelope_value*6) / 64.0);
			      */
			      in_attack_or_sustain_phase = false;
			      envelope_phase++;
			    }

			  gdouble new_value = envelope_value + envelope_rates[envelope_phase];
			  if ((new_value > envelope_offsets[envelope_phase]) ^ (envelope_rates[envelope_phase] < 0))
			    {
			      envelope_value = envelope_offsets[envelope_phase];

			      if (envelope_phase < 5)
				envelope_phase++;
			    }
			  else
			    {
			      envelope_value = new_value;
			    }
			  output =  bse_approx3_exp2 (envelope_value*6) / 64.0;
			}
		      *done++ = (!gate_active && envelope_phase == 5) ? 1.0 : 0.0;
		    }
		  else
		    {
		      const gfloat envelope_incr = 0.01;
		      const gfloat epsilon       = envelope_incr / 2;

		      if (gate_active)
			envelope_value = min<gfloat> (envelope_value + envelope_incr, 1.0);
		      else
			envelope_value = max<gfloat> (envelope_value - envelope_incr, 0.0);

		      output = envelope_value;
		      *done++ = (!gate_active && envelope_value < epsilon) ? 1.0 : 0.0;
		    }

		  output *= *in++;

		  // FIXME: panning
		  *out1++ = output;
		  *out2++ = output;
		}
            }
	  else
	    {
	      ostream_set (OCHANNEL_AUDIO_OUT1, const_values (0));
	      ostream_set (OCHANNEL_AUDIO_OUT2, const_values (0));
	    }
        }
    }
  };

public:
  BseWaveIndex *wave_index;

  GusPatchEnvelope() : wave_index (NULL)
  {
  }

  bool
  property_changed (GusPatchEnvelopePropertyID prop_id)
  {
    switch (prop_id)
      {
      case PROP_WAVE:
	wave_index = wave ? bse_wave_get_index_for_modules (wave) : NULL;
      default: ;
      }
    return false;
  }

  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (GusPatchEnvelope, Module, Properties);
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (GusPatchEnvelope);

} // Standard
} // Bse
