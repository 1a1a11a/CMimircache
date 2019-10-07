//
//  reader.h
//  LRUAnalyzer
//
//  Created by Juncheng on 5/25/16.
//  Copyright © 2016 Juncheng. All rights reserved.
//

#ifndef READER_H
#define READER_H

#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "const.h"
#include "libcsv.h"
#include "logging.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_TAB 0x09
#define FILE_SPACE 0x20
#define FILE_CR 0x0d
#define FILE_LF 0x0a
#define FILE_COMMA 0x2c
#define FILE_QUOTE 0x22

#define BINARY_FMT_MAX_LEN 32
#define MAX_LINE_LEN 1024 * 1024

// trace label_type
#define CSV 'c'
#define PLAIN 'p'
#define VSCSI 'v'
#define BINARY 'b'


// label_type, this name will be changed to label_type
typedef enum { LB_NUM, LB_STR } label_t;

typedef struct break_point {
  GArray *array;
  char mode; // r or v
  guint64 time_interval;
} break_point_t;

struct cache_line {
  gpointer label_ptr;
  char label[REQ_LABEL_MAX_LEN];
  char label_type; /* label_type of content can be either guint64(l) or char*(c)
                    */

  gint64 ts; /* deprecated, should not use, virtual timestamp */
  gint64 size;
  gint64 real_time;
  // used to check whether current request is a valid request
  gboolean valid;

  gint64 op;
  gint64 unused_param1;
  gint64 unused_param2;

  void* extra_data;

  size_t block_unit_size;
  size_t disk_sector_size;

  // not used
  /* id of cache server, used in akamaiSimulator */
  unsigned long cache_server_id;
  unsigned char traceID; /* this is for mixed trace */
  void *content;         /* the content of page/request */
  guint size_of_content; /* the size of mem area content points to */
};

typedef struct cache_line request_t;


/* declare reader struct */
struct reader;

typedef struct reader_base {

  char trace_type; /* possible types: c(csv), v(vscsi),
                    * p(plain text), b(binaryReader)  */
  char label_type; /* possible types: l(guint64), c(char*) */

  int block_unit_size; /* used when consider variable request size
                        * it is size of basic unit of a big request,
                        * in CPHY data, it is 512 bytes */
                       /* currently not used */
  int disk_sector_size;

  FILE *file;
  char file_loc[FILE_LOC_STR_SIZE];
  //    char rd_file_loc[FILE_LOC_STR_SIZE];    /* the file that stores reuse
  //    distance of the data,
  //                                             * format: gint64 per entry */
  //    char frd_file_loc[FILE_LOC_STR_SIZE];
  size_t file_size;
  void *init_params;

  char *mapped_file; /* mmap the file, this should not change during runtime
                      * offset in the file is changed using offset */
  guint64 offset;
  size_t record_size; /* the size of one record, used to
                       * locate the memory location of next element,
                       * when used in vscsiReaser and binaryReader,
                       * it is a const value,
                       * when it is used in plainReader or csvReader,
                       * it is the size of last record, it does not
                       * include LFCR or 0 */

  gint64 total_num; /* number of records */

  gint ver;

  void *params; /* currently not used */

} reader_base_t;

typedef struct reader_data_unique {

  double *hit_ratio;
  double *hit_ratio_shards;
  double log_base;

} reader_data_unique_t;

typedef struct reader_data_share {
  break_point_t *break_points;
  gint64 *reuse_dist;
  char reuse_dist_type; // NORMAL_REUSE_DISTANCE or FUTURE_REUSE_DISTANCE
  gint64 max_reuse_dist;
  gint *last_access;

} reader_data_share_t;

typedef struct reader {
  struct reader_base *base;
  struct reader_data_unique *udata;
  struct reader_data_share *sdata;
  void *reader_params;
} reader_t;

reader_t *setup_reader(const char *file_loc, const char file_type,
                       const char data_type, const int block_size,
                       const int disk_sector_size,
                       const void *const setup_params);

void read_one_element(reader_t *const reader, request_t *const c);

guint64 skip_N_elements(reader_t *const reader, const guint64 N);

int go_back_one_line(reader_t *const reader);

int go_back_two_lines(reader_t *const reader);

void read_one_element_above(reader_t *const reader, request_t *const c);

int read_one_request_all_info(reader_t *const reader, void *storage);

guint64 read_one_timestamp(reader_t *const reader);

void read_one_op(reader_t *const reader, void *op);

guint64 read_one_request_size(reader_t *const reader);

void reader_set_read_pos(reader_t *const reader, double pos);

guint64 get_num_of_req(reader_t *const reader);

void reset_reader(reader_t *const reader);

int close_reader(reader_t *const reader);

int close_reader_unique(reader_t *const reader);

reader_t *clone_reader(reader_t *const reader);

void set_no_eof(reader_t *const reader);

request_t *new_req_struct(void);
request_t *copy_req(request_t *req);

void destroy_req_struct(request_t *cp);

static inline gboolean find_line_ending(reader_t *const reader, char **line_end,
                                        long *const line_len) {
  /**
   *  find the closest line ending, save at line_end
   *  line_end should point to the character that is not current line,
   *  in other words, the character after all LFCR
   *  line_len is the length of current line, does not include CRLF, nor \0
   *  return TRUE, if end of file
   *  return FALSE else
   */

  size_t size = MAX_LINE_LEN;
  *line_end = NULL;

  while (*line_end == NULL) {
    if (size > (long)reader->base->file_size - reader->base->offset)
      size = reader->base->file_size - reader->base->offset;
    *line_end = (char *)memchr(
        (void *)((char *)(reader->base->mapped_file) + reader->base->offset),
        CSV_LF, size);
    if (*line_end == NULL)
      *line_end = (char *)memchr((char *)(reader->base->mapped_file) +
                                     reader->base->offset,
                                 CSV_CR, size);

    if (*line_end == NULL) {
      if (size == MAX_LINE_LEN) {
        WARNING("line length exceeds %d characters, now double max_line_len\n",
                MAX_LINE_LEN);
        size *= 2;
      } else {
        /*  end of trace, does not -1 here
         *  if file ending has no CRLF, then file_end points to end of file,
         * return TRUE; if file ending has one or more CRLF, it will goes to
         * next while, then file_end points to end of file, still return TRUE
         */
        *line_end =
            (char *)(reader->base->mapped_file) + reader->base->file_size;
        *line_len = size;
        reader->base->record_size = *line_len;
        return TRUE;
      }
    }
  }
  // currently line_end points to LFCR
  *line_len =
      *line_end - ((char *)(reader->base->mapped_file) + reader->base->offset);

  while ((long)(*line_end - (char *)(reader->base->mapped_file)) <
             (long)(reader->base->file_size) - 1 &&
         (*(*line_end + 1) == CSV_CR || *(*line_end + 1) == CSV_LF ||
          *(*line_end + 1) == CSV_TAB || *(*line_end + 1) == CSV_SPACE)) {
    (*line_end)++;
    if ((long)(*line_end - (char *)(reader->base->mapped_file)) ==
        (long)(reader->base->file_size) - 1) {
      reader->base->record_size = *line_len;
      return TRUE;
    }
  }
  if ((long)(*line_end - (char *)(reader->base->mapped_file)) ==
      (long)(reader->base->file_size) - 1) {
    reader->base->record_size = *line_len;
    return TRUE;
  }
  // move to next line, non LFCR
  (*line_end)++;
  reader->base->record_size = *line_len;

  return FALSE;
}

#ifdef __cplusplus
}
#endif

#endif /* reader_h */
