

#include "generalProfiler.h"
#include "partition.h"
#include "heatmap.h"


#include "reader.h"
#include "FIFO.h"
#include "Optimal.h"
#include "LRU_K.h"
#include "LRU.h"
#include "cacheHeader.h"

#include "csvReader.h"
#include "binaryReader.h"
#include "reader.h"

#include "FIFO.h"
#include "LFU.h"
#include "LFUFast.h"
#include "Optimal.h"
#include "eviction_stat.h"
#include "AMP.h"
#include "Mithril.h"
#include "LRUPage.h"
#include "PG.h"
#include "ARC.h"
#include "SLRU.h"
#include "LRFU.h"
#include "SLRUML.h"
#include "Score.h"


#define CACHESIZE 40000
#define BIN_SIZE (CACHESIZE)
#define TIME_INTERVAL 120 * 1000000
#define MODE 'r'
#define NUM_OF_THREADS 1

#define BLOCK_UNIT_SIZE 0    // 16 * 1024
#define DISK_SECTOR_SIZE 0  // 512


reader_t* getReader(char* fileloc) {
    return NULL;
}





int generalProfilerTest(int argc, char* argv[]) {
    int i;

    if (argc < 5) {
        printf("usage: %s filename fileType dataType alg cache_size(optional) bin_size(optional)\n", argv[0]);
        exit(1);
    }
    reader_t* reader;
    char dataType = argv[3][0];
    gint64 cache_size = CACHESIZE;
    gint64 bin_size = BIN_SIZE;

    if (argv[2][0] != 'c')
        reader = setup_reader(argv[1], argv[2][0], argv[3][0], BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, NULL);
    else {
        csvReader_init_params* init_params = (void*) new_csvReader_init_params(5, -1, 1, -1, TRUE, ',', 0);
        reader = setup_reader(argv[1], 'c', argv[3][0], BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, init_params);
    }
    if (argc >= 6)
        cache_size = atol(argv[5]);
    if (argc >= 7)
        bin_size = atol(argv[6]);

    struct_cache* cache;
    void *init_params_g = NULL;

    if (strcmp(argv[4], "LRU") == 0)
        cache = LRUPage_init(cache_size, dataType, BLOCK_UNIT_SIZE, NULL);
    else if (strcmp(argv[4], "FIFO") == 0)
        cache = fifo_init(cache_size, dataType, BLOCK_UNIT_SIZE, NULL);
    else if (strcmp(argv[4], "LFU") == 0)
        cache = LFU_init(cache_size, dataType, BLOCK_UNIT_SIZE, NULL);
    else if (strcmp(argv[4], "LFUFast") == 0)
        cache = LFU_fast_init(cache_size, dataType, BLOCK_UNIT_SIZE, NULL);
    else if (strcmp(argv[4], "ARC") == 0) {
        ARC_init_params_t *init_params = g_new0(ARC_init_params_t, 1);
        init_params_g = init_params;
        init_params->ghost_list_factor = 100;
        cache = ARC_init(cache_size, dataType, BLOCK_UNIT_SIZE, init_params);
    }
    else if (strcmp(argv[4], "SLRU") == 0) {
        SLRU_init_params_t *init_params = g_new0(SLRU_init_params_t, 1);
        init_params_g = init_params;
        init_params->N_segments = 2;
        cache = SLRU_init(cache_size, dataType, BLOCK_UNIT_SIZE, init_params);
    }
    else if (strcmp(argv[4], "SLRUML") == 0) {
        SLRUML_init_params_t *init_params = g_new0(SLRUML_init_params_t, 1);
        init_params_g = init_params;
        init_params->N_segments = 5;
        cache = SLRUML_init(cache_size, dataType, BLOCK_UNIT_SIZE, init_params);
    }
    else if (strcmp(argv[4], "Score") == 0) {
        Score_init_params_t *init_params = g_new0(Score_init_params_t, 1);
        init_params_g = init_params;
        cache = Score_init(cache_size, dataType, BLOCK_UNIT_SIZE, init_params);
    }
    else if (strcmp(argv[4], "LRFU") == 0) {
        ARC_init_params_t *init_params = g_new0(ARC_init_params_t, 1);
        init_params_g = init_params;
        init_params->ghost_list_factor = 100;
        cache = ARC_init(cache_size, dataType, BLOCK_UNIT_SIZE, init_params);
    }
    else if (strcmp(argv[4], "Optimal") == 0) {
        struct optimal_init_params* init_params = g_new0(struct optimal_init_params, 1);
        init_params_g = init_params;
        init_params->reader = reader;
        init_params->ts = 0;
        cache = optimal_init(cache_size, dataType, BLOCK_UNIT_SIZE, (void*)init_params);
    }
    else if (strcmp(argv[4], "PG") == 0) {
        PG_init_params_t* init_params = g_new0(PG_init_params_t, 1);
        init_params_g = init_params;
        init_params->prefetch_threshold = 0.3;
        init_params->lookahead = 1;
        init_params->cache_type = "LRU";
        init_params->max_meta_data = 0.1;
        init_params->block_size = 64 * 1024;
        cache = PG_init(cache_size, dataType, BLOCK_UNIT_SIZE, (void*)init_params);
    }
    else if (strcmp(argv[4], "AMP") == 0) {
        struct AMP_init_params* AMP_init_params = g_new0(struct AMP_init_params, 1);
        init_params_g = AMP_init_params;
        AMP_init_params->APT = 4;
        AMP_init_params->K      = 1;
        AMP_init_params->p_threshold = 256;
        AMP_init_params->read_size = 8;
        cache = AMP_init(cache_size, dataType, BLOCK_UNIT_SIZE, AMP_init_params);
    }

    else if (strcmp(argv[4], "Mithril") == 0) {
        Mithril_init_params_t* init_params = g_new0(Mithril_init_params_t, 1);
        init_params_g = init_params;
        init_params->cache_type = "LRU";
        init_params->confidence = 0;
        init_params->lookahead_range = 20;
        init_params->max_support = 12;
        init_params->min_support = 3;
        init_params->pf_list_size = 2;
        init_params->max_metadata_size = 0.1;
        init_params->block_size = BLOCK_UNIT_SIZE;
        init_params->sequential_type = 0;
        init_params->sequential_K = 0;
//        init_params->recording_loc = miss;
        init_params->AMP_pthreshold = 256;
        init_params->cycle_time = 2;
        init_params->rec_trigger = each_req;
        cache = Mithril_init(cache_size, dataType, BLOCK_UNIT_SIZE, init_params);
    }
    else {
        printf("cannot recognize algorithm\n");
        exit(1);
    }


    printf("after initialization, begin profiling\n");
//    gdouble* err_array = LRU_evict_err_statistics(reader, cache, 1000000);

//    for (i=0; i<reader->break_points->array->len-1; i++)
//        printf("%d: %lf\n", i, err_array[i]);
    int type = 2;

    if (type == 1) {
        partition_t *partitions = get_partition(reader, cache, 2);
        printf("final partition: ");
        for (i = 0; i < 2; i++)
            printf("%llu \t", (unsigned long long ) (partitions->current_partition[i]));
        printf("\n");
        printf("partition history length %u\n", partitions->partition_history[0]->len);
        free_partition_t(partitions);
    }
    if (type == 2) {
        return_res_t** res = profiler(reader, cache, NUM_OF_THREADS, (int) bin_size, 0, -1);
        for (i = 0; i < CACHESIZE / BIN_SIZE + 1; i++) {
            printf("%d, %lld: %lld\n", i, res[i]->cache_size, res[i]->hit_count);
            g_free(res[i]);
        }

        cache->core->destroy(cache);
        g_free(res);
    }
    if (type == 3) {
        return_res_t** res = profiler_partition(reader, cache, NUM_OF_THREADS, (int) bin_size);
        for (i = 0; i < CACHESIZE / BIN_SIZE + 1; i++) {
            printf("%d, %lld: %lld\n", i, res[i]->cache_size, res[i]->hit_count);
            g_free(res[i]);
        }
        cache->core->destroy(cache);
        g_free(res);
    }

    if (init_params_g)
        g_free(init_params_g);
    printf("after profiling\n");
    close_reader(reader);
    printf("test_finished!\n");
    return 0;
}



int readerTest(char* argv[]) {

    reader_t* reader = setup_reader("../data/trace.vscsi", 'v', 'l', BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, NULL);

    csvReader_init_params* init_params1 = (void*) new_csvReader_init_params(5, -1, 2, -1, TRUE, ',', -1);
    reader_t* reader2 = setup_reader("../data/trace.csv", 'c', 'l', BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, init_params1);

    reader_t* reader3 = setup_reader("../data/trace.csv", 'c', 'c', BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, init_params1);
    reader_t* reader4 = setup_reader("../data/trace.txt", 'p', 'c', BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, NULL);
    reader_t* reader42 = setup_reader("../data/trace.txt", 'p', 'l', BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, NULL);

    binary_init_params_t* init_params2 = g_new0(binary_init_params_t, 1);
    strcpy(init_params2->fmt, "<3I2H2Q");
    init_params2->label_pos = 6;
    init_params2->real_time_pos = 7;

    reader_t* reader5 = setup_reader("../data/trace.vscsi", 'b', 'l', BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, init_params2);
    binary_init_params_t* init_params3 = g_new0(binary_init_params_t, 1);
    strcpy(init_params3->fmt, "<I16s2H64s");
    init_params3->label_pos = 5;
    init_params3->real_time_pos = 1;
    // reader_t* reader6 = setup_reader("20161011.bin", 'b', 'c', init_params3);



    int i;
    cache_line *cp = new_cacheline();

    // vscsiReader
    printf("begin vscsi Reader, LBA is added 1\n");
    cp->type = 'l';
    for (i = 0; i < 3; i++) {
        read_one_element(reader, cp);
        printf("read in %lu\n", *(guint64*)(cp->item_p));
    }
    reset_reader(reader);
    for (i = 0; i < 3; i++) {
        read_one_element(reader, cp);
        printf("read in %lu\n", *(guint64*)(cp->item_p));
    }

    // csvReader
    printf("begin testing csvReader in guint64 mode\n");
    get_num_of_req(reader2);
    printf("csvReader %lu lines\n", reader2->base->total_num);
    for (i = 0; i < 3; i++) {
        read_one_element(reader2, cp);
        printf("read in %lu\n", *(guint64*)(cp->item_p));
    }
    reset_reader(reader2);
    for (i = 0; i < 3; i++) {
        read_one_element(reader2, cp);
        printf("read in %lu\n", *(guint64*)(cp->item_p));
    }
    reset_reader(reader2);
//    get_future_reuse_dist(reader2, 0, -1);


    printf("begin testing csvReader in string mode\n");
    cp->type = 'c';
    for (i = 0; i < 3; i++) {
        read_one_element(reader3, cp);
        printf("read in %s\n", cp->item);
    }
    reset_reader(reader3);
    for (i = 0; i < 3; i++) {
        read_one_element(reader3, cp);
        printf("read in %s\n", cp->item);
    }

    // plain reader
    printf("begin testing plainReader in guint64 mode\n");
    cp->type = 'l';
    get_num_of_req(reader42);
    printf("csvReader %lu lines\n", reader42->base->total_num);
    for (i = 0; i < 3; i++) {
        read_one_element(reader42, cp);
        printf("read in %lu\n", *(guint64*)(cp->item_p));
    }
    reset_reader(reader42);
    for (i = 0; i < 3; i++) {
        read_one_element(reader42, cp);
        printf("read in %lu\n", *(guint64*)(cp->item_p));
    }
    get_future_reuse_dist(reader42, 0, -1);


    printf("begin testing plainReader in string mode\n");
    cp->type = 'c';
    get_num_of_req(reader4);
    printf("csvReader %lu lines\n", reader4->base->total_num);
    for (i = 0; i < 3; i++) {
        read_one_element(reader4, cp);
        printf("read in %s\n", cp->item);
    }
    reset_reader(reader4);
    for (i = 0; i < 3; i++) {
        read_one_element(reader4, cp);
        printf("read in %s\n", cp->item);
    }
    get_future_reuse_dist(reader4, 0, -1);


    // binaryReader
    printf("begin binaryReader, LBA\n");
    cp->type = 'l';
    for (i = 0; i < 3; i++) {
        read_one_element(reader5, cp);
        printf("read in %lu\n", *(guint64*)(cp->item_p));
    }
    reset_reader(reader5);
    for (i = 0; i < 3; i++) {
        read_one_element(reader5, cp);
        printf("read in %lu\n", *(guint64*)(cp->item_p));
    }


    g_free(init_params1);
    destroy_cacheline(cp);
    close_reader(reader);
    close_reader(reader2);
    close_reader(reader3);
    close_reader(reader4);
    close_reader(reader42);
    close_reader(reader5);

    return 1;
}


int heatmapTest(int argc, char* argv[]) {
    printf("test_begin!\n");

    if (argc < 4) {
        printf("usage: program filename fileType dataType (alg)\n");
        exit(1);
    }
    reader_t* reader;
    char dataType = argv[3][0];

    if (argv[2][0] != 'c')
        reader = setup_reader(argv[1], argv[2][0], argv[3][0], BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, NULL);
    else {
        csvReader_init_params* init_params = (void*) new_csvReader_init_params(5, -1, 2, -1, TRUE, ',', -1);
        reader = setup_reader(argv[1], 'c', argv[3][0], BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, init_params);
    }

    printf("break points %d\n", get_bp_rtime(reader, 20 * 1000000, -1)->len);

    struct_cache* fifo = fifo_init(CACHESIZE, dataType, 0, NULL);

    struct optimal_init_params* init_params2 = g_new0(struct optimal_init_params, 1);
    init_params2->next_access = NULL;
    init_params2->reader = reader;
    struct_cache* optimal = optimal_init(CACHESIZE, dataType, 0, (void*)init_params2);


    struct_cache* cache = cache_init(CACHESIZE, reader->base->type, BLOCK_UNIT_SIZE);
    cache->core->type = e_LRU;
//    cache->core->type = e_Optimal;


    draw_dict* dd ;

    printf("after initialization, begin profiling\n");
    printf("hit_ratio_start_time_end_time\n");
    dd = heatmap(reader, cache, MODE, TIME_INTERVAL, -1, -1, hr_st_et, 0, 0, 8);
    free_draw_dict(dd);


//    gint64 time_interval,
//    gint64 bin_size,
//    gint64 num_of_pixel_for_time_dim,
//    heatmap_type_e plot_type,


    printf("hr_interval_size\n");
    dd = heatmap(reader, optimal, MODE, TIME_INTERVAL, 20000, -1, hr_interval_size, 0, 0, 8);

    free_draw_dict(dd);



    printf("rd_distribution CDF\n");
    dd = heatmap(reader, NULL, MODE, TIME_INTERVAL, -1, -1, rd_distribution_CDF, 0, 0, 8);
    free_draw_dict(dd);

//    printf("rd_distribution\n");
//    dd = heatmap(reader, NULL, MODE, TIME_INTERVAL, -1, rd_distribution, 0, 0, 8);
//    free_draw_dict(dd);

    printf("future rd_distribution\n");
    dd = heatmap(reader, NULL, MODE, TIME_INTERVAL, -1, -1, future_rd_distribution, 0, 0, 8);
    free_draw_dict(dd);

//    printf("diff hit_ratio_start_time_end_time\n");
//    dd = differential_heatmap(reader, cache, fifo, MODE, TIME_INTERVAL, -1, hit_ratio_start_time_end_time, 0, 0, 8);
//    free_draw_dict(dd);

    printf("computation finished\n");

    cache_destroy(cache);
    fifo->core->destroy(fifo);
    close_reader(reader);

    printf("test_finished!\n");

    return 0;
}


void eviction_stat_test(int argc, char* argv[]) {

    printf("test_begin!\n");

    if (argc < 5) {
        printf("usage: program filename fileType dataType alg");
        exit(1);
    }
    reader_t* reader;
    char dataType = argv[3][0];

    if (argv[2][0] != 'c')
        reader = setup_reader(argv[1], argv[2][0], argv[3][0], BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, NULL);
    else {
        csvReader_init_params* init_params = (void*) new_csvReader_init_params(4, -1, 1, -1, TRUE, ',', -1);
        reader = setup_reader(argv[1], 'c', argv[3][0], BLOCK_UNIT_SIZE, DISK_SECTOR_SIZE, init_params);
    }

    if (reader->base->total_num == -1)
        get_num_of_req(reader);
    printf("begin initialization, totally %ld requests\n", reader->base->total_num);


    struct optimal_init_params* init_params2 = g_new0(struct optimal_init_params, 1);
    init_params2->reader = reader;
    init_params2->ts = 0;
    struct_cache* cache = optimal_init(CACHESIZE, dataType, BLOCK_UNIT_SIZE, (void*)init_params2);
    printf("after initialization, begin profiling\n");


    gint64* result = eviction_stat(reader, cache, evict_freq);

    printf("first %ld\n", result[0]);


    cache->core->destroy(cache);
    g_free(result);
}

void test1(char* argv[]) {
    reader_t* reader;

    csvReader_init_params* init_params = (void*) new_csvReader_init_params(6, -1, 1, -1, FALSE, '\t', -1);
    reader = setup_reader(argv[1], 'c', 'c', 0, 0, init_params);

    cache_line_t* cp = new_cacheline();
    int i;
    for (i = 0; i < 18; i++) {
        read_one_element(reader, cp);
        printf("read in %s, time %ld\n", cp->item, cp->real_time);
    }
    destroy_cacheline(cp);
}


int main(int argc, char* argv[]) {
//    readerTest(argv);
//    generalProfilerTest(argc, argv);
//    test1(argv);
    LRUTest(argc, argv);
//    heatmapTest(argc, argv);

//    eviction_stat_test(argc, argv);

//    hrpe_test(argc, argv);
    return 1;
}

