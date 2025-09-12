#ifndef FM225_H_
#define FM225_H_

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <string.h>

// 宏定义常量
#define BCC_START_INDEX 2 // 校验码计算起始索引（跳过帧头2字节，从指令码开始）
#define MIN_FRAME_LENGTH 6 // 最小帧长度（字节）：帧头2字节 + 指令1字节 + 数据长度2字节 + BCC1字节

// 录入方向定义（覆盖所有支持的人脸朝向，未定义时默认正向）
#define FACE_DIRECTION_UP 0x10        // 录入朝上的人脸
#define FACE_DIRECTION_DOWN 0x08      // 录入朝下的人脸
#define FACE_DIRECTION_LEFT 0x04      // 录入朝左的人脸
#define FACE_DIRECTION_RIGHT 0x02     // 录入朝右的人脸
#define FACE_DIRECTION_MIDDLE 0x01    // 录入正向的人脸
#define FACE_DIRECTION_UNDEFINED 0x00 // 未定义方向，默认按正向处理

// 命令定义
#define CMD_ENROLL_ITG 0x26  // 注册人脸命令
#define CMD_DELETE_FACE 0x21 // 删除所有人脸命令
#define CMD_VERIFY_FACE 0x12 // 验证人脸命令
#define CMD_RESET_FACE 0x10  // 终止操作命令

#define RX_BUFF_SIZE 128
#define TX_BUFF_SIZE 64

extern uint8_t TX_BUFFER[TX_BUFF_SIZE];
extern uint8_t RX_BUFFER[RX_BUFF_SIZE];
extern UART_HandleTypeDef huart1;

// 函数声明
bool verify_received_data(const uint8_t *recv_data, uint16_t data_len);
bool face_enroll(uint8_t admin, const uint8_t user_name[32],
                 uint8_t face_direction, uint8_t enroll_type,
                 uint8_t enable_duplicate, uint8_t timeout);
bool face_delete_all();
bool face_verify(uint8_t pd_rightaway, uint8_t timeout);
bool face_reset();

#endif /* FM225_H_ */
