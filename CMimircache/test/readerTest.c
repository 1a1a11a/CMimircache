

//
//  readerTest.c
//  test
//
//  Created by Jason on 2/20/18.
//  Copyright © 2018 Juncheng. All rights reserved.
//


#include "reader.h"
#include "csvReader.h"
#include "binaryReader.h"


#define BLOCK_UNIT_SIZE 0    // 16 * 1024
#define DISK_SECTOR_SIZE 0  // 512


void readerTest() {

    reader_t* reader_vscsi = setup_reader("../data/trace.vscsi", 'v', 'l', BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, NULL);

    reader_t* reader_plain_c = setup_reader("../data/trace.txt", 'p', 'c', BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, NULL);
    reader_t* reader_plain_l = setup_reader("../data/trace.txt", 'p', 'l', BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, NULL);

    csvReader_init_params* init_params_csv = g_new0(csvReader_init_params, 1);
    init_params_csv->delimiter = ',';
    init_params_csv->real_time_column = 2;
    init_params_csv->label_column = 5;
    init_params_csv->has_header = TRUE;
    reader_t* reader_csv_l = setup_reader("../data/trace.csv", 'c', 'l', BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, init_params_csv);
    reader_t* reader_csv_c = setup_reader("../data/trace.csv", 'c', 'c', BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, init_params_csv);

    binary_init_params_t* init_params_bin = g_new0(binary_init_params_t, 1);
    strcpy(init_params_bin->fmt, "<3I2H2Q");
    init_params_bin->label_pos = 6;
    init_params_bin->real_time_pos = 7;
    reader_t* reader_bin_l = setup_reader("../data/trace.vscsi", 'b', 'l', BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, init_params_bin);


    int i;
    cache_line *cp = new_cacheline();

    // TRUE DATA
    guint64 trace_length = 113872;
    guint64 trace_data[3] = {42932745, 42932746, 42932747};
    char* trace_data_s[3] = {"42932745", "42932746", "42932747"};
    
    // vscsiReader
    cp->type = 'l';
    g_assert_cmpuint(get_num_of_req(reader_vscsi), ==, trace_length);
    for (i = 0; i < 3; i++) {
        read_one_element(reader_vscsi, cp);
        g_assert_cmpuint(*(guint64*)(cp->item_p), ==, trace_data[i]);
    }
    reset_reader(reader_vscsi);
    for (i = 0; i < 3; i++) {
        read_one_element(reader_vscsi, cp);
        g_assert_cmpuint(*(guint64*)(cp->item_p), ==, trace_data[i]);
    }
    reset_reader(reader_vscsi);
    

    // csvReader
    g_assert_cmpuint(get_num_of_req(reader_csv_l), ==, trace_length);
    for (i = 0; i < 3; i++) {
        read_one_element(reader_csv_l, cp);
        g_assert_cmpuint(*(guint64*)(cp->item_p), ==, trace_data[i]);
    }
    reset_reader(reader_csv_l);
    for (i = 0; i < 3; i++) {
        read_one_element(reader_csv_l, cp);
        g_assert_cmpuint(*(guint64*)(cp->item_p), ==, trace_data[i]);
    }
    reset_reader(reader_csv_l);


    cp->type = 'c';
    for (i = 0; i < 3; i++) {
        read_one_element(reader_csv_c, cp);
        g_assert_cmpstr(cp->item, ==, trace_data_s[i]);
    }
    reset_reader(reader_csv_c);
    for (i = 0; i < 3; i++) {
        read_one_element(reader_csv_c, cp);
        g_assert_cmpstr(cp->item, ==, trace_data_s[i]);
    }

    // plain reader
    cp->type = 'l';
    g_assert_cmpuint(get_num_of_req(reader_plain_l), ==, trace_length);
    for (i = 0; i < 3; i++) {
        read_one_element(reader_plain_l, cp);
        g_assert_cmpuint(*(guint64*)(cp->item_p), ==, trace_data[i]);
    }
    reset_reader(reader_plain_l);
    for (i = 0; i < 3; i++) {
        read_one_element(reader_plain_l, cp);
        g_assert_cmpuint(*(guint64*)(cp->item_p), ==, trace_data[i]);
    }

    cp->type = 'c';
    g_assert_cmpuint(get_num_of_req(reader_plain_c), ==, trace_length);
    for (i = 0; i < 3; i++) {
        read_one_element(reader_plain_c, cp);
        g_assert_cmpstr(cp->item, ==, trace_data_s[i]);
    }
    reset_reader(reader_plain_c);
    for (i = 0; i < 3; i++) {
        read_one_element(reader_plain_c, cp);
        g_assert_cmpstr(cp->item, ==, trace_data_s[i]);
    }


    // binaryReader
    cp->type = 'l';
    g_assert_cmpuint(get_num_of_req(reader_bin_l), ==, trace_length);
    for (i = 0; i < 3; i++) {
        read_one_element(reader_bin_l, cp);
        g_assert_cmpuint(*(guint64*)(cp->item_p), ==, trace_data[i]);
    }
    reset_reader(reader_bin_l);
    for (i = 0; i < 3; i++) {
        read_one_element(reader_bin_l, cp);
        g_assert_cmpuint(*(guint64*)(cp->item_p), ==, trace_data[i]);
    }


    g_free(init_params_csv);
    g_free(init_params_bin);
    destroy_cacheline(cp);
    close_reader(reader_vscsi);
    close_reader(reader_csv_l);
    close_reader(reader_csv_c);
    close_reader(reader_plain_c);
    close_reader(reader_plain_l);
    close_reader(reader_bin_l);

}



int main(int argc, char* argv[]) {
    g_test_init (&argc, &argv, NULL);
    g_test_add_func ("/test/readerTest", (GTestFunc) readerTest);
    return g_test_run();
}

