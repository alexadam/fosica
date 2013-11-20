#include "fluidsynth.h"

fluid_synth_t* synth;
fluid_audio_driver_t* adriver;
fluid_sequencer_t* sequencer;
short synthSeqID, mySeqID;
unsigned int now;
unsigned int seqduration;

// prototype
void seq_callback(unsigned int time, fluid_event_t* event, fluid_sequencer_t* seq, void* data);

void createsynth() 
{
    fluid_settings_t* settings;
    settings = new_fluid_settings();
    fluid_settings_setstr(settings, "synth.reverb.active", "yes");
    fluid_settings_setstr(settings, "synth.chorus.active", "no");
    fluid_settings_setstr(settings, "audio.driver", "alsa");
    synth = new_fluid_synth(settings);
    adriver = new_fluid_audio_driver(settings, synth);
    sequencer = new_fluid_sequencer2(0);

    // register synth as first destination
    synthSeqID = fluid_sequencer_register_fluidsynth(sequencer, synth);

    // register myself as second destination
    mySeqID = fluid_sequencer_register_client(sequencer, "me", seq_callback, NULL);

    // the sequence duration, in ms
    seqduration = 1000;
}

void deletesynth() 
{
    delete_fluid_sequencer(sequencer);
    delete_fluid_audio_driver(adriver);
    delete_fluid_synth(synth);
}

void loadsoundfont() 
{
    int fluid_res;
    // put your own path here
    fluid_res = fluid_synth_sfload(synth, "/home/alex/Desktop/sf/sf.sf2", 1);
}

void sendnoteon(int chan, short key, unsigned int date) 
{
    int fluid_res;
    fluid_event_t *evt = new_fluid_event();
    fluid_event_set_source(evt, -1);
    fluid_event_set_dest(evt, synthSeqID);
    fluid_event_noteon(evt, chan, key, 127);
    fluid_res = fluid_sequencer_send_at(sequencer, evt, date, 1);
    delete_fluid_event(evt);
}

void schedule_next_callback() 
{
    int fluid_res;
    // I want to be called back before the end of the next sequence
    unsigned int callbackdate = now + seqduration/2;
    fluid_event_t *evt = new_fluid_event();
    fluid_event_set_source(evt, -1);
    fluid_event_set_dest(evt, mySeqID);
    fluid_event_timer(evt, NULL);
    fluid_res = fluid_sequencer_send_at(sequencer, evt, callbackdate, 1);
    delete_fluid_event(evt);
}

void schedule_next_sequence() {
    // Called more or less before each sequence start
    // the next sequence start date
    now = now + seqduration;

    // the sequence to play

    // the beat : 2 beats per sequence
    sendnoteon(0, 60, now + seqduration/2);
    sendnoteon(0, 60, now + seqduration);

    // melody
    sendnoteon(1, 45, now + seqduration/10);
    sendnoteon(1, 50, now + 4*seqduration/10);
    sendnoteon(1, 55, now + 8*seqduration/10);

    sendnoteon(1, 35, now + seqduration/10);
    sendnoteon(1, 35, now + 4*seqduration/10);
    sendnoteon(1, 39, now + 8*seqduration/10);

	fluid_synth_noteon(synth, 22, 60, 100);

    // so that we are called back early enough to schedule the next sequence
    schedule_next_callback();
}

/* sequencer callback */
void seq_callback(unsigned int time, fluid_event_t* event, fluid_sequencer_t* seq, void* data) {
    schedule_next_sequence();
}

int main(void) {
    createsynth();
    loadsoundfont();

    // initialize our absolute date
    now = fluid_sequencer_get_tick(sequencer);
    schedule_next_sequence();

    sleep(100000);
    deletesynth();
    return 0;
}
