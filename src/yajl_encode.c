/*
 * Copyright 2007-2009, Lloyd Hilaiel.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 
 *  3. Neither the name of Lloyd Hilaiel nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

#include "yajl_encode.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void CharToHex(unsigned char c, char * hexBuf)
{
    const char * hexchar = "0123456789abcdef";
    hexBuf[0] = hexchar[c >> 4];
    hexBuf[1] = hexchar[c & 0x0F];
}

void
yajl_string_encode(yajl_buf buf, const unsigned char * str,
                   unsigned int len, int encode_utf8)
{
    unsigned int beg = 0;
    unsigned int end = 0;    
    char hexBuf[7] = "\\u0000";
    char rawBuf[5] = "\0\0\0\0";

    while (end < len) {
		unsigned int inc = 1;
        const char * escaped = NULL;
        switch (str[end]) {
            case '\r': escaped = "\\r"; break;
            case '\n': escaped = "\\n"; break;
            case '\\': escaped = "\\\\"; break;
            /* case '/': escaped = "\\/"; break; */
            case '"': escaped = "\\\""; break;
            case '\f': escaped = "\\f"; break;
            case '\b': escaped = "\\b"; break;
            case '\t': escaped = "\\t"; break;
            default:
                // utf8: U-00000080 - U-000007FF: 110xxxxx 10xxxxxx
                    if (((str[end] & 0xE0) == 0xC0) && 
                        (end+1<len) && 
                        ((str[end+1] & 0xC0) == 0x80)) 
                    {
                        if (encode_utf8) 
                        {
                            unsigned int cp = (str[end] & 0x1F) << 6;
                            cp |= str[end+1] & 0x3F;
                            inc = 2;
                            CharToHex((cp >> 8) & 0xFF, hexBuf + 2);
                            CharToHex(cp & 0xFF, hexBuf + 4);
                            escaped = hexBuf;
                        } else {
                            rawBuf[0] = str[end];
                            rawBuf[1] = str[end+1];
                            rawBuf[2] = 0;
                            rawBuf[3] = 0;
                            inc = 2;
                            escaped = rawBuf;
                        }
                        break;
                    }
                    // utf8: U-00000800 - U-0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx
                    else 
                    if (((str[end] & 0xF0) == 0xE0) &&
                        (end+2<len) &&
                        ((str[end+1] & 0xC0) == 0x80) &&
                        ((str[end+2] & 0xC0) == 0x80))
                    {
                        if (encode_utf8) 
                        {
                            unsigned int cp = (str[end] & 0x0F) << 12;
                            cp |= (str[end+1] & 0x3F) << 6;
                            cp |= (str[end+2] & 0x3F);
                            inc = 3;
                            CharToHex((cp >> 8) & 0xFF, hexBuf + 2);
                            CharToHex(cp & 0xFF, hexBuf + 4);
                            escaped = hexBuf;
                        } else 
                        {
                            rawBuf[0] = str[end];
                            rawBuf[1] = str[end+1];
                            rawBuf[2] = str[end+2];
                            rawBuf[3] = 0;
                            inc = 3;
                            escaped = rawBuf;
                        }
                        break;
                    }

                    if (!(str[end] >= ' ' && str[end] < 127)) { // don't know why isprint doesn't work!!! 
                        // convert to \uXXXX in spite of encode_utf8 or not
                        hexBuf[2] = '0';
                        hexBuf[3] = '0';
                        CharToHex(str[end], hexBuf + 4);
                        escaped = hexBuf;
                    }
                break;
        }
        if (escaped != NULL) {
            yajl_buf_append(buf, str + beg, end - beg);
            yajl_buf_append(buf, escaped, strlen(escaped));
            end += inc;
            beg = end;
        } else {
            end += inc;
        }
    }
    yajl_buf_append(buf, str + beg, end - beg);
}

static void hexToDigit(unsigned int * val, const unsigned char * hex)
{
    unsigned int i;
    for (i=0;i<4;i++) {
        unsigned char c = hex[i];
        if (c >= 'A') c = (c & ~0x20) - 7;
        c -= '0';
        assert(!(c & 0xF0));
        *val = (*val << 4) | c;
    }
}

static void Utf32toUtf8(unsigned int codepoint, char * utf8Buf) 
{
    if (codepoint < 0x80) {
        utf8Buf[0] = (char) codepoint;
        utf8Buf[1] = 0;
    } else if (codepoint < 0x0800) {
        utf8Buf[0] = (char) ((codepoint >> 6) | 0xC0);
        utf8Buf[1] = (char) ((codepoint & 0x3F) | 0x80);
        utf8Buf[2] = 0;
    } else if (codepoint < 0x10000) {
        utf8Buf[0] = (char) ((codepoint >> 12) | 0xE0);
        utf8Buf[1] = (char) (((codepoint >> 6) & 0x3F) | 0x80);
        utf8Buf[2] = (char) ((codepoint & 0x3F) | 0x80);
        utf8Buf[3] = 0;
    } else if (codepoint < 0x200000) {
        utf8Buf[0] =(char)((codepoint >> 18) | 0xF0);
        utf8Buf[1] =(char)(((codepoint >> 12) & 0x3F) | 0x80);
        utf8Buf[2] =(char)(((codepoint >> 6) & 0x3F) | 0x80);
        utf8Buf[3] =(char)((codepoint & 0x3F) | 0x80);
        utf8Buf[4] = 0;
    } else {
        utf8Buf[0] = '?';
        utf8Buf[1] = 0;
    }
}

void yajl_string_decode(yajl_buf buf, const unsigned char * str,
                        unsigned int len)
{
    unsigned int beg = 0;
    unsigned int end = 0;    

    while (end < len) {
        if (str[end] == '\\') {
            char utf8Buf[5];
            const char * unescaped = "?";
            yajl_buf_append(buf, str + beg, end - beg);
            switch (str[++end]) {
                case 'r': unescaped = "\r"; break;
                case 'n': unescaped = "\n"; break;
                case '\\': unescaped = "\\"; break;
                case '/': unescaped = "/"; break;
                case '"': unescaped = "\""; break;
                case 'f': unescaped = "\f"; break;
                case 'b': unescaped = "\b"; break;
                case 't': unescaped = "\t"; break;
                case 'u': {
                    unsigned int codepoint = 0;
                    hexToDigit(&codepoint, str + ++end);
                    end+=3;
                    /* check if this is a surrogate */
                    if ((codepoint & 0xFC00) == 0xD800) {
                        end++;
                        if (str[end] == '\\' && str[end + 1] == 'u') {
                            unsigned int surrogate = 0;
                            hexToDigit(&surrogate, str + end + 2);
                            codepoint =
                                (((codepoint & 0x3F) << 10) | 
                                 ((((codepoint >> 6) & 0xF) + 1) << 16) | 
                                 (surrogate & 0x3FF));
                            end += 5;
                        } else {
                            unescaped = "?";
                            break;
                        }
                    }
                    
                    Utf32toUtf8(codepoint, utf8Buf);
                    unescaped = utf8Buf;
                    break;
                }
                default:
                    assert("this should never happen" == NULL);
            }
            yajl_buf_append(buf, unescaped, strlen(unescaped));
            beg = ++end;
        } else {
            end++;
        }
    }
    yajl_buf_append(buf, str + beg, end - beg);
}
