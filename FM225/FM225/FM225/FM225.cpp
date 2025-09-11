#include <iostream>
#include <cstdint>

// 类型定义使用typedef更规范，避免宏定义的副作用
typedef bool esp_err_t;
#define ESP_OK true
#define ESP_FAIL false
#define DEBUG

// 宏定义常量全大写，保持命名一致性
#define BCC_START_INDEX     2       // 校验码计算起始索引
#define MIN_FRAME_LENGTH    6       // 最小帧长度（字节）

// 帧头常量定义，增加constexpr提升编译期检查
constexpr uint8_t FRAME_HEADER[2] = { 0xEF, 0xAA };  // 帧头固定值（2字节），模块通信起始标识
constexpr const char* TAG = "FM225";               // 日志标签

// 日志宏定义优化：空宏使用do-while(0)避免编译问题
#ifdef DEBUG
#define ESP_LOGE(tag, fmt, ...) printf("ERROR [%s]: " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) printf("INFO  [%s]: " fmt "\n", tag, ##__VA_ARGS__)
#else
#define ESP_LOGE(tag, fmt, ...) do {} while(0)
#define ESP_LOGI(tag, fmt, ...) do {} while(0)
#endif

// ========================== 通用工具函数 ==========================
/**
 * @brief 计算数据帧的BCC校验码（异或校验算法）
 * @param frame_data 数据帧缓冲区（需非空）
 * @param frame_len  数据帧总长度
 * @return uint8_t   计算得到的BCC校验码，参数无效返回0
 */
static uint8_t calculate_bcc(const uint8_t* frame_data, uint16_t frame_len)
{
	uint8_t bcc = 0;  // 异或校验初始值为0

	// 异或校验计算：从起始索引到校验码前一位
	for (uint16_t i = BCC_START_INDEX; i < frame_len - 1; ++i)
	{
		bcc ^= frame_data[i];  // 依次与每个字节进行异或运算
	}

	return bcc;
}

/**
 * @brief 校验指纹模块接收数据的有效性（验证帧结构和BCC校验码）
 * @param recv_data 接收的数据包缓冲区（需非空）
 * @param data_len  实际接收的字节数
 * @return esp_err_t 校验结果：ESP_OK=有效，ESP_FAIL=无效
 */
static esp_err_t verify_received_data(const uint8_t* recv_data, uint16_t data_len)
{
	// 基础合法性检查
	if (recv_data == nullptr || data_len < MIN_FRAME_LENGTH)
	{
#ifdef DEBUG
		ESP_LOGE(TAG, "校验失败：数据为空或长度不足（最小需%d字节，当前%d字节）",
			MIN_FRAME_LENGTH, data_len);
#endif
		return ESP_FAIL;
	}

	// 验证帧头（前2字节）
	if (recv_data[0] != FRAME_HEADER[0] || recv_data[1] != FRAME_HEADER[1])
	{
#ifdef DEBUG
		ESP_LOGE(TAG, "校验失败：帧头不匹配（期望%02X%02X，实际%02X%02X）",
			FRAME_HEADER[0], FRAME_HEADER[1], recv_data[0], recv_data[1]);
#endif
		return ESP_FAIL;
	}

	// 验证BCC校验码（最后1个字节）
	const uint8_t received_bcc = recv_data[data_len - 1];
	const uint8_t calculated_bcc = calculate_bcc(recv_data, data_len);

	if (calculated_bcc != received_bcc)
	{
#ifdef DEBUG
		ESP_LOGE(TAG, "校验失败：BCC不匹配（期望0x%02X，实际0x%02X）",
			calculated_bcc, received_bcc);
#endif
		return ESP_FAIL;
	}

#ifdef DEBUG
	ESP_LOGI(TAG, "校验成功：数据有效");
#endif
	return ESP_OK;
}

// ========================== 功能函数 ==========================
/**
 * @brief 指纹模块自动注册函数
 * @param enroll_id 注册ID（1-100）
 * @note 函数实现待补充
 */
static esp_err_t auto_enroll(uint8_t enroll_id) 
{
	// 参数合法性检查
	if (enroll_id >= 100)
	{
		// ID超出范围
#ifdef DEBUG
		ESP_LOGE(TAG, "注册失败：ID超出范围（需0-99，当前%d）", enroll_id);
#endif
		return ESP_FAIL;
	}
}

// ========================== 测试函数 ==========================
/**
 * @brief 测试BCC校验和数据验证功能的正确性
 */
void test_bcc_verification()
{
	// 测试1：有效的数据帧（BCC正确）
	uint8_t valid_frame[] = { 0xEF, 0xAA, 0x01, 0x02, 0x03, 0x00 };  // 0x01^0x02^0x03=0x00
	ESP_LOGI(TAG, "测试有效数据帧: %s",
		verify_received_data(valid_frame, sizeof(valid_frame)) ? "通过" : "失败");

	// 测试2：无效帧头
	uint8_t invalid_header[] = { 0xAA, 0xEF, 0x01, 0x02, 0x03, 0x00 };
	ESP_LOGI(TAG, "测试无效帧头: %s",
		!verify_received_data(invalid_header, sizeof(invalid_header)) ? "通过" : "失败");

	// 测试3：BCC错误
	uint8_t invalid_bcc[] = { 0xEF, 0xAA, 0x01, 0x02, 0x03, 0x01 };  // 正确BCC应为0x00
	ESP_LOGI(TAG, "测试无效BCC: %s",
		!verify_received_data(invalid_bcc, sizeof(invalid_bcc)) ? "通过" : "失败");

	// 测试4：长度不足
	uint8_t short_frame[] = { 0xEF, 0xAA, 0x01, 0x01, 0x00 };  // 修正多余逗号
	ESP_LOGI(TAG, "测试长度不足: %s",
		!verify_received_data(short_frame, sizeof(short_frame)) ? "通过" : "失败");
}

int main()
{
	ESP_LOGI(TAG, "FM225 BCC校验测试程序启动");
	test_bcc_verification();
	return 0;
}
