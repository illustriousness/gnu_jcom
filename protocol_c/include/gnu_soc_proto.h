#ifndef GNU_SOC_PROTO_H_
#define GNU_SOC_PROTO_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GNU_SOC_PROTO_HEAD0 0xAAu
#define GNU_SOC_PROTO_HEAD1 0x55u

#define GNU_SOC_PROTO_LED_MODE_MASK 0x7Fu
#define GNU_SOC_PROTO_LED_ID_MASK   0x80u

#define GNU_SOC_PROTO_SYS_FLAG_READY              (1u << 0)
#define GNU_SOC_PROTO_SYS_FLAG_RUNNING            (1u << 1)
#define GNU_SOC_PROTO_SYS_FLAG_CHARGING           (1u << 2)
#define GNU_SOC_PROTO_SYS_FLAG_SLEEPING           (1u << 3)
#define GNU_SOC_PROTO_SYS_FLAG_FAULT_EXIST        (1u << 4)
#define GNU_SOC_PROTO_SYS_FLAG_MOTOR_FAULT_EXIST  (1u << 5)
#define GNU_SOC_PROTO_SYS_FLAG_TIME_VALID         (1u << 6)
#define GNU_SOC_PROTO_SYS_FLAG_REMOTE_CTL_TIMEOUT (1u << 7)
#define GNU_SOC_PROTO_SYS_FLAG_CUTTER_ENABLED     (1u << 8)
#define GNU_SOC_PROTO_SYS_FLAG_LIFT_ENABLED       (1u << 9)
#define GNU_SOC_PROTO_SYS_FLAG_FILE_TX_REQ        (1u << 10)

#define GNU_SOC_PROTO_SENSOR_FLAG_BUMPER_LEFT  (1u << 0)
#define GNU_SOC_PROTO_SENSOR_FLAG_ESTOP        (1u << 1)
#define GNU_SOC_PROTO_SENSOR_FLAG_BUMPER_RIGHT (1u << 2)
#define GNU_SOC_PROTO_SENSOR_FLAG_LIFT_LEFT    (1u << 3)
#define GNU_SOC_PROTO_SENSOR_FLAG_LIFT_RIGHT   (1u << 4)
#define GNU_SOC_PROTO_SENSOR_FLAG_CUTTER_LIMIT (1u << 5)
#define GNU_SOC_PROTO_SENSOR_FLAG_DOCK_CONTACT (1u << 6)
#define GNU_SOC_PROTO_SENSOR_FLAG_RAIN         (1u << 7)

#define GNU_SOC_PROTO_MODULE_FAULT_FLAG_BATTERY_PACK (1u << 0)
#define GNU_SOC_PROTO_MODULE_FAULT_FLAG_MOTOR_LEFT   (1u << 1)
#define GNU_SOC_PROTO_MODULE_FAULT_FLAG_MOTOR_RIGHT  (1u << 2)
#define GNU_SOC_PROTO_MODULE_FAULT_FLAG_MOTOR_CUT    (1u << 3)
#define GNU_SOC_PROTO_MODULE_FAULT_FLAG_MOTOR_LIFT   (1u << 4)
#define GNU_SOC_PROTO_MODULE_FAULT_FLAG_ESTOP        (1u << 5)

#define GNU_SOC_PROTO_KEY_EVENT_FLAG_POWER_OFF (1u << 0)
#define GNU_SOC_PROTO_KEY_EVENT_FLAG_DOCK      (1u << 1)
#define GNU_SOC_PROTO_KEY_EVENT_FLAG_ESTOP     (1u << 2)
#define GNU_SOC_PROTO_KEY_EVENT_FLAG_START     (1u << 3)
#define GNU_SOC_PROTO_KEY_EVENT_FLAG_UNLOCK    (1u << 4)
#define GNU_SOC_PROTO_KEY_EVENT_FLAG_NETCFG    (1u << 5)

typedef enum
{
    GNU_SOC_PROTO_CMD_REPORT_MCU_STATE = 0x01,
    GNU_SOC_PROTO_CMD_REPORT_SOC_CTRL = 0x80,
    GNU_SOC_PROTO_CMD_CTRL_MOTOR = 0x81,
    GNU_SOC_PROTO_CMD_CTRL_LED = 0x82,
    GNU_SOC_PROTO_CMD_SET_TIME = 0x83,
    GNU_SOC_PROTO_CMD_CTRL_POWER = 0x84,
    GNU_SOC_PROTO_CMD_CTRL_BUZZER = 0x85,
    GNU_SOC_PROTO_CMD_CTRL_FILE_TRANSFER = 0x86,
    GNU_SOC_PROTO_CMD_ACTIVATE_TERMINAL = 0xFF,
} gnu_soc_proto_cmd_t;

typedef enum
{
    GNU_SOC_PROTO_LED_BLINK = 0,
    GNU_SOC_PROTO_LED_WATERFALL = 1,
    GNU_SOC_PROTO_LED_BREATHE = 2,
} gnu_soc_proto_led_mode_t;

typedef enum
{
    GNU_SOC_PROTO_LED_ID_STRIP = 0,
    GNU_SOC_PROTO_LED_ID_LED0 = 1,
} gnu_soc_proto_led_id_t;

typedef enum
{
    GNU_SOC_PROTO_POWER_ACTION_SHUTDOWN = 0,
    GNU_SOC_PROTO_POWER_ACTION_SLEEP = 1,
} gnu_soc_proto_power_action_t;

typedef enum
{
    GNU_SOC_PROTO_FILE_TRANSFER_SOC_TO_MCU = 0,
    GNU_SOC_PROTO_FILE_TRANSFER_MCU_TO_SOC = 1,
} gnu_soc_proto_file_transfer_direction_t;

typedef enum
{
    GNU_SOC_PROTO_STATUS_OK = 0,
    GNU_SOC_PROTO_STATUS_NEED_MORE = 1,
    GNU_SOC_PROTO_STATUS_BAD_ARG = -1,
    GNU_SOC_PROTO_STATUS_BAD_FRAME = -2,
    GNU_SOC_PROTO_STATUS_BAD_CRC = -3,
    GNU_SOC_PROTO_STATUS_NO_SPACE = -4,
} gnu_soc_proto_status_t;

#pragma pack(push, 1)

typedef struct
{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t yaw;
    int16_t pitch;
    int16_t roll;
    int16_t temperature;
} gnu_soc_proto_imu_report_t;

typedef struct
{
    int16_t speed;
    uint16_t current;
    int32_t encoder;
    uint16_t fault_code;
    uint16_t reserve;
} gnu_soc_proto_single_motor_report_t;

typedef struct
{
    uint8_t soc;
    uint8_t reserve;
    int16_t temperature;
} gnu_soc_proto_battery_report_t;

typedef struct
{
    uint16_t system_flags;
    uint16_t sensor_flags;
    uint16_t module_fault_flags;
    uint16_t key_event_flags;
} gnu_soc_proto_system_report_t;

typedef struct
{
    int16_t v_real;
    int16_t w_real;
    gnu_soc_proto_imu_report_t imu;
    gnu_soc_proto_single_motor_report_t left;
    gnu_soc_proto_single_motor_report_t right;
    gnu_soc_proto_single_motor_report_t cut;
    gnu_soc_proto_battery_report_t battery;
    gnu_soc_proto_system_report_t system;
} gnu_soc_proto_mcu_report_t;

typedef struct
{
    int16_t v_ctl;
    int16_t w_ctl;
} gnu_soc_proto_soc_ctrl_t;

typedef struct
{
    uint8_t motor_id;
    int32_t payload;
} gnu_soc_proto_motor_cfg_t;

typedef struct
{
    uint8_t mode_and_id;
    uint8_t color_r;
    uint8_t color_g;
    uint8_t color_b;
    uint16_t interval;
} gnu_soc_proto_led_cfg_t;

typedef struct
{
    uint8_t buzzer_id;
    uint8_t repeat_cnt;
    uint16_t on_time_ms;
    uint16_t off_time_ms;
} gnu_soc_proto_buzzer_ctrl_t;

typedef struct
{
    uint32_t timestamp;
} gnu_soc_proto_time_cfg_t;

typedef struct
{
    uint8_t action;
} gnu_soc_proto_power_cfg_t;

typedef struct
{
    uint8_t direction;
    uint8_t file_type;
} gnu_soc_proto_file_transfer_ctrl_t;

#pragma pack(pop)

typedef union
{
    gnu_soc_proto_soc_ctrl_t soc_ctrl;
    gnu_soc_proto_motor_cfg_t motor_cfg;
    gnu_soc_proto_led_cfg_t led_cfg;
    gnu_soc_proto_time_cfg_t time_cfg;
    gnu_soc_proto_power_cfg_t power_cfg;
    gnu_soc_proto_buzzer_ctrl_t buzzer_ctrl;
    gnu_soc_proto_file_transfer_ctrl_t file_transfer_ctrl;
} gnu_soc_proto_soc_cmd_payload_u;

enum
{
    GNU_SOC_PROTO_SOC_CMD_PAYLOAD_MAX_LEN = (int)sizeof(gnu_soc_proto_soc_cmd_payload_u),
    GNU_SOC_PROTO_MCU_REPORT_PAYLOAD_LEN = (int)sizeof(gnu_soc_proto_mcu_report_t),
    GNU_SOC_PROTO_PAYLOAD_MAX_LEN = (GNU_SOC_PROTO_SOC_CMD_PAYLOAD_MAX_LEN > GNU_SOC_PROTO_MCU_REPORT_PAYLOAD_LEN)
                                        ? GNU_SOC_PROTO_SOC_CMD_PAYLOAD_MAX_LEN
                                        : GNU_SOC_PROTO_MCU_REPORT_PAYLOAD_LEN,
    GNU_SOC_PROTO_FRAME_MAX_LEN = GNU_SOC_PROTO_PAYLOAD_MAX_LEN + 6,
};

typedef struct
{
    uint8_t cmd_id;
    uint8_t payload_len;
    uint8_t payload[GNU_SOC_PROTO_PAYLOAD_MAX_LEN];
} gnu_soc_proto_packet_t;

typedef struct
{
    uint8_t state;
    uint8_t len_field;
    uint16_t body_index;
    uint8_t frame_buf[GNU_SOC_PROTO_FRAME_MAX_LEN];
} gnu_soc_proto_parser_t;

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define GNU_SOC_PROTO_STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)
#elif defined(__cplusplus) && (__cplusplus >= 201103L)
#define GNU_SOC_PROTO_STATIC_ASSERT(cond, msg) static_assert((cond), msg)
#else
#define GNU_SOC_PROTO_STATIC_ASSERT(cond, msg)
#endif

GNU_SOC_PROTO_STATIC_ASSERT(sizeof(gnu_soc_proto_imu_report_t) == 20, "imu report size mismatch");
GNU_SOC_PROTO_STATIC_ASSERT(sizeof(gnu_soc_proto_single_motor_report_t) == 12, "single motor report size mismatch");
GNU_SOC_PROTO_STATIC_ASSERT(sizeof(gnu_soc_proto_battery_report_t) == 4, "battery report size mismatch");
GNU_SOC_PROTO_STATIC_ASSERT(sizeof(gnu_soc_proto_system_report_t) == 8, "system report size mismatch");
GNU_SOC_PROTO_STATIC_ASSERT(sizeof(gnu_soc_proto_mcu_report_t) == 72, "mcu report size mismatch");
GNU_SOC_PROTO_STATIC_ASSERT(sizeof(gnu_soc_proto_soc_ctrl_t) == 4, "soc ctrl size mismatch");
GNU_SOC_PROTO_STATIC_ASSERT(sizeof(gnu_soc_proto_motor_cfg_t) == 5, "motor cfg size mismatch");
GNU_SOC_PROTO_STATIC_ASSERT(sizeof(gnu_soc_proto_led_cfg_t) == 6, "led cfg size mismatch");
GNU_SOC_PROTO_STATIC_ASSERT(sizeof(gnu_soc_proto_buzzer_ctrl_t) == 6, "buzzer ctrl size mismatch");
GNU_SOC_PROTO_STATIC_ASSERT(sizeof(gnu_soc_proto_time_cfg_t) == 4, "time cfg size mismatch");
GNU_SOC_PROTO_STATIC_ASSERT(sizeof(gnu_soc_proto_power_cfg_t) == 1, "power cfg size mismatch");
GNU_SOC_PROTO_STATIC_ASSERT(sizeof(gnu_soc_proto_file_transfer_ctrl_t) == 2, "file transfer cfg size mismatch");

static inline uint8_t gnu_soc_proto_led_make_mode_and_id(uint8_t mode, uint8_t id)
{
    return (uint8_t)((mode & GNU_SOC_PROTO_LED_MODE_MASK) | ((id != 0u) ? GNU_SOC_PROTO_LED_ID_MASK : 0u));
}

static inline uint8_t gnu_soc_proto_led_extract_mode(uint8_t mode_and_id)
{
    return (uint8_t)(mode_and_id & GNU_SOC_PROTO_LED_MODE_MASK);
}

static inline uint8_t gnu_soc_proto_led_extract_id(uint8_t mode_and_id)
{
    return (uint8_t)((mode_and_id & GNU_SOC_PROTO_LED_ID_MASK) != 0u);
}

uint16_t gnu_soc_proto_crc16_ccitt_false(const uint8_t *data, size_t len);

void gnu_soc_proto_parser_init(gnu_soc_proto_parser_t *parser);

gnu_soc_proto_status_t gnu_soc_proto_parser_feed(gnu_soc_proto_parser_t *parser,
                                                  uint8_t ch,
                                                  gnu_soc_proto_packet_t *packet_out);

gnu_soc_proto_status_t gnu_soc_proto_build_frame(uint8_t cmd_id,
                                                  const uint8_t *payload,
                                                  uint8_t payload_len,
                                                  uint8_t *out_buf,
                                                  size_t out_buf_cap,
                                                  size_t *out_len);

gnu_soc_proto_status_t gnu_soc_proto_build_mcu_report_frame(const gnu_soc_proto_mcu_report_t *report,
                                                             uint8_t *out_buf,
                                                             size_t out_buf_cap,
                                                             size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* GNU_SOC_PROTO_H_ */
