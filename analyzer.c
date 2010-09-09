#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // uint8_t, uint16_t, etc.
#include <string.h> // strlen
#include <netinet/in.h> // hton, ntoh
#include "analyzer.h"

// File-scope ("global") variables
const uchar* flv_signature = "FLV";

const uchar* sound_formats[] = {
  "Linear PCM, platform endian",
  "ADPCM",
  "MP3",
  "Linear PCM, little endian",
  "Nellymoser 16-kHz mono",
  "Nellymoser 8-kHz mono",
  "Nellymoser",
  "G.711 A-law logarithmic PCM",
  "G.711 mu-law logarithmic PCM",
  "not defined by standard",
  "AAC",
  "Speex",
  "not defined by standard",
  "not defined by standard",
  "MP3 8-Khz",
  "Device-specific sound"
};

const uchar* sound_rates[] = {
  "5.5-Khz",
  "11-Khz",
  "22-Khz",
  "44-Khz"
};

const uchar* sound_sizes[] = {
  "8 bit",
  "16 bit"
};

const uchar* sound_types[] = {
  "Mono",
  "Stereo"
};

const uchar* frame_types[] = {
  "not defined by standard",
  "keyframe (for AVC, a seekable frame)",
  "inter frame (for AVC, a non-seekable frame)",
  "disposable inter frame (H.263 only)",
  "generated keyframe (reserved for server use only)",
  "video info/command frame"
};

const uchar* codec_ids[] = {
  "not defined by standard",
  "JPEG (currently unused)",
  "Sorenson H.263",
  "Screen video",
  "On2 VP6",
  "On2 VP6 with alpha channel",
  "Screen video version 2",
  "AVC"
};

const uchar* avc_packet_types[] = {
  "AVC sequence header",
  "AVC NALU",
  "AVC end of sequence (lower level NALU sequence ender is not required or supported)"
};

extern debug;

FILE* in;

void die() {
  printf("Error!\n");
    exit(-1);
}

// recover values that are less than one byte
uchar get_bits(uchar value, uchar start_bit, uchar count) {
  uchar mask;

  mask = ((1 << count)-1) << start_bit;
  return (mask & value) >> start_bit;
  
}

void print_header(struct flv_header* flv_header) {

  printf("FLV file version %u\n", flv_header->version);
  printf("  Contains audio tags: ");
  if(flv_header->type_flags & (1 << FLV_HEADER_AUDIO_BIT)) {
    printf("Yes\n");
  } else {
    printf("No\n");
  }
  printf("  Contains video tags: ");
  if(flv_header->type_flags & (1 << FLV_HEADER_VIDEO_BIT)) {
    printf("Yes\n");
  } else {
    printf("No\n");
  }
  printf("  Data offset: %lu\n",  (unsigned long) flv_header->data_offset);

}

size_t fread_1(uint8_t* ptr) {
  return fread(ptr, 1, 1, in);
}

size_t fread_3(uint32_t* ptr) {
  size_t count;
  uint8_t bytes[3];
  *ptr = 0;
  count = fread(bytes, 3, 1, in);
  *ptr = (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
  return count * 3;
}

size_t fread_4(uint32_t* ptr) {
  size_t count;
  uint8_t bytes[4];
  *ptr = 0;
  count = fread(bytes, 4, 1, in);
  *ptr = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
  return count * 4;
}

size_t fread_4s(uint32_t* ptr) {
  size_t count;
  uint8_t bytes[4];
  *ptr = 0;
  count = fread(bytes, 4, 1, in);
  return count * 4;
}

struct audio_tag* read_audio_tag(struct flv_tag* flv_tag) {
  size_t count;
  uchar byte;
  struct audio_tag* tag;

  tag = malloc(sizeof(struct audio_tag));
  count = fread_1(&byte);
  
  tag->sound_format = get_bits(byte, 4, 4);
  tag->sound_rate = get_bits(byte, 2, 2);
  tag->sound_size = get_bits(byte, 1, 1);
  tag->sound_type = get_bits(byte, 0, 1);

  printf("  Audio tag:\n");
  printf("    Sound format: %u - %s\n", tag->sound_format, sound_formats[tag->sound_format]);
  printf("    Sound rate: %u - %s\n", tag->sound_rate, sound_rates[tag->sound_rate]);

  printf("    Sound size: %u - %s\n", tag->sound_size, sound_sizes[tag->sound_size]);
  printf("    Sound type: %u - %s\n", tag->sound_type, sound_types[tag->sound_type]);

  tag->data = malloc((size_t) flv_tag->data_size-1);
  count = fread(tag->data, 1, (size_t) flv_tag->data_size-1, in);

  return tag;
}

struct video_tag* read_video_tag(struct flv_tag* flv_tag) {
  size_t count;
  uchar byte;
  struct video_tag* tag;

  tag = malloc(sizeof(struct video_tag));

  count = fread_1(&byte);
  
  tag->frame_type = get_bits(byte, 4, 4);
  tag->codec_id = get_bits(byte, 0, 4);

  printf("  Video tag:\n");
  printf("    Frame type: %u - %s\n", tag->frame_type, frame_types[tag->frame_type]);
  printf("    Codec ID: %u - %s\n", tag->codec_id, codec_ids[tag->codec_id]);
  
  // AVC-specific stuff
  if(tag->codec_id == FLV_CODEC_ID_AVC) {
    tag->data = read_avc_video_tag(tag, flv_tag, flv_tag->data_size - count);
  } else {
    tag->data = malloc((size_t) flv_tag->data_size - count);
    count = fread(tag->data, 1, (size_t) flv_tag->data_size - count, in);    
  }

  return tag;
}

struct avc_video_tag* read_avc_video_tag(struct video_tag* video_tag, struct flv_tag* flv_tag, uint32_t data_size) {
  size_t count;
  struct avc_video_tag* tag;

  tag = malloc(sizeof(struct avc_video_tag));

  count = fread_1(&(tag->avc_packet_type));
  count += fread_4s(&(tag->composition_time));
  
  printf("    AVC video tag:\n");
  printf("      AVC packet type: %u - %s\n", tag->avc_packet_type, avc_packet_types[tag->avc_packet_type]);
  printf("      AVC composition time: %i\n", tag->composition_time);

  tag->data = malloc((size_t) data_size - count);
  count = fread(tag->data, 1, (size_t) data_size - count, in);

  return tag;
}



void init_analyzer(FILE* in_file) {
  in = in_file;
}

int analyze() {
  struct flv_tag* tag;
  read_header();

  for(;;) {
    tag = read_tag(); // read the tag
    if(!tag) {
      return 0;
    }
    free_tag(tag); // and free it
  }

}

void free_tag(struct flv_tag* tag) {
  struct audio_tag* audio_tag;
  struct video_tag* video_tag;
  struct avc_video_tag* avc_video_tag;

  if(tag->tag_type == TAGTYPE_VIDEODATA) {
    video_tag = (struct video_tag*) tag->data;
    if(video_tag->codec_id == FLV_CODEC_ID_AVC) {
      avc_video_tag = (struct avc_video_tag*) video_tag->data;
      free(avc_video_tag->data);
      free(video_tag->data);
      free(tag->data);
      free(tag);
    } else {
      free(video_tag->data);
      free(tag->data);
      free(tag);
    }
  } else if(tag->tag_type == TAGTYPE_AUDIODATA) {


    audio_tag = (struct audio_tag*) tag->data;
    free(audio_tag->data);
    free(tag->data);
    free(tag);
  } else {

    free(tag->data);
    free(tag);
  }
}

int read_header() {
  size_t count;
  int i;
  struct flv_header* flv_header;

  flv_header = malloc(sizeof(struct flv_header));
  count = fread(flv_header, 1, sizeof(struct flv_header), in);

  // XXX strncmp
  for(i=0; i < strlen(flv_signature); i++) {
    if(flv_header->signature[i] != flv_signature[i]) {
      die();
    }
  }

  flv_header->data_offset = ntohl(flv_header->data_offset);

  print_header(flv_header);

  return 0;
  
}


void print_general_tag_info(struct flv_tag* tag) {
  printf("  Data size: %lu\n", (unsigned long) tag->data_size);
  printf("  Timestamp: %lu\n", (unsigned long) tag->timestamp);
  printf("  Timestamp extended: %u\n", tag->timestamp_ext);
  printf("  StreamID: %lu\n", (unsigned long) tag->stream_id);
}

struct flv_tag* read_tag() {
  size_t count;
  uint32_t prev_tag_size;
  struct flv_tag* tag;

  tag = malloc(sizeof(struct flv_tag));

  count = fread_4(&prev_tag_size);

  // Start reading next tag
  count = fread_1(&(tag->tag_type));
  if(feof(in)) {
    return NULL;
  }
  count = fread_3(&(tag->data_size));
  count = fread_3(&(tag->timestamp));
  count = fread_1(&(tag->timestamp_ext));
  count = fread_3(&(tag->stream_id));  

  printf("\n");
  printf("Prev tag size: %lu\n", (unsigned long) prev_tag_size);
  
  printf("Tag type: %u - ", tag->tag_type);
  switch(tag->tag_type) {
  case TAGTYPE_AUDIODATA:
    printf("Audio data\n");
    print_general_tag_info(tag);
    tag->data = (void*) read_audio_tag(tag);
    break;
  case TAGTYPE_VIDEODATA:
    printf("Video data\n");
    print_general_tag_info(tag);
    tag->data = malloc((size_t) tag->data_size);
    tag->data = (void*) read_video_tag(tag);
    break;
  case TAGTYPE_SCRIPTDATAOBJECT:
    printf("Script data object\n");
    print_general_tag_info(tag);
    tag->data = malloc((size_t) tag->data_size);
    count = fread(tag->data, 1, (size_t) tag->data_size, in);
    break;
  default:
    printf("Unknown tag type!\n");
    die();
  }
  
  // Did we reach end of file?
  if(feof(in)) {
    return NULL;
  }

  return tag;
}
