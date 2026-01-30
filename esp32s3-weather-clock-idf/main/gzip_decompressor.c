/**
 * @file gzip_decompressor.c
 * @brief GZIP解压缩功能实现文件
 * @author ESP-IDF Version
 */

#include "gzip_decompressor.h"
#include "esp_log.h"
#include "zlib.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "GZIP_DECOMPRESSOR";

/**
 * @brief 解压缩GZIP数据（使用zlib库）
 * 
 * @param compressed_data 压缩数据指针
 * @param compressed_size 压缩数据大小
 * @param decompressed_size 解压缩后的数据大小（输出参数）
 * 
 * @return 解压缩后的数据指针（需要调用者free），失败返回NULL
 */
char* gzip_decompress(const uint8_t *compressed_data, size_t compressed_size, size_t *decompressed_size)
{
    if (compressed_data == NULL || compressed_size == 0 || decompressed_size == NULL) {
        ESP_LOGE(TAG, "Invalid input parameters");
        return NULL;
    }

    ESP_LOGI(TAG, "Decompressing GZIP data, compressed size: %d bytes", compressed_size);

    /* 初始化zlib流 */
    z_stream d_stream = {0};
    d_stream.zalloc = Z_NULL;
    d_stream.zfree = Z_NULL;
    d_stream.opaque = Z_NULL;
    d_stream.next_in = (Bytef *)compressed_data;
    d_stream.avail_in = (uInt)compressed_size;

    /* 使用47作为windowBits参数，支持gzip格式（16 + 31） */
    int ret = inflateInit2(&d_stream, 47);
    if (ret != Z_OK) {
        ESP_LOGE(TAG, "inflateInit2 failed: %d", ret);
        return NULL;
    }

    /* 初始分配解压缩缓冲区（假设解压缩后的数据不超过压缩数据的10倍） */
    size_t initial_size = compressed_size * 10;
    if (initial_size < 4096) {
        initial_size = 4096;
    }

    char *decompressed_data = (char *)malloc(initial_size);
    if (decompressed_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for decompressed data");
        inflateEnd(&d_stream);
        return NULL;
    }

    d_stream.next_out = (Bytef *)decompressed_data;
    d_stream.avail_out = (uInt)initial_size;

    /* 执行解压缩 */
    ret = inflate(&d_stream, Z_FINISH);
    if (ret != Z_STREAM_END && ret != Z_OK) {
        ESP_LOGE(TAG, "inflate failed: %d, message: %s", ret, d_stream.msg ? d_stream.msg : "none");
        free(decompressed_data);
        inflateEnd(&d_stream);
        return NULL;
    }

    /* 获取解压缩后的数据大小 */
    *decompressed_size = d_stream.total_out;
    ESP_LOGI(TAG, "Decompression successful, decompressed size: %d bytes", *decompressed_size);

    /* 清理zlib流 */
    inflateEnd(&d_stream);

    /* 添加字符串结束符 */
    char *result = (char *)realloc(decompressed_data, *decompressed_size + 1);
    if (result == NULL) {
        ESP_LOGE(TAG, "Failed to reallocate memory for null terminator");
        free(decompressed_data);
        return NULL;
    }
    result[*decompressed_size] = '\0';

    return result;
}

/**
 * @brief 检查数据是否为GZIP格式
 */
bool gzip_is_compressed(const uint8_t *data, size_t size)
{
    if (data == NULL || size < 2) {
        return false;
    }
    /* GZIP魔术字节：0x1F 0x8B */
    return (data[0] == 0x1F && data[1] == 0x8B);
}
