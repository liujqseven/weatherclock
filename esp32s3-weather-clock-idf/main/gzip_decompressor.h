/**
 * @file gzip_decompressor.h
 * @brief GZIP解压缩功能头文件
 * @author ESP-IDF Version
 */

#ifndef GZIP_DECOMPRESSOR_H
#define GZIP_DECOMPRESSOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief 解压缩GZIP数据（使用zlib库）
 * 
 * @param compressed_data 压缩数据指针
 * @param compressed_size 压缩数据大小
 * @param decompressed_size 解压缩后的数据大小（输出参数）
 * 
 * @return 解压缩后的数据指针（需要调用者free），失败返回NULL
 */
char* gzip_decompress(const uint8_t *compressed_data, size_t compressed_size, size_t *decompressed_size);

/**
 * @brief 检查数据是否为GZIP格式
 * 
 * @param data 数据指针
 * @param size 数据大小
 * 
 * @return 是GZIP格式返回true，否则返回false
 */
bool gzip_is_compressed(const uint8_t *data, size_t size);

#endif /* GZIP_DECOMPRESSOR_H */
