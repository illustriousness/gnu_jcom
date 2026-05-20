#include "gnu_soc_proto.h"

#include <string.h>

#define GNU_SOC_PROTO_LEN_FIELD_MIN 1u
#define GNU_SOC_PROTO_LEN_FIELD_MAX ((uint8_t)(GNU_SOC_PROTO_PAYLOAD_MAX_LEN + 1u))
#define GNU_SOC_PROTO_SERIALIZED_PAYLOAD_MAX_LEN 0xFEu

typedef enum
{
    GNU_SOC_PROTO_STATE_HEAD0 = 0,
    GNU_SOC_PROTO_STATE_HEAD1,
    GNU_SOC_PROTO_STATE_LENGTH,
    GNU_SOC_PROTO_STATE_BODY_AND_CRC,
} gnu_soc_proto_state_t;

static void gnu_soc_proto_parser_reset(gnu_soc_proto_parser_t *parser)
{
    if (parser == NULL)
    {
        return;
    }

    parser->state = (uint8_t)GNU_SOC_PROTO_STATE_HEAD0;
    parser->len_field = 0u;
    parser->body_index = 0u;
}

static gnu_soc_proto_status_t gnu_soc_proto_packet_from_frame(const gnu_soc_proto_parser_t *parser,
                                                               gnu_soc_proto_packet_t *packet_out)
{
    uint8_t payload_len;

    if ((parser == NULL) || (packet_out == NULL))
    {
        return GNU_SOC_PROTO_STATUS_BAD_ARG;
    }

    if ((parser->len_field < GNU_SOC_PROTO_LEN_FIELD_MIN) || (parser->len_field > GNU_SOC_PROTO_LEN_FIELD_MAX))
    {
        return GNU_SOC_PROTO_STATUS_BAD_FRAME;
    }

    payload_len = (uint8_t)(parser->len_field - GNU_SOC_PROTO_LEN_FIELD_MIN);
    if (payload_len > GNU_SOC_PROTO_PAYLOAD_MAX_LEN)
    {
        return GNU_SOC_PROTO_STATUS_BAD_FRAME;
    }

    packet_out->cmd_id = parser->frame_buf[3];
    packet_out->payload_len = payload_len;

    if (payload_len > 0u)
    {
        memcpy(packet_out->payload, &parser->frame_buf[4], payload_len);
    }

    return GNU_SOC_PROTO_STATUS_OK;
}

uint16_t gnu_soc_proto_crc16_ccitt_false(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    size_t i;

    if ((data == NULL) && (len > 0u))
    {
        return 0u;
    }

    for (i = 0u; i < len; i++)
    {
        uint8_t bit;
        crc ^= (uint16_t)((uint16_t)data[i] << 8);

        for (bit = 0u; bit < 8u; bit++)
        {
            if ((crc & 0x8000u) != 0u)
            {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

void gnu_soc_proto_parser_init(gnu_soc_proto_parser_t *parser)
{
    if (parser == NULL)
    {
        return;
    }

    memset(parser, 0, sizeof(*parser));
    gnu_soc_proto_parser_reset(parser);
}

gnu_soc_proto_status_t gnu_soc_proto_parser_feed(gnu_soc_proto_parser_t *parser,
                                                  uint8_t ch,
                                                  gnu_soc_proto_packet_t *packet_out)
{
    if (parser == NULL)
    {
        return GNU_SOC_PROTO_STATUS_BAD_ARG;
    }

    switch ((gnu_soc_proto_state_t)parser->state)
    {
    case GNU_SOC_PROTO_STATE_HEAD0:
        if (ch == GNU_SOC_PROTO_HEAD0)
        {
            parser->frame_buf[0] = ch;
            parser->state = (uint8_t)GNU_SOC_PROTO_STATE_HEAD1;
        }
        return GNU_SOC_PROTO_STATUS_NEED_MORE;

    case GNU_SOC_PROTO_STATE_HEAD1:
        if (ch == GNU_SOC_PROTO_HEAD1)
        {
            parser->frame_buf[1] = ch;
            parser->state = (uint8_t)GNU_SOC_PROTO_STATE_LENGTH;
            return GNU_SOC_PROTO_STATUS_NEED_MORE;
        }

        if (ch == GNU_SOC_PROTO_HEAD0)
        {
            parser->frame_buf[0] = ch;
            parser->state = (uint8_t)GNU_SOC_PROTO_STATE_HEAD1;
            return GNU_SOC_PROTO_STATUS_BAD_FRAME;
        }

        gnu_soc_proto_parser_reset(parser);
        return GNU_SOC_PROTO_STATUS_BAD_FRAME;

    case GNU_SOC_PROTO_STATE_LENGTH:
        if ((ch < GNU_SOC_PROTO_LEN_FIELD_MIN) || (ch > GNU_SOC_PROTO_LEN_FIELD_MAX))
        {
            gnu_soc_proto_parser_reset(parser);
            return GNU_SOC_PROTO_STATUS_BAD_FRAME;
        }

        parser->frame_buf[2] = ch;
        parser->len_field = ch;
        parser->body_index = 0u;
        parser->state = (uint8_t)GNU_SOC_PROTO_STATE_BODY_AND_CRC;
        return GNU_SOC_PROTO_STATUS_NEED_MORE;

    case GNU_SOC_PROTO_STATE_BODY_AND_CRC:
    {
        const uint16_t need_len = (uint16_t)parser->len_field + 2u;

        if ((uint16_t)(3u + parser->body_index) >= (uint16_t)GNU_SOC_PROTO_FRAME_MAX_LEN)
        {
            gnu_soc_proto_parser_reset(parser);
            return GNU_SOC_PROTO_STATUS_BAD_FRAME;
        }

        parser->frame_buf[3u + parser->body_index] = ch;
        parser->body_index++;

        if (parser->body_index < need_len)
        {
            return GNU_SOC_PROTO_STATUS_NEED_MORE;
        }

        {
            const uint16_t crc_index = (uint16_t)(3u + parser->len_field);
            const uint16_t calc_crc = gnu_soc_proto_crc16_ccitt_false(&parser->frame_buf[2],
                                                                       (size_t)(1u + parser->len_field));
            const uint16_t recv_crc = (uint16_t)parser->frame_buf[crc_index] |
                                      (uint16_t)((uint16_t)parser->frame_buf[crc_index + 1u] << 8);
            gnu_soc_proto_status_t status;

            if (calc_crc != recv_crc)
            {
                gnu_soc_proto_parser_reset(parser);
                return GNU_SOC_PROTO_STATUS_BAD_CRC;
            }

            status = gnu_soc_proto_packet_from_frame(parser, packet_out);
            gnu_soc_proto_parser_reset(parser);
            return status;
        }
    }

    default:
        gnu_soc_proto_parser_reset(parser);
        return GNU_SOC_PROTO_STATUS_BAD_FRAME;
    }
}

gnu_soc_proto_status_t gnu_soc_proto_build_frame(uint8_t cmd_id,
                                                  const uint8_t *payload,
                                                  uint8_t payload_len,
                                                  uint8_t *out_buf,
                                                  size_t out_buf_cap,
                                                  size_t *out_len)
{
    uint16_t len_field;
    size_t frame_len;
    uint16_t crc;

    if ((out_buf == NULL) || (out_len == NULL))
    {
        return GNU_SOC_PROTO_STATUS_BAD_ARG;
    }

    if ((payload_len > 0u) && (payload == NULL))
    {
        return GNU_SOC_PROTO_STATUS_BAD_ARG;
    }

    if (payload_len > GNU_SOC_PROTO_SERIALIZED_PAYLOAD_MAX_LEN)
    {
        return GNU_SOC_PROTO_STATUS_BAD_ARG;
    }

    len_field = (uint16_t)payload_len + 1u;
    if (len_field > 0xFFu)
    {
        return GNU_SOC_PROTO_STATUS_BAD_ARG;
    }

    frame_len = (size_t)payload_len + 6u;
    if (out_buf_cap < frame_len)
    {
        return GNU_SOC_PROTO_STATUS_NO_SPACE;
    }

    out_buf[0] = GNU_SOC_PROTO_HEAD0;
    out_buf[1] = GNU_SOC_PROTO_HEAD1;
    out_buf[2] = (uint8_t)len_field;
    out_buf[3] = cmd_id;

    if (payload_len > 0u)
    {
        memcpy(&out_buf[4], payload, payload_len);
    }

    crc = gnu_soc_proto_crc16_ccitt_false(&out_buf[2], (size_t)(1u + len_field));
    out_buf[4u + payload_len] = (uint8_t)(crc & 0xFFu);
    out_buf[5u + payload_len] = (uint8_t)((crc >> 8) & 0xFFu);

    *out_len = frame_len;
    return GNU_SOC_PROTO_STATUS_OK;
}

gnu_soc_proto_status_t gnu_soc_proto_build_mcu_report_frame(const gnu_soc_proto_mcu_report_t *report,
                                                             uint8_t *out_buf,
                                                             size_t out_buf_cap,
                                                             size_t *out_len)
{
    if (report == NULL)
    {
        return GNU_SOC_PROTO_STATUS_BAD_ARG;
    }

    return gnu_soc_proto_build_frame((uint8_t)GNU_SOC_PROTO_CMD_REPORT_MCU_STATE,
                                     (const uint8_t *)report,
                                     (uint8_t)sizeof(*report),
                                     out_buf,
                                     out_buf_cap,
                                     out_len);
}
