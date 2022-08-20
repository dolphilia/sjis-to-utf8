#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include "table_sjis.h"
#include "table_utf8.h"

void println(const char* str, ...) {
    va_list args;
    va_start(args, str);
    vprintf(str, args);
    printf("\n");
    va_end(args);
}

FILE* file_open_read(char* filename) {
    FILE* file_ptr = fopen(filename, "r");
    if (file_ptr == NULL) {
        println("ファイルオープンに失敗しました"); // ファイルオープンエラーの処理
        exit(0);
    }
    return file_ptr;
}

void file_seek_end(FILE* file_ptr) {
    bool is_error = fseek(file_ptr, 0, SEEK_END);
    if (is_error) { // ファイルサイズの取得
        println("fseek(fp, 0, SEEK_END)に失敗しました"); // fseek エラーの処理
        exit(0);
    }
}

void file_seek_set(FILE* file_ptr) {
    bool is_error = fseek(file_ptr, 0L, SEEK_SET);
    if (is_error) {
        println("fseek(fp, 0, SEEK_SET)に失敗しました"); // fseek エラーの処理
        exit(0);
    }
}

int32_t get_file_length(FILE* file_ptr) {
    int32_t length = 0;
    file_seek_end(file_ptr);
    length = ftell(file_ptr);
    file_seek_set(file_ptr);
    return length;
}

uint8_t* memory_alloc(int32_t size) {
    uint8_t* memory = (unsigned char*)malloc(size);
    if (memory == NULL) {
        println("メモリ割り当てに失敗しました"); // メモリ割り当てエラーの処理
        exit(0);
    }
    return memory;
}

void file_read(FILE* file_ptr, uint8_t* data, int32_t size) {
    bool is_error = fread(data, 1, size, file_ptr) < size;
    if (is_error) {
        println("ファイルの読み取りに失敗しました"); // ファイル読み取りエラーの処理
        exit(0);
    }
}

uint8_t* get_file_raw_data(char* filename, int32_t* size) {
    FILE *file_ptr;
    uint8_t* raw_data;
    file_ptr = file_open_read(filename);
    *size = get_file_length(file_ptr);
    raw_data = memory_alloc(*size); // ファイル全体を格納するメモリを割り当てる 
    file_read(file_ptr, raw_data, *size);
    fclose(file_ptr);
    return raw_data;
}

// 1バイトのutf8コードをchar配列に格納する
void utf8_1byte_to_char(char* str,uint32_t utf8_code) {
    uint32_t tmp_utf8 = utf8_code;
    str[1] = '\0';
    str[0] = (uint8_t)tmp_utf8;
}

// 2バイトのutf8コードをchar配列に格納する
void utf8_2byte_to_char(char* str, uint32_t utf8_code) {
    uint32_t tmp_utf8 = utf8_code;
    str[2] = '\0';
    str[1] = (uint8_t)tmp_utf8;
    tmp_utf8 = tmp_utf8 >> 8;
    str[0] = (uint8_t)tmp_utf8;
}

// 3バイトのutf8コードをchar配列に格納する
void utf8_3byte_to_char(char* str, uint32_t utf8_code) {
    uint32_t tmp_utf8 = utf8_code;
    str[3] = '\0';
    str[2] = (uint8_t)tmp_utf8;
    tmp_utf8 = tmp_utf8 >> 8;
    str[1] = (uint8_t)tmp_utf8;
    tmp_utf8 = tmp_utf8 >> 8;
    str[0] = (uint8_t)tmp_utf8;
}

void utf8_to_char(char* str, uint32_t utf8_code) {
    if (utf8_code < 0x80) {
        utf8_1byte_to_char(str, utf8_code);
    } else if (utf8_code <= 0xF1A0) {
        utf8_2byte_to_char(str, utf8_code);
    } else {
        utf8_3byte_to_char(str, utf8_code);
    }
}

int search_sjis_index(uint32_t* table, uint32_t sjis_code) { //指定したSJISコードにマッチする位置を返す
    int table_len = sizeof(table_sjis) / sizeof(uint32_t);
    for (int i = 0; i < table_len; i++) {
        if (sjis_code == table_sjis[i]) {
            return i;
        }
    }
    //printf("\n%d\n",sjis_code);
    return 0;
}

void print_utf8_from_sjis(uint32_t* table, uint32_t sjis_code) {
    int index = search_sjis_index(table, sjis_code);
    if (index) {
        char str[4] = "";
        utf8_to_char(str, table_utf8[index]);
        printf("%s", str);
    } else { // マッチしなかった場合は変換せずに格納する
        char str[4] = "";
        utf8_to_char(str, sjis_code);
        printf("%s", str);
    }
}

uint32_t get_2byte_from_raw_data(uint8_t* data, int offset) {
    uint32_t code = 0;
    code += data[offset + 0]; // ２バイト分流し込む
    code = code << 8;
    code += data[offset + 1];
    return code;
}

void print_sjis_data(uint8_t* data, int32_t size) {
    for(int offset = 0; offset < size; ) {
        if ((data[offset] < 0x80) ||
            (data[offset] >= 0xA1 && data[offset] <= 0xDF)) { // 1バイト目が0x81未満なら１バイト文字
            uint32_t sjis_code = 0;
            sjis_code += data[offset]; // ２バイト分流し込む
            offset += 1;
            print_utf8_from_sjis(table_sjis, sjis_code);
        } else { // 1バイト目が0x81以上なら２バイト文字
            uint32_t sjis_code = 0;
            sjis_code = get_2byte_from_raw_data(data, offset);
            offset += 2;
            print_utf8_from_sjis(table_sjis, sjis_code);
        }
    }
}

int main() {
    uint8_t* data;
    int32_t size;
    data = get_file_raw_data("sjis.txt", &size);
    print_sjis_data(data, size);
    free(data);
    return 0;
}