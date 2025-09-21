#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

 #define DEBUG

// 宏定义常量
#define BCC_START_INDEX 2  // 校验码计算起始索引（跳过帧头2字节，从指令码开始）
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
#define CMD_DELETE_USER 0x20  // 删除指定用户命令

// 帧头常量定义
static const uint8_t FRAME_HEADER[2] = {0xEF, 0xAA};

// 全局常量：合法的人脸录入方向列表（用于参数合法性校验，避免无效值传入）
const uint8_t VALID_FACE_DIRS[] = {
    FACE_DIRECTION_UP,
    FACE_DIRECTION_DOWN,
    FACE_DIRECTION_LEFT,
    FACE_DIRECTION_RIGHT,
    FACE_DIRECTION_MIDDLE,
    FACE_DIRECTION_UNDEFINED};
// 全局常量：合法人脸方向的数量（避免循环中重复计算sizeof）
const size_t VALID_FACE_DIR_CNT = sizeof(VALID_FACE_DIRS) / sizeof(VALID_FACE_DIRS[0]);

// ========================== 通用工具函数 ==========================
/**
 * @brief 计算数据帧的BCC校验码（采用异或校验算法，模块通信标准校验方式）
 * @param frame_data 数据帧缓冲区（需非空，且长度不小于MIN_FRAME_LENGTH）
 * @param frame_len  数据帧总长度（单位：字节，需包含帧头、指令、数据、BCC的完整长度）
 * @return uint8_t   计算得到的BCC校验码；若参数无效（空指针/长度不足），返回0
 */
static uint8_t calculate_bcc(const uint8_t *frame_data, uint16_t frame_len)
{
    uint8_t bcc = 0; // 异或校验初始值为0（异或运算中性元，不影响初始结果）

    // 参数合法性预检查：空指针或长度不足时直接返回0
    if (frame_data == NULL || frame_len < MIN_FRAME_LENGTH)
    {
#ifdef DEBUG
        printf("BCC计算失败：数据帧为空或长度不足（最小需%d字节）\n", MIN_FRAME_LENGTH);
#endif
        return 0;
    }

    // 异或校验计算：从BCC_START_INDEX（指令码）开始，到BCC前1字节结束
    for (uint16_t i = BCC_START_INDEX; i < frame_len - 1; ++i)
    {
        bcc ^= frame_data[i]; // 依次与每个字节进行异或运算，累积结果
    }

    return bcc;
}

/**
 * @brief 校验人脸识别模块接收数据的有效性（验证帧结构完整性和BCC校验码正确性）
 * @param recv_data 接收的数据包缓冲区（需非空，存储模块返回的完整数据帧）
 * @param data_len  实际接收的字节数（需准确反映recv_data中有效数据的长度）
 * @return bool     校验结果：true=数据有效（帧结构+校验码均正确），false=数据无效
 */
static bool verify_received_data(const uint8_t* recv_data, uint16_t data_len)
{
    // 基础合法性检查：空指针或长度为零，直接判定无效
    if (recv_data == NULL|| data_len == 0)
    {
#ifdef DEBUG
        printf("数据校验失败：数据为空\n");
#endif
        return false;
    }

    // 验证帧头：必须与模块约定的FRAME_HEADER完全一致（前2字节）
    if (recv_data[0] != FRAME_HEADER[0] || recv_data[1] != FRAME_HEADER[1])
    {
#ifdef DEBUG
        printf("数据校验失败：帧头不匹配（期望%02X%02X，实际%02X%02X）\n",
               FRAME_HEADER[0], FRAME_HEADER[1], recv_data[0], recv_data[1]);
#endif
        return false;
    }

    // 验证BCC校验码：接收的BCC（最后1字节）需与计算的BCC一致
    const uint8_t received_bcc = recv_data[data_len - 1];              // 接收的BCC（帧尾）
    const uint8_t calculated_bcc = calculate_bcc(recv_data, data_len); // 实时计算的BCC

    if (calculated_bcc != received_bcc)
    {
#ifdef DEBUG
        printf("数据校验失败：BCC不匹配（期望0x%02X，实际0x%02X）\n",
               calculated_bcc, received_bcc);
#endif
        return false;
    }

    // 所有校验项通过，数据有效
#ifdef DEBUG
    printf("数据校验成功：接收数据结构完整且校验码正确\n");
#endif
    return true;
}

// ========================== 功能函数 ==========================
/**
 * @brief 删除指定用户的人脸数据
 * @param 待删除用户的ID
 * @return true: 命令帧构建成功且发送成功；false: 帧构建失败或发送失败
 */
static bool face_delete_user(uint16_t id)
{ 
	// 验证ID的合法性：ID范围为1~100
    if (id < 1 || id > 100)
	{
        #ifdef DEBUG
		printf("删除失败：用户ID无效（需1~100，实际%d）\n", id);
        #endif
        return false;
	}

    // 构建删除命令帧：初始化全0，避免未赋值字节的随机值影响校验
    uint8_t frame[8] = { 0 };

    // 填充帧头（2字节）：固定为FRAME_HEADER，模块识别数据帧的起始标识
    frame[0] = FRAME_HEADER[0];
    frame[1] = FRAME_HEADER[1];

    // 填充指令码（1字节）：删除指定用户命令CMD_DELETE_USER
    frame[2] = CMD_DELETE_USER;

    // 填充数据长度（2字节，高字节在前）
    frame[3] = 0x00; // 数据长度高8位
    frame[4] = 0x02; // 数据长度低8位

	// 填充附加数据（2字节）：按模块协议顺序排列
	frame[5] = (id >> 8) & 0xFF; // 第1字节：用户ID高8位
	frame[6] = id & 0xFF;        // 第2字节：用户ID低8位

    // 计算并填充BCC校验码（1字节，帧尾）：基于整个帧的前5字节计算
    frame[7] = calculate_bcc(frame, sizeof(frame));

    // 调试模式：打印发送的命令帧详情（便于排查帧结构是否正确）
#ifdef DEBUG
    printf("发送删除指定脸帧（共%d字节）：", (uint16_t)sizeof(frame));
    for (size_t i = 0; i < sizeof(frame); ++i)
    {
        printf("%02X ", frame[i]);
    }
    printf("\n");
#endif

    // 实际串口发送逻辑：需根据硬件平台实现UART_Send函数
    // 示例：
    // if (!UART_Send(frame, sizeof(frame)))
    // {
    //     printf("删除失败：串口发送命令帧失败\n");
    //     return false;
    // }

    // 调试模式下，帧构建成功即返回true（实际需结合串口发送结果）
    return true;
}

/**
 * @brief 删除人脸识别模块中存储的所有人脸数据（清空用户库，需谨慎调用）
 * @param 无输入参数（功能固定为删除全部，无需额外配置）
 * @return true: 命令帧构建成功且发送成功；false: 帧构建失败或发送失败
 * @note  调用后模块内所有注册用户数据将被永久删除，且无法恢复
 */
static bool face_delete_all()
{
    // 构建删除命令帧：初始化全0，避免未赋值字节的随机值影响校验
    uint8_t frame[6] = {0};

    // 填充帧头（2字节）：固定为FRAME_HEADER，模块识别数据帧的起始标识
    frame[0] = FRAME_HEADER[0];
    frame[1] = FRAME_HEADER[1];

    // 填充指令码（1字节）：删除所有人脸命令CMD_DELETE_FACE
    frame[2] = CMD_DELETE_FACE;

    // 填充数据长度（2字节，高字节在前）：删除命令无附加数据，故长度为0x0000
    frame[3] = 0x00; // 数据长度高8位
    frame[4] = 0x00; // 数据长度低8位

    // 计算并填充BCC校验码（1字节，帧尾）：基于整个帧的前5字节计算
    frame[5] = calculate_bcc(frame, sizeof(frame));

    // 调试模式：打印发送的命令帧详情（便于排查帧结构是否正确）
#ifdef DEBUG
    printf("发送删除所有人脸帧（共%d字节）：", (uint16_t)sizeof(frame));
    for (size_t i = 0; i < sizeof(frame); ++i)
    {
        printf("%02X ", frame[i]);
    }
    printf("\n");
#endif

    // 实际串口发送逻辑：需根据硬件平台实现UART_Send函数
    // 示例：
    // if (!UART_Send(frame, sizeof(frame)))
    // {
    //     printf("删除失败：串口发送命令帧失败\n");
    //     return false;
    // }

    // 调试模式下，帧构建成功即返回true（实际需结合串口发送结果）
    return true;
}
/**
 * @brief 停止命令
 * @param 无输入参数
 * @return true: 命令帧构建成功且发送成功；false: 帧构建失败或发送失败
 * @note  终止当前的操作（录入或验证），需重新调用相应函数启动新操作
 */
static bool face_reset()
{
    // 构建删除命令帧：初始化全0，避免未赋值字节的随机值影响校验
    uint8_t frame[6] = { 0 };

    // 填充帧头（2字节）：固定为FRAME_HEADER，模块识别数据帧的起始标识
    frame[0] = FRAME_HEADER[0];
    frame[1] = FRAME_HEADER[1];

    // 填充指令码（1字节）：删除终止操作命令CMD_RESET_FACE
    frame[2] = CMD_RESET_FACE;

    // 填充数据长度（2字节，高字节在前）：删除命令无附加数据，故长度为0x0000
    frame[3] = 0x00; // 数据长度高8位
    frame[4] = 0x00; // 数据长度低8位

    // 计算并填充BCC校验码（1字节，帧尾）：基于整个帧的前5字节计算
    frame[5] = calculate_bcc(frame, sizeof(frame));

    // 调试模式：打印发送的命令帧详情（便于排查帧结构是否正确）
#ifdef DEBUG
    printf("发送终止操作帧（共%d字节）：", (uint16_t)sizeof(frame));
    for (size_t i = 0; i < sizeof(frame); ++i)
    {
        printf("%02X ", frame[i]);
    }
    printf("\n");
#endif

    // 实际串口发送逻辑：需根据硬件平台实现UART_Send函数
    // 示例：
    // if (!UART_Send(frame, sizeof(frame)))
    // {
    //     printf("删除失败：串口发送命令帧失败\n");
    //     return false;
    // }

    // 调试模式下，帧构建成功即返回true（实际需结合串口发送结果）
    return true;
}
/**
 * @brief 执行人脸验证操作（比对当前采集的人脸与模块内已注册的人脸数据）
 * @param pd_rightaway 解锁成功后是否立刻断电（0x00=不断电，0x01=立刻断电，仅2个有效值）
 * @param timeout      验证人脸超时时间（单位：秒）：0→默认10秒，1~60→实际值，>60→强制60秒
 * @return true: 命令帧构建成功且发送成功；false: 参数无效或发送失败
 * @note  验证超时后，模块将停止采集人脸，需重新调用该函数启动验证
 */
static bool face_verify(uint8_t pd_rightaway, uint8_t timeout)
{
    // 校验"验证后断电"参数：仅允许0x00（不断电）和0x01（断电），无效则返回失败
    if (pd_rightaway != 0x00 && pd_rightaway != 0x01)
    {
#ifdef DEBUG
        printf("验证失败：'验证后断电'参数无效（需0x00/0x01，实际0x%02X）\n", pd_rightaway);
#endif
        return false;
    }

    // 校验并修正超时时
    if (timeout > 60)
    {
        timeout = 60; // >60→强制60秒（避免超长超时导致模块资源占用）
#ifdef DEBUG
        printf("验证参数修正：超时时间过长，强制调整为60秒\n");
#endif
    }

    // 调试模式：打印最终生效的验证参数（便于确认配置是否正确）
#ifdef DEBUG
    printf("验证参数：验证后立刻断电=%s, 验证超时时间=%d秒\n", (pd_rightaway == 0x01 ? "是" : "否"), timeout);
#endif

    // 构建验证命令帧：初始化全0，避免随机值影响
    uint8_t frame[8] = {0};

    // 填充帧头（2字节）：固定起始标识
    frame[0] = FRAME_HEADER[0];
    frame[1] = FRAME_HEADER[1];

    // 填充指令码（1字节）：人脸验证命令CMD_VERIFY_FACE
    frame[2] = CMD_VERIFY_FACE;

    // 填充数据长度（2字节，高字节在前）：附加数据共2字节（pd_rightaway + timeout）
    frame[3] = 0x00; // 数据长度高8位
    frame[4] = 0x02; // 数据长度低8位（0x0002 = 2字节）

    // 填充附加数据（2字节）：按模块协议顺序排列
    frame[5] = pd_rightaway; // 第1字节：验证后断电标志
    frame[6] = timeout;      // 第2字节：验证超时时间

    // 计算并填充BCC校验码（1字节，帧尾）：基于前7字节计算
    frame[7] = calculate_bcc(frame, sizeof(frame));

    // 调试模式：打印发送的命令帧详情
#ifdef DEBUG
    printf("发送人脸验证帧（共%d字节）：", (uint16_t)sizeof(frame));
    for (size_t i = 0; i < sizeof(frame); ++i)
    {
        printf("%02X ", frame[i]);
    }
    printf("\n");
#endif

    // 实际串口发送逻辑：需根据硬件平台实现UART_Send函数
    // 示例：
    // if (!UART_Send(frame, sizeof(frame)))
    // {
    //     printf("验证失败：串口发送命令帧失败\n");
    //     return false;
    // }

    // 调试模式下，帧构建成功即返回true（实际需结合串口发送结果）
    return true;
}

/**
 * @brief 向人脸识别模块注册新用户人脸（录入用户信息和人脸特征数据）
 * @param admin           是否为管理员注册（0x00=普通用户，0x01=管理员，仅2个有效值）
 * @param user_name       录入用户姓名（固定32字节，建议以'\0'结尾，不足32字节自动补0）
 * @param face_direction  录入方向（需为FACE_DIRECTION_*宏定义值，无效值将返回失败）
 * @param enroll_type     注册类型（0x00=交互录入，0x01=单帧录入，仅2个有效值）
 * @param enable_duplicate 是否允许重复录入（0x00=禁止重复，0x01=允许重复，仅2个有效值）
 * @param timeout         录入超时时间（单位：秒）：0→默认10秒，1~60→实际值，>60→强制60秒
 * @return true: 命令帧构建成功且发送成功；false: 参数无效或发送失败
 */
static bool face_enroll(uint8_t admin, const uint8_t user_name[32], uint8_t face_direction,
                        uint8_t enroll_type, uint8_t enable_duplicate, uint8_t timeout)
{
    // 逐项校验输入参数合法性（避免无效值导致模块异常）
    // 校验管理员标识：仅允许0x00（普通）和0x01（管理员）
    if (admin != 0x00 && admin != 0x01)
    {
#ifdef DEBUG
        printf("注册失败：'管理员标识'参数无效（需0x00/0x01，实际0x%02X）\n", admin);
#endif
        return false;
    }

    // 校验用户名指针：不允许为空（需指向有效的32字节缓冲区）
    if (user_name == NULL)
    {
#ifdef DEBUG
        printf("注册失败：用户名字符串指针为空\n");
#endif
        return false;
    }

    // 校验人脸录入方向：必须在VALID_FACE_DIRS列表中（避免无效方向）
    bool is_dir_valid = false;
    for (size_t i = 0; i < VALID_FACE_DIR_CNT; ++i)
    {
        if (face_direction == VALID_FACE_DIRS[i])
        {
            is_dir_valid = true;
            break;
        }
    }
    if (!is_dir_valid)
    {
#ifdef DEBUG
        printf("注册失败：'人脸录入方向'参数无效（实际0x%02X）\n", face_direction);
#endif
        return false;
    }

    // 校验注册类型：仅允许0x00（交互）和0x01（单帧）
    if (enroll_type != 0x00 && enroll_type != 0x01)
    {
#ifdef DEBUG
        printf("注册失败：'注册类型'参数无效（需0x00/0x01，实际0x%02X）\n", enroll_type);
#endif
        return false;
    }

    // 校验重复录入控制：仅允许0x00（禁止）和0x01（允许）
    if (enable_duplicate != 0x00 && enable_duplicate != 0x01)
    {
#ifdef DEBUG
        printf("注册失败：'重复录入控制'参数无效（需0x00/0x01，实际0x%02X）\n", enable_duplicate);
#endif
        return false;
    }

    // 校验并修正超时时间
    if (timeout > 60)
    {
        timeout = 60; // >60→强制60秒
#ifdef DEBUG
        printf("注册参数修正：超时时间过长，强制调整为60秒\n");
#endif
    }

    // 清理用户名（确保严格符合32字节长度，不足补0，不修改原输入）
    uint8_t clean_name[32] = {0}; // 临时缓冲区，默认全0（自动补0）
    // 安全拷贝32字节（无论原用户名是否以'\0'结尾，均不会越界）
    memcpy(clean_name, user_name, 32);

    // 调试模式：打印最终生效的注册参数（便于确认配置）
#ifdef DEBUG
    printf("注册参数：管理员=%s, 用户名=%s, 录入方向=0x%02X, 注册类型=%s, 允许重复=%s, 超时=%d秒\n",
           (admin == 0x01 ? "是" : "否"),
           clean_name,
           face_direction,
           (enroll_type == 0x00 ? "交互录入" : "单帧录入"),
           (enable_duplicate == 0x01 ? "允许" : "禁止"),
           timeout);
#endif

    // 构建注册命令帧：初始化全0，避免未赋值字节的随机值影响校验
    uint8_t frame[46] = {0}; // 总长度=帧头2 + 指令1 + 数据长度2 + 数据40 + BCC1 = 46字节

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
    frame[45] = calculate_bcc(frame, sizeof(frame));

    // 调试模式：打印发送的命令帧详情（便于排查帧结构问题）
#ifdef DEBUG
    printf("发送人脸注册帧（共%d字节）：", (uint16_t)sizeof(frame));
    for (size_t i = 0; i < sizeof(frame); ++i)
    {
        printf("%02X ", frame[i]);
    }
    printf("\n");
#endif

    // 实际串口发送逻辑：需根据硬件平台实现UART_Send函数
    // 示例：
    // if (!UART_Send(frame, sizeof(frame)))
    // {
    //     printf("注册失败：串口发送命令帧失败\n");
    //     return false;
    // }

    // 调试模式下，帧构建成功即返回true（实际需结合串口发送结果）
    return true;
}

// ========================== 测试函数 ==========================
/**
 * @brief 测试BCC校验和数据验证功能的正确性（验证工具函数逻辑）
 * @param 无输入参数
 * @return 无返回值（通过控制台打印测试结果）
 * @note  包含有效帧、无效帧头、无效BCC、长度不足4种典型场景测试
 */
void test_bcc_verification()
{
    printf("\n=== 开始BCC校验与数据验证测试 ===\n");

    // 测试1：有效的数据帧（帧头正确+BCC正确+长度足够）
    uint8_t valid_frame[] = {0xEF, 0xAA, 0x01, 0x02, 0x03, 0x00}; // 0x01^0x02^0x03=0x00（BCC正确）
    printf("测试1-有效数据帧: %s\n",
           verify_received_data(valid_frame, sizeof(valid_frame)) ? "通过" : "失败");

    // 测试2：无效帧头（前2字节与FRAME_HEADER不匹配）
    uint8_t invalid_header[] = {0xAA, 0xEF, 0x01, 0x02, 0x03, 0x00}; // 帧头错误
    printf("测试2-无效帧头: %s\n",
           !verify_received_data(invalid_header, sizeof(invalid_header)) ? "通过" : "失败");

    // 测试3：BCC错误（计算的BCC与帧尾存储的BCC不一致）
    uint8_t invalid_bcc[] = {0xEF, 0xAA, 0x01, 0x02, 0x03, 0x01}; // 正确BCC应为0x00，此处为0x01
    printf("测试3-无效BCC: %s\n",
           !verify_received_data(invalid_bcc, sizeof(invalid_bcc)) ? "通过" : "失败");

    // 测试4：长度不足（总字节数小于MIN_FRAME_LENGTH）
    uint8_t short_frame[] = {0xEF, 0xAA, 0x01, 0x01, 0x00}; // 仅5字节，小于最小6字节
    printf("测试4-长度不足: %s\n",
           !verify_received_data(short_frame, sizeof(short_frame)) ? "通过" : "失败");

	uint8_t empty_frame[64] = { 0 }; // 空数据帧
    printf("测试5-空数据帧: %s\n",
		!verify_received_data(empty_frame, 0) ? "通过" : "失败");

    printf("=== BCC校验与数据验证测试结束 ===\n\n");
}

/**
 * @brief 主函数（程序入口，用于测试人脸识别模块驱动功能）
 * @return int 程序退出码（0表示正常退出）
 * @note  可通过注释/解注释不同函数调用，测试注册、删除、验证等功能
 */
int main()
{
    printf("=== FM225 人脸识别模块驱动测试程序启动 ===\n");

    // 测试BCC校验功能
     test_bcc_verification();

    // 测试人脸注册功能（管理员用户，用户名为"Admin001"，朝上方向，单帧录入，禁止重复，超时10秒）
    uint8_t admin_name[32] = "Admin001"; // 剩下的会自动补0
    face_enroll(0x01, admin_name, FACE_DIRECTION_UP, 0x01, 0x00, 10);

    // 测试删除所有人脸功能
    face_delete_all();

    // 测试人脸验证功能（验证成功后断电，超时15秒）
    face_verify(0x01, 15);

    // 测试终止操作功能
    face_reset();

	// 测试删除指定用户功能（删除ID为5的用户）
	face_delete_user(5);

    printf("=== FM225 人脸识别模块驱动测试程序结束 ===\n");
    return 0;
}
