// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_INSTRUMENT_H__
#define __BSE_INSTRUMENT_H__

#include	<bse/bseitem.hh>



/* --- BSE type macros --- */
#define BSE_TYPE_INSTRUMENT		 (BSE_TYPE_ID (BseInstrument))
#define BSE_INSTRUMENT(object)		 (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_INSTRUMENT, BseInstrument))
#define BSE_INSTRUMENT_CLASS(class)	 (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_INSTRUMENT, BseInstrumentClass))
#define BSE_IS_INSTRUMENT(object)	 (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_INSTRUMENT))
#define BSE_IS_INSTRUMENT_CLASS(class)	 (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_INSTRUMENT))
#define BSE_INSTRUMENT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_INSTRUMENT, BseInstrumentClass))


/* --- BseInstrument --- */
typedef struct _BseEnvelope BseEnvelope;
typedef enum
{
  BSE_INSTRUMENT_NONE,			/*< skip >*/
  BSE_INSTRUMENT_WAVE,			/*< nick=Custom Wave >*/
  BSE_INSTRUMENT_SYNTH_NET,		/*< nick=Custom Synth Net >*/
  BSE_INSTRUMENT_STANDARD_PIANO,	/*< nick=Piano >*/
  BSE_INSTRUMENT_LAST			/*< skip >*/
} BseInstrumentType;
struct _BseEnvelope
{
  guint	 delay_time;
  guint	 attack_time;
  gfloat attack_level;
  guint	 decay_time;
  gfloat sustain_level;
  guint	 sustain_time;
  gfloat release_level;
  guint	 release_time;
};
struct _BseInstrument
{
  BseItem	     parent_instance;

  BseInstrumentType  type;
  BseWave	    *wave;
  BseSNet	    *user_snet;
  BseSNet	    *seq_snet;	/* sequencer snet */

  gfloat	     volume_factor;
  gint		     balance;
  gint		     transpose;
  gint		     fine_tune;

  BseEnvelope	     env;
};
struct _BseInstrumentClass
{
  BseItemClass parent_class;
};


/* --- prototypes -- */



#endif /* __BSE_INSTRUMENT_H__ */
