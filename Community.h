/*
/// # Community-Created Open Audio Plugin Format Header File
///
/// ## About
/// This is a header file for a universal, free and open standard for all audio
/// plugins. 
///
/// ## Details
/// - Simple, Direct, Efficient, Portable
/// - Written in C
/// - No dependencies
/// - Single header library
/// - Small codebase

/// ## Features
/// - Planning
///
/// ## Usage
/// - Planning
///
/// ### Flags
/// ### Constants
/// ### Dependencies
/// ## Examples
*/

/*
 * ==============================================================
 *
 *                          CONSTANTS
 *
 * ===============================================================
 */
#define VERSION 0.0f

/*
 * ==============================================================
 *
 *                          HELPER
 *
 * ===============================================================
 */

/*
 * ===============================================================
 *
 *                          BASIC
 *
 * ===============================================================
 */

/* ============================================================================
 *
 *                                  API
 *
 * =========================================================================== 
 */


#pragma once

#include <stdint.h>

#ifdef __cplusplus__
extern "C" {
#endif

enum OpiEventType
{
    opiEventMidi,
    opiEventAutomation
};

// event-type support flags, for future extensibility
static const uint32_t   opiEventWantMidi        = 1<<0;
static const uint32_t   opiEventWantAutomation  = 1<<1;

// common fields for all events
struct OpiEvent
{
    uint32_t    type;   // OpiEventType
    uint32_t    delta;  // delta frames from start of block
};

// This is for simple MIDI commands like note-on/off, CC etc.
// Most current plugins (or frameworks) have to support this stuff
// anyway and higher-level note control could be added as separate
// event types at a later point in time (rather than bloating this).
struct OpiEventMidi
{
    uint32_t    type;   // OpiEventType
    uint32_t    delta;  // delta frames from start of block
    
    uint8_t     data[4];
};

struct OpiEventAutomation
{
    uint32_t    type;   // OpiEventType
    uint32_t    delta;  // delta frames from start of block

    uint32_t    paramIndex;
    float       targetValue;
    
    uint32_t    smoothFrames;   // frames to interpolate over (0 = snap)
};

// OpiTimeInfo flags: bitwise OR together
static const uint32_t   opiTimeTransportPlaying = 1<<0;
static const uint32_t   opiTimeSamplePosValid   = 1<<1;
static const uint32_t   opiTimePpqPosValid      = 1<<2;
static const uint32_t   opiTimeTempoValid       = 1<<3;

struct OpiTimeInfo
{
    uint32_t    infoSize;       // = sizeof(OpiTimeInfo) for future extensions
    uint32_t    flags;
    
    uint64_t    samplePos;      // sample position
    
    double      ppqPos;         // song position in quarter notes
    double      tempo;          // tempo in beats per minute
    
    // FIXME: time signature info?
};

// Prototype for the host and plugin dispatcher callbacks.
//
// While the general idea is to pass any parameters as an operation specific struct
// many operations required an index and promoting this to an explicit argument
// reduces the amount of special case structures that are required.
//
// For operations where the index doesn't make sense, it should be set to 0.
typedef intptr_t (*OpiCallback)(struct OpiPlugin *, int32_t op, int32_t idx, void *);

// The main plugin struct can be very simple; plugins can "derive" from this
// structure by adding whatever fields they need after the ones defined here.
struct OpiPlugin
{
    OpiCallback dispatchToHost;     // host dispatcher callback
    OpiCallback dispatchToPlugin;   // plugin dispatcher callback

    void *  ptrHost;                // host private pointer
};

// opcodes for dispatchToHost (void* parameter type in parenthesis)
//
enum OpiHostOps
{
    opiHostParamState,  // set parameter automation state, 1 = editing (uint32_t *)
    opiHostParamValue,  // send parameter automation, normalized [0,1] (float *)

    opiHostPatchChange, // notify the host that all past state should be flushed
    opiHostResizeEdit,  // resize editor, call once at init (struct OpiEditSize *)

    opiHostSetLatency,  // request that host refresh opiPlugGetLatency
};

struct OpiEditSize
{
    uint32_t    w;  // width in pixels (logical pixels for Retina, etc)
    uint32_t    h;  // height in pixels (logical pixels for Retina, etc)
};

// opcodes for dispatchToPlugin (parameters in parenthesis)
//
// The plugin should always return 0 for unknown or unimplemented opcodes
// and 1 for those it implements, unless otherwise indicated below.
//
// Operations marked [RT] are considered part of the "real-time context" and
// must not be called concurrently with each other, but ONLY opiPlugProcess
// is required to be "real-time safe" in the usual sense.
//
// Operations marked [UI] are considered part of the "interactive context" and
// must not be called concurrently with each other.
//
// Operations marked [ANY] can be called concurrently with everything else
// (eg. two opiSetParam calls to the same parameter concurrently are valid)
//
// Finally opiPlugDestroy must never be called while any other call active.
//
// opiPlugConfig is only valid while a plugin is in disable state (the default)
// and opiPlugProcess/opiPlugReset are only valid while a plugin is in enable state.
//
// opiPlugEnable implies a full reset, so opiPlugReset is only indended for use when
// the host wants the plugin to reset, but processing will continue immediately
//
enum OpiPlugOps
{
    opiPlugProcess,     // [RT] process (struct OpiProcessInfo)
    opiPlugDestroy,     // plugin should deallocate itself

    opiPlugNumInputs,   // [ANY] return the number of input busses
    opiPlugNumOutputs,  // [ANY] return the number of output busses

    opiPlugMaxChannels, // [ANY] return the maximum  number of channels (per bus)
    opiPlugInEventMask, // [ANY] return mask of event-types the plugin wants
    opiPlugOutEventMask,// [ANY] return mask of event-types the plugin will generate

    opiPlugGetLatency,  // [ANY] return current latency
    
    opiPlugConfig,      // [RT] configure processing parameters (struct OpiConfig)
    opiPlugReset,       // [RT] reset plugin state, but continue processing
    
    opiPlugEnable,      // [RT] start processing
    opiPlugDisable,     // [RT] stop processing

    opiPlugOpenEdit,    // [UI] open editor (platform HWND, NSView, etc)
    opiPlugCloseEdit,   // [UI] close editor

    opiPlugSaveChunk,   // [UI] save state into a chunk (struct OpiChunk*)
    opiPlugLoadChunk,   // [UI] load state from a chunk (struct OpiChunk*)

    opiPlugNumParam,    // [ANY] return number of parameters
    opiPlugGetParam,    // [ANY] get the parameter value (float *)
    opiPlugSetParam,    // [ANY] set the parameter value (float *)

    opiPlugGetParamName,    // [UI] get the name of the parameter (struct OpiString *)
    
    opiPlugValueToString,   // [UI] convert value to string (struct OpiParamString *)
    opiPlugStringToValue,   // [UI] convert string to value (struct OpiParamString *)

    opiPlugGetPatchName,    // [UI] get current patch name (struct OpiString *)
    opiPlugSetPatchName,    // [UI] set current patch (struct OpiString *)
};

// each logical bus is a collection of channels
// the number of channels must be set by calling opiPlugConfig (see below)
//
// All the buffers are allocated by the host.
//
// If a channel is completely silent (all zeroes) then a bit in the silenceMask
// can be set (by host for inputs and plugin for outputs) to indicate that
// processing this channel is not necessary (ie. soft-bypass is possible).
//
// Note that the buffer must still be cleared (ie. the flag is just a hint).
//
// The host can also set the silenceMask for outputs before a process call to
// indicate that the contents are already zero and the plugin need not clear
// them explicitly if it only wants to output silence.
//
// RATIONALE: The ability (of both host and plugin) to skip completely zero
// buffers gives most of the performance benefits of other soft-bypass schemes,
// while still leaving the plugin in full control over it's own processing and
// the additional complexity is very minor (eg. the plugin can just always set
// silenceMask to zero for all the outputs if it doesn't care about any of this).
//
struct OpiBusChannels
{
    uint64_t    silenceMask;    // bitmask of fully silent channels (1 = silent)
    float       *channels[];
};

// The host must provide the events in order sorted by delta-time and in case of
// identical delta times, by logical ordering: the event placed first in the queue
// is considered to happen "first" even though both happen during the same frame.
//
// RATIONALE: We require the list of events to be sorted, because it is typically
// much easier to maintain such a list in sorted order (or merge multiple sorted
// lists), rather than to explicitly sort when no such guarantees are provided.
//
// If plugin wants to send outbound events, it should set the outEvents pointer
// and the number of events to non-zero values.
//
struct OpiProcessInfo
{
    uint32_t    processInfoSize; // = sizeof(OpiProcessInfo) for future extensions
    uint32_t    nFrames;
    
    struct OpiTimeInfo * timeInfo;  // pointer to time info

    OpiBusChannels  *inputs;        // pointer to array of OpiBusChannels
    OpiBusChannels  *outputs;       // pointer to array of OpiBusChannels

    OpiEvent        **inEvents;     // pointer to an array of pointers to events
    OpiEvent        **outEvents;    // pointer to an array of pointers to events
    
    uint32_t        nInEvents;      // number of input events
    uint32_t        nOutEvents;     // number of output events
};

struct OpiBusConfig
{
    uint32_t    nChannels;      // number of channels (0 = disconnected)
};

// the plugin must be in disabled (initial) state when opiPlugConfig is called
//
// it is NOT required that a plugin supports any given channel configuration,
// but at bare minimum all plugins should support nChannels == 2 for each bus
//
// if the plugin returns 0, then the host must retry with another configuration
//
// RATIONALE: Since different busses can potentially have dependencies on each
// other in terms of number of channels, yet the plugin might still support a
// large number of different configurations (only few of which are likely to be
// relevant in any given situation) it seems to make the most sense to just let
// the host try the applicable configurations one by one in order of preference.
//
struct OpiConfig
{
    uint32_t    configSize; // = sizeof(OpiConfig) for future extensions
    
    uint32_t    blocksize;
    float       samplerate;

    uint32_t    busConfigSize;  // = sizeof(OpiBusConfig) for future extensions

    OpiBusConfig *inBusChannels;    // array of bus configurations
    OpiBusConfig *outBusChannels;   // array of bus configurations
};

// This is passed by host to both opiPlugSaveChunk and opiPlugLoadChunk
// but for opiPlugSaveChunk the plugin sets the pointer and the data size and
// the buffer remains valid until next [UI] context call to dispatcher.
struct OpiChunk
{
    void *      data;
    uint32_t    size;
};

// This is used for operations that get or set strings. The pointer and size
// are always filled by the party that provides the contents and must remain
// valid until the next [UI) context dispatcher call.
// All strings must use UTF-8 encoding.
struct OpiString
{
    char *      data;
    uint32_t    size;
};

// This is used for converting between parameter values and strings.
// Buffer must remain valid until next [UI] context dispatcher call.
//
// RATIONALE: There are various situations where the host might want to convert
// between parameter values and strings, eg. when editing automation curves
// without modifying the actual parameter state of the plugin. As such, it makes
// the most sense to simply specify such conversions as operations separate from
// everything else.
struct OpiParamString
{
    char *      data;
    uint32_t    size;
    float       value;
};

// Plugin entry point
#ifndef DLLEXPORT
# ifdef _WIN32
#  define DLLEXPORT __declspec(dllexport)
# else
#  define DLLEXPORT __attribute__((visibility("default")))
# endif
#endif

DLLEXPORT OpiPlugin * OpiPluginEntrypoint(OpiCallback hostCallback)
{
    return 0;
}

#ifdef __cplusplus__
} // extern "C"
#endif


/*
/// ## License
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~none
///    ------------------------------------------------------------------------------
///    This software is available under 2 licenses -- choose whichever you prefer.
///    ------------------------------------------------------------------------------
///    ALTERNATIVE A - MIT License
///    Permission is hereby granted, free of charge, to any person obtaining a copy of
///    this software and associated documentation files (the "Software"), to deal in
///    the Software without restriction, including without limitation the rights to
///    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
///    of the Software, and to permit persons to whom the Software is furnished to do
///    so, subject to the following conditions:
///    The above copyright notice and this permission notice shall be included in all
///    copies or substantial portions of the Software.
///    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
///    SOFTWARE.
///    ------------------------------------------------------------------------------
///    ALTERNATIVE B - Public Domain (www.unlicense.org)
///    This is free and unencumbered software released into the public domain.
///    Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
///    software, either in source code form or as a compiled binary, for any purpose,
///    commercial or non-commercial, and by any means.
///    In jurisdictions that recognize copyright laws, the author or authors of this
///    software dedicate any and all copyright interest in the software to the public
///    domain. We make this dedication for the benefit of the public at large and to
///    the detriment of our heirs and successors. We intend this dedication to be an
///    overt act of relinquishment in perpetuity of all present and future rights to
///    this software under copyright law.
///    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///    AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
///    ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
///    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
///    ------------------------------------------------------------------------------
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// ## Changelog
/// [date][x.yy.zz]-[description]
/// -[date]: date on which the change has been pushed
/// -[x.yy.zz]: Numerical version string representation. Each version number on the right
///             resets back to zero if version on the left is incremented.
///    - [x]: Major version with API and library breaking changes
///    - [yy]: Minor version with non-breaking API and library changes
///    - [zz]: Bug fix version with no direct changes to API
///
/// - 2020/07/08 (0.00.1) - Initial Prototype
/// - 2020/07/07 (0.00.0) - Initial

/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// ## Credits
/// [Name], [Role and Contributions]
/// 
/// mystran, Developer, Structure and Code Contributor
/// Sabrina H (KVR-Vertion), Audio Plugin Enthusiast for cheering on the devs.
/// 
*/
