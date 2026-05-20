#include <stdio.h>
#include <string.h>

#include "gnu_soc_proto.h"

#define TEST_CHECK(cond)                                                                  \
    do                                                                                     \
    {                                                                                      \
        if (!(cond))                                                                       \
        {                                                                                  \
            fprintf(stderr, "[soc_proto_c_test] CHECK failed: %s (line %d)\n", #cond, __LINE__); \
            return 1;                                                                      \
        }                                                                                  \
    } while (0)

static int test_build_and_parse_mcu_report(void)
{
    gnu_soc_proto_mcu_report_t report;
    uint8_t frame[GNU_SOC_PROTO_FRAME_MAX_LEN];
    size_t frame_len = 0u;
    gnu_soc_proto_parser_t parser;
    gnu_soc_proto_packet_t packet;
    size_t i;

    for (i = 0u; i < sizeof(report); i++)
    {
        ((uint8_t *)&report)[i] = (uint8_t)(i & 0xFFu);
    }

    TEST_CHECK(gnu_soc_proto_build_mcu_report_frame(&report, frame, sizeof(frame), &frame_len) == GNU_SOC_PROTO_STATUS_OK);
    TEST_CHECK(frame_len == (sizeof(report) + 6u));
    TEST_CHECK(frame[0] == GNU_SOC_PROTO_HEAD0);
    TEST_CHECK(frame[1] == GNU_SOC_PROTO_HEAD1);
    TEST_CHECK(frame[2] == (uint8_t)(sizeof(report) + 1u));
    TEST_CHECK(frame[3] == (uint8_t)GNU_SOC_PROTO_CMD_REPORT_MCU_STATE);

    gnu_soc_proto_parser_init(&parser);
    for (i = 0u; i < frame_len; i++)
    {
        const gnu_soc_proto_status_t st = gnu_soc_proto_parser_feed(&parser, frame[i], &packet);
        if (i + 1u < frame_len)
        {
            TEST_CHECK(st == GNU_SOC_PROTO_STATUS_NEED_MORE);
        }
        else
        {
            TEST_CHECK(st == GNU_SOC_PROTO_STATUS_OK);
        }
    }

    TEST_CHECK(packet.cmd_id == (uint8_t)GNU_SOC_PROTO_CMD_REPORT_MCU_STATE);
    TEST_CHECK(packet.payload_len == sizeof(report));
    TEST_CHECK(memcmp(packet.payload, &report, sizeof(report)) == 0);

    return 0;
}

static int test_crc_error_detect(void)
{
    gnu_soc_proto_mcu_report_t report;
    uint8_t frame[GNU_SOC_PROTO_FRAME_MAX_LEN];
    size_t frame_len = 0u;
    gnu_soc_proto_parser_t parser;
    gnu_soc_proto_packet_t packet;
    size_t i;
    int saw_crc_error = 0;

    memset(&report, 0x5Au, sizeof(report));
    TEST_CHECK(gnu_soc_proto_build_mcu_report_frame(&report, frame, sizeof(frame), &frame_len) == GNU_SOC_PROTO_STATUS_OK);
    frame[frame_len - 1u] ^= 0x01u;

    gnu_soc_proto_parser_init(&parser);
    for (i = 0u; i < frame_len; i++)
    {
        const gnu_soc_proto_status_t st = gnu_soc_proto_parser_feed(&parser, frame[i], &packet);
        if (st == GNU_SOC_PROTO_STATUS_BAD_CRC)
        {
            saw_crc_error = 1;
        }
    }

    TEST_CHECK(saw_crc_error == 1);
    return 0;
}

static int test_build_and_parse_soc_ctrl(void)
{
    gnu_soc_proto_soc_ctrl_t ctrl;
    uint8_t frame[GNU_SOC_PROTO_FRAME_MAX_LEN];
    size_t frame_len = 0u;
    gnu_soc_proto_parser_t parser;
    gnu_soc_proto_packet_t packet;
    size_t i;

    ctrl.v_ctl = 100;
    ctrl.w_ctl = -200;

    TEST_CHECK(gnu_soc_proto_build_frame((uint8_t)GNU_SOC_PROTO_CMD_REPORT_SOC_CTRL,
                                         (const uint8_t *)&ctrl,
                                         (uint8_t)sizeof(ctrl),
                                         frame,
                                         sizeof(frame),
                                         &frame_len) == GNU_SOC_PROTO_STATUS_OK);

    TEST_CHECK(frame_len == (sizeof(ctrl) + 6u));

    gnu_soc_proto_parser_init(&parser);
    for (i = 0u; i < frame_len; i++)
    {
        const gnu_soc_proto_status_t st = gnu_soc_proto_parser_feed(&parser, frame[i], &packet);
        if (i + 1u < frame_len)
        {
            TEST_CHECK(st == GNU_SOC_PROTO_STATUS_NEED_MORE);
        }
        else
        {
            TEST_CHECK(st == GNU_SOC_PROTO_STATUS_OK);
        }
    }

    TEST_CHECK(packet.cmd_id == (uint8_t)GNU_SOC_PROTO_CMD_REPORT_SOC_CTRL);
    TEST_CHECK(packet.payload_len == sizeof(ctrl));
    TEST_CHECK(memcmp(packet.payload, &ctrl, sizeof(ctrl)) == 0);

    return 0;
}

int main(void)
{
    if (test_build_and_parse_mcu_report() != 0)
    {
        return 1;
    }

    if (test_crc_error_detect() != 0)
    {
        return 1;
    }

    if (test_build_and_parse_soc_ctrl() != 0)
    {
        return 1;
    }

    printf("soc_proto_c_test: OK\n");
    return 0;
}
