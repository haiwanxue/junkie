// -*- c-basic-offset: 4; c-backslash-column: 79; indent-tabs-mode: nil -*-
// vim:sw=4 ts=4 sts=4 expandtab
#include <stdlib.h>
#undef NDEBUG
#include <assert.h>
#include <junkie/cpp.h>
#include <junkie/tools/ext.h>
#include <junkie/tools/objalloc.h>
#include <junkie/proto/pkt_wait_list.h>
#include <junkie/proto/cap.h>
#include <junkie/proto/ip.h>
#include <junkie/proto/eth.h>
#include <junkie/proto/tcp.h>
#include "lib.h"
#include "proto/postgres.c"
#include "sql_test.h"

static struct parse_test {
    uint8_t const *packet;
    int size;
    enum proto_parse_status ret;         // Expected proto status
    struct sql_proto_info expected;
    enum way way;
} parse_tests[] = {

    // A select
    {
        .packet = (uint8_t const []) {
            0x51,0x00,0x00,0x00,0x1f,0x73,0x65,0x6c,0x65,0x63,0x74,0x20,0x2a,0x20,0x66,0x72,
            0x6f,0x6d,0x20,0x6d,0x65,0x74,0x61,0x5f,0x74,0x61,0x62,0x6c,0x65,0x20,0x3b,0x00
        },
        .size = 0x20,
        .ret = PROTO_OK,
        .way = FROM_SERVER,
        .expected = {
            .info = { .head_len = 0x20, .payload = 0x0},
            .msg_type = SQL_QUERY,
            .set_values = SQL_SQL,
            .u = { .query = { .sql = "select * from meta_table ;" } },
        },
    },

    // A authentification query
    {
        .packet = (uint8_t const []) {
            0x52,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x05,0x32,0xf7,0xdb,0x40
        },
        .size = 0xd,
        .ret = PROTO_OK,
        .way = FROM_SERVER,
        .expected = {
            .info = { .head_len = 0xd, .payload = 0},
            .msg_type = SQL_STARTUP,
        },
    },

    // a successuful authentification
    {
        .packet = (uint8_t const []) {
            0x52,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x53,0x00,0x00,0x00,0x1b,0x63,0x6c,
            0x69,0x65,0x6e,0x74,0x5f,0x65,0x6e,0x63,0x6f,0x64,0x69,0x6e,0x67,0x00,0x4c,0x41,
            0x54,0x49,0x4e,0x31,0x00,0x53,0x00,0x00,0x00,0x17,0x44,0x61,0x74,0x65,0x53,0x74,
            0x79,0x6c,0x65,0x00,0x49,0x53,0x4f,0x2c,0x20,0x4d,0x44,0x59,0x00,0x53,0x00,0x00,
            0x00,0x19,0x69,0x6e,0x74,0x65,0x67,0x65,0x72,0x5f,0x64,0x61,0x74,0x65,0x74,0x69,
            0x6d,0x65,0x73,0x00,0x6f,0x6e,0x00,0x53,0x00,0x00,0x00,0x1b,0x49,0x6e,0x74,0x65,
            0x72,0x76,0x61,0x6c,0x53,0x74,0x79,0x6c,0x65,0x00,0x70,0x6f,0x73,0x74,0x67,0x72,
            0x65,0x73,0x00,0x53,0x00,0x00,0x00,0x14,0x69,0x73,0x5f,0x73,0x75,0x70,0x65,0x72,
            0x75,0x73,0x65,0x72,0x00,0x6f,0x6e,0x00,0x53,0x00,0x00,0x00,0x1b,0x73,0x65,0x72,
            0x76,0x65,0x72,0x5f,0x65,0x6e,0x63,0x6f,0x64,0x69,0x6e,0x67,0x00,0x4c,0x41,0x54,
            0x49,0x4e,0x31,0x00,0x53,0x00,0x00,0x00,0x19,0x73,0x65,0x72,0x76,0x65,0x72,0x5f,
            0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x00,0x38,0x2e,0x34,0x2e,0x36,0x00,0x53,0x00,
            0x00,0x00,0x20,0x73,0x65,0x73,0x73,0x69,0x6f,0x6e,0x5f,0x61,0x75,0x74,0x68,0x6f,
            0x72,0x69,0x7a,0x61,0x74,0x69,0x6f,0x6e,0x00,0x72,0x69,0x78,0x65,0x64,0x00,0x53,
            0x00,0x00,0x00,0x24,0x73,0x74,0x61,0x6e,0x64,0x61,0x72,0x64,0x5f,0x63,0x6f,0x6e,
            0x66,0x6f,0x72,0x6d,0x69,0x6e,0x67,0x5f,0x73,0x74,0x72,0x69,0x6e,0x67,0x73,0x00,
            0x6f,0x66,0x66,0x00,0x53,0x00,0x00,0x00,0x17,0x54,0x69,0x6d,0x65,0x5a,0x6f,0x6e,
            0x65,0x00,0x6c,0x6f,0x63,0x61,0x6c,0x74,0x69,0x6d,0x65,0x00,0x4b,0x00,0x00,0x00,
            0x0c,0x00,0x00,0x7c,0x0d,0x14,0x1f,0xfb,0xa4,0x5a,0x00,0x00,0x00,0x05,0x49
        },
        .size = 0x12f,
        .ret = PROTO_OK,
        .way = FROM_SERVER,
        .expected = {
            .info = { .head_len = 0x12f, .payload = 0},
            .msg_type = SQL_STARTUP,
            .request_status = SQL_REQUEST_COMPLETE,
            .set_values = SQL_REQUEST_STATUS | SQL_ENCODING,
            .u = { .startup = { .encoding = SQL_ENCODING_LATIN1 } },
        },
    },

    // A bind + parse
    {
        .packet = (uint8_t const []) {
            0x42, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x53, 0x5f, 0x31, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x35, 0x00, 0x73, 0x65, 0x6c, 0x65,
            0x63, 0x74, 0x20, 0x6e, 0x65, 0x78, 0x74, 0x76, 0x61, 0x6c, 0x28, 0x27,
            0x6d, 0x61, 0x73, 0x74, 0x65, 0x72, 0x70, 0x5f, 0x74, 0x61, 0x62, 0x6c,
            0x65, 0x5f, 0x31, 0x5f, 0x69, 0x6e, 0x5f, 0x70, 0x6b, 0x65, 0x79, 0x5f,
            0x73, 0x65, 0x71, 0x27, 0x29, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00,
            0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00,
            0x00, 0x06, 0x50, 0x00, 0x45, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x53, 0x00, 0x00, 0x00, 0x04
        },
        .size = 0x73,
        .ret = PROTO_OK,
        .way = FROM_SERVER,
        .expected = {
            .info = { .head_len = 0x73, .payload = 0x0},
            .msg_type = SQL_QUERY,
            .set_values = SQL_SQL,
            .u = { .query = { .sql = "select nextval('masterp_table_1_in_pkey_seq')" } },
        },
    },

    // A row description
    {
        .packet = (uint8_t const []) {
            0x32, 0x00, 0x00, 0x00, 0x04, 0x43, 0x00, 0x00, 0x00, 0x0a, 0x42, 0x45,
            0x47, 0x49, 0x4e, 0x00, 0x31, 0x00, 0x00, 0x00, 0x04, 0x32, 0x00, 0x00,
            0x00, 0x04, 0x54, 0x00, 0x00, 0x00, 0x20, 0x00, 0x01, 0x6e, 0x65, 0x78,
            0x74, 0x76, 0x61, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x14, 0x00, 0x08, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x44,
            0x00, 0x00, 0x00, 0x12, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x31, 0x33,
            0x39, 0x38, 0x35, 0x36, 0x35, 0x30, 0x43, 0x00, 0x00, 0x00, 0x0d, 0x53,
            0x45, 0x4c, 0x45, 0x43, 0x54, 0x20, 0x31, 0x00, 0x5a, 0x00, 0x00, 0x00,
            0x05, 0x54
        },
        .size = 0x62,
        .ret = PROTO_OK,
        .way = FROM_SERVER,
        .expected = {
            .info = { .head_len = 0x62, .payload = 0x0},
            .msg_type = SQL_QUERY,
            .set_values = SQL_REQUEST_STATUS | SQL_NB_ROWS | SQL_NB_FIELDS,
            .request_status = SQL_REQUEST_COMPLETE,
            .u = { .query = { .nb_fields = 1, .nb_rows = 1 } },
        },
    },

};

static unsigned cur_test;

static void pgsql_info_check(struct proto_subscriber unused_ *s, struct proto_info const *info_,
        size_t unused_ cap_len, uint8_t const unused_ *packet, struct timeval const unused_ *now)
{
    // Check info against parse_tests[cur_test].expected
    struct sql_proto_info const *const info = DOWNCAST(info_, info, sql_proto_info);
    struct sql_proto_info const *const expected = &parse_tests[cur_test].expected;
    assert(!compare_expected_sql(info, expected));
}

static void parse_check(void)
{
    struct timeval now;
    timeval_set_now(&now);
    struct parser *parser = proto_pgsql->ops->parser_new(proto_pgsql);
    struct pgsql_parser *pgsql_parser = DOWNCAST(parser, parser, pgsql_parser);
    assert(pgsql_parser);
    struct proto_subscriber sub;
    hook_subscriber_ctor(&pkt_hook, &sub, pgsql_info_check);

    for (cur_test = 0; cur_test < NB_ELEMS(parse_tests); cur_test++) {
        struct parse_test const *test = parse_tests + cur_test;
        printf("Check packet %d of size 0x%x (%d)\n", cur_test, test->size, test->size);
        pgsql_parser->phase = NONE;
        enum proto_parse_status ret = pg_parse(parser, NULL, 0, test->packet, test->size,
                test->size, &now, test->size, test->packet);
        assert(ret == test->ret);
    }
}

static void fetch_nb_rows_check(void)
{
    static struct nbr_test {
        char const *result;
        enum proto_parse_status expected_status;
        unsigned expected_nb_rows;
    } tests[] = {
        { "INSERT 16",  PROTO_OK, 16 },
        { "UPDATE 1",   PROTO_OK, 1 },
        { "INSERT 666", PROTO_OK, 666 },
        { "INSERT",     PROTO_PARSE_ERR, 0 },
        { "",           PROTO_PARSE_ERR, 0 },
        { "INSERT 23X", PROTO_PARSE_ERR, 0 },
    };

    for (unsigned t = 0; t < NB_ELEMS(tests); t++) {
        struct nbr_test const *const test = tests+t;
        unsigned nb_rows = -1;
        enum proto_parse_status status = fetch_nb_rows(test->result, &nb_rows);
        assert(status == test->expected_status);
        if (status == PROTO_OK) {
            assert(nb_rows == test->expected_nb_rows);
        }
    }
}

int main(void)
{
    log_init();
    ext_init();
    mutex_init();
    objalloc_init();
    streambuf_init();
    proto_init();
    pkt_wait_list_init();
    ref_init();
    cap_init();
    eth_init();
    ip_init();
    ip6_init();
    tcp_init();
    pgsql_init();
    log_set_level(LOG_DEBUG, NULL);
    log_set_file("postgres_check.log");

    fetch_nb_rows_check();
    parse_check();

    doomer_stop();
    pgsql_fini();
    tcp_fini();
    ip6_fini();
    ip_fini();
    eth_fini();
    cap_fini();
    ref_fini();
    pkt_wait_list_fini();
    proto_fini();
    streambuf_fini();
    objalloc_fini();
    mutex_fini();
    ext_fini();
    log_fini();
    return EXIT_SUCCESS;
}

