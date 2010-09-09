#define FLV_HEADER_AUDIO_BIT 2
#define FLV_HEADER_VIDEO_BIT 0

#define FLV_CODEC_ID_AVC 7

enum tag_types {
  TAGTYPE_AUDIODATA = 8,
  TAGTYPE_VIDEODATA = 9,
  TAGTYPE_SCRIPTDATAOBJECT = 18
};

// Types
typedef unsigned char uchar;

struct flv_header {
  uchar signature[3];
  uchar version;
  uchar type_flags;
  uint32_t data_offset;
} __attribute__((__packed__));

struct flv_tag {
  uchar tag_type;
  uint32_t data_size;
  uint32_t timestamp;
  uchar timestamp_ext;
  uint32_t stream_id;
  void* data; // will point to an audio_tag or video_tag
};

struct audio_tag {
  uchar sound_format;
  uchar sound_rate;
  uchar sound_size;
  uchar sound_type;
  void* data;
};

struct video_tag {
  uchar frame_type;
  uchar codec_id;
  void* data;
};

struct avc_video_tag {
  uchar avc_packet_type;
  int32_t composition_time;
  void* data;
};

// Functions

void die();
int read_header();
struct flv_tag* read_tag();
void print_header();
struct audio_tag* read_audio_tag(struct flv_tag* flv_tag);
struct video_tag* read_video_tag(struct flv_tag* flv_tag);
struct avc_video_tag* read_avc_video_tag(struct video_tag* video_tag, struct flv_tag* flv_tag, uint32_t data_size);
int read_header();
uchar get_bits(uchar value, uchar start_bit, uchar count);
void print_header(struct flv_header* flv_header);
size_t fread_1(uint8_t* ptr);
size_t fread_3(uint32_t* ptr);
size_t fread_4(uint32_t* ptr);
size_t fread_4s(uint32_t* ptr);
void free_tag(struct flv_tag* tag);
void init_analyzer(FILE* in_file);
int analyze();
