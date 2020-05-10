# QtTestPlayAudio
Plays a stereo wav file into the default windows output

Right now this can only safely play a stereo wav with uncompressed float samples into into a stereo output, 
as it just copies raw bits from the file to the output. I did a quick scetch in how to copy samplewise and react
to different channel configs, which is in the code and commented out.
