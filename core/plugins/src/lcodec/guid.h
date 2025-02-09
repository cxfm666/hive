#pragma once

#include <stdlib.h>
#include <time.h>
#include <math.h>

//i  - group，8位，(0~255)
//g  - index，12位  (0~4095)
//t  - gtype, 4位   (0~15) 
//s  - 序号，  9位  (0~511)
//ts - 时间戳，30位
//共63位，防止出现负数

namespace lcodec {

    const uint32_t GROUP_BITS   = 8;
    const uint32_t INDEX_BITS   = 12;
    const uint32_t TYPE_BITS    = 4;
    const uint32_t SNUM_BITS    = 9; 

    const uint32_t LETTER_LEN   = 12;
    const uint32_t LETTER_SIZE  = 62;

    //基准时钟：2022-10-01 08:00:00
    const uint32_t BASE_TIME    = 1664582400;

    const int32_t MAX_GROUP    = ((1 << GROUP_BITS) - 1);      //256 - 1
    const int32_t MAX_INDEX    = ((1 << INDEX_BITS) - 1);      //4096 - 1
    const int32_t MAX_TYPE     = ((1 << TYPE_BITS) - 1);       //16   - 1
    const int32_t MAX_SNUM     = ((1 << SNUM_BITS) - 1);       //512  - 1
    const int32_t MAX_TIME     = ((1 << 30) - 1);              //30   - 1

    //每一group独享一个id生成种子
    static thread_local time_t last_time = 0;
    static thread_local size_t serial_inedx_table[(1 << GROUP_BITS)] = { 0 };

    static const char letter[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    static uint64_t guid_new(uint32_t group, uint32_t index,uint32_t gtype){
        if (group == 0) {
            group = rand();
        }
        if (index == 0) {
            index = rand();
        }
        if (gtype == 0) {
            gtype = rand();
        }
        group %= MAX_GROUP;
        index %= MAX_INDEX;
        gtype %= MAX_TYPE;
        time_t now_time;
        time(&now_time);
        size_t serial_index = 0;
        if (now_time > last_time) {
            serial_inedx_table[group] = 0;
            last_time = now_time;
        }
        else {
            serial_index = ++serial_inedx_table[group];
            //种子溢出以后，时钟往前推
            if (serial_index >= MAX_SNUM) {
                serial_inedx_table[group] = 0;
                last_time++;
                serial_index = 0;
            }
        }
        return ((last_time - BASE_TIME) << (SNUM_BITS + TYPE_BITS + GROUP_BITS + INDEX_BITS)) |
                (serial_index << (TYPE_BITS + GROUP_BITS + INDEX_BITS)) | 
                (gtype << (GROUP_BITS + INDEX_BITS)) |
                (index << GROUP_BITS) | group;
    }

    static int guid_encode(lua_State* L) {
        char tmp[LETTER_LEN];
        memset(tmp, 0, LETTER_LEN);
        uint64_t val = (lua_gettop(L) > 0) ? lua_tointeger(L, 1) : guid_new(0, 0,0);
        for (int i = 0; i < LETTER_LEN - 1; ++i) {
            tmp[i] = letter[val % LETTER_SIZE];
            val /= LETTER_SIZE;
            if (val == 0) break;
        }
        lua_pushstring(L, tmp);
        return 1;
    }

    static int find_index(char val) {
        if (val >= 97) return val - 61;
        if (val >= 65) return val - 55;
        return val - 48;
    }

    static uint64_t guid_decode(std::string sval){
        uint64_t val = 0;
        size_t len = sval.size();
        const char* cval = sval.c_str();
        for (int i = 0; i < len; ++i) {
            val += uint64_t(find_index(cval[i]) * pow(LETTER_SIZE, i));
        }
        return val;
    }

    size_t format_guid(lua_State* L) {
        int type = lua_type(L, 1);
        if (type == LUA_TSTRING) {
            char* chEnd = NULL;
            const char* sguid = lua_tostring(L, 1);
            return strtoull(sguid, &chEnd, 16);
        } else if (type == LUA_TNUMBER) {
            return lua_tointeger(L, 1);
        }
        //luaL_error(L, "guid only support number or string!");        
        return 0;
    }

    static int guid_group(lua_State* L) {
        size_t guid = format_guid(L);
        lua_pushinteger(L, guid & MAX_GROUP);
        return 1;
    }

    static int guid_index(lua_State* L) {
        size_t guid = format_guid(L);
        lua_pushinteger(L, (guid >> GROUP_BITS) & MAX_INDEX);
        return 1;
    }

    static int guid_type(lua_State* L) {
        size_t guid = format_guid(L);
        lua_pushinteger(L, (guid >> (GROUP_BITS + INDEX_BITS)) & MAX_TYPE);
        return 1;
    }

    static int guid_serial(lua_State* L) {
        size_t guid = format_guid(L);
        lua_pushinteger(L, (guid >> (GROUP_BITS + INDEX_BITS + TYPE_BITS)) & MAX_SNUM);
        return 1;
    }

    static int guid_time(lua_State* L) {
        size_t guid = format_guid(L);
        size_t time = (guid >> (GROUP_BITS + INDEX_BITS + TYPE_BITS + SNUM_BITS)) & MAX_TIME;
        lua_pushinteger(L, time + BASE_TIME);
        return 1;
    }

    static int guid_source(lua_State* L) {
        size_t guid = format_guid(L);
        lua_pushinteger(L, guid & MAX_GROUP);
        lua_pushinteger(L, (guid >> GROUP_BITS) & MAX_INDEX);
        lua_pushinteger(L, (guid >> (GROUP_BITS + INDEX_BITS)) & MAX_TYPE);
        lua_pushinteger(L, ((guid >> (GROUP_BITS + INDEX_BITS + TYPE_BITS + SNUM_BITS)) & MAX_TIME) + BASE_TIME);
        lua_pushinteger(L, (guid >> (GROUP_BITS + INDEX_BITS + TYPE_BITS)) & MAX_SNUM);
        return 5;
    }
}
