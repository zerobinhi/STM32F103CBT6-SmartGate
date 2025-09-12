
#include "fm225.h"

// 帧头常量定义
static const uint8_t FRAME_HEADER[2] = {0xEF, 0xAA};

uint8_t TX_BUFFER[TX_BUFF_SIZE] = {0};   // 发送缓冲区
uint8_t RX_BUFFER[RX_BUFF_SIZE] = {0};   // 接收缓冲区
uint8_t user_buffer[RX_BUFF_SIZE] = {0}; // 用户数据缓冲区
uint8_t user_buffer_len = 0;             // 用户数据长度

// 全局常量：合法的人脸录入方向列表（用于参数合法性校验，避免无效值传入）
const uint8_t VALID_FACE_DIRS[] = {
    FACE_DIRECTION_UP,    FACE_DIRECTION_DOWN,   FACE_DIRECTION_LEFT,
    FACE_DIRECTION_RIGHT, FACE_DIRECTION_MIDDLE, FACE_DIRECTION_UNDEFINED};
// 全局常量：合法人脸方向的数量（避免循环中重复计算sizeof）
const size_t VALID_FACE_DIR_CNT =
    sizeof(VALID_FACE_DIRS) / sizeof(VALID_FACE_DIRS[0]);

// ========================== 通用工具函数 ==========================
/**
 * @brief 计算数据帧的BCC校验码（采用异或校验算法，模块通信标准校验方式）
 * @param frame_data 数据帧缓冲区（需非空，且长度不小于MIN_FRAME_LENGTH）
 * @param frame_len
 * 数据帧总长度（单位：字节，需包含帧头、指令、数据、BCC的完整长度）
 * @return uint8_t   计算得到的BCC校验码；若参数无效（空指针/长度不足），返回0
 */
static uint8_t calculate_bcc(const uint8_t *frame_data, uint16_t frame_len) {
  uint8_t bcc = 0; // 异或校验初始值为0（异或运算中性元，不影响初始结果）

  // 参数合法性预检查：空指针或长度不足时直接返回0
  if (frame_data == NULL || frame_len < MIN_FRAME_LENGTH) {
    return 0;
  }

  // 异或校验计算：从BCC_START_INDEX（指令码）开始，到BCC前1字节结束
  for (uint16_t i = BCC_START_INDEX; i < frame_len - 1; ++i) {
    bcc ^= frame_data[i]; // 依次与每个字节进行异或运算，累积结果
  }

  return bcc;
}

/**
 * @brief 校验人脸识别模块接收数据的有效性（验证帧结构完整性和BCC校验码正确性）
 * @param recv_data 接收的数据包缓冲区（需非空，存储模块返回的完整数据帧）
 * @param data_len  实际接收的字节数（需准确反映recv_data中有效数据的长度）
 * @return bool 校验结果：true=数据有效（帧结构+校验码均正确），false=数据无效
 */
bool verify_received_data(const uint8_t *recv_data, uint16_t data_len) {
  // 基础合法性检查：空指针或长度为零，直接判定无效
  if (recv_data == NULL || data_len == 0) {
    return false;
  }

  // 验证帧头：必须与模块约定的FRAME_HEADER完全一致（前2字节）
  if (recv_data[0] != FRAME_HEADER[0] || recv_data[1] != FRAME_HEADER[1]) {
    return false;
  }

  // 验证BCC校验码：接收的BCC（最后1字节）需与计算的BCC一致
  const uint8_t received_bcc = recv_data[data_len - 1]; // 接收的BCC（帧尾）
  const uint8_t calculated_bcc =
      calculate_bcc(recv_data, data_len); // 实时计算的BCC

  if (calculated_bcc != received_bcc) {
    return false;
  }

  // 所有校验项通过，数据有效
  return true;
}

// ========================== 功能函数 ==========================
/**
 * @brief 删除人脸识别模块中存储的所有人脸数据（清空用户库，需谨慎调用）
 * @param 无输入参数（功能固定为删除全部，无需额外配置）
 * @return true: 命令帧构建成功且发送成功；false: 帧构建失败或发送失败
 * @note  调用后模块内所有注册用户数据将被永久删除，且无法恢复
 */
bool face_delete_all() {
  // 构建删除命令帧：初始化全0，避免未赋值字节的随机值影响校验
  uint8_t frame[64] = {0};

  // 填充帧头（2字节）：固定为FRAME_HEADER，模块识别数据帧的起始标识
  frame[0] = FRAME_HEADER[0];
  frame[1] = FRAME_HEADER[1];

  // 填充指令码（1字节）：删除所有人脸命令CMD_DELETE_FACE
  frame[2] = CMD_DELETE_FACE;

  // 填充数据长度（2字节，高字节在前）：删除命令无附加数据，故长度为0x0000
  frame[3] = 0x00; // 数据长度高8位
  frame[4] = 0x00; // 数据长度低8位

  // 计算并填充BCC校验码（1字节，帧尾）：基于整个帧的前5字节计算
  frame[5] = calculate_bcc(frame, 6);

  // 实际的串口发送逻辑
  if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *)frame, sizeof(frame)) !=
      HAL_OK) {
    return false;
  }
  return true;
}
/**
 * @brief 停止命令
 * @param 无输入参数
 * @return true: 命令帧构建成功且发送成功；false: 帧构建失败或发送失败
 * @note  终止当前的操作（录入或验证），需重新调用相应函数启动新操作
 */
bool face_reset() {
  // 构建删除命令帧：初始化全0，避免未赋值字节的随机值影响校验
  uint8_t frame[64] = {0};

  // 填充帧头（2字节）：固定为FRAME_HEADER，模块识别数据帧的起始标识
  frame[0] = FRAME_HEADER[0];
  frame[1] = FRAME_HEADER[1];

  // 填充指令码（1字节）：删除终止操作命令CMD_RESET_FACE
  frame[2] = CMD_RESET_FACE;

  // 填充数据长度（2字节，高字节在前）：删除命令无附加数据，故长度为0x0000
  frame[3] = 0x00; // 数据长度高8位
  frame[4] = 0x00; // 数据长度低8位

  // 计算并填充BCC校验码（1字节，帧尾）：基于整个帧的前5字节计算
  frame[5] = calculate_bcc(frame, 6);

  // 实际的串口发送逻辑
  if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *)frame, sizeof(frame)) !=
      HAL_OK) {
    return false;
  }
  return true;
}
/**
 * @brief 执行人脸验证操作（比对当前采集的人脸与模块内已注册的人脸数据）
 * @param pd_rightaway
 * 解锁成功后是否立刻断电（0x00=不断电，0x01=立刻断电，仅2个有效值）
 * @param timeout
 * 验证人脸超时时间（单位：秒）：0→默认10秒，1~60→实际值，>60→强制60秒
 * @return true: 命令帧构建成功且发送成功；false: 参数无效或发送失败
 * @note  验证超时后，模块将停止采集人脸，需重新调用该函数启动验证
 */
bool face_verify(uint8_t pd_rightaway, uint8_t timeout) {

  // 校验"验证后断电"参数：仅允许0x00（不断电）和0x01（断电），无效则返回失败
  if (pd_rightaway != 0x00 && pd_rightaway != 0x01) {
    return false;
  }

  // 校验并修正超时时
  if (timeout > 60) {
    timeout = 60; // >60→强制60秒（避免超长超时导致模块资源占用）
  }

  // 构建验证命令帧：初始化全0，避免未赋值字节的随机值影响校验
  uint8_t frame[64] = {0};

  // 填充帧头（2字节）：固定起始标识
  frame[0] = FRAME_HEADER[0];
  frame[1] = FRAME_HEADER[1];

  // 填充指令码（1字节）：人脸验证命令CMD_VERIFY_FACE
  frame[2] = CMD_VERIFY_FACE;

  // 填充数据长度（2字节，高字节在前）：附加数据共2字节（pd_rightaway +
  // timeout）
  frame[3] = 0x00; // 数据长度高8位
  frame[4] = 0x02; // 数据长度低8位（0x0002 = 2字节）

  // 填充附加数据（2字节）：按模块协议顺序排列
  frame[5] = pd_rightaway; // 第1字节：验证后断电标志
  frame[6] = timeout;      // 第2字节：验证超时时间

  // 计算并填充BCC校验码（1字节，帧尾）：基于前7字节计算
  frame[7] = calculate_bcc(frame, 8);

  // 实际串口发送逻辑
  if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *)frame, sizeof(frame)) !=
      HAL_OK) {
    return false;
  }

  return true;
}

/**
 * @brief 向人脸识别模块注册新用户人脸（录入用户信息和人脸特征数据）
 * @param admin 是否为管理员注册（0x00=普通用户，0x01=管理员，仅2个有效值）
 * @param user_name
 * 录入用户姓名（固定32字节，建议以'\0'结尾，不足32字节自动补0）
 * @param face_direction
 * 录入方向（需为FACE_DIRECTION_*宏定义值，无效值将返回失败）
 * @param enroll_type     注册类型（0x00=交互录入，0x01=单帧录入，仅2个有效值）
 * @param enable_duplicate
 * 是否允许重复录入（0x00=禁止重复，0x01=允许重复，仅2个有效值）
 * @param timeout
 * 录入超时时间（单位：秒）：0→默认10秒，1~60→实际值，>60→强制60秒
 * @return true: 命令帧构建成功且发送成功；false: 参数无效或发送失败
 * @note  管理员用户拥有删除其他用户的权限，普通用户仅拥有验证权限
 */
bool face_enroll(uint8_t admin, const uint8_t user_name[32],
                 uint8_t face_direction, uint8_t enroll_type,
                 uint8_t enable_duplicate, uint8_t timeout) {
  // 逐项校验输入参数合法性（避免无效值导致模块异常）
  // 校验管理员标识：仅允许0x00（普通）和0x01（管理员）
  if (admin != 0x00 && admin != 0x01) {
    return false;
  }

  // 校验用户名指针：不允许为空（需指向有效的32字节缓冲区）
  if (user_name == NULL) {
    return false;
  }

  // 校验人脸录入方向：必须在VALID_FACE_DIRS列表中（避免无效方向）
  bool is_dir_valid = false;
  for (size_t i = 0; i < VALID_FACE_DIR_CNT; ++i) {
    if (face_direction == VALID_FACE_DIRS[i]) {
      is_dir_valid = true;
      break;
    }
  }
  if (!is_dir_valid) {
    return false;
  }

  // 校验注册类型：仅允许0x00（交互）和0x01（单帧）
  if (enroll_type != 0x00 && enroll_type != 0x01) {
    return false;
  }

  // 校验重复录入控制：仅允许0x00（禁止）和0x01（允许）
  if (enable_duplicate != 0x00 && enable_duplicate != 0x01) {
    return false;
  }

  // 校验并修正超时时间
  if (timeout > 60) {
    timeout = 60; // >60→强制60秒
  }

  // 清理用户名（确保严格符合32字节长度，不足补0，不修改原输入）
  uint8_t clean_name[32] = {0}; // 临时缓冲区，默认全0（自动补0）
  // 安全拷贝32字节（无论原用户名是否以'\0'结尾，均不会越界）
  memcpy(clean_name, user_name, 32);

  // 调试模式：打印最终生效的注册参数（便于确认配置）

  // 构建注册命令帧：初始化全0，避免未赋值字节的随机值影响校验
  uint8_t frame[64] = {0};

  // 填充帧头（2字节）：固定起始标识
  frame[0] = FRAME_HEADER[0];
  frame[1] = FRAME_HEADER[1];

  // 填充指令码（1字节）：人脸注册命令CMD_ENROLL_ITG
  frame[2] = CMD_ENROLL_ITG;

  // 填充数据长度（2字节，高字节在前）：附加数据共40字节
  frame[3] = 0x00; // 数据长度高8位
  frame[4] = 0x28; // 数据长度低8位（0x0028 = 40字节）

  // 填充附加数据（40字节，严格按模块协议顺序排列）
  frame[5] = admin;                  // 第1字节：管理员标识
  memcpy(&frame[6], clean_name, 32); // 第2-33字节：32字节用户名（索引6-37）
  frame[38] = face_direction;        // 第34字节：人脸录入方向（索引38）
  frame[39] = enroll_type;           // 第35字节：注册类型（索引39）
  frame[40] = enable_duplicate;      // 第36字节：重复录入控制（索引40）
  frame[41] = timeout;               // 第37字节：录入超时时间（索引41）
  // 第38-40字节：预留位（3字节，索引42-44），已初始化为0，无需额外赋值

  // 计算并填充BCC校验码（1字节，帧尾）：基于前45字节计算
  frame[45] = calculate_bcc(frame, 46);

  // 实际串口发送逻辑
  if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *)frame, 46) != HAL_OK) {
    return false;
  }

  return true;
}
