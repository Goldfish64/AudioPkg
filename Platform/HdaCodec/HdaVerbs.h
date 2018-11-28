/*
 * File: HdaVerbs.h
 *
 * Copyright (c) 2018 John Davis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _EFI_HDA_VERBS_H_
#define _EFI_HDA_VERBS_H_

// Macros for building verbs.
#define HDA_CODEC_VERB_4BIT(Verb, Payload)  ((((UINT32)Verb) << 16) | (Payload & 0xFFFF))
#define HDA_CODEC_VERB_12BIT(Verb, Payload) ((((UINT32)Verb) << 8) | (Payload & 0xFF))

//
// Verbs
//
#define HDA_VERB_GET_PARAMETER 0xF00 // Get Parameter: Payload - parameter ID, Response: parameter value.

//
// Parameters.
//
#define HDA_PARAMETER_VENDOR_ID     0x00
#define HDA_PARAMETER_REVISION_ID   0x02

#endif
